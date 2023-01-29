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

#include "builder.hpp"
#include "buildmenu.hpp"
#include "game_manager.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskactivate.hpp"
#include "taskescort.hpp"
#include "units_manager.hpp"

enum {
    CREATE_UNIT_STATE_0,
    CREATE_UNIT_STATE_1,
    CREATE_UNIT_STATE_WAITING_FOR_MATERIALS,
    CREATE_UNIT_STATE_3,
    CREATE_UNIT_STATE_4,
    CREATE_UNIT_STATE_5
};

TaskCreateUnit::TaskCreateUnit(ResourceID unit_type, Task* task, Point site_)
    : TaskCreate(task, task->GetFlags(), unit_type), site(site_), op_state(CREATE_UNIT_STATE_0) {}

TaskCreateUnit::TaskCreateUnit(UnitInfo* unit_, Task* task)
    : TaskCreate(task, unit_), site(unit_->grid_x, unit_->grid_y), op_state(CREATE_UNIT_STATE_3) {}

TaskCreateUnit::~TaskCreateUnit() {}

int TaskCreateUnit::GetMemoryUse() const { return 4; }

unsigned short TaskCreateUnit::GetFlags() const {
    unsigned short result;

    if (parent) {
        result = parent->GetFlags();

    } else {
        result = flags;
    }

    return result;
}

char* TaskCreateUnit::WriteStatusLog(char* buffer) const {
    sprintf(buffer, "Create a %s", UnitsManager_BaseUnits[unit_type].singular_name);

    if (unit && unit->GetBuildRate() > 1) {
        strcat(buffer, " at x2 rate");
    }

    return buffer;
}

unsigned char TaskCreateUnit::GetType() const { return TaskType_TaskCreateUnit; }

void TaskCreateUnit::AddUnit(UnitInfo& unit_) {
    if (op_state == CREATE_UNIT_STATE_1 && (unit_.flags & STATIONARY)) {
        op_state = CREATE_UNIT_STATE_WAITING_FOR_MATERIALS;
        unit = unit_;

        if (IsUnitStillNeeded()) {
            unit->AddTask(this);
            WaitForMaterials();
        }

    } else if (op_state == CREATE_UNIT_STATE_5 && (unit_.flags & STATIONARY)) {
        TaskManager.RemindAvailable(&unit_);

    } else if (op_state != CREATE_UNIT_STATE_5 && unit_type == unit_.unit_type) {
        op_state = CREATE_UNIT_STATE_5;

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

        TaskManager.RemindAvailable(&unit_);

        if (unit) {
            TaskManager.RemindAvailable(&*unit);
        }

        parent = nullptr;

        TaskManager.RemoveTask(*this);
    }
}

void TaskCreateUnit::Begin() {
    if (unit) {
        unit->AddTask(this);
    }

    RemindTurnStart(true);
}

void TaskCreateUnit::BeginTurn() {
    if (op_state == CREATE_UNIT_STATE_WAITING_FOR_MATERIALS) {
        WaitForMaterials();
    }

    if (op_state == CREATE_UNIT_STATE_3 && unit->state == ORDER_STATE_UNIT_READY) {
        op_state = CREATE_UNIT_STATE_4;

        unit->GetParent()->AddTask(this);

        SmartPointer<Task> activate_task(new (std::nothrow) TaskActivate(unit->GetParent(), this, &*unit));

        TaskManager.AppendTask(*activate_task);
    }

    EndTurn();
}

void TaskCreateUnit::EndTurn() {
    if (op_state == CREATE_UNIT_STATE_0) {
        SmartPointer<TaskObtainUnits> task_obtain_units = new (std::nothrow) TaskObtainUnits(this, site);
        op_state = CREATE_UNIT_STATE_1;

        task_obtain_units->AddUnit(Builder_GetBuilderType(unit_type));

        TaskManager.AppendTask(*task_obtain_units);
    }

    if (op_state != CREATE_UNIT_STATE_5) {
        IsUnitStillNeeded();
    }
}

bool TaskCreateUnit::Execute(UnitInfo& unit_) {
    if (op_state != CREATE_UNIT_STATE_5 && unit_.unit_type == unit_type) {
        AddUnit(unit_);
    }

    if (op_state != CREATE_UNIT_STATE_5 && unit == unit_) {
        BeginTurn();
    }

    return false;
}

void TaskCreateUnit::RemoveUnit(UnitInfo& unit_) {
    if (unit == unit_) {
        if (op_state <= CREATE_UNIT_STATE_3) {
            op_state = CREATE_UNIT_STATE_0;
        }

        unit = nullptr;
    }
}

void TaskCreateUnit::Task_vfunc27(Zone* zone, char mode) {}

bool TaskCreateUnit::Task_vfunc28() { return op_state >= CREATE_UNIT_STATE_3; }

void TaskCreateUnit::WaitForMaterials() {
    SDL_assert(op_state == CREATE_UNIT_STATE_WAITING_FOR_MATERIALS);

    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
        Cargo materials;
        Cargo capacity;

        unit->GetComplex()->GetCargoInfo(materials, capacity);

        if (materials.raw >= 10) {
            if ((unit_type == ENGINEER || unit_type == CONSTRCT) && Task_GetReadyUnitsCount(team, unit_type) > 0 &&
                TaskManager_NeedToReserveRawMaterials(team)) {
                return;
            }

            if (materials.fuel < 10) {
                int mining_station_count = 0;
                int buildings_count = 0;
                int power_consumption = 0;
                int required_mining_stations;
                int buildings_to_shut_down;

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team == team) {
                        if ((*it).unit_type == MININGST) {
                            ++mining_station_count;

                        } else if ((*it).unit_type == GREENHSE || (*it).unit_type == COMMTWR) {
                            ++buildings_count;

                        } else if ((*it).orders == ORDER_BUILD && (*it).state != ORDER_STATE_UNIT_READY) {
                            int consumption = Cargo_GetPowerConsumptionRate((*it).unit_type);

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
                    if (buildings_count == 0) {
                        return;
                    }

                    buildings_to_shut_down = buildings_count - required_mining_stations + 1;

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                         it != UnitsManager_StationaryUnits.End(); ++it) {
                        if (buildings_to_shut_down > 0) {
                            if ((*it).team == team && (*it).unit_type == GREENHSE && (*it).orders == ORDER_POWER_ON) {
                                UnitsManager_SetNewOrder(&*it, ORDER_POWER_OFF, ORDER_STATE_0);

                                --buildings_to_shut_down;
                            }

                        } else {
                            break;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                         it != UnitsManager_StationaryUnits.End(); ++it) {
                        if (buildings_to_shut_down > 0) {
                            if ((*it).team == team && (*it).unit_type == COMMTWR && (*it).orders == ORDER_POWER_ON) {
                                UnitsManager_SetNewOrder(&*it, ORDER_POWER_OFF, ORDER_STATE_0);

                                --buildings_to_shut_down;
                            }

                        } else {
                            break;
                        }
                    }

                    if (buildings_to_shut_down > 0) {
                        return;
                    }
                }
            }

            {
                SmartObjectArray<ResourceID> build_list = unit->GetBuildList();

                op_state = CREATE_UNIT_STATE_3;

                if (materials.raw > 100 && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
                    unit->SetBuildRate(2);

                } else {
                    unit->SetBuildRate(1);
                }

                build_list.Clear();
                build_list.PushBack(&unit_type);

                unit->target_grid_x = 0;
                unit->target_grid_y = 0;

                unit->BuildOrder();
            }
        }
    }
}

bool TaskCreateUnit::IsUnitStillNeeded() {
    bool result;

    if (op_state <= CREATE_UNIT_STATE_3) {
        if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
            if (Task_EstimateTurnsTillMissionEnd() >
                    UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)
                        ->GetAttribute(ATTRIB_TURNS) &&
                parent && parent->Task_vfunc9()) {
                result = true;

            } else if (unit && op_state == CREATE_UNIT_STATE_3 &&
                       (unit->state != ORDER_STATE_1 ||
                        unit->build_time != BuildMenu_GetTurnsToBuild(unit_type, team))) {
                result = true;

            } else {
                if (op_state == CREATE_UNIT_STATE_3 && unit) {
                    UnitsManager_SetNewOrder(&*unit, ORDER_HALT_BUILDING, ORDER_STATE_13);
                }

                op_state = CREATE_UNIT_STATE_5;

                if (unit) {
                    TaskManager.RemindAvailable(&*unit);
                }

                unit = nullptr;
                parent = nullptr;

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

bool TaskCreateUnit::Task_vfunc29() { return op_state >= CREATE_UNIT_STATE_3; }
