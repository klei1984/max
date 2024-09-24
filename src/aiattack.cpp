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
#include "ai.hpp"
#include "aiplayer.hpp"
#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskattack.hpp"
#include "taskfrontalattack.hpp"
#include "taskmove.hpp"
#include "tasktransport.hpp"
#include "taskwaittoattack.hpp"
#include "transportermap.hpp"
#include "units_manager.hpp"
#include "zonewalker.hpp"

int32_t AiAttack_GetTargetValue(UnitInfo* unit) {
    int32_t result;

    int32_t attack_range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
    int32_t turns_to_build = unit->GetBaseValues()->GetAttribute(ATTRIB_TURNS);

    if (turns_to_build > 4) {
        if (attack_range >= 10 || unit->GetUnitType() == COMMANDO || unit->GetUnitType() == SUBMARNE ||
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

            if (it != UnitsManager_StationaryUnits.End()) {
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

bool AiAttack_ChooseSiteToSabotage(UnitInfo* unit1, UnitInfo* unit2, Point* site, int32_t* projected_damage,
                                   int32_t caution_level) {
    uint8_t** info_map = AiPlayer_Teams[unit1->team].GetInfoMap();
    Point position;
    bool result = false;
    int16_t** damage_potential_map = AiPlayer_Teams[unit1->team].GetDamagePotentialMap(unit1, caution_level, true);
    int32_t distance;
    int32_t minimum_distance = INT_MAX;
    int32_t damage_potential = 0;
    int32_t range;

    *projected_damage = INT_MAX;

    if (Task_IsAdjacent(unit2, unit1->grid_x, unit1->grid_y)) {
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

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t i = 0; i < range; ++i) {
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
                            if (Access_IsAccessible(unit1->GetUnitType(), unit1->team, position.x, position.y, 2)) {
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

bool AiAttack_ChooseSiteForAttacker(UnitInfo* unit, Point target, Point* site, int32_t* projected_damage,
                                    int32_t caution_level, int32_t range, bool mode) {
    ZoneWalker walker(target, range);
    uint8_t** info_map = AiPlayer_Teams[unit->team].GetInfoMap();
    int16_t** damage_potential_map;
    bool result = false;
    int32_t distance1 = range;
    int32_t distance2;
    int32_t distance3;
    int32_t damage;
    TransporterMap transporter_map(unit, 0x02, CAUTION_LEVEL_NONE);
    bool is_better;

    if (mode) {
        distance1 = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE) + unit->speed;
        if ((UnitsManager_BaseUnits[unit->GetUnitType()].land_type & (SURFACE_TYPE_LAND | SURFACE_TYPE_WATER)) ==
                (SURFACE_TYPE_LAND | SURFACE_TYPE_WATER) &&
            !(unit->flags & MOBILE_AIR_UNIT)) {
            mode = false;
        }
    }

    distance1 = distance1 * distance1;

    damage_potential_map = AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit, caution_level, true);

    *projected_damage = INT16_MAX;

    distance2 = Access_GetDistance(unit, target);
    distance3 = Access_GetDistance(unit, target);

    damage = 0;

    if (distance3 <= range * range && distance3 <= distance1) {
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
                        if (Access_IsAccessible(unit->GetUnitType(), unit->team, walker.GetGridX(), walker.GetGridY(),
                                                2)) {
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

bool AiAttack_IsWithinReach(UnitInfo* unit, uint16_t team, bool* relevant_teams) {
    bool result;
    int32_t distance;
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

    if (unit->GetUnitType() == COMMANDO && (target->flags & ELECTRONIC_UNIT) && !(target->flags & HOVERING) &&
        (target->GetOrder() != ORDER_DISABLE || !(target->flags & STATIONARY))) {
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
            int32_t attack_range = attacker->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
            bool is_valid = false;

            if (AiAttack_IsValidSabotageTarget(attacker, target)) {
                attack_range = 1;
                is_valid = Task_IsAdjacent(target, attacker->grid_x, attacker->grid_y);

            } else {
                is_valid = Access_IsWithinAttackRange(attacker, target->grid_x, target->grid_y, attack_range);
            }

            if (is_valid) {
                attacker->target_grid_x = target->grid_x;
                attacker->target_grid_y = target->grid_y;

            } else {
                int32_t attack_radius = attacker->GetBaseValues()->GetAttribute(ATTRIB_ATTACK_RADIUS);

                if (attack_radius >= 1 && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE &&
                    attacker->shots > 0) {
                    ZoneWalker walker(Point(target->grid_x, target->grid_y), attack_radius);
                    Point site;
                    bool is_found = false;
                    int32_t distance = attacker->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
                    int32_t distance2;
                    int32_t minimal_distance{INT32_MAX};

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
            }

            if (attacker->shots > 0 || attacker->GetUnitType() != COMMANDO) {
                if (attacker->GetUnitType() == COMMANDO && attack_range == 1) {
                    TaskManager_word_1731C0 = 2;
                    attacker->SetParent(target);

                    attacker->target_grid_x = (dos_rand() * 101) >> 15;

                    if (target->GetOrder() == ORDER_DISABLE ||
                        (!(target->flags & STATIONARY) &&
                         UnitsManager_GetStealthChancePercentage(attacker, target, ORDER_AWAIT_STEAL_UNIT) >=
                             ini_get_setting(INI_MAX_PERCENT))) {
                        UnitsManager_SetNewOrder(attacker, ORDER_AWAIT_STEAL_UNIT, ORDER_STATE_INIT);

                    } else {
                        UnitsManager_SetNewOrder(attacker, ORDER_AWAIT_DISABLE_UNIT, ORDER_STATE_INIT);
                    }

                    result = true;

                } else {
                    if (attacker->shots > 0) {
                        TaskManager_word_1731C0 = 2;
                    }

                    attacker->SetEnemy(target);

                    UnitsManager_SetNewOrder(attacker, ORDER_MOVE_TO_ATTACK, ORDER_STATE_EXECUTING_ORDER);

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
    if (attacker->GetUnitType() == COMMANDO) {
        if (AiAttack_IsValidSabotageTarget(attacker, target) &&
            Task_IsAdjacent(target, attacker->grid_x, attacker->grid_y) &&
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

bool AiAttack_FindAttackSupport(UnitInfo* unit, SmartList<UnitInfo>* units, uint16_t team, int32_t caution_level) {
    bool result = false;

    if (unit->IsVisibleToTeam(team) &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team)) {
        for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
            if ((*it).team == team && (*it).shots > 0 && Task_IsReadyToTakeOrders(&*it) &&
                AiAttack_CanAttack(&*it, unit)) {
                if ((unit->GetUnitType() != LANDMINE && unit->GetUnitType() != SEAMINE) ||
                    (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK_RADIUS) > 0 ||
                    (*it).GetBaseValues()->GetAttribute(ATTRIB_TURNS) < 6) {
                    Point position((*it).grid_x, (*it).grid_y);

                    if (((*it).GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
                         ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) ||
                        (AiPlayer_Teams[team].GetDamagePotential(&*it, position, caution_level, true) < (*it).hits)) {
                        AiAttack_ProcessAttack(&*it, unit);

                        result = true;
                    }
                }
            }
        }
    }

    return result;
}

int32_t AiAttack_EstimateAttackValue(UnitInfo* unit, int32_t predicted_damage) {
    UnitValues* base_values = unit->GetBaseValues();

    return ((base_values->GetAttribute(ATTRIB_HITS) + predicted_damage - unit->hits) *
            base_values->GetAttribute(ATTRIB_TURNS)) /
           base_values->GetAttribute(ATTRIB_HITS);
}

bool AiAttack_IsAttackProfitable(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int32_t damage_to_friendly,
                                 int32_t caution_level, int32_t damage_to_enemy, bool is_desperation_attack) {
    bool result;
    int32_t projected_damage = AiPlayer_CalculateProjectedDamage(friendly_unit, enemy_unit, caution_level);

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

bool AiAttack_IsAttackProfitable(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int32_t damage_to_friendly,
                                 int32_t caution_level, bool is_desperation_attack) {
    int32_t projected_damage = AiPlayer_Teams[friendly_unit->team].GetPredictedAttack(enemy_unit, caution_level);

    return AiAttack_IsAttackProfitable(friendly_unit, enemy_unit, damage_to_friendly, caution_level, projected_damage,
                                       is_desperation_attack);
}

void AiAttack_GetTargetTeams(uint16_t team, bool* teams) {
    if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        int16_t target_team;

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

SpottedUnit* AiAttack_SelectTargetToAttack(UnitInfo* unit, int32_t range, int32_t scan, int32_t caution_level,
                                           bool mode) {
    SpottedUnit* spotted_unit = nullptr;
    UnitValues* base_values = unit->GetBaseValues();
    int32_t minimum_score = 0;
    uint16_t unit_team = unit->team;
    int16_t** damage_potential_map = nullptr;
    int32_t minimum_damage = 32000;
    int32_t unit_scan = base_values->GetAttribute(ATTRIB_SCAN);
    int32_t unit_range = base_values->GetAttribute(ATTRIB_RANGE);
    uint8_t surface_type = UnitsManager_BaseUnits[unit->GetUnitType()].land_type;
    Point unit_position;
    bool teams[PLAYER_TEAM_MAX];
    int32_t distance;

    AiAttack_GetTargetTeams(unit_team, teams);

    unit_scan = std::min(unit_scan, unit_range);
    unit_scan = unit_scan * unit_scan;
    unit_range = unit_range * unit_range;

    range = range * range;
    scan = scan * scan;

    for (SmartList<SpottedUnit>::Iterator it = AiPlayer_Teams[unit->team].GetSpottedUnits().Begin();
         it != AiPlayer_Teams[unit->team].GetSpottedUnits().End(); ++it) {
        UnitInfo* target_unit = (*it).GetUnit();

        if (teams[target_unit->team] && target_unit->GetOrder() != ORDER_IDLE &&
            target_unit->GetOrderState() != ORDER_STATE_DESTROY && target_unit->hits > 0 &&
            target_unit->GetOrder() != ORDER_EXPLODE) {
            unit_position = (*it).GetLastPosition();
            distance = Access_GetDistance(unit, unit_position);

            if (distance <= range) {
                if (target_unit->IsVisibleToTeam(unit->team) || distance <= scan) {
                    if (unit->GetUnitType() == COMMANDO && !unit->IsVisibleToTeam(target_unit->team)) {
                        if (!AiAttack_IsValidSabotageTarget(unit, target_unit) ||
                            (UnitsManager_GetStealthChancePercentage(unit, target_unit, ORDER_AWAIT_DISABLE_UNIT) <=
                                 85 &&
                             target_unit->GetOrder() != ORDER_DISABLE)) {
                            continue;
                        }

                    } else if (!Access_IsValidAttackTarget(unit, target_unit)) {
                        continue;
                    }

                    if ((target_unit->IsVisibleToTeam(unit_team) ||
                         ((target_unit->GetUnitType() != COMMANDO || unit->GetUnitType() == INFANTRY ||
                           unit->GetUnitType() == COMMANDO) &&
                          (!UnitsManager_IsUnitUnderWater(target_unit) || unit->GetUnitType() == CORVETTE))) &&
                        (target_unit->GetOrder() != ORDER_DISABLE || (target_unit->flags & STATIONARY) ||
                         unit->GetUnitType() == COMMANDO || ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_EXPERT)) {
                        if (!target_unit->IsVisibleToTeam(unit_team) &&
                            UnitsManager_TeamInfo[unit_team]
                                .heat_map_complete[ResourceManager_MapSize.x * unit_position.y + unit_position.x]) {
                            (*it).UpdatePosition();
                            unit_position = (*it).GetLastPosition();
                        }

                        if ((target_unit->GetUnitType() != LANDMINE && target_unit->GetUnitType() != SEAMINE) ||
                            base_values->GetAttribute(ATTRIB_ATTACK_RADIUS) > 0 ||
                            base_values->GetAttribute(ATTRIB_TURNS) < 6) {
                            if (target_unit->IsVisibleToTeam(unit_team) ||
                                AiPlayer_TerrainMap.TerrainMap_sub_690D6(unit_position, surface_type) <= unit_scan) {
                                if (AiPlayer_TerrainMap.TerrainMap_sub_690D6(unit_position, surface_type) <=
                                    unit_range) {
                                    int32_t score;
                                    int32_t damage_potential;

                                    if (mode) {
                                        uint32_t unit_flags = target_unit->GetField221();

                                        score = unit_flags & 0xFE;

                                        if (score) {
                                            unit_flags &= ~0xFE;

                                            if (score > 2) {
                                                unit_flags |= score - 2;
                                            }

                                            target_unit->SetField221(unit_flags);

                                            score <<= 7;
                                        }

                                    } else {
                                        score = 0;
                                    }

                                    score |= AiAttack_GetTargetFlags(unit, target_unit, unit_team);

                                    if (caution_level > CAUTION_LEVEL_NONE && !damage_potential_map) {
                                        damage_potential_map =
                                            AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit, caution_level, true);
                                    }

                                    if (damage_potential_map) {
                                        damage_potential = damage_potential_map[unit_position.x][unit_position.y];

                                        if (damage_potential < unit->hits) {
                                            damage_potential = 0;
                                        }

                                    } else {
                                        damage_potential = 0;
                                    }

                                    if (!spotted_unit || (score <= minimum_score &&
                                                          (score != minimum_score ||
                                                           (damage_potential <= minimum_damage &&
                                                            (damage_potential != minimum_damage ||
                                                             spotted_unit->GetUnit()->hits >= target_unit->hits))))) {
                                        spotted_unit = &*it;
                                        minimum_score = score;
                                        minimum_damage = damage_potential;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return spotted_unit;
}

int32_t AiAttack_GetAttackPotential(UnitInfo* attacker, UnitInfo* target) {
    int32_t result;

    if (AiAttack_IsValidSabotageTarget(attacker, target)) {
        result =
            (UnitsManager_GetStealthChancePercentage(attacker, target, ORDER_AWAIT_DISABLE_UNIT) * target->hits) / 100;

    } else {
        result = UnitsManager_GetAttackDamage(attacker, target, 0);
    }

    return result;
}

void AiAttack_UpdateTargetFlags(UnitInfo* unit) { unit->SetField221(unit->GetField221() | 0xFE); }

bool AiAttack_EvaluateAttack(UnitInfo* unit, bool mode) {
    bool result;

    if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
        if (unit->ammo || (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == unit->team)) {
            if (unit->shots > 0 || !unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                if (unit->delayed_reaction || TaskManager_word_1731C0 == 2) {
                    SmartPointer<Task> wait_to_attack_task(new (std::nothrow) TaskWaitToAttack(unit));

                    TaskManager.AppendTask(*wait_to_attack_task);

                    result = TaskManager_word_1731C0 == 2;

                } else {
                    if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK &&
                        !Access_GetAttackTarget(unit, unit->target_grid_x, unit->target_grid_y)) {
                        UnitsManager_SetNewOrder(unit, ORDER_AWAIT, ORDER_STATE_EXECUTING_ORDER);
                    }

                    if (Task_IsReadyToTakeOrders(unit)) {
                        SmartPointer<SpottedUnit> spotted_unit;
                        int32_t attack_radius;

                        if (unit->shots > 0) {
                            attack_radius = unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK_RADIUS) / 2;

                        } else {
                            attack_radius = 0;
                        }

                        if ((dos_rand() * 2) >> 15) {
                            mode = true;

                        } else {
                            mode = false;
                        }

                        spotted_unit = AiAttack_SelectTargetToAttack(
                            unit, unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE) + attack_radius,
                            unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN), CAUTION_LEVEL_NONE, mode);

                        if (spotted_unit) {
                            UnitInfo* target = spotted_unit->GetUnit();

                            if (unit->GetUnitType() == COMMANDO && AiAttack_IsValidSabotageTarget(unit, target) &&
                                !Task_IsAdjacent(target, unit->grid_x, unit->grid_y)) {
                                result = false;

                            } else if (target->AreTherePins()) {
                                SmartPointer<Task> wait_to_attack_task(new (std::nothrow) TaskWaitToAttack(unit));

                                TaskManager.AppendTask(*wait_to_attack_task);

                                result = true;

                            } else {
                                if (ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_APPRENTICE) {
                                    mode = false;
                                }

                                if (mode && unit->speed > 0 &&
                                    (!unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE) ||
                                     ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_AVERAGE) &&
                                    (target->GetBaseValues()->GetAttribute(ATTRIB_TURNS) <
                                         unit->GetBaseValues()->GetAttribute(ATTRIB_TURNS) ||
                                     unit->shots * UnitsManager_GetAttackDamage(unit, target, 0) < target->hits)) {
                                    int32_t damage_potential = AiPlayer_Teams[unit->team].GetDamagePotential(
                                        unit, Point(unit->grid_x, unit->grid_y), CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE,
                                        true);

                                    if (damage_potential >= unit->hits) {
                                        if (AiAttack_FindAttackSupport(target, &UnitsManager_MobileAirUnits, unit->team,
                                                                       CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE) ||
                                            AiAttack_FindAttackSupport(target, &UnitsManager_MobileLandSeaUnits,
                                                                       unit->team,
                                                                       CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE)) {
                                            return true;
                                        }

                                        if (!AiAttack_IsAttackProfitable(unit, target, damage_potential,
                                                                         CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, false) &&
                                            !AiAttack_DecideDesperationAttack(unit, target)) {
                                            if (Task_RetreatIfNecessary(nullptr, unit,
                                                                        Ai_DetermineCautionLevel(unit))) {
                                                if (mode) {
                                                    AiAttack_UpdateTargetFlags(target);
                                                }

                                                return false;
                                            }
                                        }
                                    }
                                }

                                AiAttack_ProcessAttack(unit, target);

                                result = true;
                            }

                        } else {
                            result = false;
                        }

                    } else {
                        result = false;
                    }
                }

            } else {
                result = false;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool AiAttack_EvaluateAssault(UnitInfo* unit, Task* task,
                              void (*result_callback)(Task* task, UnitInfo* unit, char result)) {
    bool result;

    if (unit && unit->shots > 0 && unit->speed > 0 && unit->hits > 0) {
        if (Task_IsReadyToTakeOrders(unit)) {
            SmartPointer<SpottedUnit> spotted_unit;
            uint16_t unit_team = unit->team;
            int32_t caution_level = CAUTION_LEVEL_AVOID_REACTION_FIRE;
            UnitValues* unit_values = unit->GetBaseValues();
            int32_t unit_range;
            int32_t unit_speed;

            if (unit->GetUnitType() == COMMANDO) {
                unit_range = 1;

            } else {
                unit_range = unit_values->GetAttribute(ATTRIB_RANGE);
            }

            unit_speed = unit->speed;

            if (unit_speed >= 1) {
                if (!unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE) ||
                    ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_AVERAGE) {
                    caution_level = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;
                }

                spotted_unit = AiAttack_SelectTargetToAttack(unit, unit_range + unit_speed,
                                                             unit_values->GetAttribute(ATTRIB_SCAN) + unit_speed,
                                                             caution_level, true);

                if (spotted_unit) {
                    UnitInfo* target = spotted_unit->GetUnit();
                    int32_t distance = Access_GetDistance(unit, target);

                    if (target->IsVisibleToTeam(unit->team)) {
                        unit_range = std::min(unit_range, unit_values->GetAttribute(ATTRIB_SCAN));
                    }

                    if (distance <= unit_range * unit_range ||
                        (unit->GetUnitType() == COMMANDO && AiAttack_CanAttack(unit, target))) {
                        result = AiAttack_EvaluateAttack(unit);

                    } else if ((target->GetOrder() != ORDER_DISABLE ||
                                ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_EXPERT) &&
                               (AiAttack_FindAttackSupport(target, &UnitsManager_MobileAirUnits, unit_team,
                                                           CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE) ||
                                AiAttack_FindAttackSupport(target, &UnitsManager_MobileLandSeaUnits, unit_team,
                                                           CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE))) {
                        result = true;

                    } else {
                        if (!unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                            int32_t target_range = target->GetAttackRange() + unit_speed;

                            target_range = target_range * target_range;

                            if (target_range <= distance && unit->shots > 0 &&
                                Access_IsValidAttackTargetType(target->GetUnitType(), unit->GetUnitType()) &&
                                !AiAttack_DecideDesperationAttack(unit, target)) {
                                unit_speed = unit->speed - (unit_values->GetAttribute(ATTRIB_SPEED) /
                                                            unit_values->GetAttribute(ATTRIB_ROUNDS));

                                if (unit_speed < 1) {
                                    return false;
                                }
                            }
                        }

                        Point site;
                        int32_t unit_shots = unit->shots;
                        int32_t projected_damage;
                        int32_t attack_potential;
                        int32_t damage_potential;
                        bool is_site_found;

                        if (!unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                            unit->shots = 0;
                        }

                        if (AiAttack_IsValidSabotageTarget(unit, target)) {
                            is_site_found =
                                AiAttack_ChooseSiteToSabotage(unit, target, &site, &projected_damage, caution_level);

                        } else {
                            is_site_found =
                                AiAttack_ChooseSiteForAttacker(unit, spotted_unit->GetLastPosition(), &site,
                                                               &projected_damage, caution_level, unit_range, true);
                        }

                        unit->shots = unit_shots;

                        if (is_site_found) {
                            distance = (TaskManager_GetDistance(unit->grid_x - site.x, unit->grid_y - site.y) + 1) / 2;

                            if (distance > 0 && caution_level < CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE) {
                                caution_level = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;

                                projected_damage = AiPlayer_Teams[unit_team].GetDamagePotential(
                                    unit, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, true);
                            }

                            attack_potential = AiAttack_GetAttackPotential(unit, target);

                            if (projected_damage < unit->hits || caution_level == CAUTION_LEVEL_AVOID_REACTION_FIRE) {
                                damage_potential = projected_damage;

                            } else {
                                damage_potential = AiPlayer_Teams[unit_team].GetDamagePotential(
                                    unit, site, CAUTION_LEVEL_AVOID_REACTION_FIRE, true);
                            }

                            unit_shots = 0;

                            if (unit->hits < damage_potential) {
                                if (unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                                    unit_shots = unit->shots;

                                } else {
                                    unit_shots = ((unit->speed - distance) * unit_values->GetAttribute(ATTRIB_ROUNDS)) /
                                                 unit_values->GetAttribute(ATTRIB_SPEED);
                                }

                                if (unit_shots > unit->shots) {
                                    unit_shots = unit->shots;
                                }
                            }

                            if (projected_damage < unit->hits && caution_level > CAUTION_LEVEL_AVOID_REACTION_FIRE) {
                                unit_shots =
                                    std::min(unit_values->GetAttribute(ATTRIB_ROUNDS), (unit->ammo - unit_shots));
                            }

                            attack_potential = attack_potential * unit_shots;

                            if (projected_damage > attack_potential || projected_damage >= unit->hits) {
                                if (ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_APPRENTICE) {
                                    AiAttack_UpdateTargetFlags(target);

                                    result = Task_RetreatIfNecessary(task, unit, Ai_DetermineCautionLevel(unit));

                                    return result;
                                }

                                if ((target->GetBaseValues()->GetAttribute(ATTRIB_TURNS) <
                                         unit_values->GetAttribute(ATTRIB_TURNS) ||
                                     attack_potential < target->hits) &&
                                    !AiAttack_DecideDesperationAttack(unit, target) &&
                                    !AiAttack_IsAttackProfitable(unit, target, projected_damage, caution_level,
                                                                 false)) {
                                    bool is_profitable = false;

                                    if (caution_level == CAUTION_LEVEL_AVOID_REACTION_FIRE) {
                                        caution_level = CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE;

                                        projected_damage = AiPlayer_Teams[unit_team].GetDamagePotential(
                                            unit, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, true);

                                        if (AiAttack_IsAttackProfitable(unit, target, projected_damage,
                                                                        CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, 0x00)) {
                                            is_profitable = true;
                                        }
                                    }

                                    if (!is_profitable) {
                                        AiAttack_UpdateTargetFlags(target);

                                        result = Task_RetreatIfNecessary(task, unit, Ai_DetermineCautionLevel(unit));

                                        return result;
                                    }
                                }

                                if (projected_damage >= unit->hits) {
                                    if (AiAttack_FindAttackSupport(target, &UnitsManager_MobileAirUnits, unit_team,
                                                                   CAUTION_LEVEL_NONE) ||
                                        AiAttack_FindAttackSupport(target, &UnitsManager_MobileLandSeaUnits, unit_team,
                                                                   CAUTION_LEVEL_NONE)) {
                                        return true;
                                    }

                                    if (attack_potential < target->hits) {
                                        SmartPointer<Task> frontal_attack_task(new (std::nothrow) TaskFrontalAttack(
                                            unit_team, &*spotted_unit, caution_level));

                                        TaskManager.AppendTask(*frontal_attack_task);

                                        return true;
                                    }

                                    caution_level = CAUTION_LEVEL_NONE;
                                }
                            }

                            if (unit->GetTask() != task) {
                                unit->AddTask(task);
                            }

                            SmartPointer<Task> move_task(
                                new (std::nothrow) TaskMove(unit, task, 0, caution_level, site, result_callback));

                            unit->attack_site = site;

                            TaskManager.AppendTask(*move_task);

                            result = true;

                        } else {
                            AiAttack_UpdateTargetFlags(target);

                            result = false;
                        }
                    }

                } else {
                    result = false;
                }

            } else {
                result = false;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

Task* AiAttack_GetPrimaryTask(UnitInfo* unit) {
    Task* task = unit->GetTask();

    while (task) {
        int32_t task_type = task->GetType();

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

bool AiAttack_FollowAttacker(Task* task, UnitInfo* unit, uint16_t task_flags) {
    SmartPointer<UnitInfo> leader;
    bool is_execution_phase = false;
    TaskAttack* attack_task = nullptr;
    int32_t distance;
    int32_t minimum_distance{INT32_MAX};
    bool result;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == unit->team) {
            attack_task = dynamic_cast<TaskAttack*>(AiAttack_GetPrimaryTask(&*it));

            if (attack_task) {
                if (!is_execution_phase || attack_task->IsExecutionPhase()) {
                    if (attack_task->DeterminePriority(task_flags) <= 0) {
                        distance = Access_GetDistance(unit, &*it);

                        if (!leader || distance < minimum_distance ||
                            (!is_execution_phase && attack_task->IsExecutionPhase())) {
                            minimum_distance = distance;
                            leader = *it;
                            is_execution_phase = attack_task->IsExecutionPhase();
                        }
                    }
                }
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).team == unit->team) {
            attack_task = dynamic_cast<TaskAttack*>(AiAttack_GetPrimaryTask(&*it));

            if (attack_task) {
                if (!is_execution_phase || attack_task->IsExecutionPhase()) {
                    if (attack_task->DeterminePriority(task_flags) <= 0) {
                        distance = Access_GetDistance(unit, &*it);

                        if (!leader || distance < minimum_distance ||
                            (!is_execution_phase && attack_task->IsExecutionPhase())) {
                            minimum_distance = distance;
                            leader = *it;
                            is_execution_phase = attack_task->IsExecutionPhase();
                        }
                    }
                }
            }
        }
    }

    if (leader) {
        Point target_location;
        uint8_t** info_map = AiPlayer_Teams[unit->team].GetInfoMap();
        Point unit_position(unit->grid_x, unit->grid_y);
        ResourceID transporter_type = INVALID_ID;

        if (unit->flags & MOBILE_LAND_UNIT) {
            if (Task_GetReadyUnitsCount(unit->team, AIRTRANS) > 0) {
                transporter_type = AIRTRANS;

            } else if (Task_GetReadyUnitsCount(unit->team, SEATRANS) > 0) {
                transporter_type = SEATRANS;
            }
        }

        TransporterMap map(unit, 0x02, CAUTION_LEVEL_NONE, transporter_type);
        int16_t** damage_potential_map =
            AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, true);
        int32_t range = TaskManager_GetDistance(&*leader, unit) / 2;

        target_location = unit_position;

        for (int32_t i = 1; i < range; ++i) {
            --unit_position.x;
            ++unit_position.y;

            for (int32_t direction = 0; direction < 8; direction += 2) {
                for (int32_t j = 0; j < i; ++j) {
                    unit_position += Paths_8DirPointsArray[direction];

                    if (unit_position.x >= 0 && unit_position.x < ResourceManager_MapSize.x && unit_position.y >= 0 &&
                        unit_position.y < ResourceManager_MapSize.y) {
                        if (!damage_potential_map ||
                            damage_potential_map[unit_position.x][unit_position.y] < unit->hits) {
                            distance = TaskManager_GetDistance(unit_position.x - leader->grid_x,
                                                               unit_position.y - leader->grid_y) /
                                       2;

                            if (distance < range && (!info_map || !(info_map[unit_position.x][unit_position.y] & 8))) {
                                if (map.Search(unit_position) &&
                                    Access_IsAccessible(unit->GetUnitType(), unit->team, unit_position.x,
                                                        unit_position.y, 0x02)) {
                                    target_location = unit_position;
                                    range = distance;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (target_location.x != unit->grid_x || target_location.y != unit->grid_y) {
            SmartPointer<TaskMove> move_task(new (std::nothrow)
                                                 TaskMove(unit, task, 0, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE,
                                                          target_location, &TaskTransport_MoveFinishedCallback));

            move_task->SetField68(true);

            TaskManager.AppendTask(*move_task);
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool AiAttack_IsReadyToMove(UnitInfo* unit) {
    return unit->GetOrder() == ORDER_MOVE && unit->GetOrderState() != ORDER_STATE_EXECUTING_ORDER;
}

uint32_t AiAttack_GetTargetFlags(UnitInfo* attacker, UnitInfo* target, uint16_t team) {
    uint32_t result = 0;

    if (target->ammo > 0 && target->GetOrder() != ORDER_DISABLE) {
        if (target->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 35) {
            result += 0x38;

        } else {
            result = (target->GetBaseValues()->GetAttribute(ATTRIB_ATTACK) / 8) * 8;
        }
    }

    if (target->IsVisibleToTeam(team)) {
        result += 0x80;
    }

    if (target->team == PLAYER_TEAM_ALIEN) {
        result += 0x08;
    }

    if (attacker) {
        if (AiAttack_IsValidSabotageTarget(attacker, target)) {
            result += UnitsManager_GetStealthChancePercentage(attacker, target, ORDER_AWAIT_DISABLE_UNIT) / 13;

            if (target->flags & STATIONARY) {
                result += 0x40;
            }

        } else {
            result += AiAttack_GetTargetWorth(attacker, target);

            if (attacker->shots * UnitsManager_GetAttackDamage(attacker, target, 0) >= target->hits) {
                result += 0x40;

                if (target->GetUnitType() == REPAIR || target->GetUnitType() == MININGST ||
                    target->GetUnitType() == SCANNER || target->GetUnitType() == RADAR ||
                    target->GetUnitType() == GREENHSE) {
                    result += 0x38;
                }
            }
        }
    }

    result = 0xFF - result;

    return result;
}

uint32_t AiAttack_GetTargetWorth(UnitInfo* attacker, UnitInfo* target) {
    uint32_t result;

    if (Access_IsValidAttackTarget(attacker, target) && target->hits > 0) {
        int32_t value = (attacker->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) *
                         UnitsManager_GetAttackDamage(attacker, target, 0) *
                         target->GetBaseValues()->GetAttribute(ATTRIB_TURNS) * 7) /
                        (target->hits * 12);

        if (value >= 1) {
            if (value <= 7) {
                result = value;

            } else {
                result = 7;
            }

        } else {
            result = 1;
        }

    } else {
        result = 0;
    }

    return result;
}
