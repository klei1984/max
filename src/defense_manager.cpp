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

#include "defense_manager.hpp"

#include "ai.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskreload.hpp"
#include "taskrepair.hpp"
#include "taskupgrade.hpp"
#include "units_manager.hpp"

DefenseManager::DefenseManager() {
    asset_value_goal = 1;
    asset_value = 0;
}

DefenseManager::~DefenseManager() {}

void DefenseManager::ClearUnitsList() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units.Clear();
}

bool DefenseManager::IsUnitUsable(UnitInfo* unit) {
    bool result;

    if (weight_table.GetWeight(unit->unit_type)) {
        if (asset_value < asset_value_goal || unit->speed == 0) {
            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool DefenseManager::AddUnit(UnitInfo* unit) {
    bool result;

    if (IsUnitUsable(unit)) {
        /// \todo This is broken by design.
        unit_types.Remove(unit->unit_type);
        units.PushBack(*unit);

        asset_value += unit->GetNormalRateBuildCost();

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool DefenseManager::RemoveUnit(UnitInfo* unit) {
    bool result;

    if (units.Find(*unit) != units.End()) {
        asset_value -= unit->GetNormalRateBuildCost();
        units.Remove(*unit);

        result = true;

    } else {
        result = false;
    }

    return result;
}

void DefenseManager::AddRule(ResourceID unit_type, int weight) {
    if (Builder_IsBuildable(unit_type)) {
        UnitWeight unit_weight(unit_type, weight);

        weight_table.PushBack(unit_weight);
    }
}

void DefenseManager::MaintainDefences(Task* task) {
    SmartPointer<Task> maintenance_task;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).IsReadyForOrders(task)) {
            maintenance_task = nullptr;

            if ((*it).GetBaseValues()->GetAttribute(ATTRIB_HITS) != (*it).hits &&
                ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_APPRENTICE) {
                maintenance_task = new (std::nothrow) TaskRepair(&*it);

            } else if ((*it).ammo < (*it).GetBaseValues()->GetAttribute(ATTRIB_ROUNDS)) {
                maintenance_task = new (std::nothrow) TaskReload(&*it);

            } else if (((*it).flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) &&
                       !((*it).flags & REGENERATING_UNIT) && AiPlayer_Teams[(*it).team].ShouldUpgradeUnit(&*it)) {
                maintenance_task = new (std::nothrow) TaskUpgrade(&*it);
            }

            if (maintenance_task) {
                TaskManager.AppendTask(*maintenance_task);
            }
        }
    }
}

void DefenseManager::EvaluateNeeds(int* unit_counts) {
    for (int i = 0; i < unit_types.GetCount(); ++i) {
        ResourceID unit_type = Builder_GetBuilderType(*unit_types[i]);

        if (unit_counts[unit_type] > 0) {
            --unit_counts[unit_type];
        }
    }
}

void DefenseManager::PlanDefenses(int asset_value_goal_, TaskObtainUnits* task, int* unit_counts) {
    TeamUnits* team_units = UnitsManager_TeamInfo[task->GetTeam()].team_units;
    int build_costs = 0;
    int turns_till_mission_end = Task_EstimateTurnsTillMissionEnd();
    WeightTable table(weight_table, true);

    if (table.GetCount()) {
        asset_value_goal = asset_value_goal_;

        for (int i = 0; i < unit_types.GetCount(); ++i) {
            build_costs += Ai_GetNormalRateBuildCost(*unit_types[i], task->GetTeam());
        }

        for (int i = 0; i < weight_table.GetCount(); ++i) {
            UnitValues* unit_values = team_units->GetCurrentUnitValues(weight_table[i].unit_type);

            if (unit_values->GetAttribute(ATTRIB_TURNS) > turns_till_mission_end ||
                unit_values->GetAttribute(ATTRIB_SPEED) == 0 ||
                (UnitsManager_BaseUnits[weight_table[i].unit_type].flags & REGENERATING_UNIT) ||
                unit_counts[Builder_GetBuilderType(weight_table[i].unit_type)] == 0) {
                table[i].weight = 0;
            }
        }

        while (asset_value + build_costs < asset_value_goal) {
            ResourceID unit_type = table.RollUnitType();

            if (unit_type != INVALID_ID) {
                ResourceID builder_type = Builder_GetBuilderType(unit_type);

                --unit_counts[builder_type];

                if (unit_counts[builder_type] == 0) {
                    for (int i = 0; i < table.GetCount(); ++i) {
                        if (table[i].weight && Builder_GetBuilderType(table[i].unit_type) == builder_type) {
                            table[i].weight = 0;
                        }
                    }
                }

                build_costs += Ai_GetNormalRateBuildCost(unit_type, task->GetTeam());
                unit_types.PushBack(&unit_type);

                task->AddUnit(unit_type);

            } else {
                return;
            }
        }
    }
}
