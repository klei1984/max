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

#include "task_manager.hpp"
#include "units_manager.hpp"

TaskAssistMove::TaskAssistMove(unsigned short team_) : Task(team_, nullptr, 0x2800) {}

TaskAssistMove::~TaskAssistMove() {}

bool TaskAssistMove::Task_vfunc1(UnitInfo& unit) { return unit.storage == 0; }

bool TaskAssistMove::Task_vfunc2(UnitInfo& unit) {
    return unit.unit_type == AIRTRANS || unit.unit_type == SEATRANS || unit.unit_type == CLNTRANS;
}

int TaskAssistMove::GetMemoryUse() const { return units.GetMemorySize() - 6; }

char* TaskAssistMove::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Use transports to speed movement");

    return buffer;
}

unsigned char TaskAssistMove::GetType() const { return TaskType_TaskAssistMove; }

void TaskAssistMove::Task_vfunc11(UnitInfo& unit) {
    if (unit.unit_type == AIRTRANS || unit.unit_type == SEATRANS || unit.unit_type == CLNTRANS) {
        units.PushBack(unit);
        unit.PushFrontTask1List(this);
        Task_RemindMoveFinished(&unit);
    }
}

void TaskAssistMove::Execute() {
    /// \todo
}

void TaskAssistMove::EndTurn() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (Task_vfunc17((*it))) {
            return;
        }
    }
}

bool TaskAssistMove::Task_vfunc17(UnitInfo& unit) {
    /// \todo
}

void TaskAssistMove::RemoveSelf() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        (*it).RemoveTask(this);
    }

    units.Clear();

    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskAssistMove::Remove(UnitInfo& unit) { units.Remove(unit); }

void TaskAssistMove::Task_vfunc24(UnitInfo& unit1, UnitInfo& unit2) { Task_RemindMoveFinished(&unit1); }

void TaskAssistMove::Task_vfunc26(UnitInfo& unit1, UnitInfo& unit2) {
    /// \todo
}
