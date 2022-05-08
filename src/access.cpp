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

#include "ai.hpp"
#include "gui.hpp"
#include "hash.hpp"
#include "resource_manager.hpp"
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

bool Access_SetUnitDestination(int grid_x, int grid_y, int target_grid_x, int target_grid_y, bool mode) {
    SmartList<UnitInfo> units = Hash_MapHash[Point(grid_x, grid_y)];
    SmartPointer<UnitInfo> unit;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
            if ((*it).orders == ORDER_IDLE) {
                continue;
            }

            if (((*it).orders == ORDER_MOVING || (*it).orders == ORDER_MOVING_27 || (*it).orders == ORDER_ATTACKING) &&
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
        if (unit->orders == ORDER_AWAITING) {
            if (mode) {
                UnitsManager_SetNewOrderInt(&*unit, ORDER_MOVING, ORDER_STATE_39);
            } else {
                UnitsManager_SetNewOrderInt(&*unit, ORDER_MOVING, ORDER_STATE_38);
            }
        }

        return true;

    } else {
        return false;
    }
}

unsigned int Access_sub_102B9(ResourceID unit_type, unsigned short team, int grid_x, int grid_y, unsigned int flags) {
    /// \todo
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

    if (unit->orders != ORDER_IDLE && unit->team == GUI_PlayerTeamIndex) {
        result = unit->unit_type == SURVEYOR;
    } else {
        result = false;
    }

    return result;
}

bool Access_IsWithinScanRange(UnitInfo* unit, int grid_x, int grid_y, int scan_range) {
    int scan_area;
    int grid_area;

    scan_area = (scan_range * 64) * (scan_range * 64);
    grid_area = ((grid_x - unit->grid_x) * 64) * ((grid_y - unit->grid_y) * 64);

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

unsigned int Access_GetAttackTargetGroup(UnitInfo* unit) {
    unsigned int result;

    if (unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) == 0 || unit->orders == ORDER_DISABLED) {
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
            SmartList<UnitInfo> units = Hash_MapHash[Point(grid_x, grid_y)];

            for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
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
            SmartList<UnitInfo> units = Hash_MapHash[Point(grid_x, grid_y)];

            for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
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

        if (team == GUI_PlayerTeamIndex) {
            ResourceManager_Minimap[map_offset] = ResourceManager_MinimapFov[map_offset];
        }

        SmartList<UnitInfo> units = Hash_MapHash[Point(grid_x, grid_y)];

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
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

        if (team == GUI_PlayerTeamIndex) {
            ResourceManager_Minimap[map_offset] =
                ResourceManager_ColorIndexTable12[ResourceManager_MinimapFov[map_offset]];
        }

        SmartList<UnitInfo> units = Hash_MapHash[Point(grid_x, grid_y)];
        SmartList<UnitInfo>::Iterator it = units.Begin();

        if (it != nullptr) {
            if ((GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) &&
                GameManager_LockedUnits.GetCount()) {
                GameManager_UpdateDrawBounds();
            }

            for (; it != units.End(); ++it) {
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
    /// \todo
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

    Access_UpdateMinimapFogOfWar(GUI_PlayerTeamIndex, all_visible);
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

UnitInfo* Access_GetUnit3(int grid_x, int grid_y, unsigned int flags) {
    /// \todo
}
