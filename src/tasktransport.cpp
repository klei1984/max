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

#include "task_manager.hpp"

TaskTransport::TaskTransport(TaskMove* task_move_, ResourceID transporter)
    : Task(task_move->GetTeam(), nullptr, task_move->GetFlags()) {
    transporter_unit_type = transporter;
    task_move = task_move_;
    move_tasks.PushBack(*task_move_);
    task_move_->GetPassenger()->PushBackTask2List(this);
}

TaskTransport::~TaskTransport() {}

bool TaskTransport::Task_vfunc1(UnitInfo& unit) { return unit_transporter != unit; }

int TaskTransport::GetMemoryUse() const { return move_tasks.GetMemorySize() - 6; }

char* TaskTransport::WriteStatusLog(char* buffer) const {}

Rect* TaskTransport::GetBounds(Rect* bounds) {}

unsigned char TaskTransport::GetType() const { return TaskType_TaskTransport; }

bool TaskTransport::Task_vfunc9() {}

void TaskTransport::Task_vfunc11(UnitInfo& unit) {}

void TaskTransport::AddReminder() {}

void TaskTransport::Execute() {}

void TaskTransport::Task_vfunc14(Task* task) {}

void TaskTransport::EndTurn() {}

bool TaskTransport::Task_vfunc17(UnitInfo& unit) {}

void TaskTransport::RemoveSelf() {}

void TaskTransport::Remove(UnitInfo& unit) {}

void TaskTransport::Task_vfunc24(UnitInfo& unit1, UnitInfo& unit2) {}

void TaskTransport::Task_vfunc26(UnitInfo& unit1, UnitInfo& unit2) {}

void TaskTransport::Task_vfunc27(Zone* zone, char mode) {}
