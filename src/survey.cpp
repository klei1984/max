/* Copyright (c) 2022 M.A.X. Port Team
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

#include "survey.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "resource_manager.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

void Survey_RenderMarker(WindowInfo* window, int32_t grid_x, int32_t grid_y, uint16_t material_type) {
    int32_t resource_value;
    ResourceID marker_big;
    ResourceID marker_small;
    struct ImageSimpleHeader* buffer;
    bool flag = false;

    resource_value = std::min(material_type & CARGO_MASK, 16);

    if (material_type & CARGO_GOLD) {
        marker_big = GOLDMK0;
        marker_small = SMGDMK1;
    } else if (material_type & CARGO_FUEL) {
        marker_big = FUELMK0;
        marker_small = SMFLMK1;
    } else {
        marker_big = RAWMSK0;
        marker_small = SMRWMK1;
    }

    if (Gfx_ZoomLevel > 16) {
        buffer = reinterpret_cast<struct ImageSimpleHeader*>(
            ResourceManager_LoadResource(static_cast<ResourceID>(marker_big + resource_value)));

        if (Gfx_ZoomLevel != 64) {
            buffer = WindowManager_RescaleSimpleImage(buffer, Gfx_MapScalingFactor);
            flag = true;
        }

    } else {
        if (resource_value == 0) {
            return;
        }

        buffer = reinterpret_cast<struct ImageSimpleHeader*>(
            ResourceManager_LoadResource(static_cast<ResourceID>(marker_small + resource_value - 1)));

        if (Gfx_ZoomLevel != 16) {
            buffer = WindowManager_RescaleSimpleImage(buffer, Gfx_MapScalingFactor / 4);
            flag = true;
        }
    }

    WindowManager_DecodeSimpleImage(buffer, (((grid_x * 64 + 32) << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx,
                                    (((grid_y * 64 + 32) << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly, true,
                                    window);

    if (flag) {
        delete[] buffer;
    }
}

void Survey_SurveyArea(UnitInfo* unit, int32_t radius) {
    uint16_t team;
    CTInfo* team_info;
    int32_t grid_x;
    int32_t grid_y;
    UnitInfo* enemy;
    Rect bounds;

    team = unit->team;
    team_info = &UnitsManager_TeamInfo[team];
    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    for (int32_t i = std::max(0, grid_x - radius); i <= std::min(ResourceManager_MapSize.x - 1, grid_x + radius); ++i) {
        for (int32_t j = std::max(0, grid_y - radius); j <= std::min(ResourceManager_MapSize.y - 1, grid_y + radius);
             ++j) {
            if (Access_GetModifiedSurfaceType(i, j) != SURFACE_TYPE_AIR) {
                enemy = Access_GetEnemyMineOnSentry(team, i, j);
                Ai_MarkMineMapPoint(Point(i, j), team);

                if (enemy) {
                    enemy->SpotByTeam(team);
                }

                if (unit->GetUnitType() == SURVEYOR) {
                    if (team == GameManager_PlayerTeam &&
                        !(ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] &
                          team_info->team_units->hash_team_id)) {
                        rect_init(&bounds, i * 64, j * 64, i * 64 + 63, j * 64 + 63);
                        GameManager_AddDrawBounds(&bounds);
                    }

                    ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] |= team_info->team_units->hash_team_id;
                }
            }
        }
    }
}

void Survey_RenderMarkers(uint16_t team, int32_t grid_ulx, int32_t grid_uly, int32_t grid_lrx) {
    CTInfo* team_info;
    WindowInfo* window;
    int32_t grid_lry;
    uint16_t mask;

    team_info = &UnitsManager_TeamInfo[team];
    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    grid_ulx += GameManager_MapView.ulx - (GameManager_MapWindowDrawBounds.ulx / 64);
    grid_uly += GameManager_MapView.uly - (GameManager_MapWindowDrawBounds.uly / 64);
    grid_lrx += GameManager_MapView.ulx - (GameManager_MapWindowDrawBounds.ulx / 64);
    grid_lry = grid_uly + 1;

    if (GameManager_MaxSurvey) {
        mask = 0xFFFF;
    } else {
        mask = 0x0000;
    }

    mask |= team_info->team_units->hash_team_id;

    for (int32_t i = std::max(GameManager_MapView.ulx, grid_ulx); i < std::min(GameManager_MapView.lrx + 1, grid_lrx);
         ++i) {
        for (int32_t j = std::max(GameManager_MapView.uly, grid_uly);
             j < std::min(GameManager_MapView.lry + 1, grid_lry); ++j) {
            if (Access_GetModifiedSurfaceType(i, j) != SURFACE_TYPE_AIR &&
                (ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i] & mask)) {
                Survey_RenderMarker(window, i, j, ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i]);
            }
        }
    }
}

void Survey_GetResourcesInArea(int32_t grid_x, int32_t grid_y, int32_t radius, int32_t resource_limit, int16_t* raw,
                               int16_t* gold, int16_t* fuel, bool mode, uint16_t team) {
    CTInfo* team_info;
    uint16_t mask;
    uint16_t value;

    team_info = &UnitsManager_TeamInfo[team];

    *raw = 0;
    *gold = 0;
    *fuel = 0;

    if (mode) {
        mask = 0xFFFF;
    } else {
        mask = 0x0000;
    }

    mask |= team_info->team_units->hash_team_id;

    for (int32_t i = grid_x; i <= std::min(ResourceManager_MapSize.x - 1, grid_x + radius); ++i) {
        for (int32_t j = grid_y; j <= std::min(ResourceManager_MapSize.y - 1, grid_y + radius); ++j) {
            value = ResourceManager_CargoMap[ResourceManager_MapSize.x * j + i];

            if ((value & mask) && Access_GetModifiedSurfaceType(i, j) != SURFACE_TYPE_AIR) {
                if (value & CARGO_GOLD) {
                    *gold += std::min(value & CARGO_MASK, 16);
                } else if (value & CARGO_FUEL) {
                    *fuel += std::min(value & CARGO_MASK, 16);
                } else {
                    *raw += std::min(value & CARGO_MASK, 16);
                }
            }
        }
    }

    if (*raw > resource_limit) {
        *raw = resource_limit;
    }

    if (*gold > resource_limit) {
        *gold = resource_limit;
    }

    if (*fuel > resource_limit) {
        *fuel = resource_limit;
    }
}

void Survey_GetTotalResourcesInArea(int32_t grid_x, int32_t grid_y, int32_t radius, int16_t* raw, int16_t* gold,
                                    int16_t* fuel, bool mode, uint16_t team) {
    Survey_GetResourcesInArea(grid_x, grid_y, radius, 255, raw, gold, fuel, mode, team);
}
