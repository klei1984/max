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

#include "taskattack.hpp"

#include "access.hpp"
#include "aiattack.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "continent.hpp"
#include "inifile.hpp"
#include "paths_manager.hpp"
#include "randomizer.hpp"
#include "task_manager.hpp"
#include "taskrepair.hpp"
#include "tasktransport.hpp"
#include "units_manager.hpp"

TaskAttack::TaskAttack(SpottedUnit* spotted_unit, uint16_t task_flags)
    : Task(spotted_unit->GetTeam(), nullptr, task_flags) {
    op_state = ATTACK_STATE_INIT;
    attack_zone_reached = false;
    target_team = spotted_unit->GetUnit()->team;
    access_flags = 0;

    TaskKillUnit* task = new (std::nothrow) TaskKillUnit(this, spotted_unit, task_flags);

    primary_targets.PushBack(*task);
    secondary_targets.PushBack(*task);
}

TaskAttack::~TaskAttack() {}

bool TaskAttack::IsUnitUsable(UnitInfo& unit) { return !recon_unit && IsReconUnitUsable(&unit); }

int32_t TaskAttack::GetCautionLevel(UnitInfo& unit) {
    int32_t result;

    if (unit.GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
        ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        if (unit.shots > 0) {
            result = CAUTION_LEVEL_AVOID_REACTION_FIRE;

        } else {
            result = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
        }

    } else {
        if (unit.ammo > 0 || op_state >= ATTACK_STATE_BOLD_SEARCH) {
            result = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;

        } else {
            result = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
        }
    }

    return result;
}

uint16_t TaskAttack::GetFlags() const {
    uint16_t result;

    if (secondary_targets.GetCount()) {
        result = secondary_targets[0].GetFlags();

    } else {
        result = flags;
    }

    return result;
}

char* TaskAttack::WriteStatusLog(char* buffer) const {
    TaskKillUnit* task = kill_unit_task.Get();

    if (task && !task->GetUnitSpotted()) {
        task = nullptr;
    }

    if (!task && secondary_targets.GetCount() > 0) {
        task = &secondary_targets[0];
    }

    if (task && !task->GetUnitSpotted()) {
        task = nullptr;
    }

    if (task) {
        Point position;
        const char* status = " ";

        switch (op_state) {
            case ATTACK_STATE_INIT: {
                status = "Init. attack on ";
            } break;

            case ATTACK_STATE_WAIT: {
                status = "Wait to attack ";
            } break;

            case ATTACK_STATE_GATHER_FORCES: {
                status = "Gather attack on ";
            } break;

            case ATTACK_STATE_ADVANCE: {
                status = "Adv. on ";
            } break;

            case ATTACK_STATE_ADVANCE_USING_TRANSPORT: {
                status = "Adv. w/trans. on ";
            } break;

            case ATTACK_STATE_NORMAL_SEARCH: {
                status = "Search for ";
            } break;

            case ATTACK_STATE_BOLD_SEARCH: {
                status = "Bold search for ";
            } break;

            case ATTACK_STATE_ATTACK: {
                status = "Attack ";
            } break;

            default: {
                SDL_assert(0);
            }
        }

        position = task->DeterminePosition();

        sprintf(buffer, "%s %s at [%i,%i]", status,
                UnitsManager_BaseUnits[task->GetUnitSpotted()->GetUnitType()].GetSingularName(), position.x + 1,
                position.y + 1);

        if (leader) {
            char text[100];

            sprintf(text, ", leader %s %i at [%i,%i]", UnitsManager_BaseUnits[leader->GetUnitType()].GetSingularName(),
                    leader->unit_id, leader->grid_x + 1, leader->grid_y + 1);

            strcat(buffer, text);
        }

    } else {
        strcpy(buffer, "Completed attack task.");
    }

    return buffer;
}

Rect* TaskAttack::GetBounds(Rect* bounds) {
    if (kill_unit_task) {
        kill_unit_task->GetBounds(bounds);

    } else {
        Task::GetBounds(bounds);
    }

    return bounds;
}

uint8_t TaskAttack::GetType() const { return TaskType_TaskAttack; }

bool TaskAttack::IsNeeded() { return primary_targets.GetCount() > 0; }

void TaskAttack::AddUnit(UnitInfo& unit) {
    auto unit_type{unit.GetUnitType()};
    int32_t unit_index = managed_unit_types->Find(&unit_type);

    if (unit_index >= 0) {
        managed_unit_types.Remove(unit_index);
    }

    if (!recon_unit && IsReconUnitUsable(&unit)) {
        AILOG(log, "Task Attack: add spotter.");

        recon_unit = unit;
        unit.AddTask(this);

        if (!IsScheduledForTurnStart()) {
            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this));
        }

        Task_RemindMoveFinished(&unit, true);

        recon_unit->attack_site.x = 0;
        recon_unit->attack_site.y = 0;

    } else {
        TaskManager.ClearUnitTasksAndRemindAvailable(&unit);
    }
}

void TaskAttack::Begin() {
    access_flags = MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT;

    TaskManager.AppendTask(primary_targets[0]);

    primary_targets[0].GetSpottedUnit()->SetTask(this);

    RemindTurnStart(true);
}

void TaskAttack::BeginTurn() {
    if (op_state == ATTACK_STATE_INIT || !Randomizer_Generate(21)) {
        if (EvaluateLandAttack()) {
            access_flags = MOBILE_AIR_UNIT | MOBILE_LAND_UNIT;

        } else {
            access_flags = MOBILE_AIR_UNIT | MOBILE_SEA_UNIT;
        }
    }

    AssessEnemyUnits();

    attack_zone_reached = false;

    if (secondary_targets.GetCount()) {
        ChooseFirstTarget();
        UpdateReconUnit();
        EvaluateAttackReadiness();

        if (!support_attack_task && secondary_targets.GetCount() > 5) {
            support_attack_task = new (std::nothrow) TaskSupportAttack(this);

            TaskManager.AppendTask(*support_attack_task);
        }
    }
}

void TaskAttack::ChildComplete(Task* task) {
    if (task->GetType() == TaskType_TaskKillUnit) {
        AILOG(log, "Task Attack: kill unit task is complete.");

        secondary_targets.Remove(*dynamic_cast<TaskKillUnit*>(task));

        if (kill_unit_task == dynamic_cast<TaskKillUnit*>(task)) {
            kill_unit_task = nullptr;
        }

        if (secondary_targets.GetCount() > 0) {
            if (!kill_unit_task || !kill_unit_task->GetUnitSpotted()) {
                ChooseFirstTarget();
            }

        } else {
            Finish();
        }
    }
}

void TaskAttack::EndTurn() {
    EvaluateAttackReadiness();

    if (recon_unit && recon_unit->speed > 0) {
        Task_RemindMoveFinished(&*recon_unit, true);
    }

    if (support_attack_task) {
        for (SmartList<UnitInfo>::Iterator it = support_attack_task->GetUnits().Begin();
             it != support_attack_task->GetUnits().End(); ++it) {
            if ((*it).speed > 0 && Task_IsReadyToTakeOrders(&*it) && (*it).GetTask() == this) {
                Task_RemindMoveFinished(&*it, true);
            }
        }
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End(); ++it2) {
            if ((*it2).speed > 0 && Task_IsReadyToTakeOrders(&*it2) && (*it2).GetTask() == this) {
                Task_RemindMoveFinished(&*it2, true);
            }
        }
    }
}

bool TaskAttack::ExchangeOperator(UnitInfo& unit) {
    bool result;

    if (recon_unit && kill_unit_task) {
        Point position = kill_unit_task->DeterminePosition();

        if (Access_GetSquaredDistance(&unit, position) < Access_GetSquaredDistance(&*recon_unit, position) &&
            IsReconUnitUsable(&unit)) {
            SmartPointer<UnitInfo> copy(recon_unit);

            if (leader == recon_unit) {
                leader->RemoveDelayedTask(this);
                leader = unit;
                leader->AddDelayedTask(this);
            }

            recon_unit = unit;

            unit.AddTask(this);

            unit.attack_site.x = 0;
            unit.attack_site.y = 0;

            Task_RemindMoveFinished(&unit);

            TaskManager.ClearUnitTasksAndRemindAvailable(&*copy);

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::Execute(UnitInfo& unit) {
    bool result;

    if (secondary_targets.GetCount() > 0) {
        if (unit.speed > 0 && unit.IsReadyForOrders(this)) {
            AILOG(log, "Task Attack: move finished.");

            if ((unit.hits < unit.GetBaseValues()->GetAttribute(ATTRIB_HITS) / 4) ||
                ((leader == unit || recon_unit == unit) &&
                 (unit.hits < unit.GetBaseValues()->GetAttribute(ATTRIB_HITS) / 2))) {
                unit.RemoveTasks();

                SmartPointer<Task> repair_task(new (std::nothrow) TaskRepair(&unit));

                TaskManager.AppendTask(*repair_task);

                result = true;

            } else {
                result = MoveCombatUnit(this, &unit);
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

void TaskAttack::RemoveSelf() {
    if (support_attack_task) {
        support_attack_task->RemoveSelf();
    }

    if (leader) {
        leader->RemoveDelayedTask(this);
        TaskManager.ClearUnitTasksAndRemindAvailable(&*leader);
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        (*it).GetSpottedUnit()->SetTask(nullptr);
        (*it).RemoveSelf();
    }

    primary_targets.Clear();
    secondary_targets.Clear();
    kill_unit_task = nullptr;
    parent = nullptr;
    support_attack_task = nullptr;
    leader = nullptr;
    leader_task = nullptr;
    recon_unit = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskAttack::RemoveUnit(UnitInfo& unit) {
    if (recon_unit == unit) {
        recon_unit = nullptr;
    }

    if (leader == unit) {
        leader->RemoveDelayedTask(this);
        leader = nullptr;
        leader_task = nullptr;
    }

    if (!IsScheduledForTurnStart()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this));
    }
}

uint32_t TaskAttack::GetAccessFlags() const { return access_flags; }

int32_t TaskAttack::GetHighestScan() {
    int32_t unit_scan = 0;

    if (recon_unit) {
        unit_scan = recon_unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End(); ++it2) {
            int32_t scan = (*it2).GetBaseValues()->GetAttribute(ATTRIB_SCAN);

            if (scan > unit_scan) {
                unit_scan = scan;
            }
        }
    }

    return unit_scan;
}

bool TaskAttack::MoveCombatUnit(Task* task, UnitInfo* unit) {
    int32_t caution_level;
    bool result;

    AILOG(log, "Task Attack: move combat unit.");

    if (op_state > ATTACK_STATE_GATHER_FORCES) {
        caution_level = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;

    } else {
        caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
    }

    if (Task_RetreatFromDanger(task, unit, caution_level)) {
        result = true;

    } else {
        if (op_state >= ATTACK_STATE_ATTACK && unit->ammo > 0) {
            if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
                Point site;
                Point position = kill_unit_task->DeterminePosition();
                int32_t unit_range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE) +
                                     unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED);
                int32_t projected_damage;

                if (Access_GetSquaredDistance(unit, position) <= unit_range * unit_range) {
                    unit_range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
                }

                if (AiAttack_ChooseSiteForAttacker(unit, position, &site, &projected_damage,
                                                   CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, unit_range)) {
                    unit->attack_site = site;

                    if (unit->grid_x == unit->attack_site.x && unit->grid_y == unit->attack_site.y) {
                        return false;
                    }

                    if (projected_damage < unit->hits) {
                        SmartPointer<TaskMove> move_task(
                            new (std::nothrow) TaskMove(unit, task, 0, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, site,
                                                        &TaskTransport_MoveFinishedCallback));

                        TaskManager.AppendTask(*move_task);

                        return true;
                    }

                } else {
                    AILOG_LOG(log, "Attack with {} impossible.",
                              UnitsManager_BaseUnits[unit->GetUnitType()].GetSingularName());

                    TaskManager.ClearUnitTasksAndRemindAvailable(unit);

                    return false;
                }

            } else {
                return false;
            }
        }

        AILOG_LOG(log, "Attack square found, but it's too dangerous.");

        if (DetermineLeader()) {
            if (leader != unit) {
                if (Task_IsReadyToTakeOrders(&*leader)) {
                    if (unit->grid_x == unit->attack_site.x && unit->grid_y == unit->attack_site.y) {
                        result = false;

                    } else {
                        AILOG_LOG(log, "Grouping unit with {} at [{},{}]",
                                  UnitsManager_BaseUnits[leader->GetUnitType()].GetSingularName(), leader->grid_x + 1,
                                  leader->grid_y + 1);

                        if (Access_GetApproximateDistance(&*leader, unit) / 2 >
                            unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) {
                            if (MoveUnit(task, unit, GetLeaderDestination(), caution_level)) {
                                result = true;

                            } else {
                                EvaluateAttackReadiness();

                                result = false;
                            }

                        } else {
                            FindNewSiteForUnit(unit);

                            if (unit->grid_x == unit->attack_site.x && unit->grid_y == unit->attack_site.y) {
                                EvaluateAttackReadiness();

                                result = false;

                            } else {
                                SmartPointer<TaskMove> move_task(
                                    new (std::nothrow) TaskMove(unit, task, 0, caution_level, unit->attack_site,
                                                                &TaskTransport_MoveFinishedCallback));

                                move_task->SetField68(true);

                                TaskManager.AppendTask(*move_task);

                                result = true;
                            }
                        }
                    }

                } else {
                    result = false;
                }

            } else {
                result = PlanMoveForReconUnit();
            }

        } else {
            result = false;
        }
    }

    return result;
}

bool TaskAttack::IsDestinationReached(UnitInfo* unit) {
    return unit->grid_x == unit->attack_site.x && unit->grid_y == unit->attack_site.y;
}

UnitInfo* TaskAttack::DetermineLeader() {
    UnitInfo* result;

    if (leader && IsViableLeader(&*leader)) {
        result = &*leader;

    } else {
        Point position;
        SmartPointer<UnitInfo> candidate(leader);
        int32_t distance;
        int32_t minimum_distance{INT32_MAX};

        attack_zone_reached = false;

        if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
            position = kill_unit_task->DeterminePosition();

        } else {
            position.x = 0;
            position.y = 0;
        }

        leader = recon_unit;
        leader_task = this;

        if (recon_unit) {
            minimum_distance = Access_GetSquaredDistance(&*recon_unit, position);
        }

        for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
            for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End(); ++it2) {
                if (IsViableLeader(&*it2)) {
                    distance = Access_GetSquaredDistance(&*it2, position);

                    if (!recon_unit || distance < minimum_distance) {
                        leader = &*it2;
                        leader_task = &*it;
                        minimum_distance = distance;
                    }
                }
            }
        }

        if (!leader) {
            for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End();
                 ++it) {
                for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End();
                     ++it2) {
                    distance = Access_GetSquaredDistance(&*it2, position);

                    if (!leader || distance < minimum_distance) {
                        leader = &*it2;
                        leader_task = &*it;
                        minimum_distance = distance;
                    }
                }
            }
        }

        if (candidate) {
            candidate->RemoveDelayedTask(this);
        }

        if (leader) {
            leader->AddDelayedTask(this);
        }

        result = &*leader;
    }

    return result;
}

bool TaskAttack::EvaluateLandAttack() {
    UnitInfo* unit = nullptr;
    bool result;

    AILOG(log, "Task Attack: Deciding if land attack is possible.");

    for (uint32_t i = 0; !unit && i < primary_targets.GetCount(); ++i) {
        unit = primary_targets[i].GetUnitSpotted();
    }

    if (unit) {
        WeightTable weight_table;
        bool sea_unit_present = false;
        uint16_t task_flags = GetFlags();

        weight_table = AiPlayer_Teams[team].GetExtendedWeightTable(unit, 0x03);

        for (uint32_t i = 0; !unit && i < weight_table.GetCount(); ++i) {
            if (weight_table[i].weight > 0 &&
                !(UnitsManager_BaseUnits[weight_table[i].unit_type].flags & MOBILE_LAND_UNIT) &&
                (UnitsManager_BaseUnits[weight_table[i].unit_type].flags & MOBILE_SEA_UNIT)) {
                sea_unit_present = true;
            }
        }

        if (sea_unit_present) {
            AccessMap access_map;
            bool stationary_unit_present = false;
            Point position;
            Rect bounds;
            int32_t worth_of_land_units;
            int32_t worth_of_sea_units;

            rect_init(&bounds, 0, 0, 0, 0);

            for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                    if (ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * y + x] == SURFACE_TYPE_LAND) {
                        access_map.GetMapColumn(x)[y] = 2;
                    }
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
                 it != UnitsManager_GroundCoverUnits.End(); ++it) {
                if ((*it).GetUnitType() != BRIDGE && (*it).IsVisibleToTeam(team)) {
                    access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] = 2;
                }
            }

            for (SmartList<TaskKillUnit>::Iterator it = primary_targets.Begin(); it != primary_targets.End(); ++it) {
                SpottedUnit* spotted_unit = (*it).GetSpottedUnit();

                if (spotted_unit) {
                    position = spotted_unit->GetLastPosition();

                    if (access_map.GetMapColumn(position.x)[position.y] == 2) {
                        SmartPointer<Continent> continent(new (std::nothrow)
                                                              Continent(access_map.GetMap(), 3, position, 0));
                        continent->GetBounds(bounds);
                    }
                }
            }

            worth_of_land_units = 0;
            worth_of_sea_units = 0;

            weight_table = AiPlayer_Teams[team].GetExtendedWeightTable(unit, 0x01);

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team && (*it).hits > 0 && (*it).ammo > 0 &&
                    (*it).ammo >= (*it).GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) &&
                    !((*it).flags & MOBILE_AIR_UNIT) &&
                    (((*it).flags & MOBILE_SEA_UNIT) || access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] == 3) &&
                    ((*it).GetOrder() == ORDER_AWAIT || (*it).GetOrder() == ORDER_SENTRY ||
                     (*it).GetOrder() == ORDER_MOVE || (*it).GetOrder() == ORDER_MOVE_TO_UNIT)) {
                    bool is_relevant = false;

                    if ((*it).GetTask()) {
                        if ((*it).GetTask()->DeterminePriority(task_flags) > 0) {
                            is_relevant = (*it).GetTask()->Task_vfunc1(*it);
                        }

                    } else {
                        is_relevant = true;
                    }

                    if (is_relevant && weight_table.GetWeight((*it).GetUnitType())) {
                        if ((*it).flags & MOBILE_SEA_UNIT) {
                            worth_of_sea_units += (*it).GetBaseValues()->GetAttribute(ATTRIB_TURNS);

                        } else {
                            worth_of_land_units += (*it).GetBaseValues()->GetAttribute(ATTRIB_TURNS);
                        }
                    }
                }
            }

            AILOG_LOG(log, "Land unit value: {}, sea unit value {}", worth_of_land_units, worth_of_sea_units);

            if (worth_of_sea_units > 0 || worth_of_land_units > 0) {
                result = worth_of_land_units > worth_of_sea_units;

            } else {
                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team == team && (*it).GetUnitType() == LANDPLT) {
                        stationary_unit_present = true;

                        if (access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] == 3) {
                            return true;
                        }
                    }
                }

                if (!stationary_unit_present) {
                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                         it != UnitsManager_StationaryUnits.End(); ++it) {
                        if ((*it).team == team && access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] == 3) {
                            return true;
                        }
                    }
                }

                result = false;
            }

        } else {
            AILOG_LOG(log, "Must attack via land, targets are not near water");

            result = true;
        }

    } else {
        result = true;
    }

    return result;
}

void TaskAttack::Finish() {
    AILOG(log, "Task Attack: finish.");

    if (support_attack_task) {
        support_attack_task->RemoveSelf();
        support_attack_task = nullptr;
    }

    if (recon_unit) {
        TaskManager.ClearUnitTasksAndRemindAvailable(&*recon_unit);
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        (*it).GetSpottedUnit()->SetTask(nullptr);
        (*it).RemoveSelf();
    }

    primary_targets.Clear();
    secondary_targets.Clear();

    TaskManager.RemoveTask(*this);
}

bool TaskAttack::RequestReconUnit(ResourceID unit_type, int32_t safe_distance) {
    bool result;

    if (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_SCAN) >=
        safe_distance) {
        if (recon_unit) {
            if (recon_unit->GetUnitType() == unit_type) {
                return true;
            }

            TaskManager.ClearUnitTasksAndRemindAvailable(&*recon_unit);

            recon_unit = nullptr;
        }

        if (managed_unit_types->Find(&unit_type) >= 0) {
            result = true;

        } else {
            SmartPointer<TaskObtainUnits> obtain_units_task(new (std::nothrow)
                                                                TaskObtainUnits(this, DeterminePosition()));

            obtain_units_task->AddUnit(unit_type);

            managed_unit_types.PushBack(&unit_type);

            TaskManager.AppendTask(*obtain_units_task);

            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::FindReconUnit(ResourceID unit_type, int32_t safe_distance) {
    bool result;

    if (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_SCAN) >=
        safe_distance) {
        SmartList<UnitInfo>* units{nullptr};
        UnitInfo* new_recon_unit{nullptr};
        uint16_t task_flags = GetFlags();

        if (UnitsManager_BaseUnits[unit_type].flags & MOBILE_AIR_UNIT) {
            units = &UnitsManager_MobileAirUnits;

        } else {
            units = &UnitsManager_MobileLandSeaUnits;
        }

        for (auto it = units->Begin(); it != units->End(); ++it) {
            if ((*it).team == team && (*it).GetUnitType() == unit_type &&
                (*it).GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_distance &&
                (!(*it).GetTask() || (*it).GetTask()->DeterminePriority(task_flags) > 0)) {
                if (!new_recon_unit || (new_recon_unit->GetTask() && !(*it).GetTask())) {
                    new_recon_unit = &*it;
                }
            }
        }

        if (new_recon_unit) {
            if (recon_unit) {
                TaskManager.ClearUnitTasksAndRemindAvailable(&*recon_unit);
            }

            new_recon_unit->RemoveTasks();
            new_recon_unit->AddTask(this);
            recon_unit = new_recon_unit;

            recon_unit->attack_site.x = 0;
            recon_unit->attack_site.y = 0;

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::IsReconUnitUsable(UnitInfo* unit) {
    bool result;

    if (unit->GetUnitType() == AWAC || unit->GetUnitType() == SCOUT || unit->GetUnitType() == SCANNER ||
        unit->GetUnitType() == FASTBOAT) {
        int32_t safe_distance_ground;
        int32_t safe_distance_air;

        GetSafeDistances(&safe_distance_air, &safe_distance_ground);

        if (unit->GetUnitType() == AWAC) {
            result = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_distance_air;

        } else {
            result = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_distance_ground;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::IsWithinAttackRange(UnitInfo* unit, Point unit_position) {
    bool result;

    if (unit && unit->ammo > 0) {
        int32_t distance = unit->GetAttackRange() - 3;

        distance = distance * distance;

        result = Access_GetSquaredDistance(unit, unit_position) <= distance;

    } else {
        result = false;
    }

    return result;
}

void TaskAttack::GetSafeDistances(int32_t* safe_distance_air, int32_t* safe_distance_ground) {
    *safe_distance_air = 0;
    *safe_distance_ground = 0;

    if (kill_unit_task) {
        UnitInfo* unit = kill_unit_task->GetUnitSpotted();
        Point unit_position(unit->grid_x, unit->grid_y);
        int32_t safe_air_distance;
        int32_t safe_ground_distance;

        for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
            unit = (*it).GetUnitSpotted();

            if (unit) {
                UnitValues* unit_values = unit->GetBaseValues();

                if (unit_values->GetAttribute(ATTRIB_ATTACK) > 0) {
                    if (&*it == &*kill_unit_task || IsWithinAttackRange(unit, unit_position)) {
                        if (Access_IsValidAttackTargetType(unit->GetUnitType(), AWAC)) {
                            *safe_distance_air =
                                std::max(*safe_distance_air, unit_values->GetAttribute(ATTRIB_RANGE) + 1);

                        } else {
                            *safe_distance_ground =
                                std::max(*safe_distance_ground, unit_values->GetAttribute(ATTRIB_RANGE) + 1);
                        }
                    }
                }
            }
        }

        safe_air_distance =
            *safe_distance_air -
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], AWAC)->GetAttribute(ATTRIB_SCAN);

        if (access_flags & MOBILE_LAND_UNIT) {
            safe_ground_distance =
                *safe_distance_ground -
                UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], SCANNER)->GetAttribute(ATTRIB_SCAN);

        } else {
            safe_ground_distance =
                *safe_distance_ground -
                UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], FASTBOAT)->GetAttribute(ATTRIB_SCAN);
        }

        if (safe_air_distance > 0 && safe_ground_distance > 0) {
            int32_t safe_distane = std::min(safe_air_distance, safe_ground_distance);

            *safe_distance_air -= safe_distane;
            *safe_distance_ground -= safe_distane;
        }
    }
}

void TaskAttack::UpdateReconUnit() {
    if (kill_unit_task) {
        int32_t safe_air_distance;
        int32_t safe_ground_distance;

        AILOG(log, "Task Attack: Update recon distances.");

        GetSafeDistances(&safe_air_distance, &safe_ground_distance);

        if (recon_unit) {
            if (recon_unit->GetUnitType() == AWAC) {
                if (recon_unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_air_distance) {
                    return;
                }

            } else {
                if (recon_unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) >= safe_ground_distance) {
                    return;
                }
            }
        }

        AILOG_LOG(log, "Choose proper spotting unit.");

        if (access_flags & MOBILE_SEA_UNIT) {
            if (!FindReconUnit(FASTBOAT, safe_ground_distance) && !FindReconUnit(AWAC, safe_air_distance)) {
                RequestReconUnit(FASTBOAT, safe_ground_distance);
                RequestReconUnit(AWAC, safe_air_distance);
            }

        } else if (access_flags & MOBILE_LAND_UNIT) {
            if (!FindReconUnit(SCOUT, safe_ground_distance) && !FindReconUnit(SCANNER, safe_ground_distance) &&
                !FindReconUnit(AWAC, safe_air_distance) && !RequestReconUnit(SCOUT, safe_ground_distance)) {
                RequestReconUnit(SCANNER, safe_ground_distance);
                RequestReconUnit(AWAC, safe_air_distance);
            }

        } else {
            if (!FindReconUnit(AWAC, safe_air_distance)) {
                RequestReconUnit(AWAC, 0);
            }
        }
    }
}

void TaskAttack::ChooseFirstTarget() {
    AILOG(log, "Task Attack: Deciding which target to attack first.");

    if (secondary_targets.GetCount() > 0) {
        kill_unit_task = secondary_targets[0];

        if (attack_zone_reached && leader) {
            if (leader->speed == leader->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) {
                TaskKillUnit* task = &*kill_unit_task;
                secondary_targets.Remove(*kill_unit_task);
                secondary_targets.PushBack(*kill_unit_task);
                kill_unit_task = secondary_targets[0];

                if (kill_unit_task != task) {
                    attack_zone_reached = false;
                }
            }
        }
    }

    if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
        AILOG_LOG(log, "Chose to attack {}",
                  UnitsManager_BaseUnits[kill_unit_task->GetUnitSpotted()->GetUnitType()].GetSingularName());
    } else {
        AILOG_LOG(log, "No target chosen.");
    }
}

void TaskAttack::CopyTargets(TaskAttack* other) {
    for (SmartList<TaskKillUnit>::Iterator it = other->primary_targets.Begin(); it != other->primary_targets.End();
         ++it) {
        primary_targets.PushBack(*it);
    }

    for (SmartList<TaskKillUnit>::Iterator it = other->secondary_targets.Begin(); it != other->secondary_targets.End();
         ++it) {
        (*it).SetParent(this);
        (*it).GetSpottedUnit()->SetTask(this);
        secondary_targets.PushBack(*it);
    }
}

bool TaskAttack::IsTargetGroupInSight() {
    bool result;

    if (secondary_targets.GetCount() > 0) {
        uint16_t unit_team = secondary_targets[0].GetUnitSpotted()->team;

        for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
            for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End(); ++it2) {
                if ((*it2).IsVisibleToTeam(unit_team)) {
                    return true;
                }
            }
        }

        result = false;

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::IsThereTimeToPrepare() {
    bool result;

    if (op_state <= ATTACK_STATE_GATHER_FORCES) {
        if (IsTargetGroupInSight()) {
            if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
                result = !kill_unit_task->GetUnitSpotted()->IsVisibleToTeam(team);

            } else {
                result = false;
            }

        } else {
            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::MoveUnit(Task* task, UnitInfo* unit, Point site, int32_t caution_level) {
    Point target_position(unit->grid_x, unit->grid_y);
    Point position;
    auto info_map = AiPlayer_Teams[team].GetInfoMap();
    int16_t** damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(unit, caution_level, false);
    Rect bounds;
    ResourceID transporter = INVALID_ID;
    bool result;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (Task_GetReadyUnitsCount(team, AIRTRANS) > 0) {
        transporter = AIRTRANS;

    } else if (Task_GetReadyUnitsCount(team, SEATRANS) > 0) {
        transporter = SEATRANS;

    } else if ((unit->GetUnitType() == COMMANDO || unit->GetUnitType() == INFANTRY) &&
               (Task_GetReadyUnitsCount(team, CLNTRANS) > 0)) {
        transporter = CLNTRANS;

    } else if (Task_GetReadyUnitsCount(team, AIRPLT) > 0) {
        transporter = AIRTRANS;

    } else if (Task_GetReadyUnitsCount(team, SHIPYARD) > 0) {
        transporter = SEATRANS;
    }

    AILOG(log, "Task Attack: Move {} at [{},{}] near [{},{}].",
          UnitsManager_BaseUnits[unit->GetUnitType()].GetSingularName(), target_position.x + 1, target_position.y + 1,
          site.x + 1, site.y + 1);

    TransporterMap map(unit, 0x01, caution_level, transporter);
    bool is_there_time_to_prepare = IsThereTimeToPrepare();

    if (damage_potential_map) {
        if (secondary_targets.GetCount() > 0) {
            if (!Task_RetreatFromDanger(this, unit, caution_level)) {
                if (secondary_targets[0].GetUnitSpotted()) {
                    uint16_t unit_team = secondary_targets[0].GetUnitSpotted()->team;
                    int8_t* heat_map;
                    int32_t minimum_distance;
                    int32_t unit_hits;
                    int32_t distance;

                    if (unit->GetUnitType() == COMMANDO) {
                        heat_map = UnitsManager_TeamInfo[unit_team].heat_map_stealth_land;

                    } else if (UnitsManager_IsUnitUnderWater(unit)) {
                        heat_map = UnitsManager_TeamInfo[unit_team].heat_map_stealth_sea;

                    } else {
                        heat_map = UnitsManager_TeamInfo[unit_team].heat_map_complete;
                    }

                    minimum_distance = Access_GetApproximateDistance(target_position, site) / 2;

                    unit_hits = unit->hits;

                    if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
                        unit_hits = 1;
                    }

                    for (int32_t i = 1; i < minimum_distance; ++i) {
                        position.x = site.x - i;
                        position.y = site.y + i;

                        for (int32_t direction = 0; direction < 8; direction += 2) {
                            for (int32_t range = 0; range < i * 2; ++range) {
                                position += Paths_8DirPointsArray[direction];

                                if (Access_IsInsideBounds(&bounds, &position) &&
                                    damage_potential_map[position.x][position.y] < unit_hits &&
                                    (!is_there_time_to_prepare || !heat_map ||
                                     heat_map[ResourceManager_MapSize.x * position.y + position.x] == 0)) {
                                    distance = (Access_GetApproximateDistance(position, site) / 2) +
                                               (Access_GetApproximateDistance(unit->grid_x - position.x,
                                                                              unit->grid_y - position.y) /
                                                4);

                                    if (distance < minimum_distance &&
                                        !(info_map[position.x][position.y] & INFO_MAP_CLEAR_OUT_ZONE) &&
                                        map.Search(position)) {
                                        if ((!(access_flags & MOBILE_LAND_UNIT) ||
                                             Access_GetModifiedSurfaceType(position.x, position.y) !=
                                                 SURFACE_TYPE_WATER) &&
                                            Access_IsAccessible(unit->GetUnitType(), team, position.x, position.y,
                                                                AccessModifier_SameClassBlocks)) {
                                            target_position = position;
                                            minimum_distance = distance;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    unit->attack_site = target_position;

                    if (unit->grid_x == target_position.x && unit->grid_y == target_position.y) {
                        if (leader == unit) {
                            if (op_state == ATTACK_STATE_NORMAL_SEARCH) {
                                op_state = ATTACK_STATE_BOLD_SEARCH;

                                MoveUnits();

                            } else {
                                attack_zone_reached = true;

                                EvaluateAttackReadiness();
                                ChooseFirstTarget();
                            }
                        }

                        result = false;

                    } else {
                        if (unit->ammo > 0) {
                            caution_level = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
                        }

                        if (op_state < ATTACK_STATE_NORMAL_SEARCH && leader == unit) {
                            for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin();
                                 it != secondary_targets.End(); ++it) {
                                for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin();
                                     it2 != (*it).GetUnits().End(); ++it2) {
                                    (*it2).attack_site.x = 0;
                                    (*it2).attack_site.y = 0;
                                }
                            }
                        }

                        SmartPointer<TaskMove> move_task(new (std::nothrow) TaskMove(
                            unit, task, 0, caution_level, target_position, &TaskTransport_MoveFinishedCallback));

                        move_task->SetField68(true);

                        TaskManager.AppendTask(*move_task);

                        result = true;
                    }

                } else {
                    result = false;
                }

            } else {
                result = true;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

Point TaskAttack::FindClosestDirectRoute(UnitInfo* unit, int32_t caution_level) {
    Point target_position(kill_unit_task->DeterminePosition());
    Point unit_position(unit->grid_x, unit->grid_y);
    Point best_site(unit_position);
    Point site;
    AccessMap map;
    bool is_there_time_to_prepare = IsThereTimeToPrepare();
    int32_t surface_type;
    uint16_t unit_team = secondary_targets[0].GetUnitSpotted()->team;
    int8_t* heat_map;
    Rect bounds;
    int32_t distance;
    int32_t minimum_distance;

    AILOG(log, "Find destination that does not require transport.");

    if (unit->flags & MOBILE_LAND_UNIT) {
        surface_type = SURFACE_TYPE_LAND;

    } else {
        surface_type = SURFACE_TYPE_WATER;
    }

    if (unit->GetUnitType() == COMMANDO) {
        heat_map = UnitsManager_TeamInfo[unit_team].heat_map_stealth_land;

    } else if (UnitsManager_IsUnitUnderWater(unit)) {
        heat_map = UnitsManager_TeamInfo[unit_team].heat_map_stealth_sea;

    } else {
        heat_map = UnitsManager_TeamInfo[unit_team].heat_map_complete;
    }

    for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
        for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
            if (ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * site.y + site.x] == surface_type) {
                if (!is_there_time_to_prepare || !heat_map ||
                    heat_map[ResourceManager_MapSize.x * site.y + site.x] == 0) {
                    map.GetMapColumn(site.x)[site.y] = 2;
                }
            }
        }
    }

    if ((unit->flags & MOBILE_LAND_UNIT) && (unit->flags & MOBILE_SEA_UNIT)) {
        for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
            for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
                if (ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * site.y + site.x] == SURFACE_TYPE_WATER) {
                    if (!is_there_time_to_prepare || !heat_map ||
                        heat_map[ResourceManager_MapSize.x * site.y + site.x] == 0) {
                        map.GetMapColumn(site.x)[site.y] = 2;
                    }
                }
            }
        }
    }

    PathsManager_ApplyCautionLevel(map.GetMap(), unit, caution_level);

    SmartPointer<Continent> continent(new (std::nothrow) Continent(map.GetMap(), 0x03, best_site));

    rect_init(&bounds, 0, 0, 0, 0);

    continent->GetBounds(bounds);

    minimum_distance = Access_GetApproximateDistance(best_site, target_position) / 2;

    for (int32_t i = 1; i < minimum_distance; ++i) {
        site.x = target_position.x - i;
        site.y = target_position.y + i;

        for (int32_t direction = 0; direction < 8; direction += 2) {
            for (int32_t range = 0; range < i * 2; ++range) {
                site += Paths_8DirPointsArray[direction];

                if (Access_IsInsideBounds(&bounds, &site) && map.GetMapColumn(site.x)[site.y] == 3) {
                    distance = (Access_GetApproximateDistance(site, target_position) / 2) +
                               (Access_GetApproximateDistance(site, unit_position) / 6);

                    if (distance < minimum_distance &&
                        (!access_flags || Access_GetModifiedSurfaceType(site.x, site.y) != SURFACE_TYPE_WATER) &&
                        Access_IsAccessible(unit->GetUnitType(), team, site.x, site.y,
                                            AccessModifier_SameClassBlocks)) {
                        best_site = site;
                        minimum_distance = distance;
                    }
                }
            }
        }
    }

    AILOG_LOG(log, "Nearest location is [{},{}], distance {}.", best_site.x + 1, best_site.y + 1, minimum_distance);

    unit->attack_site = best_site;

    return best_site;
}

bool TaskAttack::MoveReconUnit(int32_t caution_level) {
    Point position = kill_unit_task->DeterminePosition();
    Point site;
    bool is_visible_to_team = kill_unit_task->GetUnitSpotted()->IsVisibleToTeam(team);
    bool result;

    AILOG(log, "Move recon unit.");

    if (op_state >= ATTACK_STATE_ADVANCE) {
        caution_level = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
    }

    if (!leader->ammo && !is_visible_to_team && op_state < ATTACK_STATE_BOLD_SEARCH) {
        caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
    }

    if (op_state == ATTACK_STATE_WAIT) {
        caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
    }

    if (!attack_zone_reached) {
        if ((leader->flags & MOBILE_AIR_UNIT) || op_state >= ATTACK_STATE_ADVANCE_USING_TRANSPORT ||
            op_state == ATTACK_STATE_WAIT) {
            result = MoveUnit(this, &*leader, position, caution_level);

        } else {
            site = FindClosestDirectRoute(&*leader, caution_level);

            if (leader->grid_x == site.x && leader->grid_y == site.y) {
                attack_zone_reached = true;

                EvaluateAttackReadiness();
                ChooseFirstTarget();

                result = false;

            } else {
                if (op_state < ATTACK_STATE_NORMAL_SEARCH) {
                    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin();
                         it != secondary_targets.End(); ++it) {
                        for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin();
                             it2 != (*it).GetUnits().End(); ++it2) {
                            (*it2).attack_site.x = 0;
                            (*it2).attack_site.y = 0;
                        }
                    }
                }

                leader->attack_site = site;

                SmartPointer<TaskMove> move_task(new (std::nothrow) TaskMove(&*leader, this, 0, caution_level, site,
                                                                             &TaskTransport_MoveFinishedCallback));

                move_task->SetField68(true);

                TaskManager.AppendTask(*move_task);

                result = true;
            }
        }

    } else {
        AILOG_LOG(log, "Previous destination was at leader's location.");

        result = false;
    }

    return result;
}

bool TaskAttack::IsAttackUnderControl() {
    int32_t projected_damage = 0;
    int32_t required_damage = 0;
    Point unit_location;
    bool result;

    if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
        UnitInfo* target = kill_unit_task->GetUnitSpotted();

        unit_location.x = target->grid_x;
        unit_location.y = target->grid_y;

        for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
            if ((&*it) == &*kill_unit_task || IsWithinAttackRange((*it).GetUnitSpotted(), unit_location)) {
                required_damage += (*it).GetRequiredDamage();
            }

            projected_damage += (*it).GetTotalProjectedDamage();
        }

        if (projected_damage >= required_damage) {
            if (!kill_unit_task->GetUnitSpotted()->IsVisibleToTeam(team)) {
                if (recon_unit && recon_unit->grid_x == recon_unit->attack_site.x &&
                    recon_unit->grid_y == recon_unit->attack_site.y) {
                    result = true;

                } else {
                    int32_t max_unit_worth = 0;
                    int32_t max_unit_scan = 0;
                    int32_t unit_worth;
                    int32_t unit_scan;

                    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin();
                         it != secondary_targets.End(); ++it) {
                        for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin();
                             it2 != (*it).GetUnits().End(); ++it2) {
                            unit_worth = AiAttack_GetTargetValue(&*it2);
                            unit_scan = (*it2).GetBaseValues()->GetAttribute(ATTRIB_SCAN);
                            max_unit_worth = std::max(max_unit_worth, unit_worth);
                            max_unit_scan = std::max(max_unit_scan, unit_scan);
                        }
                    }

                    result = max_unit_worth <= max_unit_scan;
                }

            } else {
                result = true;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::PlanMoveForReconUnit() {
    bool result;
    int32_t caution_level;

    AILOG(log, "Determine if moving recon unit is a good idea.");

    if (leader->speed > 0) {
        if (leader_task == leader->GetTask()) {
            if (leader->IsReadyForOrders(&*leader_task)) {
                caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;

                if (op_state >= ATTACK_STATE_ADVANCE) {
                    caution_level = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
                }

                if (!Task_RetreatFromDanger(this, &*leader, caution_level)) {
                    if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
                        if (op_state > ATTACK_STATE_GATHER_FORCES && op_state < ATTACK_STATE_NORMAL_SEARCH) {
                            if ((leader->flags & MOBILE_LAND_UNIT) &&
                                Access_GetModifiedSurfaceType(leader->grid_x, leader->grid_y) != SURFACE_TYPE_LAND &&
                                AiPlayer_Teams[team].GetStrategy() != AI_STRATEGY_SCOUT_HORDE) {
                                result = MoveReconUnit(caution_level);

                            } else {
                                leader->attack_site.x = leader->grid_x;
                                leader->attack_site.y = leader->grid_y;

                                if (IsAttackUnderControl()) {
                                    result = MoveReconUnit(caution_level);

                                } else {
                                    AILOG_LOG(log, "Not enough units nearby group yet.");

                                    result = false;
                                }
                            }

                        } else {
                            result = MoveReconUnit(caution_level);
                        }

                    } else {
                        AILOG_LOG(log, "No target.");

                        result = false;
                    }

                } else {
                    AILOG_LOG(log, "Leader is retreating.");

                    result = true;
                }

            } else {
                AILOG_LOG(log, "Leader is not ready for orders");

                result = false;
            }

        } else {
            AILOG_LOG(log, "Leader is under control of another task.");

            result = false;
        }

    } else {
        AILOG_LOG(log, "Leader has no movement");

        result = false;
    }

    return result;
}

bool TaskAttack::IsViableLeader(UnitInfo* unit) {
    bool result;

    if (unit->hits > 0) {
        if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
            if (kill_unit_task->GetUnitSpotted()->IsVisibleToTeam(team)) {
                if (unit->GetUnitType() == COMMANDO || unit->GetUnitType() == SUBMARNE) {
                    UnitInfo* target = kill_unit_task->GetUnitSpotted();
                    int32_t unit_scan = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);

                    result = Access_GetSquaredDistance(unit, target) <= unit_scan * unit_scan;

                } else {
                    result = true;
                }

            } else if (recon_unit && recon_unit != unit) {
                result = false;

            } else {
                result = AiAttack_GetTargetValue(unit) <= unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);
            }

        } else {
            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

Point TaskAttack::GetLeaderDestination() {
    Point result;

    if (leader->GetOrder() == ORDER_IDLE) {
        result = leader->attack_site;

    } else if (leader->speed > 0 && leader->path) {
        result = leader->path->GetPosition(&*leader);

    } else if (leader->speed > 0 && (leader->attack_site.x || leader->attack_site.y)) {
        result = leader->attack_site;

    } else {
        result.x = leader->grid_x;
        result.y = leader->grid_y;
    }

    return result;
}

void TaskAttack::FindNewSiteForUnit(UnitInfo* unit) {
    AccessMap access_map;
    auto info_map = AiPlayer_Teams[team].GetInfoMap();
    int32_t caution_level = unit->ammo > 0 ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE : CAUTION_LEVEL_AVOID_ALL_DAMAGE;

    PathsManager_InitAccessMap(unit, access_map.GetMap(), 0x01, caution_level);

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End(); ++it2) {
            if ((&*it2) != unit) {
                access_map.GetMapColumn((*it2).attack_site.x)[(*it2).attack_site.y] = 0;
            }
        }
    }

    if (support_attack_task) {
        for (SmartList<UnitInfo>::Iterator it = support_attack_task->GetUnits().Begin();
             it != support_attack_task->GetUnits().End(); ++it) {
            if ((&*it) != unit) {
                access_map.GetMapColumn((*it).attack_site.x)[(*it).attack_site.y] = 0;
            }
        }
    }

    if (recon_unit && recon_unit != unit) {
        access_map.GetMapColumn(recon_unit->attack_site.x)[recon_unit->attack_site.y] = 0;
    }

    Point position;
    Point destination = GetLeaderDestination();
    Point site(unit->grid_x, unit->grid_y);
    Point best_site(site);
    int32_t unit_angle = UnitsManager_GetTargetAngle(destination.x - site.x, destination.y - site.y);
    int32_t distance;
    int32_t minimum_distance = 50 * Access_GetApproximateDistance(destination, best_site);
    int32_t direction1;
    int32_t direction2;
    int32_t limit;

    for (int32_t i = 1; i < minimum_distance / 100; ++i) {
        direction1 = (unit_angle + 2) & 0x7;
        position.x = destination.x + Paths_8DirPointsArray[direction1].x * i;
        position.y = destination.y + Paths_8DirPointsArray[direction1].y * i;

        direction1 = (unit_angle + 3) & 0x6;
        direction2 = unit_angle & 0x01;

        for (; direction2 < 5; direction2 += 2) {
            switch (direction2) {
                case 0: {
                    limit = i;
                } break;

                case 1:
                case 2: {
                    limit = i * 2;
                } break;

                case 3: {
                    limit = i * 2 + 1;
                } break;

                case 4: {
                    limit = i + 1;
                } break;
            }

            for (int32_t j = 0; j < limit; ++j) {
                if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                    position.y < ResourceManager_MapSize.y && access_map.GetMapColumn(position.x)[position.y] > 0 &&
                    (!info_map || !(info_map[position.x][position.y] & INFO_MAP_CLEAR_OUT_ZONE))) {
                    distance = 50 * Access_GetApproximateDistance(position, destination) +
                               Access_GetApproximateDistance(position, site);

                    if (distance < minimum_distance) {
                        minimum_distance = distance;
                        best_site = position;
                    }
                }

                position += Paths_8DirPointsArray[direction1];
            }

            direction1 = (unit_angle + 2) & 0x7;
        }
    }

    unit->attack_site = best_site;
}

bool TaskAttack::MoveUnits() {
    bool result;

    if (DetermineLeader()) {
        Task_RemindMoveFinished(&*leader, true);
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        if ((*it).MoveUnits()) {
            return true;
        }
    }

    if (support_attack_task) {
        result = support_attack_task->AddReminders();

    } else {
        result = false;
    }

    return result;
}

bool TaskAttack::IsAnyTargetInRange() {
    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End(); ++it2) {
            int32_t unit_range = (*it2).GetBaseValues()->GetAttribute(ATTRIB_RANGE);

            unit_range = unit_range * unit_range;

            for (SmartList<TaskKillUnit>::Iterator it3 = secondary_targets.Begin(); it3 != secondary_targets.End();
                 ++it3) {
                if (Access_GetSquaredDistance(&*it2, (*it3).GetUnitSpotted()) <= unit_range) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool TaskAttack::IsReconUnitAvailable() {
    bool result;

    if (recon_unit) {
        result = true;

    } else {
        int32_t safe_distance_air = 0;
        int32_t safe_distance_ground = 0;
        UnitValues* unit_values;

        GetSafeDistances(&safe_distance_air, &safe_distance_ground);

        for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
            for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End(); ++it2) {
                unit_values = (*it2).GetBaseValues();

                if ((*it2).flags & MOBILE_AIR_UNIT) {
                    if (unit_values->GetAttribute(ATTRIB_SCAN) >= safe_distance_air) {
                        return true;
                    }

                } else {
                    if (unit_values->GetAttribute(ATTRIB_SCAN) >= safe_distance_ground) {
                        return true;
                    }
                }
            }
        }

        result = false;
    }

    return result;
}

void TaskAttack::EvaluateAttackReadiness() {
    int32_t required_damage = 0;
    int32_t projected_damage = 0;

    AILOG(log, "Considering readiness of attack.");

    if (op_state < ATTACK_STATE_WAIT) {
        op_state = ATTACK_STATE_WAIT;
    }

    if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
        if (!kill_unit_task->GetUnitSpotted()->IsVisibleToTeam(team) && !IsReconUnitAvailable()) {
            AILOG_LOG(log, "Task has no spotter.");

            op_state = ATTACK_STATE_WAIT;

            return;
        }

        UnitInfo* target = kill_unit_task->GetUnitSpotted();
        Point target_position(target->grid_x, target->grid_y);

        for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
            if ((&*it) == &*kill_unit_task || IsWithinAttackRange((*it).GetUnitSpotted(), target_position)) {
                required_damage += (*it).GetRequiredDamage();
            }

            projected_damage += (*it).GetProjectedDamage();
        }
    }

    if (projected_damage >= required_damage) {
        if (op_state == ATTACK_STATE_WAIT) {
            AILOG_LOG(log, "Enough units available.");

            op_state = ATTACK_STATE_GATHER_FORCES;
            attack_zone_reached = false;
        }

    } else {
        if (op_state > ATTACK_STATE_WAIT && (op_state < ATTACK_STATE_ATTACK || !IsAnyTargetInRange())) {
            AILOG_LOG(log, "Halt attack, too many losses.");

            op_state = ATTACK_STATE_WAIT;
        }
    }

    if (kill_unit_task && kill_unit_task->GetUnitSpotted()) {
        if (op_state == ATTACK_STATE_NORMAL_SEARCH && !IsTargetGroupInSight()) {
            AILOG_LOG(log, "Reverting to Advance, all units hidden.");

            op_state = ATTACK_STATE_ADVANCE;
        }

        if (op_state >= ATTACK_STATE_GATHER_FORCES && op_state < ATTACK_STATE_NORMAL_SEARCH) {
            if (DetermineLeader() && attack_zone_reached && Task_IsReadyToTakeOrders(&*leader) && leader->speed > 0 &&
                IsAttackUnderControl()) {
                AILOG_LOG(log, "Ready for next stage of attack.");

                ++op_state;
                attack_zone_reached = false;

                MoveUnits();
            }

            Point position = kill_unit_task->GetSpottedUnit()->GetLastPosition();

            for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End();
                 ++it) {
                for (SmartList<UnitInfo>::Iterator it2 = (*it).GetUnits().Begin(); it2 != (*it).GetUnits().End();
                     ++it2) {
                    int32_t unit_range = (*it2).GetAttackRange();

                    if (Access_GetSquaredDistance(&*it2, position) <= unit_range * unit_range) {
                        AILOG_LOG(log, "{} at [{},{}] is in assault range, shifting to attack mode.",
                                  UnitsManager_BaseUnits[(*it2).GetUnitType()].GetSingularName(), (*it2).grid_x + 1,
                                  (*it2).grid_y + 1);

                        op_state = ATTACK_STATE_NORMAL_SEARCH;
                        attack_zone_reached = false;

                        MoveUnits();
                    }
                }
            }
        }

        if (op_state == ATTACK_STATE_NORMAL_SEARCH || op_state == ATTACK_STATE_BOLD_SEARCH) {
            if (kill_unit_task->GetUnitSpotted()->IsVisibleToTeam(team)) {
                AILOG_LOG(log, "Target is visible.");

                op_state = ATTACK_STATE_ATTACK;
                attack_zone_reached = false;

                MoveUnits();
            }
        }

        if (op_state == ATTACK_STATE_ATTACK && !kill_unit_task->GetUnitSpotted()->IsVisibleToTeam(team)) {
            AILOG_LOG(log, "Target not visible, changing from Attack to Search.");

            op_state = ATTACK_STATE_NORMAL_SEARCH;
            attack_zone_reached = false;

            MoveUnits();
        }

    } else {
        if (op_state > ATTACK_STATE_WAIT) {
            AILOG_LOG(log, "Target is destroyed, halting attack.");

            op_state = ATTACK_STATE_WAIT;
        }
    }
}

bool TaskAttack::IsDefenderDangerous(SpottedUnit* spotted_unit) {
    UnitInfo* target = spotted_unit->GetUnit();
    bool result;

    if (target && target->ammo > 0) {
        int32_t unit_range = target->GetAttackRange() - 3;
        Point position = spotted_unit->GetLastPosition();

        unit_range = unit_range * unit_range;

        for (SmartList<TaskKillUnit>::Iterator it = primary_targets.Begin(); it != primary_targets.End(); ++it) {
            if ((*it).GetUnitSpotted()) {
                if (Access_GetSquaredDistance((*it).GetUnitSpotted(), position) <= unit_range) {
                    return true;
                }
            }
        }

        result = false;

    } else {
        result = false;
    }

    return result;
}

void TaskAttack::RemoveTask(TaskKillUnit* task) {
    primary_targets.Remove(*task);
    secondary_targets.Remove(*task);
    task->RemoveSelf();
}

void TaskAttack::AssessEnemyUnits() {
    bool teams[PLAYER_TEAM_MAX];
    uint16_t enemy_team;
    bool is_relevant;

    AILOG(log, "Assess which enemy units protect primary targets.");

    access_flags |= MOBILE_AIR_UNIT;

    AiAttack_GetTargetTeams(team, teams);

    enemy_team = AiPlayer_Teams[team].GetTargetTeam();

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        (*it).GetSpottedUnit()->SetTask(nullptr);
    }

    flags = 0x1F00;

    for (SmartList<TaskKillUnit>::Iterator it = primary_targets.Begin(); it != primary_targets.End(); ++it) {
        UnitInfo* target = (*it).GetUnitSpotted();

        if (target) {
            is_relevant = false;

            if ((target->flags & BUILDING) || target->GetUnitType() == CONSTRCT) {
                is_relevant = target->team == enemy_team;

                if (is_relevant && flags > 0x1700 &&
                    (target->GetUnitType() == GREENHSE || target->GetUnitType() == MININGST) &&
                    UnitsManager_TeamInfo[enemy_team].team_points > UnitsManager_TeamInfo[team].team_points) {
                    flags = 0x1700;
                }

            } else {
                if (target->team == PLAYER_TEAM_ALIEN) {
                    is_relevant = true;

                } else if (AiAttack_IsWithinReach(target, team, teams)) {
                    is_relevant = true;

                    flags = 0x1000;
                }
            }

            if (is_relevant) {
                (*it).GetSpottedUnit()->SetTask(this);

            } else {
                RemoveTask(&*it);
            }
        }
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        SpottedUnit* target = (*it).GetSpottedUnit();

        if (target->GetTask() == this || IsDefenderDangerous(target)) {
            target->SetTask(this);

        } else {
            RemoveTask(&*it);
        }
    }

    for (SmartList<SpottedUnit>::Iterator it = AiPlayer_Teams[team].GetSpottedUnits().Begin();
         it != AiPlayer_Teams[team].GetSpottedUnits().End(); ++it) {
        UnitInfo* target = (*it).GetUnit();

        if (target && target->team == enemy_team && (*it).GetTask() != this && IsDefenderDangerous(&*it)) {
            TaskAttack* attack_task = dynamic_cast<TaskAttack*>((*it).GetTask());

            if (attack_task) {
                if (attack_task->GetAccessFlags() == access_flags && (attack_task->GetFlags() & 0xFF00) == flags) {
                    AILOG_LOG(log, "Merging attack tasks.");

                    attack_task->CopyTargets(this);

                    secondary_targets.Clear();

                    Finish();

                    return;
                }

            } else {
                SmartPointer<TaskKillUnit> task_kill_unit(new (std::nothrow) TaskKillUnit(this, &*it, flags));

                (*it).SetTask(this);
                secondary_targets.PushBack(*task_kill_unit);

                TaskManager.AppendTask(*task_kill_unit);
            }
        }
    }

    for (SmartList<TaskKillUnit>::Iterator it = secondary_targets.Begin(); it != secondary_targets.End(); ++it) {
        (*it).SetFlags(flags);
    }
}

bool TaskAttack::IsExecutionPhase() { return op_state >= ATTACK_STATE_ADVANCE; }
