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
    ATTACK_TARGET_GROUP_NONE = 0x0,
    ATTACK_TARGET_GROUP_LAND_STEALTH = 0x1,
    ATTACK_TARGET_GROUP_SEA_STEALTH = 0x2,
    ATTACK_TARGET_GROUP_LAND = 0x4,
    ATTACK_TARGET_GROUP_WATER = 0x8,
    ATTACK_TARGET_GROUP_AIR = 0x10
};

static SmartList<UnitInfo>* Access_UnitsLists[] = {&UnitsManager_MobileLandSeaUnits, &UnitsManager_MobileAirUnits,
                                                   &UnitsManager_StationaryUnits, &UnitsManager_GroundCoverUnits,
                                                   &UnitsManager_ParticleUnits};

static bool Access_UpdateGroupSpeed(UnitInfo* unit);
static bool Access_IsValidAttackTargetTypeEx(ResourceID attacker, ResourceID target, unsigned int target_flags);
static bool Access_IsValidAttackTargetEx(ResourceID attacker, ResourceID target, unsigned int target_flags,
                                         Point point);
static void Access_ProcessGroupAirPath(UnitInfo* unit);

bool Access_SetUnitDestination(int grid_x, int grid_y, int target_grid_x, int target_grid_y, bool mode) {
    SmartPointer<UnitInfo> unit;

    for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
        if ((*it).flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
            if ((*it).orders == ORDER_IDLE) {
                continue;
            }

            if (((*it).orders == ORDER_MOVE || (*it).orders == ORDER_MOVE_TO_UNIT ||
                 (*it).orders == ORDER_MOVE_TO_ATTACK) &&
                (*it).state != ORDER_STATE_1 && (*it).path != nullptr && (*it).grid_x == target_grid_x &&
                (*it).grid_y == target_grid_y &&
                !(*it).path->IsInPath(grid_x - target_grid_x, grid_y - target_grid_y)) {
                return true;

            } else {
                return false;
            }

        } else {
            if ((*it).unit_type == BRIDGE) {
                unit = (*it);
            }
        }
    }

    if (unit != nullptr) {
        if (unit->orders == ORDER_AWAIT) {
            if (mode) {
                UnitsManager_SetNewOrderInt(&*unit, ORDER_MOVE, ORDER_STATE_39);
            } else {
                UnitsManager_SetNewOrderInt(&*unit, ORDER_MOVE, ORDER_STATE_38);
            }
        }

        return true;

    } else {
        return false;
    }
}

unsigned int Access_IsAccessible(ResourceID unit_type, unsigned short team, int grid_x, int grid_y,
                                 unsigned int flags) {
    unsigned int unit_flags;
    unsigned int result;
    bool debug_flag;

    debug_flag = false;

    unit_flags = UnitsManager_BaseUnits[unit_type].flags;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        result = 4;

        if (!(flags & 0x20)) {
            unsigned char surface_type;

            surface_type = Access_GetModifiedSurfaceType(grid_x, grid_y);

            if ((surface_type & SURFACE_TYPE_WATER) && (unit_flags & MOBILE_LAND_UNIT) && unit_type != SURVEYOR) {
                result = 4 + 8;
            }

            for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
                if ((*it).IsVisibleToTeam(team) || (flags & 0x10) ||
                    ((*it).IsDetectedByTeam(team) && ((*it).flags & STATIONARY))) {
                    if ((*it).orders != ORDER_IDLE || ((*it).flags & STATIONARY)) {
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
                                (((*it).unit_type == BRIDGE) ||
                                 ((*it).unit_type >= LRGRUBLE && (*it).unit_type <= SMLCONES) ||
                                 (*it).unit_type == LANDMINE || (*it).unit_type == SEAMINE)) {
                                return 0;
                            }

                            if ((*it).flags & STATIONARY) {
                                switch ((*it).unit_type) {
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
                                                   unit_type != WTRPLTFM && unit_type != BRIDGE && unit_type != ROAD) {
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

            if (debug_flag) {
                result = 8;
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

bool Access_FindReachableSpotInt(ResourceID unit_type, UnitInfo* unit, short* grid_x, short* grid_y, int range_limit,
                                 int mode, int direction) {
    unsigned short team;
    UnitValues* unit_values;
    int offset_x;
    int offset_y;

    team = unit->team;
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
    }

    for (int i = 0; i < range_limit; ++i) {
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
    int heat_map_size;

    Access_InitUnitStealthStatus(UnitsManager_GroundCoverUnits);
    Access_InitUnitStealthStatus(UnitsManager_MobileLandSeaUnits);
    Access_InitUnitStealthStatus(UnitsManager_MobileAirUnits);
    Access_InitUnitStealthStatus(UnitsManager_ParticleUnits);
    Access_InitUnitStealthStatus(UnitsManager_StationaryUnits);

    heat_map_size = ResourceManager_MapSize.x * ResourceManager_MapSize.y;

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            memset(UnitsManager_TeamInfo[team].heat_map_complete, 0, heat_map_size);
            memset(UnitsManager_TeamInfo[team].heat_map_stealth_sea, 0, heat_map_size);
            memset(UnitsManager_TeamInfo[team].heat_map_stealth_land, 0, heat_map_size);
        }
    }
}

bool Access_IsSurveyorOverlayActive(UnitInfo* unit) {
    bool result;

    if (unit->orders != ORDER_IDLE && unit->team == GameManager_PlayerTeam) {
        result = unit->unit_type == SURVEYOR;
    } else {
        result = false;
    }

    return result;
}

bool Access_IsWithinScanRange(UnitInfo* unit, int grid_x, int grid_y, int scan_range) {
    int scan_area = (scan_range * 64) * (scan_range * 64);
    int radius_x = ((grid_x - unit->grid_x) * 64);
    int radius_y = ((grid_y - unit->grid_y) * 64);
    int grid_area = radius_x * radius_x + radius_y * radius_y;

    return grid_area <= scan_area;
}

int Access_GetDistance(int grid_x, int grid_y) { return grid_x * grid_x + grid_y * grid_y; }

int Access_GetDistance(Point position1, Point position2) {
    position1 -= position2;

    return position1.x * position1.x + position1.y * position1.y;
}

int Access_GetDistance(UnitInfo* unit, Point position) {
    position.x -= unit->grid_x;
    position.y -= unit->grid_y;

    return position.x * position.x + position.y * position.y;
}

int Access_GetDistance(UnitInfo* unit1, UnitInfo* unit2) {
    Point distance;

    distance.x = unit1->grid_x - unit2->grid_x;
    distance.y = unit1->grid_y - unit2->grid_y;

    return distance.x * distance.x + distance.y * distance.y;
}

bool Access_IsWithinAttackRange(UnitInfo* unit, int grid_x, int grid_y, int attack_range) {
    int distance_x;
    int distance_y;
    int distance;
    int ratio_x;
    int ratio_y;
    bool result;

    attack_range *= 64;
    attack_range *= attack_range;
    distance_x = (grid_x - unit->grid_x) * 64;
    distance_y = (grid_y - unit->grid_y) * 64;
    distance = distance_x * distance_x + distance_y * distance_y;

    if (distance > attack_range) {
        result = false;
    } else {
        if (unit->unit_type != SUBMARNE && unit->unit_type != CORVETTE) {
            result = true;
        } else {
            distance = std::max(labs(distance_x), labs(distance_y)) * 64;

            if (distance) {
                ratio_x = (distance_x << 16) / distance;
                ratio_y = (distance_y << 16) / distance;
                distance_x = ratio_x + (unit->x << 16);
                distance_y = ratio_y + (unit->y << 16);

                for (; --distance; distance_x += ratio_x, distance_y += ratio_y) {
                    unsigned char surface_type;

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

bool Access_FindReachableSpot(ResourceID unit_type, UnitInfo* unit, short* grid_x, short* grid_y, int range,
                              int exclusion_zone, int mode) {
    for (int i = 1; i <= range; ++i) {
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

unsigned int Access_GetAttackTargetGroup(UnitInfo* unit) {
    unsigned int result;

    if (unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) == 0 || unit->orders == ORDER_DISABLE) {
        result = ATTACK_TARGET_GROUP_NONE;

    } else {
        switch (unit->unit_type) {
            case COMMANDO:
            case INFANTRY: {
                result = ATTACK_TARGET_GROUP_LAND_STEALTH | ATTACK_TARGET_GROUP_LAND | ATTACK_TARGET_GROUP_WATER;
            } break;

            case CORVETTE: {
                result = ATTACK_TARGET_GROUP_SEA_STEALTH | ATTACK_TARGET_GROUP_WATER;
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
                result = ATTACK_TARGET_GROUP_LAND | ATTACK_TARGET_GROUP_WATER;
            } break;

            case SUBMARNE: {
                result = ATTACK_TARGET_GROUP_WATER;
            } break;

            case ANTIAIR:
            case SP_FLAK:
            case FIGHTER: {
                result = ATTACK_TARGET_GROUP_AIR;
            } break;

            case ALNPLANE: {
                result = ATTACK_TARGET_GROUP_LAND | ATTACK_TARGET_GROUP_WATER | ATTACK_TARGET_GROUP_AIR;
            } break;

            case LANDMINE:
            case SEAMINE: {
                result = ATTACK_TARGET_GROUP_NONE;
            } break;

            default: {
                SDL_assert(0);
            } break;
        }
    }

    return result;
}

unsigned int Access_UpdateMapStatusAddUnit(UnitInfo* unit, int grid_x, int grid_y) {
    unsigned int result;
    unsigned short team;
    int map_offset;

    team = unit->team;
    map_offset = ResourceManager_MapSize.x * grid_y + grid_x;
    result = ATTACK_TARGET_GROUP_NONE;

    if (unit->unit_type == CORVETTE) {
        if (++UnitsManager_TeamInfo[team].heat_map_stealth_sea[map_offset] == 1) {
            for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
                if (UnitsManager_IsUnitUnderWater(&*it)) {
                    (*it).SpotByTeam(team);

                    if (unit->team != (*it).team) {
                        result |= Access_GetAttackTargetGroup(&*it);
                    }
                }
            }
        }
    }

    if (unit->unit_type == COMMANDO || unit->unit_type == INFANTRY) {
        if (++UnitsManager_TeamInfo[team].heat_map_stealth_land[map_offset] == 1) {
            for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
                if ((*it).unit_type == COMMANDO) {
                    (*it).SpotByTeam(team);

                    if (unit->team != (*it).team) {
                        result |= Access_GetAttackTargetGroup(&*it);
                    }
                }
            }
        }
    }

    if (++UnitsManager_TeamInfo[team].heat_map_complete[map_offset] == 1) {
        Ai_SetInfoMapPoint(Point(grid_x, grid_y), team);

        if (team == GameManager_PlayerTeam) {
            ResourceManager_Minimap[map_offset] = ResourceManager_MinimapFov[map_offset];
        }

        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if (!(*it).IsVisibleToTeam(team)) {
                (*it).DrawStealth(team);

                if ((*it).IsVisibleToTeam(team) && (*it).team != unit->team) {
                    result |= Access_GetAttackTargetGroup(&*it);
                }
            }
        }
    }

    return result;
}

void Access_UpdateMapStatusRemoveUnit(UnitInfo* unit, int grid_x, int grid_y) {
    unsigned short team;
    int map_offset;

    team = unit->team;
    map_offset = ResourceManager_MapSize.x * grid_y + grid_x;

    if (unit->unit_type == CORVETTE) {
        --UnitsManager_TeamInfo[team].heat_map_stealth_sea[map_offset];
    }

    if (unit->unit_type == COMMANDO || unit->unit_type == INFANTRY) {
        --UnitsManager_TeamInfo[team].heat_map_stealth_land[map_offset];
    }

    if (!--UnitsManager_TeamInfo[team].heat_map_complete[map_offset]) {
        Ai_UpdateMineMap(Point(grid_x, grid_y), team);

        if (team == GameManager_PlayerTeam) {
            ResourceManager_Minimap[map_offset] =
                ResourceManager_ColorIndexTable12[ResourceManager_MinimapFov[map_offset]];
        }

        SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)];

        if (it != nullptr) {
            if ((GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) &&
                GameManager_LockedUnits.GetCount()) {
                GameManager_UpdateDrawBounds();
            }

            for (; it != nullptr; ++it) {
                (*it).Draw(team);
            }
        }
    }
}

void Access_DrawUnit(UnitInfo* unit) {
    int map_offset;

    if (GameManager_AllVisible) {
        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                unit->SpotByTeam(team);
            }
        }
    }

    map_offset = ResourceManager_MapSize.x * unit->grid_y + unit->grid_x;

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            if (UnitsManager_TeamInfo[team].heat_map_stealth_sea[map_offset] && UnitsManager_IsUnitUnderWater(unit)) {
                unit->SpotByTeam(team);
            }

            if (unit->unit_type == COMMANDO) {
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

unsigned int Access_GetVelocity(UnitInfo* unit) {
    unsigned int result;

    if (unit->unit_type == COMMANDO || UnitsManager_IsUnitUnderWater(unit)) {
        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (unit->team != team && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                unit->IsVisibleToTeam(team)) {
                if (unit->unit_type == COMMANDO) {
                    result = 4;
                } else {
                    result = 8;
                }

                return result;
            }
        }

    } else {
        if (unit->flags & MOBILE_AIR_UNIT) {
            result = 16;

        } else if (unit->flags & MOBILE_SEA_UNIT) {
            result = 8;

        } else {
            result = 4;
        }
    }

    if (unit->unit_type == COMMANDO) {
        result = 1;
    } else {
        result = 2;
    }

    return result;
}

void Access_UpdateMapStatus(UnitInfo* unit, bool mode) {
    if (unit->orders != ORDER_DISABLE) {
        if ((unit->unit_type == SURVEYOR || unit->unit_type == MINELAYR || unit->unit_type == SEAMNLYR ||
             unit->unit_type == COMMANDO) &&
            mode) {
            Survey_SurveyArea(unit, 1);
        }

        if (GameManager_AllVisible) {
            for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                    unit->SpotByTeam(team);
                }
            }

        } else if (unit->orders != ORDER_IDLE) {
            if (mode) {
                Access_DrawUnit(unit);

            } else {
                for (int team = PLAYER_TEAM_GRAY; team >= PLAYER_TEAM_RED; --team) {
                    unit->Draw(team);
                }
            }

            if ((unit->flags & SELECTABLE) && UnitsManager_TeamInfo[unit->team].team_type != TEAM_TYPE_NONE) {
                unsigned char target_group;
                int scan;
                Rect zone;

                target_group = ATTACK_TARGET_GROUP_NONE;
                scan = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);

                if (unit->orders == ORDER_DISABLE) {
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

                for (int grid_y = zone.uly; grid_y <= zone.lry; ++grid_y) {
                    for (int grid_x = zone.ulx; grid_x <= zone.lrx; ++grid_x) {
                        if (Access_IsWithinScanRange(unit, grid_x, grid_y, scan)) {
                            if (mode) {
                                target_group |= Access_UpdateMapStatusAddUnit(unit, grid_x, grid_y);

                            } else {
                                Access_UpdateMapStatusRemoveUnit(unit, grid_x, grid_y);
                            }
                        }
                    }
                }

                if (target_group != ATTACK_TARGET_GROUP_NONE &&
                    (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_PLAYER ||
                     UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) &&
                    unit->orders != ORDER_AWAIT && ini_get_setting(INI_ENEMY_HALT)) {
                    unsigned int velocity_type;

                    velocity_type = Access_GetVelocity(unit);

                    if (unit->GetUnitList()) {
                        for (SmartList<UnitInfo>::Iterator it = unit->GetUnitList()->Begin();
                             it != unit->GetUnitList()->End(); ++it) {
                            velocity_type |= Access_GetVelocity(&*it);
                        }
                    }

                    if (velocity_type & target_group) {
                        if (unit->GetUnitList()) {
                            for (SmartList<UnitInfo>::Iterator it = unit->GetUnitList()->Begin();
                                 it != unit->GetUnitList()->End(); ++it) {
                                UnitEventEmergencyStop* unit_event;

                                unit_event = new (std::nothrow) UnitEventEmergencyStop(&*it);

                                UnitEvent_UnitEvents.PushBack(*unit_event);

                                if (Remote_IsNetworkGame) {
                                    Remote_SendNetPacket_50(&*it);
                                }
                            }

                        } else {
                            UnitEventEmergencyStop* unit_event;

                            unit_event = new (std::nothrow) UnitEventEmergencyStop(unit);

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

void Access_UpdateMinimapFogOfWar(unsigned short team, bool all_visible, bool ignore_team_heat_map) {
    memcpy(ResourceManager_Minimap, ResourceManager_MinimapFov, 112 * 112);

    if (!all_visible) {
        for (int i = 0; i < 112 * 112; ++i) {
            if (UnitsManager_TeamInfo[team].heat_map_complete[i] == 0 || ignore_team_heat_map) {
                ResourceManager_Minimap[i] = ResourceManager_ColorIndexTable12[ResourceManager_Minimap[i]];
            }
        }
    }
}

void Access_UpdateResourcesTotal(Complex* complex) {
    Cargo total;
    Cargo cargo;

    complex->material = 0;
    complex->fuel = 0;
    complex->gold = 0;
    complex->power = 0;
    complex->workers = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetComplex() == complex) {
            Cargo_GetCargoDemand(&*it, &total);
            total += *Cargo_GetCargo(&*it, &cargo);

            complex->material += total.raw;
            complex->fuel += total.fuel;
            complex->gold += total.gold;
            complex->power += total.power;
            complex->workers += total.life;
        }
    }
}

unsigned char Access_GetSurfaceType(int grid_x, int grid_y) {
    return ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * grid_y + grid_x];
}

unsigned char Access_GetModifiedSurfaceType(int grid_x, int grid_y) {
    unsigned char surface_type;

    surface_type = Access_GetSurfaceType(grid_x, grid_y);

    if (surface_type == SURFACE_TYPE_WATER || SURFACE_TYPE_COAST) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).unit_type == WTRPLTFM || (*it).unit_type == BRIDGE) {
                surface_type = SURFACE_TYPE_LAND;
            }
        }
    }

    return surface_type;
}

bool Access_IsAnyLandPresent(int grid_x, int grid_y, unsigned int flags) {
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

bool Access_IsFullyLandCovered(int grid_x, int grid_y, unsigned int flags) {
    bool result;

    if (((flags & BUILDING) || (Access_GetSurfaceType(grid_x + 1, grid_y) == SURFACE_TYPE_LAND &&
                                Access_GetSurfaceType(grid_x, grid_y + 1) == SURFACE_TYPE_LAND &&
                                Access_GetSurfaceType(grid_x + 1, grid_y + 1) == SURFACE_TYPE_LAND)) &&
        Access_GetSurfaceType(grid_x, grid_y) == SURFACE_TYPE_LAND) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

UnitInfo* Access_GetUnit8(unsigned short team, int grid_x, int grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).unit_type == SMLRUBLE) {
                unit = &*it;
                break;
            }

            if ((*it).unit_type == LRGRUBLE) {
                for (int y = (*it).grid_y; y <= (*it).grid_y + 1; ++y) {
                    for (int x = (*it).grid_x; x <= (*it).grid_x + 1; ++x) {
                        if (x != grid_x || y != grid_y) {
                            if (!Access_IsAccessible(BULLDOZR, team, grid_x, grid_y, 2)) {
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

    return unit;
}

int Access_FindUnitInUnitList(UnitInfo* unit) {
    int result;

    result = -1;

    for (int i = 0; i < sizeof(Access_UnitsLists) / sizeof(SmartList<UnitInfo>); ++i) {
        if (Access_UnitsLists[i]->Find(*unit) != nullptr) {
            result = i;
            break;
        }
    }

    return result;
}

bool Access_IsTeamInUnitList(unsigned short team, SmartList<UnitInfo>& units) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team) {
            return true;
        }
    }

    return false;
}

bool Access_IsTeamInUnitLists(unsigned short team) {
    return Access_IsTeamInUnitList(team, UnitsManager_MobileLandSeaUnits) ||
           Access_IsTeamInUnitList(team, UnitsManager_MobileAirUnits) ||
           Access_IsTeamInUnitList(team, UnitsManager_StationaryUnits);
}

UnitInfo* Access_GetSelectableUnit(UnitInfo* unit, int grid_x, int grid_y) {
    UnitInfo* unit2;

    Hash_MapHash.Remove(unit);
    Hash_MapHash.Add(unit, true);

    unit2 = Access_GetUnit3(grid_x, grid_y, SELECTABLE);

    if (unit2->orders == ORDER_IDLE || (unit2->flags & GROUND_COVER)) {
        unit2 = unit;
    }

    if (unit == unit2) {
        Hash_MapHash.Remove(unit);
        Hash_MapHash.Add(unit);
    }

    return unit2;
}

UnitInfo* Access_GetFirstMiningStation(unsigned short team) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == MININGST) {
            return &*it;
        }
    }

    return nullptr;
}

UnitInfo* Access_GetFirstActiveUnit(unsigned short team, SmartList<UnitInfo>& units) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team && (*it).orders != ORDER_IDLE) {
            return &*it;
        }
    }

    return nullptr;
}

void Access_RenewAttackOrders(SmartList<UnitInfo>& units, unsigned short team) {
    UnitInfo* unit;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        unit = (*it).GetFirstFromUnitList();

        if (unit) {
            if ((*it).team == team && (&(*it) == unit)) {
                Access_GroupAttackOrder(unit, 0);
            }

        } else {
            if (((*it).orders == ORDER_MOVE || (*it).orders == ORDER_MOVE_TO_UNIT ||
                 (*it).orders == ORDER_MOVE_TO_ATTACK) &&
                (*it).state == ORDER_STATE_1) {
                if ((*it).team == team && (*it).speed != 0 && (*it).engine == 2) {
                    UnitsManager_SetNewOrder(&(*it), (*it).orders, ORDER_STATE_0);
                }
            }
        }
    }
}

bool Access_UpdateGroupSpeed(UnitInfo* unit) {
    int max_speed;
    SmartList<UnitInfo>* units;

    units = unit->GetUnitList();

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (((*it).orders == ORDER_MOVE || (*it).orders == ORDER_MOVE_TO_ATTACK) &&
            ((*it).state == ORDER_STATE_5 || (*it).state == ORDER_STATE_6 || (*it).state == ORDER_STATE_NEW_ORDER)) {
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
        UnitInfo* enemy;
        UnitInfo* friendly;
        SmartList<UnitInfo>* units;

        if (unit->orders == ORDER_MOVE_TO_ATTACK) {
            enemy = unit->GetEnemy();

            SDL_assert(enemy != nullptr);

        } else {
            enemy = nullptr;
        }

        units = unit->GetUnitList();

        for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
            friendly = &*it;

            if ((mode || (friendly->group_speed - 1) != 0) &&
                (friendly->orders == ORDER_MOVE || friendly->orders == ORDER_MOVE_TO_ATTACK ||
                 friendly->orders == ORDER_AWAIT)) {
                if (friendly->orders == ORDER_MOVE_TO_ATTACK) {
                    if (friendly->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) &&
                        Access_IsValidAttackTarget(enemy, friendly)) {
                        friendly->group_speed = 0;
                        friendly->target_grid_x = unit->target_grid_x;
                        friendly->target_grid_y = unit->target_grid_y;
                        friendly->SetEnemy(enemy);
                        UnitsManager_SetNewOrder(friendly, ORDER_MOVE_TO_ATTACK, ORDER_STATE_NEW_ORDER);
                        Paths_RequestPath(friendly, 2);
                    }

                } else {
                    friendly->target_grid_x = friendly->point.x - unit->point.x + unit->target_grid_x;
                    friendly->target_grid_y = friendly->point.y - unit->point.y + unit->target_grid_y;

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

                        if (friendly->orders == ORDER_MOVE_TO_ATTACK) {
                            friendly->orders = ORDER_AWAIT;
                            friendly->state = ORDER_STATE_1;
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

int Access_GetStoredUnitCount(UnitInfo* unit) {
    SmartList<UnitInfo>* units;
    int result;

    if (unit->unit_type == HANGAR) {
        units = &UnitsManager_MobileAirUnits;

    } else {
        units = &UnitsManager_MobileLandSeaUnits;
    }

    result = 0;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).orders == ORDER_IDLE && (*it).GetParent() == unit) {
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
    short grid_x = unit->grid_x;
    short grid_y = unit->grid_y;
    bool result;

    if (unit->orders == ORDER_SENTRY || unit->orders == ORDER_IDLE || unit->orders == ORDER_DISABLE ||
        unit->orders == ORDER_EXPLODE || unit->state == ORDER_STATE_14 || unit->state == ORDER_STATE_2 ||
        (unit->orders == ORDER_BUILD && unit->state != ORDER_STATE_UNIT_READY)) {
        result = false;

    } else if (Access_IsWithinMovementRange(unit) ||
               (unit->shots && Access_FindReachableSpot(unit->unit_type, unit, &grid_x, &grid_y,
                                                        unit_values->GetAttribute(ATTRIB_RANGE), 0, 1))) {
        result = true;

    } else {
        result = unit->popup->test(unit);
    }

    return result;
}

UnitInfo* Access_SeekNextUnit(unsigned short team, UnitInfo* unit, bool seek_direction) {
    int list_index;
    int seek_index;
    SmartList<UnitInfo>::Iterator it;
    UnitInfo* result;

    list_index = Access_FindUnitInUnitList(unit);

    if (list_index >= 0) {
        it = Access_UnitsLists[list_index]->Find(*unit);

    } else if (seek_direction) {
        list_index = 4;

        it = Access_UnitsLists[list_index]->Last();

    } else {
        list_index = 0;

        it = Access_UnitsLists[list_index]->Begin();
    }

    seek_index = list_index;

    if (seek_direction) {
        do {
            if (it != nullptr) {
                ++it;
            }

            do {
                if (it == nullptr) {
                    ++seek_index;

                    if (seek_index > 4) {
                        seek_index = 0;
                    }

                    it = Access_UnitsLists[seek_index]->Begin();
                }
            } while (seek_index != list_index && it == nullptr);
        } while (it != nullptr && &*it != unit && ((*it).team != team || !Access_IsValidNextUnit(&*it)));

    } else {
        do {
            if (it != nullptr) {
                --it;
            }

            do {
                if (it == nullptr) {
                    --seek_index;

                    if (seek_index < 0) {
                        seek_index = 4;
                    }

                    it = Access_UnitsLists[seek_index]->Last();
                }
            } while (seek_index != list_index && it == nullptr);
        } while (it != nullptr && &*it != unit && ((*it).team != team || !Access_IsValidNextUnit(&*it)));
    }

    if (it != nullptr && Access_IsValidNextUnit(&*it)) {
        result = &*it;

    } else {
        result = nullptr;
    }

    return result;
}

bool Access_IsChildOfUnitInList(UnitInfo* unit, SmartList<UnitInfo>::Iterator* it) {
    for (; *it != nullptr; ++*it) {
        if ((*(*it)).orders == ORDER_IDLE && (*(*it)).GetParent() == unit) {
            break;
        }
    }

    return *it != nullptr;
}

UnitInfo* Access_GetUnit5(int grid_x, int grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if (((*it).flags & GROUND_COVER) || (*it).unit_type == HARVSTER) {
                unit = &*it;
                break;
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetUnit3(int grid_x, int grid_y, unsigned int flags) {
    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        UnitInfo* unit;
        SmartList<UnitInfo>::Iterator it;

        for (it = Hash_MapHash[Point(grid_x, grid_y)], unit = &*it; it != nullptr;) {
            if (((*it).flags & flags) && !((*it).flags & GROUND_COVER) && (*it).orders != ORDER_IDLE) {
                return &*it;
            }

            Hash_MapHash.Remove(&*it);
            Hash_MapHash.Add(&*it);

            ++it;

            if (it != nullptr) {
                if (&*it == unit) {
                    if ((*it).flags & flags) {
                        return &*it;

                    } else {
                        return nullptr;
                    }

                } else if (unit->unit_type == LRGTAPE || unit->unit_type == SMLTAPE) {
                    unit = &*it;
                }
            }
        }
    }

    return nullptr;
}

UnitInfo* Access_GetUnit1(int grid_x, int grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).unit_type == BRIDGE) {
                unit = &*it;
                break;
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetUnit2(int grid_x, int grid_y, unsigned short team) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).team == team && ((*it).flags & SELECTABLE) && !((*it).flags & (HOVERING | GROUND_COVER)) &&
                (*it).unit_type != LANDPAD && (*it).orders != ORDER_IDLE && (*it).GetId() != 0xFFFF) {
                unit = &*it;
                break;
            }
        }

        if (unit && (unit->unit_type == LRGTAPE || unit->unit_type == SMLTAPE)) {
            unit = unit->GetParent();
        }
    }

    return unit;
}

void Access_DestroyUtilities(int grid_x, int grid_y, bool remove_slabs, bool remove_rubble, bool remove_connectors,
                             bool remove_road) {
    for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
        switch ((*it).unit_type) {
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

void Access_DestroyGroundCovers(int grid_x, int grid_y) {
    for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
        switch ((*it).unit_type) {
            case ROAD:
            case LANDPAD:
            case WTRPLTFM:
            case BRIDGE: {
                UnitsManager_DestroyUnit(&*it);
            } break;
        }
    }
}

UnitInfo* Access_GetUnit6(unsigned short team, int grid_x, int grid_y, unsigned int flags) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).team != team && ((*it).IsVisibleToTeam(team) || GameManager_MaxSpy) &&
                (*it).orders != ORDER_IDLE && ((*it).flags & flags) && (*it).orders != ORDER_EXPLODE &&
                (*it).state != ORDER_STATE_14) {
                unit = &*it;
                break;
            }
        }

        if (unit && (unit->unit_type == LRGTAPE || unit->unit_type == SMLTAPE)) {
            unit = unit->GetParent();
        }
    }

    return unit;
}

UnitInfo* Access_GetUnit7(unsigned short team, int grid_x, int grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).team == team && (*it).unit_type >= LRGTAPE && (*it).unit_type <= SMLCONES) {
                unit = &*it;
                break;
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetUnit4(int grid_x, int grid_y, unsigned short team, unsigned int flags) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).team == team && ((*it).flags & flags) && (*it).orders != ORDER_IDLE && (*it).GetId() != 0xFFFF &&
                (*it).unit_type != LRGTAPE && (*it).unit_type != SMLTAPE && !((*it).flags & GROUND_COVER)) {
                unit = &*it;
                break;
            }
        }

        if (unit == nullptr) {
            for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
                if ((*it).team == team && ((*it).flags & flags) && (*it).orders != ORDER_IDLE &&
                    (*it).GetId() != 0xFFFF) {
                    unit = &*it;
                    break;
                }
            }
        }

        if (unit && (unit->unit_type == LRGTAPE || unit->unit_type == SMLTAPE)) {
            unit = unit->GetParent();
        }
    }

    return unit;
}

unsigned int Access_GetValidAttackTargetTypes(ResourceID unit_type) {
    unsigned int target_types;

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

bool Access_IsValidAttackTargetTypeEx(ResourceID attacker, ResourceID target, unsigned int target_flags) {
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

bool Access_IsValidAttackTargetEx(ResourceID attacker, ResourceID target, unsigned int target_flags, Point point) {
    unsigned char surface_type;
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

    for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(unit->grid_x, unit->grid_y)]; it != nullptr; ++it) {
        if (((*it).flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT | STATIONARY)) && !((*it).flags & GROUND_COVER) &&
            (*it).orders != ORDER_IDLE) {
            result = true;
            break;
        }
    }

    return result;
}

bool Access_IsValidAttackTarget(UnitInfo* attacker, UnitInfo* target, Point point) {
    unsigned int target_flags;
    ResourceID target_type;
    bool result;

    target_flags = target->flags;
    target_type = target->unit_type;

    if (!(attacker->flags & MOBILE_AIR_UNIT) || (attacker->flags & HOVERING)) {
        if ((target_flags & MOBILE_AIR_UNIT) || !(attacker->flags & HOVERING)) {
            target_flags &= ~MOBILE_AIR_UNIT;
            target_flags |= MOBILE_LAND_UNIT;
        }

        if (target_type == LRGTAPE || target_type == SMLTAPE) {
            target_type = target->GetParent()->unit_type;
        }

        result = Access_IsValidAttackTargetEx(attacker->unit_type, target_type, target_flags, point);

    } else {
        result = false;
    }

    return result;
}

bool Access_IsValidAttackTarget(UnitInfo* attacker, UnitInfo* target) {
    return Access_IsValidAttackTarget(attacker, target, Point(target->grid_x, target->grid_y));
}

UnitInfo* Access_GetAttackTarget(UnitInfo* unit, int grid_x, int grid_y, bool mode) {
    UnitInfo* result;
    bool normal_unit;

    result = nullptr;
    normal_unit = false;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).team == unit->team) {
                if (!((*it).flags & MISSILE_UNIT)) {
                    normal_unit = true;
                }

            } else if (((*it).flags & SELECTABLE) && (*it).orders != ORDER_EXPLODE && (*it).state != ORDER_STATE_14 &&
                       (*it).orders != ORDER_IDLE && Access_IsValidAttackTarget(unit, &*it, Point(grid_x, grid_y))) {
                if (mode) {
                    (*it).SpotByTeam(unit->team);
                }

                if ((*it).IsVisibleToTeam(unit->team)) {
                    result = &*it;
                    break;
                }
            }
        }

        if (result) {
            if ((result->unit_type == LRGTAPE || result->unit_type == SMLTAPE)) {
                result = unit->GetParent();

            } else if (normal_unit && (result->flags & GROUND_COVER)) {
                result = nullptr;
            }
        }
    }

    return result;
}

UnitInfo* Access_GetEnemyMineOnSentry(unsigned short team, int grid_x, int grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if (((*it).unit_type == LANDMINE || (*it).unit_type == SEAMINE) && (*it).team != team &&
                (*it).orders == ORDER_SENTRY) {
                unit = &*it;
                break;
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetAttackTarget2(UnitInfo* unit, int grid_x, int grid_y) {
    UnitInfo* result;

    result = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).team != unit->team && (*it).IsVisibleToTeam(unit->team) &&
                Access_IsValidAttackTarget(unit, &*it)) {
                result = &*it;
                break;
            }
        }
    }

    return unit;
}

UnitInfo* Access_GetReceiverUnit(UnitInfo* unit, int grid_x, int grid_y) {
    UnitInfo* result;
    unsigned short team;
    unsigned int flags;

    result = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        team = unit->team;
        flags = unit->flags;

        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).team == team && (*it).orders != ORDER_IDLE) {
                if (flags & MOBILE_LAND_UNIT) {
                    if (((*it).unit_type == BARRACKS || (*it).unit_type == CLNTRANS || (*it).unit_type == SEATRANS) &&
                        (unit->unit_type == COMMANDO || unit->unit_type == INFANTRY)) {
                        result = &*it;
                        break;

                    } else if (((*it).unit_type == DEPOT || (*it).unit_type == SEATRANS) &&
                               (unit->unit_type != COMMANDO && unit->unit_type != INFANTRY)) {
                        result = &*it;
                        break;
                    }
                }

                if ((flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == MOBILE_SEA_UNIT) {
                    if ((*it).unit_type == DOCK) {
                        result = &*it;
                        break;
                    }
                }

                if (flags & MOBILE_AIR_UNIT) {
                    if ((*it).unit_type == LANDPAD || (*it).unit_type == HANGAR) {
                        result = &*it;
                        break;
                    }
                }
            }
        }
    }

    return result;
}

UnitInfo* Access_GetTeamBuilding(unsigned short team, int grid_x, int grid_y) {
    UnitInfo* unit;

    unit = nullptr;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
            if ((*it).team == team && (*it).orders != ORDER_IDLE && (*it).GetId() != 0xFFFF &&
                (((*it).flags & (CONNECTOR_UNIT | STANDALONE)) ||
                 (((*it).flags) & (GROUND_COVER | BUILDING)) == BUILDING)) {
                unit = &*it;
                break;
            }
        }
    }

    return unit;
}

void Access_MultiSelect(UnitInfo* unit, Rect* bounds) {
    Rect selection;
    UnitInfo* parent;
    UnitInfo* unit2;
    int unit_count;
    bool limit_reached;

    selection.ulx = (bounds->ulx + 16) / 64;
    selection.uly = (bounds->uly + 16) / 64;
    selection.lrx = (bounds->lrx - 16) / 64;
    selection.lry = (bounds->lry - 16) / 64;

    parent = nullptr;

    if (unit) {
        if (unit->team == GameManager_PlayerTeam &&
            (unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) && unit->state == ORDER_STATE_1 &&
            unit->grid_x >= selection.ulx && unit->grid_x <= selection.lrx && unit->grid_y >= selection.uly &&
            unit->grid_y <= selection.lry) {
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

    for (int y = selection.uly; y <= selection.lry && !limit_reached; ++y) {
        for (int x = selection.ulx; x <= selection.lrx && !limit_reached; ++x) {
            unit2 = Access_GetUnit4(x, y, GameManager_PlayerTeam, MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT);

            if (unit2 && (unit2->state == ORDER_STATE_1 || unit2->state == ORDER_STATE_2)) {
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

        if (parent == unit2) {
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

        if (Remote_IsNetworkGame) {
            Remote_SendNetPacket_41(unit);
        }

        unit->ClearUnitList();
    }
}

bool Access_AreTaskEventsPending() {
    for (int i = std::size(Access_UnitsLists) - 1; i >= 0; --i) {
        for (SmartList<UnitInfo>::Iterator it = Access_UnitsLists[i]->Begin(); it != Access_UnitsLists[i]->End();
             ++it) {
            if ((*it).orders == ORDER_FIRE || (*it).orders == ORDER_EXPLODE || (*it).orders == ORDER_ACTIVATE ||
                (*it).orders == ORDER_AWAIT_TAPE_POSITIONING || (*it).orders == ORDER_AWAIT_DISABLE_UNIT ||
                (*it).orders == ORDER_AWAIT_STEAL_UNIT || (*it).orders == ORDER_AWAIT_SCALING ||
                (*it).state == ORDER_STATE_14 || ((*it).orders == ORDER_MOVE && (*it).state != ORDER_STATE_1) ||
                ((*it).orders == ORDER_MOVE_TO_UNIT && (*it).state != ORDER_STATE_1) ||
                ((*it).orders == ORDER_MOVE_TO_ATTACK && (*it).state != ORDER_STATE_1) ||
                (*it).state == ORDER_STATE_0) {
                if ((*it).state == ORDER_STATE_NEW_ORDER && (*it).team == GameManager_PlayerTeam &&
                    !PathsManager_HasRequest(&*it)) {
                    if ((*it).path != nullptr) {
                        if ((*it).GetFirstFromUnitList()) {
                            Access_ProcessNewGroupOrder(&*it);

                        } else {
                            (*it).state = ORDER_STATE_7;

                            if (Remote_IsNetworkGame) {
                                Remote_SendNetPacket_08(&*it);
                            }
                        }

                    } else if (((*it).flags & MOBILE_AIR_UNIT) && (*it).orders != ORDER_MOVE_TO_ATTACK) {
                        Access_ProcessGroupAirPath(&*it);

                    } else {
                        Paths_RequestPath(&*it, 2);
                    }
                }

                return true;
            }
        }
    }

    for (int i = 0; i < std::size(UnitsManager_DelayedAttackTargets); ++i) {
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
                if ((*it).state == ORDER_STATE_NEW_ORDER && (*it).path == nullptr) {
                    if ((*it).hits > 0) {
                        if (((*it).flags & MOBILE_AIR_UNIT) && (*it).orders != ORDER_MOVE_TO_ATTACK) {
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

                if (unit->state == ORDER_STATE_NEW_ORDER) {
                    unit->state = ORDER_STATE_7;

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
        (*it).orders = ORDER_AWAIT;
        (*it).target_grid_x = (*it).grid_x;
        (*it).target_grid_y = (*it).grid_y;
        (*it).point.x = (*it).grid_x;
        (*it).point.y = (*it).grid_y;
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
            if (((*it).orders == ORDER_MOVE || (*it).orders == ORDER_MOVE_TO_ATTACK) &&
                ((*it).state == ORDER_STATE_5 || (*it).state == ORDER_STATE_6 ||
                 (*it).state == ORDER_STATE_NEW_ORDER)) {
                return true;
            }
        }
    }

    return false;
}

bool Access_IsInsideBounds(Rect* bounds, Point* point) {
    return point->x >= bounds->ulx && point->x < bounds->lrx && point->y >= bounds->uly && point->y < bounds->lry;
}