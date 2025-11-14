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

#include "taskkillunit.hpp"

#include "access.hpp"
#include "aiattack.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "task_manager.hpp"
#include "taskattack.hpp"
#include "taskmove.hpp"
#include "taskrepair.hpp"
#include "ticktimer.hpp"
#include "units_manager.hpp"

TaskKillUnit::TaskKillUnit(TaskAttack* task_attack, SpottedUnit* spotted_unit_, uint16_t priority_)
    : Task(spotted_unit_->GetTeam(), task_attack, priority_) {
    UnitInfo* target = spotted_unit_->GetUnit();
    spotted_unit = spotted_unit_;
    unit_requests = 0;
    seek_target = true;
    hits = target->hits;

    if (target->GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0 && target->team != PLAYER_TEAM_ALIEN) {
        required_damage = hits;
        hits *= 2;

    } else {
        required_damage = 1;
    }

    projected_damage = 0;
}

TaskKillUnit::~TaskKillUnit() {}

int32_t TaskKillUnit::GetProjectedDamage(UnitInfo* attacker, UnitInfo* target) {
    int32_t damage_potential =
        AiAttack_GetAttackPotential(attacker, target) * attacker->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);

    if (target->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0) {
        if ((target->GetBaseValues()->GetAttribute(ATTRIB_SPEED) == 0 &&
             attacker->GetBaseValues()->GetAttribute(ATTRIB_RANGE) >
                 target->GetBaseValues()->GetAttribute(ATTRIB_RANGE)) ||
            !Access_IsValidAttackTargetType(target->GetUnitType(), attacker->GetUnitType())) {
            damage_potential += target->hits - 1;
        }
    }

    return damage_potential;
}

void TaskKillUnit::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskKillUnit* unit_kill_task = dynamic_cast<TaskKillUnit*>(task);

    if (result != TASKMOVE_RESULT_SUCCESS || !AiAttack_EvaluateAssault(unit, task, &MoveFinishedCallback)) {
        unit_kill_task->GiveOrdersToUnit(unit);
    }
}

void TaskKillUnit::FindVaildTypes() {
    TaskAttack* attack_task = dynamic_cast<TaskAttack*>(&*parent);
    uint32_t unit_flags = 0;
    uint32_t unit_flags2 = attack_task->GetAccessFlags();

    if (spotted_unit) {
        AILOG(log, "Kill unit: Find Valid Types");

        weight_table = AiPlayer_Teams[team].GetExtendedWeightTable(spotted_unit->GetUnit(), 0x01);

        for (uint32_t i = 0; i < weight_table.GetCount(); ++i) {
            if (weight_table[i].weight > 0) {
                if (weight_table[i].unit_type == SCOUT &&
                    AiPlayer_Teams[team].GetStrategy() != AI_STRATEGY_SCOUT_HORDE) {
                    unit_flags |= MOBILE_LAND_UNIT;

                } else {
                    unit_flags |= UnitsManager_BaseUnits[weight_table[i].unit_type].flags &
                                  (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT);
                }
            }
        }

        if (spotted_unit->GetUnit()->flags & MOBILE_AIR_UNIT) {
            unit_flags2 |= MOBILE_AIR_UNIT;
        }

        if (unit_flags & unit_flags2) {
            unit_flags &= unit_flags2;

            if (unit_flags & MOBILE_LAND_UNIT) {
                AILOG_LOG(log, "Selecting land units.");
            }

            if (unit_flags & MOBILE_AIR_UNIT) {
                AILOG_LOG(log, "Selecting air units.");
            }

            if (unit_flags & MOBILE_SEA_UNIT) {
                AILOG_LOG(log, "Selecting sea units.");
            }

        } else if (unit_flags & MOBILE_AIR_UNIT) {
            unit_flags = MOBILE_AIR_UNIT;

            AILOG_LOG(log, "Forcing valid type to air units.");

        } else {
            unit_flags = 0;

            AILOG_LOG(log, "No valid types available");
        }

        for (uint32_t i = 0; i < weight_table.GetCount(); ++i) {
            if (weight_table[i].weight > 0) {
                if (weight_table[i].unit_type != COMMANDO) {
                    if (unit_flags & UnitsManager_BaseUnits[weight_table[i].unit_type].flags) {
                        if (weight_table[i].unit_type == SCOUT && !(unit_flags & MOBILE_LAND_UNIT) &&
                            AiPlayer_Teams[team].GetStrategy() != AI_STRATEGY_SCOUT_HORDE) {
                            weight_table[i].weight = 0;
                        }

                    } else {
                        weight_table[i].weight = 0;
                    }
                }
            }
        }
    }
}

bool TaskKillUnit::GetNewUnits() {
    TransporterMap* map;
    bool is_found = false;
    bool result;

    if (spotted_unit) {
        AILOG(log, "Kill {}: Get new units ",
              UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName());

        if (!weight_table.GetCount()) {
            FindVaildTypes();

            is_found = true;
        }

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if (!weight_table.GetWeight((*it).GetUnitType())) {
                TaskManager.ClearUnitTasksAndRemindAvailable(&*it);
            }
        }

        if (hits > projected_damage) {
            if (seek_target) {
                SmartPointer<UnitInfo> unit1;
                SmartPointer<UnitInfo> unit2;
                int32_t distance;

                if (!is_found) {
                    FindVaildTypes();
                    is_found = true;
                }

                if (parent) {
                    unit2 = dynamic_cast<TaskAttack*>(&*parent)->DetermineLeader();

                } else {
                    unit2 = nullptr;
                }

                if (unit2 && !(unit2->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT)) &&
                    unit2->GetUnitType() != COMMANDO) {
                    ResourceID unit_type = INVALID_ID;

                    if (Task_GetReadyUnitsCount(team, AIRTRANS) > 0) {
                        unit_type = AIRTRANS;

                    } else if (Task_GetReadyUnitsCount(team, SEATRANS) > 0) {
                        unit_type = SEATRANS;
                    }

                    map = new (std::nothrow)
                        TransporterMap(&*unit2, 0x01, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, unit_type);

                } else {
                    map = nullptr;
                }

                do {
                    unit1 = FindClosestCombatUnit(&UnitsManager_MobileLandSeaUnits, nullptr, &distance, map);
                    unit1 = FindClosestCombatUnit(&UnitsManager_MobileAirUnits, &*unit1, &distance, nullptr);

                    if (unit1) {
                        unit1->RemoveTasks();
                        AddUnit(*unit1);

                        if (TickTimer_HaveTimeToThink()) {
                            if (!unit2 && parent) {
                                unit2 = dynamic_cast<TaskAttack*>(&*parent)->DetermineLeader();
                            }

                        } else {
                            AILOG_LOG(log, "Get new units halted, {} msecs since frame update",
                                      TickTimer_GetElapsedTime());

                            if (!IsScheduledForTurnStart()) {
                                TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this));
                            }

                            delete map;

                            return true;
                        }
                    }

                } while (unit1 && hits > projected_damage);

                delete map;

                seek_target = false;
            }

            if (hits > projected_damage) {
                if (!unit_requests) {
                    if (!is_found) {
                        FindVaildTypes();

                        is_found = true;
                    }

                    SmartPointer<TaskObtainUnits> obtain_units_task;
                    SmartPointer<UnitInfo> unit;
                    WeightTable table(weight_table, true);
                    int32_t turns_till_mission_end = Task_EstimateTurnsTillMissionEnd();
                    int32_t remaining_hits;
                    ResourceID unit_type;

                    for (uint32_t i = 0; i < table.GetCount(); ++i) {
                        if (table[i].unit_type != INVALID_ID) {
                            if (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], table[i].unit_type)
                                        ->GetAttribute(ATTRIB_TURNS) > turns_till_mission_end ||
                                (UnitsManager_BaseUnits[table[i].unit_type].flags & REGENERATING_UNIT)) {
                                table[i].weight = 0;
                            }
                        }
                    }

                    remaining_hits = hits - projected_damage;

                    do {
                        unit_type = table.RollUnitType();

                        if (unit_type != INVALID_ID) {
                            AILOG_LOG(log, "adding {}", UnitsManager_BaseUnits[unit_type].GetSingularName());

                            unit = new (std::nothrow) UnitInfo(unit_type, team, 0xFFFF);

                            remaining_hits -= GetProjectedDamage(&*unit, spotted_unit->GetUnit());

                            obtain_units_task =
                                new (std::nothrow) TaskObtainUnits(this, spotted_unit->GetLastPosition());

                            ++unit_requests;

                            obtain_units_task->AddUnit(unit_type);

                            TaskManager.AppendTask(*obtain_units_task);

                        } else {
                            remaining_hits = 0;
                        }

                    } while (remaining_hits > 0);

                    result = true;

                } else {
                    AILOG_LOG(log, "obtainer in progress.");

                    result = false;
                }

            } else {
                result = false;
            }

        } else {
            AILOG_LOG(log, "no new units needed.");

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

UnitInfo* TaskKillUnit::FindClosestCombatUnit(SmartList<UnitInfo>* units_, UnitInfo* unit, int32_t* distance,
                                              TransporterMap* map) {
    uint32_t task_priority = GetPriority();
    bool is_found;

    for (SmartList<UnitInfo>::Iterator it = units_->Begin(); it != units_->End(); ++it) {
        if ((*it).team == team && (*it).hits > 0 && (*it).ammo > 0) {
            if ((*it).GetOrder() == ORDER_AWAIT || (*it).GetOrder() == ORDER_SENTRY || (*it).GetOrder() == ORDER_MOVE ||
                (*it).GetOrder() == ORDER_MOVE_TO_UNIT) {
                is_found = false;

                if ((*it).GetTask()) {
                    if ((*it).GetTask()->ComparePriority(task_priority) > 0) {
                        is_found = (*it).GetTask()->IsUnitTransferable(*it);
                    }

                } else {
                    is_found = true;
                }

                if (is_found) {
                    if (IsUnitUsable(*it)) {
                        if (!map || map->Search(Point((*it).grid_x, (*it).grid_y))) {
                            int32_t distance_ = Access_GetSquaredDistance(&*it, spotted_unit->GetLastPosition());

                            if (!unit || *distance > distance_) {
                                unit = &*it;
                                *distance = distance_;
                            }
                        }
                    }
                }
            }
        }
    }

    return unit;
}

bool TaskKillUnit::GiveOrdersToUnit(UnitInfo* unit) {
    bool result;

    if (unit->IsReadyForOrders(this) && parent && unit->speed > 0) {
        AILOG(log, "Kill {}: give orders to {}.",
              spotted_unit ? UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName() : "unit",
              UnitsManager_BaseUnits[unit->GetUnitType()].GetSingularName());

        if (spotted_unit && unit->ammo >= unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS)) {
            result = dynamic_cast<TaskAttack*>(&*parent)->MoveCombatUnit(this, unit);

        } else {
            if (spotted_unit) {
                AILOG_LOG(log, "Removing unit, no ammunition");

            } else {
                AILOG_LOG(log, "Removing unit, task finished.");
            }

            managed_unit = unit;
            TaskManager.ClearUnitTasksAndRemindAvailable(unit);

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskKillUnit::IsUnitUsable(UnitInfo& unit) {
    bool result;

    if (spotted_unit && unit.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) > 0 &&
        unit.ammo >= unit.GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) && managed_unit != unit) {
        if (weight_table.GetWeight(unit.GetUnitType())) {
            result = hits > projected_damage;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

uint16_t TaskKillUnit::GetPriority() const {
    uint16_t result;

    if (spotted_unit) {
        result = base_priority + AiAttack_GetTargetFlags(nullptr, spotted_unit->GetUnit(), team);

    } else {
        result = base_priority + 0xFF;
    }

    return result;
}

char* TaskKillUnit::WriteStatusLog(char* buffer) const {
    if (spotted_unit && spotted_unit->GetUnit()) {
        char unit_name[50];

        spotted_unit->GetUnit()->GetDisplayName(unit_name, sizeof(unit_name));

        sprintf(buffer, "Kill %s at [%i,%i]",
                UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName(),
                spotted_unit->GetLastPositionX() + 1, spotted_unit->GetLastPositionY() + 1);

        if (required_damage > projected_damage) {
            strcat(buffer,
                   SmartString().Sprintf(40, " (%i points needed)", required_damage - projected_damage).GetCStr());

        } else {
            strcat(buffer, " (ready)");
        }

    } else {
        strcpy(buffer, "Completed Kill Unit task.");
    }

    return buffer;
}

Rect* TaskKillUnit::GetBounds(Rect* bounds) {
    if (spotted_unit) {
        bounds->ulx = spotted_unit->GetLastPositionX();
        bounds->uly = spotted_unit->GetLastPositionY();
        bounds->lrx = bounds->ulx + 1;
        bounds->lry = bounds->uly + 1;

    } else {
        Task::GetBounds(bounds);
    }

    return bounds;
}

uint8_t TaskKillUnit::GetType() const { return TaskType_TaskKillUnit; }

bool TaskKillUnit::IsNeeded() { return hits > projected_damage && spotted_unit && spotted_unit->GetUnit()->hits > 0; }

void TaskKillUnit::AddUnit(UnitInfo& unit) {
    AILOG(log, "Kill {}: Add {}.",
          spotted_unit ? UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName() : "unit",
          UnitsManager_BaseUnits[unit.GetUnitType()].GetSingularName());

    if (spotted_unit) {
        projected_damage += GetProjectedDamage(&unit, spotted_unit->GetUnit());

        unit.AddTask(this);
        units.PushBack(unit);

        unit.attack_site.x = 0;
        unit.attack_site.y = 0;

        if (parent && !parent->IsScheduledForTurnEnd()) {
            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*parent));
        }

    } else {
        AILOG_LOG(log, "unit refused.");

        TaskManager.ClearUnitTasksAndRemindAvailable(&unit);
    }
}

void TaskKillUnit::BeginTurn() {
    if (spotted_unit) {
        Point position;

        AILOG(log, "Kill {}: Begin Turn.",
              UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName());

        managed_unit = nullptr;

        position = spotted_unit->GetLastPosition();

        GetNewUnits();
        MoveUnits();
    }
}

void TaskKillUnit::ChildComplete(Task* task) {
    AILOG(log, "Kill Unit: Child Complete.");

    if (spotted_unit) {
        if (task->GetType() == TaskType_TaskObtainUnits) {
            --unit_requests;
            GetNewUnits();
        }
    }
}

void TaskKillUnit::EndTurn() {
    if (spotted_unit) {
        AILOG(log, "Kill {}: End Turn.",
              UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName());

        MoveUnits();
    }
}

bool TaskKillUnit::Execute(UnitInfo& unit) {
    bool result;

    if (spotted_unit) {
        AILOG(log, "Kill {}: move {} finished.",
              UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName(),
              UnitsManager_BaseUnits[unit.GetUnitType()].GetSingularName());

        if (unit.IsReadyForOrders(this) && unit.speed > 0) {
            if (unit.hits < unit.GetBaseValues()->GetAttribute(ATTRIB_HITS) / 4) {
                unit.RemoveTasks();

                SmartPointer<Task> repair_task(new (std::nothrow) TaskRepair(&unit));

                TaskManager.AppendTask(*repair_task);

                result = true;

            } else {
                if (AiAttack_EvaluateAssault(&unit, this, &MoveFinishedCallback)) {
                    result = true;

                } else {
                    result = GiveOrdersToUnit(&unit);
                }
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

void TaskKillUnit::RemoveSelf() {
    if (spotted_unit) {
        AILOG(log, "Kill {}: cancelled.",
              UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName());

        spotted_unit = nullptr;

        if (parent) {
            char buffer[200];

            AILOG_LOG(log, "Notifying {} that task is finished", parent->WriteStatusLog(buffer));

            parent->ChildComplete(this);

            parent = nullptr;
        }

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            TaskManager.ClearUnitTasksAndRemindAvailable(&*it);
        }

        managed_unit = nullptr;
        units.Clear();

        TaskManager.RemoveTask(*this);
    }
}

bool TaskKillUnit::CheckReactions() { return false; }

void TaskKillUnit::RemoveUnit(UnitInfo& unit) {
    SmartPointer<Task> task(this);

    AILOG(log, "Kill {}: Remove {}",
          spotted_unit ? UnitsManager_BaseUnits[spotted_unit->GetUnit()->GetUnitType()].GetSingularName() : "unit",
          UnitsManager_BaseUnits[unit.GetUnitType()].GetSingularName());

    units.Remove(unit);

    if (spotted_unit) {
        projected_damage -= GetProjectedDamage(&unit, spotted_unit->GetUnit());
    }

    if (parent) {
        parent->RemoveUnit(unit);
    }

    if (!IsScheduledForTurnStart()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this));
    }
}

void TaskKillUnit::EventUnitDestroyed(UnitInfo& unit) {
    if (GetUnitSpotted() == &unit) {
        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            TaskManager.ClearUnitTasksAndRemindAvailable(&*it, true);
        }

        units.Clear();

        RemoveSelf();
    }
}

void TaskKillUnit::EventEnemyUnitSpotted(UnitInfo& unit) {}

int32_t TaskKillUnit::GetTotalProjectedDamage() {
    int32_t result = 0;
    TaskAttack* attack_task = dynamic_cast<TaskAttack*>(&*parent);

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (attack_task->IsDestinationReached(&*it)) {
            result += GetProjectedDamage(&*it, spotted_unit->GetUnit());
        }
    }

    return result;
}

bool TaskKillUnit::MoveUnits() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).speed > 0 && (*it).IsReadyForOrders(this)) {
            Task_RemindMoveFinished(&*it, false);
        }
    }

    return false;
}

SpottedUnit* TaskKillUnit::GetSpottedUnit() const { return &*spotted_unit; }

UnitInfo* TaskKillUnit::GetUnitSpotted() const {
    UnitInfo* result;

    if (spotted_unit) {
        result = spotted_unit->GetUnit();

    } else {
        result = nullptr;
    }

    return result;
}

SmartList<UnitInfo>& TaskKillUnit::GetUnits() { return units; }

uint16_t TaskKillUnit::GetRequiredDamage() const { return required_damage; }

uint16_t TaskKillUnit::GetProjectedDamage() const { return projected_damage; }
