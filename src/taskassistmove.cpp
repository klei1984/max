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

#include "taskassistmove.hpp"

#include "access.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "game_manager.hpp"
#include "remote.hpp"
#include "task_manager.hpp"
#include "taskcreateunit.hpp"
#include "taskmove.hpp"
#include "tasktransport.hpp"
#include "units_manager.hpp"

TaskAssistMove::TaskAssistMove(uint16_t team_) : Task(team_, nullptr, 0x2800) {}

TaskAssistMove::~TaskAssistMove() {}

void TaskAssistMove::RequestTransport(UnitInfo* unit1, UnitInfo* unit2) {
    if (GameManager_IsActiveTurn(team)) {
        if (unit1->GetUnitType() == AIRTRANS) {
            unit1->SetParent(unit2);
            UnitsManager_SetNewOrder(unit1, ORDER_LOAD, ORDER_STATE_INIT);

        } else {
            unit2->target_grid_x = unit1->grid_x;
            unit2->target_grid_y = unit1->grid_y;

            if (unit2->speed < 2) {
                unit2->speed = 2;
            }

            SmartPointer<GroundPath> path = new (std::nothrow) GroundPath(unit1->grid_x, unit1->grid_y);

            path->AddStep(unit1->grid_x - unit2->grid_x, unit1->grid_y - unit2->grid_y);

            unit2->path = &*path;
            unit2->Redraw();

            UnitsManager_SetNewOrder(unit2, ORDER_MOVE_TO_UNIT, ORDER_STATE_IN_PROGRESS);

            if (Remote_IsNetworkGame) {
                Remote_SendNetPacket_38(unit2);
            }
        }
    }
}

void TaskAssistMove::CompleteTransport(UnitInfo* unit1, UnitInfo* unit2, Point site) {
    if (GameManager_IsActiveTurn(team)) {
        if (Access_GetTeamUnit(site.x, site.y, team, MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
            SmartPointer<Zone> zone = new (std::nothrow) Zone(unit2, this);

            zone->Add(&site);

            AiPlayer_Teams[team].ClearZone(&*zone);

        } else {
            unit1->target_grid_x = site.x;
            unit1->target_grid_y = site.y;

            unit1->SetParent(unit2);

            if (unit1->GetUnitType() == AIRTRANS) {
                UnitsManager_SetNewOrder(unit1, ORDER_UNLOAD, ORDER_STATE_INIT);

            } else {
                UnitsManager_SetNewOrder(unit1, ORDER_ACTIVATE, ORDER_STATE_EXECUTING_ORDER);
            }
        }
    }
}

bool TaskAssistMove::Task_vfunc1(UnitInfo& unit) { return unit.storage == 0; }

bool TaskAssistMove::IsUnitUsable(UnitInfo& unit) {
    return unit.GetUnitType() == AIRTRANS || unit.GetUnitType() == SEATRANS || unit.GetUnitType() == CLNTRANS;
}

char* TaskAssistMove::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Use transports to speed movement");

    return buffer;
}

uint8_t TaskAssistMove::GetType() const { return TaskType_TaskAssistMove; }

void TaskAssistMove::AddUnit(UnitInfo& unit) {
    SDL_assert(unit.team == team);

    if (IsUnitUsable(unit)) {
        units.PushBack(unit);
        unit.AddTask(this);
        Task_RemindMoveFinished(&unit);
    }
}

void TaskAssistMove::BeginTurn() {
    SmartPointer<UnitInfo> local_unit;
    int32_t unit_count_transport = 0;
    int32_t unit_count_client = 0;
    bool is_found;

    if (Builder_IsBuildable(AIRTRANS)) {
        is_found = false;

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if ((*it).GetUnitType() == AIRTRANS && (*it).storage == 0) {
                is_found = true;
            }
        }

        if (!is_found) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team && ((*it).flags & MOBILE_LAND_UNIT)) {
                    ++unit_count_client;
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                 it != UnitsManager_MobileAirUnits.End(); ++it) {
                if ((*it).team == team && (*it).GetUnitType() == AIRTRANS) {
                    ++unit_count_transport;
                }
            }

            for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin();
                 it != TaskManager.GetTaskList().End(); ++it) {
                if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskCreateUnit &&
                    dynamic_cast<TaskCreate*>(it->Get())->GetUnitType() == AIRTRANS) {
                    ++unit_count_transport;
                }
            }

            if (unit_count_transport < (unit_count_client + 10) / 20) {
                TaskCreateUnit* create_unit_task = new (std::nothrow)
                    TaskCreateUnit(AIRTRANS, this, Point(ResourceManager_MapSize.x / 2, ResourceManager_MapSize.y / 2));

                TaskManager.AppendTask(*create_unit_task);
            }
        }
    }

    if (Builder_IsBuildable(CLNTRANS)) {
        unit_count_client = 0;
        unit_count_transport = 0;
        is_found = false;

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if ((*it).GetUnitType() == CLNTRANS && (*it).storage == 0) {
                is_found = true;
            }
        }

        if (!is_found) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team) {
                    if ((*it).GetUnitType() == INFANTRY || (*it).GetUnitType() == COMMANDO) {
                        ++unit_count_client;

                    } else if ((*it).GetUnitType() == CLNTRANS) {
                        ++unit_count_transport;
                    }
                }
            }

            for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin();
                 it != TaskManager.GetTaskList().End(); ++it) {
                if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskCreateUnit &&
                    dynamic_cast<TaskCreate*>(it->Get())->GetUnitType() == CLNTRANS) {
                    ++unit_count_transport;
                }
            }

            if (unit_count_transport < (unit_count_client + 8) / 10) {
                TaskCreateUnit* create_unit_task = new (std::nothrow)
                    TaskCreateUnit(CLNTRANS, this, Point(ResourceManager_MapSize.x / 2, ResourceManager_MapSize.y / 2));

                TaskManager.AppendTask(*create_unit_task);
            }
        }
    }
}

void TaskAssistMove::EndTurn() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (Execute((*it))) {
            return;
        }
    }
}

bool TaskAssistMove::Execute(UnitInfo& unit) {
    UnitInfo* client_unit = nullptr;
    Point client_position;
    Point client_destination;
    int32_t distance;
    int32_t minimum_distance{INT32_MAX};
    bool result;

    if (unit.IsReadyForOrders(this)) {
        if (unit.storage < unit.GetBaseValues()->GetAttribute(ATTRIB_STORAGE) && unit.speed > 0) {
            TransporterMap map(
                &unit, 1,
                unit.GetUnitType() == CLNTRANS ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE : CAUTION_LEVEL_AVOID_ALL_DAMAGE);

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team && ((*it).flags & MOBILE_LAND_UNIT) && (*it).GetUnitType() != SURVEYOR) {
                    if ((*it).GetTask() && (*it).GetTask()->GetType() == TaskType_TaskMove &&
                        Task_IsReadyToTakeOrders(it->Get())) {
                        if (dynamic_cast<TaskMove*>((*it).GetTask())->IsReadyForTransport() || (*it).speed == 0) {
                            distance = Access_GetDistance(it->Get(), &unit);

                            if (client_unit == nullptr || distance < minimum_distance) {
                                if (TaskTransport_Search(&unit, it->Get(), &map)) {
                                    client_unit = it->Get();
                                    minimum_distance = distance;
                                    client_position.x = (*it).grid_x;
                                    client_position.y = (*it).grid_y;
                                }
                            }
                        }
                    }
                }
            }

            if (client_unit &&
                Access_GetDistance(&unit, client_position) <= ((unit.GetUnitType() == AIRTRANS) ? 0 : 3)) {
                RequestTransport(&unit, client_unit);

                result = true;

                return result;
            }
        }

        if (unit.storage > 0) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).GetOrder() == ORDER_IDLE && (*it).GetParent() == &unit && (*it).team == team) {
                    if (!(*it).GetTask() || (*it).GetTask()->GetType() != TaskType_TaskMove) {
                        TaskMove* task_move =
                            new (std::nothrow) TaskMove(it->Get(), &TaskTransport_MoveFinishedCallback);

                        TaskManager.AppendTask(*task_move);
                    }

                    SDL_assert((*it).GetTask()->GetType() == TaskType_TaskMove);

                    client_destination = dynamic_cast<TaskMove*>((*it).GetTask())->GetDestination();

                    distance = Access_GetDistance(&unit, client_destination);

                    if (client_destination.x >= 0 && (client_unit == nullptr || distance < minimum_distance)) {
                        client_unit = it->Get();
                        minimum_distance = distance;
                        client_position = client_destination;
                    }
                }
            }
        }

        if (client_unit) {
            distance = (unit.GetUnitType() == AIRTRANS) ? 0 : 3;

            if (Access_GetDistance(&unit, client_position) <= distance) {
                CompleteTransport(&unit, client_unit, client_position);

            } else {
                SmartPointer<TaskMove> task_move =
                    new (std::nothrow) TaskMove(&unit, this, distance,
                                                unit.GetUnitType() == CLNTRANS ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE
                                                                               : CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                                client_position, &TaskTransport_MoveFinishedCallback);

                TaskManager.AppendTask(*task_move);
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

void TaskAssistMove::RemoveSelf() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        (*it).RemoveTask(this);
    }

    units.Clear();

    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskAssistMove::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }

void TaskAssistMove::EventUnitLoaded(UnitInfo& unit1, UnitInfo& unit2) { Task_RemindMoveFinished(&unit1); }

void TaskAssistMove::EventUnitUnloaded(UnitInfo& unit1, UnitInfo& unit2) {
    if (unit2.GetTask() && unit2.GetTask()->GetType() == TaskType_TaskMove) {
        dynamic_cast<TaskMove*>(unit2.GetTask())->RemoveTransport();
    }

    Task_RemindMoveFinished(&unit1);

    unit2.ScheduleDelayedTasks(true);
}
