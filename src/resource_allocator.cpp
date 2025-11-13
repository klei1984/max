/* Copyright (c) 2021 M.A.X. Port Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "resource_allocator.hpp"

#include "access.hpp"
#include "inifile.hpp"
#include "randomizer.hpp"
#include "resource_manager.hpp"

int32_t ResourceRange::GetValue() const {
    return ((Randomizer_Generate(max - min + 1) + min) + (Randomizer_Generate(max - min + 1) + min) + 1) / 2;
}

ResourceAllocator::ResourceAllocator(uint16_t material_type) : material_type(material_type) {
    max_resources = ini_get_setting(INI_MAX_RESOURCES);
    mixed_resource_seperation = ini_get_setting(INI_MIXED_RESOURCE_SEPERATION);
    mixed_resource_seperation_variance = mixed_resource_seperation / 5;

    switch (material_type) {
        case CARGO_MATERIALS: {
            m_point.x = 0;
            m_point.y = 0;
            normal = ResourceRange(ini_get_setting(INI_RAW_NORMAL_LOW), ini_get_setting(INI_RAW_NORMAL_HIGH));
            concentrate =
                ResourceRange(ini_get_setting(INI_RAW_CONCENTRATE_LOW), ini_get_setting(INI_RAW_CONCENTRATE_HIGH));
            concentrate_seperation = ini_get_setting(INI_RAW_CONCENTRATE_SEPERATION);
            concentrate_variance = concentrate_seperation / 5;
            concentrate_diffusion = ini_get_setting(INI_RAW_CONCENTRATE_DIFFUSION);
        } break;

        case CARGO_GOLD: {
            m_point.x = 1;
            m_point.y = 0;
            normal = ResourceRange(ini_get_setting(INI_GOLD_NORMAL_LOW), ini_get_setting(INI_GOLD_NORMAL_HIGH));
            concentrate =
                ResourceRange(ini_get_setting(INI_GOLD_CONCENTRATE_LOW), ini_get_setting(INI_GOLD_CONCENTRATE_HIGH));
            concentrate_seperation = ini_get_setting(INI_GOLD_CONCENTRATE_SEPERATION);
            concentrate_variance = concentrate_seperation / 5;
            concentrate_diffusion = ini_get_setting(INI_GOLD_CONCENTRATE_DIFFUSION);
        } break;

        case CARGO_FUEL: {
            m_point.x = 1;
            m_point.y = 1;
            normal = ResourceRange(ini_get_setting(INI_FUEL_NORMAL_LOW), ini_get_setting(INI_FUEL_NORMAL_HIGH));
            concentrate =
                ResourceRange(ini_get_setting(INI_FUEL_CONCENTRATE_LOW), ini_get_setting(INI_FUEL_CONCENTRATE_HIGH));
            concentrate_seperation = ini_get_setting(INI_FUEL_CONCENTRATE_SEPERATION);
            concentrate_variance = concentrate_seperation / 5;
            concentrate_diffusion = ini_get_setting(INI_FUEL_CONCENTRATE_DIFFUSION);
        } break;
    }
}

int32_t ResourceAllocator::GetZoneResoureLevel(int32_t grid_x, int32_t grid_y) {
    int32_t result = 0;
    Rect bounds;

    rect_init(&bounds, grid_x, grid_y, grid_x + 2, grid_y + 2);

    if (bounds.ulx < 0) {
        bounds.ulx = 0;
    }

    if (bounds.uly < 0) {
        bounds.uly = 0;
    }

    if (ResourceManager_MapSize.x < bounds.lrx) {
        bounds.lrx = ResourceManager_MapSize.x;
    }

    if (ResourceManager_MapSize.y < bounds.lry) {
        bounds.lry = ResourceManager_MapSize.y;
    }

    for (int32_t i = bounds.ulx; i < bounds.lrx; ++i) {
        for (int32_t j = bounds.uly; j < bounds.lry; ++j) {
            result += ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] & 0x1F;
        }
    }

    return result;
}

int32_t ResourceAllocator::OptimizeResources(int32_t grid_x, int32_t grid_y, int32_t level_min, int32_t level_max) {
    int32_t zone_resource_level = GetZoneResoureLevel(grid_x, grid_y);

    if ((zone_resource_level + level_min) > level_max) {
        level_min = level_max - zone_resource_level;
    }

    if (level_min < 0) {
        level_min = 0;
    }

    return level_min;
}

void ResourceAllocator::Optimize(Point point, int32_t resource_level, int32_t resource_value) {
    Rect bounds;
    int32_t map_resource_amount;
    int32_t material_value;
    int32_t distance_factor;
    int32_t grid_x;
    int32_t grid_y;

    point.x = ((((point.x + 1) - m_point.x) / 2) * 2) + m_point.x;
    point.y = ((((point.y + 1) - m_point.y) / 2) * 2) + m_point.y;

    if (point.x < 0) {
        point.x = 0;
    }

    if (point.y < 0) {
        point.y = 0;
    }

    if (point.x >= ResourceManager_MapSize.x) {
        point.x = ResourceManager_MapSize.x - 1;
    }

    if (point.y >= ResourceManager_MapSize.y) {
        point.y = ResourceManager_MapSize.y - 1;
    }

    bounds.lry = point.y - 4;

    if (bounds.lry < 0) {
        bounds.lry = 0;
    }

    bounds.lry = ((bounds.lry / 2) * 2) + m_point.y;

    bounds.lrx = point.x - 4;

    if (bounds.lrx < 0) {
        bounds.lrx = 0;
    }

    bounds.lrx = ((bounds.lrx / 2) * 2) + m_point.x;

    bounds.uly = point.y + 5;

    if (bounds.uly > ResourceManager_MapSize.y) {
        bounds.uly = ResourceManager_MapSize.y;
    }

    bounds.ulx = point.x + 5;

    if (bounds.ulx > ResourceManager_MapSize.x) {
        bounds.ulx = ResourceManager_MapSize.x;
    }

    map_resource_amount = ResourceManager_CargoMap[point.y * ResourceManager_MapSize.x + point.x] & 0x1F;

    if (point.x > 0) {
        if (point.y > 0) {
            resource_level = ResourceAllocator::OptimizeResources(point.x - 1, point.y - 1, resource_level,
                                                                  resource_value + map_resource_amount);
        }

        if (point.y < ResourceManager_MapSize.y - 1) {
            resource_level = ResourceAllocator::OptimizeResources(point.x - 1, point.y, resource_level,
                                                                  resource_value + map_resource_amount);
        }
    }

    if (point.x < ResourceManager_MapSize.x - 1) {
        if (point.y > 0) {
            resource_level = ResourceAllocator::OptimizeResources(point.x, point.y - 1, resource_level,
                                                                  resource_value + map_resource_amount);
        }

        if (point.y < ResourceManager_MapSize.y - 1) {
            resource_level = ResourceAllocator::OptimizeResources(point.x, point.y, resource_level,
                                                                  resource_value + map_resource_amount);
        }
    }

    resource_level *= 100;

    for (grid_x = bounds.lrx; grid_x < bounds.ulx; grid_x += 2) {
        for (grid_y = bounds.lry; grid_y < bounds.uly; grid_y += 2) {
            distance_factor =
                ((Access_GetApproximateDistance(point.x - grid_x, point.y - grid_y) * 10) / concentrate_diffusion) + 10;
            material_value = resource_level / (distance_factor * distance_factor);

            if (material_value > (ResourceManager_CargoMap[grid_y * ResourceManager_MapSize.x + grid_x] & 0x1F)) {
                ResourceManager_CargoMap[grid_y * ResourceManager_MapSize.x + grid_x] = material_type + material_value;
            }
        }
    }
}

void ResourceAllocator::PopulateCargoMap() const {
    for (int32_t i = m_point.x; i < ResourceManager_MapSize.x; i += 2) {
        for (int32_t j = m_point.y; j < ResourceManager_MapSize.y; j += 2) {
            ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] = material_type + normal.GetValue();
        }
    }
}

void ResourceAllocator::ConcentrateResources() {
    Point point1;
    Point point2;
    bool flag = false;

    point1.x = Randomizer_Generate((concentrate_seperation * 4) / 5 + 1);
    point1.y = Randomizer_Generate(concentrate_seperation / 2 + 1);

    for (int32_t i = point1.x; i < ResourceManager_MapSize.x; i += (concentrate_seperation * 4) / 5) {
        for (int32_t j = flag ? (concentrate_seperation / 2 + point1.y) : point1.y; j < ResourceManager_MapSize.y;
             j += concentrate_seperation) {
            point2.x = Randomizer_Generate(concentrate_variance * 2 + 1) - concentrate_variance + i;
            point2.y = Randomizer_Generate(concentrate_variance * 2 + 1) - concentrate_variance + j;

            Optimize(point2, concentrate.GetValue(), max_resources);
        }

        flag = !flag;
    }
}

void ResourceAllocator::SettleMinimumResourceLevels(int32_t min_level) {
    for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
        for (int32_t j = 0; j < ResourceManager_MapSize.y; ++j) {
            if (GetZoneResoureLevel(i - 1, j - 1) < min_level && GetZoneResoureLevel(i - 1, j) < min_level &&
                GetZoneResoureLevel(i, j - 1) < min_level && GetZoneResoureLevel(i, j) < min_level) {
                ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] = CARGO_MATERIALS;
            }
        }
    }
}

void ResourceAllocator::SeparateResources(ResourceAllocator* allocator) {
    Point point1;
    Point point2;
    bool flag = false;

    point1.x = Randomizer_Generate(((mixed_resource_seperation * 4) / 5) + 1);
    point1.y = Randomizer_Generate(mixed_resource_seperation / 2 + 1);

    for (int32_t i = point1.x; i < ResourceManager_MapSize.x; i += (mixed_resource_seperation * 4) / 5) {
        for (int32_t j = flag ? (mixed_resource_seperation / 2 + point1.y) : point1.y; j < ResourceManager_MapSize.y;
             j += mixed_resource_seperation) {
            point2.x = Randomizer_Generate(mixed_resource_seperation_variance * 2 + 1) -
                       mixed_resource_seperation_variance + i;
            point2.y = Randomizer_Generate(mixed_resource_seperation_variance * 2 + 1) -
                       mixed_resource_seperation_variance + j;

            this->Optimize(point2, Randomizer_Generate(5) + 8, max_resources);
            allocator->Optimize(point2, Randomizer_Generate(5) + 8, max_resources);
        }

        flag = !flag;
    }
}
