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
#include <array>
#include <cstring>
#include <vector>

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

namespace {

constexpr size_t SURFACE_BASE_SLOT_COUNT{32};

struct SurfaceBaseCache {
    const World* world{nullptr};
    Point size{0, 0};
    std::array<std::vector<uint8_t>, SURFACE_BASE_SLOT_COUNT> slots;

    void Reset(const World* new_world, const Point new_size) {
        world = new_world;
        size = new_size;

        for (auto& slot : slots) {
            slot.clear();
            slot.shrink_to_fit();
        }
    }
};

SurfaceBaseCache AccessMap_SurfaceBaseCache;

}  // namespace

void AccessMap::ApplySurfaceBase(int32_t surface_types, uint8_t water_value) {
    const size_t key = (static_cast<size_t>(surface_types) & 0x0F) | ((water_value == 8) ? 0x10 : 0);

    // A different world or a resized map invalidates every cached base.
    if (AccessMap_SurfaceBaseCache.world != m_world || AccessMap_SurfaceBaseCache.size != m_size) {
        AccessMap_SurfaceBaseCache.Reset(m_world, m_size);
    }

    std::vector<uint8_t>& base = AccessMap_SurfaceBaseCache.slots[key];

    if (base.size() != m_data.size()) {
        base.assign(m_data.size(), 0);

        for (int32_t index_x = 0; index_x < m_size.x; ++index_x) {
            for (int32_t index_y = 0; index_y < m_size.y; ++index_y) {
                const uint8_t surface_type = m_world->GetSurfaceType(index_x, index_y);
                uint8_t value = 0;

                if (surface_type == SURFACE_TYPE_LAND) {
                    if (surface_types & SURFACE_TYPE_LAND) {
                        value = 4;
                    }

                } else if (surface_type == SURFACE_TYPE_COAST) {
                    if (surface_types & SURFACE_TYPE_COAST) {
                        value = 4;
                    }

                } else if (surface_type == SURFACE_TYPE_WATER) {
                    if (surface_types & SURFACE_TYPE_WATER) {
                        value = water_value;
                    }
                }

                base[static_cast<size_t>(index_x) * m_size.y + index_y] = value;
            }
        }
    }

    std::memcpy(m_data.data(), base.data(), m_data.size());
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

void AccessMap::ApplyDamageMask(const int16_t* const* damage_potential_map, int32_t unit_hits) {
    for (int32_t i = 0; i < m_size.x; ++i) {
        for (int32_t j = 0; j < m_size.y; ++j) {
            if (damage_potential_map[i][j] >= unit_hits) {
                (*this)(i, j) = 0;
            }
        }
    }
}

void AccessMap::ProcessGroundCover(UnitInfo* unit, int32_t surface_type) {
    const uint16_t team = unit->team;
    const bool process_paved =
        (surface_type & SURFACE_TYPE_LAND) && unit->GetLayingState() != 2 && unit->GetLayingState() != 1;

    static std::vector<UnitInfo*> surface_units;
    static std::vector<UnitInfo*> paved_units;
    static std::vector<UnitInfo*> tape_units;
    static std::vector<UnitInfo*> mine_units;

    surface_units.clear();
    paved_units.clear();
    tape_units.clear();
    mine_units.clear();

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
         it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if (!((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team))) {
            continue;
        }

        UnitInfo* const cover = &*it;

        switch (cover->GetUnitType()) {
            case BRIDGE: {
                surface_units.push_back(cover);

                if (process_paved) {
                    paved_units.push_back(cover);
                }
            } break;

            case WTRPLTFM: {
                surface_units.push_back(cover);
            } break;

            case ROAD:
            case SMLSLAB:
            case LRGSLAB: {
                if (process_paved) {
                    paved_units.push_back(cover);
                }
            } break;

            case LRGTAPE:
            case SMLTAPE: {
                tape_units.push_back(cover);
            } break;

            case LANDMINE:
            case SEAMINE: {
                mine_units.push_back(cover);
            } break;
        }
    }

    for (UnitInfo* const cover : surface_units) {
        if (cover->GetUnitType() == BRIDGE) {
            if (surface_type & (SURFACE_TYPE_LAND | SURFACE_TYPE_WATER)) {
                (*this)(cover->grid_x, cover->grid_y) = 4;
            }

        } else {
            if (surface_type & SURFACE_TYPE_LAND) {
                (*this)(cover->grid_x, cover->grid_y) = 4;

            } else {
                (*this)(cover->grid_x, cover->grid_y) = 0;
            }
        }
    }

    for (UnitInfo* const cover : paved_units) {
        (*this)(cover->grid_x, cover->grid_y) = 2;

        if (cover->flags & BUILDING) {
            (*this)(cover->grid_x + 1, cover->grid_y) = 2;
            (*this)(cover->grid_x, cover->grid_y + 1) = 2;
            (*this)(cover->grid_x + 1, cover->grid_y + 1) = 2;
        }
    }

    for (UnitInfo* const cover : tape_units) {
        (*this)(cover->grid_x, cover->grid_y) = 0;

        if (cover->flags & BUILDING) {
            (*this)(cover->grid_x + 1, cover->grid_y) = 0;
            (*this)(cover->grid_x, cover->grid_y + 1) = 0;
            (*this)(cover->grid_x + 1, cover->grid_y + 1) = 0;
        }
    }

    // mine fields shall be processed after everything else otherwise the map would overwrite no-go zones
    for (UnitInfo* const cover : mine_units) {
        if (cover->team != team && cover->IsDetectedByTeam(team)) {
            (*this)(cover->grid_x, cover->grid_y) = 0;
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

        const uint8_t water_value = ((surface_types & SURFACE_TYPE_LAND) && unit->GetUnitType() != SURVEYOR) ? 8 : 4;

        ApplySurfaceBase(surface_types, water_value);

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

                ApplyDamageMask(damage_potential_map, unit_hits);
            }
        }
    }
}
