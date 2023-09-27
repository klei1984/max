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

enum {
    DEFENSE_TYPE_BASIC_LAND,
    DEFENSE_TYPE_ADVANCED_LAND,
    DEFENSE_TYPE_LAND_ANTIAIR,
    DEFENSE_TYPE_LAND_STEALTH,
    DEFENSE_TYPE_SEA_STEALTH,
    DEFENSE_TYPE_BASIC_SEA,
    DEFENSE_TYPE_ADVANCED_SEA,
    DEFENSE_TYPE_SEA_ANTIAIR,
};

bool TaskDefenseReserve::IsAdjacentToWater(UnitInfo* unit) {
    Point position;
    Rect bounds;
    int32_t unit_size;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (unit->flags & BUILDING) {
        unit_size = 2;

    } else {
        unit_size = 1;
    }

    position.x = unit->grid_x - 1;
    position.y = unit->grid_x + unit_size;

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t range = 0; range < unit_size + 1; ++range) {
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

TaskDefenseReserve::TaskDefenseReserve(uint16_t team_, Point site_) : Task(team_, nullptr, 0x1900) {
    int32_t ai_strategy = AiPlayer_Teams[team].GetStrategy();
    site = site_;

    managers[DEFENSE_TYPE_BASIC_LAND].AddRule(ALNTANK, 1);
    managers[DEFENSE_TYPE_BASIC_LAND].AddRule(GUNTURRT, 1);

    if (ai_strategy != AI_STRATEGY_AIR && ai_strategy != AI_STRATEGY_FAST_ATTACK &&
        ai_strategy != AI_STRATEGY_SCOUT_HORDE) {
        managers[DEFENSE_TYPE_BASIC_LAND].AddRule(TANK, 12);
    }

    if (ai_strategy == AI_STRATEGY_ESPIONAGE) {
        managers[DEFENSE_TYPE_BASIC_LAND].AddRule(COMMANDO, 4);
    }

    if (ai_strategy != AI_STRATEGY_TANK_HORDE) {
        managers[DEFENSE_TYPE_BASIC_LAND].AddRule(BOMBER, 6);
    }

    managers[DEFENSE_TYPE_ADVANCED_LAND].AddRule(ALNASGUN, 1);
    managers[DEFENSE_TYPE_ADVANCED_LAND].AddRule(ARTYTRRT, 1);
    managers[DEFENSE_TYPE_ADVANCED_LAND].AddRule(ANTIMSSL, 1);

    if (ai_strategy != AI_STRATEGY_MISSILES) {
        managers[DEFENSE_TYPE_ADVANCED_LAND].AddRule(ARTILLRY, 4);
    }

    if (ai_strategy != AI_STRATEGY_FAST_ATTACK && ai_strategy != AI_STRATEGY_SCOUT_HORDE) {
        managers[DEFENSE_TYPE_ADVANCED_LAND].AddRule(ROCKTLCH, 5);
        managers[DEFENSE_TYPE_ADVANCED_LAND].AddRule(MISSLLCH, 10);
    }

    managers[DEFENSE_TYPE_LAND_ANTIAIR].AddRule(ANTIAIR, 1);

    if (ai_strategy != AI_STRATEGY_FAST_ATTACK) {
        managers[DEFENSE_TYPE_LAND_ANTIAIR].AddRule(FIGHTER, 12);
    }

    if (ai_strategy != AI_STRATEGY_AIR) {
        managers[DEFENSE_TYPE_LAND_ANTIAIR].AddRule(SP_FLAK, 6);
    }

    managers[DEFENSE_TYPE_LAND_ANTIAIR].AddRule(ALNPLANE, 1);

    managers[DEFENSE_TYPE_SEA_STEALTH].AddRule(CORVETTE, 12);

    if (ai_strategy != AI_STRATEGY_AIR && ai_strategy != AI_STRATEGY_FAST_ATTACK &&
        ai_strategy != AI_STRATEGY_SCOUT_HORDE) {
        if (ai_strategy != AI_STRATEGY_TANK_HORDE) {
            managers[DEFENSE_TYPE_BASIC_SEA].AddRule(SUBMARNE, 6);
        }

        managers[DEFENSE_TYPE_BASIC_SEA].AddRule(BATTLSHP, 6);
    }

    managers[DEFENSE_TYPE_BASIC_SEA].AddRule(GUNTURRT, 1);
    managers[DEFENSE_TYPE_BASIC_SEA].AddRule(JUGGRNT, 1);

    if (ai_strategy != AI_STRATEGY_SEA && ai_strategy != AI_STRATEGY_TANK_HORDE) {
        managers[DEFENSE_TYPE_BASIC_SEA].AddRule(BOMBER, 6);
    }

    managers[DEFENSE_TYPE_ADVANCED_SEA].AddRule(MSSLBOAT, 20);
    managers[DEFENSE_TYPE_ADVANCED_SEA].AddRule(ARTYTRRT, 1);
    managers[DEFENSE_TYPE_ADVANCED_SEA].AddRule(ANTIMSSL, 1);

    managers[DEFENSE_TYPE_SEA_ANTIAIR].AddRule(ANTIAIR, 1);

    if (ai_strategy != AI_STRATEGY_AIR) {
        managers[DEFENSE_TYPE_SEA_ANTIAIR].AddRule(FASTBOAT, 6);
    }

    if (ai_strategy != AI_STRATEGY_SEA && ai_strategy != AI_STRATEGY_FAST_ATTACK &&
        ai_strategy != AI_STRATEGY_SCOUT_HORDE) {
        managers[DEFENSE_TYPE_SEA_ANTIAIR].AddRule(FIGHTER, 12);
    }

    managers[DEFENSE_TYPE_SEA_ANTIAIR].AddRule(ALNPLANE, 1);
}

TaskDefenseReserve::~TaskDefenseReserve() {}

bool TaskDefenseReserve::IsUnitUsable(UnitInfo& unit) {
    if (unit.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) > 0) {
        for (int32_t i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
            if (managers[i].IsUnitUsable(&unit)) {
                return true;
            }
        }
    }

    return false;
}

char* TaskDefenseReserve::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Defense Reserve");

    return buffer;
}

uint8_t TaskDefenseReserve::GetType() const { return TaskType_TaskDefenseReserve; }

void TaskDefenseReserve::AddUnit(UnitInfo& unit) {
    for (int32_t i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        if (managers[i].AddUnit(&unit)) {
            unit.AddTask(this);
            unit.ScheduleDelayedTasks(false);

            return;
        }
    }
}

void TaskDefenseReserve::BeginTurn() {
    if (!IsScheduledForTurnEnd()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
    }
}

void TaskDefenseReserve::EndTurn() {
    SmartPointer<TaskObtainUnits> obtain_units_task;
    int32_t unit_counts[UNIT_END];
    uint16_t target_team;
    int32_t total_assets_land;
    int32_t total_assets_sea;
    int32_t total_assets_air;
    int32_t total_assets_stealth_land;
    int32_t total_assets_stealth_sea;

    memset(unit_counts, 0, sizeof(unit_counts));

    target_team = AiPlayer_Teams[team].GetTargetTeam();

    for (int32_t i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
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

    for (int32_t i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
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

        managers[DEFENSE_TYPE_LAND_ANTIAIR].PlanDefenses(
            (total_assets_land * total_assets_air) / (total_assets_land + total_assets_sea), &*obtain_units_task,
            unit_counts);

        managers[DEFENSE_TYPE_SEA_ANTIAIR].PlanDefenses(
            (total_assets_sea * total_assets_air) / (total_assets_sea + total_assets_land), &*obtain_units_task,
            unit_counts);
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

    managers[DEFENSE_TYPE_LAND_STEALTH].PlanDefenses(total_assets_stealth_land, &*obtain_units_task, unit_counts);
    managers[DEFENSE_TYPE_BASIC_LAND].PlanDefenses(total_assets_land / 2, &*obtain_units_task, unit_counts);
    managers[DEFENSE_TYPE_ADVANCED_LAND].PlanDefenses(total_assets_land / 2, &*obtain_units_task, unit_counts);
    managers[DEFENSE_TYPE_SEA_STEALTH].PlanDefenses(total_assets_stealth_sea, &*obtain_units_task, unit_counts);
    managers[DEFENSE_TYPE_BASIC_SEA].PlanDefenses(total_assets_sea / 2, &*obtain_units_task, unit_counts);
    managers[DEFENSE_TYPE_ADVANCED_SEA].PlanDefenses(total_assets_sea / 2, &*obtain_units_task, unit_counts);

    TaskManager.AppendTask(*obtain_units_task);
}

bool TaskDefenseReserve::Execute(UnitInfo& unit) {
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
    for (int32_t i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        managers[i].ClearUnitsList();
    }

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskDefenseReserve::RemoveUnit(UnitInfo& unit) {
    for (int32_t i = 0; i < TASK_DEFENSE_MANAGER_COUNT; ++i) {
        managers[i].RemoveUnit(&unit);
    }
}
