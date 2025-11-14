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

#include "taskcreateunit.hpp"

#include <format>

#include "ai.hpp"
#include "ailog.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "game_manager.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskactivate.hpp"
#include "taskescort.hpp"
#include "units_manager.hpp"

enum {
    CREATE_UNIT_STATE_INITIALIZING,
    CREATE_UNIT_STATE_OBTAININING_BUILDER,
    CREATE_UNIT_STATE_WAITING_FOR_MATERIALS,
    CREATE_UNIT_STATE_BUILDING,
    CREATE_UNIT_STATE_ACTIVATING,
    CREATE_UNIT_STATE_FINISHED
};

TaskCreateUnit::TaskCreateUnit(ResourceID unit_type, Task* task, Point site_)
    : TaskCreate(task, task->GetPriority(), unit_type), site(site_), op_state(CREATE_UNIT_STATE_INITIALIZING) {}

TaskCreateUnit::TaskCreateUnit(UnitInfo* unit_, Task* task)
    : TaskCreate(task, unit_), site(unit_->grid_x, unit_->grid_y), op_state(CREATE_UNIT_STATE_BUILDING) {}

TaskCreateUnit::~TaskCreateUnit() {}

uint16_t TaskCreateUnit::GetPriority() const {
    uint16_t result;

    if (m_parent) {
        result = m_parent->GetPriority();

    } else {
        result = m_base_priority;
    }

    return result;
}

std::string TaskCreateUnit::WriteStatusLog() const {
    std::string result = std::format("Create a {}", UnitsManager_BaseUnits[unit_type].GetSingularName());

    if (builder && builder->GetBuildRate() > 1) {
        result += " at x2 rate";
    }

    return result;
}

uint8_t TaskCreateUnit::GetType() const { return TaskType_TaskCreateUnit; }

void TaskCreateUnit::AddUnit(UnitInfo& unit_) {
    AILOG(log, "Task Create Unit: Add builder {}.", UnitsManager_BaseUnits[unit_.GetUnitType()].GetSingularName());

    if (op_state == CREATE_UNIT_STATE_OBTAININING_BUILDER && (unit_.flags & STATIONARY)) {
        op_state = CREATE_UNIT_STATE_WAITING_FOR_MATERIALS;
        builder = unit_;

        if (IsUnitStillNeeded()) {
            builder->AddTask(this);
            WaitForMaterials();
        }

    } else if (op_state == CREATE_UNIT_STATE_FINISHED && (unit_.flags & STATIONARY)) {
        TaskManager.ClearUnitTasksAndRemindAvailable(&unit_);

    } else if (op_state != CREATE_UNIT_STATE_FINISHED && unit_type == unit_.GetUnitType()) {
        op_state = CREATE_UNIT_STATE_FINISHED;

        switch (unit_type) {
            case MISSLLCH:
            case SCANNER: {
                SmartPointer<Task> tank_escort(new (std::nothrow) TaskEscort(&unit_, TANK));
                TaskManager.AppendTask(*tank_escort);

                SmartPointer<Task> aa_escort(new (std::nothrow) TaskEscort(&unit_, SP_FLAK));
                TaskManager.AppendTask(*aa_escort);
            } break;

            case BOMBER:
            case AWAC:
            case FASTBOAT:
            case SP_FLAK: {
                SmartPointer<Task> fighter_escort(new (std::nothrow) TaskEscort(&unit_, FIGHTER));
                TaskManager.AppendTask(*fighter_escort);
            } break;
        }

        TaskManager.ClearUnitTasksAndRemindAvailable(&unit_);

        if (builder) {
            TaskManager.ClearUnitTasksAndRemindAvailable(&*builder);
        }

        m_parent = nullptr;

        TaskManager.RemoveTask(*this);
    }
}

void TaskCreateUnit::Init() {
    AILOG(log, "Task Create Unit: Begin.");

    if (builder) {
        builder->AddTask(this);
    }

    RemindTurnStart(true);
}

void TaskCreateUnit::BeginTurn() {
    AILOG(log, "Task Create Unit: Begin Turn.");

    if (op_state == CREATE_UNIT_STATE_WAITING_FOR_MATERIALS) {
        WaitForMaterials();
    }

    if (op_state == CREATE_UNIT_STATE_BUILDING && builder->GetOrderState() == ORDER_STATE_UNIT_READY) {
        op_state = CREATE_UNIT_STATE_ACTIVATING;

        builder->GetParent()->AddTask(this);

        SmartPointer<Task> activate_task(new (std::nothrow) TaskActivate(builder->GetParent(), this, &*builder));

        TaskManager.AppendTask(*activate_task);
    }

    EndTurn();
}

void TaskCreateUnit::EndTurn() {
    AILOG(log, "Create {}: End Turn.", UnitsManager_BaseUnits[unit_type].GetSingularName());

    if (op_state == CREATE_UNIT_STATE_INITIALIZING) {
        const auto builder_type = Builder_GetBuilderType(unit_type);

        if (builder_type != INVALID_ID) {
            SmartPointer<TaskObtainUnits> task_obtain_units = new (std::nothrow) TaskObtainUnits(this, site);
            op_state = CREATE_UNIT_STATE_OBTAININING_BUILDER;

            task_obtain_units->AddUnit(builder_type);

            TaskManager.AppendTask(*task_obtain_units);

        } else {
            RemoveSelf();
        }
    }

    if (op_state != CREATE_UNIT_STATE_FINISHED) {
        IsUnitStillNeeded();
    }
}

bool TaskCreateUnit::Execute(UnitInfo& unit_) {
    if (op_state != CREATE_UNIT_STATE_FINISHED && unit_.GetUnitType() == unit_type) {
        AddUnit(unit_);
    }

    if (op_state != CREATE_UNIT_STATE_FINISHED && builder == unit_) {
        BeginTurn();
    }

    return false;
}

void TaskCreateUnit::RemoveUnit(UnitInfo& unit_) {
    AILOG(log, "Task Create Unit: Remove {}.", UnitsManager_BaseUnits[unit_.GetUnitType()].GetSingularName());

    if (builder == unit_) {
        if (op_state <= CREATE_UNIT_STATE_BUILDING) {
            op_state = CREATE_UNIT_STATE_INITIALIZING;
        }

        builder = nullptr;
    }
}

void TaskCreateUnit::EventZoneCleared(Zone* zone, bool status) {}

bool TaskCreateUnit::Task_vfunc28() { return op_state >= CREATE_UNIT_STATE_BUILDING; }

void TaskCreateUnit::WaitForMaterials() {
    SDL_assert(op_state == CREATE_UNIT_STATE_WAITING_FOR_MATERIALS);

    if (GameManager_IsActiveTurn(m_team)) {
        Cargo materials;
        Cargo capacity;

        builder->GetComplex()->GetCargoInfo(materials, capacity);

        if (materials.raw >= 10) {
            if ((unit_type == ENGINEER || unit_type == CONSTRCT) && Task_GetReadyUnitsCount(m_team, unit_type) > 0 &&
                TaskManager_NeedToReserveRawMaterials(m_team)) {
                return;
            }

            if (materials.fuel < 10) {
                int32_t mining_station_count = 0;
                int32_t buildings_count = 0;
                int32_t power_consumption = 0;
                int32_t required_mining_stations;

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team == m_team) {
                        if ((*it).GetUnitType() == MININGST) {
                            ++mining_station_count;

                        } else if ((*it).GetUnitType() == GREENHSE || (*it).GetUnitType() == COMMTWR) {
                            ++buildings_count;

                        } else if ((*it).GetOrder() == ORDER_BUILD && (*it).GetOrderState() != ORDER_STATE_UNIT_READY) {
                            int32_t consumption = Cargo_GetPowerConsumptionRate((*it).GetUnitType());

                            if (consumption > 0) {
                                power_consumption += consumption;
                            }
                        }
                    }
                }

                required_mining_stations = 6 - (mining_station_count % 6);

                if (required_mining_stations > 3) {
                    required_mining_stations = 0;
                }

                required_mining_stations -= power_consumption;

                if (buildings_count >= required_mining_stations) {
                    if (buildings_count > 0) {
                        auto buildings_to_shut_down = buildings_count - required_mining_stations + 1;

                        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                             it != UnitsManager_StationaryUnits.End(); ++it) {
                            if (buildings_to_shut_down > 0) {
                                if ((*it).team == m_team && (*it).GetUnitType() == GREENHSE &&
                                    (*it).GetOrder() == ORDER_POWER_ON) {
                                    UnitsManager_SetNewOrder(&*it, ORDER_POWER_OFF, ORDER_STATE_INIT);

                                    --buildings_to_shut_down;
                                }

                            } else {
                                break;
                            }
                        }

                        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                             it != UnitsManager_StationaryUnits.End(); ++it) {
                            if (buildings_to_shut_down > 0) {
                                if ((*it).team == m_team && (*it).GetUnitType() == COMMTWR &&
                                    (*it).GetOrder() == ORDER_POWER_ON) {
                                    UnitsManager_SetNewOrder(&*it, ORDER_POWER_OFF, ORDER_STATE_INIT);

                                    --buildings_to_shut_down;
                                }

                            } else {
                                break;
                            }
                        }

                        if (buildings_to_shut_down > 0) {
                            return;
                        }

                    } else {
                        Cargo mining_production;
                        Cargo mining_capacity;

                        /* consider corner case where there is plenty of available raw materials but there are no fuel
                         * reserves even if production capacity is available */

                        builder->GetComplex()->GetCargoMining(mining_production, mining_capacity);

                        if (materials.raw < (capacity.raw / 2) || materials.fuel != 0 ||
                            mining_capacity.fuel == mining_production.fuel) {
                            return;
                        }
                    }
                }
            }

            {
                SmartObjectArray<ResourceID> build_list = builder->GetBuildList();

                op_state = CREATE_UNIT_STATE_BUILDING;

                if (materials.raw > 100 && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
                    builder->SetBuildRate(2);

                } else {
                    builder->SetBuildRate(1);
                }

                build_list.Clear();
                build_list.PushBack(&unit_type);

                builder->target_grid_x = 0;
                builder->target_grid_y = 0;

                builder->BuildOrder();
            }
        }
    }
}

bool TaskCreateUnit::IsUnitStillNeeded() {
    bool result;

    if (op_state <= CREATE_UNIT_STATE_BUILDING) {
        if (GameManager_IsActiveTurn(m_team)) {
            if (Task_EstimateTurnsTillMissionEnd() >
                    UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[m_team], unit_type)
                        ->GetAttribute(ATTRIB_TURNS) &&
                m_parent && m_parent->IsNeeded()) {
                result = true;

            } else if (builder && op_state == CREATE_UNIT_STATE_BUILDING &&
                       (builder->GetOrderState() != ORDER_STATE_EXECUTING_ORDER ||
                        builder->build_time != BuildMenu_GetTurnsToBuild(unit_type, m_team))) {
                result = true;

            } else {
                AILOG(log, "Create {}: aborting, no longer needed.",
                      UnitsManager_BaseUnits[unit_type].GetSingularName());

                if (op_state == CREATE_UNIT_STATE_BUILDING && builder) {
                    UnitsManager_SetNewOrder(&*builder, ORDER_HALT_BUILDING, ORDER_STATE_BUILD_CANCEL);
                }

                op_state = CREATE_UNIT_STATE_FINISHED;

                if (builder) {
                    TaskManager.ClearUnitTasksAndRemindAvailable(&*builder);
                }

                builder = nullptr;
                m_parent = nullptr;

                TaskManager.RemoveTask(*this);

                result = false;
            }

        } else {
            result = true;
        }

    } else {
        result = true;
    }

    return result;
}

bool TaskCreateUnit::Task_vfunc29() { return op_state >= CREATE_UNIT_STATE_BUILDING; }
