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

#include "aiattack.hpp"

#include "units_manager.hpp"

int AiAttack_GetTargetValue(UnitInfo* unit) {
    int result;

    int attack_range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
    int turns_to_build = unit->GetBaseValues()->GetAttribute(ATTRIB_TURNS);

    if (turns_to_build > 4) {
        if (attack_range >= 10 || unit->unit_type == COMMANDO || unit->unit_type == SUBMARNE ||
            unit->GetBaseValues()->GetAttribute(ATTRIB_TURNS) < 8) {
            result = attack_range;

        } else {
            result = 10;
        }

    } else {
        result = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);
    }

    return result;
}

bool AiAttack_DecideDesperationAttack(UnitInfo* unit1, UnitInfo* unit2) {}

bool AiAttack_ChooseSiteToSabotage(UnitInfo* unit1, UnitInfo* unit2, Point* site, int* projected_damage,
                                   int caution_level) {}

bool AiAttack_ChooseSiteToAttack(UnitInfo* unit, Point target, Point site, int* projected_damage, int caution_level,
                                 int range, bool mode) {}

bool AiAttack_sub_176F3(UnitInfo* unit, unsigned short team, bool* relevant_teams) {}

bool AiAttack_IsValidSabotageTarget(UnitInfo* unit, UnitInfo* target) {
    bool result;

    if (unit->unit_type == COMMANDO && (target->flags & ELECTRONIC_UNIT) && !(target->flags & HOVERING) &&
        (target->orders != ORDER_DISABLE || !(target->flags & STATIONARY))) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool AiAttack_ProcessAttack(UnitInfo* unit1, UnitInfo* unit2) {}

bool AiAttack_CanAttack(UnitInfo* unit1, UnitInfo* unit2) {}

bool AiAttack_FindAttackSupport(UnitInfo* unit, SmartList<UnitInfo>* units, unsigned short team, int caution_level) {}

int AiAttack_EstimateAttackValue(UnitInfo* unit, int predicted_damage) {}

bool AiAttack_IsAttackProfitable(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int damage_to_friendly,
                                 int caution_level, int damage_to_enemy, bool is_desperation_attack) {}

bool AiAttack_IsAttackProfitable(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int damage_to_friendly,
                                 int caution_level, bool is_desperation_attack) {}

void AiAttack_GetTargetTeams(unsigned short team, bool* teams) {}

SpottedUnit* AiAttack_SelectTargetToAttack(UnitInfo* unit, int range, int scan, int caution_level, bool mode) {}

int AiAttack_GetAttackPotential(UnitInfo* unit1, UnitInfo* unit2) {
    int result;

    if (AiAttack_IsValidSabotageTarget(unit1, unit2)) {
        result = (UnitsManager_GetStealthChancePercentage(unit1, unit2, ORDER_AWAIT_DISABLE_UNIT) * unit2->hits) / 100;

    } else {
        result = UnitsManager_GetAttackDamage(unit1, unit2, 0);
    }

    return result;
}

void AiAttack_sub_187FF(UnitInfo* unit) { unit->SetField221(unit->GetField221() | 0xFE); }

bool AiAttack_EvaluateAttack(UnitInfo* unit, bool mode) {}

bool AiAttack_EvaluateAssault(UnitInfo* unit, Task* task,
                              void (*result_callback)(Task* task, UnitInfo* unit, char result)) {}

Task* AiAttack_GetPrimaryTask(UnitInfo* unit) {
    Task* task = unit->GetTask();

    while (task) {
        int task_type = task->GetType();

        if (task_type != TaskType_TaskAttack) {
            if (task_type == TaskType_TaskMove || task_type == TaskType_TaskFindPath ||
                task_type == TaskType_TaskKillUnit) {
                task = task->GetParent();

            } else {
                task = nullptr;
                break;
            }

        } else {
            break;
        }
    }

    return task;
}

bool AiAttack_ChaseAttacker(Task* task, UnitInfo* unit, unsigned short task_flags) {}
