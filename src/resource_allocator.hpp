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

#ifndef RESOURCE_ALLOCATOR_HPP
#define RESOURCE_ALLOCATOR_HPP

#include <cstdint>

#include "enums.hpp"
#include "point.hpp"

class ResourceRange {
private:
    int16_t min;
    int16_t max;

public:
    ResourceRange() : min(0), max(0) {}
    ResourceRange(const ResourceRange& other) : min(other.min), max(other.max) {}
    ResourceRange(int32_t min, int32_t max) : min(min), max(max) {}

    int32_t GetValue() const;
};

class ResourceAllocator {
private:
    ResourceRange normal;
    ResourceRange concentrate;
    Point m_point;
    int32_t max_resources;
    int32_t mixed_resource_seperation;
    int32_t mixed_resource_seperation_variance;
    uint16_t material_type;
    int16_t concentrate_seperation;
    int16_t concentrate_variance;
    int16_t concentrate_diffusion;

    static int32_t GetZoneResoureLevel(int32_t grid_x, int32_t grid_y);
    static int32_t OptimizeResources(int32_t grid_x, int32_t grid_y, int32_t level_min, int32_t level_max);

public:
    ResourceAllocator(uint16_t material_type);

    void Optimize(Point point, int32_t resource_level, int32_t resource_value);
    void PopulateCargoMap() const;
    void ConcentrateResources();
    static void SettleMinimumResourceLevels(int32_t min_level);
    void SeparateResources(ResourceAllocator* allocator);
};

#endif /* RESOURCE_ALLOCATOR_HPP */
