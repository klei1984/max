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

#include "taskcreate.hpp"

#include "task_manager.hpp"

TaskCreate::TaskCreate(Task* task, uint16_t priority, ResourceID unit_type_)
    : Task(task->GetTeam(), task, priority), unit_type(unit_type_) {}

TaskCreate::TaskCreate(Task* task, UnitInfo* unit_) : Task(unit_->team, task, task->GetPriority()), builder(unit_) {
    if (builder->GetOrderState() == ORDER_STATE_UNIT_READY) {
        unit_type = unit_->GetParent()->GetUnitType();

    } else {
        unit_type = unit_->GetConstructedUnitType();
    }
}

TaskCreate::~TaskCreate() {}

void TaskCreate::RemoveSelf() {
    if (builder) {
        TaskManager.ClearUnitTasksAndRemindAvailable(&*builder);
    }

    builder = nullptr;
    m_parent = nullptr;

    TaskManager.RemoveTask(*this);
}

ResourceID TaskCreate::GetUnitType() const { return unit_type; }
