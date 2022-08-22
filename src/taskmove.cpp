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

#include "taskmove.hpp"

#include "task_manager.hpp"

TaskMove::TaskMove(UnitInfo* unit, Task* task, unsigned short minimum_distance, unsigned char caution_level,
                   Point destination, void (*callback)(Task* task, UnitInfo* unit, char result))
    : Task(unit->team, task, task ? task->GetFlags() : 0x1000) {}

TaskMove::TaskMove(UnitInfo* unit, void (*callback)(Task* task, UnitInfo* unit, char result))
    : Task(unit->team, nullptr, 0x2900) {}

TaskMove::~TaskMove() {}

int TaskMove::GetMemoryUse() const {}

char* TaskMove::WriteStatusLog(char* buffer) const {}

unsigned char TaskMove::GetType() const { return TaskType_TaskMove; }

void TaskMove::SetField68(bool value) {}

UnitInfo* TaskMove::GetPassenger() {}

void TaskMove::SetDestination(Point site) {}

ResourceID TaskMove::GetTransporterType() const {}

Point TaskMove::GetPoint2() const {}

void TaskMove::TaskMove_sub_4C66B(bool mode) {}

bool TaskMove::TaskMove_sub_4D247() {}

Point TaskMove::GetDestination() {}
