/* Copyright (c) 2023 M.A.X. Port Team
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

#include "saveloadchecks.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "hash.hpp"
#include "units_manager.hpp"

static bool SaveLoadChecks_IsHashMapCorrect(UnitInfo* unit);
static bool SaveLoadChecks_CorrectHashMap(SmartList<UnitInfo>* units);

bool SaveLoadChecks_OrderedLists() {
    struct ListSortCompare {
        static bool GroundCoverUnits(const SmartList<UnitInfo>::Iterator& lhs,
                                     const SmartList<UnitInfo>::Iterator& rhs) {
            return UnitInfo::GetDrawLayer(lhs->Get()->GetUnitType()) <
                   UnitInfo::GetDrawLayer(rhs->Get()->GetUnitType());
        }

        static bool SeaParticles(const SmartList<UnitInfo>::Iterator& lhs, const SmartList<UnitInfo>::Iterator& rhs) {
            return ((lhs->Get()->flags & (MISSILE_UNIT | GROUND_COVER)) == (MISSILE_UNIT | GROUND_COVER));
        }

        static bool StationaryUnits(const SmartList<UnitInfo>::Iterator& lhs,
                                    const SmartList<UnitInfo>::Iterator& rhs) {
            return lhs->Get()->y < rhs->Get()->y;
        }

        static bool HoveringAirUnits(const SmartList<UnitInfo>::Iterator& lhs,
                                     const SmartList<UnitInfo>::Iterator& rhs) {
            return !(lhs->Get()->flags & HOVERING);
        }

        static bool Explosions(const SmartList<UnitInfo>::Iterator& lhs, const SmartList<UnitInfo>::Iterator& rhs) {
            return !((lhs->Get()->flags & EXPLODING) && lhs->Get()->GetUnitType() != RKTSMOKE);
        }
    };

    UnitsManager_GroundCoverUnits.Sort(ListSortCompare::GroundCoverUnits);
    UnitsManager_GroundCoverUnits.Sort(ListSortCompare::SeaParticles);
    UnitsManager_StationaryUnits.Sort(ListSortCompare::StationaryUnits);
    UnitsManager_MobileAirUnits.Sort(ListSortCompare::HoveringAirUnits);
    UnitsManager_ParticleUnits.Sort(ListSortCompare::Explosions);

    return false;
}

bool SaveLoadChecks_Defect11() {
    bool result = false;

    for (auto it = UnitsManager_StationaryUnits.Begin(), end = UnitsManager_StationaryUnits.End(); it != end; ++it) {
        SmartPointer<UnitInfo> shop(*it);

        if (shop->GetUnitType() == DEPOT || (*it).GetUnitType() == DOCK || (*it).GetUnitType() == HANGAR) {
            const int32_t stored_units = Access_GetStoredUnitCount(&*shop);
            const int32_t storable_units = shop->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            if (stored_units > storable_units || shop->storage != stored_units) {
                AiLog log("Repair shop corruption detected at [%i,%i].", shop->grid_x, shop->grid_y);

                shop->storage = stored_units;

                result = true;
            }
        }
    }

    return result;
}

bool SaveLoadChecks_IsHashMapCorrect(UnitInfo* unit) {
    bool result{false};

    if (unit->hits > 0) {
        SmartPointer<UnitInfo> parent(unit->GetParent());

        if (parent &&
            (parent->GetUnitType() == AIRTRANS || parent->GetUnitType() == SEATRANS ||
             parent->GetUnitType() == CLNTRANS || parent->GetUnitType() == DEPOT || parent->GetUnitType() == HANGAR ||
             parent->GetUnitType() == DOCK || parent->GetUnitType() == BARRACKS)) {
            result = true;
        }
    }

    if (!result) {
        int32_t cell_count;
        Rect bounds;

        if ((unit->flags) & BUILDING) {
            rect_init(&bounds, unit->grid_x, unit->grid_y, unit->grid_x + 2, unit->grid_y + 2);
            cell_count = 4;

        } else if (unit->in_transit) {
            rect_init(&bounds, unit->last_target.x, unit->last_target.y, unit->last_target.x + 1,
                      unit->last_target.y + 1);
            cell_count = 1;

        } else {
            rect_init(&bounds, unit->grid_x, unit->grid_y, unit->grid_x + 1, unit->grid_y + 1);
            cell_count = 1;
        }

        for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
            for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                const auto units = Hash_MapHash[Point(x, y)];

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if (&*it == unit) {
                            --cell_count;
                            break;
                        }
                    }
                }
            }
        }

        result = (cell_count == 0);
    }

    return result;
}

bool SaveLoadChecks_CorrectHashMap(SmartList<UnitInfo>* units) {
    bool result = false;

    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
        SmartPointer<UnitInfo> unit(*it);

        if (!SaveLoadChecks_IsHashMapCorrect(&*unit)) {
            if (unit->flags & SELECTABLE) {
                SmartPointer<UnitInfo> parent(unit->GetParent());

                if (unit->hits > 0) {
                    AiLog log("Map hash corruption detected. %s at [%i,%i] is missing.",
                              UnitsManager_BaseUnits[unit->GetUnitType()].singular_name, unit->grid_x + 1,
                              unit->grid_y + 1);

                    Hash_MapHash.Remove(&*unit);
                    Hash_MapHash.Add(&*unit);
                    Access_UpdateMapStatus(&*unit, true);

                } else {
                    AiLog log("Map hash corruption detected. %s at [%i,%i] is already gone.",
                              UnitsManager_BaseUnits[unit->GetUnitType()].singular_name, unit->grid_x + 1,
                              unit->grid_y + 1);

                    Hash_MapHash.Remove(&*unit);
                    units->Remove(it);
                    Access_UpdateMapStatus(&*unit, false);
                }

            } else {
                AiLog log("Map hash corruption detected. %s at [%i,%i] is missing.",
                          UnitsManager_BaseUnits[unit->GetUnitType()].singular_name, unit->grid_x + 1,
                          unit->grid_y + 1);

                Hash_MapHash.Remove(&*unit);
                Hash_MapHash.Add(&*unit);
                Access_UpdateMapStatus(&*unit, true);
            }

            result = true;
        }
    }

    return result;
}

bool SaveLoadChecks_Defect151() {
    bool result = false;

    for (auto it = UnitsManager_StationaryUnits.Begin(), end = UnitsManager_StationaryUnits.End(); it != end; ++it) {
        if ((*it).flags & CONNECTOR_UNIT) {
            const auto units = Hash_MapHash[Point((*it).grid_x, (*it).grid_y)];

            if (units) {
                // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                for (auto hash_it = units->Begin(), hash_end = units->End(); hash_it != hash_end; ++hash_it) {
                    if (((*hash_it).flags & STATIONARY) && !((*hash_it).flags & (GROUND_COVER | CONNECTOR_UNIT)) &&
                        !((*hash_it).orders == ORDER_IDLE && (*hash_it).state == ORDER_STATE_BUILDING_READY)) {
                        AiLog log("Unit corruption detected. Connector at [%i,%i] overlaps with %s at [%i,%i].",
                                  (*it).grid_x + 1, (*it).grid_y + 1,
                                  UnitsManager_BaseUnits[(*hash_it).GetUnitType()].singular_name, (*hash_it).grid_x + 1,
                                  (*hash_it).grid_y + 1);

                        UnitsManager_DestroyUnit(&*it);

                        result = true;
                    }
                }
            }
        }
    }

    return result;
}

bool SaveLoadChecks_Defect183() {
    bool result = false;

    // first remove already destroyed units from the hash map
    for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
            const auto units = Hash_MapHash[Point(x, y)];

            if (units) {
                // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                    if (!(*it).in_transit) {
                        Rect bounds;
                        Point site(x, y);
                        Point position((*it).grid_x, (*it).grid_y);

                        if (((*it).flags) & BUILDING) {
                            rect_init(&bounds, position.x, position.y, position.x + 2, position.y + 2);

                        } else {
                            rect_init(&bounds, position.x, position.y, position.x + 1, position.y + 1);
                        }

                        if (!Access_IsInsideBounds(&bounds, &site)) {
                            AiLog log("Map hash corruption detected in pot [%i,%i]. %s is at [%i,%i].", site.x + 1,
                                      site.y + 1, UnitsManager_BaseUnits[(*it).GetUnitType()].singular_name,
                                      position.x + 1, position.y + 1);

                            Hash_MapHash.Remove(&*it);
                            Access_UpdateMapStatus(&*it, false);

                            (*it).grid_x = site.x;
                            (*it).grid_y = site.y;

                            Hash_MapHash.Remove(&*it);
                            Access_UpdateMapStatus(&*it, false);

                            result = true;
                        }
                    }
                }
            }
        }
    }

    // then recover or remove units missing from the hash map
    result |= SaveLoadChecks_CorrectHashMap(&UnitsManager_GroundCoverUnits);
    result |= SaveLoadChecks_CorrectHashMap(&UnitsManager_StationaryUnits);
    result |= SaveLoadChecks_CorrectHashMap(&UnitsManager_MobileLandSeaUnits);
    result |= SaveLoadChecks_CorrectHashMap(&UnitsManager_ParticleUnits);
    result |= SaveLoadChecks_CorrectHashMap(&UnitsManager_MobileAirUnits);

    return result;
}
