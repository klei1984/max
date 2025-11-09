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

#include "tasktransport.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "remote.hpp"
#include "task_manager.hpp"
#include "taskdump.hpp"
#include "units_manager.hpp"

TaskTransport::TaskTransport(TaskMove* task_move_, ResourceID transporter)
    : Task(task_move_->GetTeam(), nullptr, task_move_->GetFlags()) {
    transporter_unit_type = transporter;
    task_move = task_move_;
    move_tasks.PushBack(*task_move_);
    task_move_->GetPassenger()->AddDelayedTask(this);
}

TaskTransport::~TaskTransport() {}

void TaskTransport::AddClients(SmartList<TaskMove>* list) {
    if (unit_transporter && unit_transporter->storage) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).GetOrder() == ORDER_IDLE && unit_transporter == (*it).GetParent() && (*it).team == team) {
                if (!(*it).GetTask() || (*it).GetTask()->GetType() != TaskType_TaskMove) {
                    TaskMove* task = new (std::nothrow) TaskMove(&*it, &TaskTransport_MoveFinishedCallback);

                    TaskManager.AppendTask(*task);
                }

                SDL_assert((*it).GetTask()->GetType() == TaskType_TaskMove);
                SDL_assert(unit_transporter->storage >= 0);

                list->PushBack(*dynamic_cast<TaskMove*>((*it).GetTask()));

                if (list->GetCount() == static_cast<uint32_t>(unit_transporter->storage)) {
                    return;
                }
            }
        }
    }
}

bool TaskTransport::WillTransportNewClient(TaskMove* task) {
    bool result;

    if (!unit_transporter || unit_transporter->hits > 0) {
        if (!unit_transporter || (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], transporter_unit_type)
                                      ->GetAttribute(ATTRIB_STORAGE) > unit_transporter->storage)) {
            if (task->GetPassenger()) {
                if (move_tasks.GetCount() != 0 || (unit_transporter && unit_transporter->storage != 0)) {
                    if (unit_transporter) {
                        int32_t distance;
                        Point position(unit_transporter->grid_x, unit_transporter->grid_y);
                        UnitInfo* passenger = task->GetPassenger();

                        distance = TaskManager_GetDistance(unit_transporter->grid_x - passenger->grid_x,
                                                           unit_transporter->grid_y - passenger->grid_y);

                        if (move_tasks.GetCount()) {
                            int32_t distance2;

                            for (SmartList<TaskMove>::Iterator it = move_tasks.Begin(); it != move_tasks.End(); ++it) {
                                if ((*it).GetPassenger()) {
                                    passenger = (*it).GetPassenger();

                                    distance2 = TaskManager_GetDistance(unit_transporter->grid_x - passenger->grid_x,
                                                                        unit_transporter->grid_y - passenger->grid_y);

                                    if (distance2 < distance) {
                                        return false;
                                    }
                                }
                            }
                        }

                        if (unit_transporter->storage > 0) {
                            SmartList<TaskMove> moves;
                            int32_t distance2;

                            AILOG(log, "Task Transport: Will Transport ");

                            AddClients(&moves);

                            for (SmartList<TaskMove>::Iterator it = moves.Begin(); it != moves.End(); ++it) {
                                if ((*it).GetPassenger()) {
                                    distance2 = TaskManager_GetDistance(position, (*it).GetDestination());

                                    if (distance2 < distance) {
                                        AILOG_LOG(log, "FALSE");

                                        return false;
                                    }
                                }
                            }

                            AILOG_LOG(log, "TRUE");

                            result = true;

                        } else {
                            result = true;
                        }

                    } else {
                        if (static_cast<uint32_t>(
                                UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], transporter_unit_type)
                                    ->GetAttribute(ATTRIB_STORAGE)) > move_tasks.GetCount()) {
                            result = true;

                        } else {
                            result = false;
                        }
                    }

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

bool TaskTransport::ChooseNewTask() {
    int32_t distance;
    int32_t minimum_distance{INT32_MAX};
    bool result;

    AILOG(log, "Transport: Choose New Task.");

    if (unit_transporter) {
        Point position(unit_transporter->grid_x, unit_transporter->grid_y);
        Point point;

        task_move = nullptr;

        if (unit_transporter->storage < unit_transporter->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
            TransporterMap map(&*unit_transporter, 1,
                               unit_transporter->GetUnitType() == CLNTRANS ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE
                                                                           : CAUTION_LEVEL_AVOID_ALL_DAMAGE);

            if (move_tasks.GetCount()) {
                for (SmartList<TaskMove>::Iterator it = move_tasks.Begin(); it != move_tasks.End(); ++it) {
                    if ((*it).GetPassenger() && (*it).GetPassenger()->GetOrder() != ORDER_IDLE &&
                        (*it).GetTransporterType() != INVALID_ID) {
                        distance = Access_GetDistance((*it).GetPassenger(), position);

                        if (!task_move || distance < minimum_distance) {
                            minimum_distance = distance;
                            task_move = &*it;
                        }

                    } else {
                        if ((*it).GetPassenger()) {
                            (*it).GetPassenger()->RemoveDelayedTask(this);
                        }

                        move_tasks.Remove(*it);
                    }
                }

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                     it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                    if ((*it).team == team && ((*it).flags & MOBILE_LAND_UNIT) && (*it).GetUnitType() != SURVEYOR) {
                        if ((*it).GetTask() && (*it).GetTask()->GetType() == TaskType_TaskMove &&
                            Task_IsReadyToTakeOrders(&*it)) {
                            TaskMove* move = dynamic_cast<TaskMove*>((*it).GetTask());

                            if ((*it).speed == 0 || move->GetTransporterType() != INVALID_ID) {
                                distance = Access_GetDistance(&*it, position);

                                if (!task_move || distance < minimum_distance) {
                                    if (TaskTransport_Search(&*unit_transporter, &*it, &map)) {
                                        task_move = move;
                                        minimum_distance = distance;
                                        point.x = (*it).grid_x;
                                        point.y = (*it).grid_y;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (unit_transporter->storage > 0) {
            SmartList<TaskMove> moves;

            AddClients(&moves);

            for (SmartList<TaskMove>::Iterator it = moves.Begin(); it != moves.End(); ++it) {
                if ((*it).GetPassenger()) {
                    distance = Access_GetDistance(position, (*it).GetDestination());

                    if (!task_move || distance < minimum_distance) {
                        minimum_distance = distance;
                        task_move = &*it;
                    }

                } else {
                    moves.Remove(*it);
                }
            }
        }

        if (task_move) {
            AILOG_LOG(log, "Transport: moving to drop off {}.",
                      UnitsManager_BaseUnits[task_move->GetPassenger()->GetUnitType()].GetSingularName());

            result = true;

        } else {
            SmartPointer<Task> task(this);

            TaskManager.RemindAvailable(&*unit_transporter);

            unit_transporter = nullptr;

            if (task_obtain_units) {
                task_obtain_units->RemoveSelf();
                task_obtain_units = nullptr;
            }

            AILOG_LOG(log, "Transport Task Removed.");

            TaskManager.RemoveTask(*this);

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskTransport::IsClient(TaskMove* move) {
    bool result{false};

    for (auto it = move_tasks.Begin(), it_end = move_tasks.End(); it != it_end; ++it) {
        if (it->Get() == move) {
            result = true;
            break;
        }
    }

    return result;
}

void TaskTransport::AddClient(TaskMove* move) {
    AILOG(log, "Transport: Add Client {}.",
          UnitsManager_BaseUnits[move->GetPassenger()->GetUnitType()].GetSingularName());

    move_tasks.PushBack(*move);
    move->GetPassenger()->AddDelayedTask(this);

    if (!task_move) {
        task_move = move;
    }

    if (unit_transporter) {
        Task_RemoveMovementTasks(&*unit_transporter);
        Task_RemindMoveFinished(&*unit_transporter, true);

    } else {
        AILOG_LOG(log, "Waiting for transport.");
    }
}

void TaskTransport::RemoveClient(TaskMove* move) {
    if (move->GetPassenger()) {
        if (move->GetPassenger()->GetOrder() != ORDER_IDLE) {
            move->RemoveTransport();
        }

        move->GetPassenger()->RemoveDelayedTask(this);
    }

    move_tasks.Remove(*move);
}

void TaskTransport::MoveFinishedCallback1(Task* task, UnitInfo* unit, char result) {
    if (result == TASKMOVE_RESULT_SUCCESS) {
        Task_RemindMoveFinished(unit, unit->speed > 0);

    } else if (result == TASKMOVE_RESULT_BLOCKED) {
        TaskTransport* transport = dynamic_cast<TaskTransport*>(task);

        transport->RemoveClient(transport->task_move.Get());
        transport->ChooseNewTask();
    }
}

void TaskTransport::MoveFinishedCallback2(Task* task, UnitInfo* unit, char result) {
    if (result == TASKMOVE_RESULT_SUCCESS) {
        Task_RemindMoveFinished(unit, unit->speed > 0);

    } else if (result == TASKMOVE_RESULT_BLOCKED) {
        TaskTransport* transport = dynamic_cast<TaskTransport*>(task);

        if (transport->task_move->GetPassenger()) {
            AILOG(log, "Transport: dump client {}.",
                  UnitsManager_BaseUnits[transport->task_move->GetPassenger()->GetUnitType()].GetSingularName());

            SmartPointer<Task> dump =
                new (std::nothrow) TaskDump(transport, &*transport->task_move, &*transport->unit_transporter);

            TaskManager.AppendTask(*dump);

        } else {
            AILOG(log, "Transport: move task without passenger.");

            transport->RemoveClient(transport->task_move.Get());
            transport->ChooseNewTask();
        }
    }
}

bool TaskTransport::Task_vfunc1(UnitInfo& unit) { return unit_transporter != unit; }

char* TaskTransport::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Transport units: ");

    if (unit_transporter || !task_obtain_units) {
        if (task_move && task_move->GetPassenger()) {
            if (task_move->GetPassenger()->GetOrder() == ORDER_IDLE) {
                if (unit_transporter && unit_transporter->storage > 0) {
                    char unit_name[40];

                    task_move->GetPassenger()->GetDisplayName(unit_name, sizeof(unit_name));

                    sprintf(buffer, "Transport units: drop off %s at [%i,%i]", unit_name,
                            task_move->GetDestination().x + 1, task_move->GetDestination().y + 1);
                }

            } else {
                char unit_name[40];

                task_move->GetPassenger()->GetDisplayName(unit_name, sizeof(unit_name));

                sprintf(buffer, "Transport units: pick up %s at [%i,%i]", unit_name,
                        task_move->GetPassenger()->grid_x + 1, task_move->GetPassenger()->grid_y + 1);
            }

        } else {
            strcat(buffer, " doing nothing. ");
        }

    } else {
        sprintf(buffer, "Transport units: Waiting for %s.",
                UnitsManager_BaseUnits[transporter_unit_type].GetSingularName());
    }

    return buffer;
}

Rect* TaskTransport::GetBounds(Rect* bounds) {
    if (unit_transporter) {
        bounds->ulx = unit_transporter->grid_x;
        bounds->uly = unit_transporter->grid_y;
        bounds->lrx = bounds->ulx + 1;
        bounds->lry = bounds->uly + 1;

    } else if (task_move) {
        bounds->ulx = task_move->GetTransporterWaypoint().x;
        bounds->uly = task_move->GetTransporterWaypoint().y;
        bounds->lrx = bounds->ulx + 1;
        bounds->lry = bounds->uly + 1;
    }

    return bounds;
}

uint8_t TaskTransport::GetType() const { return TaskType_TaskTransport; }

bool TaskTransport::IsNeeded() { return !unit_transporter && move_tasks.GetCount() > 0; }

void TaskTransport::AddUnit(UnitInfo& unit) {
    AILOG(log, "Transport: Add {}.", UnitsManager_BaseUnits[unit.GetUnitType()].GetSingularName());

    if (!unit_transporter && unit.GetUnitType() == transporter_unit_type) {
        unit_transporter = unit;
        unit_transporter->AddTask(this);
        if (ChooseNewTask()) {
            Task_RemindMoveFinished(&unit, true);
        }
    }
}

void TaskTransport::Begin() {
    if (unit_transporter) {
        AILOG(log, "Transport: Begin.");

        unit_transporter->AddTask(this);

    } else {
        task_obtain_units = new (std::nothrow) TaskObtainUnits(this, task_move->GetTransporterWaypoint());

        task_obtain_units->AddUnit(transporter_unit_type);

        TaskManager.AppendTask(*task_obtain_units);
    }
}

void TaskTransport::BeginTurn() { EndTurn(); }

void TaskTransport::ChildComplete(Task* task) {
    AILOG(log, "Transport: Child Complete: ");

    if (task->GetType() == TaskType_TaskMove) {
        TaskMove* move = dynamic_cast<TaskMove*>(task);

        if (move->GetPassenger()) {
            move->GetPassenger()->RemoveDelayedTask(this);
        }

        move_tasks.Remove(*move);

        if (&*task_move == task) {
            if (unit_transporter) {
                AILOG_LOG(log, "client {}",
                          UnitsManager_BaseUnits[task_move->GetPassenger()->GetUnitType()].GetSingularName());

                Task_RemoveMovementTasks(&*unit_transporter);
                Task_RemindMoveFinished(&*unit_transporter, true);
            }

            task_move = nullptr;

        } else {
            AILOG_LOG(log, "secondary client.");
        }
    }

    if (&*task_obtain_units == task) {
        AILOG_LOG(log, "obtainer.");

        task_obtain_units = nullptr;
    }
}

void TaskTransport::EndTurn() {
    AILOG(log, "Transport: End Turn.");

    if (unit_transporter && unit_transporter->IsReadyForOrders(this)) {
        Execute(*unit_transporter);
    }
}

bool TaskTransport::Execute(UnitInfo& unit) {
    bool result;

    if (unit_transporter == &unit && unit.IsReadyForOrders(this)) {
        AILOG(log, "Transport: Move Finished.");

        if (Task_RetreatFromDanger(this, &unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
            result = true;

        } else if (ChooseNewTask()) {
            int32_t distance = (transporter_unit_type == AIRTRANS) ? 0 : 3;

            if (task_move->GetPassenger()->GetOrder() == ORDER_IDLE) {
                if (unit.storage > 0) {
                    const Point destination = task_move->GetDestination();

                    if (destination.x >= 0 && destination.y >= 0) {
                        if (Access_GetDistance(unit_transporter.Get(), destination) <= distance) {
                            UnloadUnit(task_move->GetPassenger());

                        } else {
                            SmartPointer<Task> move = new (std::nothrow) TaskMove(
                                &*unit_transporter, this, distance,
                                unit_transporter->GetUnitType() == CLNTRANS ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE
                                                                            : CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                destination, &MoveFinishedCallback2);

                            TaskManager.AppendTask(*move);
                        }

                        result = true;

                    } else {
                        result = false;
                    }

                } else {
                    result = false;
                }

            } else {
                SmartPointer<UnitInfo> passenger(task_move->GetPassenger());
                Point position = task_move->GetTransporterWaypoint();
                Point line_distance;

                if (Task_IsReadyToTakeOrders(&*passenger)) {
                    if (TaskManager_GetDistance(&*unit_transporter, &*passenger) > distance) {
                        if (unit_transporter->GetUnitType() == AIRTRANS ||
                            unit_transporter->GetUnitType() == CLNTRANS) {
                            position.x = passenger->grid_x;
                            position.y = passenger->grid_y;
                        }

                        line_distance.x = position.x - unit_transporter->grid_x;
                        line_distance.y = position.y - unit_transporter->grid_y;

                        if (line_distance.x * line_distance.x + line_distance.y * line_distance.y > distance) {
                            SmartPointer<Task> move = new (std::nothrow)
                                TaskMove(&*unit_transporter, this, distance, CAUTION_LEVEL_AVOID_ALL_DAMAGE, position,
                                         &MoveFinishedCallback1);

                            TaskManager.AppendTask(*move);

                            result = true;

                        } else {
                            result = false;
                        }

                    } else {
                        LoadUnit(task_move->GetPassenger());

                        result = true;
                    }

                } else {
                    result = false;
                }
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskTransport::RemoveSelf() {
    for (SmartList<TaskMove>::Iterator it = move_tasks.Begin(); it != move_tasks.End(); ++it) {
        if ((*it).GetPassenger()) {
            (*it).GetPassenger()->RemoveDelayedTask(this);
        }
    }

    move_tasks.Clear();

    if (unit_transporter) {
        TaskManager.RemindAvailable(&*unit_transporter);
    }

    if (task_obtain_units) {
        task_obtain_units->RemoveSelf();
    }

    unit_transporter = nullptr;
    task_move = nullptr;
    task_obtain_units = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskTransport::RemoveUnit(UnitInfo& unit) {
    if (unit_transporter == &unit) {
        AILOG(log, "Transport: Remove {}.", UnitsManager_BaseUnits[unit.GetUnitType()].GetSingularName());

        for (SmartList<TaskMove>::Iterator it = move_tasks.Begin(); it != move_tasks.End(); ++it) {
            RemoveClient(&*it);
        }

        move_tasks.Clear();

        TaskManager.RemoveTask(*this);
    }
}

void TaskTransport::EventUnitLoaded(UnitInfo& unit1, UnitInfo& unit2) {
    if (unit_transporter == &unit1) {
        AILOG(log, "Transport: {} Loaded.", UnitsManager_BaseUnits[unit2.GetUnitType()].GetSingularName());

        for (SmartList<TaskMove>::Iterator it = move_tasks.Begin(); it != move_tasks.End(); ++it) {
            if ((*it).GetPassenger() == &unit2) {
                unit2.RemoveDelayedTask(this);
                move_tasks.Remove(*it);
                break;
            }
        }

        Task_RemindMoveFinished(&unit1, true);
    }
}

void TaskTransport::EventUnitUnloaded(UnitInfo& unit1, UnitInfo& unit2) {
    AILOG(log, "Transport: {} Unloaded.", UnitsManager_BaseUnits[unit2.GetUnitType()].GetSingularName());

    if (unit2.GetTask() && unit2.GetTask()->GetType() == TaskType_TaskMove) {
        dynamic_cast<TaskMove*>(unit2.GetTask())->RemoveTransport();
    }

    Task_RemindMoveFinished(&unit2, true);
    Task_RemindMoveFinished(&unit1);
}

void TaskTransport::EventZoneCleared(Zone* zone, bool status) {
    if (status && unit_transporter && unit_transporter->GetOrder() == ORDER_AWAIT) {
        Task_RemindMoveFinished(&*unit_transporter);
    }
}

bool TaskTransport_Search(UnitInfo* transporter, UnitInfo* client, TransporterMap* map) {
    SmartPointer<TaskMove> move(dynamic_cast<TaskMove*>(client->GetTask()));
    Point destination = move->GetDestination();
    Point position(client->grid_x, client->grid_y);
    bool result;

    if (destination.x >= 0) {
        if (transporter->GetUnitType() != CLNTRANS ||
            (client->GetUnitType() == INFANTRY || client->GetUnitType() == COMMANDO)) {
            if (position.x != destination.x || position.y != destination.y) {
                if (map->Search(position)) {
                    if (transporter->GetUnitType() == AIRTRANS) {
                        result = map->Search(destination);

                    } else {
                        Rect bounds;
                        Point site;

                        rect_init(&bounds, std::max(0, destination.x - 1), std::max(0, destination.y - 1),
                                  std::min(static_cast<int32_t>(ResourceManager_MapSize.x), destination.x + 2),
                                  std::min(static_cast<int32_t>(ResourceManager_MapSize.y), destination.y + 2));

                        for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                            for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                                if (map->Search(site)) {
                                    return true;
                                }
                            }
                        }

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

    } else {
        result = false;
    }

    return result;
}

void TaskTransport_MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    if (result == TASKMOVE_RESULT_SUCCESS) {
        Task_RemindMoveFinished(unit, unit->speed > 0 ? true : false);
    }
}

void TaskTransport_FinishTransport(Task* const task, UnitInfo* const transporter, UnitInfo* const client, Point site) {
    if (GameManager_IsActiveTurn(task->GetTeam())) {
        AILOG(log, "TaskAssistMove: Unload {}.", UnitsManager_BaseUnits[client->GetUnitType()].GetSingularName());

        auto unit_in_the_way = Access_GetTeamUnit(site.x, site.y, task->GetTeam(), MOBILE_SEA_UNIT | MOBILE_LAND_UNIT);

        if (unit_in_the_way) {
            if (Task_IsReadyToTakeOrders(unit_in_the_way)) {
                SmartPointer<Zone> zone = new (std::nothrow) Zone(client, task);

                AILOG_LOG(log, "Must clear landing zone first.");

                zone->Add(&site);

                AiPlayer_Teams[task->GetTeam()].ClearZone(zone.Get());
            }

        } else if (Access_IsAccessible(client->GetUnitType(), task->GetTeam(), site.x, site.y,
                                       AccessModifier_SameClassBlocks)) {
            transporter->target_grid_x = site.x;
            transporter->target_grid_y = site.y;

            transporter->SetParent(client);

            if (transporter->GetUnitType() == AIRTRANS) {
                UnitsManager_SetNewOrder(transporter, ORDER_UNLOAD, ORDER_STATE_INIT);

            } else {
                UnitsManager_SetNewOrder(transporter, ORDER_ACTIVATE, ORDER_STATE_EXECUTING_ORDER);
            }

        } else {
            SDL_assert(client->GetTask()->GetType() == TaskType_TaskMove);

            dynamic_cast<TaskMove*>(client->GetTask())->SetDestination(Point(-1, -1));
        }
    }
}

bool TaskTransport::LoadUnit(UnitInfo* unit) {
    bool result;

    if (unit_transporter && unit_transporter->IsReadyForOrders(this) && GameManager_IsActiveTurn(team)) {
        AILOG(log, "Transport: Load {} on {}.", UnitsManager_BaseUnits[unit->GetUnitType()].GetSingularName(),
              UnitsManager_BaseUnits[unit_transporter->GetUnitType()].GetSingularName());

        if (Task_IsReadyToTakeOrders(unit) && GameManager_IsActiveTurn(team)) {
            int32_t distance = TaskManager_GetDistance(unit, &*unit_transporter);

            if (unit_transporter->GetUnitType() == AIRTRANS && distance > 0) {
                AILOG_LOG(log, "Too far away.");

                result = false;

            } else if ((unit_transporter->GetUnitType() == SEATRANS || unit_transporter->GetUnitType() == CLNTRANS) &&
                       distance > 3) {
                AILOG_LOG(log, "Too far away.");

                result = false;

            } else if (unit_transporter->GetUnitType() == AIRTRANS) {
                unit_transporter->SetParent(unit);

                SDL_assert(GameManager_IsActiveTurn(team));

                UnitsManager_SetNewOrder(&*unit_transporter, ORDER_LOAD, ORDER_STATE_INIT);

                result = true;

            } else if (unit->speed) {
                unit->target_grid_x = unit_transporter->grid_x;
                unit->target_grid_y = unit_transporter->grid_y;

                SDL_assert(GameManager_IsActiveTurn(team));

                SmartPointer<GroundPath> path =
                    new (std::nothrow) GroundPath(unit_transporter->grid_x, unit_transporter->grid_y);

                path->AddStep(unit_transporter->grid_x - unit->grid_x, unit_transporter->grid_y - unit->grid_y);

                unit->path = &*path;

                unit->Redraw();

                UnitsManager_SetNewOrder(unit, ORDER_MOVE_TO_UNIT, ORDER_STATE_IN_PROGRESS);

                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_38(unit);
                }

                result = true;

            } else {
                result = false;
            }

        } else {
            AILOG_LOG(log, "Unit not ready for orders.");

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskTransport::UnloadUnit(UnitInfo* client) {
    TaskTransport_FinishTransport(this, unit_transporter.Get(), client, task_move->GetDestination());
}

ResourceID TaskTransport::GetTransporterType() const { return transporter_unit_type; }
