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

#include "accessmap.hpp"
#include "pathfill.hpp"
#include "resource_manager.hpp"
#include "unitinfo.hpp"

TransporterMap::TransporterMap(UnitInfo* unit_, uint8_t flags_, uint8_t caution_level_, ResourceID unit_type_)
    : map(ResourceManager_GetActiveWorld()) {
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

        map.GetMap().Init(&*unit, flags, caution_level);

        if (unit_type != INVALID_ID && (unit->flags & MOBILE_LAND_UNIT)) {
            const World* world = ResourceManager_GetActiveWorld();
            AccessMap access_map(world);
            SmartPointer<UnitInfo> transporter(new (std::nothrow) UnitInfo(unit_type, unit->team, 0xFFFF));
            access_map.GetMap().Init(&*transporter, 0x01, CAUTION_LEVEL_AVOID_ALL_DAMAGE);

            for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                    if (access_map(x, y)) {
                        if (map(x, y) == 0) {
                            map(x, y) = (access_map(x, y) * 3) | 0x80;
                        }

                    } else {
                        map(x, y) |= 0x40;
                    }
                }
            }
        }

        filler.Fill(Point(unit->grid_x, unit->grid_y));

        is_initialized = true;
    }

    if ((map(site.x, site.y) & 0x20) && !(map(site.x, site.y) & 0x80)) {
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
        map(site.x, site.y) = 0x20;

    } else {
        map(site.x, site.y) = 0x00;
    }
}
