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

#include "access.hpp"

#include "hash.hpp"
#include "resource_manager.hpp"

unsigned char Access_GetSurfaceType(int grid_x, int grid_y) {
    return ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * grid_y + grid_x];
}

unsigned char Access_GetModifiedSurfaceType(int grid_x, int grid_y) {
    unsigned char surface_type;

    surface_type = Access_GetSurfaceType(grid_x, grid_y);

    if (surface_type == SURFACE_TYPE_WATER || SURFACE_TYPE_COAST) {
        SmartList<UnitInfo> units = Hash_MapHash[Point(grid_x, grid_y)];

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if ((*it).unit_type == WTRPLTFM || (*it).unit_type == BRIDGE) {
                surface_type = SURFACE_TYPE_LAND;
            }
        }
    }

    return surface_type;
}
