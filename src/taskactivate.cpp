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

#include "taskactivate.hpp"

#include "task_manager.hpp"

TaskActivate::TaskActivate(UnitInfo* unit_, Task* task, UnitInfo* parent_) : Task(unit_->team, task, task->GetFlags()) {
    unit = unit_;
    parent = parent_;
}

TaskActivate::~TaskActivate() {}

bool TaskActivate::Task_vfunc1(UnitInfo& unit_) { return unit != unit_; }

int TaskActivate::GetMemoryUse() const { return 4; }

char* TaskActivate::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Activate unit");

    return buffer;
}

Rect* TaskActivate::GetBounds(Rect* bounds) {}

unsigned char TaskActivate::GetType() const { return TaskType_TaskActivate; }

void TaskActivate::Task_vfunc11(UnitInfo& unit) {}

void TaskActivate::AddReminder() {}

void TaskActivate::EndTurn() {}

bool TaskActivate::Task_vfunc17(UnitInfo& unit) {}

void TaskActivate::RemoveSelf() {}

void TaskActivate::Remove(UnitInfo& unit) {}

void TaskActivate::Task_vfunc27(Zone* zone, char mode) {}
