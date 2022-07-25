/* Copyright (c) 2021 M.A.X. Port Team
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

#include "tasks.hpp"

#include "inifile.hpp"
#include "reminders.hpp"
#include "task_manager.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"

unsigned short Task::task_id = 0;
unsigned short Task::task_count = 0;

void Task_RemindMoveFinished(UnitInfo* unit, bool priority) {
    if (unit && (unit->GetField221() & 0x100) == 0) {
        TaskManager.AddReminder(new (std::nothrow) RemindMoveFinished(*unit), priority);
    }
}

bool Task_IsReadyToTakeOrders(UnitInfo* unit) {
    /// \todo
}

Task::Task(unsigned short team, Task* parent, unsigned short flags)
    : id(++task_id), team(team), parent(parent), flags(flags), field_6(true), field_7(false), field_8(false) {
    ++task_count;
}

Task::Task(const Task& other)
    : id(other.id),
      team(other.team),
      parent(other.parent),
      flags(other.flags),
      field_6(other.field_6),
      field_7(other.field_7),
      field_8(other.field_8) {
    ++task_count;
}

Task::~Task() { --task_count; }

void Task::RemindTurnEnd(bool priority) {
    if (!GetField8()) {
        TaskManager.AddReminder(new (std::nothrow) class RemindTurnEnd(*this), priority);
    }
}

void Task::RemindTurnStart(bool priority) {
    if (!GetField7()) {
        TaskManager.AddReminder(new (std::nothrow) class RemindTurnStart(*this), priority);
    }
}

bool Task::GetField6() const { return field_6; }

void Task::SetField6(bool value) { field_6 = value; }

bool Task::GetField7() const { return field_7; }

void Task::SetField7(bool value) { field_7 = value; }

bool Task::GetField8() const { return field_8; }

void Task::SetField8(bool value) { field_8 = value; }

unsigned short Task::GetTeam() const { return team; }

short Task::Task_sub_42BC4(unsigned short task_flags) {
    int result;

    if (flags + 0xFF >= task_flags) {
        if (flags - 0xFF <= task_flags) {
            result = GetFlags() - task_flags;
        } else {
            result = 1;
        }
    } else {
        result = -1;
    }

    return result;
}

Point Task::Task_sub_42D3D() {
    Rect bounds;

    GetBounds(&bounds);

    return Point(bounds.ulx, bounds.uly);
}

bool Task::Task_vfunc1(UnitInfo& unit) {
    bool result = true;

    if (parent != nullptr && !parent->Task_vfunc1(unit)) {
        result = false;
    }

    return result;
}

bool Task::Task_vfunc2(UnitInfo& unit) { return false; }

int Task::Task_vfunc3(UnitInfo& unit) {
    int result;

    if (unit.base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
        ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        if (unit.shots) {
            result = 1;
        } else {
            result = 3;
        }
    } else if (unit.ammo) {
        result = 2;
    } else {
        result = 3;
    }

    return result;
}

unsigned short Task::GetFlags() const { return flags; }

Rect* Task::GetBounds(Rect* bounds) {
    bounds->ulx = 0;
    bounds->uly = 0;
    bounds->lrx = 0;
    bounds->lry = 0;

    return bounds;
}

bool Task::Task_vfunc9() {
    bool result;

    if (parent != nullptr) {
        result = parent->Task_vfunc9();
    } else {
        result = true;
    }

    return result;
}

bool Task::Task_vfunc10() { return false; }

void Task::Task_vfunc11(UnitInfo& unit) {}

void Task::AddReminder() { RemindTurnStart(true); }

void Task::Task_vfunc13() { EndTurn(); }

void Task::Task_vfunc14(Task* task) {}

void Task::EndTurn() {}

bool Task::Task_vfunc16(UnitInfo& unit) { return false; }

bool Task::Task_vfunc17(UnitInfo& unit) { return false; }

void Task::RemoveSelf() {
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

bool Task::Task_vfunc19() { return false; }

void Task::Task_vfunc20(int unknown) {}

void Task::Remove(UnitInfo& unit) {}

void Task::Task_vfunc22(UnitInfo& unit) {}

void Task::Task_vfunc23(UnitInfo& unit) {}

void Task::Task_vfunc24(UnitInfo& unit1, UnitInfo& unit2) {}

void Task::Task_vfunc25(int unknown) {}

void Task::Task_vfunc26(UnitInfo& unit1, UnitInfo& unit2) {}

void Task::Task_vfunc27(Zone* zone, char mode) {}

TaskObtainUnits::TaskObtainUnits(Task* task, Point point)
    : Task(task->GetTeam(), this, task->GetFlags()), point(point), field_27(true), field_28(true) {}

TaskObtainUnits::~TaskObtainUnits() {}

unsigned short TaskObtainUnits::CountInstancesOfUnitType(ResourceID unit_type) {
    unsigned short count = 0;

    for (int i = 0; i < units->GetCount(); ++i) {
        if (*units[i] == unit_type) {
            ++count;
        }
    }

    return count;
}

bool TaskObtainUnits::TaskObtainUnits_sub_464C6(UnitInfo* unit, bool mode) {
    bool result;

    if (team != unit->team || unit->hits == 0) {
        result = false;
    } else if (unit->orders == ORDER_AWAIT && unit->orders != ORDER_SENTRY &&
               (unit->orders != ORDER_MOVE || unit->state != ORDER_STATE_1) &&
               (unit->orders != ORDER_MOVE_TO_UNIT || unit->state != ORDER_STATE_1) &&
               (unit->orders != ORDER_MOVE_TO_ATTACK || unit->state != ORDER_STATE_1)) {
        result = false;
    } else {
        Task* task = unit->GetTask1ListFront();

        if (task) {
            if (mode) {
                result = false;
            } else if (!task->Task_vfunc1(*unit)) {
                result = false;
            } else {
                result = task->Task_sub_42BC4(GetFlags()) > 0;
            }
        } else {
            result = true;
        }
    }

    return result;
}

UnitInfo* TaskObtainUnits::TaskObtainUnits_sub_465F4(ResourceID unit_type, bool mode) {
    SmartList<UnitInfo>* list;
    int speed = 0;
    int best_speed;
    UnitInfo* selected_unit = nullptr;
    bool is_unit_available = false;

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        list = &UnitsManager_StationaryUnits;
    } else if (UnitsManager_BaseUnits[unit_type].flags & MOBILE_AIR_UNIT) {
        list = &UnitsManager_MobileAirUnits;
    } else {
        list = &UnitsManager_MobileLandSeaUnits;
    }

    for (SmartList<UnitInfo>::Iterator unit = list->Begin(); unit != nullptr; ++unit) {
        if ((*unit).unit_type == unit_type) {
            if ((*unit).orders == ANIMATED &&
                ((*unit).flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT))) {
                speed = (*unit).build_time * (*unit).GetBaseValues()->GetAttribute(ATTRIB_SPEED) +
                        Taskmanager_sub_45F65(point.x - (*unit).grid_x, point.y - (*unit).grid_y);

                if (selected_unit == nullptr || (best_speed > speed)) {
                    is_unit_available = false;
                    selected_unit = &*unit;
                    best_speed = speed;
                }
            } else {
                if (TaskObtainUnits_sub_464C6(&*unit, mode)) {
                    speed = Taskmanager_sub_45F65(point.x - (*unit).grid_x, point.y - (*unit).grid_y);

                    if (selected_unit == nullptr || (best_speed > speed)) {
                        is_unit_available = true;
                        selected_unit = &*unit;
                        best_speed = speed;
                    }
                }
            }
        }
    }

    if (!is_unit_available) {
        selected_unit = nullptr;
    }

    return selected_unit;
}

int TaskObtainUnits::Task_vfunc4() const { return 2 * units.GetCount() + 4; }

unsigned short TaskObtainUnits::GetFlags() const {
    unsigned short flags;

    if (parent != nullptr) {
        flags = parent->GetFlags();
    } else {
        flags = 0x2900;
    }

    return flags;
}

char* TaskObtainUnits::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Obtain units: ");

    if (units->GetCount() == 0) {
        strcat(buffer, "(finished)");
    }

    for (int i = 0; i < units->GetCount() && i < 3; ++i) {
        strcat(buffer, UnitsManager_BaseUnits[*units[i]].singular_name);
        if ((i + 1) < units->GetCount() && i < 2) {
            strcat(buffer, ", ");
        }
    }

    if (units->GetCount() > 3) {
        strcat(buffer, "...");
    }

    return buffer;
}

unsigned char TaskObtainUnits::GetType() const { return TaskType_TaskObtainUnits; }

bool TaskObtainUnits::Task_vfunc9() {
    bool result;

    if (units->GetCount() && parent != nullptr) {
        result = parent->Task_vfunc9();
    } else {
        result = false;
    }

    return result;
}

void TaskObtainUnits::Task_vfunc11(UnitInfo& unit) {
    int index = units->Find(&unit.unit_type);

    if (CountInstancesOfUnitType(unit.unit_type)) {
        units->Remove(index);
        parent->Task_vfunc11(unit);

        if (units->GetCount() == 0) {
            if (parent != nullptr) {
                parent->Task_vfunc14(this);
            }

            parent = nullptr;
            TaskManager.RemoveTask(*this);
        }
    }
}

void TaskObtainUnits::AddReminder() {
    if (field_28) {
        RemindTurnEnd(true);

        field_27 = true;
        field_28 = false;
    }
}

void TaskObtainUnits::Task_vfunc13() {
    field_28 = true;
    EndTurn();
}

void TaskObtainUnits::EndTurn() {
    if (parent == nullptr || !parent->Task_vfunc9()) {
        units.Clear();
    }

    if (field_27) {
        for (int i = units.GetCount() - 1; i >= 0; --i) {
            UnitInfo* unit = TaskObtainUnits_sub_465F4(*units[i], false);

            if (unit) {
                Task* task;

                units.Remove(i);
                task = unit->GetTask1ListFront();

                if (task) {
                    unit->ClearTask1List();

                    if (unit->orders != ORDER_AWAIT) {
                        /// \todo Implement missing stuff
                        // sub_103053(unit, ORDER_AWAITING, 24);
                    }
                }

                parent->Task_vfunc11(*unit);
            }
        }

        bool IsUnitTypeRequested[UNIT_END];

        memset(IsUnitTypeRequested, false, sizeof(IsUnitTypeRequested));

        for (int i = 0; i < units.GetCount(); ++i) {
            if (UnitsManager_BaseUnits[*units[i]].flags & STATIONARY) {
                TaskManager.TaskManager_sub_44954(*units[i], team, point, this);
            } else if (*units[i] == CONSTRCT || *units[i] == ENGINEER) {
                TaskManager.TaskManager_sub_449D0(*units[i], team, point, this);
            } else if (!IsUnitTypeRequested[*units[i]]) {
                IsUnitTypeRequested[*units[i]] = true;

                TaskManager.TaskManager_sub_44A73(*units[i], team, CountInstancesOfUnitType(*units[i]), this, point);
            }
        }

        field_27 = false;
    }

    if (!units.GetCount()) {
        if (parent != nullptr) {
            parent->Task_vfunc14(this);
        }

        parent = nullptr;
        TaskManager.RemoveTask(*this);
    }
}

void TaskObtainUnits::RemoveSelf() {
    units.Clear();
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

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

TaskRendezvous::TaskRendezvous(UnitInfo* unit1, UnitInfo* unit2, Task* task,
                               void (TaskGetResource::*function)(int unknown, char mode))
    : Task(task->GetTeam(), task, task->GetFlags()), unit1(unit1), unit2(unit2), function(function) {}

TaskRendezvous::~TaskRendezvous() {}

int TaskRendezvous::Task_vfunc4() const { return 4; }

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

void TaskRendezvous::Task_vfunc13() { EndTurn(); }

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
