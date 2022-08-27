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

#include "task.hpp"

#include "inifile.hpp"
#include "reminders.hpp"
#include "task_manager.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"

unsigned short Task::task_id = 0;
unsigned short Task::task_count = 0;

static SmartList<UnitInfo>* Task_GetUnitList(ResourceID unit_type);

void Task_RemindMoveFinished(UnitInfo* unit, bool priority) {
    if (unit && (unit->GetField221() & 0x100) == 0) {
        TaskManager.AddReminder(new (std::nothrow) RemindMoveFinished(*unit), priority);
    }
}

bool Task_IsReadyToTakeOrders(UnitInfo* unit) {
    /// \todo
}

void Task_RemoveMovementTasks(UnitInfo* unit) {
    /// \todo
}

bool Task_ShouldReserveShot(UnitInfo* unit, Point site) {
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

Task* Task::GetParent() { return &*parent; }

void Task::SetParent(Task* task) { parent = task; }

void Task::SetFlags(unsigned short flags_) { flags = flags_; }

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

bool Task_RetreatIfNecessary(Task* task, UnitInfo* unit, int caution_level) {}

bool Task_sub_43671(Task* task, UnitInfo* unit, int caution_level) {
    bool result;

    if (task->GetField6() && Task_RetreatIfNecessary(task, unit, caution_level)) {
        task->SetField6(false);

        result = true;

    } else {
        result = false;
    }

    return result;
}

SmartList<UnitInfo>* Task_GetUnitList(ResourceID unit_type) {
    unsigned int flags = UnitsManager_BaseUnits[unit_type].flags;
    SmartList<UnitInfo>* list;

    if (flags & STATIONARY) {
        if (flags & GROUND_COVER) {
            list = &UnitsManager_GroundCoverUnits;

        } else {
            list = &UnitsManager_StationaryUnits;
        }

    } else if (flags & MOBILE_AIR_UNIT) {
        list = &UnitsManager_MobileAirUnits;

    } else {
        list = &UnitsManager_MobileLandSeaUnits;
    }

    return list;
}

int Task_GetReadyUnitsCount(unsigned short team, ResourceID unit_type) {
    SmartList<UnitInfo>* units = Task_GetUnitList(unit_type);
    int count = 0;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type && (*it).state != ORDER_STATE_BUILDING_READY) {
            ++count;
        }
    }

    return count;
}

bool Task::Task_vfunc1(UnitInfo& unit) {
    bool result = true;

    if (parent != nullptr && !parent->Task_vfunc1(unit)) {
        result = false;
    }

    return result;
}

bool Task::Task_vfunc2(UnitInfo& unit) { return false; }

int Task::GetCautionLevel(UnitInfo& unit) {
    int result;

    if (unit.base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
        ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        if (unit.shots) {
            result = CAUTION_LEVEL_AVOID_REACTION_FIRE;
        } else {
            result = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
        }
    } else if (unit.ammo) {
        result = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
    } else {
        result = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
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

void Task::BeginTurn() { EndTurn(); }

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

unsigned short Task::GetId() const { return id; }
