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
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "remote.hpp"
#include "task_manager.hpp"
#include "taskdump.hpp"
#include "units_manager.hpp"

TaskTransport::TaskTransport(TaskMove* task_move_, ResourceID transporter)
    : Task(task_move->GetTeam(), nullptr, task_move->GetFlags()) {
    transporter_unit_type = transporter;
    task_move = task_move_;
    move_tasks.PushBack(*task_move_);
    task_move_->GetPassenger()->PushBackTask2List(this);
}

TaskTransport::~TaskTransport() {}

void TaskTransport::AddClients(SmartList<TaskMove>* list) {
    if (unit_transporter && unit_transporter->storage) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).orders == ORDER_IDLE && unit_transporter == (*it).GetParent() && (*it).team == team) {
                if (!(*it).GetTask1ListFront() || (*it).GetTask1ListFront()->GetType() != TaskType_TaskMove) {
                    TaskMove* task = new (std::nothrow) TaskMove(&*it, &TaskTransport_MoveFinishedCallback);

                    TaskManager.AddTask(*task);
                }

                SDL_assert((*it).GetTask1ListFront()->GetType() == TaskType_TaskMove);

                list->PushBack(*dynamic_cast<TaskMove*>((*it).GetTask1ListFront()));

                if (list->GetCount() == unit_transporter->storage) {
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
                        int distance;
                        Point position(unit_transporter->grid_x, unit_transporter->grid_y);
                        UnitInfo* passenger = task->GetPassenger();

                        distance = TaskManager_GetDistance(unit_transporter->grid_x - passenger->grid_x,
                                                           unit_transporter->grid_y - passenger->grid_y);

                        if (move_tasks.GetCount()) {
                            int distance2;

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
                            int distance2;

                            AddClients(&moves);

                            for (SmartList<TaskMove>::Iterator it = moves.Begin(); it != moves.End(); ++it) {
                                if ((*it).GetPassenger()) {
                                    distance2 = TaskManager_GetDistance(position, (*it).GetDestination());

                                    if (distance2 < distance) {
                                        return false;
                                    }
                                }
                            }

                            result = true;

                        } else {
                            result = true;
                        }

                    } else {
                        if ((UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], transporter_unit_type)
                                 ->GetAttribute(ATTRIB_STORAGE) > move_tasks.GetCount())) {
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
    int distance;
    int minimum_distance;
    bool result;

    if (unit_transporter) {
        Point position(unit_transporter->grid_x, unit_transporter->grid_y);
        Point point;

        task_move = nullptr;

        if (unit_transporter->storage < unit_transporter->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
            TransporterMap map(&*unit_transporter, 1,
                               unit_transporter->unit_type == CLNTRANS ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE
                                                                       : CAUTION_LEVEL_AVOID_ALL_DAMAGE);

            if (move_tasks.GetCount()) {
                for (SmartList<TaskMove>::Iterator it = move_tasks.Begin(); it != move_tasks.End(); ++it) {
                    if ((*it).GetPassenger() && (*it).GetPassenger()->orders != ORDER_IDLE &&
                        (*it).GetTransporterType() != INVALID_ID) {
                        distance = Access_GetDistance((*it).GetPassenger(), position);

                        if (!task_move || distance < minimum_distance) {
                            minimum_distance = distance;
                            task_move = &*it;
                        }

                    } else {
                        if ((*it).GetPassenger()) {
                            (*it).GetPassenger()->RemoveFromTask2List(this);
                        }

                        move_tasks.Remove(*it);
                    }
                }

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                     it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                    if ((*it).team == team && ((*it).flags & MOBILE_LAND_UNIT) && (*it).unit_type != SURVEYOR) {
                        if ((*it).GetTask1ListFront() && (*it).GetTask1ListFront()->GetType() == TaskType_TaskMove &&
                            Task_IsReadyToTakeOrders(&*it)) {
                            TaskMove* move = dynamic_cast<TaskMove*>((*it).GetTask1ListFront());

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
            result = true;

        } else {
            SmartPointer<Task> task(this);

            TaskManager.RemindAvailable(&*unit_transporter);

            unit_transporter = nullptr;

            if (task_obtain_units) {
                task_obtain_units->RemoveSelf();
                task_obtain_units = nullptr;
            }

            TaskManager.RemoveTask(*this);

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskTransport::AddMove(TaskMove* move) {
    move_tasks.PushBack(*move);
    move->GetPassenger()->PushBackTask2List(this);

    if (!task_move) {
        task_move = move;
    }

    if (unit_transporter) {
        Task_RemoveMovementTasks(&*unit_transporter);
        Task_RemindMoveFinished(&*unit_transporter, true);
    }
}

void TaskTransport::RemoveMove(TaskMove* move) {
    if (move->GetPassenger()) {
        if (move->GetPassenger()->orders != ORDER_IDLE) {
            move->TaskMove_sub_4C66B(false);
        }

        move->GetPassenger()->RemoveFromTask2List(this);
    }

    move_tasks.Remove(*move);
}

void TaskTransport::MoveFinishedCallback1(Task* task, UnitInfo* unit, char result) {
    if (result == 0) {
        Task_RemindMoveFinished(unit, unit->speed > 0);

    } else if (result == 2) {
        TaskTransport* transport = dynamic_cast<TaskTransport*>(task);

        transport->RemoveMove(&*transport->task_move);
        transport->ChooseNewTask();
    }
}

void TaskTransport::MoveFinishedCallback2(Task* task, UnitInfo* unit, char result) {
    if (result == 0) {
        Task_RemindMoveFinished(unit, unit->speed > 0);

    } else if (result == 2) {
        TaskTransport* transport = dynamic_cast<TaskTransport*>(task);

        SmartPointer<Task> dump =
            new (std::nothrow) TaskDump(transport, &*transport->task_move, &*transport->unit_transporter);

        TaskManager.AddTask(*dump);
    }
}

bool TaskTransport::Task_vfunc1(UnitInfo& unit) { return unit_transporter != unit; }

int TaskTransport::GetMemoryUse() const { return move_tasks.GetMemorySize() - 6; }

char* TaskTransport::WriteStatusLog(char* buffer) const {
    if (unit_transporter || !task_obtain_units) {
        if (task_move && task_move->GetPassenger()) {
            if (task_move->GetPassenger()->orders == ORDER_IDLE) {
                if (unit_transporter && unit_transporter->storage > 0) {
                    char unit_name[40];

                    task_move->GetPassenger()->GetDisplayName(unit_name);

                    sprintf(buffer, "Transport units: drop off %s at [%i,%i]", unit_name,
                            task_move->GetDestination().x + 1, task_move->GetDestination().y + 1);
                }

            } else {
                char unit_name[40];

                task_move->GetPassenger()->GetDisplayName(unit_name);

                sprintf(buffer, "Transport units: pick up %s at [%i,%i]", unit_name,
                        task_move->GetPassenger()->grid_x + 1, task_move->GetPassenger()->grid_y + 1);
            }

        } else {
            strcat(buffer, " doing nothing. ");
        }

    } else {
        sprintf(buffer, "Transport units: Waiting for %s.",
                UnitsManager_BaseUnits[transporter_unit_type].singular_name);
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
        bounds->ulx = task_move->GetPoint2().x;
        bounds->uly = task_move->GetPoint2().y;
        bounds->lrx = bounds->ulx + 1;
        bounds->lry = bounds->uly + 1;
    }

    return bounds;
}

unsigned char TaskTransport::GetType() const { return TaskType_TaskTransport; }

bool TaskTransport::Task_vfunc9() { return !unit_transporter && move_tasks.GetCount() > 0; }

void TaskTransport::Task_vfunc11(UnitInfo& unit) {
    if (!unit_transporter && unit.unit_type == transporter_unit_type) {
        unit_transporter = unit;
        unit_transporter->PushFrontTask1List(this);
        if (ChooseNewTask()) {
            Task_RemindMoveFinished(&unit, true);
        }
    }
}

void TaskTransport::AddReminder() {
    if (unit_transporter) {
        unit_transporter->PushFrontTask1List(this);

    } else {
        task_obtain_units = new (std::nothrow) TaskObtainUnits(this, task_move->GetPoint2());

        task_obtain_units->AddUnit(transporter_unit_type);

        TaskManager.AddTask(*task_obtain_units);
    }
}

void TaskTransport::BeginTurn() { EndTurn(); }

void TaskTransport::ChildComplete(Task* task) {
    if (task->GetType() == TaskType_TaskMove) {
        TaskMove* move = dynamic_cast<TaskMove*>(task);

        if (move->GetPassenger()) {
            move->GetPassenger()->RemoveFromTask2List(this);
        }

        move_tasks.Remove(*move);

        if (&*task_move == task) {
            if (unit_transporter) {
                Task_RemoveMovementTasks(&*unit_transporter);
                Task_RemindMoveFinished(&*unit_transporter, true);
            }

            task_move = nullptr;
        }
    }

    if (&*task_obtain_units == task) {
        task_obtain_units = nullptr;
    }
}

void TaskTransport::EndTurn() {
    if (unit_transporter && unit_transporter->IsReadyForOrders(this)) {
        Task_vfunc17(*unit_transporter);
    }
}

bool TaskTransport::Task_vfunc17(UnitInfo& unit) {
    bool result;

    if (unit_transporter == &unit && unit.IsReadyForOrders(this)) {
        if (Task_RetreatFromDanger(this, &unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
            result = true;

        } else if (ChooseNewTask()) {
            int distance;

            if (transporter_unit_type == AIRTRANS) {
                distance = 0;

            } else {
                distance = 3;
            }

            if (task_move->GetPassenger()->orders == ORDER_IDLE) {
                if (unit.storage > 0) {
                    Point destination = task_move->GetDestination();

                    if (destination.x >= 0) {
                        if (TaskManager_GetDistance(Point(unit_transporter->grid_x, unit_transporter->grid_y),
                                                    task_move->GetDestination()) > distance) {
                            SmartPointer<Task> move = new (std::nothrow)
                                TaskMove(&*unit_transporter, this, distance, CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                         task_move->GetDestination(), &MoveFinishedCallback2);

                            TaskManager.AddTask(*move);

                        } else {
                            UnloadUnit(task_move->GetPassenger());
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
                Point position = task_move->GetPoint2();
                Point line_distance;

                if (Task_IsReadyToTakeOrders(&*passenger)) {
                    if (TaskManager_GetDistance(&*unit_transporter, &*passenger) > distance) {
                        if (unit_transporter->unit_type == AIRTRANS || unit_transporter->unit_type == CLNTRANS) {
                            position.x = passenger->grid_x;
                            position.y = passenger->grid_y;

                            line_distance.x = position.x - unit_transporter->grid_x;
                            line_distance.y = position.y - unit_transporter->grid_y;

                            if (line_distance.x * line_distance.x + line_distance.y * line_distance.y > distance) {
                                SmartPointer<Task> move = new (std::nothrow)
                                    TaskMove(&*unit_transporter, this, distance, CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                             position, &MoveFinishedCallback1);

                                TaskManager.AddTask(*move);

                                result = true;

                            } else {
                                result = false;
                            }
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
            (*it).GetPassenger()->RemoveFromTask2List(this);
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
        for (SmartList<TaskMove>::Iterator it = move_tasks.Begin(); it != move_tasks.End(); ++it) {
            RemoveMove(&*it);
        }

        move_tasks.Clear();

        TaskManager.RemoveTask(*this);
    }
}

void TaskTransport::Task_vfunc24(UnitInfo& unit1, UnitInfo& unit2) {
    if (unit_transporter == &unit1) {
        for (SmartList<TaskMove>::Iterator it = move_tasks.Begin(); it != move_tasks.End(); ++it) {
            if ((*it).GetPassenger() == &unit2) {
                unit2.RemoveFromTask2List(this);
                move_tasks.Remove(*it);
                break;
            }
        }

        Task_RemindMoveFinished(&unit1, true);
    }
}

void TaskTransport::Task_vfunc26(UnitInfo& unit1, UnitInfo& unit2) {
    if (unit2.GetTask1ListFront() && unit2.GetTask1ListFront()->GetType() == TaskType_TaskMove) {
        dynamic_cast<TaskMove*>(unit2.GetTask1ListFront())->TaskMove_sub_4C66B(true);
    }

    Task_RemindMoveFinished(&unit2, true);
    Task_RemindMoveFinished(&unit1);
}

void TaskTransport::Task_vfunc27(Zone* zone, char mode) {
    if (mode && unit_transporter && unit_transporter->orders == ORDER_AWAIT) {
        Task_RemindMoveFinished(&*unit_transporter);
    }
}

bool TaskTransport_Search(UnitInfo* unit1, UnitInfo* unit2, TransporterMap* map) {
    SmartPointer<TaskMove> move(dynamic_cast<TaskMove*>(unit2->GetTask1ListFront()));
    Point destination = move->GetDestination();
    Point position(unit2->grid_x, unit2->grid_y);
    bool result;

    if (destination.x >= 0) {
        if (unit1->unit_type != CLNTRANS || (unit2->unit_type == INFANTRY || unit2->unit_type == COMMANDO)) {
            if (position.x != destination.x || position.y != destination.y) {
                if (map->Search(position)) {
                    if (unit1->unit_type == AIRTRANS) {
                        result = map->Search(destination);

                    } else {
                        Rect bounds;
                        Point site;

                        rect_init(&bounds, std::max(0, destination.x - 1), std::max(0, destination.y - 1),
                                  std::min(static_cast<int>(ResourceManager_MapSize.x), destination.x + 2),
                                  std::min(static_cast<int>(ResourceManager_MapSize.y), destination.y + 2));

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
    if (!result) {
        Task_RemindMoveFinished(unit, unit->speed > 0 ? true : false);
    }
}

bool TaskTransport::LoadUnit(UnitInfo* unit) {
    bool result;

    if (unit_transporter && unit_transporter->IsReadyForOrders(this) &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team)) {
        if (Task_IsReadyToTakeOrders(unit) &&
            (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team)) {
            int distance = TaskManager_GetDistance(unit, &*unit_transporter);

            if (unit_transporter->unit_type == AIRTRANS && distance > 0) {
                result = false;

            } else if ((unit_transporter->unit_type == SEATRANS || unit_transporter->unit_type == CLNTRANS) &&
                       distance > 3) {
                result = false;

            } else if (unit_transporter->unit_type == AIRTRANS) {
                unit_transporter->SetParent(unit);

                SDL_assert(GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team);

                UnitsManager_SetNewOrder(&*unit_transporter, ORDER_LOAD, ORDER_STATE_0);

                result = true;

            } else if (unit->speed) {
                unit->target_grid_x = unit_transporter->grid_x;
                unit->target_grid_y = unit_transporter->grid_y;

                SDL_assert(GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team);

                SmartPointer<GroundPath> path =
                    new (std::nothrow) GroundPath(unit_transporter->grid_x, unit_transporter->grid_y);

                path->AddStep(unit_transporter->grid_x - unit->grid_x, unit_transporter->grid_y - unit->grid_y);

                unit->path = &*path;

                unit->Redraw();

                UnitsManager_SetNewOrder(unit, ORDER_MOVE_TO_UNIT, ORDER_STATE_5);

                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_38(unit);
                }

                result = true;

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

void TaskTransport::UnloadUnit(UnitInfo* unit) {
    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
        Point destination = task_move->GetDestination();

        if (destination.x >= 0 && unit_transporter == unit->GetParent()) {
            UnitInfo* unit_in_the_way =
                Access_GetUnit4(destination.x, destination.y, team, MOBILE_SEA_UNIT | MOBILE_LAND_UNIT);

            if (unit_in_the_way) {
                SmartPointer<Zone> zone = new (std::nothrow) Zone(unit, this);

                zone->Add(&destination);

                if (Task_IsReadyToTakeOrders(unit_in_the_way)) {
                    AiPlayer_Teams[team].ClearZone(&*zone);
                }

            } else {
                unit_transporter->target_grid_x = destination.x;
                unit_transporter->target_grid_y = destination.y;

                unit_transporter->SetParent(unit);
            }

            SDL_assert(GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team);

            if (unit_transporter->unit_type == AIRTRANS) {
                UnitsManager_SetNewOrder(&*unit_transporter, ORDER_UNLOAD, ORDER_STATE_0);

            } else {
                UnitsManager_SetNewOrder(&*unit_transporter, ORDER_ACTIVATE, ORDER_STATE_1);
            }
        }
    }
}

ResourceID TaskTransport::GetTransporterType() const { return transporter_unit_type; }
