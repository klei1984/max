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

TaskAssistMove::TaskAssistMove(uint16_t team_) : Task(team_, nullptr, TASK_PRIORITY_ASSIST_MOVE) {}

TaskAssistMove::~TaskAssistMove() {}

void TaskAssistMove::RequestTransport(UnitInfo* transporter, UnitInfo* client) {
    if (GameManager_IsActiveTurn(team)) {
        if (transporter->GetUnitType() == AIRTRANS) {
            transporter->SetParent(client);
            UnitsManager_SetNewOrder(transporter, ORDER_LOAD, ORDER_STATE_INIT);

        } else {
            client->target_grid_x = transporter->grid_x;
            client->target_grid_y = transporter->grid_y;

            if (client->speed < 2) {
                client->speed = 2;
            }

            SmartPointer<GroundPath> path = new (std::nothrow) GroundPath(transporter->grid_x, transporter->grid_y);

            path->AddStep(transporter->grid_x - client->grid_x, transporter->grid_y - client->grid_y);

            client->path = &*path;
            client->Redraw();

            UnitsManager_SetNewOrder(client, ORDER_MOVE_TO_UNIT, ORDER_STATE_IN_PROGRESS);

            if (Remote_IsNetworkGame) {
                Remote_SendNetPacket_38(client);
            }
        }
    }
}

void TaskAssistMove::CompleteTransport(UnitInfo* transporter, UnitInfo* client, Point site) {
    TaskTransport_FinishTransport(this, transporter, client, site);
}

bool TaskAssistMove::IsUnitTransferable(UnitInfo& unit) { return unit.storage == 0; }

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
        transporters.PushBack(unit);
        unit.AddTask(this);
        Task_RemindMoveFinished(&unit);
    }
}

void TaskAssistMove::BeginTurn() {
    if (Builder_IsBuildable(AIRTRANS)) {
        int32_t unit_count_transport{0};
        int32_t unit_count_client{0};
        bool is_found{false};

        for (auto& transporter : transporters) {
            if (transporter.GetUnitType() == AIRTRANS && transporter.storage == 0) {
                is_found = true;
                break;
            }
        }

        if (!is_found) {
            for (auto& unit : UnitsManager_MobileLandSeaUnits) {
                if (unit.team == team && (unit.flags & MOBILE_LAND_UNIT)) {
                    ++unit_count_client;
                }
            }

            for (auto& unit : UnitsManager_MobileAirUnits) {
                if (unit.team == team && unit.GetUnitType() == AIRTRANS) {
                    ++unit_count_transport;
                }
            }

            for (auto it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End(); ++it) {
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
        int32_t unit_count_transport{0};
        int32_t unit_count_client{0};
        bool is_found{false};

        for (auto& transporter : transporters) {
            if (transporter.GetUnitType() == CLNTRANS && transporter.storage == 0) {
                is_found = true;
                break;
            }
        }

        if (!is_found) {
            for (auto& unit : UnitsManager_MobileLandSeaUnits) {
                if (unit.team == team) {
                    if (unit.GetUnitType() == INFANTRY || unit.GetUnitType() == COMMANDO) {
                        ++unit_count_client;

                    } else if (unit.GetUnitType() == CLNTRANS) {
                        ++unit_count_transport;
                    }
                }
            }

            for (auto it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End(); ++it) {
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
    for (auto& transporter : transporters) {
        if (Execute(transporter)) {
            return;
        }
    }
}

bool TaskAssistMove::Execute(UnitInfo& transporter) {
    UnitInfo* client_unit{nullptr};
    int32_t minimum_distance{INT32_MAX};
    int32_t distance;
    Point client_position;
    Point client_destination;
    bool result;

    if (transporter.IsReadyForOrders(this)) {
        if (transporter.storage < transporter.GetBaseValues()->GetAttribute(ATTRIB_STORAGE) && transporter.speed > 0) {
            TransporterMap map(&transporter, 1,
                               (transporter.GetUnitType() == CLNTRANS) ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE
                                                                       : CAUTION_LEVEL_AVOID_ALL_DAMAGE);

            for (auto it = UnitsManager_MobileLandSeaUnits.Begin(); it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team && ((*it).flags & MOBILE_LAND_UNIT) && (*it).GetUnitType() != SURVEYOR) {
                    if ((*it).GetTask() && (*it).GetTask()->GetType() == TaskType_TaskMove &&
                        Task_IsReadyToTakeOrders(it->Get())) {
                        if (dynamic_cast<TaskMove*>((*it).GetTask())->IsReadyForTransport() || (*it).speed == 0) {
                            distance = Access_GetSquaredDistance(it->Get(), &transporter);

                            if (client_unit == nullptr || distance < minimum_distance) {
                                if (TaskTransport_Search(&transporter, it->Get(), &map)) {
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

            if (client_unit && Access_GetSquaredDistance(&transporter, client_position) <=
                                   ((transporter.GetUnitType() == AIRTRANS) ? 0 : 3)) {
                RequestTransport(&transporter, client_unit);

                result = true;

                return result;
            }
        }

        if (transporter.storage > 0) {
            for (auto it = UnitsManager_MobileLandSeaUnits.Begin(); it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).GetOrder() == ORDER_IDLE && (*it).GetParent() == &transporter && (*it).team == team) {
                    if (!(*it).GetTask() || (*it).GetTask()->GetType() != TaskType_TaskMove) {
                        TaskMove* task_move =
                            new (std::nothrow) TaskMove(it->Get(), &TaskTransport_MoveFinishedCallback);

                        TaskManager.AppendTask(*task_move);
                    }

                    SDL_assert((*it).GetTask()->GetType() == TaskType_TaskMove);

                    client_destination = dynamic_cast<TaskMove*>((*it).GetTask())->GetDestination();

                    distance = Access_GetSquaredDistance(&transporter, client_destination);

                    if (client_destination.x >= 0 && (client_unit == nullptr || distance < minimum_distance)) {
                        client_unit = it->Get();
                        minimum_distance = distance;
                        client_position = client_destination;
                    }
                }
            }
        }

        if (client_unit) {
            distance = (transporter.GetUnitType() == AIRTRANS) ? 0 : 3;

            if (Access_GetSquaredDistance(&transporter, client_position) <= distance) {
                CompleteTransport(&transporter, client_unit, client_position);

            } else {
                SmartPointer<TaskMove> task_move = new (std::nothrow)
                    TaskMove(&transporter, this, distance,
                             transporter.GetUnitType() == CLNTRANS ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE
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
    for (auto it = transporters.Begin(); it != transporters.End(); ++it) {
        (*it).RemoveTask(this);
    }

    transporters.Clear();

    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskAssistMove::RemoveUnit(UnitInfo& unit) { transporters.Remove(unit); }

void TaskAssistMove::EventUnitLoaded(UnitInfo& unit1, UnitInfo& unit2) { Task_RemindMoveFinished(&unit1); }

void TaskAssistMove::EventUnitUnloaded(UnitInfo& unit1, UnitInfo& unit2) {
    if (unit2.GetTask() && unit2.GetTask()->GetType() == TaskType_TaskMove) {
        dynamic_cast<TaskMove*>(unit2.GetTask())->RemoveTransport();
    }

    Task_RemindMoveFinished(&unit1);

    unit2.ScheduleDelayedTasks(true);
}
