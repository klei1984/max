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

#include "ai.hpp"

#include "access.hpp"
#include "aiattack.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "buildmenu.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "menu.hpp"
#include "production_manager.hpp"
#include "randomizer.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "task_manager.hpp"
#include "taskautosurvey.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

#define AI_SAFE_ENEMY_DISTANCE 400

static AiReactionStateType Ai_AiReactionState = AI_REACTION_STATE_IDLE;

static bool Ai_IsValidStartingPosition(Rect* bounds);
static bool Ai_IsSafeStartingPosition(int32_t grid_x, int32_t grid_y, uint16_t team);
static bool Ai_AreThereParticles();
static bool Ai_AreThereMovingUnits();

AiReactionStateType Ai_GetReactionState() { return Ai_AiReactionState; }

void Ai_SetReactionState(const AiReactionStateType state) { Ai_AiReactionState = state; }

int32_t Ai_GetNormalRateBuildCost(ResourceID unit_type, uint16_t team) {
    int32_t turns = BuildMenu_GetTurnsToBuild(unit_type, team);
    int32_t result;

    if (ResourceManager_GetUnit(unit_type).GetFlags() & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
        result = turns * Cargo_GetRawConsumptionRate(LANDPLT, 1);

    } else {
        result = turns * Cargo_GetRawConsumptionRate(CONSTRCT, 1);
    }

    return result;
}

bool Ai_IsValidStartingPosition(Rect* bounds) {
    for (int32_t x = bounds->ulx; x <= bounds->lrx; ++x) {
        for (int32_t y = bounds->uly; y <= bounds->lry; ++y) {
            if (Access_GetModifiedSurfaceType(x, y) != SURFACE_TYPE_LAND) {
                return false;
            }
        }
    }

    return true;
}

bool Ai_IsSafeStartingPosition(int32_t grid_x, int32_t grid_y, uint16_t team) {
    Rect bounds;
    Point point;

    rect_init(&bounds, grid_x - 2, grid_y - 2, grid_x + 2, grid_y + 2);

    if (Ai_IsValidStartingPosition(&bounds)) {
        for (int32_t i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
            if (team != i && UnitsManager_TeamMissionSupplies[i].units.GetCount()) {
                point.x = grid_x - UnitsManager_TeamMissionSupplies[i].starting_position.x;
                point.y = grid_y - UnitsManager_TeamMissionSupplies[i].starting_position.y;

                if (point.x * point.x + point.y * point.y < AI_SAFE_ENEMY_DISTANCE) {
                    return false;
                }
            }
        }

        return true;

    } else {
        return false;
    }
}

void Ai_SelectStartingPosition(uint16_t team) {
    Point position;

    do {
        position.x = Randomizer_Generate(ResourceManager_MapSize.x - 6) + 3;
        position.y = Randomizer_Generate(ResourceManager_MapSize.y - 6) + 3;
    } while (!Ai_IsSafeStartingPosition(position.x, position.y, team));

    UnitsManager_TeamMissionSupplies[team].starting_position = position;
}

bool Ai_SetupStrategy(uint16_t team) {
    ResourceManager_GetSettings()->SetStringValue(menu_team_name_setting[team], "Computer");

    return AiPlayer_Teams[team].SelectStrategy();
}

void Ai_SetInfoMapPoint(Point point, uint16_t team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].SetInfoMapPoint(point);
    }
}

void Ai_MarkMineMapPoint(Point point, uint16_t team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].MarkMineMapPoint(point);
    }
}

void Ai_UpdateMineMap(Point point, uint16_t team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].UpdateMineMap(point);
    }
}

void Ai_SetTasksPendingFlag(const char* event) {
    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            AiPlayer_Teams[team].ChangeTasksPendingFlag(true);
        }
    }
}

void Ai_BeginTurn(uint16_t team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].BeginTurn();
    }
}

void Ai_Init() {
    TaskManager.Clear();

    AiPlayer_TerrainDistanceField.reset();

    for (uint16_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        AiPlayer_Teams[team].Init(team);
    }
}

void Ai_FileLoad(SmartFileReader& file) {
    Ai_Init();

    for (uint16_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            AiPlayer_Teams[team].FileLoad(file);
        }
    }
}

void Ai_FileSave(SmartFileWriter& file) {
    for (uint16_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            AiPlayer_Teams[team].FileSave(file);
        }
    }
}

void Ai_AddUnitToTrackerList(UnitInfo* unit) { TaskManager.EnqueueUnitForReactionCheck(*unit); }

void Ai_EnableAutoSurvey(UnitInfo* unit) {
    SmartPointer<Task> task(new (std::nothrow) TaskAutoSurvey(unit));

    TaskManager.AppendTask(*task);
}

bool Ai_IsDangerousLocation(UnitInfo* unit, Point destination, int32_t caution_level, bool is_for_attacking) {
    bool result;

    if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER && caution_level != CAUTION_LEVEL_NONE) {
        int16_t** damage_potential_map =
            AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit, caution_level, is_for_attacking);
        int32_t unit_hits = unit->hits;

        if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
            unit_hits = 1;
        }

        if (damage_potential_map && damage_potential_map[destination.x][destination.y] >= unit_hits) {
            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void Ai_UpdateTerrainDistanceField(UnitInfo* unit) {
    if (AiPlayer_TerrainDistanceField) {
        if (unit->flags & STATIONARY) {
            if (unit->GetUnitType() == BRIDGE) {
                AiPlayer_TerrainDistanceField->OnTerrainChanged(Point(unit->grid_x, unit->grid_y),
                                                                SURFACE_TYPE_LAND | SURFACE_TYPE_WATER);
            }

            if (unit->GetUnitType() == WTRPLTFM) {
                AiPlayer_TerrainDistanceField->OnTerrainChanged(Point(unit->grid_x, unit->grid_y), SURFACE_TYPE_LAND);
            }

            if (unit->GetUnitType() != CNCT_4W && !(unit->flags & GROUND_COVER)) {
                AiPlayer_TerrainDistanceField->OnTerrainChanged(Point(unit->grid_x, unit->grid_y), SURFACE_TYPE_NONE);
            }
        }
    }

    if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER &&
        unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0) {
        AiPlayer_Teams[unit->team].AddMilitaryUnit(unit);
    }
}

void Ai_CheckEndTurn() {
    if (GameManager_PlayMode == PLAY_MODE_TURN_BASED) {
        if (GameManager_GameState == GAME_STATE_8_IN_GAME) {
            if (UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type == TEAM_TYPE_COMPUTER) {
                if (!UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].finished_turn) {
                    AiPlayer_Teams[GameManager_ActiveTurnTeam].CheckEndTurn();
                }
            }
        }

    } else {
        if (GameManager_GameState == GAME_STATE_9_END_TURN || GameManager_GameState == GAME_STATE_8_IN_GAME) {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                    if (!UnitsManager_TeamInfo[team].finished_turn) {
                        if (AiPlayer_Teams[team].CheckEndTurn()) {
                            return;
                        }
                    }
                }
            }

            if (GameManager_GameState == GAME_STATE_8_IN_GAME) {
                if (UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type == TEAM_TYPE_COMPUTER) {
                    if (UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].finished_turn) {
                        AILOG(log, "Error- had to force ENDTURN!");
                        GameManager_GameState = GAME_STATE_9_END_TURN;
                    }
                }
            }
        }
    }
}

void Ai_ClearTasksPendingFlags() {
    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        AiPlayer_Teams[team].ChangeTasksPendingFlag(false);
    }
}

int32_t Ai_DetermineCautionLevel(UnitInfo* unit) {
    int32_t result;

    if (unit->GetAiStateBits() & UnitInfo::AI_STATE_NO_RETREAT) {
        result = CAUTION_LEVEL_NONE;

    } else if (unit->GetTask()) {
        result = unit->GetTask()->GetCautionLevel(*unit);

    } else {
        result = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
    }

    return result;
}

void Ai_RemoveUnit(UnitInfo* unit) {
    Point site;

    if (unit->flags & STATIONARY) {
        Rect bounds;
        int32_t surface_type;

        unit->GetBounds(&bounds);

        for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
            for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                surface_type = ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * site.y + site.x];

                const auto units = Hash_MapHash[Point(site.x, site.y)];

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if ((*it).GetUnitType() == BRIDGE) {
                            surface_type |= SURFACE_TYPE_LAND;
                        }

                        if ((*it).GetUnitType() == WTRPLTFM) {
                            surface_type = SURFACE_TYPE_LAND;
                        }

                        if ((*it).GetUnitType() != CNCT_4W &&
                            (((*it).flags & (GROUND_COVER | STATIONARY)) == STATIONARY)) {
                            surface_type = SURFACE_TYPE_NONE;
                            break;
                        }
                    }
                }

                if (AiPlayer_TerrainDistanceField) {
                    AiPlayer_TerrainDistanceField->OnTerrainChanged(site, surface_type);
                }
            }
        }
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            AiPlayer_Teams[team].RemoveUnit(unit);
        }
    }

    TaskManager.RemoveDestroyedUnit(unit);
}

void Ai_UnitSpotted(UnitInfo* unit, uint16_t team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER && (unit->flags & SELECTABLE)) {
        AiPlayer_Teams[team].UnitSpotted(unit);
    }

    TaskManager.AddSpottedUnit(unit);
}

bool Ai_IsTargetTeam(UnitInfo* unit, UnitInfo* target) {
    bool teams[PLAYER_TEAM_MAX];

    AiAttack_GetTargetTeams(unit->team, teams);

    return teams[target->team];
}

void Ai_EvaluateAttackTargets(UnitInfo* unit) { TaskManager.CollectPotentialAttackTargets(unit); }

void Ai_CheckComputerReactions() { TaskManager.CheckComputerReactions(); }

void Ai_CheckMines(UnitInfo* unit) {
    for (uint16_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (unit->team != team && UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER &&
            unit->IsVisibleToTeam(team)) {
            AiPlayer_Teams[team].FindMines(unit);
        }
    }
}

bool Ai_AreThereParticles() {
    bool result;

    if (UnitsManager_ParticleUnits.GetCount() > 0 || UnitsManager_PendingAttacks.GetCount() > 0) {
        result = true;

    } else {
        result = UnitsManager_byte_179448;
    }

    return result;
}

bool Ai_AreThereMovingUnits() {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).GetOrder() == ORDER_MOVE &&
            ((*it).GetOrderState() == ORDER_STATE_IN_PROGRESS || (*it).GetOrderState() == ORDER_STATE_IN_TRANSITION)) {
            return true;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).GetOrder() == ORDER_MOVE &&
            ((*it).GetOrderState() == ORDER_STATE_IN_PROGRESS || (*it).GetOrderState() == ORDER_STATE_IN_TRANSITION)) {
            return true;
        }
    }

    return false;
}

void Ai_CheckReactions() {
    if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
        if (Ai_AreThereParticles()) {
            Ai_AiReactionState = AI_REACTION_STATE_ANIMATIONS_ACTIVE;

        } else if (Ai_AiReactionState == AI_REACTION_STATE_ANIMATIONS_ACTIVE) {
            Ai_AiReactionState = AI_REACTION_STATE_PROCESSING;

            TaskManager.CheckComputerReactions();

        } else {
            if (Ai_AreThereMovingUnits()) {
                Ai_AiReactionState = AI_REACTION_STATE_PROCESSING;

            } else if (Ai_AiReactionState != AI_REACTION_STATE_IDLE) {
                Ai_AiReactionState = AI_REACTION_STATE_IDLE;

                TaskManager.CheckComputerReactions();

                return;
            }

            TaskManager.ExecuteReminders();
        }
    }
}

bool TaskManager_NeedToReserveRawMaterials(uint16_t team) {
    int32_t raw_materials = -10;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team &&
            ResourceManager_GetUnit((*it).GetUnitType()).GetCargoType() == Unit::CargoType::CARGO_TYPE_RAW) {
            raw_materials += (*it).storage;

            if ((*it).GetUnitType() == ENGINEER) {
                raw_materials += -15;
            }

            if ((*it).GetUnitType() == CONSTRCT) {
                raw_materials += -30;
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team) {
            raw_materials -= Cargo_GetRawConsumptionRate((*it).GetUnitType(), 1);

            if (ResourceManager_GetUnit((*it).GetUnitType()).GetCargoType() == Unit::CargoType::CARGO_TYPE_RAW) {
                raw_materials += (*it).storage;

                if ((*it).storage == (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                    return false;
                }
            }
        }
    }

    return raw_materials < 0;
}
