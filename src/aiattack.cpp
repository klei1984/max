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

#include "access.hpp"
#include "aiplayer.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskwaittoattack.hpp"
#include "transportermap.hpp"
#include "units_manager.hpp"
#include "zonewalker.hpp"

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

bool AiAttack_DecideDesperationAttack(UnitInfo* attacker, UnitInfo* target) {
    bool result;

    if (target->ammo > 0) {
        if (Task_IsUnitDoomedToDestruction(attacker, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE)) {
            result = true;

        } else {
            SmartList<UnitInfo>::Iterator it;

            for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == attacker->team && (*it).IsVisibleToTeam(target->team) &&
                    (*it).GetBaseValues()->GetAttribute(ATTRIB_TURNS) >
                        attacker->GetBaseValues()->GetAttribute(ATTRIB_TURNS) &&
                    Access_IsValidAttackTarget(target, &*it) &&
                    Access_IsWithinAttackRange(target, (*it).grid_x, (*it).grid_y,
                                               target->GetBaseValues()->GetAttribute(ATTRIB_RANGE) +
                                                   target->GetBaseValues()->GetAttribute(ATTRIB_SPEED))) {
                    break;
                }
            }

            if (it) {
                result = true;

            } else {
                result = false;
            }
        }

    } else {
        result = false;
    }

    return result;
}

bool AiAttack_ChooseSiteToSabotage(UnitInfo* unit1, UnitInfo* unit2, Point* site, int* projected_damage,
                                   int caution_level) {
    unsigned char** info_map = AiPlayer_Teams[unit1->team].GetInfoMap();
    Point position;
    bool result = false;
    unsigned short** damage_potential_map =
        AiPlayer_Teams[unit1->team].GetDamagePotentialMap(unit1, caution_level, 0x01);
    int distance;
    int minimum_distance = INT_MAX;
    int damage_potential = 0;
    int range;

    *projected_damage = INT_MAX;

    if (unit2->IsAdjacent(unit1->grid_x, unit1->grid_y)) {
        site->x = unit1->grid_x;
        site->y = unit1->grid_y;

        minimum_distance = 0;

        if (damage_potential_map) {
            damage_potential = damage_potential_map[site->x][site->y];
        }

        *projected_damage = damage_potential;

        result = true;
    }

    if (unit2->flags & BUILDING) {
        range = 3;

    } else {
        range = 2;
    }

    position.x = unit2->grid_x - 1;
    position.y = unit2->grid_y - 1 + range;

    for (int direction = 0; direction < 8; direction += 2) {
        for (int i = 0; i < range; ++i) {
            position += Paths_8DirPointsArray[direction];

            if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                position.y < ResourceManager_MapSize.y) {
                if (damage_potential_map) {
                    damage_potential = damage_potential_map[position.x][position.y];
                }

                if (damage_potential <= *projected_damage) {
                    distance = TaskManager_GetDistance(position.x - unit1->grid_x, position.y - unit1->grid_y);

                    if (damage_potential < *projected_damage || distance < minimum_distance) {
                        if (!info_map || !(info_map[position.x][position.y] & 8)) {
                            if (Access_IsAccessible(unit1->unit_type, unit1->team, position.x, position.y, 2)) {
                                *projected_damage = damage_potential;
                                *site = position;
                                minimum_distance = distance;

                                result = true;
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

bool AiAttack_ChooseSiteForAttacker(UnitInfo* unit, Point target, Point* site, int* projected_damage, int caution_level,
                                    int range, bool mode) {
    ZoneWalker walker(target, range);
    unsigned char** info_map = AiPlayer_Teams[unit->team].GetInfoMap();
    unsigned short** damage_potential_map;
    bool result = false;
    int distance1 = range;
    int distance2;
    int distance3;
    int damage;
    TransporterMap transporter_map(unit, 0x02, CAUTION_LEVEL_NONE);
    bool is_better;

    if (mode) {
        distance1 = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE) + unit->speed;
        if ((UnitsManager_BaseUnits[unit->unit_type].land_type & (SURFACE_TYPE_LAND | SURFACE_TYPE_WATER)) ==
                (SURFACE_TYPE_LAND | SURFACE_TYPE_WATER) &&
            !(unit->flags & MOBILE_AIR_UNIT)) {
            mode = false;
        }
    }

    distance1 = distance1 * distance1;

    damage_potential_map = AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit, caution_level, 0x01);

    *projected_damage = INT16_MAX;

    distance2 = Access_GetDistance(unit, target);

    damage = 0;

    distance3 = Access_GetDistance(unit, target);

    if (range * range <= distance3 && distance3 <= distance1) {
        site->x = unit->grid_x;
        site->y = unit->grid_y;
        distance2 = 0;

        if (damage_potential_map) {
            damage = damage_potential_map[site->x][site->y];
        }

        *projected_damage = damage;

        result = true;
    }

    do {
        if (damage_potential_map) {
            damage = damage_potential_map[walker.GetGridX()][walker.GetGridY()];

            if (damage <= *projected_damage || distance2 > distance1) {
                distance3 = Access_GetDistance(unit, *walker.GetCurrentLocation());

                if (damage > *projected_damage) {
                    is_better = distance3 <= distance1;

                } else {
                    is_better = damage < *projected_damage || distance3 < distance2;
                }

                if (is_better) {
                    is_better = false;

                    if (!info_map || !(info_map[walker.GetGridX()][walker.GetGridY()] & 8)) {
                        if (Access_IsAccessible(unit->unit_type, unit->team, walker.GetGridX(), walker.GetGridY(), 2)) {
                            is_better = !mode || transporter_map.Search(*walker.GetCurrentLocation());
                        }
                    }

                    if (is_better) {
                        *projected_damage = damage;

                        result = true;

                        distance2 = distance3;
                        *site = *walker.GetCurrentLocation();
                    }
                }
            }
        }

    } while (walker.FindNext());

    return result;
}

bool AiAttack_IsWithinReach(UnitInfo* unit, unsigned short team, bool* relevant_teams) {
    bool result;
    int distance;
    Point line_distance;

    distance = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

    if (unit->team == PLAYER_TEAM_ALIEN) {
        result = false;

    } else if (relevant_teams[unit->team]) {
        if (unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
            distance += unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED);
        }

        if (unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN) > distance) {
            distance = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);
        }

        distance = distance * distance;

        if (distance) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                 it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == team) {
                    line_distance.x = unit->grid_x - (*it).grid_x;
                    line_distance.y = unit->grid_y - (*it).grid_y;

                    if (line_distance.x * line_distance.x + line_distance.y * line_distance.y <= distance) {
                        return true;
                    }
                }
            }

            result = false;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

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

bool AiAttack_ProcessAttack(UnitInfo* attacker, UnitInfo* target) {
    bool result;

    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == attacker->team) {
        if (attacker->delayed_reaction || TaskManager_word_1731C0 == 2) {
            SmartPointer<Task> wait_to_attack_task(new (std::nothrow) TaskWaitToAttack(attacker));

            TaskManager.AppendTask(*wait_to_attack_task);

            result = true;

        } else {
            int attack_range = attacker->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
            bool is_valid = false;

            if (AiAttack_IsValidSabotageTarget(attacker, target)) {
                attack_range = 1;
                is_valid = target->IsAdjacent(attacker->grid_x, attacker->grid_y);

            } else {
                is_valid = Access_IsWithinAttackRange(attacker, target->grid_x, target->grid_y, attack_range);
            }

            if (is_valid) {
                int attack_radius = attacker->GetBaseValues()->GetAttribute(ATTRIB_ATTACK_RADIUS);

                if (attack_radius >= 1 && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE &&
                    attacker->shots > 0) {
                    ZoneWalker walker(Point(target->grid_x, target->grid_y), attack_radius);
                    Point site;
                    bool is_found = false;
                    int distance = attacker->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
                    int distance2;
                    int minimal_distance;

                    distance = distance * distance;

                    do {
                        if (Access_GetDistance(attacker, *walker.GetCurrentLocation()) <= distance) {
                            distance2 = Access_GetDistance(target, *walker.GetCurrentLocation());

                            if (!is_found || distance2 < minimal_distance) {
                                is_found = true;
                                minimal_distance = distance2;
                                site = *walker.GetCurrentLocation();
                            }
                        }
                    } while (walker.FindNext());

                    if (is_found) {
                        attacker->target_grid_x = site.x;
                        attacker->target_grid_y = site.y;

                    } else {
                        return false;
                    }

                } else {
                    return false;
                }

            } else {
                attacker->target_grid_x = target->grid_x;
                attacker->target_grid_y = target->grid_y;
            }

            if (attacker->shots > 0 || attacker->unit_type != COMMANDO) {
                if (attacker->unit_type == COMMANDO && attack_range == 1) {
                    TaskManager_word_1731C0 = 2;
                    attacker->SetParent(target);

                    attacker->target_grid_x = (dos_rand() * 101) >> 15;

                    if (target->orders == ORDER_DISABLE ||
                        (!(target->flags & STATIONARY) &&
                         UnitsManager_GetStealthChancePercentage(attacker, target, ORDER_AWAIT_STEAL_UNIT) >=
                             ini_get_setting(INI_STEP_PERCENT))) {
                        UnitsManager_SetNewOrder(attacker, ORDER_AWAIT_STEAL_UNIT, ORDER_STATE_0);

                    } else {
                        UnitsManager_SetNewOrder(attacker, ORDER_AWAIT_DISABLE_UNIT, ORDER_STATE_0);
                    }

                    result = true;

                } else {
                    if (attacker->shots > 0) {
                        TaskManager_word_1731C0 = 2;
                    }

                    attacker->SetEnemy(target);

                    UnitsManager_SetNewOrder(attacker, ORDER_MOVE_TO_ATTACK, ORDER_STATE_1);

                    result = true;
                }

            } else {
                result = false;
            }
        }

    } else {
        result = false;
    }

    return result;
}

bool AiAttack_CanAttack(UnitInfo* attacker, UnitInfo* target) {
    if (attacker->unit_type == COMMANDO) {
        if (AiAttack_IsValidSabotageTarget(attacker, target) &&
            target->IsAdjacent(attacker->grid_x, attacker->grid_y) &&
            UnitsManager_GetStealthChancePercentage(attacker, target, ORDER_AWAIT_DISABLE_UNIT) > 85) {
            return true;
        }

        if (!attacker->IsVisibleToTeam(target->team)) {
            return false;
        }
    }

    return Access_IsValidAttackTarget(attacker, target) &&
           Access_IsWithinAttackRange(attacker, target->grid_x, target->grid_y,
                                      attacker->GetBaseValues()->GetAttribute(ATTRIB_RANGE));
}

bool AiAttack_FindAttackSupport(UnitInfo* unit, SmartList<UnitInfo>* units, unsigned short team, int caution_level) {
    bool result = false;

    if (unit->IsVisibleToTeam(team) &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team)) {
        for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
            if ((*it).team == team && (*it).shots > 0 && Task_IsReadyToTakeOrders(&*it) &&
                AiAttack_CanAttack(&*it, unit)) {
                if ((unit->unit_type != LANDMINE && unit->unit_type != SEAMINE) ||
                    (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK_RADIUS) > 0 ||
                    (*it).GetBaseValues()->GetAttribute(ATTRIB_TURNS) < 6) {
                    Point position((*it).grid_x, (*it).grid_y);

                    if (((*it).GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
                         ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) ||
                        (AiPlayer_Teams[team].GetDamagePotential(&*it, position, caution_level, 1) < (*it).hits)) {
                        AiAttack_ProcessAttack(&*it, unit);

                        result = true;
                    }
                }
            }
        }
    }

    return result;
}

int AiAttack_EstimateAttackValue(UnitInfo* unit, int predicted_damage) {
    UnitValues* base_values = unit->GetBaseValues();

    return ((base_values->GetAttribute(ATTRIB_HITS) + predicted_damage - unit->hits) *
            base_values->GetAttribute(ATTRIB_TURNS)) /
           base_values->GetAttribute(ATTRIB_HITS);
}

bool AiAttack_IsAttackProfitable(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int damage_to_friendly,
                                 int caution_level, int damage_to_enemy, bool is_desperation_attack) {
    bool result;
    int projected_damage = AiPlayer_CalculateProjectedDamage(friendly_unit, enemy_unit, caution_level);

    damage_to_friendly = AiAttack_EstimateAttackValue(friendly_unit, damage_to_friendly);

    if (damage_to_friendly > 0) {
        damage_to_enemy -=
            (projected_damage * damage_to_friendly) / friendly_unit->GetBaseValues()->GetAttribute(ATTRIB_HITS);
    }

    if (damage_to_enemy > 0) {
        damage_to_enemy = AiAttack_EstimateAttackValue(enemy_unit, damage_to_enemy);

    } else {
        damage_to_enemy = 0;
    }

    if (is_desperation_attack && damage_to_enemy > 0) {
        result = true;

    } else if (damage_to_friendly < damage_to_enemy) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool AiAttack_IsAttackProfitable(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int damage_to_friendly,
                                 int caution_level, bool is_desperation_attack) {
    int projected_damage = AiPlayer_Teams[friendly_unit->team].GetPredictedAttack(enemy_unit, caution_level);

    return AiAttack_IsAttackProfitable(friendly_unit, enemy_unit, damage_to_friendly, caution_level, projected_damage,
                                       is_desperation_attack);
}

void AiAttack_GetTargetTeams(unsigned short team, bool* teams) {
    if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        unsigned short target_team;

        memset(teams, 0, PLAYER_TEAM_MAX * sizeof(bool));

        teams[PLAYER_TEAM_ALIEN] = true;

        target_team = AiPlayer_Teams[team].GetTargetTeam();

        if (target_team >= PLAYER_TEAM_RED) {
            teams[target_team] = true;
        }

        for (target_team = PLAYER_TEAM_RED; target_team < PLAYER_TEAM_MAX - 1; ++target_team) {
            if (UnitsManager_TeamInfo[target_team].team_type == TEAM_TYPE_PLAYER ||
                UnitsManager_TeamInfo[target_team].team_type == TEAM_TYPE_REMOTE) {
                teams[target_team] = true;

            } else if (UnitsManager_TeamInfo[target_team].team_type == TEAM_TYPE_COMPUTER &&
                       AiPlayer_Teams[target_team].GetTargetTeam() == team) {
                teams[target_team] = true;
            }
        }

    } else {
        memset(teams, 1, PLAYER_TEAM_MAX * sizeof(bool));
    }
}

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

bool AiAttack_FollowAttacker(Task* task, UnitInfo* unit, unsigned short task_flags) {}

bool AiAttack_IsReadyToMove(UnitInfo* unit) { return unit->orders == ORDER_MOVE && unit->state != ORDER_STATE_1; }

unsigned int AiAttack_GetTargetFlags(UnitInfo* attacker, UnitInfo* target, unsigned short team) {}
