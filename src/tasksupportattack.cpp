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

#include "tasksupportattack.hpp"

#include "aiattack.hpp"
#include "ailog.hpp"
#include "task_manager.hpp"
#include "taskattack.hpp"
#include "taskgetmaterials.hpp"
#include "taskmove.hpp"
#include "taskrepair.hpp"
#include "units_manager.hpp"

void TaskSupportAttack::ObtainUnits(uint32_t unit_flags_) {
    bool needs_supply_unit = true;
    bool needs_repair_unit = true;
    int32_t highest_scan;

    AiLog log("Support attack: get units");

    unit_flags = unit_flags_;

    if (unit_flags & MOBILE_LAND_UNIT) {
        needs_supply_unit = false;
        needs_repair_unit = false;
    }

    if (unit_flags & MOBILE_SEA_UNIT) {
        needs_supply_unit = false;
    }

    highest_scan = dynamic_cast<TaskAttack*>(&*parent)->GetHighestScan();

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (((*it).flags & unit_flags) && AiAttack_GetTargetValue(&*it) <= highest_scan) {
            if ((*it).unit_type == CARGOSHP || (*it).unit_type == SPLYTRCK || (*it).unit_type == REPAIR) {
                if ((*it).IsReadyForOrders(this) && (*it).storage < 5) {
                    SmartPointer<Task> get_materials_task(new (std::nothrow) TaskGetMaterials(this, &*it, 5));

                    TaskManager.AppendTask(*get_materials_task);
                }

                if ((*it).unit_type == REPAIR) {
                    needs_repair_unit = true;

                } else {
                    needs_supply_unit = true;
                }
            }

        } else {
            TaskManager.RemindAvailable(&*it);
        }
    }

    if ((!needs_supply_unit && unit_type1 == INVALID_ID) || (!needs_repair_unit && unit_type2 == INVALID_ID)) {
        SmartPointer<TaskObtainUnits> obtain_units_task(new (std::nothrow) TaskObtainUnits(this, DeterminePosition()));

        if (!needs_supply_unit && unit_type1 == INVALID_ID) {
            if (unit_flags & MOBILE_SEA_UNIT) {
                unit_type1 = CARGOSHP;

            } else {
                unit_type1 = SPLYTRCK;
            }

            obtain_units_task->AddUnit(unit_type1);
        }

        if (!needs_repair_unit && unit_type2 == INVALID_ID && (unit_flags & MOBILE_LAND_UNIT)) {
            unit_type2 = REPAIR;

            obtain_units_task->AddUnit(unit_type2);
        }

        TaskManager.AppendTask(*obtain_units_task);
    }
}

bool TaskSupportAttack::IssueOrders(UnitInfo* unit) {
    bool result;

    if (unit->IsReadyForOrders(this) && parent && unit->speed > 0) {
        AiLog log("Support attack: give orders to %s.", UnitsManager_BaseUnits[unit->unit_type].singular_name);

        if (unit->ammo >= unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS)) {
            if (unit->hits < unit->GetBaseValues()->GetAttribute(ATTRIB_HITS) / 4) {
                unit->RemoveTasks();

                SmartPointer<Task> repair_task(new (std::nothrow) TaskRepair(unit));

                TaskManager.AppendTask(*repair_task);

                result = true;

            } else {
                result = dynamic_cast<TaskAttack*>(&*parent)->MoveCombatUnit(this, unit);
            }

        } else {
            log.Log("Removing unit, low on ammunition.");

            TaskManager.RemindAvailable(unit);

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskSupportAttack::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    if (result != TASKMOVE_RESULT_SUCCESS || !AiAttack_EvaluateAssault(unit, task, &MoveFinishedCallback)) {
        dynamic_cast<TaskSupportAttack*>(task)->IssueOrders(unit);
    }
}

TaskSupportAttack::TaskSupportAttack(Task* task) : Task(task->GetTeam(), task, 0x2500) {
    unit_flags = 0;
    unit_type1 = INVALID_ID;
    unit_type2 = INVALID_ID;
}

TaskSupportAttack::~TaskSupportAttack() {}

bool TaskSupportAttack::IsUnitUsable(UnitInfo& unit) { return false; }

char* TaskSupportAttack::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Support attack.");

    return buffer;
}

Rect* TaskSupportAttack::GetBounds(Rect* bounds) {
    Rect* result;

    if (parent) {
        result = parent->GetBounds(bounds);

    } else {
        result = Task::GetBounds(bounds);
    }

    return result;
}

uint8_t TaskSupportAttack::GetType() const { return TaskType_TaskSupportAttack; }

bool TaskSupportAttack::IsNeeded() { return parent != nullptr; }

void TaskSupportAttack::AddUnit(UnitInfo& unit) {
    if (parent) {
        unit.AddTask(this);
        units.PushBack(unit);
        unit.attack_site.x = 0;
        unit.attack_site.y = 0;

        if (unit.unit_type == CARGOSHP || unit.unit_type == SPLYTRCK) {
            unit_type1 = INVALID_ID;
        }

        if (unit.unit_type == REPAIR) {
            unit_type2 = INVALID_ID;
        }

        if ((unit.unit_type == CARGOSHP || unit.unit_type == SPLYTRCK || unit.unit_type == REPAIR) &&
            unit.storage < 5) {
            SmartPointer<Task> get_materials_task(new (std::nothrow) TaskGetMaterials(this, &unit, 5));

            TaskManager.AppendTask(*get_materials_task);
        }

    } else {
        TaskManager.RemindAvailable(&unit);
    }
}

void TaskSupportAttack::BeginTurn() {
    if (parent) {
        ObtainUnits(dynamic_cast<TaskAttack*>(&*parent)->GetAccessFlags());

        if (!IsScheduledForTurnEnd()) {
            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
        }
    }
}

void TaskSupportAttack::EndTurn() { AddReminders(); }

bool TaskSupportAttack::Execute(UnitInfo& unit) {
    bool result;

    AiLog log("Support attack: think about moving %s", UnitsManager_BaseUnits[unit.unit_type].singular_name);

    if (parent) {
        if (unit.IsReadyForOrders(this)) {
            if (AiAttack_EvaluateAssault(&unit, this, &MoveFinishedCallback)) {
                if (AiAttack_EvaluateAttack(&unit)) {
                    result = true;

                } else {
                    result = true;
                }

            } else {
                result = IssueOrders(&unit);
            }

        } else {
            result = false;
        }

    } else {
        RemoveUnit(unit);
        unit.RemoveTask(this);

        result = false;
    }

    return result;
}

void TaskSupportAttack::RemoveSelf() {
    parent = nullptr;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units.Clear();

    TaskManager.RemoveTask(*this);
}

void TaskSupportAttack::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }

bool TaskSupportAttack::AddReminders() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (AiAttack_IsReadyToMove(&*it)) {
            return true;

        } else {
            if ((*it).IsReadyForOrders(this) && (*it).speed > 0) {
                Task_RemindMoveFinished(&*it);
            }
        }
    }

    return false;
}

SmartList<UnitInfo>& TaskSupportAttack::GetUnits() { return units; }
