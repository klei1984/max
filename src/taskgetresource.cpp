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

#include "taskgetresource.hpp"

#include "task_manager.hpp"
#include "taskrendesvous.hpp"
#include "units_manager.hpp"

void TaskGetResource::TaskGetResource_sub_46DF3() {
    SmartPointer<UnitInfo> unit;

    if (unit1 != nullptr) {
        unit2 = nullptr;
        unit3 = nullptr;

        unit = Task_vfunc29();
        Task_vfunc30();

        if (unit != nullptr) {
            if (unit2 == nullptr) {
                unit2 = unit;
            } else {
                int speed1 = TaskManager_sub_4601A(&*unit, &*unit1);
                int speed2 = TaskManager_sub_4601A(&*unit2, &*unit1);

                if (speed2 / 2 > speed1) {
                    unit2 = unit;
                }
            }
        }

        if (unit2 != unit) {
            unit3 = unit2;
        }

        if (unit3 != nullptr) {
            unit3->PushFrontTask1List(this);
        }
    }
}

void TaskGetResource::TaskGetResource_sub_471A4(int unknown, char mode) {
    if (mode == 2) {
        TaskGetResource_sub_471F8();
    } else if (mode == 0) {
        EndTurn();
    }
}

UnitInfo* TaskGetResource::TaskGetResource_sub_46D29(Complex* complex) {
    UnitInfo* selected_unit = nullptr;
    int speed;
    int best_speed;

    for (SmartList<UnitInfo>::Iterator unit = UnitsManager_StationaryUnits.Begin(); unit != nullptr; ++unit) {
        if ((*unit).GetComplex() == complex) {
            speed = TaskManager_sub_4601A(&(*unit), &*unit1);

            if (selected_unit == nullptr || (best_speed > speed)) {
                selected_unit = &*unit;
                best_speed = speed;
            }
        }
    }

    return selected_unit;
}

void TaskGetResource::TaskGetResource_sub_471F8() {
    SmartPointer<UnitInfo> unit(unit3);

    unit2 = nullptr;
    unit3 = nullptr;

    if (unit != nullptr) {
        /// \todo Implement missing stuff
        // unit->unitinfo_sub_F2962(this);
        if (!unit->GetTask1ListFront()) {
            // TaskManager.TaskManager_sub_453AC(&*unit);
        }
    }
}

TaskGetResource::TaskGetResource(Task* task, UnitInfo& unit) : Task(unit.team, task, 0x2300) {
    if (task) {
        flags = task->GetFlags();
    }

    unit1 = unit;
}

TaskGetResource::~TaskGetResource() {}

void TaskGetResource::AddReminder() {
    unit1->PushFrontTask1List(this);
    if (!GetField7()) {
        TaskManager.AddReminder(new (std::nothrow) class RemindTurnStart(*this));
    }
}

void TaskGetResource::EndTurn() {
    if (unit1 != nullptr && unit1->GetTask1ListFront() == this) {
        if (unit3 == nullptr) {
            TaskGetResource_sub_46DF3();
        }

        if (unit3 != nullptr && unit3->GetTask1ListFront() == this) {
            if (unit2->UnitInfo_sub_430A2(unit1->grid_x, unit1->grid_y)) {
                /// \todo Implement missing stuff
                //                if (ini_setting_play_mode || team == byte_1737C8_team) {
                //                    if (ini_setting_play_mode != 2) {
                //                        Task_vfunc28();
                //                    }
                //                }
            } else {
                SmartPointer<TaskRendezvous> task = new (std::nothrow)
                    TaskRendezvous(&*unit1, &*unit2, this, &TaskGetResource::TaskGetResource_sub_471A4);

                TaskManager.AddTask(*task);
            }
        }
    }
}

void TaskGetResource::RemoveSelf() {
    parent = nullptr;

    TaskGetResource_sub_471F8();

    if (unit1 != nullptr) {
        /// \todo Implement missing function
        // unit1->unitinfo_sub_F2962(this);
    }

    unit1 = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskGetResource::Remove(UnitInfo& unit) {
    if (&unit == &*unit1) {
        unit1 = nullptr;
        TaskGetResource_sub_471F8();

        if (unit1 == nullptr && unit2 == nullptr) {
            TaskManager.RemoveTask(*this);
        }
    } else if (&unit == &*unit3) {
        unit2 = nullptr;
        unit3 = nullptr;
    } else if (&unit == &*unit2) {
        TaskGetResource_sub_471F8();
    }
}
