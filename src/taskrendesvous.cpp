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

#include "taskrendesvous.hpp"

#include "task_manager.hpp"
#include "taskgetresource.hpp"
#include "units_manager.hpp"

TaskRendezvous::TaskRendezvous(UnitInfo* unit1, UnitInfo* unit2, Task* task,
                               void (TaskGetResource::*function)(int unknown, char mode))
    : Task(task->GetTeam(), task, task->GetFlags()), unit1(unit1), unit2(unit2), function(function) {}

TaskRendezvous::~TaskRendezvous() {}

int TaskRendezvous::GetMemoryUse() const { return 4; }

char* TaskRendezvous::WriteStatusLog(char* buffer) const {
    if (unit1 == nullptr || unit2 == nullptr) {
        strcpy(buffer, "Completed rendezvous task.");
    } else {
        strcpy(buffer, "Bring ");
        strcat(buffer, UnitsManager_BaseUnits[unit2->unit_type].singular_name);
        strcat(buffer, " and ");
        strcat(buffer, UnitsManager_BaseUnits[unit1->unit_type].singular_name);
        strcat(buffer, " together.");
    }

    return buffer;
}

unsigned char TaskRendezvous::GetType() const { return TaskType_TaskRendezvous; }

void TaskRendezvous::AddReminder() {
    if (unit1->GetBaseValues()->GetAttribute(ATTRIB_SPEED) > 0) {
        unit1->PushFrontTask1List(this);
    }

    if (unit2->GetBaseValues()->GetAttribute(ATTRIB_SPEED) > 0) {
        unit2->PushFrontTask1List(this);
    }

    Task_RemindMoveFinished(&*unit1);
}

void TaskRendezvous::Execute() { EndTurn(); }

void TaskRendezvous::EndTurn() {
    if (unit1 != nullptr && unit2 != nullptr) {
        if (unit2->orders == ORDER_AWAIT && unit2->GetTask1ListFront() == this && unit2->speed) {
            Task_vfunc17(*unit2);
        } else if (unit1->orders == ORDER_AWAIT && unit1->GetTask1ListFront() == this && unit1->speed) {
            Task_vfunc17(*unit1);
        }
    }
}

bool TaskRendezvous::Task_vfunc17(UnitInfo& unit) {
    /// \todo Implement method
    return false;
}

void TaskRendezvous::RemoveSelf() {
    if (unit1 != nullptr) {
        /// \todo Implement missing stuff
        // unit1->unitinfo_sub_F2962(this);
    }

    if (unit2 != nullptr) {
        /// \todo Implement missing stuff
        // unit2->unitinfo_sub_F2962(this);
    }

    unit1 = nullptr;
    unit2 = nullptr;
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskRendezvous::Remove(UnitInfo& unit) {
    if (unit1 == unit || unit2 == unit) {
        if (unit1 != unit) {
            /// \todo Implement missing stuff
            // unit1->unitinfo_sub_F2962(this);
        }

        if (unit2 != unit) {
            /// \todo Implement missing stuff
            // unit2->unitinfo_sub_F2962(this);
        }

        TaskRendezvous_sub_54D3B();
    }
}

void TaskRendezvous::TaskRendezvous_sub_54D3B() {
    unit1 = nullptr;
    unit2 = nullptr;
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}
