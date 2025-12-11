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

#include "aiplayer.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "aiattack.hpp"
#include "builder.hpp"
#include "circumferencewalker.hpp"
#include "complex.hpp"
#include "continent.hpp"
#include "game_manager.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "randomizer.hpp"
#include "remote.hpp"
#include "researchmenu.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "task.hpp"
#include "task_manager.hpp"
#include "taskassistmove.hpp"
#include "taskattack.hpp"
#include "taskcheckassaults.hpp"
#include "taskdefensereserve.hpp"
#include "taskescort.hpp"
#include "taskexplore.hpp"
#include "taskfindmines.hpp"
#include "taskmanagebuildings.hpp"
#include "taskmineassistant.hpp"
#include "taskmove.hpp"
#include "taskplacemines.hpp"
#include "taskscavenge.hpp"
#include "tasksurvey.hpp"
#include "taskupdateterrain.hpp"
#include "unit.hpp"
#include "units_manager.hpp"
#include "world.hpp"
#include "zonewalker.hpp"

#define AIPLAYER_THREAT_MAP_CACHE_ENTRIES 10

AiPlayer AiPlayer_Teams[PLAYER_TEAM_MAX - 1];
std::unique_ptr<TerrainDistanceField> AiPlayer_TerrainDistanceField;
ThreatMap AiPlayer_ThreatMaps[AIPLAYER_THREAT_MAP_CACHE_ENTRIES];

void AiPlayer::AddBuilding(UnitInfo* unit) { FindManager(Point(unit->grid_x, unit->grid_y))->AddUnit(*unit); }

void AiPlayer::RebuildWeightTable(WeightTable table, ResourceID unit_type, int32_t factor) {
    for (uint32_t i = 0; i < table.GetCount(); ++i) {
        if (table[i].unit_type == unit_type) {
            table[i].weight *= factor;
        }
    }
}

void AiPlayer::RebuildWeightTables(ResourceID unit_type, int32_t factor) {
    RebuildWeightTable(weight_table_ground_defense, unit_type, factor);
    RebuildWeightTable(weight_table_scout, unit_type, factor);
    RebuildWeightTable(weight_table_tanks, unit_type, factor);
    RebuildWeightTable(weight_table_assault_guns, unit_type, factor);
    RebuildWeightTable(weight_table_missile_launcher, unit_type, factor);
    RebuildWeightTable(weight_table_air_defense, unit_type, factor);
    RebuildWeightTable(weight_table_aircrafts, unit_type, factor);
    RebuildWeightTable(weight_table_bomber, unit_type, factor);
    RebuildWeightTable(weight_table_9, unit_type, factor);
    RebuildWeightTable(weight_table_fastboat, unit_type, factor);
    RebuildWeightTable(weight_table_corvette, unit_type, factor);
    RebuildWeightTable(weight_table_submarine, unit_type, factor);
    RebuildWeightTable(weight_table_battleships, unit_type, factor);
    RebuildWeightTable(weight_table_missileboat, unit_type, factor);
    RebuildWeightTable(weight_table_support_ships, unit_type, factor);
    RebuildWeightTable(weight_table_commando, unit_type, factor);
    RebuildWeightTable(weight_table_infantry, unit_type, factor);
    RebuildWeightTable(weight_table_generic, unit_type, factor);
}

void AiPlayer::UpdateWeightTables() {
    switch (strategy) {
        case AI_STRATEGY_DEFENSIVE: {
            RebuildWeightTables(GUNTURRT, 3);
            RebuildWeightTables(ANTIMSSL, 3);
            RebuildWeightTables(ANTIAIR, 3);
        } break;

        case AI_STRATEGY_MISSILES: {
            RebuildWeightTables(MISSLLCH, 5);
            RebuildWeightTables(MSSLBOAT, 5);
        } break;

        case AI_STRATEGY_AIR: {
            RebuildWeightTables(FIGHTER, 5);
            RebuildWeightTables(BOMBER, 5);
        } break;

        case AI_STRATEGY_SEA: {
            RebuildWeightTables(FASTBOAT, 2);
            RebuildWeightTables(CORVETTE, 2);
            RebuildWeightTables(SUBMARNE, 2);
            RebuildWeightTables(BATTLSHP, 2);
            RebuildWeightTables(MSSLBOAT, 2);
        } break;

        case AI_STRATEGY_SCOUT_HORDE: {
            weight_table_tanks.Add(SCOUT, 1);
            weight_table_assault_guns.Add(SCOUT, 2);
            weight_table_ground_defense.Add(SCOUT, 1);
            weight_table_missile_launcher.Add(SCOUT, 1);

            RebuildWeightTables(SCOUT, 10);
        } break;

        case AI_STRATEGY_TANK_HORDE: {
            RebuildWeightTables(TANK, 10);
        } break;

        case AI_STRATEGY_FAST_ATTACK: {
            RebuildWeightTables(ARTILLRY, 3);
            RebuildWeightTables(BOMBER, 2);
            RebuildWeightTables(SP_FLAK, 2);
            RebuildWeightTables(CORVETTE, 2);
            RebuildWeightTables(FASTBOAT, 2);
        } break;

        case AI_STRATEGY_COMBINED_ARMS: {
            RebuildWeightTables(TANK, 5);
            RebuildWeightTables(MISSLLCH, 5);
        } break;

        case AI_STRATEGY_ESPIONAGE: {
            RebuildWeightTables(SUBMARNE, 4);
            RebuildWeightTables(COMMANDO, 4);
            RebuildWeightTables(MISSLLCH, 2);
            RebuildWeightTables(MSSLBOAT, 2);
        } break;
    }
}

void AiPlayer::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {}

Point AiPlayer::GetUnitClusterCoordinates(uint16_t team, SmartList<UnitInfo>* units) {
    int32_t team_unit_count = 0;
    int32_t grid_x = 0;
    int32_t grid_y = 0;
    Point result(0, 0);

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team) {
            ++team_unit_count;
            grid_x += (*it).grid_x;
            grid_y += (*it).grid_y;
        }
    }

    if (team_unit_count > 0) {
        result.x = grid_x / team_unit_count;
        result.y = grid_y / team_unit_count;
    }

    return result;
}

Point AiPlayer::GetTeamClusterCoordinates(uint16_t team) {
    Point site = GetUnitClusterCoordinates(team, &UnitsManager_StationaryUnits);

    if (site.x == 0 && site.y == 0) {
        site = GetUnitClusterCoordinates(team, &UnitsManager_MobileLandSeaUnits);
    }

    return site;
}

void AiPlayer::DetermineAttack(SpottedUnit* spotted_unit, uint16_t task_priority) {
    if (!spotted_unit->GetTask()) {
        if (task_priority > TASK_PRIORITY_ATTACK_MOBILE &&
            UnitsManager_TeamInfo[spotted_unit->GetUnit()->team].team_points >
                UnitsManager_TeamInfo[player_team].team_points &&
            (spotted_unit->GetUnit()->GetUnitType() == GREENHSE ||
             spotted_unit->GetUnit()->GetUnitType() == MININGST)) {
            task_priority = TASK_PRIORITY_ATTACK_PRIORITY_TARGET;
        }

        uint16_t target_task_priority = AiAttack_GetTargetFlags(nullptr, spotted_unit->GetUnit(), player_team) +
                                        task_priority + TASK_PRIORITY_ADJUST_MAJOR;
        int32_t task_index;

        for (task_index = 0; task_index < AttackTaskLimit && attack_tasks[task_index]; ++task_index) {
        }

        if (task_index == AttackTaskLimit) {
            for (--task_index; task_index >= 0 && attack_tasks[task_index]->ComparePriority(target_task_priority) <= 0;
                 --task_index) {
            }
        }

        if (task_index >= 0) {
            // expert and tougher computer players capture alien units
            if (spotted_unit->GetUnit()->team == PLAYER_TEAM_ALIEN &&
                ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_EXPERT) {
                if (TaskMove::FindUnit(&UnitsManager_MobileLandSeaUnits, player_team, COMMANDO)) {
                    if (attack_tasks[task_index]) {
                        attack_tasks[task_index]->RemoveSelf();
                    }

                    attack_tasks[task_index] = new (std::nothrow) TaskAttack(spotted_unit, task_priority);
                    TaskManager.AppendTask(*attack_tasks[task_index]);
                }

            } else {
                if (attack_tasks[task_index]) {
                    attack_tasks[task_index]->RemoveSelf();
                }

                attack_tasks[task_index] = new (std::nothrow) TaskAttack(spotted_unit, task_priority);
                TaskManager.AppendTask(*attack_tasks[task_index]);
            }
        }
    }
}

void AiPlayer::UpdatePriorityTasks() {
    for (int32_t i = 0; i < AttackTaskLimit; ++i) {
        attack_tasks[i] = nullptr;
    }

    for (auto it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End(); ++it) {
        if ((*it).GetTeam() == player_team && (*it).GetType() == TaskType_TaskAttack) {
            uint16_t task_priority = (*it).GetPriority();
            int32_t task_index;

            for (task_index = 0; task_index < AttackTaskLimit && attack_tasks[task_index] &&
                                 attack_tasks[task_index]->ComparePriority(task_priority) <= 0;
                 ++task_index) {
            }

            if (task_index == AttackTaskLimit) {
                (*it).RemoveSelf();

            } else {
                if (attack_tasks[AttackTaskLimit - 1]) {
                    attack_tasks[AttackTaskLimit - 1]->RemoveSelf();
                }

                for (int32_t i = AttackTaskLimit - 1; i > task_index; --i) {
                    attack_tasks[i] = attack_tasks[i - 1];
                }

                attack_tasks[task_index] = (*it);
            }
        }
    }
}

void AiPlayer::DetermineTargetLocation(Point position) {
    if (spotted_units.GetCount() > 0) {
        SpottedUnit* target = nullptr;
        int32_t distance;
        int32_t minimum_distance;

        for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
            UnitInfo* unit = (*it).GetUnit();

            if (!target || unit->GetUnitType() == MININGST || target->GetUnit()->GetUnitType() != MININGST) {
                distance = Access_GetApproximateDistance(position, (*it).GetLastPosition());

                if (!target || (unit->GetUnitType() == MININGST && target->GetUnit()->GetUnitType() != MININGST) ||
                    distance < minimum_distance) {
                    target = &*it;
                    minimum_distance = distance;
                }
            }
        }

        target_location = target->GetLastPosition();

    } else {
        target_location.x = ResourceManager_MapSize.x / 2;
        target_location.y = ResourceManager_MapSize.y / 2;
    }
}

bool AiPlayer::IsKeyFacility(ResourceID unit_type) {
    return unit_type == MININGST || unit_type == GREENHSE || unit_type == SHIPYARD || unit_type == LIGHTPLT ||
           unit_type == LANDPLT || unit_type == AIRPLT;
}

void AiPlayer::DetermineResearchProjects() {
    int32_t best_research_topic = RESEARCH_TOPIC_ATTACK;
    int32_t best_turns_to_complete =
        UnitsManager_TeamInfo[player_team].research_topics[best_research_topic].turns_to_complete;
    int32_t turns_to_complete;

    for (int32_t research_topic = RESEARCH_TOPIC_SHOTS; research_topic < RESEARCH_TOPIC_COST; ++research_topic) {
        turns_to_complete = UnitsManager_TeamInfo[player_team].research_topics[research_topic].turns_to_complete;

        if (research_topic == RESEARCH_TOPIC_RANGE) {
            turns_to_complete /= 2;
        }

        if (research_topic == RESEARCH_TOPIC_SHOTS) {
            turns_to_complete *=
                5 - (UnitsManager_TeamInfo[player_team].research_topics[research_topic].research_level % 5);
        }

        if (research_topic == RESEARCH_TOPIC_SPEED) {
            turns_to_complete *= 2;
        }

        if (turns_to_complete < best_turns_to_complete) {
            best_research_topic = research_topic;
            best_turns_to_complete = turns_to_complete;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == player_team && (*it).GetUnitType() == RESEARCH && (*it).GetOrder() == ORDER_POWER_ON &&
            (*it).GetOrderState() != ORDER_STATE_INIT && (*it).research_topic != best_research_topic) {
            ResearchMenu_UpdateResearchProgress(player_team, (*it).research_topic, -1);
            (*it).research_topic = best_research_topic;
            ResearchMenu_UpdateResearchProgress(player_team, (*it).research_topic, 1);
        }
    }

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_09(player_team);
    }
}

bool AiPlayer::IsPotentialSpotter(uint16_t team, UnitInfo* unit) {
    int32_t distance = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);

    distance = distance * distance;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && Access_GetSquaredDistance(unit, &*it) <= distance) {
            return true;
        }
    }

    return false;
}

void AiPlayer::UpdateAccessMap(Point point1, Point point2, AccessMap& access_map) {
    Point site(point1);
    Point distance;
    int32_t surface_type = Access_GetSurfaceType(point1.x, point1.y);
    int32_t step;

    distance.x = labs(point2.x - point1.x);
    distance.y = labs(point2.y - point1.y);

    if (distance.x > distance.y) {
        step = point2.y < point1.y ? -1 : 1;

        if (point2.x < point1.x) {
            site = point2;
            step = -step;
        }

        for (int32_t x = 0, y = distance.y / 2; x < distance.x; ++x) {
            ++site.x;
            y += distance.y;

            if (y >= distance.x) {
                site.y += step;
                y -= distance.x;
            }

            if (!access_map(site.x, site.y)) {
                surface_type = Access_GetSurfaceType(site.x, site.y);
            }

            if (surface_type == Access_GetSurfaceType(site.x, site.y)) {
                info_map[site.x][site.y] |= INFO_MAP_FRONTIER;
            }
        }

    } else {
        step = point2.x < point1.x ? -1 : 1;

        if (point2.x < point1.x) {
            site = point2;
            step = -step;
        }

        for (int32_t y = 0, x = distance.x / 2; y < distance.y; ++y) {
            ++site.y;
            x += distance.x;

            if (x >= distance.y) {
                site.x += step;
                x -= distance.y;
            }

            if (!access_map(site.x, site.y)) {
                surface_type = Access_GetSurfaceType(site.x, site.y);
            }

            if (surface_type == Access_GetSurfaceType(site.x, site.y)) {
                info_map[site.x][site.y] |= INFO_MAP_FRONTIER;
            }
        }
    }
}

bool AiPlayer::CheckAttacks() {
    if (Ai_GetReactionState() != AI_REACTION_STATE_ANIMATIONS_ACTIVE) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == player_team && AiAttack_EvaluateAttack(&*it)) {
                return true;
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == player_team && AiAttack_EvaluateAttack(&*it)) {
                return true;
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
             it != UnitsManager_MobileAirUnits.End(); ++it) {
            if ((*it).team == player_team && AiAttack_EvaluateAttack(&*it)) {
                return true;
            }
        }
    }

    return false;
}

void AiPlayer::RegisterReadyAndAbleUnits(SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == player_team && Task_IsReadyToTakeOrders(&*it) && (*it).speed > 0) {
            TaskManager.EnqueueUnitForReactionCheck(*it);
        }
    }
}

bool AiPlayer::AreActionsPending() {
    return Ai_GetReactionState() == AI_REACTION_STATE_ANIMATIONS_ACTIVE || TaskManager.GetRemindersCount() > 0;
}

bool AiPlayer::IsDemoMode() {
    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER ||
            UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_REMOTE) {
            return false;
        }
    }

    return true;
}

void AiPlayer::RegisterIdleUnits() {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == player_team && !(*it).GetTask()) {
            if ((*it).ammo > 0 || (*it).GetUnitType() == LIGHTPLT || (*it).GetUnitType() == LANDPLT ||
                (*it).GetUnitType() == SHIPYARD || (*it).GetUnitType() == AIRPLT || (*it).GetUnitType() == TRAINHAL) {
                TaskManager.ClearUnitTasksAndRemindAvailable(&*it);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == player_team && !(*it).GetTask()) {
            TaskManager.ClearUnitTasksAndRemindAvailable(&*it);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).team == player_team && !(*it).GetTask()) {
            TaskManager.ClearUnitTasksAndRemindAvailable(&*it);
        }
    }
}

int32_t AiPlayer::GetTotalProjectedDamage(UnitInfo* unit, int32_t caution_level, uint16_t team,
                                          SmartList<UnitInfo>* units) {
    int32_t result = 0;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team) {
            result += AiPlayer_GetProjectedDamage(&*it, unit, caution_level);
        }
    }

    return result;
}

void AiPlayer::UpdateMap(int16_t** map, Point position, int32_t range, int32_t damage_potential, bool normalize) {
    Point site;
    Point limit;
    int32_t distance = range * range;
    int32_t map_offset;
    int16_t* map_address{nullptr};

    site.x = std::max(position.x - range, 0) - 1;
    site.y = 0;

    limit.x = std::min(position.x + range, ResourceManager_MapSize.x - 1);
    limit.y = 0;

    for (;;) {
        ++site.y;

        if (site.y > limit.y) {
            ++site.x;

            if (site.x > limit.x) {
                return;
            }

            map_offset = (site.x - position.x) * (site.x - position.x);

            site.y = range;

            while (site.y >= 0 && (site.y * site.y + map_offset) > distance) {
                --site.y;
            }

            limit.y = std::min(site.y + position.y, ResourceManager_MapSize.y - 1);
            site.y = std::max(position.y - site.y, 0);

            map_address = &map[site.x][site.y];
        }

        if (normalize && *map_address < 0) {
            *map_address = 0;
        }

        *map_address += damage_potential;
        ++map_address;
    }
}

void AiPlayer::UpdateThreatMaps(ThreatMap* threat_map, UnitInfo* unit, Point position, int32_t range, int32_t attack,
                                int32_t shots, int32_t& ammo, bool normalize) {
    if (shots > ammo) {
        shots = ammo;
    }

    ammo -= shots;

    int32_t damage_potential = attack * shots;

    if (damage_potential > 0) {
        if (unit->GetUnitType() == SUBMARNE || unit->GetUnitType() == CORVETTE) {
            auto world = ResourceManager_GetActiveWorld();
            ZoneWalker walker(position, range);

            do {
                if (world->GetSurfaceType(walker.GetGridX(), walker.GetGridY()) &
                    (SURFACE_TYPE_WATER | SURFACE_TYPE_COAST)) {
                    threat_map->damage_potential_map[walker.GetGridX()][walker.GetGridY()] += damage_potential;
                    threat_map->shots_map[walker.GetGridX()][walker.GetGridY()] += shots;
                }
            } while (walker.FindNext());

        } else {
            UpdateMap(threat_map->damage_potential_map, position, range, damage_potential, normalize);
            UpdateMap(threat_map->shots_map, position, range, shots, false);
        }
    }
}

void AiPlayer::InvalidateThreatMaps() {
    for (auto& map : AiPlayer_ThreatMaps) {
        map.SetRiskLevel(0);
    }
}

void AiPlayer::DetermineDefenses(SmartList<UnitInfo>* units, int16_t** map) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).shots > 0 && (*it).GetOrder() != ORDER_IDLE && (*it).GetOrder() != ORDER_DISABLE) {
            int32_t unit_range = (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE);

            if (!(*it).GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                unit_range -= 3;
            }

            if (unit_range > 0) {
                int32_t attack_power = (-(*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK)) * (*it).shots;

                UpdateMap(map, Point((*it).grid_x, (*it).grid_y), unit_range, attack_power, false);
            }
        }
    }
}

void AiPlayer::DetermineThreats(UnitInfo* unit, Point position, int32_t caution_level, bool* teams,
                                ThreatMap* air_force_map, ThreatMap* ground_forces_map) {
    if ((unit->flags & MOBILE_AIR_UNIT) && air_force_map->shots_map) {
        ground_forces_map = air_force_map;
    }

    if (!(unit->speed > 0 && !teams[unit->team])) {
        UnitValues* base_values = unit->GetBaseValues();
        int32_t unit_range = base_values->GetAttribute(ATTRIB_RANGE);
        int32_t attack_range = unit_range + base_values->GetAttribute(ATTRIB_ATTACK_RADIUS);
        int32_t unit_attack = base_values->GetAttribute(ATTRIB_ATTACK);
        int32_t unit_shots = base_values->GetAttribute(ATTRIB_ROUNDS);
        int32_t unit_ammo = unit->ammo;
        int32_t unit_speed = base_values->GetAttribute(ATTRIB_SPEED);

        switch (caution_level) {
            case CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE: {
                if (base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                    UpdateThreatMaps(ground_forces_map, unit, position, unit_range, unit_attack, unit->shots, unit_ammo,
                                     true);

                    int32_t movement_range = 0;

                    if (teams[unit->team]) {
                        movement_range = unit_speed / 2;

                        UpdateThreatMaps(ground_forces_map, unit, position, attack_range + movement_range, unit_attack,
                                         unit_shots, unit_ammo, false);

                    } else {
                        UpdateThreatMaps(ground_forces_map, unit, position, attack_range + movement_range, unit_attack,
                                         unit_shots, unit_ammo, false);
                    }

                } else {
                    UpdateThreatMaps(ground_forces_map, unit, position, unit_range, unit_attack, unit->shots + 1,
                                     unit_ammo, true);

                    if (teams[unit->team]) {
                        for (;;) {
                            --unit_shots;

                            if (unit_shots <= 0) {
                                break;
                            }

                            int32_t movement_range = ((unit_speed + 1) * unit_shots) / (unit_shots + 1);

                            UpdateThreatMaps(ground_forces_map, unit, position, movement_range + attack_range,
                                             unit_attack, 1, unit_ammo, false);
                        }
                    }
                }
            } break;

            case CAUTION_LEVEL_AVOID_ALL_DAMAGE: {
                UpdateThreatMaps(ground_forces_map, unit, position, attack_range, unit_attack, unit_shots, unit_ammo,
                                 true);

                if (teams[unit->team]) {
                    int32_t movement_range = 0;

                    if (base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                        movement_range = unit_speed / 2;

                    } else {
                        movement_range = ((unit_speed + 1) * (unit_shots - 1)) / (unit_shots);

                        if (GameManager_PlayMode != PLAY_MODE_TURN_BASED) {
                            movement_range = std::max<int32_t>(movement_range, unit->speed);
                        }
                    }

                    if (movement_range > 0) {
                        UpdateThreatMaps(ground_forces_map, unit, position, movement_range + attack_range, unit_attack,
                                         unit_shots, unit_ammo, false);
                    }
                }

            } break;

            case CAUTION_LEVEL_AVOID_REACTION_FIRE: {
                UpdateThreatMaps(ground_forces_map, unit, position, unit_range, unit_attack, unit->shots, unit_ammo,
                                 false);
            } break;
        }
    }
}

void AiPlayer::SumUpMaps(int16_t** map1, int16_t** map2) {
    for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
            map1[x][y] += map2[x][y];
        }
    }
}

void AiPlayer::NormalizeThreatMap(ThreatMap* threat_map) {
    for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
            if (threat_map->damage_potential_map[x][y] < 0) {
                threat_map->damage_potential_map[x][y] = 0;
                threat_map->shots_map[x][y] = 0;
            }
        }
    }
}

bool AiPlayer::IsAbleToAttack(UnitInfo* attacker, ResourceID target_type, uint16_t team) {
    bool result;

    if (attacker->ammo > 0 && attacker->GetOrder() != ORDER_DISABLE && attacker->GetOrder() != ORDER_IDLE &&
        attacker->hits > 0 && Access_IsValidAttackTargetType(attacker->GetUnitType(), target_type)) {
        if (attacker->GetUnitType() != LANDMINE && attacker->GetUnitType() != SEAMINE) {
            if ((attacker->GetUnitType() != COMMANDO && attacker->GetUnitType() != SUBMARNE) ||
                attacker->IsVisibleToTeam(team)) {
                result = true;

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

ThreatMap* AiPlayer::GetThreatMap(int32_t risk_level, int32_t caution_level, bool is_for_attacking) {
    ThreatMap* result;

    if (ResourceManager_MapSize.x > 0) {
        if (need_init) {
            BeginTurn();
        }

        if (ResourceManager_GetSettings()->GetNumericValue("opponent") < OPPONENT_TYPE_EXPERT) {
            is_for_attacking = false;
        }

        for (int32_t i = 0; i < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++i) {
            if (AiPlayer_ThreatMaps[i].risk_level == risk_level &&
                AiPlayer_ThreatMaps[i].caution_level == caution_level &&
                AiPlayer_ThreatMaps[i].for_attacking == is_for_attacking &&
                AiPlayer_ThreatMaps[i].team == player_team) {
                AiPlayer_ThreatMaps[i].id = 0;

                result = &AiPlayer_ThreatMaps[i];

                for (int32_t j = 0; j < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++j) {
                    ++AiPlayer_ThreatMaps[j].id;
                }

                return result;
            }
        }

        int32_t index = 0;

        for (int32_t i = 1; i < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++i) {
            if (AiPlayer_ThreatMaps[index].id < AiPlayer_ThreatMaps[i].id) {
                index = i;
            }
        }

        AiPlayer_ThreatMaps[index].Init();

        AiPlayer_ThreatMaps[index].risk_level = risk_level;
        AiPlayer_ThreatMaps[index].armor = 0;
        AiPlayer_ThreatMaps[index].caution_level = caution_level;
        AiPlayer_ThreatMaps[index].for_attacking = is_for_attacking;
        AiPlayer_ThreatMaps[index].team = player_team;
        AiPlayer_ThreatMaps[index].id = 0;

        for (int32_t i = 0; i < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++i) {
            ++AiPlayer_ThreatMaps[i].id;
        }

        ThreatMap air_force_threat_map;
        const ResourceID risk_group[] = {INVALID_ID, TANK, SURVEYOR, FIGHTER, COMMANDO, COMMANDO, SUBMARNE, CLNTRANS};
        ResourceID risk_group_unit = risk_group[risk_level];
        bool teams[PLAYER_TEAM_MAX];

        AiAttack_GetTargetTeams(player_team, teams);

        if (caution_level > CAUTION_LEVEL_AVOID_REACTION_FIRE) {
            air_force_threat_map.Init();
        }

        if (caution_level > CAUTION_LEVEL_AVOID_REACTION_FIRE) {
            DetermineDefenses(&air_force, air_force_threat_map.damage_potential_map);
            DetermineDefenses(&ground_forces, AiPlayer_ThreatMaps[index].damage_potential_map);
        }

        if (is_for_attacking) {
            if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_MASTER &&
                ResourceManager_GetSettings()->GetNumericValue("cheating_computer") >=
                    COMPUTER_CHEATING_LEVEL_SHAMELESS) {
                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if (IsAbleToAttack(&*it, risk_group_unit, player_team) && (*it).team != player_team) {
                        DetermineThreats(&*it, Point((*it).grid_x, (*it).grid_y), caution_level, teams,
                                         &air_force_threat_map, &AiPlayer_ThreatMaps[index]);
                    }
                }

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                     it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                    if (IsAbleToAttack(&*it, risk_group_unit, player_team) && (*it).team != player_team) {
                        DetermineThreats(&*it, Point((*it).grid_x, (*it).grid_y), caution_level, teams,
                                         &air_force_threat_map, &AiPlayer_ThreatMaps[index]);
                    }
                }

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                     it != UnitsManager_MobileAirUnits.End(); ++it) {
                    if (IsAbleToAttack(&*it, risk_group_unit, player_team) && (*it).team != player_team) {
                        DetermineThreats(&*it, Point((*it).grid_x, (*it).grid_y), caution_level, teams,
                                         &air_force_threat_map, &AiPlayer_ThreatMaps[index]);
                    }
                }

            } else {
                for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                    if (IsAbleToAttack((*it).GetUnit(), risk_group_unit, player_team)) {
                        DetermineThreats((*it).GetUnit(), Point((*it).GetUnit()->grid_x, (*it).GetUnit()->grid_y),
                                         caution_level, teams, &air_force_threat_map, &AiPlayer_ThreatMaps[index]);
                    }
                }
            }

        } else {
            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                if (IsAbleToAttack((*it).GetUnit(), risk_group_unit, player_team)) {
                    DetermineThreats((*it).GetUnit(), (*it).GetLastPosition(), caution_level, teams,
                                     &air_force_threat_map, &AiPlayer_ThreatMaps[index]);
                }
            }
        }

        if (caution_level > CAUTION_LEVEL_AVOID_REACTION_FIRE) {
            NormalizeThreatMap(&AiPlayer_ThreatMaps[index]);
            NormalizeThreatMap(&air_force_threat_map);
            SumUpMaps(AiPlayer_ThreatMaps[index].damage_potential_map, air_force_threat_map.damage_potential_map);
            SumUpMaps(AiPlayer_ThreatMaps[index].shots_map, air_force_threat_map.shots_map);
        }

        int32_t team_count = 0;
        HeatMap* heat_maps[PLAYER_TEAM_MAX];

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            if (team != player_team && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                heat_maps[team_count] = UnitsManager_TeamInfo[team].heat_map.get();
                ++team_count;
            }
        }

        if (risk_level == 7) {
            auto world = ResourceManager_GetActiveWorld();

            for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                    if (world->GetSurfaceType(x, y) == SURFACE_TYPE_WATER) {
                        bool is_found = false;

                        for (int32_t team = 0; team < team_count; ++team) {
                            if (heat_maps[team] && heat_maps[team]->GetStealthSea(x, y)) {
                                is_found = true;
                                break;
                            }
                        }

                        if (!is_found) {
                            AiPlayer_ThreatMaps[index].damage_potential_map[x][y] = 0x00;
                        }
                    }
                }
            }
        }

        for (int32_t team = 0; team < team_count; ++team) {
            for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                    bool is_visible = false;

                    if (heat_maps[team]) {
                        if (risk_level == 6) {
                            is_visible = heat_maps[team]->GetStealthSea(x, y) > 0;
                        } else if (risk_level == 5 || risk_level == 4) {
                            is_visible = heat_maps[team]->GetStealthLand(x, y) > 0;
                        } else {
                            is_visible = heat_maps[team]->GetComplete(x, y) > 0;
                        }
                    }

                    if (is_visible) {
                        AiPlayer_ThreatMaps[index].damage_potential_map[x][y] |= 0x8000;
                    }
                }
            }
        }

        if (risk_level == 4) {
            int16_t** active_damage_potential_map = AiPlayer_ThreatMaps[index].damage_potential_map;

            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                if ((*it).GetUnit()->GetOrder() == ORDER_DISABLE) {
                    ZoneWalker walker((*it).GetLastPosition(), 4);

                    do {
                        active_damage_potential_map[walker.GetGridX()][walker.GetGridY()] |= 0x8000;

                    } while (walker.FindNext());
                }
            }
        }

        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                if (AiPlayer_ThreatMaps[index].damage_potential_map[x][y] & 0x8000) {
                    AiPlayer_ThreatMaps[index].damage_potential_map[x][y] &= ~0x8000;

                } else {
                    AiPlayer_ThreatMaps[index].damage_potential_map[x][y] = 0x00;
                }
            }
        }

        if (risk_level != 3 && risk_level != 2 && mine_map) {
            for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                    AiPlayer_ThreatMaps[index].damage_potential_map[x][y] += mine_map[x][y];

                    if (mine_map[x][y] > 0) {
                        ++AiPlayer_ThreatMaps[index].shots_map[x][y];
                    }
                }
            }
        }

        if (risk_level != 3) {
            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                UnitInfo* unit = (*it).GetUnit();

                if (unit->GetUnitType() == LANDMINE || unit->GetUnitType() == SEAMINE) {
                    AiPlayer_ThreatMaps[index].damage_potential_map[unit->grid_x][unit->grid_y] +=
                        unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK);
                    ++AiPlayer_ThreatMaps[index].shots_map[unit->grid_x][unit->grid_y];
                }
            }
        }

        result = &AiPlayer_ThreatMaps[index];

    } else {
        result = nullptr;
    }

    return result;
}

WeightTable AiPlayer::GetWeightTable(ResourceID unit_type) {
    WeightTable result;

    switch (unit_type) {
        case FASTBOAT: {
            result = weight_table_fastboat;
        } break;

        case FIGHTER:
        case ALNPLANE:
        case AIRTRANS:
        case AWAC: {
            result = weight_table_aircrafts;
        } break;

        case CORVETTE: {
            result = weight_table_corvette;
        } break;

        case JUGGRNT:
        case BATTLSHP: {
            result = weight_table_battleships;
        } break;

        case SUBMARNE: {
            result = weight_table_submarine;
        } break;

        case MSSLBOAT: {
            result = weight_table_missileboat;
        } break;

        case SEATRANS:
        case CARGOSHP: {
            result = weight_table_support_ships;
        } break;

        case ALNTANK:
        case TANK: {
            result = weight_table_tanks;
        } break;

        case ALNASGUN:
        case ARTILLRY:
        case ROCKTLCH: {
            result = weight_table_assault_guns;
        } break;

        case BOMBER: {
            result = weight_table_bomber;
        } break;

        case MISSLLCH: {
            result = weight_table_missile_launcher;
        } break;

        case COMMANDO: {
            result = weight_table_commando;
        } break;

        case INFANTRY: {
            result = weight_table_infantry;
        } break;

        case SCOUT: {
            result = weight_table_scout;
        } break;

        case SP_FLAK:
        case ANTIAIR: {
            result = weight_table_air_defense;
        } break;

        case LANDMINE:
        case SEAMINE: {
            result = weight_table_generic;
        } break;

        case GUNTURRT:
        case ARTYTRRT:
        case ANTIMSSL: {
            result = weight_table_ground_defense;
        } break;

        case INVALID_ID: {
            WeightTable table;

            table += weight_table_air_defense;
            table += weight_table_bomber;
            table += weight_table_submarine;

            if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_EXPERT) {
                table += weight_table_commando;
            }

            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                table += GetWeightTable((*it).GetUnit()->GetUnitType());
            }

            result = table;

        } break;

        default: {
            result = weight_table_generic;
        } break;
    }

    return result;
}

void AiPlayer::AddThreatToMineMap(int32_t grid_x, int32_t grid_y, int32_t range, int32_t damage_potential,
                                  int32_t factor) {
    Point position(grid_x, grid_y);
    Rect bounds;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t i = 0; i < range; ++i) {
            position += DIRECTION_OFFSETS[direction];

            if (Access_IsInsideBounds(&bounds, &position)) {
                int8_t mine_value = mine_map[position.x][position.y];

                if (mine_value >= 0) {
                    mine_map[position.x][position.y] = (((damage_potential - mine_value) * factor) / 10) + mine_value;
                }
            }
        }
    }
}

void AiPlayer::MineSpotted(UnitInfo* unit) {
    if (mine_map) {
        Point position(unit->grid_x, unit->grid_y);
        int32_t base_attack = unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK);

        AddThreatToMineMap(position.x - 1, position.y + 1, 2, base_attack, 7);
        AddThreatToMineMap(position.x - 2, position.y + 2, 4, base_attack, 5);
        AddThreatToMineMap(position.x - 3, position.y + 3, 6, base_attack, 2);
        AddThreatToMineMap(position.x - 4, position.y + 4, 8, base_attack, 1);

        if (!find_mines_task) {
            find_mines_task = new (std::nothrow) TaskFindMines(player_team, position);

            TaskManager.AppendTask(*find_mines_task);
        }
    }
}

bool AiPlayer::IsSurfaceTypePresent(Point site, int32_t range, int32_t surface_type) {
    ZoneWalker walker(site, range);

    do {
        if (Access_GetModifiedSurfaceType(walker.GetGridX(), walker.GetGridY(), true) == surface_type) {
            return true;
        }

    } while (walker.FindNext());

    return false;
}

int32_t AiPlayer::SelectTeamClan() {
    int32_t team_clan{TEAM_CLAN_RANDOM};

    switch (strategy) {
        case AI_STRATEGY_DEFENSIVE: {
            if (Randomizer_Generate(7)) {
                team_clan = TEAM_CLAN_7_KNIGHTS;

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_MISSILES: {
            if (Randomizer_Generate(7)) {
                team_clan = TEAM_CLAN_AYERS_HAND;

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_AIR: {
            if (Randomizer_Generate(7)) {
                team_clan = TEAM_CLAN_THE_CHOSEN;

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_SEA: {
            if (Randomizer_Generate(7)) {
                team_clan = TEAM_CLAN_CRIMSON_PATH;

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_SCOUT_HORDE: {
            team_clan = TEAM_CLAN_SACRED_EIGHTS;
        } break;

        case AI_STRATEGY_TANK_HORDE: {
            team_clan = TEAM_CLAN_MUSASHI;
        } break;

        case AI_STRATEGY_FAST_ATTACK: {
            team_clan = TEAM_CLAN_SACRED_EIGHTS;
        } break;

        case AI_STRATEGY_COMBINED_ARMS: {
            if (Randomizer_Generate(7)) {
                team_clan = TEAM_CLAN_VON_GRIFFIN + Randomizer_Generate(3);

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_ESPIONAGE: {
            if (Randomizer_Generate(7)) {
                team_clan = TEAM_CLAN_CRIMSON_PATH;

            } else {
                team_clan = TEAM_CLAN_VON_GRIFFIN;
            }
        } break;

        default: {
            SDL_assert(0);
        } break;
    }

    ResourceManager_GetSettings()->SetNumericValue(menu_team_clan_setting[player_team], team_clan);

    ResourceManager_InitClanUnitValues(player_team);

    return team_clan;
}

void AiPlayer::RollField3() { field_3 = Randomizer_Generate(4) + 2 + Randomizer_Generate(4) + 2; }

void AiPlayer::RollField5() {
    switch (strategy) {
        case AI_STRATEGY_DEFENSIVE:
        case AI_STRATEGY_AIR:
        case AI_STRATEGY_SEA: {
            field_5 = Randomizer_Generate(6) + 7;
        } break;

        case AI_STRATEGY_SCOUT_HORDE:
        case AI_STRATEGY_TANK_HORDE: {
            field_5 = Randomizer_Generate(3) + 1;
            field_5 = 5;
        } break;

        case AI_STRATEGY_MISSILES:
        case AI_STRATEGY_FAST_ATTACK:
        case AI_STRATEGY_COMBINED_ARMS:
        case AI_STRATEGY_ESPIONAGE: {
            field_5 = Randomizer_Generate(2) + 4;
        } break;

        default: {
            field_5 = 0;
        } break;
    }
}

void AiPlayer::RollField7() {
    if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_AVERAGE) {
        switch (strategy) {
            case AI_STRATEGY_AIR:
            case AI_STRATEGY_SEA: {
                if (Randomizer_Generate(100) + 1 < 50) {
                    field_7 = (Randomizer_Generate(5) + Randomizer_Generate(5)) * 6;

                } else {
                    field_7 = 0;
                }
            } break;

            case AI_STRATEGY_DEFENSIVE: {
                field_7 = (Randomizer_Generate(7) + Randomizer_Generate(7)) * 12;
            } break;

            case AI_STRATEGY_SCOUT_HORDE:
            case AI_STRATEGY_TANK_HORDE: {
                field_7 = 0;
            } break;

            case AI_STRATEGY_MISSILES:
            case AI_STRATEGY_FAST_ATTACK:
            case AI_STRATEGY_COMBINED_ARMS:
            case AI_STRATEGY_ESPIONAGE: {
                if (Randomizer_Generate(100) + 1 < 25) {
                    field_7 = (Randomizer_Generate(5) + Randomizer_Generate(5)) * 6;

                } else {
                    field_7 = 0;
                }
            } break;
        }

    } else {
        field_7 = 0;
    }
}

bool AiPlayer::AddUnitToTeamMissionSupplies(ResourceID unit_type, uint16_t supplies) {
    TeamMissionSupplies* mission_supplies = &UnitsManager_TeamMissionSupplies[player_team];
    int32_t build_cost = Ai_GetNormalRateBuildCost(unit_type, player_team);
    bool result;

    if (Builder_IsBuildable(player_team, unit_type) && mission_supplies->units.GetCount() < 20) {
        if (build_cost <= mission_supplies->team_gold) {
            if (supplies > (mission_supplies->team_gold - build_cost) * 5) {
                supplies = (mission_supplies->team_gold - build_cost) * 5;
            }

            mission_supplies->units.PushBack(&unit_type);
            mission_supplies->cargos.PushBack(&supplies);

            mission_supplies->team_gold -= supplies / 5 + build_cost;

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

int32_t AiPlayer::GetVictoryConditionsFactor() {
    int32_t result;

    if (ini_setting_victory_type != VICTORY_TYPE_DURATION) {
        result = 2;

    } else if (ini_setting_victory_limit < 100) {
        result = 0;

    } else if (ini_setting_victory_limit < 200) {
        result = 1;

    } else {
        result = 2;
    }

    return result;
}

void AiPlayer::RollTeamMissionSupplies(int32_t clan) {
    TeamMissionSupplies* mission_supplies = &UnitsManager_TeamMissionSupplies[player_team];
    ResourceID unit_type;
    uint16_t cargo;

    auto clans = ResourceManager_GetClans();
    std::string clan_id =
        ResourceManager_GetClanID(static_cast<TeamClanType>(UnitsManager_TeamInfo[player_team].team_clan));

    mission_supplies->team_gold = ResourceManager_GetSettings()->GetNumericValue("start_gold");

    if (clans) {
        mission_supplies->team_gold += clans->GetCredits(clan_id);
    }

    mission_supplies->units.Clear();
    mission_supplies->cargos.Clear();

    auto mission_manager = ResourceManager_GetMissionManager();

    if (mission_manager && mission_manager->GetGameRulesHandler()) {
        if (!mission_manager->GetGameRulesHandler()->GetMissionLoadout(player_team, clan_id, *mission_supplies)) {
            /* There is no error reporting from RollTeamMissionSupplies */
            SDL_assert(0);
        }
    }

    switch (strategy) {
        case AI_STRATEGY_DEFENSIVE: {
            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            AddUnitToTeamMissionSupplies(SCANNER, 0);
            AddUnitToTeamMissionSupplies(MISSLLCH, 0);

            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            AddUnitToTeamMissionSupplies(TANK, 0);

            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(CONSTRCT, 8);
            }

            ChooseInitialUpgrades(((Randomizer_Generate(5) + 4) * mission_supplies->team_gold) / 16);

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(SURVEYOR, 0);
            }

            AddUnitToTeamMissionSupplies(MINELAYR, 16);

            if (GetVictoryConditionsFactor() > 1) {
                while (AddUnitToTeamMissionSupplies(ENGINEER, 8)) {
                    AddUnitToTeamMissionSupplies(ENGINEER, 24);
                    AddUnitToTeamMissionSupplies(CONSTRCT, 20);
                }

            } else {
                AddUnitToTeamMissionSupplies(SCOUT, 0);
                ChooseInitialUpgrades(mission_supplies->team_gold / 2);

                if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_AVERAGE) {
                    while (AddUnitToTeamMissionSupplies(TANK, 0)) {
                    }

                } else {
                    while (AddUnitToTeamMissionSupplies(TANK, 0)) {
                        AddUnitToTeamMissionSupplies(MISSLLCH, 0);
                    }
                }
            }
        } break;

        case AI_STRATEGY_MISSILES: {
            AddUnitToTeamMissionSupplies(SCANNER, 0);
            AddUnitToTeamMissionSupplies(MISSLLCH, 0);
            AddUnitToTeamMissionSupplies(TANK, 0);

            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            AddUnitToTeamMissionSupplies(SCOUT, 0);

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(CONSTRCT, 0);
            }

            ChooseInitialUpgrades(((Randomizer_Generate(5) + 4) * mission_supplies->team_gold) / 16);

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            AddUnitToTeamMissionSupplies(MISSLLCH, 0);

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(SURVEYOR, 0);
            }

            while (AddUnitToTeamMissionSupplies(TANK, 0)) {
                if (GetVictoryConditionsFactor() > 1) {
                    AddUnitToTeamMissionSupplies(CONSTRCT, 20);
                    AddUnitToTeamMissionSupplies(ENGINEER, 8);
                }

                AddUnitToTeamMissionSupplies(MISSLLCH, 0);
            }
        } break;

        case AI_STRATEGY_AIR: {
            AddUnitToTeamMissionSupplies(SCOUT, 0);
            AddUnitToTeamMissionSupplies(SCANNER, 0);
            AddUnitToTeamMissionSupplies(MISSLLCH, 0);

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(CONSTRCT, 4);
            }

            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 8);
            }

            ChooseInitialUpgrades(((Randomizer_Generate(4) + 1) * mission_supplies->team_gold) / 16);

            AddUnitToTeamMissionSupplies(SCOUT, 0);

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(SURVEYOR, 0);

                if (clan != TEAM_CLAN_AXIS_INC) {
                    AddUnitToTeamMissionSupplies(CONSTRCT, 20);
                    AddUnitToTeamMissionSupplies(ENGINEER, 8);
                }
            }

            AddUnitToTeamMissionSupplies(SCOUT, 0);

            if (GetVictoryConditionsFactor() > 1) {
                while (AddUnitToTeamMissionSupplies(CONSTRCT, 40)) {
                }

            } else {
                ChooseInitialUpgrades(mission_supplies->team_gold / 2);

                while (AddUnitToTeamMissionSupplies(MISSLLCH, 0)) {
                }
            }
        } break;

        case AI_STRATEGY_SEA: {
            AddUnitToTeamMissionSupplies(SCANNER, 0);
            AddUnitToTeamMissionSupplies(MISSLLCH, 0);

            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(CONSTRCT, 0);
            }

            ChooseInitialUpgrades(((Randomizer_Generate(4) + 1) * mission_supplies->team_gold) / 16);

            AddUnitToTeamMissionSupplies(SP_FLAK, 0);
            AddUnitToTeamMissionSupplies(SCOUT, 0);

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(SURVEYOR, 0);
                AddUnitToTeamMissionSupplies(CONSTRCT, 40);
            }

            AddUnitToTeamMissionSupplies(SCOUT, 0);

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(ENGINEER, 8);
            }

            AddUnitToTeamMissionSupplies(SCOUT, 0);

            if (GetVictoryConditionsFactor() > 1) {
                while (AddUnitToTeamMissionSupplies(CONSTRCT, 40)) {
                }

            } else {
                ChooseInitialUpgrades(mission_supplies->team_gold / 2);

                while (AddUnitToTeamMissionSupplies(SCOUT, 0)) {
                }
            }
        } break;

        case AI_STRATEGY_SCOUT_HORDE: {
            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            ChooseInitialUpgrades(((Randomizer_Generate(4) + 2) * mission_supplies->team_gold) / 8);

            while (AddUnitToTeamMissionSupplies(SCOUT, 0)) {
            }
        } break;

        case AI_STRATEGY_TANK_HORDE: {
            AddUnitToTeamMissionSupplies(SCANNER, 0);
            AddUnitToTeamMissionSupplies(SCOUT, 0);
            AddUnitToTeamMissionSupplies(TANK, 0);
            AddUnitToTeamMissionSupplies(SCOUT, 0);
            AddUnitToTeamMissionSupplies(TANK, 0);
            AddUnitToTeamMissionSupplies(SCOUT, 0);
            AddUnitToTeamMissionSupplies(TANK, 0);
            AddUnitToTeamMissionSupplies(REPAIR, 0);

            ChooseInitialUpgrades(((Randomizer_Generate(6) + 1) * mission_supplies->team_gold) / 8);

            AddUnitToTeamMissionSupplies(TANK, 0);
            AddUnitToTeamMissionSupplies(TANK, 0);

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            while (AddUnitToTeamMissionSupplies(TANK, 0)) {
            }
        } break;

        case AI_STRATEGY_FAST_ATTACK: {
            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            AddUnitToTeamMissionSupplies(SCOUT, 0);
            AddUnitToTeamMissionSupplies(ARTILLRY, 0);
            AddUnitToTeamMissionSupplies(SCOUT, 0);
            AddUnitToTeamMissionSupplies(ARTILLRY, 0);

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(CONSTRCT, 0);
            }

            ChooseInitialUpgrades(((Randomizer_Generate(5) + 3) * mission_supplies->team_gold) / 12);

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(SURVEYOR, 0);
            }

            while (AddUnitToTeamMissionSupplies(SCOUT, 0)) {
                AddUnitToTeamMissionSupplies(ARTILLRY, 0);
                AddUnitToTeamMissionSupplies(SCOUT, 0);
                AddUnitToTeamMissionSupplies(ARTILLRY, 0);

                if (GetVictoryConditionsFactor() > 1) {
                    AddUnitToTeamMissionSupplies(CONSTRCT, 20);
                    AddUnitToTeamMissionSupplies(ENGINEER, 8);
                }
            }

        } break;

        case AI_STRATEGY_COMBINED_ARMS: {
            AddUnitToTeamMissionSupplies(SCANNER, 0);
            AddUnitToTeamMissionSupplies(MISSLLCH, 0);
            AddUnitToTeamMissionSupplies(TANK, 0);
            AddUnitToTeamMissionSupplies(SCOUT, 0);

            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(CONSTRCT, 0);
            }

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(SURVEYOR, 0);
            }

            AddUnitToTeamMissionSupplies(MISSLLCH, 0);
            AddUnitToTeamMissionSupplies(TANK, 0);

            ChooseInitialUpgrades(((Randomizer_Generate(10) + 3) * mission_supplies->team_gold) / 16);

            while (AddUnitToTeamMissionSupplies(SCOUT, 0)) {
                if (GetVictoryConditionsFactor() > 1) {
                    AddUnitToTeamMissionSupplies(CONSTRCT, 20);
                    AddUnitToTeamMissionSupplies(ENGINEER, 8);
                }

                AddUnitToTeamMissionSupplies(MISSLLCH, 0);
                AddUnitToTeamMissionSupplies(TANK, 0);
            }
        } break;

        case AI_STRATEGY_ESPIONAGE: {
            if (clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            AddUnitToTeamMissionSupplies(SCANNER, 0);
            AddUnitToTeamMissionSupplies(MISSLLCH, 0);
            AddUnitToTeamMissionSupplies(SCOUT, 0);

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(CONSTRCT, 0);
            }

            ChooseInitialUpgrades(((Randomizer_Generate(11) + 2) * mission_supplies->team_gold) / 16);

            if (GetVictoryConditionsFactor() > 0 && clan != TEAM_CLAN_AXIS_INC) {
                AddUnitToTeamMissionSupplies(ENGINEER, 0);
            }

            if (GetVictoryConditionsFactor() > 1) {
                AddUnitToTeamMissionSupplies(SURVEYOR, 0);
            }

            if (GetVictoryConditionsFactor() > 1) {
                while (AddUnitToTeamMissionSupplies(CONSTRCT, 20)) {
                }

            } else {
                while (AddUnitToTeamMissionSupplies(TANK, 0)) {
                    AddUnitToTeamMissionSupplies(MISSLLCH, 0);
                }
            }
        } break;
    }

    ChooseInitialUpgrades(mission_supplies->team_gold);
}

void AiPlayer::AddBuildOrder(SmartObjectArray<BuildOrder>* build_orders, uint16_t team, ResourceID unit_type,
                             int32_t attribute) {
    if (Builder_IsBuildable(team, unit_type)) {
        BuildOrder order(attribute, unit_type);

        (*build_orders).PushBack(&order);
    }
}

void AiPlayer::CheckReconnaissanceNeeds(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type, uint16_t team,
                                        uint16_t enemy_team, bool mode) {
    int32_t unit_range_max = 0;
    int32_t unit_scan = UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type)->GetAttribute(ATTRIB_SCAN);
    CTInfo* team_info = &UnitsManager_TeamInfo[enemy_team];

    for (int32_t i = 0; i < UNIT_END; ++i) {
        UnitValues* unit_values = UnitsManager_GetCurrentUnitValues(team_info, static_cast<ResourceID>(i));

        if (unit_values->GetAttribute(ATTRIB_ATTACK) > 0 && (mode || unit_values->GetAttribute(ATTRIB_SPEED)) &&
            Access_IsValidAttackTargetType(static_cast<ResourceID>(i), unit_type)) {
            if (team_info->team_units->GetBaseUnitValues(static_cast<ResourceID>(i))->GetAttribute(ATTRIB_RANGE) >
                unit_scan) {
                // do not compete with units that are in another league
                continue;
            }

            unit_range_max = std::max(unit_range_max, unit_values->GetAttribute(ATTRIB_RANGE) + 1);
        }
    }

    if (unit_range_max >
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_SCAN)) {
        AddBuildOrder(build_orders, team, unit_type, ATTRIB_SCAN);
    }
}

SmartObjectArray<BuildOrder> AiPlayer::ChooseStrategicBuildOrders(bool mode) {
    SmartObjectArray<BuildOrder> build_orders;

    if (!!IsTargetTeamDefined()) {
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (team != player_team) {
                if (!IsTargetTeamDefined() ||
                    (UnitsManager_TeamInfo[team].team_points > UnitsManager_TeamInfo[target_team].team_points &&
                     (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                      UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED))) {
                    target_team = team;
                }
            }
        }
    }

    if (IsTargetTeamDefined()) {
        if (mode) {
            switch (strategy) {
                case AI_STRATEGY_MISSILES:
                case AI_STRATEGY_COMBINED_ARMS: {
                    CheckReconnaissanceNeeds(&build_orders, SCANNER, player_team, target_team, false);
                } break;

                case AI_STRATEGY_AIR: {
                    CheckReconnaissanceNeeds(&build_orders, AWAC, player_team, target_team, true);
                } break;

                case AI_STRATEGY_SEA: {
                    CheckReconnaissanceNeeds(&build_orders, FASTBOAT, player_team, target_team, true);
                } break;
            }

        } else {
            CheckReconnaissanceNeeds(&build_orders, SCANNER, player_team, target_team, true);
            CheckReconnaissanceNeeds(&build_orders, AWAC, player_team, target_team, true);
            CheckReconnaissanceNeeds(&build_orders, FASTBOAT, player_team, target_team, true);
            CheckReconnaissanceNeeds(&build_orders, SCOUT, player_team, target_team, true);

            if (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[target_team], INFANTRY)
                    ->GetAttribute(ATTRIB_SCAN) >=
                UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], COMMANDO)
                    ->GetAttribute(ATTRIB_SCAN)) {
                AddBuildOrder(&build_orders, player_team, COMMANDO, ATTRIB_SCAN);
            }

            if (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[target_team], CORVETTE)
                    ->GetAttribute(ATTRIB_SCAN) >=
                UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], SUBMARNE)
                    ->GetAttribute(ATTRIB_SCAN)) {
                AddBuildOrder(&build_orders, player_team, SUBMARNE, ATTRIB_SCAN);
            }

            ProcessBuildOrders(build_orders);
        }
    }

    if (!build_orders->GetCount()) {
        switch (strategy) {
            case AI_STRATEGY_DEFENSIVE: {
                AddBuildOrder(&build_orders, player_team, ANTIMSSL, ATTRIB_RANGE);
                AddBuildOrder(&build_orders, player_team, ANTIMSSL, ATTRIB_ATTACK);

                if (!mode) {
                    AddBuildOrder(&build_orders, player_team, RADAR, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, GUNTURRT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, GUNTURRT, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, GUNTURRT, ATTRIB_ARMOR);
                    AddBuildOrder(&build_orders, player_team, GUNTURRT, ATTRIB_HITS);
                    AddBuildOrder(&build_orders, player_team, GUNTURRT, ATTRIB_RANGE);

                    AddBuildOrder(&build_orders, player_team, ANTIAIR, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, ANTIAIR, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, player_team, ANTIAIR, ATTRIB_ATTACK);
                }

            } break;

            case AI_STRATEGY_MISSILES: {
                AddBuildOrder(&build_orders, player_team, MISSLLCH, ATTRIB_RANGE);

                if (!mode) {
                    AddBuildOrder(&build_orders, player_team, SCANNER, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, MISSLLCH, ATTRIB_ATTACK);

                    AddBuildOrder(&build_orders, player_team, SCANNER, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, player_team, MISSLLCH, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, player_team, MSSLBOAT, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, player_team, MSSLBOAT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, MSSLBOAT, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_SCAN);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_SPEED);
                }
            } break;

            case AI_STRATEGY_AIR: {
                AddBuildOrder(&build_orders, player_team, FIGHTER, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, player_team, FIGHTER, ATTRIB_RANGE);

                AddBuildOrder(&build_orders, player_team, BOMBER, ATTRIB_ATTACK);

                if (!mode) {
                    AddBuildOrder(&build_orders, player_team, FIGHTER, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, FIGHTER, ATTRIB_HITS);
                    AddBuildOrder(&build_orders, player_team, FIGHTER, ATTRIB_ARMOR);

                    AddBuildOrder(&build_orders, player_team, AWAC, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, BOMBER, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, BOMBER, ATTRIB_HITS);
                    AddBuildOrder(&build_orders, player_team, BOMBER, ATTRIB_ARMOR);
                }

            } break;

            case AI_STRATEGY_SEA: {
                AddBuildOrder(&build_orders, player_team, BATTLSHP, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, player_team, BATTLSHP, ATTRIB_ARMOR);
                AddBuildOrder(&build_orders, player_team, BATTLSHP, ATTRIB_HITS);

                AddBuildOrder(&build_orders, player_team, MSSLBOAT, ATTRIB_RANGE);

                if (!mode) {
                    AddBuildOrder(&build_orders, player_team, SUBMARNE, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, SUBMARNE, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, BATTLSHP, ATTRIB_ROUNDS);

                    AddBuildOrder(&build_orders, player_team, MSSLBOAT, ATTRIB_ATTACK);

                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_SPEED);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_SPEED);
                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_SCAN);
                }

            } break;

            case AI_STRATEGY_SCOUT_HORDE: {
                AddBuildOrder(&build_orders, player_team, SCOUT, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, player_team, SCOUT, ATTRIB_ARMOR);
                AddBuildOrder(&build_orders, player_team, SCOUT, ATTRIB_HITS);
                AddBuildOrder(&build_orders, player_team, SCOUT, ATTRIB_SPEED);
            } break;

            case AI_STRATEGY_TANK_HORDE: {
                AddBuildOrder(&build_orders, player_team, SCOUT, ATTRIB_SPEED);

                AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_ROUNDS);
                AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_HITS);
                AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_ARMOR);
                AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_SPEED);

                if (!mode) {
                    AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_RANGE);
                }
            } break;

            case AI_STRATEGY_FAST_ATTACK: {
                AddBuildOrder(&build_orders, player_team, SCOUT, ATTRIB_SCAN);

                AddBuildOrder(&build_orders, player_team, ARTILLRY, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, player_team, ARTILLRY, ATTRIB_ROUNDS);
                AddBuildOrder(&build_orders, player_team, ARTILLRY, ATTRIB_RANGE);

                if (!mode) {
                    AddBuildOrder(&build_orders, player_team, ARTILLRY, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, player_team, SCOUT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, SCOUT, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, player_team, SP_FLAK, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, SP_FLAK, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, SP_FLAK, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, player_team, SP_FLAK, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, player_team, BOMBER, ATTRIB_ATTACK);

                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_SPEED);
                    AddBuildOrder(&build_orders, player_team, FASTBOAT, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_SPEED);
                    AddBuildOrder(&build_orders, player_team, CORVETTE, ATTRIB_SCAN);
                }
            } break;

            case AI_STRATEGY_COMBINED_ARMS: {
                AddBuildOrder(&build_orders, player_team, MISSLLCH, ATTRIB_RANGE);

                AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_ARMOR);
                AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_HITS);

                if (!mode) {
                    AddBuildOrder(&build_orders, player_team, SCANNER, ATTRIB_SCAN);
                    AddBuildOrder(&build_orders, player_team, SCANNER, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, player_team, MISSLLCH, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, MISSLLCH, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, player_team, TANK, ATTRIB_SPEED);
                }
            } break;

            case AI_STRATEGY_ESPIONAGE: {
                AddBuildOrder(&build_orders, player_team, COMMANDO, ATTRIB_SCAN);
                AddBuildOrder(&build_orders, player_team, COMMANDO, ATTRIB_SPEED);

                AddBuildOrder(&build_orders, player_team, MISSLLCH, ATTRIB_RANGE);

                if (!mode) {
                    AddBuildOrder(&build_orders, player_team, SUBMARNE, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, player_team, SUBMARNE, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, SCANNER, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, AWAC, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, player_team, MSSLBOAT, ATTRIB_RANGE);

                    AddBuildOrder(&build_orders, player_team, CLNTRANS, ATTRIB_SPEED);
                }
            } break;
        }
    }

    if (!mode) {
        ProcessBuildOrders(build_orders);
    }

    return build_orders;
}

void AiPlayer::ProcessBuildOrders(SmartObjectArray<BuildOrder> build_orders) {
    SmartObjectArray<ResourceID> unit_types;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == player_team && (*it).GetOrder() == ORDER_BUILD &&
            (*it).GetOrderState() != ORDER_STATE_UNIT_READY) {
            ResourceID constructed_unit_type = (*it).GetConstructedUnitType();

            if (unit_types->Find(&constructed_unit_type) < 0) {
                unit_types.PushBack(&constructed_unit_type);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == player_team && (*it).GetOrder() == ORDER_BUILD &&
            (*it).GetOrderState() != ORDER_STATE_UNIT_READY) {
            ResourceID constructed_unit_type = (*it).GetConstructedUnitType();

            if (unit_types->Find(&constructed_unit_type) < 0) {
                unit_types.PushBack(&constructed_unit_type);
            }
        }
    }

    for (int32_t i = build_orders.GetCount() - 1; i >= 0; --i) {
        if (unit_types->Find(&build_orders[i]->unit_type) < 0) {
            build_orders.Remove(i);
        }
    }
}

SmartObjectArray<BuildOrder> AiPlayer::ChooseGenericBuildOrders() {
    SmartObjectArray<BuildOrder> result;

    AddBuildOrder(&result, player_team, ANTIMSSL, ATTRIB_RANGE);
    AddBuildOrder(&result, player_team, ANTIMSSL, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, ANTIMSSL, ATTRIB_ROUNDS);

    AddBuildOrder(&result, player_team, RADAR, ATTRIB_SCAN);

    AddBuildOrder(&result, player_team, GUNTURRT, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, GUNTURRT, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, GUNTURRT, ATTRIB_ARMOR);
    AddBuildOrder(&result, player_team, GUNTURRT, ATTRIB_HITS);
    AddBuildOrder(&result, player_team, GUNTURRT, ATTRIB_RANGE);

    AddBuildOrder(&result, player_team, ANTIAIR, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, ANTIAIR, ATTRIB_RANGE);
    AddBuildOrder(&result, player_team, ANTIAIR, ATTRIB_ATTACK);

    AddBuildOrder(&result, player_team, MISSLLCH, ATTRIB_RANGE);
    AddBuildOrder(&result, player_team, MISSLLCH, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, MISSLLCH, ATTRIB_ROUNDS);

    AddBuildOrder(&result, player_team, SCANNER, ATTRIB_SCAN);

    AddBuildOrder(&result, player_team, MSSLBOAT, ATTRIB_RANGE);
    AddBuildOrder(&result, player_team, MSSLBOAT, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, MSSLBOAT, ATTRIB_ROUNDS);

    AddBuildOrder(&result, player_team, FASTBOAT, ATTRIB_SCAN);
    AddBuildOrder(&result, player_team, FASTBOAT, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, FASTBOAT, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, FASTBOAT, ATTRIB_RANGE);

    AddBuildOrder(&result, player_team, FIGHTER, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, FIGHTER, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, FIGHTER, ATTRIB_RANGE);
    AddBuildOrder(&result, player_team, FIGHTER, ATTRIB_HITS);
    AddBuildOrder(&result, player_team, FIGHTER, ATTRIB_ARMOR);

    AddBuildOrder(&result, player_team, BOMBER, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, BOMBER, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, BOMBER, ATTRIB_HITS);
    AddBuildOrder(&result, player_team, BOMBER, ATTRIB_ARMOR);

    AddBuildOrder(&result, player_team, AWAC, ATTRIB_SCAN);

    AddBuildOrder(&result, player_team, BATTLSHP, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, BATTLSHP, ATTRIB_ARMOR);
    AddBuildOrder(&result, player_team, BATTLSHP, ATTRIB_RANGE);
    AddBuildOrder(&result, player_team, BATTLSHP, ATTRIB_HITS);
    AddBuildOrder(&result, player_team, BATTLSHP, ATTRIB_ROUNDS);

    AddBuildOrder(&result, player_team, SUBMARNE, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, SUBMARNE, ATTRIB_SCAN);
    AddBuildOrder(&result, player_team, SUBMARNE, ATTRIB_ROUNDS);

    AddBuildOrder(&result, player_team, CORVETTE, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, CORVETTE, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, CORVETTE, ATTRIB_SCAN);
    AddBuildOrder(&result, player_team, CORVETTE, ATTRIB_RANGE);

    AddBuildOrder(&result, player_team, SCOUT, ATTRIB_SPEED);
    AddBuildOrder(&result, player_team, SCOUT, ATTRIB_SCAN);

    AddBuildOrder(&result, player_team, TANK, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, TANK, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, TANK, ATTRIB_HITS);
    AddBuildOrder(&result, player_team, TANK, ATTRIB_ARMOR);

    AddBuildOrder(&result, player_team, ARTILLRY, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, ARTILLRY, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, ARTILLRY, ATTRIB_RANGE);
    AddBuildOrder(&result, player_team, ARTILLRY, ATTRIB_SPEED);

    AddBuildOrder(&result, player_team, SP_FLAK, ATTRIB_ATTACK);
    AddBuildOrder(&result, player_team, SP_FLAK, ATTRIB_ROUNDS);
    AddBuildOrder(&result, player_team, SP_FLAK, ATTRIB_RANGE);

    AddBuildOrder(&result, player_team, COMMANDO, ATTRIB_SCAN);
    AddBuildOrder(&result, player_team, COMMANDO, ATTRIB_SPEED);

    ProcessBuildOrders(result);

    return result;
}

void AiPlayer::ChooseInitialUpgrades(int32_t team_gold) {
    SmartObjectArray<BuildOrder> strategic_orders = ChooseStrategicBuildOrders(true);
    SmartObjectArray<BuildOrder> orders;

    ChooseUpgrade(strategic_orders, orders);

    while (build_order.unit_type != INVALID_ID && upgrade_cost <= team_gold) {
        team_gold -= upgrade_cost;
        UnitsManager_TeamMissionSupplies[player_team].team_gold -= upgrade_cost;
        UnitsManager_TeamInfo[player_team].stats_gold_spent_on_upgrades += upgrade_cost;

        UpgradeUnitType();

        strategic_orders = ChooseStrategicBuildOrders(true);

        ChooseUpgrade(strategic_orders, orders);
    }
}

void AiPlayer::UpgradeUnitType() {
    if (build_order.unit_type != INVALID_ID) {
        SmartPointer<UnitValues> unit_values(
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], build_order.unit_type));
        auto upgrade_level =
            TeamUnits_UpgradeOffsetFactor(player_team, build_order.unit_type, build_order.primary_attribute);

        if (build_order.primary_attribute == RESEARCH_TOPIC_HITS ||
            build_order.primary_attribute == RESEARCH_TOPIC_SPEED) {
            auto current_level = unit_values->GetAttribute(build_order.primary_attribute);

            if (current_level + upgrade_level > UINT16_MAX) {
                upgrade_level = UINT16_MAX - current_level;
            }
        }

        unit_values = new (std::nothrow) UnitValues(*unit_values);

        unit_values->UpdateVersion();
        unit_values->SetUnitsBuilt(0);

        unit_values->AddAttribute(build_order.primary_attribute, upgrade_level);

        UnitsManager_TeamInfo[player_team].team_units->SetCurrentUnitValues(build_order.unit_type, *unit_values);

        if (Remote_IsNetworkGame) {
            Remote_SendNetPacket_10(player_team, build_order.unit_type);
        }
    }
}

AiPlayer::AiPlayer() : info_map(nullptr), mine_map(nullptr), target_location(1, 1) {}

AiPlayer::~AiPlayer() {
    if (info_map) {
        for (int32_t i = 0; i < dimension_x; ++i) {
            delete[] info_map[i];
        }

        delete[] info_map;
        info_map = nullptr;
    }

    if (mine_map) {
        for (int32_t i = 0; i < dimension_x; ++i) {
            delete[] mine_map[i];
        }

        delete[] mine_map;
        mine_map = nullptr;
    }
}

int32_t AiPlayer::GetStrategy() const { return strategy; }

int16_t AiPlayer::GetTargetTeam() const { return target_team; }

uint8_t** AiPlayer::GetInfoMap() { return info_map; }

Point AiPlayer::GetTargetLocation() const { return target_location; }

uint16_t AiPlayer::GetField5() const { return field_5; }

int8_t** AiPlayer::GetMineMap() { return mine_map; }

void AiPlayer::AddTransportOrder(TransportOrder* transport_order) { transport_orders.PushBack(*transport_order); }

Task* AiPlayer::FindManager(Point site) {
    SmartPointer<Task> task;

    if (task_list.GetCount() > 0) {
        Point position;
        Point location;
        SmartList<Task>::Iterator task_it = task_list.Begin();
        Rect bounds;
        int32_t distance;
        int32_t minimum_distance;

        task = *task_it;

        task->GetBounds(&bounds);

        position = site;

        position -= Point(bounds.ulx, bounds.uly);

        minimum_distance = position.x * position.x + position.y * position.y;

        for (++task_it; task_it != task_list.End(); ++task_it) {
            (*task_it).GetBounds(&bounds);

            position = site;

            position -= Point(bounds.ulx, bounds.uly);

            distance = position.x * position.x + position.y * position.y;

            if (distance < minimum_distance) {
                task = *task_it;
                minimum_distance = distance;
            }
        }

    } else {
        task = new (std::nothrow) TaskManageBuildings(player_team, site);

        task_list.PushBack(*task);
        TaskManager.AppendTask(*task);

        defense_reserve_task = new (std::nothrow) TaskDefenseReserve(player_team, site);
        TaskManager.AppendTask(*defense_reserve_task);

        attack_reserve_task = new (std::nothrow) TaskAttackReserve(player_team, site);
        TaskManager.AppendTask(*attack_reserve_task);
    }

    return &*task;
}

SmartList<SpottedUnit>& AiPlayer::GetSpottedUnits() { return spotted_units; }

void AiPlayer::AddMilitaryUnit(UnitInfo* unit) {
    if (Access_IsValidAttackTargetType(unit->GetUnitType(), FIGHTER)) {
        air_force.PushBack(*unit);

    } else {
        ground_forces.PushBack(*unit);
    }

    if (unit->shots > 0 && !need_init) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindAttack(*unit), true);
    }
}

bool AiPlayer::IsTargetTeamDefined() const { return (target_team >= PLAYER_TEAM_RED && target_team < PLAYER_TEAM_MAX); }

bool AiPlayer::IsTargetTeam(const uint8_t team) const {
    bool result;

    if (ResourceManager_GetSettings()->GetNumericValue("cheating_computer") >= COMPUTER_CHEATING_LEVEL_INSUFFERABLE) {
        result = (team == target_team);

    } else {
        result = (team != player_team) && (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) &&
                 (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED);
    }

    return result;
}

void AiPlayer::DetermineTargetTeam() {
    if (ResourceManager_GetSettings()->GetNumericValue("cheating_computer") >= COMPUTER_CHEATING_LEVEL_SHAMELESS) {
        int32_t player_team_points = UnitsManager_TeamInfo[player_team].team_points;
        int32_t team_building_counts[PLAYER_TEAM_MAX];
        int32_t target_team_points;
        int32_t enemy_team_points;

        memset(team_building_counts, 0, sizeof(team_building_counts));

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            ++team_building_counts[(*it).team];
        }

        if (IsTargetTeamDefined()) {
            target_team_points = UnitsManager_TeamInfo[target_team].team_points;

            if (UnitsManager_TeamInfo[target_team].team_type != TEAM_TYPE_COMPUTER) {
                if (target_team_points < player_team_points) {
                    ++target_team_points;
                } else {
                    target_team_points += 1000;
                }
            }

        } else {
            target_team_points = -1;
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE && player_team != team && target_team != team) {
                enemy_team_points = UnitsManager_TeamInfo[team].team_points;

                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER) {
                    if (enemy_team_points < player_team_points) {
                        ++enemy_team_points;
                    } else {
                        enemy_team_points += 1000;
                    }
                }

                if (!IsTargetTeamDefined() || target_team_points < enemy_team_points ||
                    (target_team_points == enemy_team_points &&
                     team_building_counts[target_team] < team_building_counts[team])) {
                    target_team = team;
                    target_team_points = enemy_team_points;
                }
            }
        }

    } else {
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (team != player_team) {
                if (!IsTargetTeamDefined() ||
                    (UnitsManager_TeamInfo[team].team_points > UnitsManager_TeamInfo[target_team].team_points &&
                     (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                      UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED))) {
                    target_team = team;
                }
            }
        }
    }
}

void AiPlayer::BeginTurn() {
    if (UnitsManager_TeamInfo[player_team].team_type == TEAM_TYPE_COMPUTER) {
        if (!AiPlayer_TerrainDistanceField) {
            AiPlayer_TerrainDistanceField = std::make_unique<TerrainDistanceField>(ResourceManager_MapSize);
        }

        InvalidateThreatMaps();

        field_16 = 0;

        transport_orders.Clear();

        if (GameManager_PlayMode == PLAY_MODE_TURN_BASED) {
            const char* team_colors[PLAYER_TEAM_MAX - 1] = {_(ba19), _(d486), _(ec7d), _(0d5a)};
            char message[80];

            sprintf(message, _(2ab8), team_colors[player_team]);

            MessageManager_DrawMessage(message, 0, 0);
        }

        Point position = GetTeamClusterCoordinates(player_team);

        DetermineTargetLocation(position);

        UpdatePriorityTasks();

        DetermineTargetTeam();

        if (need_init) {
            SDL_assert(check_assaults_task == nullptr);

            check_assaults_task = new (std::nothrow) TaskCheckAssaults(player_team);

            TaskManager.AppendTask(*check_assaults_task);

            if (strategy == AI_STRATEGY_RANDOM) {
                switch (UnitsManager_TeamInfo[player_team].team_clan) {
                    case TEAM_CLAN_THE_CHOSEN: {
                        strategy = AI_STRATEGY_AIR;
                    } break;

                    case TEAM_CLAN_CRIMSON_PATH: {
                        strategy = AI_STRATEGY_SEA;
                    } break;

                    case TEAM_CLAN_AYERS_HAND: {
                        strategy = AI_STRATEGY_MISSILES;
                    } break;

                    case TEAM_CLAN_SACRED_EIGHTS: {
                        strategy = AI_STRATEGY_FAST_ATTACK;
                    } break;

                    case TEAM_CLAN_7_KNIGHTS: {
                        strategy = AI_STRATEGY_DEFENSIVE;
                    } break;

                    default: {
                        strategy = AI_STRATEGY_COMBINED_ARMS;
                    } break;
                }
            }

            if (!field_3) {
                RollField3();
            }

            if (field_5 < 0) {
                RollField5();
            }

            if (field_7 < 0) {
                RollField7();
            }

            UpdateWeightTables();

            dimension_x = ResourceManager_MapSize.x;

            if (!info_map) {
                info_map = new (std::nothrow) uint8_t*[ResourceManager_MapSize.x];

                for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
                    info_map[i] = new (std::nothrow) uint8_t[ResourceManager_MapSize.y];

                    memset(info_map[i], INFO_MAP_NO_INFO, ResourceManager_MapSize.y);
                }
            }

            if (!mine_map) {
                mine_map = new (std::nothrow) int8_t*[ResourceManager_MapSize.x];

                for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
                    mine_map[i] = new (std::nothrow) int8_t[ResourceManager_MapSize.y];

                    memset(mine_map[i], 0, ResourceManager_MapSize.y);
                }
            }

            for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                    if (UnitsManager_TeamInfo[player_team].heat_map &&
                        UnitsManager_TeamInfo[player_team].heat_map->GetComplete(x, y)) {
                        info_map[x][y] = INFO_MAP_EXPLORED;
                    }
                }
            }

            if (position.x || position.y) {
                SmartPointer<Task> survey(new (std::nothrow) TaskSurvey(player_team, position));

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                     it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                    if ((*it).team == player_team && (*it).GetUnitType() == SURVEYOR) {
                        survey->AddUnit(*it);
                    }
                }

                TaskManager.AppendTask(*survey);

                SmartPointer<Task> explore(new (std::nothrow) TaskExplore(player_team, position));

                TaskManager.AppendTask(*explore);
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
                 it != UnitsManager_GroundCoverUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).GetOrder() != ORDER_IDLE &&
                    ((*it).GetUnitType() == WTRPLTFM || (*it).GetUnitType() == BRIDGE)) {
                    AddBuilding(&*it);
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                 it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).GetOrder() != ORDER_IDLE) {
                    AddBuilding(&*it);
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).GetOrder() == ORDER_BUILD) {
                    AddBuilding(&*it);
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).GetOrder() == ORDER_IDLE &&
                    (*it).GetOrderState() == ORDER_STATE_PREPARE_STORE) {
                    SmartPointer<Task> move(new (std::nothrow) TaskMove(&*it, &MoveFinishedCallback));

                    TaskManager.AppendTask(*move);
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                 it != UnitsManager_MobileAirUnits.End(); ++it) {
                if ((*it).team == player_team) {
                    if ((*it).GetUnitType() == AWAC || (*it).GetUnitType() == BOMBER) {
                        SmartPointer<Task> escort(new (std::nothrow) TaskEscort(&*it, FIGHTER));

                        TaskManager.AppendTask(*escort);
                    }

                    if ((*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0) {
                        AddMilitaryUnit(&*it);
                    }
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == player_team) {
                    if ((*it).GetUnitType() == MISSLLCH || (*it).GetUnitType() == SCANNER) {
                        SmartPointer<Task> tank_escort(new (std::nothrow) TaskEscort(&*it, TANK));
                        TaskManager.AppendTask(*tank_escort);

                        SmartPointer<Task> sp_flak_escort(new (std::nothrow) TaskEscort(&*it, SP_FLAK));
                        TaskManager.AppendTask(*sp_flak_escort);
                    }

                    if ((*it).GetUnitType() == SP_FLAK || (*it).GetUnitType() == FASTBOAT) {
                        SmartPointer<Task> escort(new (std::nothrow) TaskEscort(&*it, FIGHTER));
                        TaskManager.AppendTask(*escort);
                    }

                    if ((*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0) {
                        AddMilitaryUnit(&*it);
                    }
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                 it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == player_team) {
                    if ((*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0) {
                        AddMilitaryUnit(&*it);
                    }
                }
            }

            if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_EXPERT &&
                ResourceManager_GetSettings()->GetNumericValue("cheating_computer") >=
                    COMPUTER_CHEATING_LEVEL_TOLERABLE) {
                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team != player_team &&
                        ((*it).GetUnitType() == MININGST || (*it).GetUnitType() == GREENHSE)) {
                        Ai_UnitSpotted(&*it, player_team);
                    }
                }
            }

            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                if (IsKeyFacility((*it).GetUnit()->GetUnitType()) && IsTargetTeam((*it).GetUnit()->team)) {
                    DetermineAttack(&*it, TASK_PRIORITY_ATTACK_DEFAULT);
                }
            }

            if (!find_mines_task) {
                bool is_found = false;
                int32_t grid_x;
                int32_t grid_y;

                for (grid_x = 0; grid_x < ResourceManager_MapSize.x && !is_found; ++grid_x) {
                    for (grid_y = 0; grid_y < ResourceManager_MapSize.y && !is_found; ++grid_y) {
                        if (mine_map[grid_x][grid_y] > 0) {
                            is_found = true;
                        }
                    }
                }

                if (is_found) {
                    find_mines_task = new (std::nothrow) TaskFindMines(player_team, Point(grid_x, grid_y));

                    TaskManager.AppendTask(*find_mines_task);
                }
            }

            SmartPointer<Task> mine_assistant(new (std::nothrow) TaskMineAssistant(player_team));
            TaskManager.AppendTask(*mine_assistant);

            SmartPointer<Task> update_terrain(new (std::nothrow) TaskUpdateTerrain(player_team));
            TaskManager.AppendTask(*update_terrain);

            if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_APPRENTICE) {
                SmartPointer<Task> scavenge(new (std::nothrow) TaskScavenge(player_team));
                TaskManager.AppendTask(*scavenge);

                SmartPointer<Task> assist_move(new (std::nothrow) TaskAssistMove(player_team));
                TaskManager.AppendTask(*assist_move);
            }

            need_init = false;
        }

        bool teams[PLAYER_TEAM_MAX];

        AiAttack_GetTargetTeams(player_team, teams);

        for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
            if (!(*it).GetTask()) {
                UnitInfo* target = (*it).GetUnit();

                if (AiAttack_IsWithinReach(target, player_team, teams)) {
                    DetermineAttack(&*it, TASK_PRIORITY_ATTACK_MOBILE);

                } else if (IsTargetTeam(target->team) || target->team == PLAYER_TEAM_ALIEN) {
                    if ((target->GetUnitType() == CONSTRCT && target->GetOrder() == ORDER_BUILD) ||
                        IsKeyFacility(target->GetUnitType()) ||
                        (target->team == PLAYER_TEAM_ALIEN &&
                         (target->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)))) {
                        DetermineAttack(&*it, TASK_PRIORITY_ATTACK_DEFAULT);
                    }
                }
            }
        }

        int32_t fuel_reserves = 0;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == player_team && (*it).GetUnitType() == FDUMP) {
                fuel_reserves += (*it).storage;
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == player_team && (*it).GetUnitType() == MININGST && (*it).GetOrder() != ORDER_POWER_ON &&
                (*it).GetOrder() != ORDER_IDLE) {
                UnitsManager_SetNewOrder(&*it, ORDER_POWER_ON, ORDER_STATE_INIT);
            }
        }

        if (fuel_reserves >= 10) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                 it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == player_team &&
                    ((*it).GetUnitType() == COMMTWR || (*it).GetUnitType() == GREENHSE ||
                     (*it).GetUnitType() == RESEARCH) &&
                    (*it).GetOrder() != ORDER_POWER_ON && (*it).GetOrder() != ORDER_IDLE) {
                    UnitsManager_SetNewOrder(&*it, ORDER_POWER_ON, ORDER_STATE_INIT);
                }
            }
        }

        SmartObjectArray<BuildOrder> strategic_orders = ChooseStrategicBuildOrders(false);
        SmartObjectArray<BuildOrder> generic_orders = ChooseGenericBuildOrders();

        ChooseUpgrade(strategic_orders, generic_orders);

        int32_t team_gold = UnitsManager_TeamInfo[player_team].team_units->GetGold();

        while (build_order.unit_type != INVALID_ID && upgrade_cost <= team_gold) {
            team_gold -= upgrade_cost;
            UnitsManager_TeamInfo[player_team].team_units->SetGold(team_gold);
            UnitsManager_TeamInfo[player_team].stats_gold_spent_on_upgrades += upgrade_cost;

            UpgradeUnitType();

            if (Remote_IsNetworkGame) {
                Remote_SendNetPacket_09(player_team);
            }

            ChooseUpgrade(strategic_orders, generic_orders);
        }

        RegisterIdleUnits();
        DetermineResearchProjects();

        for (auto it = UnitsManager_StationaryUnits.Begin(), it_end = UnitsManager_StationaryUnits.End(); it != it_end;
             ++it) {
            if ((*it).team == player_team) {
                if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_AVERAGE) {
                    if ((*it).GetUnitType() == RADAR || (*it).GetUnitType() == GUNTURRT ||
                        (*it).GetUnitType() == ARTYTRRT || (*it).GetUnitType() == ANTIMSSL ||
                        (*it).GetUnitType() == ANTIAIR) {
                        if (ShouldUpgradeUnit(it->Get())) {
                            AiPlayer_UpgradeStationaryUnit(it->Get());
                        }
                    }
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == player_team) {
                if ((*it).GetTask()) {
                    Task_RemindMoveFinished(&*it);
                }

                if ((*it).shots > 0) {
                    TaskManager.AppendReminder(new (std::nothrow) class RemindAttack(*it), true);
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
             it != UnitsManager_MobileAirUnits.End(); ++it) {
            if ((*it).team == player_team) {
                if ((*it).GetTask()) {
                    Task_RemindMoveFinished(&*it);
                }

                if ((*it).shots > 0) {
                    TaskManager.AppendReminder(new (std::nothrow) class RemindAttack(*it), true);
                }
            }
        }

        TaskManager.BeginTurn(player_team);
    }
}

void AiPlayer::GuessEnemyAttackDirections() {
    if (info_map) {
        const World* world = ResourceManager_GetActiveWorld();
        AccessMap access_map(world);
        SmartList<UnitInfo> units;
        Point site;

        for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
            for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
                info_map[site.x][site.y] &= ~INFO_MAP_FRONTIER;
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == player_team) {
                units.PushBack(*it);
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
             it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == player_team && IsPotentialSpotter(player_team, &*it)) {
                units.PushBack(*it);
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
             it != UnitsManager_MobileAirUnits.End(); ++it) {
            if ((*it).team == player_team && IsPotentialSpotter(player_team, &*it)) {
                units.PushBack(*it);
            }
        }

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            ZoneWalker walker(Point((*it).grid_x, (*it).grid_y), (*it).GetBaseValues()->GetAttribute(ATTRIB_SCAN) + 8);

            do {
                access_map(walker.GetGridX(), walker.GetGridY()) = 0x01;
            } while (walker.FindNext());
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (team != player_team && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
                    for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
                        if (UnitsManager_TeamInfo[team].heat_map &&
                            UnitsManager_TeamInfo[team].heat_map->GetComplete(site.x, site.y)) {
                            access_map(site.x, site.y) = 0x00;
                        }
                    }
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            CircumferenceWalker walker(Point((*it).grid_x, (*it).grid_y),
                                       (*it).GetBaseValues()->GetAttribute(ATTRIB_SCAN) + 10);

            do {
                if (!access_map(walker.GetGridX(), walker.GetGridY())) {
                    UpdateAccessMap(*walker.GetGridXY(), Point((*it).grid_x, (*it).grid_y), access_map.GetMap());
                }
            } while (walker.FindNext());
        }
    }
}

void AiPlayer::PlanMinefields() {
    if (info_map && field_7 > 0) {
        const World* world = ResourceManager_GetActiveWorld();
        AccessMap access_map(world);

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == player_team) {
                if ((*it).GetUnitType() == GUNTURRT || (*it).GetUnitType() == ANTIMSSL ||
                    (*it).GetUnitType() == ARTYTRRT) {
                    ZoneWalker walker(Point((*it).grid_x, (*it).grid_y),
                                      (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE));
                    int32_t surface_type = Access_GetSurfaceType((*it).grid_x, (*it).grid_y);

                    do {
                        if (Access_GetSurfaceType(walker.GetGridX(), walker.GetGridY()) == surface_type) {
                            access_map(walker.GetGridX(), walker.GetGridY()) = 0x01;
                        }

                    } while (walker.FindNext());

                } else if ((*it).flags & BUILDING) {
                    ZoneWalker walker(Point((*it).grid_x, (*it).grid_y), 8);
                    int32_t surface_type = Access_GetSurfaceType((*it).grid_x, (*it).grid_y);

                    do {
                        if (Access_GetSurfaceType(walker.GetGridX(), walker.GetGridY()) == surface_type) {
                            access_map(walker.GetGridX(), walker.GetGridY()) = 0x01;
                        }

                    } while (walker.FindNext());
                }
            }
        }

        for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
             ++it) {
            if ((*it).GetTeam() == player_team && (*it).GetType() == TaskType_TaskCreateBuilding) {
                TaskCreateBuilding* create_building = dynamic_cast<TaskCreateBuilding*>(&*it);

                if (create_building->Task_vfunc28()) {
                    if (ResourceManager_GetUnit(create_building->GetUnitType()).GetFlags() & BUILDING) {
                        Point position = create_building->DeterminePosition();
                        ZoneWalker walker(position, 8);
                        int32_t surface_type = Access_GetSurfaceType(position.x, position.y);

                        do {
                            if (Access_GetSurfaceType(walker.GetGridX(), walker.GetGridY()) == surface_type) {
                                access_map(walker.GetGridX(), walker.GetGridY()) = 0x01;
                            }

                        } while (walker.FindNext());
                    }
                }
            }
        }

        /// \todo The entire block is dead code due to the inner for loops.
        for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
             ++it) {
            if ((*it).GetTeam() == player_team && (*it).GetType() == TaskType_TaskCreateBuilding) {
                TaskCreateBuilding* create_building = dynamic_cast<TaskCreateBuilding*>(&*it);

                if (create_building->Task_vfunc28()) {
                    Rect bounds;

                    create_building->GetBounds(&bounds);

                    if (ResourceManager_GetUnit(create_building->GetUnitType()).GetFlags() & BUILDING) {
                        bounds.ulx = std::max(0, bounds.ulx - 2);
                        bounds.uly = std::max(0, bounds.uly - 2);
                        bounds.lrx = std::min(static_cast<int32_t>(ResourceManager_MapSize.x), bounds.lrx + 2);
                        bounds.lry = std::min(static_cast<int32_t>(ResourceManager_MapSize.y), bounds.lry + 2);

                        for (int32_t x = bounds.ulx; x < 0; ++x) {
                            for (int32_t y = bounds.uly; y < 0; ++y) {
                                access_map(x, y) = 0x00;
                            }
                        }
                    }
                }
            }
        }

        /// \todo The entire block is dead code due to the inner for loops.
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            if (ResourceManager_GetUnit((*it).GetUnitType()).GetFlags() & BUILDING) {
                bounds.ulx = std::max(0, bounds.ulx - 2);
                bounds.uly = std::max(0, bounds.uly - 2);
                bounds.lrx = std::min(static_cast<int32_t>(ResourceManager_MapSize.x), bounds.lrx + 2);
                bounds.lry = std::min(static_cast<int32_t>(ResourceManager_MapSize.y), bounds.lry + 2);

                for (int32_t x = bounds.ulx; x < 0; ++x) {
                    for (int32_t y = bounds.uly; y < 0; ++y) {
                        access_map(x, y) = 0x00;
                    }
                }
            }
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (team != player_team) {
                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                    for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
                        for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                            if (UnitsManager_TeamInfo[team].heat_map &&
                                UnitsManager_TeamInfo[team].heat_map->GetComplete(x, y)) {
                                access_map(x, y) = 0x00;
                            }
                        }
                    }
                }
            }
        }

        int32_t counter1 = 0;
        int32_t counter2 = 0;
        int32_t counter3 = 0;

        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                if (access_map(x, y)) {
                    ++counter1;

                    if (info_map[x][y] & INFO_MAP_MINE_FIELD) {
                        ++counter2;

                        access_map(x, y) = 0x00;
                    }

                } else {
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).team == player_team && ((*it).GetUnitType() == LANDMINE || (*it).GetUnitType() == SEAMINE)) {
                if (access_map((*it).grid_x, (*it).grid_y)) {
                    ++counter3;

                    info_map[(*it).grid_x][(*it).grid_y] &= ~INFO_MAP_MINE_FIELD;
                    access_map((*it).grid_x, (*it).grid_y) = 0x00;
                }
            }
        }

        int32_t probability = ((field_7 * counter1) / 100) - counter2 - counter3;

        if (probability >= 32 - counter2) {
            probability = 32 - counter2;
        }

        counter1 -= counter2 + counter3;

        if (probability > 0) {
            for (int32_t x = 0; x < ResourceManager_MapSize.x && probability; ++x) {
                for (int32_t y = 0; y < ResourceManager_MapSize.y && probability; ++y) {
                    if (access_map(x, y)) {
                        if (Randomizer_Generate(counter1) + 1 <= probability) {
                            info_map[x][y] |= INFO_MAP_MINE_FIELD;
                            --probability;
                        }

                        --counter1;
                    }
                }
            }

            if (!place_mines_task) {
                place_mines_task = (new (std::nothrow) TaskPlaceMines(player_team));
                TaskManager.AppendTask(*place_mines_task);
            }
        }
    }
}

void AiPlayer::ChangeTasksPendingFlag(bool value) { tasks_pending = value; }

bool AiPlayer::CheckEndTurn() {
    bool result;

    if (!need_init) {
        if (!tasks_pending) {
            if (!field_16) {
                TaskManager.MarkTasksForProcessing(player_team);
            }

            ++field_16;

            if (field_16 >= 10) {
                if (field_16 != 10 || AreActionsPending()) {
                    if (field_16 != 20) {
                        if (field_16 != 30) {
                            if (field_16 != 60) {
                                if (field_16 > 120) {
                                    if (GameManager_PlayMode == PLAY_MODE_TURN_BASED ||
                                        GameManager_GameState == GAME_STATE_9_END_TURN || IsDemoMode()) {
                                        GameManager_RefreshOrders(player_team, true);
                                    }
                                }

                                result = false;

                            } else {
                                TaskManager.EndTurn(player_team);

                                result = true;
                            }

                        } else {
                            RegisterReadyAndAbleUnits(&UnitsManager_MobileLandSeaUnits);
                            RegisterReadyAndAbleUnits(&UnitsManager_MobileAirUnits);

                            result = true;
                        }

                    } else {
                        CheckAttacks();

                        result = true;
                    }

                } else {
                    TaskManager.EndTurn(player_team);

                    result = true;
                }

            } else {
                result = false;
            }

        } else {
            field_16 = 0;

            result = false;
        }

    } else {
        BeginTurn();

        result = true;
    }

    return result;
}

bool AiPlayer::CreateBuilding(ResourceID unit_type, Point position, Task* task) {
    return dynamic_cast<TaskManageBuildings*>(FindManager(position))
        ->CreateBuilding(unit_type, task, task->GetPriority());
}

void AiPlayer::Init(uint16_t team) {
    strategy = AI_STRATEGY_RANDOM;
    field_3 = 0;
    target_team = -1;
    field_5 = -1;
    field_7 = -1;

    if (info_map) {
        for (int32_t x = 0; x < dimension_x; ++x) {
            delete[] info_map[x];
        }

        delete[] info_map;
        info_map = nullptr;
    }

    if (mine_map) {
        for (int32_t x = 0; x < dimension_x; ++x) {
            delete[] mine_map[x];
        }

        delete[] mine_map;
        mine_map = nullptr;
    }

    for (int32_t i = 0; i < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++i) {
        AiPlayer_ThreatMaps[i].SetRiskLevel(0);
    }

    spotted_units.Clear();
    air_force.Clear();
    ground_forces.Clear();

    player_team = team;
    need_init = true;
    tasks_pending = true;

    task_list.Clear();

    find_mines_task = nullptr;
    place_mines_task = nullptr;

    task_clear_ground_zone = nullptr;
    task_clear_air_zone = nullptr;

    check_assaults_task = nullptr;
    attack_reserve_task = nullptr;
    defense_reserve_task = nullptr;

    for (int32_t i = 0; i < AttackTaskLimit; ++i) {
        attack_tasks[i] = nullptr;
    }

    int32_t opponent = ResourceManager_GetSettings()->GetNumericValue("opponent");

    weight_table_ground_defense.Clear();
    weight_table_ground_defense.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        weight_table_ground_defense.Add(COMMANDO, 2);
    }

    weight_table_ground_defense.Add(TANK, 1);
    weight_table_ground_defense.Add(ARTILLRY, 1);
    weight_table_ground_defense.Add(ROCKTLCH, 1);
    weight_table_ground_defense.Add(MISSLLCH, 2);
    weight_table_ground_defense.Add(BATTLSHP, 1);
    weight_table_ground_defense.Add(MSSLBOAT, 2);
    weight_table_ground_defense.Add(BOMBER, 2);
    weight_table_ground_defense.Add(JUGGRNT, 1);
    weight_table_ground_defense.Add(ALNTANK, 1);
    weight_table_ground_defense.Add(ALNASGUN, 1);
    weight_table_ground_defense.Add(ALNPLANE, 1);

    weight_table_scout.Clear();
    weight_table_scout.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        weight_table_scout.Add(COMMANDO, 1);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_scout.Add(ARTYTRRT, 1);
        weight_table_scout.Add(ANTIMSSL, 1);
    }

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        weight_table_scout.Add(GUNTURRT, 1);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_scout.Add(SUBMARNE, 1);
    }

    weight_table_scout.Add(SCOUT, 2);
    weight_table_scout.Add(TANK, 1);
    weight_table_scout.Add(ARTILLRY, 2);
    weight_table_scout.Add(ROCKTLCH, 1);
    weight_table_scout.Add(MISSLLCH, 2);
    weight_table_scout.Add(BATTLSHP, 2);
    weight_table_scout.Add(MSSLBOAT, 2);
    weight_table_scout.Add(BOMBER, 3);
    weight_table_scout.Add(JUGGRNT, 1);
    weight_table_scout.Add(ALNTANK, 1);
    weight_table_scout.Add(ALNASGUN, 1);
    weight_table_scout.Add(ALNPLANE, 1);

    weight_table_tanks.Clear();
    weight_table_tanks.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        weight_table_tanks.Add(GUNTURRT, 1);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_tanks.Add(ANTIMSSL, 1);
    }

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        weight_table_tanks.Add(COMMANDO, 1);
    }

    weight_table_tanks.Add(TANK, 2);
    weight_table_tanks.Add(ARTILLRY, 1);
    weight_table_tanks.Add(ROCKTLCH, 1);
    weight_table_tanks.Add(MISSLLCH, 2);
    weight_table_tanks.Add(BATTLSHP, 2);
    weight_table_tanks.Add(MSSLBOAT, 2);
    weight_table_tanks.Add(BOMBER, 3);
    weight_table_tanks.Add(JUGGRNT, 1);
    weight_table_tanks.Add(ALNTANK, 1);
    weight_table_tanks.Add(ALNASGUN, 1);
    weight_table_tanks.Add(ALNPLANE, 1);

    weight_table_assault_guns.Clear();
    weight_table_assault_guns.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        weight_table_assault_guns.Add(COMMANDO, 1);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_assault_guns.Add(ANTIMSSL, 1);
    }

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        weight_table_assault_guns.Add(GUNTURRT, 1);
    }

    weight_table_assault_guns.Add(TANK, 2);
    weight_table_assault_guns.Add(ARTILLRY, 1);
    weight_table_assault_guns.Add(ROCKTLCH, 1);
    weight_table_assault_guns.Add(MISSLLCH, 2);
    weight_table_assault_guns.Add(BATTLSHP, 2);
    weight_table_assault_guns.Add(MSSLBOAT, 2);
    weight_table_assault_guns.Add(BOMBER, 3);
    weight_table_assault_guns.Add(JUGGRNT, 1);
    weight_table_assault_guns.Add(ALNTANK, 1);
    weight_table_assault_guns.Add(ALNASGUN, 1);
    weight_table_assault_guns.Add(ALNPLANE, 1);

    weight_table_missile_launcher.Clear();
    weight_table_missile_launcher.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        weight_table_missile_launcher.Add(COMMANDO, 1);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_missile_launcher.Add(ANTIMSSL, 1);
    }

    weight_table_missile_launcher.Add(TANK, 1);
    weight_table_missile_launcher.Add(ARTILLRY, 1);
    weight_table_missile_launcher.Add(ROCKTLCH, 1);
    weight_table_missile_launcher.Add(MISSLLCH, 2);
    weight_table_missile_launcher.Add(BATTLSHP, 1);
    weight_table_missile_launcher.Add(MSSLBOAT, 2);
    weight_table_missile_launcher.Add(BOMBER, 3);
    weight_table_missile_launcher.Add(JUGGRNT, 1);
    weight_table_missile_launcher.Add(ALNTANK, 1);
    weight_table_missile_launcher.Add(ALNASGUN, 1);
    weight_table_missile_launcher.Add(ALNPLANE, 1);

    weight_table_air_defense.Clear();
    weight_table_air_defense.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        weight_table_air_defense.Add(COMMANDO, 2);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_air_defense.Add(ANTIMSSL, 1);
    }

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        weight_table_air_defense.Add(GUNTURRT, 1);
    }

    weight_table_air_defense.Add(SCOUT, 1);
    weight_table_air_defense.Add(TANK, 1);
    weight_table_air_defense.Add(ARTILLRY, 1);
    weight_table_air_defense.Add(ROCKTLCH, 1);
    weight_table_air_defense.Add(MISSLLCH, 1);
    weight_table_air_defense.Add(BATTLSHP, 1);
    weight_table_air_defense.Add(MSSLBOAT, 1);
    weight_table_air_defense.Add(JUGGRNT, 1);
    weight_table_air_defense.Add(ALNTANK, 1);
    weight_table_air_defense.Add(ALNASGUN, 1);

    weight_table_aircrafts.Clear();
    weight_table_aircrafts.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_aircrafts.Add(ANTIAIR, 3);
    }

    weight_table_aircrafts.Add(SP_FLAK, 3);
    weight_table_aircrafts.Add(FASTBOAT, 3);
    weight_table_aircrafts.Add(FIGHTER, 1);
    weight_table_aircrafts.Add(ALNPLANE, 1);

    weight_table_bomber.Clear();
    weight_table_bomber.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_bomber.Add(ANTIAIR, 1);
    }

    weight_table_bomber.Add(SP_FLAK, 1);
    weight_table_bomber.Add(FASTBOAT, 1);
    weight_table_bomber.Add(FIGHTER, 3);

    weight_table_bomber.Add(ALNPLANE, 3);

    weight_table_9.Clear();
    weight_table_9.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_9.Add(ANTIAIR, 1);
    }

    weight_table_9.Add(SP_FLAK, 1);
    weight_table_9.Add(FASTBOAT, 1);
    weight_table_9.Add(FIGHTER, 1);
    weight_table_9.Add(ALNPLANE, 1);

    weight_table_fastboat.Clear();
    weight_table_fastboat.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_fastboat.Add(ARTYTRRT, 1);
        weight_table_fastboat.Add(ANTIMSSL, 1);
        weight_table_fastboat.Add(SUBMARNE, 1);
    }

    weight_table_fastboat.Add(ARTILLRY, 1);
    weight_table_fastboat.Add(ROCKTLCH, 1);
    weight_table_fastboat.Add(MISSLLCH, 1);
    weight_table_fastboat.Add(CORVETTE, 1);
    weight_table_fastboat.Add(BATTLSHP, 1);
    weight_table_fastboat.Add(MSSLBOAT, 1);
    weight_table_fastboat.Add(JUGGRNT, 1);
    weight_table_fastboat.Add(ALNTANK, 1);
    weight_table_fastboat.Add(ALNASGUN, 1);

    weight_table_corvette.Clear();
    weight_table_corvette.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_corvette.Add(ARTYTRRT, 1);
        weight_table_corvette.Add(ANTIMSSL, 1);
    }

    weight_table_corvette.Add(ARTILLRY, 1);
    weight_table_corvette.Add(ROCKTLCH, 1);
    weight_table_corvette.Add(MISSLLCH, 1);
    weight_table_corvette.Add(CORVETTE, 1);
    weight_table_corvette.Add(BATTLSHP, 2);
    weight_table_corvette.Add(MSSLBOAT, 2);
    weight_table_corvette.Add(BOMBER, 3);
    weight_table_corvette.Add(JUGGRNT, 1);
    weight_table_corvette.Add(ALNASGUN, 1);
    weight_table_corvette.Add(ALNPLANE, 1);

    weight_table_submarine.Clear();
    weight_table_submarine.SetTeam(player_team);
    weight_table_submarine.Add(CORVETTE, 1);

    weight_table_battleships.Clear();
    weight_table_battleships.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_battleships.Add(SUBMARNE, 3);
        weight_table_battleships.Add(ANTIMSSL, 1);
    }

    weight_table_battleships.Add(ARTILLRY, 1);

    weight_table_battleships.Add(ROCKTLCH, 1);
    weight_table_battleships.Add(MISSLLCH, 1);
    weight_table_battleships.Add(CORVETTE, 1);
    weight_table_battleships.Add(BATTLSHP, 2);
    weight_table_battleships.Add(MSSLBOAT, 1);
    weight_table_battleships.Add(BOMBER, 3);
    weight_table_battleships.Add(JUGGRNT, 1);
    weight_table_battleships.Add(ALNASGUN, 1);
    weight_table_battleships.Add(ALNPLANE, 1);

    weight_table_support_ships.Clear();
    weight_table_support_ships.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        weight_table_support_ships.Add(GUNTURRT, 1);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_support_ships.Add(ARTYTRRT, 1);
        weight_table_support_ships.Add(ANTIMSSL, 1);
        weight_table_support_ships.Add(SUBMARNE, 1);
    }

    weight_table_support_ships.Add(SCOUT, 1);
    weight_table_support_ships.Add(TANK, 1);
    weight_table_support_ships.Add(ARTILLRY, 1);
    weight_table_support_ships.Add(ROCKTLCH, 1);
    weight_table_support_ships.Add(MISSLLCH, 1);
    weight_table_support_ships.Add(SP_FLAK, 1);
    weight_table_support_ships.Add(CORVETTE, 1);
    weight_table_support_ships.Add(BATTLSHP, 1);
    weight_table_support_ships.Add(MSSLBOAT, 1);
    weight_table_support_ships.Add(BOMBER, 1);
    weight_table_support_ships.Add(JUGGRNT, 1);
    weight_table_support_ships.Add(ALNTANK, 1);
    weight_table_support_ships.Add(ALNASGUN, 1);
    weight_table_support_ships.Add(ALNPLANE, 1);

    weight_table_missileboat.Clear();
    weight_table_missileboat.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_missileboat.Add(ANTIMSSL, 1);
    }

    weight_table_missileboat.Add(MISSLLCH, 1);
    weight_table_missileboat.Add(CORVETTE, 1);
    weight_table_missileboat.Add(BATTLSHP, 1);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_missileboat.Add(SUBMARNE, 3);
    }

    weight_table_missileboat.Add(MSSLBOAT, 1);
    weight_table_missileboat.Add(BOMBER, 3);
    weight_table_missileboat.Add(JUGGRNT, 1);
    weight_table_missileboat.Add(ALNPLANE, 1);

    weight_table_commando.Clear();
    weight_table_commando.SetTeam(player_team);
    weight_table_commando.Add(INFANTRY, 1);

    weight_table_infantry.Clear();
    weight_table_infantry.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        weight_table_infantry.Add(ANTIMSSL, 1);
    }

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        weight_table_infantry.Add(GUNTURRT, 1);
    }

    weight_table_infantry.Add(TANK, 1);

    weight_table_infantry.Add(ARTILLRY, 1);
    weight_table_infantry.Add(ROCKTLCH, 1);
    weight_table_infantry.Add(MISSLLCH, 1);
    weight_table_infantry.Add(SP_FLAK, 1);
    weight_table_infantry.Add(INFANTRY, 1);
    weight_table_infantry.Add(BATTLSHP, 1);
    weight_table_infantry.Add(MSSLBOAT, 1);
    weight_table_infantry.Add(BOMBER, 1);
    weight_table_infantry.Add(JUGGRNT, 1);
    weight_table_infantry.Add(ALNTANK, 1);
    weight_table_infantry.Add(ALNASGUN, 1);
    weight_table_infantry.Add(ALNPLANE, 1);

    weight_table_generic.Clear();
    weight_table_generic.SetTeam(player_team);

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        weight_table_generic.Add(COMMANDO, 1);
        weight_table_generic.Add(INFANTRY, 1);
    }

    weight_table_generic.Add(SCOUT, 1);
    weight_table_generic.Add(TANK, 1);
    weight_table_generic.Add(ARTILLRY, 1);
    weight_table_generic.Add(ROCKTLCH, 1);
    weight_table_generic.Add(MISSLLCH, 1);
    weight_table_generic.Add(BATTLSHP, 1);
    weight_table_generic.Add(MSSLBOAT, 1);
    weight_table_generic.Add(BOMBER, 1);
    weight_table_generic.Add(JUGGRNT, 1);
    weight_table_generic.Add(ALNTANK, 1);
    weight_table_generic.Add(ALNASGUN, 1);
    weight_table_generic.Add(ALNPLANE, 1);
}

int32_t AiPlayer::GetPredictedAttack(UnitInfo* unit, int32_t caution_level) {
    return GetTotalProjectedDamage(unit, caution_level, player_team, &UnitsManager_MobileLandSeaUnits) +
           GetTotalProjectedDamage(unit, caution_level, player_team, &UnitsManager_MobileAirUnits);
}

int16_t** AiPlayer::GetDamagePotentialMap(UnitInfo* unit, int32_t caution_level, bool is_for_attacking) {
    int16_t** result;
    ThreatMap* threat_map = GetThreatMap(ThreatMap::GetRiskLevel(unit), caution_level, is_for_attacking);

    if (threat_map) {
        threat_map->Update(unit->GetBaseValues()->GetAttribute(ATTRIB_ARMOR));

        result = threat_map->damage_potential_map;

    } else {
        result = nullptr;
    }

    return result;
}

int16_t** AiPlayer::GetDamagePotentialMap(ResourceID unit_type, int32_t caution_level, bool is_for_attacking) {
    int16_t** result;
    ThreatMap* threat_map = GetThreatMap(ThreatMap::GetRiskLevel(unit_type), caution_level, is_for_attacking);

    if (threat_map) {
        threat_map->Update(UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], unit_type)
                               ->GetAttribute(ATTRIB_ARMOR));

        result = threat_map->damage_potential_map;

    } else {
        result = nullptr;
    }

    return result;
}

int32_t AiPlayer::GetDamagePotential(UnitInfo* unit, Point site, int32_t caution_level, bool is_for_attacking) {
    int32_t result;

    if (caution_level == CAUTION_LEVEL_NONE) {
        result = 0;

    } else {
        ThreatMap* threat_map = GetThreatMap(ThreatMap::GetRiskLevel(unit), caution_level, is_for_attacking);

        if (threat_map) {
            result = threat_map->damage_potential_map[site.x][site.y] +
                     threat_map->shots_map[site.x][site.y] *
                         (threat_map->armor - unit->GetBaseValues()->GetAttribute(ATTRIB_ARMOR));

        } else {
            result = 0;
        }
    }

    return result;
}

void AiPlayer::ClearZone(Zone* zone) {
    if (zone->unit->flags & MOBILE_AIR_UNIT) {
        if (!task_clear_air_zone) {
            task_clear_air_zone = new (std::nothrow) TaskClearZone(player_team, MOBILE_AIR_UNIT);

            TaskManager.AppendTask(*task_clear_air_zone);
        }

        task_clear_air_zone->AddZone(zone);

    } else {
        if (!task_clear_ground_zone) {
            task_clear_ground_zone = new (std::nothrow) TaskClearZone(player_team, MOBILE_SEA_UNIT | MOBILE_LAND_UNIT);
            TaskManager.AppendTask(*task_clear_ground_zone);
        }

        task_clear_ground_zone->AddZone(zone);
    }
}

WeightTable AiPlayer::GetFilteredWeightTable(ResourceID unit_type, uint16_t flags) {
    WeightTable table(GetWeightTable(unit_type));

    if (flags & 0x01) {
        for (uint32_t i = 0; i < table.GetCount(); ++i) {
            if (table[i].unit_type != INVALID_ID &&
                (ResourceManager_GetUnit(table[i].unit_type).GetFlags() & STATIONARY)) {
                table[i].weight = 0;
            }
        }
    }

    if (flags & 0x02) {
        for (uint32_t i = 0; i < table.GetCount(); ++i) {
            if (table[i].unit_type != INVALID_ID &&
                (ResourceManager_GetUnit(table[i].unit_type).GetFlags() & REGENERATING_UNIT)) {
                table[i].weight = 0;
            }
        }
    }

    return table;
}

void AiPlayer::SetInfoMapPoint(Point site) {
    if (info_map) {
        info_map[site.x][site.y] |= INFO_MAP_EXPLORED;
    }
}

void AiPlayer::UpdateMineMap(Point site) {
    if (mine_map && mine_map[site.x][site.y] < 0) {
        mine_map[site.x][site.y] = 0;
    }
}

void AiPlayer::MarkMineMapPoint(Point site) {
    if (mine_map) {
        mine_map[site.x][site.y] = -1;
    }
}

int8_t AiPlayer::GetMineMapEntry(Point site) {
    int8_t result;

    if (mine_map) {
        result = mine_map[site.x][site.y];

    } else {
        result = 0;
    }

    return result;
}

void AiPlayer::FindMines(UnitInfo* unit) {
    if (mine_map) {
        Point position(unit->grid_x, unit->grid_y);
        uint16_t team = unit->team;
        int32_t attack =
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], LANDMINE)->GetAttribute(ATTRIB_ATTACK);

        mine_map[position.x][position.y] = (attack + mine_map[position.x][position.y]) / 2;

        if (!find_mines_task) {
            find_mines_task = new (std::nothrow) TaskFindMines(player_team, position);

            TaskManager.AppendTask(*find_mines_task);
        }
    }
}

WeightTable AiPlayer::GetExtendedWeightTable(UnitInfo* target, uint8_t flags) {
    WeightTable table = GetFilteredWeightTable(target->GetUnitType(), flags);
    TeamUnits* team_units = UnitsManager_TeamInfo[player_team].team_units;
    Point position(target->grid_x, target->grid_y);

    if (target->team == PLAYER_TEAM_ALIEN &&
        ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_EXPERT) {
        table.Clear();
        table.Add(COMMANDO, 1);

    } else {
        if (target->team == PLAYER_TEAM_ALIEN) {
            table = GetFilteredWeightTable(ADUMP, flags);
        }

        if (target->flags & STATIONARY) {
            bool is_water_present = false;
            Rect bounds;
            auto world = ResourceManager_GetActiveWorld();

            target->GetBounds(&bounds);

            for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
                for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                    if (world->GetSurfaceType(x, y) == SURFACE_TYPE_WATER) {
                        is_water_present = true;
                    }
                }
            }

            if (is_water_present) {
                table.Add(CORVETTE, 1);
            }

            if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_AVERAGE) {
                table.Add(SUBMARNE, 1);
            }
        }

        for (uint32_t i = 0; i < table.GetCount(); ++i) {
            if (table[i].unit_type != INVALID_ID) {
                int32_t surface_type = ResourceManager_GetUnit(table[i].unit_type).GetLandType();
                int32_t unit_range = team_units->GetCurrentUnitValues(table[i].unit_type)->GetAttribute(ATTRIB_RANGE);

                if (surface_type == SURFACE_TYPE_LAND &&
                    !IsSurfaceTypePresent(position, unit_range, SURFACE_TYPE_LAND)) {
                    table[i].weight = 0;
                }

                if (surface_type == SURFACE_TYPE_WATER &&
                    !IsSurfaceTypePresent(position, unit_range, SURFACE_TYPE_WATER)) {
                    table[i].weight = 0;
                }
            }
        }
    }

    return table;
}

bool AiPlayer::ShouldUpgradeUnit(UnitInfo* unit) {
    bool result;
    UnitValues* base_values = unit->GetBaseValues();
    UnitValues* current_values =
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], unit->GetUnitType());

    if (base_values != current_values) {
        if (build_order.unit_type != unit->GetUnitType()) {
            if (!(unit->flags & REGENERATING_UNIT)) {
                if (unit->GetUnitType() != CONSTRCT && unit->GetUnitType() != ENGINEER &&
                    unit->GetUnitType() != BULLDOZR && unit->GetUnitType() != MINELAYR) {
                    if (base_values->GetAttribute(ATTRIB_RANGE) < current_values->GetAttribute(ATTRIB_RANGE)) {
                        result = true;

                    } else {
                        if (base_values->GetAttribute(ATTRIB_HITS) * 13 <
                            current_values->GetAttribute(ATTRIB_HITS) * 10) {
                            result = true;

                        } else {
                            if (base_values->GetAttribute(ATTRIB_ARMOR) * 12 <
                                current_values->GetAttribute(ATTRIB_ARMOR) * 10) {
                                result = true;

                            } else {
                                if (base_values->GetAttribute(ATTRIB_ATTACK) * 13 <
                                    current_values->GetAttribute(ATTRIB_ATTACK) * 10) {
                                    result = true;

                                } else {
                                    if (base_values->GetAttribute(ATTRIB_SPEED) * 13 <
                                        current_values->GetAttribute(ATTRIB_SPEED) * 10) {
                                        result = true;

                                    } else {
                                        if (base_values->GetAttribute(ATTRIB_ROUNDS) <
                                            current_values->GetAttribute(ATTRIB_ROUNDS)) {
                                            result = true;

                                        } else {
                                            if (base_values->GetAttribute(ATTRIB_SCAN) * 12 <
                                                current_values->GetAttribute(ATTRIB_SCAN) * 10) {
                                                result = true;

                                            } else {
                                                result = false;
                                            }
                                        }
                                    }
                                }
                            }
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

void AiPlayer::RemoveUnit(UnitInfo* unit) {
    if (player_team == unit->team) {
        air_force.Remove(*unit);
        ground_forces.Remove(*unit);

    } else {
        for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
            if (unit == (*it).GetUnit()) {
                spotted_units.Remove(*it);
            }
        }
    }
}

void AiPlayer::UnitSpotted(UnitInfo* unit) {
    if (unit->team != player_team &&
        (!(unit->flags & GROUND_COVER) || unit->GetUnitType() == LANDMINE || unit->GetUnitType() == SEAMINE)) {
        SmartList<SpottedUnit>::Iterator spotted_unit;

        InvalidateThreatMaps();

        for (spotted_unit = spotted_units.Begin(); spotted_unit != spotted_units.End(); ++spotted_unit) {
            if ((*spotted_unit).GetUnit() == unit) {
                break;
            }
        }

        if (spotted_unit != spotted_units.End()) {
            (*spotted_unit).UpdatePosition();

        } else {
            SmartPointer<SpottedUnit> spotted_unit2(new (std::nothrow) SpottedUnit(unit, player_team));

            spotted_units.PushBack(*spotted_unit2);

            if (unit->GetUnitType() == LANDMINE || unit->GetUnitType() == SEAMINE) {
                MineSpotted(unit);
            }

            bool is_key_facility = false;

            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                if (IsKeyFacility((*it).GetUnit()->GetUnitType())) {
                    is_key_facility = true;
                    break;
                }
            }

            if ((IsKeyFacility(unit->GetUnitType()) || (!is_key_facility && unit->GetUnitType() == CONSTRCT)) &&
                IsTargetTeam(unit->team)) {
                DetermineAttack(&*spotted_unit2, TASK_PRIORITY_ATTACK_DEFAULT);

            } else if (!is_key_facility && unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) > 0) {
                if (IsTargetTeam(unit->team)) {
                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                         it != UnitsManager_StationaryUnits.End(); ++it) {
                        if ((*it).team == unit->team && IsKeyFacility((*it).GetUnitType()) &&
                            Access_GetApproximateDistance(unit, &*it) < 30) {
                            spotted_unit2 = new (std::nothrow) SpottedUnit(&*it, player_team);

                            spotted_unit2->SetPosition(Point(unit->grid_x, unit->grid_y));

                            spotted_units.PushBack(*spotted_unit2);

                            DetermineAttack(&*spotted_unit2, TASK_PRIORITY_ATTACK_DEFAULT);

                            return;
                        }
                    }

                    for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                        if ((*it).GetUnit()->GetUnitType() == CONSTRCT) {
                            return;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                        if ((*it).team == unit->team && (*it).GetUnitType() == CONSTRCT &&
                            Access_GetApproximateDistance(unit, &*it) < 30) {
                            spotted_unit2 = new (std::nothrow) SpottedUnit(&*it, player_team);

                            spotted_unit2->SetPosition(Point(unit->grid_x, unit->grid_y));

                            spotted_units.PushBack(*spotted_unit2);

                            DetermineAttack(&*spotted_unit2, TASK_PRIORITY_ATTACK_DEFAULT);

                            return;
                        }
                    }
                }
            }
        }
    }
}

bool AiPlayer::MatchPath(TaskPathRequest* request) {
    UnitInfo* unit = request->GetClient();
    Point position(unit->grid_x, unit->grid_y);
    int32_t minimum_distance = request->GetMinimumDistance();
    Point site = request->GetDestination();
    int32_t distance = Access_GetApproximateDistance(position, site) / 2;
    bool result;

    distance -= minimum_distance;

    if (distance >= unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) {
        int32_t transport_category = TransportOrder::DetermineCategory(unit->GetUnitType());
        bool flag = request->GetField31();
        TransportOrder* transport_order = nullptr;
        Point source;
        Point destination;
        int32_t minimum_distance2;

        for (SmartList<TransportOrder>::Iterator it = transport_orders.Begin(); it != transport_orders.End(); ++it) {
            if ((*it).GetTransportCategory() == transport_category) {
                int32_t distance1;
                int32_t distance2;

                source = (*it).MatchStartPosition(position);
                distance1 = Access_GetApproximateDistance(position, source) / 2;
                destination = (*it).MatchClosestPosition(site, source);
                distance2 = Access_GetApproximateDistance(site, destination) / 2;
                distance2 -= minimum_distance;

                if (distance2 < 0) {
                    distance2 = 0;
                }

                if (distance2 < distance / 3 && distance1 < distance / 3 &&
                    (flag || (*it).GetUnitType() == INVALID_ID)) {
                    if (!transport_order || distance1 + distance2 < minimum_distance2) {
                        transport_order = &*it;
                        minimum_distance2 = distance1 + distance2;
                    }
                }
            }
        }

        if (transport_order) {
            transport_order->UsePrecalculatedPath(request);

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

bool AiPlayer::SelectStrategy() {
    SmartArray<Continent> continents;
    const World* world = ResourceManager_GetActiveWorld();
    AccessMap access_map(world);
    Point site;
    int32_t continent_size = 0;
    uint16_t strategy_scores[AI_STRATEGY_MAX];

    for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
        for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
            switch (Access_GetSurfaceType(site.x, site.y)) {
                case SURFACE_TYPE_LAND: {
                    access_map(site.x, site.y) = 0x02;
                } break;

                case SURFACE_TYPE_WATER:
                case SURFACE_TYPE_COAST: {
                    access_map(site.x, site.y) = 0x01;
                } break;

                default: {
                    access_map(site.x, site.y) = 0x00;
                } break;
            }
        }
    }

    for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
        for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
            if (access_map(site.x, site.y) == 0x02) {
                SmartPointer<Continent> continent(new (std::nothrow)
                                                      Continent(access_map.GetMap(), continents.GetCount() + 3, site));

                if (continent->IsViableContinent(false, player_team)) {
                    continents.Insert(&*continent);
                }
            }
        }
    }

    int32_t opponent_class = ResourceManager_GetSettings()->GetNumericValue("opponent");

    for (uint32_t i = 0; i < AI_STRATEGY_MAX; ++i) {
        strategy_scores[i] = 0;
    }

    if (opponent_class >= OPPONENT_TYPE_AVERAGE) {
        int32_t continent_score = 0;
        continent_size = 0;

        for (uint32_t i = 0; i < continents.GetCount(); ++i) {
            if (continents[i].GetContinentSize() > continent_size) {
                continent_size = continents[i].GetContinentSize();
            }
        }

        for (uint32_t i = 0; i < continents.GetCount(); ++i) {
            if (continent_size <= 80 || continents[i].GetContinentSize() >= 80) {
                if (continents[i].IsViableContinent(true, player_team)) {
                    ++continent_score;

                } else {
                    continent_score = 100;
                }
            }
        }

        if (continent_score == 1) {
            switch (opponent_class) {
                case OPPONENT_TYPE_AVERAGE: {
                    strategy_scores[AI_STRATEGY_TANK_HORDE] = 1;
                } break;

                case OPPONENT_TYPE_EXPERT: {
                    strategy_scores[AI_STRATEGY_TANK_HORDE] = 6;
                } break;

                case OPPONENT_TYPE_MASTER: {
                    strategy_scores[AI_STRATEGY_TANK_HORDE] = 12;
                } break;

                case OPPONENT_TYPE_GOD: {
                    strategy_scores[AI_STRATEGY_TANK_HORDE] = 24;
                } break;
            }
        }
    }

    continent_size = 0;

    for (uint32_t i = 0; i < continents.GetCount(); ++i) {
        continents[i].TestIsolated();
    }

    for (int64_t i = static_cast<int64_t>(continents.GetCount()) - 1; i >= 0; --i) {
        if (continents[i].IsViableContinent(true, player_team)) {
            if (continents[i].GetContinentSize() > continent_size) {
                continent_size = continents[i].GetContinentSize();
            }

        } else {
            continents.Erase(i);
        }
    }

    if (continent_size > 80) {
        for (int64_t i = static_cast<int64_t>(continents.GetCount()) - 1; i >= 0; --i) {
            if (continents[i].GetContinentSize() < 80) {
                continents.Erase(i);
            }
        }
    }

    int32_t isolated_continents = 0;
    int32_t continents_in_close_proximity = 0;

    for (uint32_t i = 0; i < continents.GetCount(); ++i) {
        if (continents[i].IsIsolated()) {
            ++isolated_continents;
        }

        if (continents[i].IsCloseProximity()) {
            ++continents_in_close_proximity;
        }
    }

    strategy_scores[AI_STRATEGY_MISSILES] = continents.GetCount();

    if (opponent_class >= OPPONENT_TYPE_APPRENTICE) {
        strategy_scores[AI_STRATEGY_DEFENSIVE] = continents.GetCount();
    }

    if (opponent_class >= OPPONENT_TYPE_AVERAGE && strategy_scores[AI_STRATEGY_TANK_HORDE] == 0) {
        strategy_scores[AI_STRATEGY_DEFENSIVE] = continents.GetCount();

        if (isolated_continents > 0) {
            strategy_scores[AI_STRATEGY_SCOUT_HORDE] = 1;

        } else {
            strategy_scores[AI_STRATEGY_SCOUT_HORDE] = continents.GetCount();
        }

        if (opponent_class >= OPPONENT_TYPE_MASTER) {
            strategy_scores[AI_STRATEGY_SCOUT_HORDE] *= 2;
        }

        if (opponent_class >= OPPONENT_TYPE_GOD) {
            strategy_scores[AI_STRATEGY_SCOUT_HORDE] *= 2;
        }
    }

    strategy_scores[AI_STRATEGY_AIR] = continents_in_close_proximity * 2;

    if (opponent_class >= OPPONENT_TYPE_EXPERT) {
        strategy_scores[AI_STRATEGY_TANK_HORDE] = continents.GetCount();
    }

    strategy_scores[AI_STRATEGY_SEA] = isolated_continents * 2;

    strategy_scores[AI_STRATEGY_FAST_ATTACK] = continents.GetCount() - continents_in_close_proximity;

    strategy_scores[AI_STRATEGY_COMBINED_ARMS] = (continents.GetCount() + continents_in_close_proximity) * 2;

    strategy = AI_STRATEGY_RANDOM;

    {
        char strategy_string[80];
        const char* strategy_strings[AI_STRATEGY_MAX] = {"random",        "defensive",   "missiles",   "air",
                                                         "sea",           "scout horde", "tank horde", "fast attack",
                                                         "combined arms", "espionage"};

        strncpy(strategy_string,
                ResourceManager_GetSettings()->GetStringValue(menu_team_strategy_setting[player_team]).c_str(),
                sizeof(strategy_string) - 1);
        strategy_string[sizeof(strategy_string) - 1] = '\0';
        if (strategy_string[0] != '\0') {
            for (int32_t i = 0; i < AI_STRATEGY_MAX; ++i) {
                if (!SDL_strcasecmp(strategy_string, strategy_strings[i])) {
                    if (strategy_scores[i] == 0) {
                        strategy = AI_STRATEGY_RANDOM;

                    } else {
                        strategy = i;
                    }

                    break;
                }
            }
        }
    }

    if (strategy == AI_STRATEGY_RANDOM) {
        int32_t total_score = 0;
        int32_t potential_strategies = 0;

        for (int32_t i = 0; i < AI_STRATEGY_MAX; ++i) {
            if (strategy_scores[i] > 0) {
                ++potential_strategies;
            }
        }

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER && team != player_team &&
                AiPlayer_Teams[team].GetStrategy() != AI_STRATEGY_RANDOM) {
                --potential_strategies;
            }
        }

        if (potential_strategies > 0) {
            for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER && team != player_team &&
                    AiPlayer_Teams[team].GetStrategy() != AI_STRATEGY_RANDOM) {
                    strategy_scores[AiPlayer_Teams[team].GetStrategy()] = 0;
                }
            }
        }

        for (int32_t i = 0; i < AI_STRATEGY_MAX; ++i) {
            total_score += strategy_scores[i];

            if (strategy_scores[i] > 0) {
                ++potential_strategies;
            }
        }

        total_score = Randomizer_Generate(total_score) + 1;

        int32_t index = -1;

        do {
            ++index;
            total_score -= strategy_scores[index];
        } while (total_score > 0);

        strategy = index;
    }

    switch (strategy) {
        case AI_STRATEGY_SEA: {
            for (int64_t i = static_cast<int64_t>(continents.GetCount()) - 1; i >= 0; --i) {
                if (!continents[i].IsIsolated()) {
                    continents.Erase(i);
                }
            }
        } break;

        case AI_STRATEGY_FAST_ATTACK:
        case AI_STRATEGY_COMBINED_ARMS: {
            for (int64_t i = static_cast<int64_t>(continents.GetCount()) - 1; i >= 0; --i) {
                if (continents[i].IsCloseProximity()) {
                    continents.Erase(i);
                }
            }
        } break;

        case AI_STRATEGY_AIR: {
            for (int64_t i = static_cast<int64_t>(continents.GetCount()) - 1; i >= 0; --i) {
                if (!continents[i].IsCloseProximity()) {
                    continents.Erase(i);
                }
            }
        } break;
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (continents.GetCount() > 1 && team != player_team &&
            UnitsManager_TeamMissionSupplies[team].units.GetCount() > 0 &&
            UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            uint8_t filler = access_map(UnitsManager_TeamMissionSupplies[team].starting_position.x,
                                        UnitsManager_TeamMissionSupplies[team].starting_position.y);

            for (int64_t i = static_cast<int64_t>(continents.GetCount()) - 1; i >= 0 && continents.GetCount() > 1;
                 --i) {
                if (continents[i].GetFiller() == filler) {
                    continents.Erase(i);
                }
            }
        }
    }

    Point map_center(ResourceManager_MapSize.x / 2, ResourceManager_MapSize.y / 2);

    if (strategy == AI_STRATEGY_DEFENSIVE || strategy == AI_STRATEGY_AIR || strategy == AI_STRATEGY_SEA ||
        strategy == AI_STRATEGY_ESPIONAGE) {
        int32_t maximum_distance = 0;
        int32_t distance;

        for (uint32_t i = 0; i < continents.GetCount(); ++i) {
            Point continent_center = continents[i].GetCenter();

            distance = Access_GetApproximateDistance(map_center, continent_center);

            if (distance > maximum_distance) {
                maximum_distance = distance;
            }
        }

        for (int64_t i = static_cast<int64_t>(continents.GetCount()) - 1; i >= 0; --i) {
            Point continent_center = continents[i].GetCenter();

            distance = Access_GetApproximateDistance(map_center, continent_center);

            if (distance < maximum_distance / 2) {
                continents.Erase(i);
            }
        }
    }

    if (strategy == AI_STRATEGY_SCOUT_HORDE) {
        int32_t minimum_distance = ResourceManager_MapSize.x + ResourceManager_MapSize.y;
        int32_t distance;

        for (uint32_t i = 0; i < continents.GetCount(); ++i) {
            Point continent_center = continents[i].GetCenter();

            distance = Access_GetApproximateDistance(map_center, continent_center);

            if (distance < minimum_distance) {
                minimum_distance = distance;
            }
        }

        for (int64_t i = static_cast<int64_t>(continents.GetCount()) - 1; i >= 0; --i) {
            Point continent_center = continents[i].GetCenter();

            distance = Access_GetApproximateDistance(map_center, continent_center);

            if (distance > minimum_distance * 2) {
                continents.Erase(i);
            }
        }
    }

    int32_t total_continent_size = 0;
    bool result;

    for (uint32_t i = 0; i < continents.GetCount(); ++i) {
        total_continent_size += continents[i].GetContinentSize();
    }

    if (total_continent_size) {
        total_continent_size = Randomizer_Generate(total_continent_size) + 1;

        int32_t index = -1;

        do {
            ++index;

            total_continent_size -= continents[index].GetContinentSize();
        } while (total_continent_size > 0);

        continents[index].SelectLandingSite(player_team, strategy);

        continents.Release();

        RollTeamMissionSupplies(SelectTeamClan());
        RollField3();
        RollField5();
        RollField7();

        result = true;

    } else {
        UnitsManager_TeamInfo[player_team].team_type = TEAM_TYPE_NONE;

        result = false;
    }

    return result;
}

void AiPlayer::FileSave(SmartFileWriter& file) {
    uint32_t item_count;

    file.Write(player_team);
    file.Write(strategy);
    file.Write(field_3);
    file.Write(field_5);
    file.Write(field_7);
    file.Write(target_team);

    file.WriteObjectCount(spotted_units.GetCount());

    for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
        (*it).FileSave(file);
    }

    item_count = info_map ? 1 : 0;

    file.WriteObjectCount(item_count);

    if (item_count) {
        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            file.Write(info_map[x], ResourceManager_MapSize.y);
        }
    }

    item_count = mine_map ? 1 : 0;

    file.WriteObjectCount(item_count);

    if (item_count) {
        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            file.Write(mine_map[x], ResourceManager_MapSize.y);
        }
    }

    file.Write(target_location);
}

void AiPlayer::FileLoad(SmartFileReader& file) {
    uint32_t item_count;

    file.Read(player_team);
    file.Read(strategy);
    file.Read(field_3);
    file.Read(field_5);
    file.Read(field_7);
    file.Read(target_team);

    item_count = file.ReadObjectCount();

    spotted_units.Clear();
    air_force.Clear();
    ground_forces.Clear();

    for (uint32_t i = 0; i < item_count; ++i) {
        SmartPointer<SpottedUnit> spotted_unit(new (std::nothrow) SpottedUnit(file));

        spotted_units.PushBack(*spotted_unit);
    }

    SDL_assert(info_map == nullptr);

    item_count = file.ReadObjectCount();

    if (item_count) {
        info_map = new (std::nothrow) uint8_t*[ResourceManager_MapSize.x];

        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            info_map[x] = new (std::nothrow) uint8_t[ResourceManager_MapSize.y];

            file.Read(info_map[x], ResourceManager_MapSize.y);

            for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                info_map[x][y] &= (INFO_MAP_EXPLORED | INFO_MAP_MINE_FIELD);
            }
        }
    }

    SDL_assert(mine_map == nullptr);

    item_count = file.ReadObjectCount();

    if (item_count) {
        mine_map = new (std::nothrow) int8_t*[ResourceManager_MapSize.x];

        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            mine_map[x] = new (std::nothrow) int8_t[ResourceManager_MapSize.y];

            file.Read(mine_map[x], ResourceManager_MapSize.y);
        }
    }

    file.Read(target_location);
}

void AiPlayer::ChooseUpgrade(SmartObjectArray<BuildOrder> build_orders1, SmartObjectArray<BuildOrder> build_orders2) {
    upgrade_cost = INT16_MAX;
    build_order.unit_type = INVALID_ID;

    for (uint32_t i = 0; i < build_orders1.GetCount(); ++i) {
        if (build_orders1[i]->primary_attribute == RESEARCH_TOPIC_HITS ||
            build_orders1[i]->primary_attribute == RESEARCH_TOPIC_SPEED) {
            SmartPointer<UnitValues> unit_values =
                UnitsManager_TeamInfo[player_team].team_units->GetCurrentUnitValues(build_orders1[i]->unit_type);

            auto current_level = unit_values->GetAttribute(build_orders1[i]->primary_attribute);

            if (current_level == UINT8_MAX) {
                continue;
            }
        }

        int32_t new_upgrade_cost =
            TeamUnits_GetUpgradeCost(player_team, build_orders1[i]->unit_type, build_orders1[i]->primary_attribute);

        if (new_upgrade_cost < upgrade_cost) {
            build_order = *build_orders1[i];
            upgrade_cost = new_upgrade_cost;
        }
    }

    int32_t factor = 2;

    for (uint32_t i = 0; i < build_orders2.GetCount(); ++i) {
        if (build_orders2[i]->primary_attribute == RESEARCH_TOPIC_HITS ||
            build_orders2[i]->primary_attribute == RESEARCH_TOPIC_SPEED) {
            SmartPointer<UnitValues> unit_values =
                UnitsManager_TeamInfo[player_team].team_units->GetCurrentUnitValues(build_orders2[i]->unit_type);

            auto current_level = unit_values->GetAttribute(build_orders2[i]->primary_attribute);

            if (current_level == UINT8_MAX) {
                continue;
            }
        }

        int32_t new_upgrade_cost =
            TeamUnits_GetUpgradeCost(player_team, build_orders2[i]->unit_type, build_orders2[i]->primary_attribute);

        if (new_upgrade_cost * factor < upgrade_cost) {
            build_order = *build_orders2[i];
            upgrade_cost = new_upgrade_cost;

            factor = 1;
        }
    }
}

void AiPlayer_UpgradeStationaryUnit(UnitInfo* unit) {
    Complex* complex = unit->GetComplex();

    if (complex) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        if (materials.raw >= ((capacity.raw * 3) / 4)) {
            for (auto it = UnitsManager_StationaryUnits.Begin(), it_end = UnitsManager_StationaryUnits.End();
                 it != it_end; ++it) {
                if ((*it).GetComplex() == complex && (*it).GetUnitType() == ADUMP) {
                    if (Task_IsReadyToTakeOrders(it->Get())) {
                        (*it).SetParent(unit);
                        UnitsManager_SetNewOrder(it->Get(), ORDER_UPGRADE, ORDER_STATE_INIT);
                        break;
                    }
                }
            }
        }
    }
}

int32_t AiPlayer_CalculateProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int32_t caution_level) {
    UnitValues* unit_values = friendly_unit->GetBaseValues();
    int32_t range = unit_values->GetAttribute(ATTRIB_RANGE);
    int32_t distance;
    int32_t shots;
    int32_t result;

    if (!enemy_unit->IsVisibleToTeam(friendly_unit->team)) {
        range = std::min(range, unit_values->GetAttribute(ATTRIB_SCAN));
    }

    distance = Access_GetSquaredDistance(friendly_unit, enemy_unit);
    shots = 0;

    if (AiPlayer_TerrainDistanceField->GetMinimumRange(
            Point(enemy_unit->grid_x, enemy_unit->grid_y),
            ResourceManager_GetUnit(friendly_unit->GetUnitType()).GetLandType()) <= range * range) {
        if (unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE) &&
            ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_AVERAGE) {
            int32_t distance2 = ((friendly_unit->speed / 2) + range);

            if (distance <= distance2 * distance2) {
                shots = friendly_unit->shots;
            }

        } else {
            int32_t base_rounds = unit_values->GetAttribute(ATTRIB_ROUNDS);
            int32_t base_speed = unit_values->GetAttribute(ATTRIB_SPEED);

            for (shots = friendly_unit->shots; shots > 0; --shots) {
                int32_t distance2 = friendly_unit->speed - ((base_speed * shots) / base_rounds) + range;

                if (distance <= distance2 * distance2) {
                    break;
                }
            }
        }

        if (caution_level > CAUTION_LEVEL_AVOID_REACTION_FIRE) {
            range += friendly_unit->speed;
        }

        if (caution_level > CAUTION_LEVEL_AVOID_REACTION_FIRE && distance <= range * range) {
            shots = std::min(unit_values->GetAttribute(ATTRIB_ROUNDS), friendly_unit->ammo - shots);
        }

        if (shots > 0) {
            result = shots * UnitsManager_GetAttackDamage(friendly_unit, enemy_unit, 0);

        } else {
            result = 0;
        }

    } else {
        result = 0;
    }

    return result;
}

int32_t AiPlayer_GetProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int32_t caution_level) {
    int32_t result;

    if (friendly_unit->shots > 0 && Task_IsReadyToTakeOrders(friendly_unit) &&
        Access_IsValidAttackTarget(friendly_unit, enemy_unit)) {
        if (caution_level != CAUTION_LEVEL_AVOID_REACTION_FIRE ||
            friendly_unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
            if (!friendly_unit->GetTask() ||
                friendly_unit->GetTask()->ComparePriority(TASK_PRIORITY_FRONTAL_ATTACK) > 0) {
                result = AiPlayer_CalculateProjectedDamage(friendly_unit, enemy_unit, caution_level);

            } else {
                result = 0;
            }

        } else {
            result = 0;
        }

    } else {
        result = 0;
    }

    return result;
}
