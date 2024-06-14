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

#include "taskremoverubble.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "taskrendezvous.hpp"
#include "units_manager.hpp"

TaskRemoveRubble::TaskRemoveRubble(Task* task, UnitInfo* unit_, uint16_t flags_) : Task(task->GetTeam(), task, flags_) {
    target = unit_;
}

TaskRemoveRubble::~TaskRemoveRubble() {}

char* TaskRemoveRubble::WriteStatusLog(char* buffer) const {
    if (!target) {
        strcpy(buffer, "Completed rubble removal");

    } else {
        sprintf(buffer, "Remove rubble from [%i,%i]", target->grid_x + 1, target->grid_y + 1);
    }

    return buffer;
}

Rect* TaskRemoveRubble::GetBounds(Rect* bounds) {
    if (!target) {
        bounds = Task::GetBounds(bounds);

    } else {
        bounds->ulx = target->grid_x;
        bounds->uly = target->grid_y;
        bounds->lrx = bounds->ulx + 1;
        bounds->lry = bounds->uly + 1;
    }

    return bounds;
}

uint8_t TaskRemoveRubble::GetType() const { return TaskType_TaskRemoveRubble; }

bool TaskRemoveRubble::IsNeeded() { return !unit; }

void TaskRemoveRubble::AddUnit(UnitInfo& unit_) {
    if (!unit) {
        unit = unit_;
        unit_.AddTask(this);
        Task_RemindMoveFinished(&unit_);
    }
}

void TaskRemoveRubble::Begin() {
    if (!unit) {
        ObtainUnit();
    }
}

void TaskRemoveRubble::EndTurn() {
    if (target && unit) {
        Execute(*unit);
    }
}

bool TaskRemoveRubble::Execute(UnitInfo& unit_) {
    bool result;

    if (unit_.IsReadyForOrders(this)) {
        if (target) {
            target = Access_GetRemovableRubble(unit_.team, target->grid_x, target->grid_y);
        }

        if (target && !Ai_IsDangerousLocation(&unit_, Point(target->grid_x, target->grid_y),
                                              CAUTION_LEVEL_AVOID_ALL_DAMAGE, true)) {
            AiLog log("Remove Rubble: move finished.");

            if (unit_.GetBaseValues()->GetAttribute(ATTRIB_STORAGE) != unit_.storage) {
                if (unit_.grid_x == target->grid_x && unit_.grid_y == target->grid_y) {
                    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
                        unit_.SetParent(&*target);

                        unit_.build_time = (target->flags & BUILDING) ? 4 : 1;

                        UnitsManager_SetNewOrder(&unit_, ORDER_CLEAR, ORDER_STATE_INIT);
                    }

                    result = true;

                } else if (unit_.speed) {
                    if (Task_RetreatFromDanger(this, &unit_, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                        result = true;

                    } else {
                        SmartPointer<TaskMove> move_task(
                            new (std::nothrow) TaskMove(&unit_, this, 0, CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                                        Point(target->grid_x, target->grid_y), &MoveFinishedCallback));

                        TaskManager.AppendTask(*move_task);

                        result = true;
                    }

                } else {
                    result = false;
                }

            } else {
                result = DumpMaterials(&unit_);
            }

        } else {
            RemoveTask();

            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskRemoveRubble::RemoveSelf() {
    parent = nullptr;

    RemoveTask();
}

void TaskRemoveRubble::RemoveUnit(UnitInfo& unit_) {
    if (unit == unit_) {
        unit = nullptr;

        if (target) {
            ObtainUnit();
        }
    }
}

void TaskRemoveRubble::RemoveTask() {
    SmartPointer<Task> task(this);

    if (unit) {
        TaskManager.RemindAvailable(&*unit);
        unit = nullptr;
    }

    if (parent) {
        parent->ChildComplete(this);
    }

    target = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskRemoveRubble::MoveFinishedCallback(Task* task, UnitInfo* unit_, char result) {
    if (result == TASKMOVE_RESULT_SUCCESS) {
        Task_RemindMoveFinished(unit_);
    }
}

bool TaskRemoveRubble::DumpMaterials(UnitInfo* unit_) {
    bool result;

    if (unit_->speed) {
        UnitInfo* best_unit = nullptr;
        int32_t distance;
        int32_t minimum_distance{INT32_MAX};

        AiLog log("Remove Rubble: attempt to dump materials.");

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == team && (*it).storage < (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE) &&
                (*it).hits > 0 && UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type == CARGO_TYPE_RAW) {
                Complex* complex = (*it).GetComplex();

                for (SmartList<UnitInfo>::Iterator building = UnitsManager_StationaryUnits.Begin();
                     building != UnitsManager_StationaryUnits.End(); ++building) {
                    if ((*building).GetComplex() == complex) {
                        if (Task_IsAdjacent(building->Get(), unit_->grid_x, unit_->grid_y)) {
                            if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
                                unit_->target_grid_x =
                                    std::min(static_cast<int32_t>(unit_->storage),
                                             (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - (*it).storage);
                                unit_->SetParent(it->Get());

                                UnitsManager_SetNewOrder(unit_, ORDER_TRANSFER, ORDER_STATE_INIT);
                            }

                            result = true;

                            return result;

                        } else {
                            distance = TaskManager_GetDistance(building->Get(), unit_);

                            if (!best_unit || distance < minimum_distance) {
                                minimum_distance = distance;

                                best_unit = building->Get();
                            }
                        }
                    }
                }
            }
        }

        if (best_unit) {
            SmartPointer<Task> rendezvous_task(new (std::nothrow)
                                                   TaskRendezvous(unit_, best_unit, this, &MoveFinishedCallback));

            TaskManager.AppendTask(*rendezvous_task);

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskRemoveRubble::ObtainUnit() {
    SmartPointer<TaskObtainUnits> obtain_task(new (std::nothrow) TaskObtainUnits(this, DeterminePosition()));
    obtain_task->AddUnit(BULLDOZR);

    TaskManager.AppendTask(*obtain_task);
}
