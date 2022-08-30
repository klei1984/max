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

#include "taskexplore.hpp"

void TaskExplore::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {}

TaskExplore::TaskExplore(unsigned short team_, Point point_) : TaskAbstractSearch(team_, nullptr, 0x1B00, point_) {}

TaskExplore::~TaskExplore() {}

bool TaskExplore::IsUnitUsable(UnitInfo& unit) {}

int TaskExplore::GetMemoryUse() const { return units.GetMemorySize() - 6; }

char* TaskExplore::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Explore map.");

    return buffer;
}

unsigned char TaskExplore::GetType() const { return TaskType_TaskExplore; }

bool TaskExplore::Task_vfunc17(UnitInfo& unit) {}

void TaskExplore::TaskAbstractSearch_vfunc28(UnitInfo& unit) {}

bool TaskExplore::IsVisited(UnitInfo& unit, Point point) {}

void TaskExplore::ObtainUnit() {}
