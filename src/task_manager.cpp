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

#include "task_manager.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "inifile.hpp"
#include "missionmanager.hpp"
#include "reminders.hpp"
#include "resource_manager.hpp"
#include "taskactivate.hpp"
#include "taskcreateunit.hpp"
#include "taskreload.hpp"
#include "taskrepair.hpp"
#include "taskupgrade.hpp"
#include "ticktimer.hpp"
#include "units_manager.hpp"

class TaskManager TaskManager;

uint16_t TaskManager_word_1731C0;

static const char* const TaskManager_TaskNames[] = {"Activate",
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
                                                    "FrontierAssistant",
                                                    "Move",
                                                    "MoveHome",
                                                    "ObtainUnits",
                                                    "PlaceMines",
                                                    "ConnectionAssistant",
                                                    "RadarAssistant",
                                                    "Unknown",
                                                    "Unknown",
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

int32_t TaskManager_GetDistance(int32_t distance_x, int32_t distance_y) {
    int32_t result;

    distance_x = labs(distance_x);
    distance_y = labs(distance_y);

    if (distance_x >= distance_y) {
        result = 2 * distance_x + distance_y;
    } else {
        result = 2 * distance_y + distance_x;
    }

    return result;
}

int32_t TaskManager_GetDistance(Point point1, Point point2) {
    return TaskManager_GetDistance(point1.x - point2.x, point1.y - point2.y);
}

int32_t TaskManager_GetDistance(UnitInfo* unit1, UnitInfo* unit2) {
    return TaskManager_GetDistance(unit1->grid_x - unit2->grid_x, unit1->grid_y - unit2->grid_y);
}

bool TaskManager_NeedToReserveRawMaterials(uint16_t team) {
    int32_t raw_materials = -10;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type == CARGO_TYPE_RAW) {
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

            if (UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type == CARGO_TYPE_RAW) {
                raw_materials += (*it).storage;

                if ((*it).storage == (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                    return false;
                }
            }
        }
    }

    return raw_materials < 0;
}

TaskManager::TaskManager() : reminder_counter(0) {}

TaskManager::~TaskManager() {}

bool TaskManager::IsUnitNeeded(ResourceID unit_type, uint16_t team, uint16_t flags) {
    bool result;

    AILOG(log, "Task: should build {}?", UnitsManager_BaseUnits[unit_type].singular_name);

    int32_t available_count = 0;
    int32_t requested_count = 0;
    int32_t turns_till_mission_end = Task_EstimateTurnsTillMissionEnd();
    SmartList<UnitInfo>* unit_list;

    if (turns_till_mission_end >=
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_TURNS) + 5) {
        if (unit_type != CONSTRCT || turns_till_mission_end >= 25) {
            if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
                unit_list = &UnitsManager_StationaryUnits;

            } else {
                unit_list = &UnitsManager_MobileLandSeaUnits;
            }

            for (SmartList<UnitInfo>::Iterator it = unit_list->Begin(); it != unit_list->End(); ++it) {
                if ((*it).team == team && (*it).GetUnitType() == unit_type) {
                    ++available_count;

                    if (!(*it).GetTask()) {
                        return false;
                    }
                }
            }

            for (SmartList<TaskObtainUnits>::Iterator it = unit_requests.Begin(); it != unit_requests.End(); ++it) {
                if ((*it).GetTeam() == team && (*it).CountInstancesOfUnitType(unit_type) &&
                    (*it).DeterminePriority(flags + 250) <= 0) {
                    requested_count += (*it).CountInstancesOfUnitType(unit_type);
                }
            }

            for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                if ((*it).GetTeam() == team &&
                    ((*it).GetType() == TaskType_TaskCreateBuilding || (*it).GetType() == TaskType_TaskCreateUnit)) {
                    TaskCreate* create_task = dynamic_cast<TaskCreate*>(&*it);

                    if (create_task->GetUnitType() == unit_type) {
                        if (create_task->Task_vfunc28() || create_task->DeterminePriority(flags + 250) <= 0) {
                            return false;
                        }
                    }
                }
            }

            if (requested_count >= available_count) {
                if (available_count <= 0) {
                    result = true;

                } else if (TaskManager_NeedToReserveRawMaterials(team)) {
                    AILOG_LOG(log, "No, existing {} have a materials shortage",
                              UnitsManager_BaseUnits[unit_type].plural_name);

                    result = false;

                } else {
                    result = true;
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

bool TaskManager::CheckTasksThinking(uint16_t team) {
    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetTeam() == team && (*it).IsThinking()) {
            char text[200];

            AILOG(log, "Task thinking: {}", (*it).WriteStatusLog(text));

            return true;
        }
    }

    return false;
}

void TaskManager::CheckComputerReactions() {
    if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
        if (GameManager_PlayMode != PLAY_MODE_TURN_BASED ||
            UnitsManager_TeamInfo[GameManager_ActiveTurnTeam].team_type == TEAM_TYPE_COMPUTER) {
            AILOG(log, "Checking computer reactions");

            for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                if (GameManager_IsActiveTurn((*it).GetTeam())) {
                    if ((*it).CheckReactions()) {
                        return;
                    }
                }
            }

            if (TaskManager_word_1731C0 == 0) {
                for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
                    if (GameManager_IsActiveTurn((*it).team) && (*it).hits > 0 && (*it).speed > 0 &&
                        Task_IsReadyToTakeOrders(&*it) &&
                        UnitsManager_TeamInfo[(*it).team].team_type == TEAM_TYPE_COMPUTER) {
                        if ((*it).GetTask()) {
                            Task_RetreatFromDanger((*it).GetTask(), &*it, Ai_DetermineCautionLevel(&*it));

                        } else {
                            Task_RetreatIfNecessary(nullptr, &*it, Ai_DetermineCautionLevel(&*it));
                        }

                        units.Remove(*it);

                        if (!TickTimer_HaveTimeToThink()) {
                            TaskManager_word_1731C0 = 1;

                            return;
                        }
                    }
                }
            }
        }
    }
}

void TaskManager::EnumeratePotentialAttackTargets(UnitInfo* unit) {
    if (unit->ammo) {
        int32_t range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

        range = range * range;

        if (Access_GetValidAttackTargetTypes(unit->GetUnitType()) & MOBILE_LAND_UNIT) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team != unit->team && (*it).IsVisibleToTeam(unit->team) &&
                    UnitsManager_TeamInfo[(*it).team].team_type == TEAM_TYPE_COMPUTER &&
                    Access_GetDistance(&*it, unit) <= range) {
                    units.PushBack(*it);
                }
            }
        }

        if (Access_GetValidAttackTargetTypes(unit->GetUnitType()) & MOBILE_AIR_UNIT) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                 it != UnitsManager_MobileAirUnits.End(); ++it) {
                if ((*it).team != unit->team && (*it).IsVisibleToTeam(unit->team) &&
                    UnitsManager_TeamInfo[(*it).team].team_type == TEAM_TYPE_COMPUTER &&
                    Access_GetDistance(&*it, unit) <= range) {
                    units.PushBack(*it);
                }
            }
        }
    }
}

void TaskManager::AppendUnit(UnitInfo& unit) { units.PushBack(unit); }

void TaskManager::CreateBuilding(ResourceID unit_type, uint16_t team, Point site, Task* task) {
    if (IsUnitNeeded(unit_type, team, task->GetFlags())) {
        AiPlayer_Teams[team].CreateBuilding(unit_type, site, task);
    }
}

void TaskManager::CreateUnit(ResourceID unit_type, uint16_t team, Point site, Task* task) {
    if (IsUnitNeeded(unit_type, team, task->GetFlags())) {
        SmartPointer<TaskCreateUnit> create_unit_task(new (std::nothrow) TaskCreateUnit(unit_type, task, site));

        AppendTask(*create_unit_task);
    }
}

void TaskManager::ManufactureUnits(ResourceID unit_type, uint16_t team, int32_t requested_amount, Task* task,
                                   Point site) {
    uint16_t task_flags = task->GetFlags();
    uint16_t task_team = task->GetTeam();

    AILOG(log, "Task: Request {}.", UnitsManager_BaseUnits[unit_type].singular_name);

    if (Task_EstimateTurnsTillMissionEnd() >=
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[task_team], unit_type)->GetAttribute(ATTRIB_TURNS)) {
        uint16_t unit_counters[UNIT_END];
        const auto builder_type = Builder_GetBuilderType(unit_type);
        int32_t builder_count;
        int32_t buildable_count;
        int32_t unit_count;

        memset(unit_counters, 0, sizeof(unit_counters));

        for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
            if ((*it).GetTeam() == task->GetTeam() && (*it).GetType() == TaskType_TaskCreateUnit) {
                if ((*it).DeterminePriority(task_flags + 250) <= 0) {
                    ++unit_counters[dynamic_cast<TaskCreateUnit*>(&*it)->GetUnitType()];
                }
            }
        }

        builder_count = 0;
        unit_count = 0;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == task_team && (*it).GetUnitType() == builder_type) {
                ++builder_count;
            }
        }

        for (const auto unit : Builder_GetBuildableUnits(builder_type)) {
            unit_count += unit_counters[unit];
        }

        buildable_count = (builder_count * 2 + 1) - unit_count;

        if (buildable_count > requested_amount) {
            buildable_count = requested_amount;
        }

        for (int32_t i = 0; i < buildable_count; ++i) {
            SmartPointer<TaskCreateUnit> create_unit_task(new (std::nothrow) TaskCreateUnit(unit_type, task, site));

            AppendTask(*create_unit_task);
        }
    }
}

void TaskManager::AppendTask(Task& task) {
    AILOG(log, "Task Manager: append task '{}'.", TaskManager_GetTaskName(&task));

    tasks.PushBack(task);

    if (task.GetType() == TaskType_TaskObtainUnits) {
        unit_requests.PushBack(*dynamic_cast<TaskObtainUnits*>(&task));
    }

    task.Begin();
}

void TaskManager::AppendReminder(Reminder* reminder, bool priority) {
    if (priority) {
        priority_reminders.PushBack(*reminder);

    } else {
        normal_reminders.PushBack(*reminder);
    }
}

bool TaskManager::ExecuteReminders() {
    bool result;

    if (normal_reminders.GetCount() + priority_reminders.GetCount() > 0) {
        AILOG(log, "Execute reminders");

        if (TickTimer_HaveTimeToThink()) {
            SmartPointer<Reminder> reminder;
            int32_t reminders_executed = 0;

            while (normal_reminders.GetCount() + priority_reminders.GetCount() > 0) {
                if (normal_reminders.GetCount() == 0) {
                    reminder_counter = 0;
                }

                if (reminder_counter >= 2 || priority_reminders.GetCount() == 0) {
                    reminder = normal_reminders[0];

                    normal_reminders.Remove(*reminder);

                    reminder_counter = 0;

                } else {
                    reminder = priority_reminders[0];

                    priority_reminders.Remove(*reminder);

                    ++reminder_counter;
                }

                ++reminders_executed;

                reminder->Execute();

                if (!TickTimer_HaveTimeToThink()) {
                    AILOG_LOG(log, "{} reminders executed", reminders_executed);
                    break;
                }
            }

        } else {
            AILOG_LOG(log, "No reminders executed, {} msecs since frame update", TickTimer_GetElapsedTime());
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

void TaskManager::BeginTurn(uint16_t team) {
    AILOG(log, "Task Manager: begin turn.");

    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetTeam() == team && (*it).GetType() != TaskType_TaskTransport) {
            (*it).ChangeInitNeededFlag(true);
        }
    }

    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetTeam() == team && (*it).GetType() != TaskType_TaskTransport) {
            (*it).RemindTurnStart();
        }
    }

    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskTransport) {
            (*it).RemindTurnStart();
        }
    }

    if (AiLog_IsEnabled()) {
        uint16_t reminders[REMINDER_TYPE_COUNT];

        memset(reminders, 0, sizeof(reminders));

        for (SmartList<Reminder>::Iterator it = normal_reminders.Begin(); it != normal_reminders.End(); ++it) {
            ++reminders[(*it).GetType()];
        }

        AILOG_LOG(log, "Turn start reminders: {}", reminders[REMINDER_TYPE_TURN_START]);
        AILOG_LOG(log, "Turn end reminders: {}", reminders[REMINDER_TYPE_TURN_END]);
        AILOG_LOG(log, "Available reminders: {}", reminders[REMINDER_TYPE_AVAILABLE]);
        AILOG_LOG(log, "Move reminders: {}", reminders[REMINDER_TYPE_MOVE]);
        AILOG_LOG(log, "Attack reminders: {}", reminders[REMINDER_TYPE_ATTACK]);
    }
}

void TaskManager::EndTurn(uint16_t team) {
    AILOG(log, "Task Manager: end turn.");

    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetTeam() == team) {
            (*it).RemindTurnEnd();
        }
    }
}

void TaskManager::ChangeFlagsSet(uint16_t team) {
    AILOG(log, "Task Manager: change flags set");

    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetTeam() == team) {
            (*it).ChangeInitNeededFlag(true);
        }
    }
}

void TaskManager::Clear() {
    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        (*it).RemoveSelf();
    }

    tasks.Clear();
    unit_requests.Clear();
    normal_reminders.Clear();
    priority_reminders.Clear();
    units.Clear();
}

void TaskManager::RemindAvailable(UnitInfo* unit, bool priority) {
    SmartPointer<UnitInfo> backup(unit);
    char unit_name[200];

    unit->GetDisplayName(unit_name, sizeof(unit_name));

    AILOG(log, "Task manager: make {} at [{},{}] available.", unit_name, unit->grid_x + 1, unit->grid_y + 1);

    unit->RemoveTasks();
    unit->ChangeAiStateBits(UnitInfo::AI_STATE_MOVE_FINISHED_REMINDER, false);

    if (unit->hits > 0 && UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
        Reminder* reminder = new (std::nothrow) class RemindAvailable(*unit);

        AppendReminder(reminder, priority);
    }
}

void TaskManager::FindTaskForUnit(UnitInfo* unit) {
    if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
        if (!(unit->flags & STATIONARY) || unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO) ||
            unit->GetUnitType() == LIGHTPLT || unit->GetUnitType() == LANDPLT || unit->GetUnitType() == SHIPYARD ||
            unit->GetUnitType() == AIRPLT || unit->GetUnitType() == TRAINHAL) {
            if (!unit->GetTask() && unit->hits > 0) {
                SmartPointer<TaskObtainUnits> obtain_units_task;
                uint16_t task_flags{UINT16_MAX};

                if (AiLog_IsEnabled()) {
                    char unit_name[300];

                    unit->GetDisplayName(unit_name, sizeof(unit_name));

                    AILOG(log, "Task manager: find a task for {}.", unit_name);
                }

                {
                    /* in case a builder unit is blocked from activating another unit is disabled
                     * make sure to restore operating states in a way to be able to continue the
                     * requested activation later
                     */
                    auto client{unit->GetParent()};

                    if (client) {
                        auto client_task{client->GetTask()};

                        if (client_task && client_task->GetType() == TaskType_TaskActivate) {
                            auto activate_task{dynamic_cast<TaskActivate*>(client_task)};

                            if (activate_task->GetContainer() == unit) {
                                if (unit->GetOrder() == ORDER_HALT_BUILDING_2) {
                                    UnitsManager_SetNewOrder(unit, ORDER_BUILD, ORDER_STATE_UNIT_READY);
                                }

                                return;
                            }
                        }
                    }
                }

                {
                    /* in case a taskless payload unit is waiting to be activated by a container unit
                     * do not assign it any tasks to be able to continue the requested activation later
                     */
                    if (unit->GetOrder() == ORDER_IDLE && unit->GetOrderState() == ORDER_STATE_STORE) {
                        auto container{unit->GetParent()};

                        if (container && container->GetParent() == unit) {
                            Rect bounds;
                            auto position{Point(unit->grid_x, unit->grid_y)};

                            container->GetBounds(&bounds);

                            if (Access_IsInsideBounds(&bounds, &position)) {
                                return;
                            }
                        }
                    }
                }

                if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_APPRENTICE) {
                    if (unit->GetBaseValues()->GetAttribute(ATTRIB_HITS) != unit->hits ||
                        (unit->GetOrderState() == ORDER_STATE_STORE &&
                         unit->GetParent()->GetOrderState() != ORDER_STATE_UNIT_READY)) {
                        if (!(unit->flags & STATIONARY)) {
                            SmartPointer<Task> repair_task(new (std::nothrow) TaskRepair(unit));

                            AppendTask(*repair_task);

                            return;
                        }
                    }
                }

                if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_EXPERT) {
                    if (unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
                        if (!(unit->flags & REGENERATING_UNIT)) {
                            if (AiPlayer_Teams[unit->team].ShouldUpgradeUnit(unit)) {
                                SmartPointer<Task> upgrade_task(new (std::nothrow) TaskUpgrade(unit));

                                AppendTask(*upgrade_task);

                                return;
                            }
                        }
                    }
                }

                if (unit->ammo < unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS)) {
                    SmartPointer<Task> reload_task(new (std::nothrow) TaskReload(unit));

                    AppendTask(*reload_task);

                    return;
                }

                if (unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
                    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                        if ((*it).GetTeam() == unit->team && (*it).ExchangeOperator(*unit)) {
                            return;
                        }
                    }
                }

                for (SmartList<TaskObtainUnits>::Iterator it = unit_requests.Begin(); it != unit_requests.End(); ++it) {
                    if ((*it).GetTeam() == unit->team) {
                        if ((!obtain_units_task || (*it).DeterminePriority(task_flags) < 0) &&
                            (*it).CountInstancesOfUnitType(unit->GetUnitType())) {
                            obtain_units_task = *it;
                            task_flags = obtain_units_task->GetFlags();
                        }
                    }
                }

                if (obtain_units_task) {
                    obtain_units_task->AddUnit(*unit);

                    return;
                }

                if (unit->GetUnitType() == LIGHTPLT || unit->GetUnitType() == LANDPLT ||
                    unit->GetUnitType() == SHIPYARD || unit->GetUnitType() == AIRPLT ||
                    unit->GetUnitType() == TRAINHAL || unit->GetUnitType() == CONSTRCT ||
                    unit->GetUnitType() == ENGINEER) {
                    for (SmartList<TaskObtainUnits>::Iterator it = unit_requests.Begin(); it != unit_requests.End();
                         ++it) {
                        if ((*it).GetTeam() == unit->team) {
                            (*it).Begin();
                        }
                    }
                }

                SmartPointer<Task> best_task;
                int32_t distance;
                int32_t minimum_distance{INT32_MAX};

                for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                    if ((*it).GetTeam() == unit->team && (*it).IsUnitUsable(*unit)) {
                        Rect bounds;

                        (*it).GetBounds(&bounds);

                        distance = (unit->grid_x - bounds.ulx) * (unit->grid_x - bounds.ulx) +
                                   (unit->grid_y - bounds.uly) * (unit->grid_y - bounds.uly);

                        if (!best_task || distance < minimum_distance) {
                            best_task = *it;
                            minimum_distance = distance;
                        }
                    }
                }

                if (best_task) {
                    best_task->AddUnit(*unit);
                }
            }
        }

    } else {
        unit->RemoveTasks();
    }
}

void TaskManager::RemoveTask(Task& task) {
    if (task.GetType() == TaskType_TaskObtainUnits) {
        unit_requests.Remove(*dynamic_cast<TaskObtainUnits*>(&task));
    }

    AILOG(log, "Task Manager: remove task '{}'.", TaskManager_GetTaskName(&task));

    tasks.Remove(task);
}

void TaskManager::RemoveDestroyedUnit(UnitInfo* unit) {
    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        (*it).EventUnitDestroyed(*unit);
    }
}

void TaskManager::AddSpottedUnit(UnitInfo* unit) {
    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        (*it).EventEnemyUnitSpotted(*unit);
    }
}

uint32_t TaskManager::GetRemindersCount() const { return normal_reminders.GetCount() + priority_reminders.GetCount(); }

SmartList<Task>& TaskManager::GetTaskList() { return tasks; }

const char* TaskManager_GetTaskName(Task* task) { return task ? TaskManager_TaskNames[task->GetType()] : ""; }
