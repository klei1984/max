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

#include "transportermap.hpp"

#include "pathfill.hpp"
#include "paths_manager.hpp"
#include "resource_manager.hpp"
#include "unitinfo.hpp"

TransporterMap::TransporterMap(UnitInfo* unit_, unsigned char flags_, unsigned char caution_level_,
                               ResourceID unit_type_) {
    unit = unit_;
    flags = flags_;
    caution_level = caution_level_;
    is_initialized = false;
    unit_type = unit_type_;
}

TransporterMap::~TransporterMap() {}

bool TransporterMap::Search(Point site) {
    bool result;

    if (!is_initialized) {
        PathFill filler(map.GetMap());

        PathsManager_InitAccessMap(&*unit, map.GetMap(), flags, caution_level);

        if (unit_type != INVALID_ID && (unit->flags & MOBILE_LAND_UNIT)) {
            AccessMap access_map;
            SmartPointer<UnitInfo> transporter(new (std::nothrow) UnitInfo(unit_type, unit->team, 0xFFFF));
            PathsManager_InitAccessMap(&*transporter, access_map.GetMap(), 0x01, CAUTION_LEVEL_AVOID_ALL_DAMAGE);

            for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
                for (int j = 0; j < ResourceManager_MapSize.y; ++j) {
                    if (access_map.GetMapColumn(i)[j]) {
                        if (map.GetMapColumn(site.x)[site.y] == 0) {
                            map.GetMapColumn(site.x)[site.y] = (access_map.GetMapColumn(i)[j] * 3) | 0x80;
                        }

                    } else {
                        map.GetMapColumn(site.x)[site.y] |= 0x40;
                    }
                }
            }
        }

        filler.Fill(Point(unit->grid_x, unit->grid_y));

        is_initialized = true;
    }

    if ((map.GetMapColumn(site.x)[site.y] & 0x20) && !(map.GetMapColumn(site.x)[site.y] & 0x80)) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

void TransporterMap::UpdateSite(Point site, bool mode) {
    if (!is_initialized) {
        Search(site);
    }

    if (mode) {
        map.GetMapColumn(site.x)[site.y] = 0x20;

    } else {
        map.GetMapColumn(site.x)[site.y] = 0x00;
    }
}
