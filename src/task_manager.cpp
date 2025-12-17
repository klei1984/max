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
#include "game_manager.hpp"
#include "missionmanager.hpp"
#include "reminders.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "taskactivate.hpp"
#include "taskcreateunit.hpp"
#include "taskreload.hpp"
#include "taskrepair.hpp"
#include "taskupgrade.hpp"
#include "ticktimer.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

class TaskManager TaskManager;

TaskManager::TaskManager() : reminder_counter(0) {}

TaskManager::~TaskManager() {}

bool TaskManager::IsUnitNeeded(ResourceID unit_type, uint16_t team, uint16_t task_priority) {
    bool result;

    AILOG(log, "Task: should build {}?", ResourceManager_GetUnit(unit_type).GetSingularName().data());

    int32_t available_count = 0;
    int32_t requested_count = 0;
    int32_t turns_till_mission_end = Task_EstimateTurnsTillMissionEnd();
    SmartList<UnitInfo>* unit_list;

    if (turns_till_mission_end >=
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_TURNS) + 5) {
        if (unit_type != CONSTRCT || turns_till_mission_end >= 25) {
            if (ResourceManager_GetUnit(unit_type).GetFlags() & STATIONARY) {
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
                    (*it).ComparePriority(task_priority + TASK_PRIORITY_ADJUST_MAJOR) <= 0) {
                    requested_count += (*it).CountInstancesOfUnitType(unit_type);
                }
            }

            for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                if ((*it).GetTeam() == team &&
                    ((*it).GetType() == TaskType_TaskCreateBuilding || (*it).GetType() == TaskType_TaskCreateUnit)) {
                    TaskCreate* create_task = dynamic_cast<TaskCreate*>(&*it);

                    if (create_task->GetUnitType() == unit_type) {
                        if (create_task->IsActivelyBuilding() ||
                            create_task->ComparePriority(task_priority + TASK_PRIORITY_ADJUST_MAJOR) <= 0) {
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
                              ResourceManager_GetUnit(unit_type).GetPluralName().data());

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

bool TaskManager::AreTasksThinking(uint16_t team) {
    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetTeam() == team && (*it).IsThinking()) {
            AILOG(log, "Task thinking: {}", (*it).WriteStatusLog());

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

            if (Ai_GetReactionState() == AI_REACTION_STATE_IDLE) {
                for (SmartList<UnitInfo>::Iterator it = units_to_check.Begin(); it != units_to_check.End(); ++it) {
                    if (GameManager_IsActiveTurn((*it).team) && (*it).hits > 0 && (*it).speed > 0 &&
                        Task_IsReadyToTakeOrders(&*it) &&
                        UnitsManager_TeamInfo[(*it).team].team_type == TEAM_TYPE_COMPUTER) {
                        if ((*it).GetTask()) {
                            Task_RetreatFromDanger((*it).GetTask(), &*it, Ai_DetermineCautionLevel(&*it));

                        } else {
                            Task_RetreatIfNecessary(nullptr, &*it, Ai_DetermineCautionLevel(&*it));
                        }

                        units_to_check.Remove(*it);

                        if (!TickTimer_HaveTimeToThink()) {
                            Ai_SetReactionState(AI_REACTION_STATE_PROCESSING);

                            return;
                        }
                    }
                }
            }
        }
    }
}

void TaskManager::CollectPotentialAttackTargets(UnitInfo* unit) {
    if (unit->ammo) {
        int32_t range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

        range = range * range;

        if (Access_GetValidAttackTargetTypes(unit->GetUnitType()) & MOBILE_LAND_UNIT) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team != unit->team && (*it).IsVisibleToTeam(unit->team) &&
                    UnitsManager_TeamInfo[(*it).team].team_type == TEAM_TYPE_COMPUTER &&
                    Access_GetSquaredDistance(&*it, unit) <= range) {
                    units_to_check.PushBack(*it);
                }
            }
        }

        if (Access_GetValidAttackTargetTypes(unit->GetUnitType()) & MOBILE_AIR_UNIT) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                 it != UnitsManager_MobileAirUnits.End(); ++it) {
                if ((*it).team != unit->team && (*it).IsVisibleToTeam(unit->team) &&
                    UnitsManager_TeamInfo[(*it).team].team_type == TEAM_TYPE_COMPUTER &&
                    Access_GetSquaredDistance(&*it, unit) <= range) {
                    units_to_check.PushBack(*it);
                }
            }
        }
    }
}

void TaskManager::EnqueueUnitForReactionCheck(UnitInfo& unit) { units_to_check.PushBack(unit); }

void TaskManager::CreateBuilding(ResourceID unit_type, uint16_t team, Point site, Task* task) {
    if (IsUnitNeeded(unit_type, team, task->GetPriority())) {
        AiPlayer_Teams[team].CreateBuilding(unit_type, site, task);
    }
}

void TaskManager::CreateUnit(ResourceID unit_type, uint16_t team, Point site, Task* task) {
    if (IsUnitNeeded(unit_type, team, task->GetPriority())) {
        SmartPointer<TaskCreateUnit> create_unit_task(new (std::nothrow) TaskCreateUnit(unit_type, task, site));

        AppendTask(*create_unit_task);
    }
}

void TaskManager::AppendTask(Task& task) {
    AILOG(log, "Task Manager: append task '{}'.", Task_GetName(&task));

    tasks.PushBack(task);

    if (task.GetType() == TaskType_TaskObtainUnits) {
        unit_requests.PushBack(*dynamic_cast<TaskObtainUnits*>(&task));
    }

    task.Init();
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
            (*it).SetProcessingNeeded(true);
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

void TaskManager::MarkTasksForProcessing(uint16_t team) {
    AILOG(log, "Task Manager: request task processing.");

    for (SmartList<Task>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetTeam() == team) {
            (*it).SetProcessingNeeded(true);
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
    units_to_check.Clear();

    reminder_counter = 0;
}

void TaskManager::ClearUnitTasksAndRemindAvailable(UnitInfo* unit, bool priority) {
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
                uint16_t task_priority{UINT16_MAX};

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

                if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_APPRENTICE) {
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

                if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_EXPERT) {
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
                        if ((!obtain_units_task || (*it).ComparePriority(task_priority) < 0) &&
                            (*it).CountInstancesOfUnitType(unit->GetUnitType())) {
                            obtain_units_task = *it;
                            task_priority = obtain_units_task->GetPriority();
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
                            (*it).Init();
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

    AILOG(log, "Task Manager: remove task '{}'.", Task_GetName(&task));

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
