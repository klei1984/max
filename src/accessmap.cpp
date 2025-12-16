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

#include "accessmap.hpp"

#include <algorithm>
#include <cstring>

#include "access.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "enums.hpp"
#include "resource_manager.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"
#include "world.hpp"
#include "zonewalker.hpp"

AccessMap::AccessMap(const World* world)
    : m_world(world),
      m_data(static_cast<size_t>(world->GetMapSize().x) * world->GetMapSize().y, 0),
      m_size(world->GetMapSize()) {}

void AccessMap::Fill(uint8_t value) { std::memset(m_data.data(), value, m_data.size()); }

void AccessMap::FillColumn(int32_t x, uint8_t value) { std::memset(&m_data[x * m_size.y], value, m_size.y); }

void AccessMap::FillColumn(int32_t x, int32_t y_start, int32_t count, uint8_t value) {
    std::memset(&m_data[x * m_size.y + y_start], value, count);
}

bool AccessMap::IsProcessed(int32_t grid_x, int32_t grid_y) const {
    if (grid_x >= 0 && grid_x < m_size.x && grid_y >= 0 && grid_y < m_size.y) {
        uint8_t value = (*this)(grid_x, grid_y);

        return value && !(value & 0x80);
    }

    return false;
}

void AccessMap::ProcessStationaryUnits(UnitInfo* unit) {
    uint16_t team = unit->team;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetUnitType() != CNCT_4W && ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team))) {
            (*this)((*it).grid_x, (*it).grid_y) = 0;

            if ((*it).flags & BUILDING) {
                (*this)((*it).grid_x + 1, (*it).grid_y) = 0;
                (*this)((*it).grid_x, (*it).grid_y + 1) = 0;
                (*this)((*it).grid_x + 1, (*it).grid_y + 1) = 0;
            }
        }
    }
}

void AccessMap::ProcessMobileUnits(SmartList<UnitInfo>* units, UnitInfo* unit, uint8_t flags) {
    uint16_t team = unit->team;
    bool is_air_pathfinder = (unit->flags & MOBILE_AIR_UNIT);

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).GetOrder() != ORDER_IDLE && (*it).IsVisibleToTeam(team)) {
            if ((flags & AccessModifier_SameClassBlocks) ||
                ((flags & AccessModifier_EnemySameClassBlocks) && (*it).team != team)) {
                // Skip hovering aircraft when building land unit access maps
                if (((*it).flags & MOBILE_AIR_UNIT) && !is_air_pathfinder && ((*it).flags & HOVERING)) {
                    continue;
                }

                (*this)((*it).grid_x, (*it).grid_y) = 0;

                if ((*it).path != nullptr && (*it).GetOrderState() != ORDER_STATE_EXECUTING_ORDER && (&*it) != unit) {
                    Point position = (*it).path->GetPosition(&*it);
                    (*this)(position.x, position.y) = 0;
                }
            }
        }

        // Block landed air units for land/sea unit pathfinding (redundant check but kept for clarity)
        if (!is_air_pathfinder && ((*it).flags & MOBILE_AIR_UNIT) && !((*it).flags & HOVERING) &&
            (*it).IsVisibleToTeam(team) && ((*it).GetOrder() != ORDER_IDLE || ((*it).flags & STATIONARY))) {
            (*this)((*it).grid_x, (*it).grid_y) = 0;
        }
    }
}

void AccessMap::ProcessMapSurface(int32_t surface_type, uint8_t value) {
    for (int32_t index_x = 0; index_x < m_size.x; ++index_x) {
        for (int32_t index_y = 0; index_y < m_size.y; ++index_y) {
            if (m_world->GetSurfaceType(index_x, index_y) == surface_type) {
                (*this)(index_x, index_y) = value;
            }
        }
    }
}

void AccessMap::ProcessGroundCover(UnitInfo* unit, int32_t surface_type) {
    uint16_t team = unit->team;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
         it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team)) {
            switch ((*it).GetUnitType()) {
                case BRIDGE: {
                    if (surface_type & SURFACE_TYPE_LAND) {
                        (*this)((*it).grid_x, (*it).grid_y) = 4;
                    }
                } break;

                case WTRPLTFM: {
                    if (surface_type & SURFACE_TYPE_LAND) {
                        (*this)((*it).grid_x, (*it).grid_y) = 4;

                    } else {
                        (*this)((*it).grid_x, (*it).grid_y) = 0;
                    }
                } break;
            }
        }
    }

    if ((surface_type & SURFACE_TYPE_LAND) && unit->GetLayingState() != 2 && unit->GetLayingState() != 1) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team)) {
                if ((*it).GetUnitType() == ROAD || (*it).GetUnitType() == SMLSLAB || (*it).GetUnitType() == LRGSLAB ||
                    (*it).GetUnitType() == BRIDGE) {
                    (*this)((*it).grid_x, (*it).grid_y) = 2;

                    if ((*it).flags & BUILDING) {
                        (*this)((*it).grid_x + 1, (*it).grid_y) = 2;
                        (*this)((*it).grid_x, (*it).grid_y + 1) = 2;
                        (*this)((*it).grid_x + 1, (*it).grid_y + 1) = 2;
                    }
                }
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
         it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team)) {
            if ((*it).GetUnitType() == LRGTAPE || (*it).GetUnitType() == LRGTAPE) {
                (*this)((*it).grid_x, (*it).grid_y) = 0;

                if ((*it).flags & BUILDING) {
                    (*this)((*it).grid_x + 1, (*it).grid_y) = 0;
                    (*this)((*it).grid_x, (*it).grid_y + 1) = 0;
                    (*this)((*it).grid_x + 1, (*it).grid_y + 1) = 0;
                }
            }
        }
    }

    // mine fields shall be processed after everything else otherwise the map would overwrite no-go zones
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
         it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team)) {
            switch ((*it).GetUnitType()) {
                case LANDMINE:
                case SEAMINE: {
                    if ((*it).team != team && (*it).IsDetectedByTeam(team)) {
                        (*this)((*it).grid_x, (*it).grid_y) = 0;
                    }
                } break;
            }
        }
    }
}

void AccessMap::ProcessDangers(UnitInfo* unit) {
    uint16_t team = unit->team;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team != team && (*it).IsVisibleToTeam(team) &&
            (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0 && (*it).GetOrder() != ORDER_DISABLE &&
            (*it).GetOrder() != ORDER_IDLE && (*it).hits > 0 &&
            Access_IsValidAttackTargetType((*it).GetUnitType(), unit->GetUnitType())) {
            ProcessSurface(&*it);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team != team && (*it).IsVisibleToTeam(team) &&
            (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0 && (*it).GetOrder() != ORDER_DISABLE &&
            (*it).GetOrder() != ORDER_IDLE && (*it).hits > 0 &&
            Access_IsValidAttackTargetType((*it).GetUnitType(), unit->GetUnitType())) {
            ProcessSurface(&*it);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).team != team && (*it).IsVisibleToTeam(team) &&
            (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0 && (*it).GetOrder() != ORDER_DISABLE &&
            (*it).GetOrder() != ORDER_IDLE && (*it).hits > 0 &&
            Access_IsValidAttackTargetType((*it).GetUnitType(), unit->GetUnitType())) {
            ProcessSurface(&*it);
        }
    }
}

void AccessMap::ProcessSurface(UnitInfo* unit) {
    int32_t range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
    Point position(unit->grid_x, unit->grid_y);

    if (unit->GetUnitType() == SUBMARNE || unit->GetUnitType() == CORVETTE) {
        ZoneWalker walker(position, range);

        do {
            if (m_world->GetSurfaceType(walker.GetGridX(), walker.GetGridY()) &
                (SURFACE_TYPE_WATER | SURFACE_TYPE_COAST)) {
                (*this)(walker.GetGridX(), walker.GetGridY()) = 0;
            }
        } while (walker.FindNext());

    } else {
        Point point3;
        Point point4;
        int32_t range_square;
        int32_t distance_square;

        range_square = range * range;

        point3.x = std::max(position.x - range, 0);
        point4.x = std::min(position.x + range, m_size.x - 1);

        for (; point3.x <= point4.x; ++point3.x) {
            distance_square = (point3.x - position.x) * (point3.x - position.x);

            for (point3.y = range; point3.y >= 0 && (point3.y * point3.y + distance_square) > range_square;
                 --point3.y) {
            }

            point4.y = std::min(position.y + point3.y, m_size.y - 1);
            point3.y = std::max(position.y - point3.y, 0);

            if (point4.y >= point3.y) {
                FillColumn(point3.x, point3.y, point4.y - point3.y + 1, 0);
            }
        }
    }
}

void AccessMap::Init(UnitInfo* unit, uint8_t flags, int32_t caution_level) {
    AILOG(log, "Mark cost map for {}.", ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data());

    if (unit->flags & MOBILE_AIR_UNIT) {
        Fill(4);

        ProcessMobileUnits(&UnitsManager_MobileAirUnits, unit, flags);

    } else {
        int32_t surface_types = ResourceManager_GetUnit(unit->GetUnitType()).GetLandType();

        Fill(0);

        if (surface_types & SURFACE_TYPE_LAND) {
            ProcessMapSurface(SURFACE_TYPE_LAND, 4);
        }

        if (surface_types & SURFACE_TYPE_COAST) {
            ProcessMapSurface(SURFACE_TYPE_COAST, 4);
        }

        if (surface_types & SURFACE_TYPE_WATER) {
            if ((surface_types & SURFACE_TYPE_LAND) && unit->GetUnitType() != SURVEYOR) {
                ProcessMapSurface(SURFACE_TYPE_WATER, 8);

            } else {
                ProcessMapSurface(SURFACE_TYPE_WATER, 4);
            }
        }

        ProcessGroundCover(unit, surface_types);
        ProcessMobileUnits(&UnitsManager_MobileLandSeaUnits, unit, flags);
        ProcessMobileUnits(&UnitsManager_MobileAirUnits, unit, flags);
        ProcessStationaryUnits(unit);
    }

    if (caution_level > 0) {
        ApplyCautionLevel(unit, caution_level);
    }
}

void AccessMap::ApplyCautionLevel(UnitInfo* unit, int32_t caution_level) {
    if (caution_level > 0) {
        if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_PLAYER) {
            ProcessDangers(unit);
        }

        if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
            int32_t unit_hits = unit->hits;
            int16_t** damage_potential_map;

            if (unit->GetId() == 0xFFFF) {
                damage_potential_map =
                    AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit->GetUnitType(), caution_level, true);

            } else {
                damage_potential_map = AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit, caution_level, true);
            }

            if (damage_potential_map) {
                if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
                    unit_hits = 1;
                }

                for (int32_t i = 0; i < m_size.x; ++i) {
                    for (int32_t j = 0; j < m_size.y; ++j) {
                        if (damage_potential_map[i][j] >= unit_hits) {
                            (*this)(i, j) = 0;
                        }
                    }
                }
            }
        }
    }
}
