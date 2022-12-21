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

#include "taskrepair.hpp"

#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "pathrequest.hpp"
#include "task_manager.hpp"
#include "taskactivate.hpp"
#include "taskcreateunit.hpp"
#include "taskgetmaterials.hpp"
#include "taskmovehome.hpp"
#include "taskrendezvous.hpp"
#include "units_manager.hpp"

static void TaskRepair_RendesvousResultCallback();

void TaskRepair_RendesvousResultCallback() {}

void TaskRepair::ChooseUnitToRepair() {
    SmartPointer<UnitInfo> unit;

    if (target_unit != nullptr) {
        operator_unit = nullptr;

        unit = SelectRepairShop();

        SelectOperator();

        if (unit != nullptr) {
            if (operator_unit != nullptr) {
                int distance1;
                int distance2;

                distance1 = TaskManager_GetDistance(&*unit, &*target_unit);
                distance2 = TaskManager_GetDistance(&*operator_unit, &*target_unit);

                if (distance2 / 2 > distance1) {
                    operator_unit = unit;
                }

            } else {
                operator_unit = unit;
            }
        }

        if (operator_unit != nullptr) {
            operator_unit->PushFrontTask1List(this);
        }
    }
}

void TaskRepair::DoRepairs() {
    if (target_unit->GetTask() == this ||
        !(operator_unit->flags & (MOBILE_AIR_UNIT || MOBILE_SEA_UNIT || MOBILE_LAND_UNIT))) {
        if (GameManager_PlayMode != PLAY_MODE_UNKNOWN &&
            (GameManager_PlayMode != PLAY_MODE_TURN_BASED || team == GameManager_ActiveTurnTeam)) {
            if (operator_unit->flags & STATIONARY) {
                Cargo materials;
                Cargo capacity;

                operator_unit->GetComplex()->GetCargoInfo(materials, capacity);

                if (materials.raw > 0) {
                    if (Task_IsReadyToTakeOrders(&*operator_unit)) {
                        IssueOrder();
                    }
                }

            } else if (operator_unit->storage > 0) {
                if (Task_IsReadyToTakeOrders(&*operator_unit)) {
                    IssueOrder();
                }

            } else if (operator_unit->GetTask() == this) {
                SmartPointer<Task> task(new (std::nothrow)
                                            TaskGetMaterials(this, &*operator_unit, GetTurnsToComplete()));

                TaskManager.AppendTask(*task);
            }
        }
    }
}

UnitInfo* TaskRepair::SelectRepairShop() {
    UnitInfo* result;
    ResourceID repair_shop;

    result = nullptr;

    repair_shop = GetRepairShopType();

    if (repair_shop != INVALID_ID) {
        int distance;
        int shortest_distance;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == team && (*it).unit_type == repair_shop) {
                distance = TaskManager_GetDistance(&*it, &*target_unit);

                if (result == nullptr || distance < shortest_distance) {
                    result = &*it;
                    shortest_distance = distance;
                }
            }
        }
    }

    return result;
}

void TaskRepair::RemoveMovementTasks() {
    if (operator_unit != nullptr) {
        operator_unit->RemoveTask(this);
        Task_RemoveMovementTasks(&*operator_unit);
        operator_unit = nullptr;
    }
}

void TaskRepair::RendezvousResultCallback(Task* task, UnitInfo* unit, char result) {
    if (result == 2) {
        dynamic_cast<TaskRepair*>(task)->RemoveMovementTasks();

    } else if (result == 0) {
        dynamic_cast<TaskRepair*>(task)->EndTurn();
    }
}

ResourceID TaskRepair::GetRepairShopType() {
    ResourceID result;

    if (target_unit->unit_type == COMMANDO || target_unit->unit_type == INFANTRY) {
        result = BARRACKS;

    } else if (target_unit->flags & MOBILE_LAND_UNIT) {
        result = DEPOT;
    }

    else if (target_unit->flags & MOBILE_AIR_UNIT) {
        result = HANGAR;

    } else if (target_unit->flags & MOBILE_SEA_UNIT) {
        result = DOCK;

    } else {
        result = INVALID_ID;
    }

    return result;
}

void TaskRepair::CreateUnitIfNeeded(ResourceID unit_type) {
    SmartList<UnitInfo>* unit_list;

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        unit_list = &UnitsManager_StationaryUnits;

    } else {
        unit_list = &UnitsManager_MobileLandSeaUnits;
    }

    for (SmartList<UnitInfo>::Iterator it = unit_list->Begin(); it != unit_list->End(); ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type) {
            return;
        }
    }

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
         ++it) {
        if ((*it).GetTeam() == team &&
            ((*it).GetType() == TaskType_TaskCreateBuilding || (*it).GetType() == TaskType_TaskCreateUnit)) {
            if (dynamic_cast<TaskCreate*>(&*it)->GetUnitType() == unit_type) {
                return;
            }
        }
    }

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        AiPlayer_Teams[team].CreateBuilding(unit_type, Point(target_unit->grid_x, target_unit->grid_y), this);

    } else {
        SmartPointer<Task> task(new (std::nothrow)
                                    TaskCreateUnit(unit_type, this, Point(target_unit->grid_x, target_unit->grid_y)));

        TaskManager.AppendTask(*task);
    }
}

TaskRepair::TaskRepair(UnitInfo* unit) : Task(unit->team, nullptr, 0x1100), target_unit(unit) {}

TaskRepair::~TaskRepair() {}

bool TaskRepair::Task_vfunc1(UnitInfo& unit) { return target_unit != unit; }

int TaskRepair::GetMemoryUse() const { return 4; }

char* TaskRepair::WriteStatusLog(char* buffer) const {
    if (target_unit != nullptr) {
        strcpy(buffer, "Repair ");
        strcat(buffer, UnitsManager_BaseUnits[target_unit->unit_type].singular_name);

    } else {
        strcpy(buffer, "Finished repair task.");
    }

    return buffer;
}

Rect* TaskRepair::GetBounds(Rect* bounds) {
    target_unit->GetBounds(bounds);

    return bounds;
}

unsigned char TaskRepair::GetType() const { return TaskType_TaskRepair; }

void TaskRepair::Begin() {
    target_unit->PushFrontTask1List(this);
    CreateUnit();
    ChooseUnitToRepair();
    Task_RemindMoveFinished(&*target_unit);
}

void TaskRepair::BeginTurn() { EndTurn(); }

void TaskRepair::EndTurn() {
    if (target_unit != nullptr && target_unit->GetTask() == this) {
        Task_vfunc17(*target_unit);
    }
}

bool TaskRepair::Task_vfunc17(UnitInfo& unit) {
    bool result;

    if (target_unit == unit && target_unit->GetTask() == this) {
        if (IsInPerfectCondition()) {
            SmartPointer<Task> task;

            if (target_unit->state == ORDER_STATE_3) {
                task = new (std::nothrow) TaskActivate(&*target_unit, this, target_unit->GetParent());

                TaskManager.AppendTask(*task);

            } else {
                task = this;

                RemoveMovementTasks();

                target_unit->RemoveTask(this);
                target_unit = nullptr;

                TaskManager.RemoveTask(*this);
            }

            result = true;

        } else {
            if (operator_unit == nullptr) {
                ChooseUnitToRepair();
            }

            if (operator_unit == nullptr) {
                if (target_unit->speed > 0 && target_unit->IsReadyForOrders(this)) {
                    SmartPointer<Task> task(new (std::nothrow) TaskMoveHome(&*target_unit, this));

                    TaskManager.AppendTask(*task);

                    result = true;

                } else {
                    result = false;
                }

            } else {
                if (target_unit->state == ORDER_STATE_3) {
                    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || team == GameManager_ActiveTurnTeam) {
                        DoRepairs();
                    }

                    result = true;

                } else {
                    if ((operator_unit->flags & STATIONARY) || operator_unit->storage > 0 ||
                        operator_unit->GetTask() != this) {
                        if (Task_IsAdjacent(&*operator_unit, target_unit->grid_x, target_unit->grid_y)) {
                            if (GameManager_PlayMode != PLAY_MODE_UNKNOWN &&
                                (GameManager_PlayMode != PLAY_MODE_TURN_BASED || team == GameManager_ActiveTurnTeam)) {
                                if (operator_unit->flags & BUILDING) {
                                    target_unit->target_grid_x = operator_unit->grid_x;
                                    target_unit->target_grid_y = operator_unit->grid_y;

                                    if (target_unit->target_grid_x == target_unit->grid_x &&
                                        target_unit->target_grid_y == target_unit->grid_y) {
                                        ++target_unit->target_grid_x;
                                    }

                                    UnitsManager_SetNewOrder(&*target_unit, ORDER_MOVE_TO_UNIT, ORDER_STATE_0);

                                } else {
                                    DoRepairs();
                                }

                                result = true;

                            } else {
                                result = false;
                            }

                        } else if (!Task_RetreatFromDanger(this, &*target_unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                            SmartPointer<Task> task(new (std::nothrow) TaskRendezvous(
                                &*target_unit, &*operator_unit, this, &TaskRepair::RendezvousResultCallback));

                            TaskManager.AppendTask(*task);
                        }

                        result = true;

                    } else {
                        SmartPointer<Task> task(new (std::nothrow)
                                                    TaskGetMaterials(this, &*operator_unit, GetTurnsToComplete()));

                        TaskManager.AppendTask(*task);

                        result = true;
                    }
                }
            }
        }

    } else {
        result = false;
    }

    return result;
}

void TaskRepair::RemoveSelf() {
    if (target_unit != nullptr) {
        target_unit->RemoveTask(this);
    }

    if (operator_unit != nullptr) {
        operator_unit->RemoveTask(this);
    }

    target_unit = nullptr;
    operator_unit = nullptr;

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskRepair::RemoveUnit(UnitInfo& unit) {
    if (target_unit == unit) {
        SmartPointer<Task> task(this);

        RemoveMovementTasks();

        target_unit = nullptr;

        TaskManager.RemoveTask(*this);

    } else if (operator_unit == unit) {
        operator_unit = nullptr;
    }
}

void TaskRepair::SelectOperator() {
    if (GetRepairShopType() == DEPOT) {
        UnitInfo* unit;
        int distance;
        int shortest_distance;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == team && (*it).unit_type == REPAIR && (*it).hits > 0 &&
                ((*it).orders == ORDER_AWAIT || ((*it).orders == ORDER_MOVE && (*it).speed == 0)) &&
                target_unit != (*it)) {
                if ((*it).GetTask() == nullptr || (*it).GetTask()->DeterminePriority(flags) > 0) {
                    distance = TaskManager_GetDistance(&*it, &*target_unit);

                    if (unit == nullptr || distance < shortest_distance) {
                        unit = &*it;
                        shortest_distance = distance;
                    }
                }
            }
        }

        operator_unit = unit;
    }
}

int TaskRepair::GetTurnsToComplete() {
    int result;

    if (target_unit->hits < target_unit->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
        result = target_unit->GetTurnsToRepair();
    } else {
        result = 1;
    }

    return result;
}

bool TaskRepair::IsInPerfectCondition() {
    bool result;

    if (target_unit->hits < target_unit->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
        result = false;

    } else if (target_unit->state != ORDER_STATE_3 ||
               target_unit->ammo >= target_unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

void TaskRepair::CreateUnit() {
    ResourceID unit_type;

    unit_type = GetRepairShopType();

    if (unit_type != INVALID_ID) {
        CreateUnitIfNeeded(unit_type);
    }

    if (unit_type == DEPOT || unit_type == INVALID_ID) {
        CreateUnitIfNeeded(REPAIR);
    }
}

void TaskRepair::IssueOrder() {
    operator_unit->SetParent(&*target_unit);

    if (target_unit->hits < target_unit->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
        UnitsManager_SetNewOrder(&*operator_unit, ORDER_REPAIR, ORDER_STATE_0);

    } else if (target_unit->ammo < target_unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO) &&
               target_unit->state == ORDER_STATE_3) {
        UnitsManager_SetNewOrder(&*operator_unit, ORDER_RELOAD, ORDER_STATE_0);
    }
}
