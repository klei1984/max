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

#include "taskcreatebuilding.hpp"

TaskCreateBuilding::TaskCreateBuilding(Task* task, unsigned short flags, ResourceID unit_type, Point site,
                                       TaskManageBuildings* manager)
    : TaskCreate(task, flags, unit_type) {}

TaskCreateBuilding::~TaskCreateBuilding() {}

int TaskCreateBuilding::GetMemoryUse() const {}

char* TaskCreateBuilding::WriteStatusLog(char* buffer) const {}

Rect* TaskCreateBuilding::GetBounds(Rect* bounds) {}

unsigned char TaskCreateBuilding::GetType() const {}

bool TaskCreateBuilding::Task_vfunc9() {}

void TaskCreateBuilding::Task_vfunc11(UnitInfo& unit) {}

void TaskCreateBuilding::Begin() {}

void TaskCreateBuilding::BeginTurn() {}

void TaskCreateBuilding::ChildComplete(Task* task) {}

void TaskCreateBuilding::EndTurn() {}

bool TaskCreateBuilding::Task_vfunc16(UnitInfo& unit) {}

bool TaskCreateBuilding::Task_vfunc17(UnitInfo& unit) {}

void TaskCreateBuilding::RemoveSelf() {}

void TaskCreateBuilding::RemoveUnit(UnitInfo& unit) {}

void TaskCreateBuilding::Task_vfunc27(Zone* zone, char mode) {}

bool TaskCreateBuilding::Task_vfunc28() {}

bool TaskCreateBuilding::IsBuilding() {}
