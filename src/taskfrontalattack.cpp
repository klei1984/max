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

#include "taskfrontalattack.hpp"

#include "access.hpp"
#include "aiattack.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "units_manager.hpp"

void TaskFrontalAttack::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskFrontalAttack* frontal_attack_task = dynamic_cast<TaskFrontalAttack*>(task);

    if (result == TASKMOVE_RESULT_BLOCKED) {
        frontal_attack_task->units2.PushBack(*unit);
        frontal_attack_task->units1.Remove(*unit);
    }

    frontal_attack_task->IssueOrders();
}

void TaskFrontalAttack::Finish() {
    SmartPointer<Task> frontal_attack_task(this);

    for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
        Task_RemoveMovementTasks(&*it);
        (*it).RemoveTask(this);
    }

    units1.Clear();

    for (SmartList<UnitInfo>::Iterator it = units2.Begin(); it != units2.End(); ++it) {
        Task_RemoveMovementTasks(&*it);
        (*it).RemoveTask(this);
    }

    units2.Clear();

    TaskManager.RemoveTask(*this);
}

void TaskFrontalAttack::IssueOrders() {
    UnitInfo* target = spotted_unit->GetUnit();
    int units_ready_for_orders = 0;

    if (GameManager_PlayMode != PLAY_MODE_UNKNOWN && target->hits > 0) {
        for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
            if ((*it).shots == 0 && ((*it).speed == 0 || ((*it).GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
                                                          ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE))) {
                (*it).RemoveTask(this);
                units1.Remove(*it);
            }
        }

        if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
            for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
                if (Task_IsReadyToTakeOrders(&*it)) {
                    if ((*it).speed == 0 || Access_GetDistance(&*it, spotted_unit->GetLastPosition()) <=
                                                (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE) *
                                                    (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE)) {
                        if (!AiAttack_EvaluateAttack(&*it)) {
                            units2.PushBack(*it);
                            units1.Remove(*it);
                        }

                        if (units1.GetCount() > 0 && !GetField8()) {
                            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
                        }

                        return;
                    }
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
            if ((*it).speed == 0) {
                (*it).RemoveTask(this);
                units1.Remove(*it);
            }
        }

        while (units1.GetCount() > 0) {
            UnitInfo* attacker = nullptr;
            bool has_attack_target = false;
            int unit_value;
            int best_unit_value;

            for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
                if ((*it).orders == ORDER_MOVE_TO_ATTACK || Access_GetDistance(&*it, spotted_unit->GetLastPosition()) <=
                                                                (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE) *
                                                                    (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE)) {
                    has_attack_target = true;

                } else if ((*it).GetTask() == this || (*it).GetTask() == nullptr ||
                           (*it).GetTask()->GetType() == TaskType_TaskFindPath ||
                           (*it).GetTask()->GetType() == TaskType_TaskMove) {
                    UnitValues* unit_values = (*it).GetBaseValues();

                    if (units1.GetCount() == 1 ||
                        UnitsManager_TeamInfo[team]
                                .heat_map_complete[target->grid_y * ResourceManager_MapSize.x + target->grid_x] != 1 ||
                        Access_GetDistance(&*it, target) >
                            unit_values->GetAttribute(ATTRIB_SCAN) * unit_values->GetAttribute(ATTRIB_SCAN)) {
                        unit_value = ((unit_values->GetAttribute(ATTRIB_ARMOR) * 4 + (*it).hits) * 12) /
                                     unit_values->GetAttribute(ATTRIB_TURNS);

                        if (attacker && unit_value < best_unit_value && !attacker->IsReadyForOrders(this)) {
                            unit_value = best_unit_value;
                        }

                        if ((*it).IsReadyForOrders(this)) {
                            ++units_ready_for_orders;
                        }

                        if (!attacker || unit_value > best_unit_value ||
                            (unit_value == best_unit_value && !attacker->IsReadyForOrders(this))) {
                            attacker = &*it;
                            best_unit_value = unit_value;
                        }
                    }
                }
            }

            if (attacker) {
                if (attacker->IsReadyForOrders(this)) {
                    Point site;
                    int damage_potential_on_friendly_unit;
                    int attacker_range;
                    int total_predicted_damage_to_enemy;
                    int local_caution_level;
                    bool is_found;

                    if (attacker->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
                        ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
                        local_caution_level = CAUTION_LEVEL_AVOID_REACTION_FIRE;

                    } else {
                        local_caution_level = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
                    }

                    if (AiAttack_IsValidSabotageTarget(attacker, target)) {
                        is_found = AiAttack_ChooseSiteToSabotage(
                            attacker, target, &site, &damage_potential_on_friendly_unit, local_caution_level);

                    } else {
                        attacker_range = attacker->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

                        if (!spotted_unit->GetUnit()->IsVisibleToTeam(team)) {
                            attacker_range =
                                std::min(attacker_range, attacker->GetBaseValues()->GetAttribute(ATTRIB_SCAN));
                        }

                        is_found = AiAttack_ChooseSiteForAttacker(attacker, spotted_unit->GetLastPosition(), &site,
                                                                  &damage_potential_on_friendly_unit,
                                                                  local_caution_level, attacker_range, true);
                    }

                    if (is_found && !AiAttack_DecideDesperationAttack(attacker, target)) {
                        total_predicted_damage_to_enemy = 0;

                        for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
                            total_predicted_damage_to_enemy +=
                                AiPlayer_CalculateProjectedDamage(&*it, target, local_caution_level);
                        }

                        damage_potential_on_friendly_unit =
                            AiPlayer_Teams[team].GetDamagePotential(attacker, site, local_caution_level, 0x1);

                        if (!AiAttack_IsAttackProfitable(attacker, target, damage_potential_on_friendly_unit,
                                                         local_caution_level, total_predicted_damage_to_enemy, true)) {
                            is_found = false;

                            if (local_caution_level == CAUTION_LEVEL_AVOID_REACTION_FIRE) {
                                total_predicted_damage_to_enemy = 0;

                                for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
                                    total_predicted_damage_to_enemy += AiPlayer_CalculateProjectedDamage(
                                        &*it, target, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE);
                                }

                                damage_potential_on_friendly_unit = AiPlayer_Teams[team].GetDamagePotential(
                                    attacker, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, 0x1);

                                if (AiAttack_IsAttackProfitable(attacker, target, damage_potential_on_friendly_unit,
                                                                CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE,
                                                                total_predicted_damage_to_enemy, true)) {
                                    is_found = true;
                                }
                            }
                        }
                    }

                    if (is_found) {
                        if (attacker->grid_x != site.x || attacker->grid_y != site.y) {
                            SmartPointer<TaskMove> move_task(new (std::nothrow) TaskMove(
                                attacker, this, 0, CAUTION_LEVEL_NONE, site, &MoveFinishedCallback));

                            move_task->SetField69(true);
                            move_task->SetField68(true);

                            field_19 = true;

                            TaskManager.AppendTask(*move_task);

                            if (units_ready_for_orders > 1) {
                                RemindTurnEnd(true);
                            }

                            return;

                        } else {
                            break;
                        }

                    } else {
                        units1.Remove(*attacker);
                        units2.PushBack(*attacker);

                        if (!GetField8()) {
                            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
                        }

                        return;
                    }

                } else {
                    return;
                }

            } else {
                if (!has_attack_target) {
                    for (SmartList<UnitInfo>::Iterator it = units2.Begin(); it != units2.End(); ++it) {
                        if (Task_RetreatIfNecessary(this, &*it, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE)) {
                            if (!GetField8()) {
                                TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
                            }

                            return;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
                        if (Task_RetreatIfNecessary(this, &*it, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE)) {
                            if (!GetField8()) {
                                TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
                            }

                            return;
                        }
                    }
                }

                Finish();

                return;
            }
        }

    } else {
        Finish();
    }
}

TaskFrontalAttack::TaskFrontalAttack(unsigned short team_, SpottedUnit* spotted_unit_, int caution_level_)
    : Task(team_, nullptr, 0x400) {
    spotted_unit = spotted_unit_;
    caution_level = caution_level_;
    field_19 = false;
}

TaskFrontalAttack::~TaskFrontalAttack() {}

int TaskFrontalAttack::GetCautionLevel(UnitInfo& unit) {
    int result;

    if (units1.Find(unit)) {
        result = CAUTION_LEVEL_NONE;

    } else {
        result = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
    }

    return result;
}

int TaskFrontalAttack::GetMemoryUse() const { return units1.GetMemorySize() - 6 + units2.GetMemorySize() - 10; }

char* TaskFrontalAttack::WriteStatusLog(char* buffer) const {
    if (spotted_unit && spotted_unit->GetUnit() && spotted_unit->GetUnit()->hits > 0) {
        sprintf(buffer, "Frontal attack on %s in [%i,%i]",
                UnitsManager_BaseUnits[spotted_unit->GetUnit()->unit_type].singular_name,
                spotted_unit->GetLastPositionX() + 1, spotted_unit->GetLastPositionY() + 1);

    } else {
        strcpy(buffer, "Completed frontal attack.");
    }

    return buffer;
}

unsigned char TaskFrontalAttack::GetType() const { return TaskType_TaskFrontalAttack; }

void TaskFrontalAttack::AddUnit(UnitInfo& unit) {
    unit.AddTask(this);
    units1.PushBack(unit);
}

void TaskFrontalAttack::Begin() {
    UnitInfo* target = spotted_unit->GetUnit();

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).team == team && ((*it).GetTask() == nullptr || (*it).GetTask()->DeterminePriority(flags) > 0) &&
            AiPlayer_GetProjectedDamage(&*it, target, caution_level) > 0) {
            AddUnit(*it);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && ((*it).GetTask() == nullptr || (*it).GetTask()->DeterminePriority(flags) > 0) &&
            AiPlayer_GetProjectedDamage(&*it, target, caution_level) > 0) {
            AddUnit(*it);
        }
    }

    if (!GetField8()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
    }
}

void TaskFrontalAttack::BeginTurn() {
    if (field_19) {
        Finish();
    }
}

void TaskFrontalAttack::EndTurn() {
    if (units1.GetCount()) {
        IssueOrders();

    } else {
        Finish();
    }
}

bool TaskFrontalAttack::Task_vfunc17(UnitInfo& unit) {
    IssueOrders();

    return true;
}

void TaskFrontalAttack::RemoveSelf() {
    parent = nullptr;
    spotted_unit = nullptr;

    for (SmartList<UnitInfo>::Iterator it = units1.Begin(); it != units1.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units1.Clear();

    for (SmartList<UnitInfo>::Iterator it = units2.Begin(); it != units2.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units2.Clear();

    TaskManager.RemoveTask(*this);
}

void TaskFrontalAttack::RemoveUnit(UnitInfo& unit) {
    units1.Remove(unit);
    units2.Remove(unit);

    if (units1.GetCount()) {
        if (!GetField8()) {
            TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
        }

    } else {
        Finish();
    }
}

void TaskFrontalAttack::Task_vfunc23(UnitInfo& unit) {
    if (spotted_unit->GetUnit() == &unit) {
        Finish();
    }
}
