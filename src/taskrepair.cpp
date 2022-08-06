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

#include "ai_player.hpp"
#include "game_manager.hpp"
#include "pathrequest.hpp"
#include "task_manager.hpp"
#include "taskactivate.hpp"
#include "taskcreateunit.hpp"
#include "taskgetmaterials.hpp"
#include "taskmovehome.hpp"
#include "taskrendesvous.hpp"
#include "units_manager.hpp"

static void TaskRepair_RendesvousResultCallback();

void TaskRepair_RendesvousResultCallback() {}

void TaskRepair::ChooseUnitToRepair() {
    SmartPointer<UnitInfo> unit;

    if (unit_to_repair != nullptr) {
        repair_unit = nullptr;

        unit = SelectRepairShop();

        SelectRepairUnit();

        if (unit != nullptr) {
            if (repair_unit != nullptr) {
                int distance1;
                int distance2;

                distance1 = TaskManager_sub_4601A(&*unit, &*unit_to_repair);
                distance2 = TaskManager_sub_4601A(&*repair_unit, &*unit_to_repair);

                if (distance2 / 2 > distance1) {
                    repair_unit = unit;
                }

            } else {
                repair_unit = unit;
            }
        }

        if (repair_unit != nullptr) {
            repair_unit->PushFrontTask1List(this);
        }
    }
}

void TaskRepair::DoRepairs() {
    if (unit_to_repair->GetTask1ListFront() == this ||
        !(repair_unit->flags & (MOBILE_AIR_UNIT || MOBILE_SEA_UNIT || MOBILE_LAND_UNIT))) {
        if (GameManager_PlayMode != PLAY_MODE_UNKNOWN &&
            (GameManager_PlayMode != PLAY_MODE_TURN_BASED || team == GameManager_ActiveTurnTeam)) {
            if (repair_unit->flags & STATIONARY) {
                Cargo materials;
                Cargo capacity;

                repair_unit->GetComplex()->GetCargoInfo(materials, capacity);

                if (materials.raw > 0) {
                    if (Task_IsReadyToTakeOrders(&*repair_unit)) {
                        IssueOrder();
                    }
                }

            } else if (repair_unit->storage > 0) {
                if (Task_IsReadyToTakeOrders(&*repair_unit)) {
                    IssueOrder();
                }

            } else if (repair_unit->GetTask1ListFront() == this) {
                SmartPointer<Task> task(new (std::nothrow) TaskGetMaterials(this, &*repair_unit, GetTurnsToRepair()));

                TaskManager.AddTask(*task);
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
                distance = TaskManager_sub_4601A(&*it, &*unit_to_repair);

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
    if (repair_unit != nullptr) {
        repair_unit->RemoveTask(this);
        Task_RemoveMovementTasks(&*repair_unit);
        repair_unit = nullptr;
    }
}

void TaskRepair::RendesvousResultCallback(Task* task, int unknown, char result) {
    if (result == 2) {
        dynamic_cast<TaskRepair*>(task)->RemoveMovementTasks();

    } else if (result == 0) {
        dynamic_cast<TaskRepair*>(task)->EndTurn();
    }
}

ResourceID TaskRepair::GetRepairShopType() {
    ResourceID result;

    if (unit_to_repair->unit_type == COMMANDO || unit_to_repair->unit_type == INFANTRY) {
        result = BARRACKS;

    } else if (unit_to_repair->flags & MOBILE_LAND_UNIT) {
        result = DEPOT;
    }

    else if (unit_to_repair->flags & MOBILE_AIR_UNIT) {
        result = HANGAR;

    } else if (unit_to_repair->flags & MOBILE_SEA_UNIT) {
        result = DOCK;

    } else {
        result = INVALID_ID;
    }

    return result;
}

void TaskRepair::CreateUnit(ResourceID unit_type) {
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

    for (SmartList<Task>::Iterator it = TaskManager.tasklist.Begin(); it != TaskManager.tasklist.End(); ++it) {
        if ((*it).GetTeam() == team &&
            ((*it).GetType() == TaskType_TaskCreateBuilding || (*it).GetType() == TaskType_TaskCreateUnit)) {
            if (dynamic_cast<TaskCreate*>(&*it)->GetUnitType() == unit_type) {
                return;
            }
        }
    }

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        AiPlayer_Teams[team].CreateBuilding(unit_type, Point(unit_to_repair->grid_x, unit_to_repair->grid_y), this);

    } else {
        SmartPointer<Task> task(
            new (std::nothrow) TaskCreateUnit(unit_type, this, Point(unit_to_repair->grid_x, unit_to_repair->grid_y)));

        TaskManager.AddTask(*task);
    }
}

TaskRepair::TaskRepair(UnitInfo* unit) : Task(unit->team, nullptr, 0x1100), unit_to_repair(unit) {}

TaskRepair::~TaskRepair() {}

bool TaskRepair::Task_vfunc1(UnitInfo& unit) { return unit_to_repair != unit; }

int TaskRepair::GetMemoryUse() const { return 4; }

char* TaskRepair::WriteStatusLog(char* buffer) const {
    if (unit_to_repair != nullptr) {
        strcpy(buffer, "Repair ");
        strcat(buffer, UnitsManager_BaseUnits[unit_to_repair->unit_type].singular_name);

    } else {
        strcpy(buffer, "Finished repair task.");
    }

    return buffer;
}

Rect* TaskRepair::GetBounds(Rect* bounds) {
    unit_to_repair->GetBounds(bounds);

    return bounds;
}

unsigned char TaskRepair::GetType() const { return TaskType_TaskRepair; }

void TaskRepair::AddReminder() {
    unit_to_repair->PushFrontTask1List(this);
    TaskRepair_vfunc31();
    ChooseUnitToRepair();
    Task_RemindMoveFinished(&*unit_to_repair);
}

void TaskRepair::Execute() { EndTurn(); }

void TaskRepair::EndTurn() {
    if (unit_to_repair != nullptr && unit_to_repair->GetTask1ListFront() == this) {
        Task_vfunc17(*unit_to_repair);
    }
}

bool TaskRepair::Task_vfunc17(UnitInfo& unit) {
    bool result;

    if (unit_to_repair == unit && unit_to_repair->GetTask1ListFront() == this) {
        if (IsInPerfectCondition()) {
            SmartPointer<Task> task;

            if (unit_to_repair->state == ORDER_STATE_3) {
                task = new (std::nothrow) TaskActivate(&*unit_to_repair, this, unit_to_repair->GetParent());

                TaskManager.AddTask(*task);

            } else {
                task = this;

                RemoveMovementTasks();

                unit_to_repair->RemoveTask(this);
                unit_to_repair = nullptr;

                TaskManager.RemoveTask(*this);
            }

            result = true;

        } else {
            if (repair_unit == nullptr) {
                ChooseUnitToRepair();
            }

            if (repair_unit == nullptr) {
                if (unit_to_repair->speed > 0 && unit_to_repair->IsReadyForOrders(this)) {
                    SmartPointer<Task> task(new (std::nothrow) TaskMoveHome(&*unit_to_repair, this));

                    TaskManager.AddTask(*task);

                    result = true;

                } else {
                    result = false;
                }

            } else {
                if (unit_to_repair->state == ORDER_STATE_3) {
                    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || team == GameManager_ActiveTurnTeam) {
                        DoRepairs();
                    }

                    result = true;

                } else {
                    if ((repair_unit->flags & STATIONARY) || repair_unit->storage > 0 ||
                        repair_unit->GetTask1ListFront() != this) {
                        if (repair_unit->IsAdjacent(unit_to_repair->grid_x, unit_to_repair->grid_y)) {
                            if (GameManager_PlayMode != PLAY_MODE_UNKNOWN &&
                                (GameManager_PlayMode != PLAY_MODE_TURN_BASED || team == GameManager_ActiveTurnTeam)) {
                                if (repair_unit->flags & BUILDING) {
                                    unit_to_repair->target_grid_x = repair_unit->grid_x;
                                    unit_to_repair->target_grid_y = repair_unit->grid_y;

                                    if (unit_to_repair->target_grid_x == unit_to_repair->grid_x &&
                                        unit_to_repair->target_grid_y == unit_to_repair->grid_y) {
                                        ++unit_to_repair->target_grid_x;
                                    }

                                    UnitsManager_SetNewOrder(&*unit_to_repair, ORDER_MOVE_TO_UNIT, ORDER_STATE_0);

                                } else {
                                    DoRepairs();
                                }

                                result = true;

                            } else {
                                result = false;
                            }

                        } else if (!Task_sub_43671(this, &*unit_to_repair, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                            SmartPointer<Task> task(new (std::nothrow) TaskRendezvous(
                                &*unit_to_repair, &*repair_unit, this, &TaskRepair::RendesvousResultCallback));

                            TaskManager.AddTask(*task);
                        }

                        result = true;

                    } else {
                        SmartPointer<Task> task(new (std::nothrow)
                                                    TaskGetMaterials(this, &*repair_unit, GetTurnsToRepair()));

                        TaskManager.AddTask(*task);

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
    if (unit_to_repair != nullptr) {
        unit_to_repair->RemoveTask(this);
    }

    if (repair_unit != nullptr) {
        repair_unit->RemoveTask(this);
    }

    unit_to_repair = nullptr;
    repair_unit = nullptr;

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskRepair::Remove(UnitInfo& unit) {
    if (unit_to_repair == unit) {
        SmartPointer<Task> task(this);

        RemoveMovementTasks();

        unit_to_repair = nullptr;

        TaskManager.RemoveTask(*this);

    } else if (repair_unit == unit) {
        repair_unit = nullptr;
    }
}

void TaskRepair::SelectRepairUnit() {
    if (GetRepairShopType() == DEPOT) {
        UnitInfo* unit;
        int distance;
        int shortest_distance;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == team && (*it).unit_type == REPAIR && (*it).hits > 0 &&
                ((*it).orders == ORDER_AWAIT || ((*it).orders == ORDER_MOVE && (*it).speed == 0)) &&
                unit_to_repair != (*it)) {
                if ((*it).GetTask1ListFront() == nullptr || (*it).GetTask1ListFront()->Task_sub_42BC4(flags) > 0) {
                    distance = TaskManager_sub_4601A(&*it, &*unit_to_repair);

                    if (unit == nullptr || distance < shortest_distance) {
                        unit = &*it;
                        shortest_distance = distance;
                    }
                }
            }
        }

        repair_unit = unit;
    }
}

int TaskRepair::GetTurnsToRepair() {
    int result;

    if (unit_to_repair->hits < unit_to_repair->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
        result = unit_to_repair->GetTurnsToRepair();
    } else {
        result = 1;
    }

    return result;
}

bool TaskRepair::IsInPerfectCondition() {
    bool result;

    if (unit_to_repair->hits < unit_to_repair->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
        result = false;

    } else if (unit_to_repair->state != ORDER_STATE_3 ||
               unit_to_repair->ammo >= unit_to_repair->GetBaseValues()->GetAttribute(ATTRIB_AMMO)) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

void TaskRepair::TaskRepair_vfunc31() {
    ResourceID unit_type;

    unit_type = GetRepairShopType();

    if (unit_type != INVALID_ID) {
        CreateUnit(unit_type);
    }

    if (unit_type == DEPOT || unit_type == INVALID_ID) {
        CreateUnit(REPAIR);
    }
}

void TaskRepair::IssueOrder() {
    repair_unit->SetParent(&*unit_to_repair);

    if (unit_to_repair->hits < unit_to_repair->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
        UnitsManager_SetNewOrder(&*repair_unit, ORDER_REPAIR, ORDER_STATE_0);

    } else if (unit_to_repair->ammo < unit_to_repair->GetBaseValues()->GetAttribute(ATTRIB_AMMO) &&
               unit_to_repair->state == ORDER_STATE_3) {
        UnitsManager_SetNewOrder(&*repair_unit, ORDER_RELOAD, ORDER_STATE_0);
    }
}
