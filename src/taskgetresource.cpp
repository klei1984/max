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

#include "taskgetresource.hpp"

#include "game_manager.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "taskrendezvous.hpp"
#include "units_manager.hpp"

void TaskGetResource::ChooseSource() {
    SmartPointer<UnitInfo> unit;

    if (requestor) {
        supplier = nullptr;
        source = nullptr;

        unit = FindBuilding();
        FindTruck();

        if (unit) {
            if (!supplier) {
                supplier = unit;

            } else {
                int32_t distance1 = TaskManager_GetDistance(&*unit, &*requestor);
                int32_t distance2 = TaskManager_GetDistance(&*supplier, &*requestor);

                if (distance2 / 2 > distance1) {
                    supplier = unit;
                }
            }
        }

        if (supplier != unit) {
            source = supplier;
        }

        if (source) {
            source->AddTask(this);
        }
    }
}

void TaskGetResource::RendezvousResultCallback(Task* task, UnitInfo* unit, char mode) {
    if (mode == TASKMOVE_RESULT_BLOCKED) {
        dynamic_cast<TaskGetResource*>(task)->ReleaseSource();

    } else if (mode == TASKMOVE_RESULT_SUCCESS) {
        dynamic_cast<TaskGetResource*>(task)->EndTurn();
    }
}

UnitInfo* TaskGetResource::FindClosestBuilding(Complex* complex) {
    UnitInfo* best_building = nullptr;
    int32_t distance;
    int32_t minimum_distance{INT32_MAX};

    for (SmartList<UnitInfo>::Iterator unit = UnitsManager_StationaryUnits.Begin();
         unit != UnitsManager_StationaryUnits.End(); ++unit) {
        if ((*unit).GetComplex() == complex) {
            distance = TaskManager_GetDistance(&(*unit), &*requestor);

            if (best_building == nullptr || (minimum_distance > distance)) {
                best_building = &*unit;
                minimum_distance = distance;
            }
        }
    }

    return best_building;
}

void TaskGetResource::ReleaseSource() {
    SmartPointer<UnitInfo> unit(source);

    supplier = nullptr;
    source = nullptr;

    if (unit) {
        unit->RemoveTask(this);

        if (!unit->GetTask()) {
            TaskManager.RemindAvailable(&*unit);
        }
    }
}

TaskGetResource::TaskGetResource(Task* task, UnitInfo& unit) : Task(unit.team, task, 0x2300) {
    if (task) {
        flags = task->GetFlags();
    }

    requestor = unit;
}

TaskGetResource::~TaskGetResource() {}

void TaskGetResource::Begin() {
    requestor->AddTask(this);

    if (!IsScheduledForTurnStart()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this));
    }
}

void TaskGetResource::EndTurn() {
    if (requestor && requestor->GetTask() == this) {
        if (!source) {
            ChooseSource();
        }

        if (source && source->GetTask() == this) {
            if (Task_IsAdjacent(&*supplier, requestor->grid_x, requestor->grid_y)) {
                if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
                    if (GameManager_IsActiveTurn(team)) {
                        DoTransfer();
                    }
                }

            } else {
                SmartPointer<TaskRendezvous> task = new (std::nothrow)
                    TaskRendezvous(&*requestor, &*supplier, this, &TaskGetResource::RendezvousResultCallback);

                TaskManager.AppendTask(*task);
            }
        }
    }
}

void TaskGetResource::RemoveSelf() {
    parent = nullptr;

    ReleaseSource();

    if (requestor) {
        requestor->RemoveTask(this);
    }

    requestor = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskGetResource::RemoveUnit(UnitInfo& unit) {
    if (&unit == &*requestor) {
        requestor = nullptr;
        ReleaseSource();

        if (!requestor && !supplier) {
            TaskManager.RemoveTask(*this);
        }

    } else if (&unit == &*source) {
        supplier = nullptr;
        source = nullptr;

    } else if (&unit == &*supplier) {
        ReleaseSource();
    }
}
