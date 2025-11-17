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

#include <format>

#include "access.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "pathrequest.hpp"
#include "task_manager.hpp"
#include "taskactivate.hpp"
#include "taskcreateunit.hpp"
#include "taskgetmaterials.hpp"
#include "taskmove.hpp"
#include "taskmovehome.hpp"
#include "taskrendezvous.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

static void TaskRepair_RendesvousResultCallback();

void TaskRepair_RendesvousResultCallback() {}

void TaskRepair::ChooseOperator() {
    SmartPointer<UnitInfo> unit;

    if (target_unit != nullptr) {
        operator_unit = nullptr;

        AILOG(log, "Choose unit to repair {}.",
              ResourceManager_GetUnit(target_unit->GetUnitType()).GetSingularName().data());

        unit = SelectRepairShop();

        SelectOperator();

        if (unit != nullptr) {
            if (operator_unit != nullptr) {
                int32_t distance1;
                int32_t distance2;

                distance1 = Access_GetApproximateDistance(&*unit, &*target_unit);
                distance2 = Access_GetApproximateDistance(&*operator_unit, &*target_unit);

                if (distance2 / 2 > distance1) {
                    operator_unit = unit;
                }

            } else {
                operator_unit = unit;
            }
        }

        if (operator_unit != nullptr) {
            AILOG_LOG(log, "Found {}.", ResourceManager_GetUnit(operator_unit->GetUnitType()).GetSingularName().data());

            operator_unit->AddTask(this);
        }
    }
}

void TaskRepair::DoRepairs() {
    if (target_unit->GetTask() == this ||
        (operator_unit->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == 0) {
        if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
            if (GameManager_IsActiveTurn(m_team)) {
                AILOG(log, "Repair/reload {}: perform repair.",
                      ResourceManager_GetUnit(target_unit->GetUnitType()).GetSingularName().data());

                if (operator_unit->flags & STATIONARY) {
                    Cargo materials;
                    Cargo capacity;

                    operator_unit->GetComplex()->GetCargoInfo(materials, capacity);

                    if (materials.raw > 0) {
                        if (Task_IsReadyToTakeOrders(&*operator_unit)) {
                            IssueOrder();
                        }

                    } else {
                        AILOG_LOG(log, "{} has no material.",
                                  ResourceManager_GetUnit(operator_unit->GetUnitType()).GetSingularName().data());
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
}

UnitInfo* TaskRepair::SelectRepairShop() {
    UnitInfo* result;
    ResourceID repair_shop;

    result = nullptr;

    repair_shop = GetRepairShopType();

    if (repair_shop != INVALID_ID) {
        int32_t distance;
        int32_t minimum_distance{INT32_MAX};

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == m_team && (*it).GetUnitType() == repair_shop) {
                distance = Access_GetApproximateDistance(&*it, &*target_unit);

                if (result == nullptr || distance < minimum_distance) {
                    result = &*it;
                    minimum_distance = distance;
                }
            }
        }
    }

    return result;
}

void TaskRepair::RemoveOperator() {
    if (operator_unit != nullptr) {
        operator_unit->RemoveTask(this);
        Task_RemoveMovementTasks(&*operator_unit);
        operator_unit = nullptr;
    }
}

void TaskRepair::RendezvousResultCallback(Task* task, UnitInfo* unit, char result) {
    AILOG(log, "Repair: rendezvous result.");

    if (result == TASKMOVE_RESULT_BLOCKED) {
        dynamic_cast<TaskRepair*>(task)->RemoveOperator();

    } else if (result == TASKMOVE_RESULT_SUCCESS) {
        dynamic_cast<TaskRepair*>(task)->EndTurn();
    }
}

ResourceID TaskRepair::GetRepairShopType() {
    ResourceID result;

    if (target_unit->GetUnitType() == COMMANDO || target_unit->GetUnitType() == INFANTRY) {
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

    if (ResourceManager_GetUnit(unit_type).GetFlags() & STATIONARY) {
        unit_list = &UnitsManager_StationaryUnits;

    } else {
        unit_list = &UnitsManager_MobileLandSeaUnits;
    }

    for (SmartList<UnitInfo>::Iterator it = unit_list->Begin(); it != unit_list->End(); ++it) {
        if ((*it).team == m_team && (*it).GetUnitType() == unit_type) {
            return;
        }
    }

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
         ++it) {
        if ((*it).GetTeam() == m_team &&
            ((*it).GetType() == TaskType_TaskCreateBuilding || (*it).GetType() == TaskType_TaskCreateUnit)) {
            if (dynamic_cast<TaskCreate*>(&*it)->GetUnitType() == unit_type) {
                return;
            }
        }
    }

    if (ResourceManager_GetUnit(unit_type).GetFlags() & STATIONARY) {
        AiPlayer_Teams[m_team].CreateBuilding(unit_type, Point(target_unit->grid_x, target_unit->grid_y), this);

    } else {
        SmartPointer<Task> task(new (std::nothrow)
                                    TaskCreateUnit(unit_type, this, Point(target_unit->grid_x, target_unit->grid_y)));

        TaskManager.AppendTask(*task);
    }
}

TaskRepair::TaskRepair(UnitInfo* unit) : Task(unit->team, nullptr, TASK_PRIORITY_REPAIR), target_unit(unit) {}

TaskRepair::~TaskRepair() {}

bool TaskRepair::IsUnitTransferable(UnitInfo& unit) { return target_unit != unit; }

std::string TaskRepair::WriteStatusLog() const {
    if (target_unit != nullptr) {
        return std::format("Repair {}", ResourceManager_GetUnit(target_unit->GetUnitType()).GetSingularName().data());

    } else {
        return "Finished repair task.";
    }
}

Rect* TaskRepair::GetBounds(Rect* bounds) {
    target_unit->GetBounds(bounds);

    return bounds;
}

uint8_t TaskRepair::GetType() const { return TaskType_TaskRepair; }

void TaskRepair::Init() {
    AILOG(log, "Begin repair/reload/upgrade unit.");

    target_unit->AddTask(this);
    CreateUnit();
    ChooseOperator();
    Task_RemindMoveFinished(&*target_unit);
}

void TaskRepair::BeginTurn() { EndTurn(); }

void TaskRepair::EndTurn() {
    if (target_unit != nullptr && target_unit->GetTask() == this) {
        Execute(*target_unit);
    }
}

bool TaskRepair::Execute(UnitInfo& unit) {
    bool result;

    if (target_unit == unit && target_unit->GetTask() == this) {
        AILOG(log, "Repair/reload {} move unit.",
              ResourceManager_GetUnit(target_unit->GetUnitType()).GetSingularName().data());

        if (IsInPerfectCondition()) {
            SmartPointer<Task> task;

            if (target_unit->GetOrderState() == ORDER_STATE_STORE) {
                task = new (std::nothrow) TaskActivate(&*target_unit, this, target_unit->GetParent());

                TaskManager.AppendTask(*task);

            } else {
                task = this;

                RemoveOperator();

                target_unit->RemoveTask(this);
                target_unit = nullptr;

                TaskManager.RemoveTask(*this);
            }

            result = true;

        } else {
            if (operator_unit == nullptr) {
                ChooseOperator();
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
                if (target_unit->GetOrderState() == ORDER_STATE_STORE) {
                    if (GameManager_IsActiveTurn(m_team)) {
                        AILOG_LOG(log, "{} is inside {}.",
                                  ResourceManager_GetUnit(target_unit->GetUnitType()).GetSingularName().data(),
                                  ResourceManager_GetUnit(operator_unit->GetUnitType()).GetSingularName().data());

                        DoRepairs();
                    }

                    result = true;

                } else {
                    if ((operator_unit->flags & STATIONARY) || operator_unit->storage > 0 ||
                        operator_unit->GetTask() != this) {
                        if (Task_IsAdjacent(&*operator_unit, target_unit->grid_x, target_unit->grid_y)) {
                            AILOG_LOG(log, "Adjacent to {}.",
                                      ResourceManager_GetUnit(operator_unit->GetUnitType()).GetSingularName().data());

                            if (GameManager_PlayMode != PLAY_MODE_UNKNOWN && GameManager_IsActiveTurn(m_team)) {
                                if (operator_unit->flags & STATIONARY) {
                                    if (target_unit->GetOrder() == ORDER_AWAIT) {
                                        // only enter repair shop if there is free capacity in it
                                        if (Access_GetStoredUnitCount(&*operator_unit) <
                                            operator_unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                                            AILOG_LOG(log, "Sending Board order.");

                                            target_unit->move_to_grid_x = operator_unit->grid_x;
                                            target_unit->move_to_grid_y = operator_unit->grid_y;

                                            if (target_unit->move_to_grid_x == target_unit->grid_x &&
                                                target_unit->move_to_grid_y == target_unit->grid_y) {
                                                ++target_unit->move_to_grid_x;
                                            }

                                            UnitsManager_SetNewOrder(&*target_unit, ORDER_MOVE_TO_UNIT,
                                                                     ORDER_STATE_INIT);

                                            result = true;

                                        } else {
                                            result = false;
                                        }

                                    } else {
                                        result = false;
                                    }

                                } else {
                                    DoRepairs();

                                    result = true;
                                }

                            } else {
                                result = false;
                            }

                        } else {
                            if (!Task_RetreatFromDanger(this, &*target_unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                                // only request rendezvous once
                                const auto tasks = operator_unit->GetTasks();
                                SmartPointer<Task> task;

                                for (auto it = tasks.Begin(); it != tasks.End(); ++it) {
                                    if ((*it).GetType() == TaskType_TaskRendezvous && (*it).GetParent() == this) {
                                        task = *it;
                                        break;
                                    }
                                }

                                if (!task) {
                                    SmartPointer<Task> task_rendezvous(new (std::nothrow) TaskRendezvous(
                                        &*target_unit, &*operator_unit, this, &TaskRepair::RendezvousResultCallback));

                                    TaskManager.AppendTask(*task_rendezvous);
                                }
                            }

                            result = true;
                        }

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

    m_parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskRepair::RemoveUnit(UnitInfo& unit) {
    AILOG(log, "Repair {}: remove {}.", ResourceManager_GetUnit(target_unit->GetUnitType()).GetSingularName().data(),
          ResourceManager_GetUnit(unit.GetUnitType()).GetSingularName().data());

    if (target_unit == unit) {
        SmartPointer<Task> task(this);

        RemoveOperator();

        target_unit = nullptr;

        TaskManager.RemoveTask(*this);

    } else if (operator_unit == unit) {
        operator_unit = nullptr;
    }
}

void TaskRepair::SelectOperator() {
    if (GetRepairShopType() == DEPOT) {
        UnitInfo* unit = nullptr;
        int32_t distance;
        int32_t minimum_distance{INT32_MAX};

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == m_team && (*it).GetUnitType() == REPAIR && (*it).hits > 0 &&
                ((*it).GetOrder() == ORDER_AWAIT || ((*it).GetOrder() == ORDER_MOVE && (*it).speed == 0)) &&
                target_unit != (*it)) {
                if ((*it).GetTask() == nullptr || (*it).GetTask()->ComparePriority(m_base_priority) > 0) {
                    distance = Access_GetApproximateDistance(&*it, &*target_unit);

                    if (unit == nullptr || distance < minimum_distance) {
                        unit = &*it;
                        minimum_distance = distance;
                    }
                }
            }
        }

        operator_unit = unit;
    }
}

int32_t TaskRepair::GetTurnsToComplete() {
    int32_t result;

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

    } else if (target_unit->GetOrderState() != ORDER_STATE_STORE ||
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
    AILOG(log, "Sending repair order.");

    operator_unit->SetParent(&*target_unit);

    if (target_unit->hits < target_unit->GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
        UnitsManager_SetNewOrder(&*operator_unit, ORDER_REPAIR, ORDER_STATE_INIT);

    } else if (target_unit->ammo < target_unit->GetBaseValues()->GetAttribute(ATTRIB_AMMO) &&
               target_unit->GetOrderState() == ORDER_STATE_STORE) {
        UnitsManager_SetNewOrder(&*operator_unit, ORDER_RELOAD, ORDER_STATE_INIT);
    }
}
