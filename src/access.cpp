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

#include <array>

#include "ai.hpp"
#include "ailog.hpp"
#include "buildmenu.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "paths_manager.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "survey.hpp"
#include "unitevents.hpp"
#include "units_manager.hpp"

enum {
    TARGET_CLASS_NONE = 0x0,
    TARGET_CLASS_LAND_STEALTH = 0x1,
    TARGET_CLASS_SEA_STEALTH = 0x2,
    TARGET_CLASS_LAND = 0x4,
    TARGET_CLASS_WATER = 0x8,
    TARGET_CLASS_AIR = 0x10
};

static const SmartList<UnitInfo>* Access_UnitsLists[] = {&UnitsManager_MobileLandSeaUnits, &UnitsManager_MobileAirUnits,
                                                         &UnitsManager_StationaryUnits, &UnitsManager_GroundCoverUnits,
                                                         &UnitsManager_ParticleUnits};

static bool Access_UpdateGroupSpeed(UnitInfo* unit);
static bool Access_IsValidAttackTargetTypeEx(ResourceID attacker, ResourceID target, uint32_t target_flags);
static bool Access_IsValidAttackTargetEx(ResourceID attacker, ResourceID target, uint32_t target_flags, Point point);
static void Access_ProcessGroupAirPath(UnitInfo* unit);

bool Access_SetUnitDestination(int32_t grid_x, int32_t grid_y, int32_t target_grid_x, int32_t target_grid_y,
                               bool mode) {
    SmartPointer<UnitInfo> unit;
    const auto units = Hash_MapHash[Point(target_grid_x, target_grid_y)];

    if (units) {
        // the end node must be cached in case Hash_MapHash.Remove() deletes the list
        for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
            if ((*it).flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
                if ((*it).GetOrder() == ORDER_IDLE) {
                    continue;
                }

                if (((*it).GetOrder() == ORDER_MOVE || (*it).GetOrder() == ORDER_MOVE_TO_UNIT ||
                     (*it).GetOrder() == ORDER_MOVE_TO_ATTACK) &&
                    (*it).GetOrderState() != ORDER_STATE_EXECUTING_ORDER && (*it).path != nullptr &&
                    (*it).grid_x == target_grid_x && (*it).grid_y == target_grid_y &&
                    !(*it).path->IsInPath(grid_x - target_grid_x, grid_y - target_grid_y)) {
                    return true;

                } else {
                    return false;
                }

            } else {
                if ((*it).GetUnitType() == BRIDGE) {
                    unit = (*it);
                }
            }
        }
    }

    if (unit != nullptr) {
        if (unit->GetOrder() == ORDER_AWAIT) {
            if (mode) {
                UnitsManager_SetNewOrderInt(&*unit, ORDER_MOVE, ORDER_STATE_LOWER);
            } else {
                UnitsManager_SetNewOrderInt(&*unit, ORDER_MOVE, ORDER_STATE_ELEVATE);
            }
        }

        return true;

    } else {
        return false;
    }
}

uint32_t Access_IsAccessible(ResourceID unit_type, uint16_t team, int32_t grid_x, int32_t grid_y, uint32_t flags) {
    uint32_t unit_flags;
    uint32_t result;

    unit_flags = UnitsManager_BaseUnits[unit_type].flags;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        result = 4;

        if (!(flags & 0x20)) {
            uint8_t surface_type = Access_GetModifiedSurfaceType(grid_x, grid_y);

            if ((surface_type & SURFACE_TYPE_WATER) && (unit_flags & MOBILE_LAND_UNIT) && unit_type != SURVEYOR) {
                result = 4 + 8;
            }

            const auto units = Hash_MapHash[Point(grid_x, grid_y)];

            if (units) {
                // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                    if ((*it).IsVisibleToTeam(team) || (flags & 0x10) ||
                        ((*it).IsDetectedByTeam(team) && ((*it).flags & STATIONARY))) {
                        if ((*it).GetOrder() != ORDER_IDLE || ((*it).flags & STATIONARY)) {
                            if (unit_flags & MOBILE_AIR_UNIT) {
                                if ((flags & 0x02) || ((flags & 0x01) && (*it).team != team)) {
                                    if ((*it).flags & MOBILE_AIR_UNIT) {
                                        return 0;
                                    }
                                }
                            } else {
                                if ((flags & 0x02) || ((flags & 0x01) && (*it).team != team)) {
                                    if ((*it).flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
                                        return 0;
                                    }
                                }

                                if ((unit_flags & STATIONARY) && unit_type != CNCT_4W &&
                                    (((*it).GetUnitType() == BRIDGE) ||
                                     ((*it).GetUnitType() >= LRGRUBLE && (*it).GetUnitType() <= SMLCONES) ||
                                     (*it).GetUnitType() == LANDMINE || (*it).GetUnitType() == SEAMINE)) {
                                    return 0;
                                }

                                if ((*it).flags & STATIONARY) {
                                    switch ((*it).GetUnitType()) {
                                        case ROAD: {
                                            if (unit_type == ROAD) {
                                                return 0;

                                            } else {
                                                result = 2;
                                            }
                                        } break;

                                        case BRIDGE: {
                                            result = 4;

                                            if (unit_flags & MOBILE_LAND_UNIT) {
                                                result = 2;

                                                if ((*it).IsBridgeElevated() && (flags & 0x8)) {
                                                    surface_type = SURFACE_TYPE_WATER;

                                                } else {
                                                    surface_type = SURFACE_TYPE_LAND;
                                                }

                                            } else {
                                                if (!(*it).IsBridgeElevated() && (flags & 0x08)) {
                                                    surface_type = SURFACE_TYPE_LAND;

                                                } else {
                                                    surface_type = SURFACE_TYPE_WATER;
                                                }
                                            }
                                        } break;

                                        case LANDMINE:
                                        case SEAMINE: {
                                            if ((*it).team != team && (*it).IsDetectedByTeam(team)) {
                                                return 0;
                                            }
                                        } break;

                                        case LRGRUBLE:
                                        case SMLRUBLE: {
                                            surface_type = SURFACE_TYPE_LAND;
                                        } break;

                                        case LRGSLAB:
                                        case SMLSLAB: {
                                            if (unit_type == ROAD) {
                                                return 0;

                                            } else {
                                                result = 2;
                                            }
                                        } break;

                                        case CNCT_4W: {
                                            if (unit_type == CNCT_4W) {
                                                return 0;

                                            } else if ((unit_flags & STATIONARY) && (*it).team != team &&
                                                       unit_type != WTRPLTFM && unit_type != BRIDGE &&
                                                       unit_type != ROAD) {
                                                return 0;
                                            }

                                        } break;

                                        case WALDO: {
                                        } break;

                                        case WTRPLTFM: {
                                            surface_type = SURFACE_TYPE_LAND;
                                            result = 4;
                                        } break;

                                        default: {
                                            return 0;
                                        } break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (!(UnitsManager_BaseUnits[unit_type].land_type & surface_type)) {
                result = 0;
            }
        }
    } else {
        result = 0;
    }

    return result;
}

bool Access_FindReachableSpotInt(ResourceID unit_type, UnitInfo* unit, int16_t* grid_x, int16_t* grid_y,
                                 int32_t range_limit, int32_t mode, int32_t direction) {
    UnitValues* unit_values;
    int32_t offset_x{0};
    int32_t offset_y{0};

    unit_values = unit->GetBaseValues();

    switch (direction) {
        case 0: {
            offset_x = 1;
            offset_y = 0;
        } break;

        case 1: {
            offset_x = 0;
            offset_y = 1;
        } break;

        case 2: {
            offset_x = -1;
            offset_y = 0;
        } break;

        case 3: {
            offset_x = 0;
            offset_y = -1;
        } break;

        default: {
            SDL_assert(0);
        } break;
    }

    for (int32_t i = 0; i < range_limit; ++i) {
        if (*grid_x >= 0 && *grid_x < ResourceManager_MapSize.x && *grid_y >= 0 &&
            *grid_y < ResourceManager_MapSize.y) {
            switch (mode) {
                case 0: {
                    if (Access_IsAccessible(unit_type, unit->team, *grid_x, *grid_y, 0x02)) {
                        return true;
                    }
                } break;

                case 1: {
                    if (Access_GetAttackTarget(unit, *grid_x, *grid_y)) {
                        return Access_IsWithinAttackRange(unit, *grid_x, *grid_y,
                                                          unit_values->GetAttribute(ATTRIB_RANGE));
                    }
                } break;
            }
        }

        *grid_x += offset_x;
        *grid_y += offset_y;
    }

    return false;
}

void Access_InitUnitStealthStatus(SmartList<UnitInfo>& units) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        (*it).InitStealthStatus();
    }
}

void Access_InitStealthMaps() {
    const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

    Access_InitUnitStealthStatus(UnitsManager_GroundCoverUnits);
    Access_InitUnitStealthStatus(UnitsManager_MobileLandSeaUnits);
    Access_InitUnitStealthStatus(UnitsManager_MobileAirUnits);
    Access_InitUnitStealthStatus(UnitsManager_ParticleUnits);
    Access_InitUnitStealthStatus(UnitsManager_StationaryUnits);

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            memset(UnitsManager_TeamInfo[team].heat_map_complete, 0, map_cell_count);
            memset(UnitsManager_TeamInfo[team].heat_map_stealth_sea, 0, map_cell_count);
            memset(UnitsManager_TeamInfo[team].heat_map_stealth_land, 0, map_cell_count);
        }
    }
}

bool Access_IsSurveyorOverlayActive(UnitInfo* unit) {
    bool result;

    if (unit->GetOrder() != ORDER_IDLE && unit->team == GameManager_PlayerTeam) {
        result = unit->GetUnitType() == SURVEYOR;
    } else {
        result = false;
    }

    return result;
}

bool Access_IsWithinScanRange(UnitInfo* unit, int32_t grid_x, int32_t grid_y, int32_t scan_range) {
    int32_t scan_area = (scan_range * 64) * (scan_range * 64);
    int32_t radius_x = ((grid_x - unit->grid_x) * 64);
    int32_t radius_y = ((grid_y - unit->grid_y) * 64);
    int32_t grid_area = radius_x * radius_x + radius_y * radius_y;

    return grid_area <= scan_area;
}

int32_t Access_GetDistance(int32_t grid_x, int32_t grid_y) { return grid_x * grid_x + grid_y * grid_y; }

int32_t Access_GetDistance(Point position1, Point position2) {
    position1 -= position2;

    return position1.x * position1.x + position1.y * position1.y;
}

int32_t Access_GetDistance(UnitInfo* unit, Point position) {
    position.x -= unit->grid_x;
    position.y -= unit->grid_y;

    return position.x * position.x + position.y * position.y;
}

int32_t Access_GetDistance(UnitInfo* unit1, UnitInfo* unit2) {
    Point distance;

    distance.x = unit1->grid_x - unit2->grid_x;
    distance.y = unit1->grid_y - unit2->grid_y;

    return distance.x * distance.x + distance.y * distance.y;
}

bool Access_IsWithinAttackRange(UnitInfo* unit, int32_t grid_x, int32_t grid_y, int32_t attack_range) {
    int32_t distance_x;
    int32_t distance_y;
    int32_t distance;
    int32_t ratio_x;
    int32_t ratio_y;
    bool result;

    attack_range *= 64;
    attack_range *= attack_range;
    distance_x = (grid_x - unit->grid_x) * 64;
    distance_y = (grid_y - unit->grid_y) * 64;
    distance = distance_x * distance_x + distance_y * distance_y;

    if (distance > attack_range) {
        result = false;
    } else {
        if (unit->GetUnitType() != SUBMARNE && unit->GetUnitType() != CORVETTE) {
            result = true;
        } else {
            distance = std::max(labs(distance_x), labs(distance_y)) * 64;

            if (distance) {
                ratio_x = (distance_x << 16) / distance;
                ratio_y = (distance_y << 16) / distance;
                distance_x = ratio_x + (unit->x << 16);
                distance_y = ratio_y + (unit->y << 16);

                for (; --distance; distance_x += ratio_x, distance_y += ratio_y) {
                    uint8_t surface_type;

                    surface_type = Access_GetSurfaceType((distance_x >> 16) >> 6, (distance_y >> 16) >> 6);

                    if (surface_type != SURFACE_TYPE_WATER && surface_type != SURFACE_TYPE_COAST) {
                        return false;
                    }
                }

                result = true;

            } else {
                result = true;
            }
        }
    }

    return result;
}

bool Access_FindReachableSpot(ResourceID unit_type, UnitInfo* unit, int16_t* grid_x, int16_t* grid_y, int32_t range,
                              int32_t exclusion_zone, int32_t mode) {
    for (int32_t i = 1; i <= range; ++i) {
        --*grid_x;
        --*grid_y;

        if (Access_FindReachableSpotInt(unit_type, unit, grid_x, grid_y, exclusion_zone + 2 * i, mode, 0) ||
            Access_FindReachableSpotInt(unit_type, unit, grid_x, grid_y, exclusion_zone + 2 * i, mode, 1) ||
            Access_FindReachableSpotInt(unit_type, unit, grid_x, grid_y, exclusion_zone + 2 * i, mode, 2) ||
            Access_FindReachableSpotInt(unit_type, unit, grid_x, grid_y, exclusion_zone + 2 * i, mode, 3)) {
            return true;
        }
    }

    return false;
}

uint32_t Access_GetAttackTargetGroup(UnitInfo* unit) {
    uint32_t result{TARGET_CLASS_NONE};

    if (unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) == 0 || unit->GetOrder() == ORDER_DISABLE) {
        result = TARGET_CLASS_NONE;

    } else {
        switch (unit->GetUnitType()) {
            case COMMANDO:
            case INFANTRY: {
                result = TARGET_CLASS_LAND_STEALTH | TARGET_CLASS_LAND | TARGET_CLASS_WATER;
            } break;

            case CORVETTE: {
                result = TARGET_CLASS_SEA_STEALTH | TARGET_CLASS_WATER;
            } break;

            case GUNTURRT:
            case ARTYTRRT:
            case ANTIMSSL:
            case SCOUT:
            case TANK:
            case ARTILLRY:
            case ROCKTLCH:
            case MISSLLCH:
            case FASTBOAT:
            case BATTLSHP:
            case MSSLBOAT:
            case BOMBER:
            case JUGGRNT:
            case ALNTANK:
            case ALNASGUN: {
                result = TARGET_CLASS_LAND | TARGET_CLASS_WATER;
            } break;

            case SUBMARNE: {
                result = TARGET_CLASS_WATER;
            } break;

            case ANTIAIR:
            case SP_FLAK:
            case FIGHTER: {
                result = TARGET_CLASS_AIR;
            } break;

            case ALNPLANE: {
                result = TARGET_CLASS_LAND | TARGET_CLASS_WATER | TARGET_CLASS_AIR;
            } break;

            case LANDMINE:
            case SEAMINE: {
                result = TARGET_CLASS_NONE;
            } break;

            default: {
                SDL_assert(0);
            } break;
        }
    }

    return result;
}

uint32_t Access_UpdateMapStatusAddUnit(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    uint32_t result;
    uint16_t team;
    int32_t map_offset;

    team = unit->team;
    map_offset = ResourceManager_MapSize.x * grid_y + grid_x;
    result = TARGET_CLASS_NONE;

    if (unit->GetUnitType() == CORVETTE) {
        ++UnitsManager_TeamInfo[team].heat_map_stealth_sea[map_offset];

        if (UnitsManager_TeamInfo[team].heat_map_stealth_sea[map_offset] == 1) {
            const auto units = Hash_MapHash[Point(grid_x, grid_y)];

            if (units) {
                // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                    if (UnitsManager_IsUnitUnderWater(&*it)) {
                        (*it).SpotByTeam(team);

                        if (unit->team != (*it).team) {
                            result |= Access_GetAttackTargetGroup(&*it);
                        }
                    }
                }
            }
        }
    }

    if (unit->GetUnitType() == COMMANDO || unit->GetUnitType() == INFANTRY) {
        ++UnitsManager_TeamInfo[team].heat_map_stealth_land[map_offset];

        if (UnitsManager_TeamInfo[team].heat_map_stealth_land[map_offset] == 1) {
            const auto units = Hash_MapHash[Point(grid_x, grid_y)];

            if (units) {
                // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                    if ((*it).GetUnitType() == COMMANDO) {
                        (*it).SpotByTeam(team);

                        if (unit->team != (*it).team) {
                            result |= Access_GetAttackTargetGroup(&*it);
                        }
                    }
                }
            }
        }
    }

    ++UnitsManager_TeamInfo[team].heat_map_complete[map_offset];

    if (UnitsManager_TeamInfo[team].heat_map_complete[map_offset] == 1) {
        Ai_SetInfoMapPoint(Point(grid_x, grid_y), team);

        if (team == GameManager_PlayerTeam) {
            ResourceManager_MinimapFov[map_offset] = ResourceManager_Minimap[map_offset];
        }

        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if (!(*it).IsVisibleToTeam(team)) {
                    (*it).DrawStealth(team);

                    if ((*it).IsVisibleToTeam(team) && (*it).team != unit->team) {
                        result |= Access_GetAttackTargetGroup(&*it);
                    }
                }
            }
        }
    }

    return result;
}

void Access_UpdateMapStatusRemoveUnit(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    uint16_t team;
    int32_t map_offset;

    team = unit->team;
    map_offset = ResourceManager_MapSize.x * grid_y + grid_x;

    if (unit->GetUnitType() == CORVETTE) {
        --UnitsManager_TeamInfo[team].heat_map_stealth_sea[map_offset];
    }

    if (unit->GetUnitType() == COMMANDO || unit->GetUnitType() == INFANTRY) {
        --UnitsManager_TeamInfo[team].heat_map_stealth_land[map_offset];
    }

    --UnitsManager_TeamInfo[team].heat_map_complete[map_offset];

    SDL_assert(UnitsManager_TeamInfo[team].heat_map_complete[map_offset] >= 0);
    SDL_assert(UnitsManager_TeamInfo[team].heat_map_stealth_land[map_offset] >= 0);
    SDL_assert(UnitsManager_TeamInfo[team].heat_map_stealth_sea[map_offset] >= 0);

    if (0 == UnitsManager_TeamInfo[team].heat_map_complete[map_offset]) {
        Ai_UpdateMineMap(Point(grid_x, grid_y), team);

        if (team == GameManager_PlayerTeam) {
            ResourceManager_MinimapFov[map_offset] =
                ResourceManager_ColorIndexTable12[ResourceManager_Minimap[map_offset]];
        }

        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            auto it = units->Begin();
            auto end = units->End();

            if (it != end) {
                if ((GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) &&
                    GameManager_LockedUnits.GetCount()) {
                    GameManager_UpdateDrawBounds();
                }

                for (; it != end; ++it) {
                    (*it).Draw(team);
                }
            }
        }
    }
}

void Access_DrawUnit(UnitInfo* unit) {
    const int32_t map_offset{ResourceManager_MapSize.x * unit->grid_y + unit->grid_x};

    if (GameManager_AllVisible) {
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                unit->SpotByTeam(team);
            }
        }
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            if (UnitsManager_TeamInfo[team].heat_map_stealth_sea[map_offset] && UnitsManager_IsUnitUnderWater(unit)) {
                unit->SpotByTeam(team);
            }

            if (unit->GetUnitType() == COMMANDO) {
                if (UnitsManager_TeamInfo[team].heat_map_stealth_land[map_offset]) {
                    unit->SpotByTeam(team);
                }
            }

            if (UnitsManager_TeamInfo[team].heat_map_complete[map_offset]) {
                unit->DrawStealth(team);
            } else {
                unit->Draw(team);
            }
        }
    }
}

uint32_t Access_GetTargetClass(UnitInfo* unit) {
    uint32_t result;

    if (unit->GetUnitType() == COMMANDO || UnitsManager_IsUnitUnderWater(unit)) {
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (unit->team != team && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                unit->IsVisibleToTeam(team)) {
                if (unit->GetUnitType() == COMMANDO) {
                    result = TARGET_CLASS_LAND;
                } else {
                    result = TARGET_CLASS_WATER;
                }

                return result;
            }
        }

        if (unit->GetUnitType() == COMMANDO) {
            result = TARGET_CLASS_LAND_STEALTH;
        } else {
            result = TARGET_CLASS_SEA_STEALTH;
        }

    } else {
        if (unit->flags & MOBILE_AIR_UNIT) {
            result = TARGET_CLASS_AIR;

        } else if (unit->flags & MOBILE_SEA_UNIT) {
            result = TARGET_CLASS_WATER;

        } else {
            result = TARGET_CLASS_LAND;
        }
    }

    return result;
}

void Access_UpdateMapStatus(UnitInfo* unit, bool mode) {
    if (unit->GetOrder() != ORDER_DISABLE) {
        if ((unit->GetUnitType() == SURVEYOR || unit->GetUnitType() == MINELAYR || unit->GetUnitType() == SEAMNLYR ||
             unit->GetUnitType() == COMMANDO) &&
            mode) {
            Survey_SurveyArea(unit, 1);
        }

        if (GameManager_AllVisible) {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                    unit->SpotByTeam(team);
                }
            }

        } else if (unit->GetOrder() != ORDER_IDLE) {
            if (mode) {
                Access_DrawUnit(unit);

            } else {
                for (int32_t team = PLAYER_TEAM_GRAY; team >= PLAYER_TEAM_RED; --team) {
                    unit->Draw(team);
                }
            }

            if ((unit->flags & SELECTABLE) && UnitsManager_TeamInfo[unit->team].team_type != TEAM_TYPE_NONE) {
                uint8_t enemy_target_class = TARGET_CLASS_NONE;
                int32_t scan = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);
                Rect zone;

                if (unit->GetOrder() == ORDER_DISABLE) {
                    scan = 0;
                }

                rect_init(&zone, unit->grid_x - scan, unit->grid_y - scan, unit->grid_x + scan, unit->grid_y + scan);

                if (zone.ulx < 0) {
                    zone.ulx = 0;
                }

                if (zone.uly < 0) {
                    zone.uly = 0;
                }

                if (zone.lrx > ResourceManager_MapSize.x - 1) {
                    zone.lrx = ResourceManager_MapSize.x - 1;
                }

                if (zone.lry > ResourceManager_MapSize.y - 1) {
                    zone.lry = ResourceManager_MapSize.y - 1;
                }

                for (int32_t grid_y = zone.uly; grid_y <= zone.lry; ++grid_y) {
                    for (int32_t grid_x = zone.ulx; grid_x <= zone.lrx; ++grid_x) {
                        if (Access_IsWithinScanRange(unit, grid_x, grid_y, scan)) {
                            if (mode) {
                                enemy_target_class |= Access_UpdateMapStatusAddUnit(unit, grid_x, grid_y);

                            } else {
                                Access_UpdateMapStatusRemoveUnit(unit, grid_x, grid_y);
                            }
                        }
                    }
                }

                if (enemy_target_class != TARGET_CLASS_NONE &&
                    (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_PLAYER ||
                     UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) &&
                    unit->GetOrder() != ORDER_AWAIT && ini_get_setting(INI_ENEMY_HALT)) {
                    uint32_t friendly_target_class = Access_GetTargetClass(unit);

                    if (unit->GetUnitList()) {
                        for (SmartList<UnitInfo>::Iterator it = unit->GetUnitList()->Begin();
                             it != unit->GetUnitList()->End(); ++it) {
                            friendly_target_class |= Access_GetTargetClass(&*it);
                        }
                    }

                    if (friendly_target_class & enemy_target_class) {
                        AiLog log("Access: %s at [%i,%i] spotted enemies",
                                  UnitsManager_BaseUnits[unit->GetUnitType()].singular_name, unit->grid_x + 1,
                                  unit->grid_y + 1);

                        if (unit->GetUnitList()) {
                            for (SmartList<UnitInfo>::Iterator it = unit->GetUnitList()->Begin();
                                 it != unit->GetUnitList()->End(); ++it) {
                                UnitEventEmergencyStop* unit_event = new (std::nothrow) UnitEventEmergencyStop(&*it);

                                UnitEvent_UnitEvents.PushBack(*unit_event);

                                if (Remote_IsNetworkGame) {
                                    Remote_SendNetPacket_50(it->Get());
                                }
                            }

                        } else {
                            UnitEventEmergencyStop* unit_event = new (std::nothrow) UnitEventEmergencyStop(unit);

                            UnitEvent_UnitEvents.PushBack(*unit_event);

                            if (Remote_IsNetworkGame) {
                                Remote_SendNetPacket_50(unit);
                            }
                        }
                    }
                }
            }
        }
    }
}

void Access_UpdateUnitVisibilityStatus(SmartList<UnitInfo>& units) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        Access_UpdateMapStatus(&*it, true);
    }
}

void Access_UpdateVisibilityStatus(bool all_visible) {
    GameManager_AllVisible = all_visible;
    Access_InitStealthMaps();

    if (!GameManager_AllVisible) {
        Access_UpdateUnitVisibilityStatus(UnitsManager_MobileAirUnits);
        Access_UpdateUnitVisibilityStatus(UnitsManager_GroundCoverUnits);
        Access_UpdateUnitVisibilityStatus(UnitsManager_MobileLandSeaUnits);
        Access_UpdateUnitVisibilityStatus(UnitsManager_StationaryUnits);
        Access_UpdateUnitVisibilityStatus(UnitsManager_ParticleUnits);
    }

    Access_UpdateMinimapFogOfWar(GameManager_PlayerTeam, all_visible);
    GameManager_UpdateDrawBounds();
}

void Access_UpdateMinimapFogOfWar(uint16_t team, bool all_visible, bool ignore_team_heat_map) {
    const uint32_t map_cell_count{static_cast<uint32_t>(ResourceManager_MapSize.x * ResourceManager_MapSize.y)};

    memcpy(ResourceManager_MinimapFov, ResourceManager_Minimap, map_cell_count);

    if (!all_visible) {
        for (uint32_t i = 0; i < map_cell_count; ++i) {
            if (UnitsManager_TeamInfo[team].heat_map_complete[i] == 0 || ignore_team_heat_map) {
                ResourceManager_MinimapFov[i] = ResourceManager_ColorIndexTable12[ResourceManager_MinimapFov[i]];
            }
        }
    }
}

void Access_UpdateResourcesTotal(Complex* complex) {
    Cargo inventory;

    complex->material = 0;
    complex->fuel = 0;
    complex->gold = 0;
    complex->power = 0;
    complex->workers = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetComplex() == complex) {
            inventory = Cargo_GetNetProduction(it->Get());
            inventory += Cargo_GetInventory(it->Get());

            complex->material += inventory.raw;
            complex->fuel += inventory.fuel;
            complex->gold += inventory.gold;
            complex->power += inventory.power;
            complex->workers += inventory.life;
        }
    }
}

uint8_t Access_GetSurfaceType(int32_t grid_x, int32_t grid_y) {
    uint8_t result;

    if (ResourceManager_MapSurfaceMap) {
        result = ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * grid_y + grid_x];
    } else {
        result = SURFACE_TYPE_NONE;
    }

    return result;
}

uint8_t Access_GetModifiedSurfaceType(int32_t grid_x, int32_t grid_y) {
    uint8_t surface_type;

    surface_type = Access_GetSurfaceType(grid_x, grid_y);

    if (surface_type == SURFACE_TYPE_WATER || surface_type == SURFACE_TYPE_COAST) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).GetUnitType() == WTRPLTFM || (*it).GetUnitType() == BRIDGE) {
                    surface_type = SURFACE_TYPE_LAND;
                }
            }
        }
    }

    return surface_type;
}

bool Access_IsAnyLandPresent(int32_t grid_x, int32_t grid_y, uint32_t flags) {
    bool result;

    if (((flags & BUILDING) && (Access_GetSurfaceType(grid_x + 1, grid_y) == SURFACE_TYPE_LAND ||
                                Access_GetSurfaceType(grid_x, grid_y + 1) == SURFACE_TYPE_LAND ||
                                Access_GetSurfaceType(grid_x + 1, grid_y + 1) == SURFACE_TYPE_LAND)) ||
        Access_GetSurfaceType(grid_x, grid_y) == SURFACE_TYPE_LAND) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool Access_IsFullyLandCovered(int32_t grid_x, int32_t grid_y, uint32_t flags) {
    bool result;

    if ((!(flags & BUILDING) || (Access_GetSurfaceType(grid_x + 1, grid_y) == SURFACE_TYPE_LAND &&
                                 Access_GetSurfaceType(grid_x, grid_y + 1) == SURFACE_TYPE_LAND &&
                                 Access_GetSurfaceType(grid_x + 1, grid_y + 1) == SURFACE_TYPE_LAND)) &&
        Access_GetSurfaceType(grid_x, grid_y) == SURFACE_TYPE_LAND) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

UnitInfo* Access_GetRemovableRubble(uint16_t team, int32_t grid_x, int32_t grid_y) {
    UnitInfo* unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).GetUnitType() == SMLRUBLE) {
                    unit = &*it;
                    break;
                }

                if ((*it).GetUnitType() == LRGRUBLE) {
                    for (int32_t y = (*it).grid_y; y <= (*it).grid_y + 1; ++y) {
                        for (int32_t x = (*it).grid_x; x <= (*it).grid_x + 1; ++x) {
                            if (x != grid_x || y != grid_y) {
                                if (!Access_IsAccessible(BULLDOZR, team, x, y, 2)) {
                                    return nullptr;
                                }
                            }
                        }
                    }

                    unit = &*it;
                    break;
                }
            }
        }
    }

    return unit;
}

int32_t Access_FindUnitInUnitList(UnitInfo* unit) {
    int32_t result;

    result = -1;

    for (uint32_t i = 0; i < std::size(Access_UnitsLists); ++i) {
        if (Access_UnitsLists[i]->Find(*unit) != Access_UnitsLists[i]->End()) {
            result = i;
            break;
        }
    }

    return result;
}

bool Access_IsTeamInUnitList(uint16_t team, SmartList<UnitInfo>& units) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team) {
            return true;
        }
    }

    return false;
}

bool Access_IsTeamInUnitLists(uint16_t team) {
    return Access_IsTeamInUnitList(team, UnitsManager_MobileLandSeaUnits) ||
           Access_IsTeamInUnitList(team, UnitsManager_MobileAirUnits) ||
           Access_IsTeamInUnitList(team, UnitsManager_StationaryUnits);
}

UnitInfo* Access_GetSelectableUnit(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    UnitInfo* unit2;

    Hash_MapHash.Remove(unit);
    Hash_MapHash.Add(unit, true);

    unit2 = Access_GetUnit3(grid_x, grid_y, SELECTABLE);

    if (unit2) {
        if (unit2->GetOrder() == ORDER_IDLE || (unit2->flags & GROUND_COVER)) {
            unit2 = unit;
        }

        if (unit == unit2) {
            Hash_MapHash.Remove(unit);
            Hash_MapHash.Add(unit);
        }
    }

    return unit2;
}

UnitInfo* Access_GetFirstMiningStation(uint16_t team) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == MININGST) {
            return &*it;
        }
    }

    return nullptr;
}

UnitInfo* Access_GetFirstActiveUnit(uint16_t team, SmartList<UnitInfo>& units) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team && (*it).GetOrder() != ORDER_IDLE) {
            return &*it;
        }
    }

    return nullptr;
}

void Access_RenewAttackOrders(SmartList<UnitInfo>& units, uint16_t team) {
    UnitInfo* unit;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        unit = (*it).GetFirstFromUnitList();

        if (unit) {
            if ((*it).team == team && (&(*it) == unit)) {
                Access_GroupAttackOrder(unit, 0);
            }

        } else {
            if (((*it).GetOrder() == ORDER_MOVE || (*it).GetOrder() == ORDER_MOVE_TO_UNIT ||
                 (*it).GetOrder() == ORDER_MOVE_TO_ATTACK) &&
                (*it).GetOrderState() == ORDER_STATE_EXECUTING_ORDER) {
                if ((*it).team == team && (*it).speed > 0 && (*it).engine == 2) {
                    UnitsManager_SetNewOrder(&(*it), (*it).GetOrder(), ORDER_STATE_INIT);
                }
            }
        }
    }
}

bool Access_UpdateGroupSpeed(UnitInfo* unit) {
    int32_t max_speed = INT32_MAX;
    SmartList<UnitInfo>* units;

    units = unit->GetUnitList();

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (((*it).GetOrder() == ORDER_MOVE || (*it).GetOrder() == ORDER_MOVE_TO_ATTACK) &&
            ((*it).GetOrderState() == ORDER_STATE_IN_PROGRESS || (*it).GetOrderState() == ORDER_STATE_IN_TRANSITION ||
             (*it).GetOrderState() == ORDER_STATE_NEW_ORDER)) {
            return false;
        }

        if ((*it).speed < max_speed) {
            max_speed = (*it).speed;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        (*it).group_speed = max_speed + 1;
    }

    return true;
}

void Access_GroupAttackOrder(UnitInfo* unit, bool mode) {
    if (Access_UpdateGroupSpeed(unit)) {
        UnitInfo* enemy{nullptr};
        UnitInfo* friendly;

        if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK) {
            enemy = unit->GetEnemy();

            SDL_assert(enemy != nullptr);
        }

        SmartList<UnitInfo>* units = unit->GetUnitList();

        for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
            friendly = &*it;

            if ((mode || (friendly->group_speed - 1) != 0) &&
                (friendly->GetOrder() == ORDER_MOVE || friendly->GetOrder() == ORDER_MOVE_TO_ATTACK ||
                 friendly->GetOrder() == ORDER_AWAIT)) {
                if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK) {
                    if (friendly->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) &&
                        Access_IsValidAttackTarget(friendly, enemy)) {
                        friendly->group_speed = 0;
                        friendly->target_grid_x = unit->target_grid_x;
                        friendly->target_grid_y = unit->target_grid_y;
                        friendly->SetEnemy(enemy);
                        UnitsManager_SetNewOrder(friendly, ORDER_MOVE_TO_ATTACK, ORDER_STATE_NEW_ORDER);
                        Paths_RequestPath(friendly, 2);
                    }

                } else {
                    friendly->target_grid_x = friendly->attack_site.x - unit->attack_site.x + unit->target_grid_x;
                    friendly->target_grid_y = friendly->attack_site.y - unit->attack_site.y + unit->target_grid_y;

                    if (friendly->target_grid_x < 0) {
                        friendly->target_grid_x = 0;

                    } else if (friendly->target_grid_x >= ResourceManager_MapSize.x) {
                        friendly->target_grid_x = ResourceManager_MapSize.x - 1;
                    }

                    if (friendly->target_grid_y < 0) {
                        friendly->target_grid_y = 0;

                    } else if (friendly->target_grid_y >= ResourceManager_MapSize.y) {
                        friendly->target_grid_y = ResourceManager_MapSize.y - 1;
                    }

                    if (friendly->grid_x != friendly->target_grid_x || friendly->grid_y != friendly->target_grid_y) {
                        friendly->path = nullptr;

                        if (friendly->GetOrder() == ORDER_MOVE_TO_ATTACK) {
                            friendly->SetOrder(ORDER_AWAIT);
                            friendly->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
                        }

                        if (friendly->flags & MOBILE_AIR_UNIT) {
                            UnitsManager_SetNewOrder(friendly, ORDER_MOVE, ORDER_STATE_NEW_ORDER);
                            UnitsManager_Unit = friendly;

                        } else if (Paths_RequestPath(friendly, 1)) {
                            UnitsManager_SetNewOrder(friendly, ORDER_MOVE, ORDER_STATE_NEW_ORDER);
                        }
                    }
                }
            }
        }
    }
}

int32_t Access_GetStoredUnitCount(UnitInfo* unit) {
    SmartList<UnitInfo>* units = nullptr;
    int32_t result = 0;

    if (unit->GetUnitType() == HANGAR) {
        units = &UnitsManager_MobileAirUnits;

    } else {
        units = &UnitsManager_MobileLandSeaUnits;
    }

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).GetOrder() == ORDER_IDLE && (*it).GetParent() == unit) {
            ++result;
        }
    }

    return result;
}

int32_t Access_GetRepairShopClientCount(uint16_t team, ResourceID unit_type) {
    int32_t result = 0;
    uint32_t flags;
    SmartList<UnitInfo>* units;

    switch (unit_type) {
        case DEPOT: {
            units = &UnitsManager_MobileLandSeaUnits;
            flags = (MOBILE_LAND_UNIT | ELECTRONIC_UNIT);

        } break;

        case HANGAR: {
            units = &UnitsManager_MobileAirUnits;
            flags = (MOBILE_AIR_UNIT | ELECTRONIC_UNIT);
        } break;

        case DOCK: {
            units = &UnitsManager_MobileLandSeaUnits;
            flags = (MOBILE_SEA_UNIT | ELECTRONIC_UNIT);
        } break;

        case BARRACKS: {
            units = &UnitsManager_MobileLandSeaUnits;
            flags = (MOBILE_LAND_UNIT);
        } break;

        default: {
            return 0;
        } break;
    }

    for (auto it = units->Begin(); it != units->End(); ++it) {
        const uint32_t unit_flags =
            (*it).flags & (MOBILE_AIR_UNIT | MOBILE_LAND_UNIT | MOBILE_SEA_UNIT | ELECTRONIC_UNIT);

        if ((*it).team == team && (unit_flags == flags) && (*it).hits > 0) {
            ++result;
        }
    }

    return result;
}

bool Access_IsWithinMovementRange(UnitInfo* unit) {
    bool result;

    if (unit->speed) {
        if (unit->path != nullptr) {
            if (unit->path->GetMovementCost(unit) < (unit->speed * 4 + unit->move_fraction)) {
                result = true;
            } else {
                result = false;
            }

        } else {
            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

bool Access_IsValidNextUnit(UnitInfo* unit) {
    SmartPointer<UnitValues> unit_values(unit->GetBaseValues());
    int16_t grid_x = unit->grid_x;
    int16_t grid_y = unit->grid_y;
    bool result;

    if (unit->GetOrder() == ORDER_SENTRY || unit->GetOrder() == ORDER_IDLE || unit->GetOrder() == ORDER_DISABLE ||
        unit->GetOrder() == ORDER_EXPLODE || unit->GetOrderState() == ORDER_STATE_DESTROY ||
        unit->GetOrderState() == ORDER_STATE_READY_TO_EXECUTE_ORDER ||
        (unit->GetOrder() == ORDER_BUILD && unit->GetOrderState() != ORDER_STATE_UNIT_READY)) {
        result = false;

    } else if (Access_IsWithinMovementRange(unit) ||
               (unit->shots > 0 && Access_FindReachableSpot(unit->GetUnitType(), unit, &grid_x, &grid_y,
                                                            unit_values->GetAttribute(ATTRIB_RANGE), 0, 1))) {
        result = true;

    } else {
        result = unit->popup->test(unit);
    }

    return result;
}

UnitInfo* Access_SeekNextUnit(uint16_t team, UnitInfo* unit, bool seek_direction) {
    int32_t list_index;
    int32_t seek_index;
    SmartList<UnitInfo>::Iterator it;
    SmartList<UnitInfo>::Iterator it_end;
    UnitInfo* result;

    list_index = Access_FindUnitInUnitList(unit);

    if (list_index >= 0) {
        it = Access_UnitsLists[list_index]->Find(*unit);
        it_end = Access_UnitsLists[list_index]->End();

    } else if (seek_direction) {
        list_index = 4;

        it = --Access_UnitsLists[list_index]->End();
        it_end = Access_UnitsLists[list_index]->End();

    } else {
        list_index = 0;

        it = Access_UnitsLists[list_index]->Begin();
        it_end = Access_UnitsLists[list_index]->End();
    }

    seek_index = list_index;

    if (seek_direction) {
        do {
            if (it != it_end) {
                ++it;
            }

            do {
                if (it == it_end) {
                    ++seek_index;

                    if (seek_index > 4) {
                        seek_index = 0;
                    }

                    it = Access_UnitsLists[seek_index]->Begin();
                    it_end = Access_UnitsLists[seek_index]->End();
                }
            } while (seek_index != list_index && it == it_end);
        } while (it != it_end && &*it != unit && ((*it).team != team || !Access_IsValidNextUnit(&*it)));

    } else {
        do {
            if (it != it_end) {
                --it;
            }

            do {
                if (it == it_end) {
                    --seek_index;

                    if (seek_index < 0) {
                        seek_index = 4;
                    }

                    it = --Access_UnitsLists[seek_index]->End();
                    it_end = Access_UnitsLists[seek_index]->End();
                }
            } while (seek_index != list_index && it == it_end);
        } while (it != it_end && &*it != unit && ((*it).team != team || !Access_IsValidNextUnit(&*it)));
    }

    if (it != it_end && Access_IsValidNextUnit(&*it)) {
        result = &*it;

    } else {
        result = nullptr;
    }

    return result;
}

bool Access_IsChildOfUnitInList(UnitInfo* unit, SmartList<UnitInfo>* list, SmartList<UnitInfo>::Iterator* it) {
    for (; *it != list->End(); ++*it) {
        if ((*(*it)).GetOrder() == ORDER_IDLE && (*(*it)).GetParent() == unit) {
            break;
        }
    }

    return *it != list->End();
}

UnitInfo* Access_GetQuickBuilderUnit(int32_t grid_x, int32_t grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if (((*it).flags & GROUND_COVER) || (*it).GetUnitType() == HARVSTER) {
                    unit = &*it;
                    break;
                }
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetUnit3(int32_t grid_x, int32_t grid_y, uint32_t flags) {
    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        SmartPointer<UnitInfo> unit;
        SmartList<UnitInfo>::Iterator it, end;
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (it = units->Begin(), end = units->End(), unit = *it; it != end;) {
                if (((*it).flags & flags) && !((*it).flags & GROUND_COVER) && (*it).GetOrder() != ORDER_IDLE) {
                    return &*it;
                }

                Hash_MapHash.Remove(&*it);
                Hash_MapHash.Add(&*it);

                ++it;

                if (it != end) {
                    if (*it == unit) {
                        if ((*it).flags & flags) {
                            return &*it;

                        } else {
                            return nullptr;
                        }

                    } else if (unit->GetUnitType() == LRGTAPE || unit->GetUnitType() == SMLTAPE) {
                        unit = *it;
                    }
                }
            }
        }
    }

    return nullptr;
}

UnitInfo* Access_GetUnit1(int32_t grid_x, int32_t grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).GetUnitType() == BRIDGE) {
                    unit = &*it;
                    break;
                }
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetUnit2(int32_t grid_x, int32_t grid_y, uint16_t team) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).team == team && ((*it).flags & SELECTABLE) && !((*it).flags & (HOVERING | GROUND_COVER)) &&
                    (*it).GetUnitType() != LANDPAD && (*it).GetOrder() != ORDER_IDLE && (*it).GetId() != 0xFFFF) {
                    unit = &*it;
                    break;
                }
            }
        }

        if (unit && (unit->GetUnitType() == LRGTAPE || unit->GetUnitType() == SMLTAPE)) {
            unit = unit->GetParent();
        }
    }

    return unit;
}

void Access_DestroyUtilities(int32_t grid_x, int32_t grid_y, bool remove_slabs, bool remove_rubble,
                             bool remove_connectors, bool remove_road) {
    const auto units = Hash_MapHash[Point(grid_x, grid_y)];

    if (units) {
        // the end node must be cached in case Hash_MapHash.Remove() deletes the list
        for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
            switch ((*it).GetUnitType()) {
                case SMLTAPE:
                case LRGTAPE:
                case SMLCONES:
                case LRGCONES: {
                    UnitsManager_DestroyUnit(&*it);
                } break;

                case ROAD: {
                    if (remove_road) {
                        UnitsManager_DestroyUnit(&*it);
                    }
                } break;

                case SMLSLAB:
                case LRGSLAB: {
                    if (remove_slabs) {
                        UnitsManager_DestroyUnit(&*it);
                    }
                } break;

                case CNCT_4W: {
                    if (remove_connectors) {
                        UnitsManager_RemoveConnections(&*it);
                        UnitsManager_DestroyUnit(&*it);
                    }
                } break;

                case SMLRUBLE:
                case LRGRUBLE: {
                    if (remove_rubble) {
                        UnitsManager_DestroyUnit(&*it);
                    }
                } break;
            }
        }
    }
}

void Access_DestroyGroundCovers(int32_t grid_x, int32_t grid_y) {
    const auto units = Hash_MapHash[Point(grid_x, grid_y)];

    if (units) {
        // the end node must be cached in case Hash_MapHash.Remove() deletes the list
        for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
            switch ((*it).GetUnitType()) {
                case ROAD:
                case LANDPAD:
                case WTRPLTFM:
                case BRIDGE: {
                    UnitsManager_DestroyUnit(&*it);
                } break;
            }
        }
    }
}

UnitInfo* Access_GetEnemyUnit(uint16_t team, int32_t grid_x, int32_t grid_y, uint32_t flags) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).team != team && ((*it).IsVisibleToTeam(team) || GameManager_MaxSpy) &&
                    (*it).GetOrder() != ORDER_IDLE && ((*it).flags & flags) && (*it).GetOrder() != ORDER_EXPLODE &&
                    (*it).GetOrderState() != ORDER_STATE_DESTROY) {
                    unit = &*it;
                    break;
                }
            }
        }

        if (unit && (unit->GetUnitType() == LRGTAPE || unit->GetUnitType() == SMLTAPE)) {
            unit = unit->GetParent();
        }
    }

    return unit;
}

UnitInfo* Access_GetConstructionUtility(uint16_t team, int32_t grid_x, int32_t grid_y) {
    UnitInfo* unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).team == team && (*it).GetUnitType() >= LRGTAPE && (*it).GetUnitType() <= SMLCONES) {
                    unit = &*it;
                    break;
                }
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetTeamUnit(int32_t grid_x, int32_t grid_y, uint16_t team, uint32_t flags) {
    SmartPointer<UnitInfo> unit;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).team == team && ((*it).flags & flags) && (*it).GetOrder() != ORDER_IDLE &&
                    (*it).GetId() != 0xFFFF && (*it).GetUnitType() != LRGTAPE && (*it).GetUnitType() != SMLTAPE &&
                    !((*it).flags & GROUND_COVER)) {
                    unit = *it;
                    break;
                }
            }

            if (!unit) {
                // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                    if ((*it).team == team && ((*it).flags & flags) && (*it).GetOrder() != ORDER_IDLE &&
                        (*it).GetId() != 0xFFFF) {
                        unit = *it;
                        break;
                    }
                }
            }
        }

        if (unit && (unit->GetUnitType() == LRGTAPE || unit->GetUnitType() == SMLTAPE)) {
            unit = unit->GetParent();
        }
    }

    return unit.Get();
}

uint32_t Access_GetValidAttackTargetTypes(ResourceID unit_type) {
    uint32_t target_types;

    switch (unit_type) {
        case SCOUT:
        case COMMANDO:
        case INFANTRY:
        case CORVETTE:
        case BATTLSHP:
        case SUBMARNE:
        case MSSLBOAT:
        case BOMBER:
        case JUGGRNT:
        case ALNTANK:
        case ALNASGUN:
        case TANK:
        case ARTILLRY:
        case ROCKTLCH:
        case MISSLLCH:
        case GUNTURRT:
        case ARTYTRRT:
        case ANTIMSSL: {
            target_types = MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | STATIONARY;
        } break;

        case FASTBOAT:
        case FIGHTER:
        case SP_FLAK:
        case ANTIAIR: {
            target_types = MOBILE_AIR_UNIT;
        } break;

        case ALNPLANE: {
            target_types = MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | STATIONARY;
        } break;

        case LANDMINE:
        case SEAMINE: {
            target_types = MOBILE_SEA_UNIT | MOBILE_LAND_UNIT;
        } break;

        default: {
            target_types = 0;
        } break;
    }

    return target_types;
}

bool Access_IsValidAttackTargetTypeEx(ResourceID attacker, ResourceID target, uint32_t target_flags) {
    switch (target) {
        case BRIDGE:
        case WTRPLTFM:
        case SEAMINE: {
            target_flags = MOBILE_LAND_UNIT;
        } break;

        case SUBMARNE: {
            switch (attacker) {
                case BOMBER:
                case SUBMARNE:
                case CORVETTE:
                case ALNPLANE:
                case SEAMINE: {
                } break;

                default: {
                    return false;
                } break;
            }
        } break;

        case LRGTAPE:
        case SMLTAPE: {
            return false;
        } break;
    }

    return (target_flags & Access_GetValidAttackTargetTypes(attacker)) != 0;
}

bool Access_IsValidAttackTargetEx(ResourceID attacker, ResourceID target, uint32_t target_flags, Point point) {
    uint8_t surface_type;
    bool result;

    surface_type = Access_GetSurfaceType(point.x, point.y);

    if ((attacker == SUBMARNE || attacker == CORVETTE) && (surface_type != SURFACE_TYPE_WATER) &&
        surface_type != SURFACE_TYPE_COAST) {
        result = false;

    } else {
        result = Access_IsValidAttackTargetTypeEx(attacker, target, target_flags);
    }

    return result;
}

bool Access_IsValidAttackTargetType(ResourceID attacker, ResourceID target) {
    return Access_IsValidAttackTargetTypeEx(attacker, target, UnitsManager_BaseUnits[target].flags);
}

bool Access_IsValidAttackTarget(ResourceID attacker, ResourceID target, Point point) {
    return Access_IsValidAttackTargetEx(attacker, target, UnitsManager_BaseUnits[target].flags, point);
}

bool Access_IsUnitBusyAtLocation(UnitInfo* unit) {
    bool result;

    result = false;

    const auto units = Hash_MapHash[Point(unit->grid_x, unit->grid_y)];

    if (units) {
        // the end node must be cached in case Hash_MapHash.Remove() deletes the list
        for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
            if (((*it).flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | STATIONARY)) && !((*it).flags & GROUND_COVER) &&
                (*it).GetOrder() != ORDER_IDLE) {
                result = true;
                break;
            }
        }
    }

    return result;
}

bool Access_IsValidAttackTarget(UnitInfo* attacker, UnitInfo* target, Point point) {
    uint32_t target_flags;
    ResourceID target_type;
    bool result;

    target_flags = target->flags;
    target_type = target->GetUnitType();

    if (!(attacker->flags & MOBILE_AIR_UNIT) || (attacker->flags & HOVERING)) {
        if ((target_flags & MOBILE_AIR_UNIT) && !(target_flags & HOVERING)) {
            target_flags &= ~MOBILE_AIR_UNIT;
            target_flags |= MOBILE_LAND_UNIT;
        }

        if (target_type == LRGTAPE || target_type == SMLTAPE) {
            target_type = target->GetParent()->GetUnitType();
        }

        result = Access_IsValidAttackTargetEx(attacker->GetUnitType(), target_type, target_flags, point);

    } else {
        result = false;
    }

    return result;
}

bool Access_IsValidAttackTarget(UnitInfo* attacker, UnitInfo* target) {
    return Access_IsValidAttackTarget(attacker, target, Point(target->grid_x, target->grid_y));
}

UnitInfo* Access_GetAttackTarget(UnitInfo* unit, int32_t grid_x, int32_t grid_y, bool mode) {
    UnitInfo* result{nullptr};
    bool normal_unit{false};

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).team == unit->team) {
                    if (!((*it).flags & MISSILE_UNIT)) {
                        normal_unit = true;
                    }

                } else if (((*it).flags & SELECTABLE) && (*it).GetOrder() != ORDER_EXPLODE &&
                           (*it).GetOrderState() != ORDER_STATE_DESTROY && (*it).GetOrder() != ORDER_IDLE &&
                           Access_IsValidAttackTarget(unit, &*it, Point(grid_x, grid_y))) {
                    if (mode) {
                        (*it).SpotByTeam(unit->team);
                    }

                    if ((*it).IsVisibleToTeam(unit->team)) {
                        result = &*it;
                        break;
                    }
                }
            }
        }

        if (result) {
            if ((result->GetUnitType() == LRGTAPE || result->GetUnitType() == SMLTAPE)) {
                result = result->GetParent();

            } else if (normal_unit && (result->flags & GROUND_COVER)) {
                result = nullptr;
            }
        }
    }

    return result;
}

UnitInfo* Access_GetEnemyMineOnSentry(uint16_t team, int32_t grid_x, int32_t grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if (((*it).GetUnitType() == LANDMINE || (*it).GetUnitType() == SEAMINE) && (*it).team != team &&
                    (*it).GetOrder() == ORDER_SENTRY) {
                    unit = &*it;
                    break;
                }
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetAttackTarget2(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    UnitInfo* result;

    result = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).team != unit->team && !(*it).IsVisibleToTeam(unit->team) &&
                    Access_IsValidAttackTarget(unit, &*it)) {
                    result = &*it;
                    break;
                }
            }
        }
    }

    return result;
}

UnitInfo* Access_GetReceiverUnit(UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    UnitInfo* result;
    uint16_t team;
    uint32_t flags;

    result = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        team = unit->team;
        flags = unit->flags;

        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).team == team && (*it).GetOrder() != ORDER_IDLE) {
                    if (flags & MOBILE_LAND_UNIT) {
                        if (((*it).GetUnitType() == BARRACKS || (*it).GetUnitType() == CLNTRANS ||
                             (*it).GetUnitType() == SEATRANS) &&
                            (unit->GetUnitType() == COMMANDO || unit->GetUnitType() == INFANTRY)) {
                            result = &*it;
                            break;

                        } else if (((*it).GetUnitType() == DEPOT || (*it).GetUnitType() == SEATRANS) &&
                                   (unit->GetUnitType() != COMMANDO && unit->GetUnitType() != INFANTRY)) {
                            result = &*it;
                            break;
                        }
                    }

                    if ((flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == MOBILE_SEA_UNIT) {
                        if ((*it).GetUnitType() == DOCK) {
                            result = &*it;
                            break;
                        }
                    }

                    if (flags & MOBILE_AIR_UNIT) {
                        if ((*it).GetUnitType() == LANDPAD || (*it).GetUnitType() == HANGAR) {
                            result = &*it;
                            break;
                        }
                    }
                }
            }
        }
    }

    return result;
}

UnitInfo* Access_GetTeamBuilding(uint16_t team, int32_t grid_x, int32_t grid_y) {
    UnitInfo* unit{nullptr};

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).team == team && (*it).GetOrder() != ORDER_IDLE && (*it).GetId() != 0xFFFF &&
                    (((*it).flags & (CONNECTOR_UNIT | STANDALONE)) ||
                     (((*it).flags) & (GROUND_COVER | BUILDING)) == BUILDING)) {
                    unit = &*it;
                    break;
                }
            }
        }
    }

    return unit;
}

void Access_MultiSelect(UnitInfo* unit, Rect* bounds) {
    Rect selection;
    UnitInfo* parent = nullptr;
    int32_t unit_count;
    bool limit_reached;

    selection.ulx = (bounds->ulx + 16) / 64;
    selection.uly = (bounds->uly + 16) / 64;
    selection.lrx = (bounds->lrx - 16) / 64;
    selection.lry = (bounds->lry - 16) / 64;

    if (unit) {
        if (unit->team == GameManager_PlayerTeam &&
            (unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) &&
            unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER && unit->grid_x >= selection.ulx &&
            unit->grid_x <= selection.lrx && unit->grid_y >= selection.uly && unit->grid_y <= selection.lry) {
            parent = unit;
            unit->ClearUnitList();
            unit->AllocateUnitList();
        }
    }

    if (parent) {
        unit_count = 1;

    } else {
        unit_count = 0;
    }

    limit_reached = false;

    for (int32_t y = selection.uly; y <= selection.lry && !limit_reached; ++y) {
        for (int32_t x = selection.ulx; x <= selection.lrx && !limit_reached; ++x) {
            UnitInfo* unit2 =
                Access_GetTeamUnit(x, y, GameManager_PlayerTeam, MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT);

            if (unit2 && (unit2->GetOrderState() == ORDER_STATE_EXECUTING_ORDER ||
                          unit2->GetOrderState() == ORDER_STATE_READY_TO_EXECUTE_ORDER)) {
                unit2->ClearUnitList();

                if (!parent) {
                    unit2->AllocateUnitList();
                    parent = unit2;
                    ++unit_count;
                }

                unit2->AssignUnitList(parent);

                if (parent != unit2) {
                    ++unit_count;
                }

                if (unit_count >= 10) {
                    limit_reached = true;
                }
            }
        }
    }

    if (unit_count) {
        if (unit_count == 1) {
            parent->ClearUnitList();

        } else {
            Access_UpdateMultiSelection(parent);
        }

        if (parent != unit) {
            GameManager_MenuUnitSelect(parent);
        }
    }
}

void Access_ProcessGroupAirPath(UnitInfo* unit) {
    unit->path = Paths_GetAirPath(unit);

    if (unit->path != nullptr) {
        if (Remote_IsNetworkGame) {
            Remote_SendNetPacket_38(unit);
        }

    } else {
        unit->BlockedOnPathRequest(false);

        unit->ClearUnitList();
    }
}

bool Access_AreTaskEventsPending() {
    for (int32_t i = std::size(Access_UnitsLists) - 1; i >= 0; --i) {
        for (SmartList<UnitInfo>::Iterator it = Access_UnitsLists[i]->Begin(); it != Access_UnitsLists[i]->End();
             ++it) {
            if ((*it).GetOrder() == ORDER_FIRE || (*it).GetOrder() == ORDER_EXPLODE ||
                (*it).GetOrder() == ORDER_ACTIVATE || (*it).GetOrder() == ORDER_AWAIT_TAPE_POSITIONING ||
                (*it).GetOrder() == ORDER_AWAIT_DISABLE_UNIT || (*it).GetOrder() == ORDER_AWAIT_STEAL_UNIT ||
                (*it).GetOrder() == ORDER_AWAIT_SCALING || (*it).GetOrderState() == ORDER_STATE_DESTROY ||
                ((*it).GetOrder() == ORDER_MOVE && (*it).GetOrderState() != ORDER_STATE_EXECUTING_ORDER) ||
                ((*it).GetOrder() == ORDER_MOVE_TO_UNIT && (*it).GetOrderState() != ORDER_STATE_EXECUTING_ORDER) ||
                ((*it).GetOrder() == ORDER_MOVE_TO_ATTACK && (*it).GetOrderState() != ORDER_STATE_EXECUTING_ORDER) ||
                (*it).GetOrderState() == ORDER_STATE_INIT) {
                if ((*it).GetOrderState() == ORDER_STATE_NEW_ORDER && (*it).team == GameManager_PlayerTeam &&
                    !PathsManager_HasRequest(&*it)) {
                    if ((*it).path != nullptr) {
                        if ((*it).GetFirstFromUnitList()) {
                            Access_ProcessNewGroupOrder(&*it);

                        } else {
                            (*it).SetOrderState(ORDER_STATE_ISSUING_PATH);

                            if (Remote_IsNetworkGame) {
                                Remote_SendNetPacket_08(&*it);
                            }
                        }

                    } else if (((*it).flags & MOBILE_AIR_UNIT) && (*it).GetOrder() != ORDER_MOVE_TO_ATTACK) {
                        Access_ProcessGroupAirPath(&*it);

                    } else {
                        Paths_RequestPath(&*it, 2);
                    }
                }

                return true;
            }
        }
    }

    for (uint32_t i = 0; i < std::size(UnitsManager_DelayedAttackTargets); ++i) {
        if (UnitsManager_DelayedAttackTargets[i].GetCount()) {
            return true;
        }
    }

    if (Remote_IsNetworkGame) {
        Remote_WaitEndTurnAcknowledge();
    }

    return false;
}

bool Access_ProcessNewGroupOrder(UnitInfo* unit) {
    bool result;

    if (unit) {
        SmartList<UnitInfo>* group;

        group = unit->GetUnitList();

        if (group) {
            for (SmartList<UnitInfo>::Iterator it = group->Begin(); it != group->End(); ++it) {
                if ((*it).GetOrderState() == ORDER_STATE_NEW_ORDER && (*it).path == nullptr) {
                    if ((*it).hits > 0) {
                        if (((*it).flags & MOBILE_AIR_UNIT) && (*it).GetOrder() != ORDER_MOVE_TO_ATTACK) {
                            Access_ProcessGroupAirPath(&*it);
                        }

                        return false;

                    } else {
                        (*it).ClearUnitList();
                    }
                }
            }

            for (SmartList<UnitInfo>::Iterator it = group->Begin(); it != group->End(); ++it) {
                unit = &*it;

                if (unit->GetOrderState() == ORDER_STATE_NEW_ORDER) {
                    unit->SetOrderState(ORDER_STATE_ISSUING_PATH);

                    if (Remote_IsNetworkGame) {
                        Remote_SendNetPacket_08(unit);
                    }
                }
            }

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void Access_UpdateMultiSelection(UnitInfo* unit) {
    SmartList<UnitInfo>* units;

    unit->MoveToFrontInUnitList();
    units = unit->GetUnitList();

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        (*it).SetOrder(ORDER_AWAIT);
        (*it).target_grid_x = (*it).grid_x;
        (*it).target_grid_y = (*it).grid_y;
        (*it).attack_site.x = (*it).grid_x;
        (*it).attack_site.y = (*it).grid_y;
        (*it).path = nullptr;

        if (Remote_IsNetworkGame) {
            Remote_SendNetPacket_38(&*it);
        }
    }
}

bool Access_IsGroupOrderInterrupted(UnitInfo* unit) {
    SmartList<UnitInfo>* units;

    units = unit->GetUnitList();

    if (units) {
        for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
            if (((*it).GetOrder() == ORDER_MOVE || (*it).GetOrder() == ORDER_MOVE_TO_ATTACK) &&
                ((*it).GetOrderState() == ORDER_STATE_IN_PROGRESS ||
                 (*it).GetOrderState() == ORDER_STATE_IN_TRANSITION ||
                 (*it).GetOrderState() == ORDER_STATE_NEW_ORDER)) {
                return true;
            }
        }
    }

    return false;
}

bool Access_IsInsideBounds(Rect* bounds, Point* point) {
    return point->x >= bounds->ulx && point->x < bounds->lrx && point->y >= bounds->uly && point->y < bounds->lry;
}
