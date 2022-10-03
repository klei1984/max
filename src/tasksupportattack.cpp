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

#include "tasksupportattack.hpp"

#include "aiattack.hpp"
#include "task_manager.hpp"
#include "taskattack.hpp"
#include "taskmove.hpp"

void TaskSupportAttack::GetUnits(unsigned int value) {}

bool TaskSupportAttack::IssueOrders(UnitInfo* unit) {}

void TaskSupportAttack::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    if (result != TASKMOVE_RESULT_SUCCESS || !AiAttack_EvaluateAssault(unit, task, &MoveFinishedCallback)) {
        dynamic_cast<TaskSupportAttack*>(task)->IssueOrders(unit);
    }
}

TaskSupportAttack::TaskSupportAttack(Task* task) : Task(task->GetTeam(), task, 0x2500) {
    field_33 = 0;
    field_29 = -1;
    field_31 = -1;
}

TaskSupportAttack::~TaskSupportAttack() {}

bool TaskSupportAttack::IsUnitUsable(UnitInfo& unit) { return false; }

int TaskSupportAttack::GetMemoryUse() const { return units.GetMemorySize() - 6; }

char* TaskSupportAttack::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Support attack.");

    return buffer;
}

Rect* TaskSupportAttack::GetBounds(Rect* bounds) {
    Rect* result;

    if (parent) {
        result = parent->GetBounds(bounds);

    } else {
        result = Task::GetBounds(bounds);
    }

    return result;
}

unsigned char TaskSupportAttack::GetType() const { return TaskType_TaskSupportAttack; }

bool TaskSupportAttack::Task_vfunc9() { return parent; }

void TaskSupportAttack::Task_vfunc11(UnitInfo& unit) {}

void TaskSupportAttack::BeginTurn() {
    if (parent) {
        GetUnits(dynamic_cast<TaskAttack*>(&*parent)->GetField58());

        if (!GetField8()) {
            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
        }
    }
}

void TaskSupportAttack::EndTurn() { AddReminders(); }

bool TaskSupportAttack::Task_vfunc17(UnitInfo& unit) {}

void TaskSupportAttack::RemoveSelf() {
    parent = nullptr;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units.Clear();

    TaskManager.RemoveTask(*this);
}

void TaskSupportAttack::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }

bool TaskSupportAttack::AddReminders() {}

SmartList<UnitInfo>::Iterator TaskSupportAttack::GetUnitsListIterator() { return units.Begin(); }
