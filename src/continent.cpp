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

#include "continent.hpp"

#include "access.hpp"
#include "accessmap.hpp"
#include "continentfiller.hpp"
#include "randomizer.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "units_manager.hpp"

bool Continent::IsDangerousProximity(int32_t grid_x, int32_t grid_y, uint16_t team, int32_t proximity_range) {
    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (team != i && UnitsManager_TeamMissionSupplies[i].units.GetCount() > 0) {
            Point distance;

            distance.x = grid_x - UnitsManager_TeamMissionSupplies[i].starting_position.x;
            distance.y = grid_y - UnitsManager_TeamMissionSupplies[i].starting_position.y;

            if (labs(distance.x) <= proximity_range && labs(distance.y) <= proximity_range) {
                return true;
            }
        }
    }

    return false;
}

bool Continent::IsViableSite(bool test_proximity, uint16_t team, Point site) {
    bool result;

    if (map(site.x, site.y) == filler) {
        if (test_proximity) {
            if (IsDangerousProximity(site.x, site.y, team,
                                     ResourceManager_GetSettings()->GetNumericValue("proximity_range") + 4)) {
                return false;
            }
        }

        for (int32_t i = site.x; i < site.x + 4; ++i) {
            for (int32_t j = site.y; j < site.y + 4; ++j) {
                if (map(i, j) != filler) {
                    return false;
                }
            }
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

Continent::Continent(AccessMap& map, uint16_t filler, Point point, uint8_t value) : map(map) {
    ContinentFiller continent_filler(map, filler);

    this->point = point;
    field_35 = value;
    this->filler = filler;
    continent_size = 0;
    is_isolated = false;

    continent_size = continent_filler.Fill(point);

    bounds = *continent_filler.GetBounds();
}

Continent::~Continent() {
    if (field_35) {
        for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
            for (int32_t y = bounds.uly; y < bounds.lry - 3; ++y) {
                if (map(x, y) == filler) {
                    map(x, y) = 0;
                }
            }
        }
    }
}

void Continent::GetBounds(Rect& bounds_) const { bounds_ = bounds; }

Point Continent::GetCenter() const { return Point((bounds.lrx + bounds.ulx) / 2, (bounds.lry + bounds.uly) / 2); }

bool Continent::IsIsolated() const { return is_isolated; }

uint16_t Continent::GetContinentSize() const { return continent_size; }

uint16_t Continent::GetFiller() const { return filler; }

bool Continent::IsCloseProximity() const {
    int32_t proximity_range;

    proximity_range = ResourceManager_GetSettings()->GetNumericValue("proximity_range");

    return (bounds.lrx - bounds.ulx) <= proximity_range && (bounds.lry - bounds.uly) <= proximity_range;
}

bool Continent::IsViableContinent(bool test_proximity, uint16_t team) {
    Point site;

    for (site.x = bounds.ulx; site.x < bounds.lrx - 3; ++site.x) {
        for (site.y = bounds.uly; site.y < bounds.lry - 3; ++site.y) {
            if (IsViableSite(test_proximity, team, site)) {
                return true;
            }
        }
    }

    return false;
}

void Continent::SelectLandingSite(uint16_t team, int32_t strategy) {
    Point site;

    do {
        site.x = bounds.ulx + Randomizer_Generate(bounds.lrx - bounds.ulx - 4 + 1);
        site.y = bounds.uly + Randomizer_Generate(bounds.lry - bounds.uly - 4 + 1);
    } while (!IsViableSite(true, team, site));

    UnitsManager_TeamMissionSupplies[team].starting_position = site;
}

void Continent::TestIsolated() {
    if (IsCloseProximity()) {
        Rect zone;
        Point position;
        ObjectArray<Point> points;
        const World* world = ResourceManager_GetActiveWorld();
        AccessMap access_map(world);

        zone = bounds;

        zone.ulx = std::max(0, zone.ulx - 6);
        zone.uly = std::max(0, zone.uly - 6);
        zone.lrx = std::min(static_cast<int32_t>(ResourceManager_MapSize.x), zone.lrx + 6);
        zone.lry = std::min(static_cast<int32_t>(ResourceManager_MapSize.y), zone.lry + 6);

        for (position.x = zone.ulx; position.x < zone.lrx; ++position.x) {
            for (position.y = zone.uly; position.y < zone.lry; ++position.y) {
                if (map(position.x, position.y) == filler) {
                    points.Append(&position);
                    access_map(position.x, position.y) = 0x00;

                } else {
                    access_map(position.x, position.y) = 0x7F;
                }
            }
        }

        {
            Rect zone2;
            Point position2;
            int32_t value;
            int32_t value2;

            rect_init(&zone2, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

            while (points.GetCount() > 0) {
                position = *points[points.GetCount() - 1];
                points.Remove(points.GetCount() - 1);

                value = access_map(position.x, position.y) | 0x01;
                access_map(position.x, position.y) = value;

                for (int32_t direction = 0; direction < 8; ++direction) {
                    position2 = position;

                    position2 += DIRECTION_OFFSETS[direction];

                    if (Access_IsInsideBounds(&zone2, &position2)) {
                        if (direction & 1) {
                            value2 = value + 6;

                        } else {
                            value2 = value + 4;
                        }

                        if (value2 <= 24 && value2 < access_map(position2.x, position2.y)) {
                            if (map(position2.x, position2.y) >= 3 && map(position2.x, position2.y) != filler) {
                                return;
                            }

                            value2 &= 0xFE;

                            if ((access_map(position2.x, position2.y) & 0x01) && value2 <= 20) {
                                points.Append(&position2);
                            }

                            access_map(position2.x, position2.y) = value2;
                        }
                    }
                }
            }

            is_isolated = true;
        }
    }
}
