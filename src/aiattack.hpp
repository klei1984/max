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

#ifndef AIATTACK_HPP
#define AIATTACK_HPP

#include "spottedunit.hpp"
#include "unitinfo.hpp"

int32_t AiAttack_GetTargetValue(UnitInfo* unit);
bool AiAttack_DecideDesperationAttack(UnitInfo* attacker, UnitInfo* target);
bool AiAttack_ChooseSiteToSabotage(UnitInfo* unit1, UnitInfo* unit2, Point* site, int32_t* projected_damage,
                                   int32_t caution_level);
bool AiAttack_ChooseSiteForAttacker(UnitInfo* unit, Point target, Point* site, int32_t* projected_damage, int32_t caution_level,
                                    int32_t range, bool mode = false);
bool AiAttack_IsWithinReach(UnitInfo* unit, uint16_t team, bool* relevant_teams);
bool AiAttack_IsValidSabotageTarget(UnitInfo* unit, UnitInfo* target);
bool AiAttack_ProcessAttack(UnitInfo* attacker, UnitInfo* target);
bool AiAttack_CanAttack(UnitInfo* attacker, UnitInfo* target);
bool AiAttack_FindAttackSupport(UnitInfo* unit, SmartList<UnitInfo>* units, uint16_t team, int32_t caution_level);
int32_t AiAttack_EstimateAttackValue(UnitInfo* unit, int32_t predicted_damage);
bool AiAttack_IsAttackProfitable(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int32_t damage_to_friendly,
                                 int32_t caution_level, int32_t damage_to_enemy, bool is_desperation_attack);
bool AiAttack_IsAttackProfitable(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int32_t damage_to_friendly,
                                 int32_t caution_level, bool is_desperation_attack);
void AiAttack_GetTargetTeams(uint16_t team, bool* teams);
SpottedUnit* AiAttack_SelectTargetToAttack(UnitInfo* unit, int32_t range, int32_t scan, int32_t caution_level, bool mode);
int32_t AiAttack_GetAttackPotential(UnitInfo* attacker, UnitInfo* target);
void AiAttack_UpdateTargetFlags(UnitInfo* unit);
bool AiAttack_EvaluateAttack(UnitInfo* unit, bool mode = true);
bool AiAttack_EvaluateAssault(UnitInfo* unit, Task* task,
                              void (*result_callback)(Task* task, UnitInfo* unit, char result));
Task* AiAttack_GetPrimaryTask(UnitInfo* unit);
bool AiAttack_FollowAttacker(Task* task, UnitInfo* unit, uint16_t task_flags);
bool AiAttack_IsReadyToMove(UnitInfo* unit);
uint32_t AiAttack_GetTargetFlags(UnitInfo* attacker, UnitInfo* target, uint16_t team);
uint32_t AiAttack_GetTargetWorth(UnitInfo* attacker, UnitInfo* target);

#endif /* AIATTACK_HPP */
