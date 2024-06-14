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

#include "ailog.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TaskObtainUnits::TaskObtainUnits(Task* task, Point point)
    : Task(task->GetTeam(), task, task->GetFlags()), point(point), init(true), reinit(true) {}

TaskObtainUnits::~TaskObtainUnits() {}

uint16_t TaskObtainUnits::CountInstancesOfUnitType(ResourceID unit_type) {
    uint16_t count = 0;

    for (int32_t i = 0; i < units->GetCount(); ++i) {
        if (*units[i] == unit_type) {
            ++count;
        }
    }

    return count;
}

bool TaskObtainUnits::IsValidCandidate(UnitInfo* unit, bool mode) {
    bool result;

    if (team != unit->team || unit->hits <= 0) {
        result = false;

    } else if (unit->orders != ORDER_AWAIT && unit->orders != ORDER_SENTRY &&
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

UnitInfo* TaskObtainUnits::FindUnit(ResourceID unit_type, bool mode) {
    SmartList<UnitInfo>* list;
    int32_t speed{0};
    int32_t best_speed{0};
    UnitInfo* selected_unit{nullptr};
    bool is_unit_available{false};

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        list = &UnitsManager_StationaryUnits;

    } else if (UnitsManager_BaseUnits[unit_type].flags & MOBILE_AIR_UNIT) {
        list = &UnitsManager_MobileAirUnits;

    } else {
        list = &UnitsManager_MobileLandSeaUnits;
    }

    AiLog log("Obtain Unit: Find %s ", UnitsManager_BaseUnits[unit_type].singular_name);

    for (SmartList<UnitInfo>::Iterator unit = list->Begin(); unit != list->End(); ++unit) {
        if ((*unit).GetUnitType() == unit_type) {
            if ((*unit).orders == ORDER_BUILD &&
                ((*unit).flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT))) {
                speed = (*unit).build_time * (*unit).GetBaseValues()->GetAttribute(ATTRIB_SPEED) +
                        TaskManager_GetDistance(point.x - (*unit).grid_x, point.y - (*unit).grid_y);

                if (selected_unit == nullptr || (best_speed > speed)) {
                    is_unit_available = false;
                    selected_unit = unit->Get();
                    best_speed = speed;
                }

            } else {
                if (IsValidCandidate(unit->Get(), mode)) {
                    speed = TaskManager_GetDistance(point.x - (*unit).grid_x, point.y - (*unit).grid_y);

                    if (selected_unit == nullptr || (best_speed > speed)) {
                        is_unit_available = true;
                        selected_unit = unit->Get();
                        best_speed = speed;
                    }
                }
            }
        }
    }

    if (selected_unit) {
        if (!is_unit_available) {
            log.Log("%s at [%i,%i] has %i turns left to build",
                    UnitsManager_BaseUnits[selected_unit->GetUnitType()].singular_name, selected_unit->grid_x + 1,
                    selected_unit->grid_y + 1, selected_unit->build_time);

            selected_unit = nullptr;

        } else {
            log.Log("found.");
        }

    } else {
        log.Log("not found.");
    }

    return selected_unit;
}

uint16_t TaskObtainUnits::GetFlags() const {
    uint16_t flags;

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

    for (int32_t i = 0; i < units->GetCount() && i < 3; ++i) {
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

uint8_t TaskObtainUnits::GetType() const { return TaskType_TaskObtainUnits; }

bool TaskObtainUnits::IsNeeded() {
    bool result;

    if (units->GetCount() > 0 && parent != nullptr) {
        result = parent->IsNeeded();
    } else {
        result = false;
    }

    return result;
}

void TaskObtainUnits::AddUnit(UnitInfo& unit) {
    auto unit_type{unit.GetUnitType()};

    int32_t index = units->Find(&unit_type);

    AiLog log("Obtain Units: Add %s %i.", UnitsManager_BaseUnits[unit.GetUnitType()].singular_name, unit.unit_id);

    if (CountInstancesOfUnitType(unit.GetUnitType())) {
        units->Remove(index);
        parent->AddUnit(unit);

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
    if (reinit) {
        RemindTurnEnd(true);

        init = true;
        reinit = false;
    }
}

void TaskObtainUnits::BeginTurn() {
    AiLog log("Obtain Unit: Begin Turn");

    reinit = true;
    EndTurn();
}

void TaskObtainUnits::EndTurn() {
    if (parent == nullptr || !parent->IsNeeded()) {
        units.Clear();
    }

    if (init) {
        for (int32_t i = units.GetCount() - 1; i >= 0; --i) {
            UnitInfo* unit = FindUnit(*units[i], false);

            if (unit) {
                Task* task;

                units.Remove(i);
                task = unit->GetTask();

                if (task) {
                    unit->RemoveTasks();

                    if (unit->orders != ORDER_AWAIT) {
                        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_CLEAR_PATH);
                    }
                }

                parent->AddUnit(*unit);
            }
        }

        bool IsUnitTypeRequested[UNIT_END];

        memset(IsUnitTypeRequested, false, sizeof(IsUnitTypeRequested));

        for (int32_t i = 0; i < units.GetCount(); ++i) {
            if (UnitsManager_BaseUnits[*units[i]].flags & STATIONARY) {
                TaskManager.CreateBuilding(*units[i], team, point, this);
            } else if (*units[i] == CONSTRCT || *units[i] == ENGINEER) {
                TaskManager.CreateUnit(*units[i], team, point, this);
            } else if (!IsUnitTypeRequested[*units[i]]) {
                IsUnitTypeRequested[*units[i]] = true;

                TaskManager.ManufactureUnits(*units[i], team, CountInstancesOfUnitType(*units[i]), this, point);
            }
        }

        init = false;
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
    AiLog log("Obtain Unit: Parent Complete");

    units.Clear();
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskObtainUnits::AddUnit(ResourceID unit_type) {
    SDL_assert(unit_type != INVALID_ID);

    units.PushBack(&unit_type);
}
