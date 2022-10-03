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

#include "taskescort.hpp"

#include "aiattack.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "units_manager.hpp"

bool TaskEscort::IssueOrders(UnitInfo* unit) {}

void TaskEscort::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    if (result != TASKMOVE_RESULT_SUCCESS || !AiAttack_EvaluateAssault(unit, task, &MoveFinishedCallback)) {
        if (unit->IsReadyForOrders(task)) {
            dynamic_cast<TaskEscort*>(task)->IssueOrders(unit);
        }
    }
}

TaskEscort::TaskEscort(UnitInfo* unit, ResourceID unit_type_) : Task(unit->team, nullptr, 0x0F00) {
    target = unit;
    unit_type = unit_type_;
    field_29 = 0;
}

TaskEscort::~TaskEscort() {}

bool TaskEscort::Task_vfunc1(UnitInfo& unit) { return (!target || target->hits == 0 || escort != unit); }

int TaskEscort::GetMemoryUse() const { return 4; }

char* TaskEscort::WriteStatusLog(char* buffer) const {
    if (target) {
        sprintf(buffer, "Escort %s at [%i,%i]", UnitsManager_BaseUnits[target->unit_type].singular_name,
                target->grid_x + 1, target->grid_y + 1);

    } else {
        strcpy(buffer, "Completed escort task");
    }

    return buffer;
}

unsigned char TaskEscort::GetType() const { return TaskType_TaskEscort; }

void TaskEscort::Task_vfunc11(UnitInfo& unit) {
    if (!escort && unit_type == unit.unit_type && target) {
        escort = unit;
        escort->PushFrontTask1List(this);
        escort->AddReminders(true);
        field_29 = 0;
    }
}

void TaskEscort::Begin() {
    target->PushBackTask2List(this);
    RemindTurnStart(true);
}

void TaskEscort::EndTurn() {}

bool TaskEscort::Task_vfunc17(UnitInfo& unit) {}

void TaskEscort::RemoveSelf() {
    if (escort) {
        escort->RemoveTask(this, false);

        Task_RemoveMovementTasks(&*escort);
    }

    if (target) {
        target->RemoveFromTask2List(this);
    }

    target = nullptr;
    escort = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskEscort::RemoveUnit(UnitInfo& unit) {
    if (escort == unit) {
        escort = nullptr;
    }
}
