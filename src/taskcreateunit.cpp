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

#include "taskcreateunit.hpp"

TaskCreateUnit::TaskCreateUnit(ResourceID unit_type, Task* task, Point site_)
    : TaskCreate(task, task->GetFlags(), unit_type), site(site_), state(0) {}

TaskCreateUnit::~TaskCreateUnit() {}

int TaskCreateUnit::GetMemoryUse() const {}

unsigned short TaskCreateUnit::GetFlags() const {}

char* TaskCreateUnit::WriteStatusLog(char* buffer) const {}

unsigned char TaskCreateUnit::GetType() const {}

void TaskCreateUnit::Task_vfunc11(UnitInfo& unit) {}

void TaskCreateUnit::AddReminder() {}

void TaskCreateUnit::BeginTurn() {}

void TaskCreateUnit::EndTurn() {}

bool TaskCreateUnit::Task_vfunc17(UnitInfo& unit) {}

void TaskCreateUnit::RemoveSelf() {}

void TaskCreateUnit::RemoveUnit(UnitInfo& unit) {}

void TaskCreateUnit::Task_vfunc27(Zone* zone, char mode) {}

bool TaskCreateUnit::Task_vfunc28() {}

bool TaskCreateUnit::Task_vfunc29() {}
