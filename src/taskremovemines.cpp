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

#include "taskremovemines.hpp"

#include "access.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "units_manager.hpp"

TaskRemoveMines::TaskRemoveMines(Task* task, UnitInfo* unit_) : TaskRemoveRubble(task, unit_, 0x100) {}

TaskRemoveMines::~TaskRemoveMines() {}

char* TaskRemoveMines::WriteStatusLog(char* buffer) const {
    if (target) {
        sprintf(buffer, "Remove mine from [%i,%i]", target->grid_x + 1, target->grid_y + 1);

    } else {
        strcpy(buffer, "Completed mine removal");
    }

    return buffer;
}

uint8_t TaskRemoveMines::GetType() const { return TaskType_TaskRemoveMines; }

bool TaskRemoveMines::Execute(UnitInfo& unit_) {
    bool result;

    if (unit_.IsReadyForOrders(this)) {
        if (unit_.GetLayingState() == 1) {
            UnitsManager_SetNewOrder(&unit_, ORDER_LAY_MINE, ORDER_STATE_INACTIVE);

            result = true;

        } else {
            if (target && target->hits > 0) {
                if (unit_.GetBaseValues()->GetAttribute(ATTRIB_STORAGE) == unit_.storage) {
                    result = DumpMaterials(&unit_);

                } else if (target->grid_x == unit_.grid_x && target->grid_y == unit_.grid_y) {
                    if (GameManager_IsActiveTurn(team)) {
                        UnitsManager_SetNewOrder(&unit_, ORDER_LAY_MINE, ORDER_STATE_REMOVING_MINES);
                        target = nullptr;
                    }

                    result = true;

                } else if (unit_.speed > 0) {
                    if (Task_RetreatFromDanger(this, &unit_, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                        result = true;

                    } else {
                        SmartPointer<Task> move_task(
                            new (std::nothrow) TaskMove(&unit_, this, 0, CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                                        Point(target->grid_x, target->grid_y), &MoveFinishedCallback));

                        TaskManager.AppendTask(*move_task);

                        result = true;
                    }

                } else {
                    result = false;
                }

            } else {
                RemoveTask();

                result = true;
            }
        }

    } else {
        result = false;
    }

    return result;
}

void TaskRemoveMines::ObtainUnit() {
    SmartPointer<TaskObtainUnits> obtain_task(new (std::nothrow) TaskObtainUnits(this, DeterminePosition()));

    if (Access_GetModifiedSurfaceType(target->grid_x, target->grid_y) == SURFACE_TYPE_LAND) {
        obtain_task->AddUnit(MINELAYR);

    } else {
        obtain_task->AddUnit(SEAMNLYR);
    }

    TaskManager.AppendTask(*obtain_task);
}
