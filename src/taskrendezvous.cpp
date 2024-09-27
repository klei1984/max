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

#include "taskrendezvous.hpp"

#include "access.hpp"
#include "game_manager.hpp"
#include "task_manager.hpp"
#include "taskgetresource.hpp"
#include "taskmove.hpp"
#include "transportermap.hpp"
#include "units_manager.hpp"

static bool TaskRendezvous_SearchLocation(UnitInfo* unit1, UnitInfo* unit2, Point* site);
static bool TaskRendezvous_SearchMap(UnitInfo* unit1, UnitInfo* unit2, Point* site, char mode);

bool TaskRendezvous_SearchLocation(UnitInfo* unit1, UnitInfo* unit2, Point* site) {
    return TaskRendezvous_SearchMap(unit1, unit2, site, 2) || TaskRendezvous_SearchMap(unit1, unit2, site, 1);
}

bool TaskRendezvous_SearchMap(UnitInfo* unit1, UnitInfo* unit2, Point* site, char mode) {
    TransporterMap transporter_map(unit1, 1, CAUTION_LEVEL_NONE,
                                   (unit1->flags & MOBILE_LAND_UNIT) ? AIRTRANS : INVALID_ID);
    int32_t range = (unit2->flags & BUILDING) ? 3 : 2;
    Point position1(unit1->grid_x, unit1->grid_y);
    Point position2(unit2->grid_x - 1, unit2->grid_y + range - 1);
    bool result = false;
    int32_t distance;
    int32_t minimum_distance{INT32_MAX};

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t i = 0; i < range; ++i) {
            position2 += Paths_8DirPointsArray[direction];

            distance = TaskManager_GetDistance(position1, position2);

            if (!result || distance < minimum_distance) {
                if (Access_IsAccessible(unit1->GetUnitType(), unit1->team, position2.x, position2.y, mode)) {
                    if (transporter_map.Search(position2)) {
                        *site = position2;
                        result = true;
                        minimum_distance = distance;
                    }
                }
            }
        };
    }

    return result;
}

TaskRendezvous::TaskRendezvous(UnitInfo* unit1, UnitInfo* unit2, Task* task,
                               void (*result_callback_)(Task* task, UnitInfo* unit, char mode))
    : Task(task->GetTeam(), task, task->GetFlags()), unit1(unit1), unit2(unit2), result_callback(result_callback_) {}

TaskRendezvous::~TaskRendezvous() {}

void TaskRendezvous::PrimaryMoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskRendezvous* task_rendezvous = dynamic_cast<TaskRendezvous*>(task);
    SmartPointer<UnitInfo> local_unit(task_rendezvous->unit1);

    if (local_unit && task_rendezvous->unit2) {
        if (result == TASKMOVE_RESULT_SUCCESS) {
            Task_RemindMoveFinished(unit);

        } else if (result == TASKMOVE_RESULT_BLOCKED) {
            if (local_unit->speed && local_unit->IsReadyForOrders(task_rendezvous)) {
                if ((task_rendezvous->unit2->flags & STATIONARY) ||
                    (task_rendezvous->unit2->speed && task_rendezvous->unit2->IsReadyForOrders(task_rendezvous))) {
                    task_rendezvous->Finish(TASKMOVE_RESULT_BLOCKED);
                }
            }
        }
    }
}

void TaskRendezvous::SecondaryMoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskRendezvous* task_rendezvous = dynamic_cast<TaskRendezvous*>(task);

    if (task_rendezvous->unit1 && task_rendezvous->unit2) {
        if (result == TASKMOVE_RESULT_SUCCESS) {
            Task_RemindMoveFinished(unit);

        } else if (result == TASKMOVE_RESULT_BLOCKED) {
            SmartPointer<UnitInfo> local_unit1(task_rendezvous->unit1);
            SmartPointer<UnitInfo> local_unit2(task_rendezvous->unit2);

            if (local_unit1->speed && local_unit1->IsReadyForOrders(task_rendezvous)) {
                if (local_unit2->speed == 0 ||
                    (Task_IsReadyToTakeOrders(&*local_unit2) &&
                     (local_unit2->GetTask() == task_rendezvous || local_unit2->GetTask() == nullptr ||
                      local_unit2->GetTask()->GetType() == TaskType_TaskMove) &&
                     !Task_RetreatIfNecessary(task_rendezvous, &*local_unit1, CAUTION_LEVEL_AVOID_ALL_DAMAGE))) {
                    Point destination;

                    if (TaskRendezvous_SearchLocation(&*local_unit1, &*local_unit2, &destination)) {
                        SmartPointer<TaskMove> move_task(new (std::nothrow) TaskMove(
                            &*local_unit1, task_rendezvous, 0, CAUTION_LEVEL_AVOID_ALL_DAMAGE, destination,
                            &PrimaryMoveFinishedCallback));

                        move_task->SetField68(!(local_unit2->flags & STATIONARY));

                        TaskManager.AppendTask(*move_task);

                    } else {
                        PrimaryMoveFinishedCallback(task_rendezvous, &*local_unit1, TASKMOVE_RESULT_BLOCKED);
                    }
                }

            } else {
                PrimaryMoveFinishedCallback(task_rendezvous, &*local_unit1, TASKMOVE_RESULT_BLOCKED);
            }
        }
    }
}

char* TaskRendezvous::WriteStatusLog(char* buffer) const {
    if (unit1 == nullptr || unit2 == nullptr) {
        strcpy(buffer, "Completed rendezvous task.");
    } else {
        strcpy(buffer, "Bring ");
        strcat(buffer, UnitsManager_BaseUnits[unit2->GetUnitType()].singular_name);
        strcat(buffer, " and ");
        strcat(buffer, UnitsManager_BaseUnits[unit1->GetUnitType()].singular_name);
        strcat(buffer, " together.");
    }

    return buffer;
}

uint8_t TaskRendezvous::GetType() const { return TaskType_TaskRendezvous; }

void TaskRendezvous::Begin() {
    if (unit1->GetBaseValues()->GetAttribute(ATTRIB_SPEED) > 0) {
        unit1->AddTask(this);
    }

    if (unit2->GetBaseValues()->GetAttribute(ATTRIB_SPEED) > 0) {
        unit2->AddTask(this);
    }

    Task_RemindMoveFinished(&*unit1);
}

void TaskRendezvous::BeginTurn() { EndTurn(); }

void TaskRendezvous::EndTurn() {
    if (unit1 != nullptr && unit2 != nullptr) {
        if (unit2->GetOrder() == ORDER_AWAIT && unit2->GetTask() == this && unit2->speed) {
            Execute(*unit2);
        } else if (unit1->GetOrder() == ORDER_AWAIT && unit1->GetTask() == this && unit1->speed) {
            Execute(*unit1);
        }
    }
}

bool TaskRendezvous::Execute(UnitInfo& unit) {
    bool result = false;

    if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
        if (GameManager_IsActiveTurn(team)) {
            if (unit1 && unit2 && (unit1 == unit || unit2 == unit)) {
                if (unit1->GetTask() == this || unit1->GetTask() == nullptr ||
                    unit1->GetTask()->GetType() == TaskType_TaskMove) {
                    if (Task_IsAdjacent(&*unit2, unit1->grid_x, unit1->grid_y)) {
                        Finish(TASKMOVE_RESULT_SUCCESS);

                    } else if (unit2->speed && unit2->IsReadyForOrders(this)) {
                        if (unit1->IsReadyForOrders(this) &&
                            !Task_RetreatFromDanger(this, &*unit2, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                            Point destination;

                            if (TaskRendezvous_SearchLocation(&*unit2, &*unit1, &destination)) {
                                SmartPointer<TaskMove> move_task(
                                    new (std::nothrow) TaskMove(&*unit2, this, 0, CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                                                destination, &SecondaryMoveFinishedCallback));

                                move_task->SetField68(!(unit1->flags & STATIONARY));

                                TaskManager.AppendTask(*move_task);

                            } else {
                                SecondaryMoveFinishedCallback(this, &*unit2, TASKMOVE_RESULT_BLOCKED);
                            }

                            result = true;
                        }

                    } else {
                        SecondaryMoveFinishedCallback(this, &*unit2, TASKMOVE_RESULT_BLOCKED);
                    }
                }
            }
        }
    }

    return result;
}

void TaskRendezvous::RemoveSelf() {
    if (unit1 != nullptr) {
        unit1->RemoveTask(this);
    }

    if (unit2 != nullptr) {
        unit2->RemoveTask(this);
    }

    unit1 = nullptr;
    unit2 = nullptr;
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskRendezvous::RemoveUnit(UnitInfo& unit) {
    if (unit1 == unit || unit2 == unit) {
        if (unit1 != unit) {
            unit1->RemoveTask(this);
        }

        if (unit2 != unit) {
            unit2->RemoveTask(this);
        }

        RemoveTask();
    }
}

void TaskRendezvous::RemoveTask() {
    unit1 = nullptr;
    unit2 = nullptr;
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskRendezvous::Finish(char result) {
    unit1->RemoveTask(this, false);
    unit2->RemoveTask(this, false);

    Task_RemoveMovementTasks(&*unit1);
    Task_RemoveMovementTasks(&*unit2);

    result_callback(&*parent, &*unit2, result);

    RemoveTask();
}
