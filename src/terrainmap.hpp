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

#ifndef TERRAINMAP_HPP
#define TERRAINMAP_HPP

#include "point.hpp"

class TerrainMap {
    int16_t dimension_x;
    uint16_t **land_map;
    uint16_t **water_map;

    int32_t TerrainMap_sub_68EEF(uint16_t **map, Point location);
    void SetTerrain(uint16_t **map, Point location);
    void ClearTerrain(uint16_t **map, Point location);

public:
    TerrainMap();
    ~TerrainMap();

    void Init();
    void Deinit();

    int32_t TerrainMap_sub_690D6(Point location, int32_t surface_type);
    void UpdateTerrain(Point location, int32_t surface_type);
};

#endif /* TERRAINMAP_HPP */
