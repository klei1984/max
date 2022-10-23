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

#include "taskattack.hpp"

#include "task_manager.hpp"

TaskAttack::TaskAttack(SpottedUnit* spotted_unit, unsigned short flags_)
    : Task(spotted_unit->GetTeam(), nullptr, flags_) {}

TaskAttack::~TaskAttack() {}

bool TaskAttack::IsUnitUsable(UnitInfo& unit) {}

int TaskAttack::GetCautionLevel(UnitInfo& unit) {}

int TaskAttack::GetMemoryUse() const {}

unsigned short TaskAttack::GetFlags() const {}

char* TaskAttack::WriteStatusLog(char* buffer) const {}

Rect* TaskAttack::GetBounds(Rect* bounds) {}

unsigned char TaskAttack::GetType() const {}

bool TaskAttack::Task_vfunc9() {}

void TaskAttack::Task_vfunc11(UnitInfo& unit) {}

void TaskAttack::Begin() {}

void TaskAttack::BeginTurn() {}

void TaskAttack::ChildComplete(Task* task) {}

void TaskAttack::EndTurn() {}

bool TaskAttack::Task_vfunc16(UnitInfo& unit) {}

bool TaskAttack::Task_vfunc17(UnitInfo& unit) {}

void TaskAttack::RemoveSelf() {}

void TaskAttack::RemoveUnit(UnitInfo& unit) {}

unsigned int TaskAttack::GetField58() const {}

int TaskAttack::GetHighestScan() {}

bool TaskAttack::MoveCombatUnit(Task* task, UnitInfo* unit) {}

bool TaskAttack::IsAtLocation(UnitInfo* unit) {}

UnitInfo* TaskAttack::DetermineLeader() {}
