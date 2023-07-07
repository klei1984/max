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

#include "taskattackreserve.hpp"

#include "ai.hpp"
#include "aiattack.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskmovehome.hpp"
#include "taskobtainunits.hpp"
#include "taskreload.hpp"
#include "taskrepair.hpp"
#include "taskupgrade.hpp"
#include "units_manager.hpp"
#include "weighttable.hpp"

TaskAttackReserve::TaskAttackReserve(unsigned short team, Point site_) : Task(team, nullptr, 0x2600) {
    site = site_;
    total_worth = 0;
}

TaskAttackReserve::~TaskAttackReserve() {}

void TaskAttackReserve::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskAttackReserve* attack_reserve_task = dynamic_cast<TaskAttackReserve*>(task);

    if (result || !AiAttack_EvaluateAssault(unit, task, &MoveFinishedCallback)) {
        if (unit->IsReadyForOrders(task)) {
            attack_reserve_task->ChaseAttacker(unit);
        }
    }
}

bool TaskAttackReserve::ChaseAttacker(UnitInfo* unit) {
    bool result;

    if (AiAttack_FollowAttacker(this, unit, 0x2000)) {
        result = true;

    } else {
        SmartPointer<Task> move_home_task(new (std::nothrow) TaskMoveHome(unit, this));

        TaskManager.AppendTask(*move_home_task);

        result = true;
    }

    return result;
}

bool TaskAttackReserve::IsUnitUsable(UnitInfo& unit) {
    bool result;

    if (unit.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) > 0 &&
        unit.ammo >= unit.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) &&
        unit.GetBaseValues()->GetAttribute(ATTRIB_SPEED) > 0) {
        result = true;

    } else if (unit.unit_type == ENGINEER || unit.unit_type == CONSTRCT || unit.unit_type == LANDPLT ||
               unit.unit_type == LIGHTPLT || unit.unit_type == AIRPLT || unit.unit_type == SHIPYARD ||
               unit.unit_type == TRAINHAL) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

int TaskAttackReserve::GetMemoryUse() const { return units.GetMemorySize() - 6; }

char* TaskAttackReserve::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Maintain an attack reserve.");

    return buffer;
}

unsigned char TaskAttackReserve::GetType() const { return TaskType_TaskAttackReserve; }

void TaskAttackReserve::AddUnit(UnitInfo& unit) {
    if (unit.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS)) {
        if (unit.speed > 0) {
            SmartPointer<Task> best_task;
            int distance;
            int minimum_distance{INT32_MAX};

            units.PushBack(unit);
            unit.AddTask(this);

            for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin();
                 it != TaskManager.GetTaskList().End(); ++it) {
                if ((*it).GetTeam() == unit.team && (*it).GetType() != TaskType_TaskAttackReserve &&
                    (*it).IsUnitUsable(unit)) {
                    Rect bounds;

                    (*it).GetBounds(&bounds);

                    distance = (unit.grid_x - bounds.ulx) * (unit.grid_x - bounds.ulx) +
                               (unit.grid_y - bounds.uly) * (unit.grid_y - bounds.uly);

                    if (!best_task || distance < minimum_distance) {
                        best_task = *it;
                        minimum_distance = distance;
                    }
                }
            }

            if (best_task) {
                unit.RemoveTasks();

                best_task->AddUnit(unit);
            }

        } else {
            TaskManager.RemindAvailable(&unit);
        }

    } else if (unit.team == team && unit.orders != ORDER_IDLE) {
        if (unit.unit_type == LANDPLT || unit.unit_type == LIGHTPLT || unit.unit_type == AIRPLT ||
            unit.unit_type == SHIPYARD || unit.unit_type == TRAINHAL) {
            if (unit.GetComplex()->material > 10 && unit.GetComplex()->fuel > 10) {
                int turns_till_mission_end = Task_EstimateTurnsTillMissionEnd();
                WeightTable table = AiPlayer_Teams[team].GetFilteredWeightTable(INVALID_ID, 3);

                for (int i = 0; i < table.GetCount(); ++i) {
                    if (Builder_GetBuilderType(table[i].unit_type) != unit.unit_type ||
                        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], table[i].unit_type)
                                ->GetAttribute(ATTRIB_TURNS) > turns_till_mission_end) {
                        table[i].weight = 0;
                    }
                }

                ResourceID unit_type = table.RollUnitType();

                if (unit_type != INVALID_ID) {
                    SmartPointer<TaskObtainUnits> obtain_units_task(new (std::nothrow) TaskObtainUnits(this, site));

                    obtain_units_task->AddUnit(unit_type);

                    total_worth += Ai_GetNormalRateBuildCost(unit_type, team);

                    TaskManager.AppendTask(*obtain_units_task);
                }
            }
        }

        if (unit.unit_type == CONSTRCT || unit.unit_type == ENGINEER) {
            if (!TaskManager_NeedToReserveRawMaterials(team)) {
                WeightTable table;
                bool builders_needed = false;
                ResourceID unit_type;
                unsigned short unit_types[UNIT_END];

                memset(unit_types, 0, sizeof(unit_types));

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team == team) {
                        ++unit_types[(*it).unit_type];
                    }
                }

                if (!unit_types[LANDPLT] || !unit_types[LIGHTPLT] || !unit_types[AIRPLT]) {
                    builders_needed = true;
                }

                table += AiPlayer_Teams[team].GetFilteredWeightTable(INVALID_ID, 2);

                for (int i = 0; i < table.GetCount(); ++i) {
                    if (table[i].unit_type == INVALID_ID) {
                        table[i].weight = 0;

                        continue;
                    }

                    unit_type = Builder_GetBuilderType(table[i].unit_type);

                    if (unit_type != unit.unit_type && Builder_GetBuilderType(unit_type) != unit.unit_type) {
                        table[i].weight = 0;
                    }

                    if (builders_needed) {
                        ResourceID builder = Builder_GetBuilderType(table[i].unit_type);

                        if (builder != INVALID_ID &&
                            (UnitsManager_BaseUnits[table[i].unit_type].flags &
                             (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) &&
                            unit_types[builder] > 0) {
                            table[i].weight = 0;
                        }
                    }
                }

                unit_type = table.RollUnitType();

                if (unit_type != INVALID_ID) {
                    ResourceID builder = Builder_GetBuilderType(unit_type);

                    if (builder != unit.unit_type) {
                        unit_type = builder;
                    }

                    AiPlayer_Teams[team].CreateBuilding(unit_type, site, this);
                }
            }
        }
    }
}

void TaskAttackReserve::ChildComplete(Task* task) {}

void TaskAttackReserve::EndTurn() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetTask() == this) {
            if ((*it).ammo < (*it).GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) ||
                (((*it).flags & STATIONARY) && (*it).ammo < (*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO) / 2)) {
                SmartPointer<Task> reload_task(new (std::nothrow) TaskReload(&*it));

                TaskManager.AppendTask(*reload_task);

            } else if ((*it).hits < (*it).GetBaseValues()->GetAttribute(ATTRIB_HITS) / 2 &&
                       ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_APPRENTICE) {
                SmartPointer<Task> repair_task(new (std::nothrow) TaskRepair(&*it));

                TaskManager.AppendTask(*repair_task);

            } else if (((*it).flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) &&
                       !((*it).flags & REGENERATING_UNIT) && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_EXPERT &&
                       AiPlayer_Teams[(*it).team].ShouldUpgradeUnit(&*it)) {
                SmartPointer<Task> upgrade_task(new (std::nothrow) TaskUpgrade(&*it));

                TaskManager.AppendTask(*upgrade_task);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).speed > 0 && (*it).IsReadyForOrders(this)) {
            Task_RemindMoveFinished(&*it);
        }
    }
}

bool TaskAttackReserve::Execute(UnitInfo& unit) {
    bool result;

    if (unit.speed > 0 && unit.IsReadyForOrders(this)) {
        if (AiAttack_EvaluateAssault(&unit, this, &MoveFinishedCallback)) {
            result = true;

        } else {
            result = ChaseAttacker(&unit);
        }

    } else {
        result = false;
    }

    return result;
}

void TaskAttackReserve::RemoveSelf() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units.Clear();

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskAttackReserve::RemoveUnit(UnitInfo& unit) {
    total_worth -= unit.GetNormalRateBuildCost();

    units.Remove(unit);
}
