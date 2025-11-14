/* Copyright (c) 2021 M.A.X. Port Team
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

#include "task.hpp"

#include <format>

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "inifile.hpp"
#include "reminders.hpp"
#include "task_manager.hpp"
#include "taskretreat.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"
#include "zonewalker.hpp"

uint32_t Task::task_id = 0;
uint32_t Task::task_count = 0;

static const std::string Task_TaskNames[] = {"Activate",
                                             "AssistMove",
                                             "Attack",
                                             "AttackReserve",
                                             "AutoSurvey",
                                             "Unknown",
                                             "CheckAssaults",
                                             "ClearZone",
                                             "CreateUnit",
                                             "CreateBuilding",
                                             "Escort",
                                             "Explore",
                                             "DefenseAssistant",
                                             "DefenseReserve",
                                             "Dump",
                                             "FindMines",
                                             "FindPath",
                                             "FrontalAttack",
                                             "GetMaterials",
                                             "HabitatAssistant",
                                             "KillUnit",
                                             "ManageBuildings",
                                             "MineAssisstant",
                                             "Move",
                                             "MoveHome",
                                             "ObtainUnits",
                                             "PlaceMines",
                                             "ConnectionAssistant",
                                             "RadarAssistant",
                                             "FrontierAssistant",
                                             "PowerAssistant",
                                             "Unknown",
                                             "Unknown",
                                             "Reload",
                                             "RemoveMines",
                                             "RemoveRubble",
                                             "Rendezvous",
                                             "Repair",
                                             "Retreat",
                                             "SearchDestination",
                                             "Scavenge",
                                             "Unknown",
                                             "SupportAttack",
                                             "Survey",
                                             "Unknown",
                                             "Transport",
                                             "UpdateTerrain",
                                             "Upgrade",
                                             "WaitToAttack"};

static SmartList<UnitInfo>* Task_GetUnitList(ResourceID unit_type);

std::string_view Task_GetName(Task* task) {
    return task ? std::string_view(Task_TaskNames[task->GetType()]) : std::string_view("");
}

void Task_RemindMoveFinished(UnitInfo* unit, bool priority) {
    if (unit && (unit->GetAiStateBits() & UnitInfo::AI_STATE_MOVE_FINISHED_REMINDER) == 0) {
        TaskManager.AppendReminder(new (std::nothrow) RemindMoveFinished(*unit), priority);
    }
}

bool Task_IsReadyToTakeOrders(UnitInfo* unit) {
    bool result;

    if (unit->hits > 0 && GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
        if (unit->GetOrder() == ORDER_AWAIT || unit->GetOrder() == ORDER_SENTRY ||
            (unit->GetOrder() == ORDER_MOVE && unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER) ||
            (unit->GetOrder() == ORDER_MOVE_TO_UNIT && unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER)) {
            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void Task_RemoveMovementTasks(UnitInfo* unit) {
    if (unit->GetOrder() != ORDER_IDLE || unit->hits <= 0) {
        for (SmartList<Task>::Iterator it = unit->GetTasks().Begin(); it != unit->GetTasks().End(); ++it) {
            if ((*it).GetType() == TaskType_TaskMove || (*it).GetType() == TaskType_TaskFindPath) {
                AILOG(log, "Move {}: removing old move task",
                      UnitsManager_BaseUnits[unit->GetUnitType()].GetSingularName());

                unit->RemoveTask(&*it, false);
                (*it).RemoveUnit(*unit);
            }
        }
    }
}

bool Task_ShouldReserveShot(UnitInfo* unit, Point site) {
    bool result;

    if (unit->shots > 0 && !unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
        int32_t unit_range = 0;
        bool relevant_teams[PLAYER_TEAM_MAX - 1];
        int32_t team;

        for (team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                if ((unit->GetUnitType() == SUBMARNE || unit->GetUnitType() == CLNTRANS) &&
                    Access_GetModifiedSurfaceType(site.x, site.y) == SURFACE_TYPE_WATER &&
                    !unit->IsDetectedByTeam(team)) {
                    if (UnitsManager_TeamInfo[team].heat_map_stealth_sea[ResourceManager_MapSize.x * site.y + site.x]) {
                        relevant_teams[team] = true;

                    } else {
                        relevant_teams[team] = false;
                    }

                } else if (unit->GetUnitType() == COMMANDO && !unit->IsDetectedByTeam(team)) {
                    if (UnitsManager_TeamInfo[team]
                            .heat_map_stealth_land[ResourceManager_MapSize.x * site.y + site.x]) {
                        relevant_teams[team] = true;

                    } else {
                        relevant_teams[team] = false;
                    }

                } else {
                    if (UnitsManager_TeamInfo[team].heat_map_complete[ResourceManager_MapSize.x * site.y + site.x]) {
                        relevant_teams[team] = true;

                    } else {
                        relevant_teams[team] = false;
                    }
                }

            } else {
                relevant_teams[team] = false;
            }
        }

        for (team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (relevant_teams[team] && team != unit->team) {
                break;
            }
        }

        if (team != PLAYER_TEAM_ALIEN) {
            AILOG(log, "Determine if {} should reserve a shot at [{},{}].",
                  UnitsManager_BaseUnits[unit->GetUnitType()].GetSingularName(), site.x + 1, site.y + 1);

            if (Ai_IsDangerousLocation(unit, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, true)) {
                for (SmartList<SpottedUnit>::Iterator it = AiPlayer_Teams[unit->team].GetSpottedUnits().Begin();
                     it != AiPlayer_Teams[unit->team].GetSpottedUnits().End(); ++it) {
                    UnitInfo* spotted_unit = (*it).GetUnit();
                    int32_t spotted_unit_range = spotted_unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

                    if (spotted_unit->ammo > 0 && relevant_teams[spotted_unit->team] &&
                        spotted_unit_range > unit_range &&
                        Access_IsValidAttackTarget(spotted_unit->GetUnitType(), unit->GetUnitType(), site)) {
                        UnitValues* unit_values = spotted_unit->GetBaseValues();
                        int32_t unit_distance = Access_GetSquaredDistance(unit, (*it).GetLastPosition());
                        int32_t attack_distance = unit_values->GetAttribute(ATTRIB_ATTACK_RADIUS);

                        if (unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                            attack_distance += unit_values->GetAttribute(ATTRIB_SPEED) / 2;

                        } else {
                            attack_distance += ((unit_values->GetAttribute(ATTRIB_ROUNDS) - 1) *
                                                (unit_values->GetAttribute(ATTRIB_SPEED) + 1)) /
                                               unit_values->GetAttribute(ATTRIB_ROUNDS);
                        }

                        if (unit_distance <= attack_distance * attack_distance) {
                            if (spotted_unit_range <= unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE)) {
                                if (Access_IsValidAttackTargetType(unit->GetUnitType(), spotted_unit->GetUnitType())) {
                                    unit_range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

                                } else {
                                    AILOG_LOG(log, "{} at [{},{}] outranges us.",
                                              UnitsManager_BaseUnits[spotted_unit->GetUnitType()].GetSingularName(),
                                              (*it).GetLastPositionX(), (*it).GetLastPositionY());

                                    return false;
                                }

                            } else {
                                return false;
                            }
                        }
                    }
                }

                if (unit_range) {
                    AILOG_LOG(log, "Square is dangerous but there are no longer range threats.");

                    result = true;

                } else {
                    AILOG_LOG(log, "No danger.");

                    result = false;
                }

            } else {
                result = false;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool Task_IsUnitDoomedToDestruction(UnitInfo* unit, int32_t caution_level) {
    AiPlayer* ai_player = &AiPlayer_Teams[unit->team];
    Point position(unit->grid_x, unit->grid_y);
    int32_t unit_hits = unit->hits;
    bool result;

    if (unit->GetAiStateBits() & UnitInfo::AI_STATE_NO_RETREAT) {
        result = true;

    } else {
        if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
            unit_hits = 1;
        }

        if (ai_player->GetDamagePotential(unit, position, caution_level, true) < unit_hits) {
            result = false;

        } else if (unit->speed > 0) {
            int16_t** damage_potential_map = ai_player->GetDamagePotentialMap(unit, caution_level, true);

            if (damage_potential_map) {
                ZoneWalker walker(position, unit->speed);

                do {
                    if (damage_potential_map[walker.GetGridX()][walker.GetGridY()] < unit_hits &&
                        Access_IsAccessible(unit->GetUnitType(), unit->team, walker.GetGridX(), walker.GetGridY(),
                                            AccessModifier_SameClassBlocks)) {
                        return false;
                    }
                } while (walker.FindNext());

                unit->ChangeAiStateBits(UnitInfo::AI_STATE_NO_RETREAT, true);

                result = true;

            } else {
                result = false;
            }

        } else {
            result = true;
        }
    }

    return result;
}

bool Task_IsAdjacent(UnitInfo* unit, int16_t grid_x, int16_t grid_y) {
    int32_t unit_size;

    if (unit->flags & BUILDING) {
        unit_size = 2;

    } else {
        unit_size = 1;
    }

    return unit->grid_x - 1 <= grid_x && unit->grid_x + unit_size >= grid_x && unit->grid_y - 1 <= grid_y &&
           unit->grid_y + unit_size >= grid_y;
}

int32_t Task_EstimateTurnsTillMissionEnd() {
    int32_t victory_limit = ini_setting_victory_limit;
    int32_t result;

    if (ini_setting_victory_type == VICTORY_TYPE_SCORE) {
        int32_t remaining_turns = 1000;
        int32_t turns;

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                int32_t count = 0;

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team == team && (*it).GetUnitType() == GREENHSE && (*it).GetOrder() == ORDER_POWER_ON) {
                        ++count;
                    }
                }

                if (count > 0) {
                    turns = (victory_limit - UnitsManager_TeamInfo[team].team_points) / count;

                    if (turns < remaining_turns) {
                        remaining_turns = turns;
                    }
                }
            }
        }

        result = remaining_turns;

    } else {
        result = victory_limit - GameManager_TurnCounter;
    }

    return result;
}

Task::Task(uint16_t team, Task* parent, uint16_t priority)
    : m_id(++task_id),
      m_team(team),
      m_parent(parent),
      m_base_priority(priority),
      m_needs_processing(true),
      m_scheduled_for_turn_start(false),
      m_scheduled_for_turn_end(false) {
    ++task_count;
}

Task::Task(const Task& other)
    : m_id(other.m_id),
      m_team(other.m_team),
      m_parent(other.m_parent),
      m_base_priority(other.m_base_priority),
      m_needs_processing(other.m_needs_processing),
      m_scheduled_for_turn_start(other.m_scheduled_for_turn_start),
      m_scheduled_for_turn_end(other.m_scheduled_for_turn_end) {
    ++task_count;
}

Task::~Task() { --task_count; }

void Task::RemindTurnEnd(bool priority) {
    if (!IsScheduledForTurnEnd()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this), priority);
    }
}

void Task::RemindTurnStart(bool priority) {
    if (!IsScheduledForTurnStart()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this), priority);
    }
}

bool Task::IsProcessingNeeded() const { return m_needs_processing; }

void Task::SetProcessingNeeded(bool value) { m_needs_processing = value; }

bool Task::IsScheduledForTurnStart() const { return m_scheduled_for_turn_start; }

void Task::ChangeIsScheduledForTurnStart(bool value) { m_scheduled_for_turn_start = value; }

bool Task::IsScheduledForTurnEnd() const { return m_scheduled_for_turn_end; }

void Task::ChangeIsScheduledForTurnEnd(bool value) { m_scheduled_for_turn_end = value; }

uint16_t Task::GetTeam() const { return m_team; }

Task* Task::GetParent() { return &*m_parent; }

void Task::SetParent(Task* task) { m_parent = task; }

void Task::SetPriority(uint16_t priority) { m_base_priority = priority; }

int16_t Task::ComparePriority(uint16_t task_priority) {
    int32_t result;

    if (m_base_priority + TASK_PRIORITY_TOLERANCE >= task_priority) {
        if (m_base_priority - TASK_PRIORITY_TOLERANCE <= task_priority) {
            result = GetPriority() - task_priority;
        } else {
            result = 1;
        }
    } else {
        result = -1;
    }

    return result;
}

Point Task::DeterminePosition() {
    Rect bounds;

    GetBounds(&bounds);

    return Point(bounds.ulx, bounds.uly);
}

bool Task_RetreatIfNecessary(Task* task, UnitInfo* unit, int32_t caution_level) {
    bool result;

    if (caution_level == CAUTION_LEVEL_NONE || (unit->GetAiStateBits() & UnitInfo::AI_STATE_NO_RETREAT)) {
        result = false;

    } else if (unit->speed > 0) {
        if (unit->ammo == 0 && unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0) {
            caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
        }

        if (unit->shots == 0 && unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
            ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
            caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
        }

        Point position(unit->grid_x, unit->grid_y);
        int32_t unit_hits = unit->hits;

        if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
            unit_hits = 1;
        }

        if (AiPlayer_Teams[unit->team].GetDamagePotential(unit, position, caution_level, false) >= unit_hits) {
            if (unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) > 0 &&
                (unit->shots > 0 || !unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) ||
                 ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_AVERAGE) &&
                Task_IsUnitDoomedToDestruction(unit, caution_level)) {
                unit->ChangeAiStateBits(UnitInfo::AI_STATE_NO_RETREAT, true);

                result = false;

            } else {
                SmartPointer<TaskRetreat> retreat_task(new (std::nothrow) TaskRetreat(unit, task, 0x00, caution_level));

                TaskManager.AppendTask(*retreat_task);

                result = true;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool Task_RetreatFromDanger(Task* task, UnitInfo* unit, int32_t caution_level) {
    bool result;

    if (task->IsProcessingNeeded() && Task_RetreatIfNecessary(task, unit, caution_level)) {
        task->SetProcessingNeeded(false);

        result = true;

    } else {
        result = false;
    }

    return result;
}

SmartList<UnitInfo>* Task_GetUnitList(ResourceID unit_type) {
    uint32_t flags = UnitsManager_BaseUnits[unit_type].flags;
    SmartList<UnitInfo>* list;

    if (flags & STATIONARY) {
        if (flags & GROUND_COVER) {
            list = &UnitsManager_GroundCoverUnits;

        } else {
            list = &UnitsManager_StationaryUnits;
        }

    } else if (flags & MOBILE_AIR_UNIT) {
        list = &UnitsManager_MobileAirUnits;

    } else {
        list = &UnitsManager_MobileLandSeaUnits;
    }

    return list;
}

int32_t Task_GetReadyUnitsCount(uint16_t team, ResourceID unit_type) {
    SmartList<UnitInfo>* units = Task_GetUnitList(unit_type);
    int32_t count = 0;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type &&
            (*it).GetOrderState() != ORDER_STATE_BUILDING_READY) {
            ++count;
        }
    }

    return count;
}

bool Task::IsUnitTransferable(UnitInfo& unit) {
    bool result = true;

    if (m_parent != nullptr && !m_parent->IsUnitTransferable(unit)) {
        result = false;
    }

    return result;
}

bool Task::IsUnitUsable(UnitInfo& unit) { return false; }

int32_t Task::GetCautionLevel(UnitInfo& unit) {
    int32_t result;

    if (unit.base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
        ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        if (unit.shots > 0) {
            result = CAUTION_LEVEL_AVOID_REACTION_FIRE;
        } else {
            result = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
        }
    } else if (unit.ammo) {
        result = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
    } else {
        result = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
    }

    return result;
}

uint16_t Task::GetPriority() const { return m_base_priority; }

Rect* Task::GetBounds(Rect* bounds) {
    bounds->ulx = 0;
    bounds->uly = 0;
    bounds->lrx = 0;
    bounds->lry = 0;

    return bounds;
}

bool Task::IsNeeded() {
    bool result;

    if (m_parent != nullptr) {
        result = m_parent->IsNeeded();
    } else {
        result = true;
    }

    return result;
}

bool Task::IsThinking() { return false; }

void Task::AddUnit(UnitInfo& unit) {}

void Task::Init() { RemindTurnStart(true); }

void Task::BeginTurn() { EndTurn(); }

void Task::ChildComplete(Task* task) {}

void Task::EndTurn() {}

bool Task::ExchangeOperator(UnitInfo& unit) { return false; }

bool Task::Execute(UnitInfo& unit) { return false; }

void Task::RemoveSelf() {
    m_parent = nullptr;
    TaskManager.RemoveTask(*this);
}

bool Task::CheckReactions() { return false; }

void Task::EventUnitRefueled(UnitInfo& unit) {}

void Task::RemoveUnit(UnitInfo& unit) {}

void Task::EventCargoTransfer(UnitInfo& unit) {}

void Task::EventUnitDestroyed(UnitInfo& unit) {}

void Task::EventUnitLoaded(UnitInfo& unit1, UnitInfo& unit2) {}

void Task::EventEnemyUnitSpotted(UnitInfo& unit) {}

void Task::EventUnitUnloaded(UnitInfo& unit1, UnitInfo& unit2) {}

void Task::EventZoneCleared(Zone* zone, bool status) {}

uint32_t Task::GetId() const { return m_id; }
