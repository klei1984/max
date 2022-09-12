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

#include "taskobtainunits.hpp"

#include "task_manager.hpp"
#include "units_manager.hpp"

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
        Task* task = unit->GetTask();

        if (task) {
            if (mode) {
                result = false;
            } else if (!task->Task_vfunc1(*unit)) {
                result = false;
            } else {
                result = task->DeterminePriority(GetFlags()) > 0;
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
                        TaskManager_GetDistance(point.x - (*unit).grid_x, point.y - (*unit).grid_y);

                if (selected_unit == nullptr || (best_speed > speed)) {
                    is_unit_available = false;
                    selected_unit = &*unit;
                    best_speed = speed;
                }
            } else {
                if (TaskObtainUnits_sub_464C6(&*unit, mode)) {
                    speed = TaskManager_GetDistance(point.x - (*unit).grid_x, point.y - (*unit).grid_y);

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

int TaskObtainUnits::GetMemoryUse() const { return 2 * units.GetCount() + 4; }

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
                parent->ChildComplete(this);
            }

            parent = nullptr;
            TaskManager.RemoveTask(*this);
        }
    }
}

void TaskObtainUnits::Begin() {
    if (field_28) {
        RemindTurnEnd(true);

        field_27 = true;
        field_28 = false;
    }
}

void TaskObtainUnits::BeginTurn() {
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
                task = unit->GetTask();

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
                TaskManager.CreateBuilding(*units[i], team, point, this);
            } else if (*units[i] == CONSTRCT || *units[i] == ENGINEER) {
                TaskManager.CreateUnit(*units[i], team, point, this);
            } else if (!IsUnitTypeRequested[*units[i]]) {
                IsUnitTypeRequested[*units[i]] = true;

                TaskManager.ManufactureUnits(*units[i], team, CountInstancesOfUnitType(*units[i]), this, point);
            }
        }

        field_27 = false;
    }

    if (!units.GetCount()) {
        if (parent != nullptr) {
            parent->ChildComplete(this);
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

void TaskObtainUnits::AddUnit(ResourceID unit_type) { units.PushBack(&unit_type); }
