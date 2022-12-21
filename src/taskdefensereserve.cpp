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

#include "taskdefensereserve.hpp"

#include "access.hpp"
#include "aiattack.hpp"
#include "aiplayer.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "taskmovehome.hpp"
#include "units_manager.hpp"

bool TaskDefenseReserve::IsAdjacentToWater(UnitInfo* unit) {
    Point position;
    Rect bounds;
    int unit_size;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (unit->flags & BUILDING) {
        unit_size = 2;

    } else {
        unit_size = 1;
    }

    position.x = unit->grid_x - 1;
    position.y = unit->grid_x + unit_size;

    for (int direction = 0; direction < 8; direction += 2) {
        for (int range = 0; range < unit_size + 1; ++range) {
            position += Paths_8DirPointsArray[direction];

            if (Access_IsInsideBounds(&bounds, &position) &&
                Access_GetModifiedSurfaceType(position.x, position.y) == SURFACE_TYPE_WATER) {
                return true;
            }
        }
    }

    return false;
}

void TaskDefenseReserve::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    if (result != TASKMOVE_RESULT_SUCCESS || !AiAttack_EvaluateAssault(unit, task, &MoveFinishedCallback)) {
        if (unit->IsReadyForOrders(task)) {
            dynamic_cast<TaskDefenseReserve*>(task)->SupportAttacker(unit);
        }
    }
}

bool TaskDefenseReserve::SupportAttacker(UnitInfo* unit) {
    bool result;

    if (AiAttack_FollowAttacker(this, unit, 0x1800)) {
        result = true;

    } else {
        SmartPointer<Task> move_home_task(new (std::nothrow) TaskMoveHome(unit, this));

        TaskManager.AppendTask(*move_home_task);

        result = true;
    }

    return result;
}

TaskDefenseReserve::TaskDefenseReserve(unsigned short team_, Point site_) : Task(team_, nullptr, 0x1900) {
    int ai_strategy = AiPlayer_Teams[team].GetStrategy();
    site = site_;

    managers[0].AddRule(ALNTANK, 1);
    managers[0].AddRule(GUNTURRT, 1);

    if (ai_strategy != AI_STRATEGY_AIR && ai_strategy != AI_STRATEGY_FAST_ATTACK &&
        ai_strategy != AI_STRATEGY_SCOUT_HORDE) {
        managers[0].AddRule(TANK, 12);
    }

    if (ai_strategy == AI_STRATEGY_ESPIONAGE) {
        managers[0].AddRule(COMMANDO, 4);
    }

    if (ai_strategy != AI_STRATEGY_TANK_HORDE) {
        managers[0].AddRule(BOMBER, 6);
    }

    managers[1].AddRule(ALNASGUN, 1);
    managers[1].AddRule(ARTYTRRT, 1);
    managers[1].AddRule(ANTIMSSL, 1);

    if (ai_strategy != AI_STRATEGY_MISSILES) {
        managers[1].AddRule(ARTILLRY, 4);
    }

    if (ai_strategy != AI_STRATEGY_FAST_ATTACK && ai_strategy != AI_STRATEGY_SCOUT_HORDE) {
        managers[1].AddRule(ROCKTLCH, 5);
        managers[1].AddRule(MISSLLCH, 10);
    }

    managers[2].AddRule(ANTIAIR, 1);

    if (ai_strategy != AI_STRATEGY_AIR) {
        managers[2].AddRule(SP_FLAK, 6);
    }

    managers[2].AddRule(ALNPLANE, 1);

    managers[4].AddRule(CORVETTE, 12);

    if (ai_strategy != AI_STRATEGY_AIR && ai_strategy != AI_STRATEGY_FAST_ATTACK &&
        ai_strategy != AI_STRATEGY_SCOUT_HORDE) {
        if (ai_strategy != AI_STRATEGY_TANK_HORDE) {
            managers[5].AddRule(SUBMARNE, 6);
        }

        managers[5].AddRule(BATTLSHP, 6);
    }

    managers[5].AddRule(GUNTURRT, 1);
    managers[5].AddRule(JUGGRNT, 1);

    if (ai_strategy != AI_STRATEGY_SEA && ai_strategy != AI_STRATEGY_TANK_HORDE) {
        managers[5].AddRule(BOMBER, 6);
    }

    managers[6].AddRule(MSSLBOAT, 20);
    managers[6].AddRule(ARTYTRRT, 1);
    managers[6].AddRule(ANTIMSSL, 1);

    managers[7].AddRule(ANTIAIR, 1);

    if (ai_strategy != AI_STRATEGY_AIR) {
        managers[7].AddRule(FASTBOAT, 6);
    }

    if (ai_strategy != AI_STRATEGY_SEA && ai_strategy != AI_STRATEGY_FAST_ATTACK &&
        ai_strategy != AI_STRATEGY_SCOUT_HORDE) {
        managers[7].AddRule(FIGHTER, 12);
    }

    managers[7].AddRule(ALNPLANE, 1);
}

TaskDefenseReserve::~TaskDefenseReserve() {}

bool TaskDefenseReserve::IsUnitUsable(UnitInfo& unit) {
    if (unit.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) > 0) {
        for (int i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
            if (managers[i].IsUnitUsable(&unit)) {
                return true;
            }
        }
    }

    return false;
}

int TaskDefenseReserve::GetMemoryUse() const {
    int result;

    result = units.GetMemorySize() - 6;

    for (int i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        result += managers[i].GetMemoryUse() - 22;
    }

    return result;
}

char* TaskDefenseReserve::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Defense Reserve");

    return buffer;
}

unsigned char TaskDefenseReserve::GetType() const { return TaskType_TaskDefenseReserve; }

void TaskDefenseReserve::AddUnit(UnitInfo& unit) {
    for (int i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        if (managers[i].AddUnit(&unit)) {
            unit.PushFrontTask1List(this);
            unit.AddReminders(false);

            return;
        }
    }
}

void TaskDefenseReserve::BeginTurn() {
    if (!GetField8()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
    }
}

void TaskDefenseReserve::EndTurn() {
    SmartPointer<TaskObtainUnits> obtain_units_task;
    int unit_counts[UNIT_END];
    unsigned short target_team;
    int total_assets_land;
    int total_assets_sea;
    int total_assets_air;
    int total_assets_stealth_land;
    int total_assets_stealth_sea;

    memset(unit_counts, 0, sizeof(unit_counts));

    target_team = AiPlayer_Teams[team].GetTargetTeam();

    for (int i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        managers[i].MaintainDefences(this);
    }

    obtain_units_task = new (std::nothrow) TaskObtainUnits(this, site);

    total_assets_land = 0;
    total_assets_sea = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && ((*it).flags & BUILDING)) {
            if (IsAdjacentToWater(&*it)) {
                total_assets_sea += (*it).GetNormalRateBuildCost();

            } else {
                total_assets_land += (*it).GetNormalRateBuildCost();
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team) {
            ++unit_counts[(*it).unit_type];
        }
    }

    ++unit_counts[AIRPLT];
    ++unit_counts[LANDPLT];
    ++unit_counts[LIGHTPLT];
    ++unit_counts[SHIPYARD];
    ++unit_counts[TRAINHAL];

    for (int i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        managers[i].EvaluateNeeds(unit_counts);
    }

    if (total_assets_land + total_assets_sea > 0) {
        total_assets_air = 0;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
             it != UnitsManager_MobileAirUnits.End(); ++it) {
            if ((*it).team == target_team && (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0) {
                total_assets_air += (*it).GetNormalRateBuildCost();
            }
        }

        managers[2].PlanDefenses((total_assets_land * total_assets_air) / (total_assets_land + total_assets_sea),
                                 &*obtain_units_task, unit_counts);

        managers[7].PlanDefenses((total_assets_sea * total_assets_air) / (total_assets_sea + total_assets_land),
                                 &*obtain_units_task, unit_counts);
    }

    total_assets_land = 0;
    total_assets_sea = 0;
    total_assets_stealth_land = 0;
    total_assets_stealth_sea = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == target_team && (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0) {
            if ((*it).unit_type == COMMANDO) {
                total_assets_stealth_land += (*it).GetNormalRateBuildCost();

            } else if ((*it).unit_type == SUBMARNE) {
                total_assets_stealth_sea += (*it).GetNormalRateBuildCost();

            } else if ((*it).flags & MOBILE_SEA_UNIT) {
                total_assets_sea += (*it).GetNormalRateBuildCost();

            } else {
                total_assets_land += (*it).GetNormalRateBuildCost();
            }
        }
    }

    managers[3].PlanDefenses(total_assets_stealth_land, &*obtain_units_task, unit_counts);
    managers[0].PlanDefenses(total_assets_land / 2, &*obtain_units_task, unit_counts);
    managers[1].PlanDefenses(total_assets_land / 2, &*obtain_units_task, unit_counts);
    managers[4].PlanDefenses(total_assets_stealth_sea, &*obtain_units_task, unit_counts);
    managers[5].PlanDefenses(total_assets_sea / 2, &*obtain_units_task, unit_counts);
    managers[6].PlanDefenses(total_assets_sea / 2, &*obtain_units_task, unit_counts);

    TaskManager.AppendTask(*obtain_units_task);
}

bool TaskDefenseReserve::Task_vfunc17(UnitInfo& unit) {
    bool result;

    if (unit.speed > 0 && unit.IsReadyForOrders(this)) {
        if (AiAttack_EvaluateAssault(&unit, this, &MoveFinishedCallback)) {
            result = true;

        } else {
            result = SupportAttacker(&unit);
        }

    } else {
        result = false;
    }

    return result;
}

void TaskDefenseReserve::RemoveSelf() {
    for (int i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        managers[i].ClearUnitsList();
    }

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskDefenseReserve::RemoveUnit(UnitInfo& unit) {
    for (int i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        managers[i].RemoveUnit(&unit);
    }
}
