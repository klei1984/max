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
#include "continent.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "message_manager.hpp"
#include "remote.hpp"
#include "researchmenu.hpp"
#include "resource_manager.hpp"
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
#include "units_manager.hpp"
#include "zonewalker.hpp"

#define AIPLAYER_THREAT_MAP_CACHE_ENTRIES 10

AiPlayer AiPlayer_Teams[PLAYER_TEAM_MAX - 1];
TerrainMap AiPlayer_TerrainMap;
ThreatMap AiPlayer_ThreatMaps[AIPLAYER_THREAT_MAP_CACHE_ENTRIES];

void AiPlayer::AddBuilding(UnitInfo* unit) { FindManager(Point(unit->grid_x, unit->grid_y))->AddUnit(*unit); }

void AiPlayer::RebuildWeightTable(WeightTable table, ResourceID unit_type, int factor) {
    for (int i = 0; i < table.GetCount(); ++i) {
        if (table[i].unit_type == unit_type) {
            table[i].weight *= factor;
        }
    }
}

void AiPlayer::RebuildWeightTables(ResourceID unit_type, int factor) {
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
            UnitWeight weight1(SCOUT, 1);
            weight_table_tanks.PushBack(weight1);

            UnitWeight weight2(SCOUT, 2);
            weight_table_assault_guns.PushBack(weight2);

            UnitWeight weight3(SCOUT, 1);
            weight_table_ground_defense.PushBack(weight3);

            UnitWeight weight4(SCOUT, 1);
            weight_table_missile_launcher.PushBack(weight4);

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

Point AiPlayer::GetUnitClusterCoordinates(unsigned short team, SmartList<UnitInfo>* units) {
    int team_unit_count = 0;
    int grid_x = 0;
    int grid_y = 0;
    Point result(0, 0);

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it; ++it) {
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

Point AiPlayer::GetTeamClusterCoordinates(unsigned short team) {
    Point site = GetUnitClusterCoordinates(team, &UnitsManager_StationaryUnits);

    if (site.x == 0 && site.y == 0) {
        site = GetUnitClusterCoordinates(team, &UnitsManager_MobileLandSeaUnits);
    }

    return site;
}

void AiPlayer::DetermineAttack(SpottedUnit* spotted_unit, unsigned short task_flags) {
    if (!spotted_unit->GetTask()) {
        if (task_flags > 0x1000 &&
            UnitsManager_TeamInfo[spotted_unit->GetUnit()->team].team_points >
                UnitsManager_TeamInfo[player_team].team_points &&
            (spotted_unit->GetUnit()->unit_type == GREENHSE || spotted_unit->GetUnit()->unit_type == MININGST)) {
            task_flags = 0x1700;
        }

        unsigned short target_task_flags =
            AiAttack_GetTargetFlags(nullptr, spotted_unit->GetUnit(), player_team) + task_flags + 0xFA;
        int task_index;

        for (task_index = 0; task_index < 3 && task_array[task_index]; ++task_index) {
        }

        if (task_index == 3) {
            for (--task_index; task_index >= 0 && task_array[task_index]->DeterminePriority(target_task_flags) <= 0;
                 --task_index) {
            }
        }

        if (task_index >= 0) {
            if (task_array[task_index]) {
                task_array[task_index]->RemoveSelf();
            }

            task_array[task_index] = new (std::nothrow) TaskAttack(spotted_unit, task_flags);

            TaskManager.AppendTask(*task_array[task_index]);
        }
    }
}

void AiPlayer::UpdatePriorityTasks() {
    for (int i = 0; i < 3; ++i) {
        task_array[i] = nullptr;
    }

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it; ++it) {
        if ((*it).GetTeam() == player_team && (*it).GetType() == TaskType_TaskAttack) {
            unsigned short task_flags = (*it).GetFlags();
            int task_index;

            for (task_index = 0;
                 task_index < 3 && task_array[task_index] && task_array[task_index]->DeterminePriority(task_flags) <= 0;
                 ++task_index) {
            }

            if (task_index == 3) {
                (*it).RemoveSelf();

            } else {
                if (task_array[2]) {
                    task_array[2]->RemoveSelf();
                }

                for (int i = 2; i > task_index; --i) {
                    task_array[i] = task_array[i - 1];
                }

                task_array[task_index] = (*it);
            }
        }
    }
}

void AiPlayer::DetermineTargetLocation(Point position) {
    if (spotted_units.GetCount() > 0) {
        SpottedUnit* target = nullptr;
        int distance;
        int minimum_distance;

        for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it; ++it) {
            UnitInfo* unit = (*it).GetUnit();

            if (!target || unit->unit_type == MININGST || target->GetUnit()->unit_type != MININGST) {
                distance = TaskManager_GetDistance(position, (*it).GetLastPosition());

                if (!target || (unit->unit_type == MININGST && target->GetUnit()->unit_type != MININGST) ||
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
    int best_research_topic = RESEARCH_TOPIC_ATTACK;
    int best_turns_to_complete =
        UnitsManager_TeamInfo[player_team].research_topics[best_research_topic].turns_to_complete;
    int turns_to_complete;

    for (int research_topic = RESEARCH_TOPIC_SHOTS; research_topic < RESEARCH_TOPIC_COST; ++research_topic) {
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
        if ((*it).team == player_team && (*it).unit_type == RESEARCH && (*it).orders == ORDER_POWER_ON &&
            (*it).state != ORDER_STATE_0 && (*it).research_topic != best_research_topic) {
            ResearchMenu_UpdateResearchProgress(player_team, (*it).research_topic, -1);
            (*it).research_topic = best_research_topic;
            ResearchMenu_UpdateResearchProgress(player_team, (*it).research_topic, 1);
        }
    }

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_09(player_team);
    }
}

bool AiPlayer::IsPotentialSpotter(unsigned short team, UnitInfo* unit) {
    int distance = unit->GetBaseValues()->GetAttribute(ATTRIB_SCAN);

    distance = distance * distance;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && Access_GetDistance(unit, &*it) <= distance) {
            return true;
        }
    }

    return false;
}

void AiPlayer::UpdateAccessMap(Point point1, Point point2, unsigned char** access_map) {
    Point site(point1);
    Point distance;
    int surface_type = Access_GetSurfaceType(point1.x, point1.y);
    int step;

    distance.x = labs(point2.x - point1.x);
    distance.y = labs(point2.y - point1.y);

    if (distance.x > distance.y) {
        step = point2.y < point1.y ? -1 : 1;

        if (point2.x < point1.x) {
            site = point2;
            step = -step;
        }

        for (int x = 0, y = distance.y / 2; x < distance.x; ++x) {
            ++site.x;
            y += distance.y;

            if (y >= distance.x) {
                site.y += step;
                y -= distance.x;
            }

            if (!access_map[site.x][site.y]) {
                surface_type = Access_GetSurfaceType(site.x, site.y);
            }

            if (surface_type == Access_GetSurfaceType(site.x, site.y)) {
                info_map[site.x][site.y] |= 0x04;
            }
        }

    } else {
        step = point2.x < point1.x ? -1 : 1;

        if (point2.x < point1.x) {
            site = point2;
            step = -step;
        }

        for (int y = 0, x = distance.x / 2; y < distance.y; ++y) {
            ++site.y;
            x += distance.x;

            if (x >= distance.y) {
                site.x += step;
                x -= distance.y;
            }

            if (!access_map[site.x][site.y]) {
                surface_type = Access_GetSurfaceType(site.x, site.y);
            }

            if (surface_type == Access_GetSurfaceType(site.x, site.y)) {
                info_map[site.x][site.y] |= 0x04;
            }
        }
    }
}

bool AiPlayer::CheckAttacks() {
    if (TaskManager_word_1731C0 != 2) {
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
            TaskManager.AppendUnit(*it);
        }
    }
}

bool AiPlayer::AreActionsPending() { return TaskManager_word_1731C0 == 2 || TaskManager.GetRemindersCount() > 0; }

bool AiPlayer::IsDemoMode() {
    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
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
            if ((*it).ammo > 0 || (*it).unit_type == LIGHTPLT || (*it).unit_type == LANDPLT ||
                (*it).unit_type == SHIPYARD || (*it).unit_type == AIRPLT || (*it).unit_type == TRAINHAL) {
                TaskManager.RemindAvailable(&*it);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == player_team && !(*it).GetTask()) {
            TaskManager.RemindAvailable(&*it);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).team == player_team && !(*it).GetTask()) {
            TaskManager.RemindAvailable(&*it);
        }
    }
}

int AiPlayer::GetTotalProjectedDamage(UnitInfo* unit, int caution_level, unsigned short team,
                                      SmartList<UnitInfo>* units) {
    int result = 0;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team) {
            result += AiPlayer_GetProjectedDamage(&*it, unit, caution_level);
        }
    }

    return result;
}

void AiPlayer::UpdateMap(short** map, Point position, int range, int damage_potential, bool normalize) {
    Point site;
    Point limit;
    int distance = range * range;
    int map_offset;
    short* map_address;

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

void AiPlayer::UpdateThreatMaps(ThreatMap* threat_map, UnitInfo* unit, Point position, int range, int attack, int shots,
                                int& ammo, bool normalize) {
    if (shots > ammo) {
        shots = ammo;
    }

    ammo -= shots;

    int damage_potential = attack * shots;

    if (damage_potential > 0) {
        if (unit->unit_type == SUBMARNE || unit->unit_type == CORVETTE) {
            ZoneWalker walker(position, range);

            do {
                if (ResourceManager_MapSurfaceMap[walker.GetGridY() * ResourceManager_MapSize.x + walker.GetGridX()] &
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

void AiPlayer::InvalidateThreatMap(UnitInfo* unit) {
    for (auto& map : AiPlayer_ThreatMaps) {
        if (map.team == unit->team && map.risk_level == ThreatMap::GetRiskLevel(unit)) {
            map.SetRiskLevel(0);
        }
    }
}

void AiPlayer::InvalidateThreatMaps() {
    for (auto& map : AiPlayer_ThreatMaps) {
        if (map.team == player_team) {
            map.SetRiskLevel(0);
        }
    }
}

void AiPlayer::DetermineDefenses(SmartList<UnitInfo>* units, short** map) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).shots > 0 && (*it).orders != ORDER_IDLE && (*it).orders != ORDER_DISABLE) {
            int unit_range = (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE);

            if (!(*it).GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                unit_range -= 3;
            }

            if (unit_range > 0) {
                int attack_power = (-(*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK)) * (*it).shots;

                UpdateMap(map, Point((*it).grid_x, (*it).grid_y), unit_range, attack_power, false);
            }
        }
    }
}

void AiPlayer::DetermineThreats(UnitInfo* unit, Point position, int caution_level, bool* teams,
                                ThreatMap* air_force_map, ThreatMap* ground_forces_map) {
    if ((unit->flags & MOBILE_AIR_UNIT) && air_force_map->shots_map) {
        ground_forces_map = air_force_map;
    }

    if (unit->speed <= 0 || teams[unit->team]) {
        UnitValues* base_values = unit->GetBaseValues();
        int unit_range = base_values->GetAttribute(ATTRIB_RANGE);
        int attack_range = unit_range + base_values->GetAttribute(ATTRIB_ATTACK_RADIUS);
        int unit_attack = base_values->GetAttribute(ATTRIB_ATTACK);
        int unit_shots = base_values->GetAttribute(ATTRIB_ROUNDS);
        int unit_ammo = unit->ammo;
        int unit_speed = base_values->GetAttribute(ATTRIB_SPEED);

        switch (caution_level) {
            case CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE: {
                if (base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                    UpdateThreatMaps(ground_forces_map, unit, position, unit_range, unit_attack, unit->shots, unit_ammo,
                                     true);

                    int movement_range = 0;

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

                            int movement_range = ((unit_speed + 1) * unit_shots) / (unit_shots + 1);

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
                    int movement_range = 0;

                    if (base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                        movement_range = unit_speed / 2;

                    } else {
                        movement_range = ((unit_speed + 1) * (unit_shots - 1)) / (unit_shots);

                        if (GameManager_PlayMode != PLAY_MODE_TURN_BASED) {
                            movement_range = std::max<int>(movement_range, unit->speed);
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

void AiPlayer::SumUpMaps(short** map1, short** map2) {
    for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
            map1[x][y] += map2[x][y];
        }
    }
}

void AiPlayer::NormalizeThreatMap(ThreatMap* threat_map) {
    for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
            if (threat_map->damage_potential_map[x][y] < 0) {
                threat_map->damage_potential_map[x][y] = 0;
                threat_map->shots_map[x][y] = 0;
            }
        }
    }
}

bool AiPlayer::IsAbleToAttack(UnitInfo* attacker, ResourceID target_type, unsigned short team) {
    bool result;

    if (attacker->ammo > 0 && attacker->orders != ORDER_DISABLE && attacker->orders != ORDER_IDLE &&
        attacker->hits > 0 && Access_IsValidAttackTargetType(attacker->unit_type, target_type)) {
        if (attacker->unit_type != LANDMINE && attacker->unit_type != SEAMINE) {
            if ((attacker->unit_type != COMMANDO && attacker->unit_type != SUBMARNE) ||
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

ThreatMap* AiPlayer::GetThreatMap(int risk_level, int caution_level, bool is_for_attacking) {
    ThreatMap* result;

    if (ResourceManager_MapSize.x > 0) {
        if (need_init) {
            BeginTurn();
        }

        if (ini_get_setting(INI_OPPONENT) < OPPONENT_TYPE_EXPERT) {
            is_for_attacking = false;
        }

        for (int i = 0; i < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++i) {
            if (AiPlayer_ThreatMaps[i].risk_level == risk_level &&
                AiPlayer_ThreatMaps[i].caution_level == caution_level &&
                AiPlayer_ThreatMaps[i].for_attacking == is_for_attacking &&
                AiPlayer_ThreatMaps[i].team == player_team) {
                AiPlayer_ThreatMaps[i].id = 0;

                result = &AiPlayer_ThreatMaps[i];

                for (int j = 0; j < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++j) {
                    ++AiPlayer_ThreatMaps[j].id;
                }

                return result;
            }
        }

        int index = 0;

        for (int i = 1; i < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++i) {
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

        for (int i = 0; i < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++i) {
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
            if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_MASTER) {
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

        int team_count = 0;
        signed short enemies[PLAYER_TEAM_MAX];
        char* heat_maps_stealth_sea[PLAYER_TEAM_MAX];

        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            if (team != player_team && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                enemies[team_count] = team;
                ++team_count;
            }
        }

        if (risk_level == 7) {
            for (int team = 0; team < team_count; ++team) {
                heat_maps_stealth_sea[team] = UnitsManager_TeamInfo[enemies[team]].heat_map_stealth_sea;
            }

            for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                    if (ResourceManager_MapSurfaceMap[y * ResourceManager_MapSize.x + x] == SURFACE_TYPE_WATER) {
                        bool is_found = false;

                        for (int team = 0; team < team_count; ++team) {
                            if (heat_maps_stealth_sea[team][y * ResourceManager_MapSize.x + x]) {
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

        for (int team = 0; team < team_count; ++team) {
            char* active_heat_map = UnitsManager_TeamInfo[enemies[team]].heat_map_complete;

            if (risk_level == 6) {
                active_heat_map = UnitsManager_TeamInfo[enemies[team]].heat_map_stealth_sea;

            } else if (risk_level == 5 || risk_level == 4) {
                active_heat_map = UnitsManager_TeamInfo[enemies[team]].heat_map_stealth_land;
            }

            for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                    if (active_heat_map && active_heat_map[y * ResourceManager_MapSize.x + x]) {
                        AiPlayer_ThreatMaps[index].damage_potential_map[x][y] |= 0x8000;
                    }
                }
            }
        }

        if (risk_level == 4) {
            short** active_damage_potential_map = AiPlayer_ThreatMaps[index].damage_potential_map;

            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it != spotted_units.End(); ++it) {
                if ((*it).GetUnit()->orders == ORDER_DISABLE) {
                    ZoneWalker walker((*it).GetLastPosition(), 4);

                    do {
                        active_damage_potential_map[walker.GetGridX()][walker.GetGridY()] |= 0x8000;

                    } while (walker.FindNext());
                }
            }
        }

        for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
            for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                if (AiPlayer_ThreatMaps[index].damage_potential_map[x][y] & 0x8000) {
                    AiPlayer_ThreatMaps[index].damage_potential_map[x][y] &= ~0x8000;

                } else {
                    AiPlayer_ThreatMaps[index].damage_potential_map[x][y] = 0x00;
                }
            }
        }

        if (risk_level != 3 && risk_level != 2 && mine_map) {
            for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
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

                if (unit->unit_type == LANDMINE || unit->unit_type == SEAMINE) {
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

            if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_EXPERT) {
                table += weight_table_commando;
            }

            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it; ++it) {
                table += GetWeightTable((*it).GetUnit()->unit_type);
            }

            result = table;

        } break;

        default: {
            result = weight_table_generic;
        } break;
    }

    return result;
}

void AiPlayer::AddThreatToMineMap(int grid_x, int grid_y, int range, int damage_potential, int factor) {
    Point position(grid_x, grid_y);
    Rect bounds;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    for (int direction = 0; direction < 8; direction += 2) {
        for (int i = 0; i < range; ++i) {
            position += Paths_8DirPointsArray[direction];

            if (Access_IsInsideBounds(&bounds, &position)) {
                signed char mine_value = mine_map[position.x][position.y];

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
        int base_attack = unit->GetBaseValues()->GetAttribute(ATTRIB_ATTACK);

        AddThreatToMineMap(position.x - 1, position.y + 1, 2, base_attack, 7);
        AddThreatToMineMap(position.x - 2, position.y + 2, 4, base_attack, 5);
        AddThreatToMineMap(position.x - 3, position.y + 3, 6, base_attack, 2);
        AddThreatToMineMap(position.x - 4, position.y + 4, 8, base_attack, 1);

        if (!task_3) {
            task_3 = new (std::nothrow) TaskFindMines(player_team, position);

            TaskManager.AppendTask(*task_3);
        }
    }
}

bool AiPlayer::IsSurfaceTypePresent(Point site, int range, int surface_type) {
    ZoneWalker walker(site, range);

    do {
        if (Access_GetModifiedSurfaceType(walker.GetGridX(), walker.GetGridY()) == surface_type) {
            return true;
        }
    } while (walker.FindNext());

    return false;
}

int AiPlayer::SelectTeamClan() {
    int team_clan;

    switch (strategy) {
        case AI_STRATEGY_DEFENSIVE: {
            if ((dos_rand() * 7) >> 15) {
                team_clan = TEAM_CLAN_7_KNIGHTS;

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_MISSILES: {
            if ((dos_rand() * 7) >> 15) {
                team_clan = TEAM_CLAN_AYERS_HAND;

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_AIR: {
            if ((dos_rand() * 7) >> 15) {
                team_clan = TEAM_CLAN_THE_CHOSEN;

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_SEA: {
            if ((dos_rand() * 7) >> 15) {
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
            if ((dos_rand() * 7) >> 15) {
                team_clan = TEAM_CLAN_VON_GRIFFIN + ((dos_rand() * 3) >> 15);

            } else {
                team_clan = TEAM_CLAN_AXIS_INC;
            }
        } break;

        case AI_STRATEGY_ESPIONAGE: {
            if ((dos_rand() * 7) >> 15) {
                team_clan = TEAM_CLAN_CRIMSON_PATH;

            } else {
                team_clan = TEAM_CLAN_VON_GRIFFIN;
            }
        } break;

        default: {
            SDL_assert(false);
        } break;
    }

    ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + player_team), team_clan);

    ResourceManager_InitClanUnitValues(player_team);

    return team_clan;
}

void AiPlayer::RollField3() { field_3 = ((dos_rand() * 4) >> 15) + 2 + ((dos_rand() * 4) >> 15) + 2; }

void AiPlayer::RollField5() {
    switch (strategy) {
        case AI_STRATEGY_DEFENSIVE:
        case AI_STRATEGY_AIR:
        case AI_STRATEGY_SEA: {
            field_5 = ((dos_rand() * 6) >> 15) + 7;
        } break;

        case AI_STRATEGY_SCOUT_HORDE:
        case AI_STRATEGY_TANK_HORDE: {
            field_5 = ((dos_rand() * 3) >> 15) + 1;
            field_5 = 5;
        } break;

        case AI_STRATEGY_MISSILES:
        case AI_STRATEGY_FAST_ATTACK:
        case AI_STRATEGY_COMBINED_ARMS:
        case AI_STRATEGY_ESPIONAGE: {
            field_5 = ((dos_rand() * 2) >> 15) + 4;
        } break;

        default: {
            field_5 = 0;
        } break;
    }
}

void AiPlayer::RollField7() {
    if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        switch (strategy) {
            case AI_STRATEGY_AIR:
            case AI_STRATEGY_SEA: {
                if (((dos_rand() * 100) >> 15) + 1 < 50) {
                    field_7 = (((dos_rand() * 5) >> 15) + ((dos_rand() * 5) >> 15)) * 6;

                } else {
                    field_7 = 0;
                }
            } break;

            case AI_STRATEGY_DEFENSIVE: {
                field_7 = (((dos_rand() * 7) >> 15) + ((dos_rand() * 7) >> 15)) * 12;
            } break;

            case AI_STRATEGY_SCOUT_HORDE:
            case AI_STRATEGY_TANK_HORDE: {
                field_7 = 0;
            } break;

            case AI_STRATEGY_MISSILES:
            case AI_STRATEGY_FAST_ATTACK:
            case AI_STRATEGY_COMBINED_ARMS:
            case AI_STRATEGY_ESPIONAGE: {
                if (((dos_rand() * 100) >> 15) + 1 < 25) {
                    field_7 = (((dos_rand() * 5) >> 15) + ((dos_rand() * 5) >> 15)) * 6;

                } else {
                    field_7 = 0;
                }
            } break;
        }

    } else {
        field_7 = 0;
    }
}

bool AiPlayer::AddUnitToTeamMissionSupplies(ResourceID unit_type, unsigned short supplies) {
    TeamMissionSupplies* mission_supplies = &UnitsManager_TeamMissionSupplies[player_team];
    int build_cost = Ai_GetNormalRateBuildCost(unit_type, player_team);
    bool result;

    if (Builder_IsBuildable(unit_type) && mission_supplies->units.GetCount() < 20) {
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

int AiPlayer::GetVictoryConditionsFactor() {
    int result;

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

void AiPlayer::RollTeamMissionSupplies(int clan) {
    TeamMissionSupplies* mission_supplies = &UnitsManager_TeamMissionSupplies[player_team];
    ResourceID unit_type;
    unsigned short cargo;

    mission_supplies->team_gold =
        ini_clans.GetClanGold(UnitsManager_TeamInfo[player_team].team_clan) + ini_get_setting(INI_START_GOLD);

    mission_supplies->units.Clear();

    unit_type = CONSTRCT;
    mission_supplies->units.PushBack(&unit_type);

    unit_type = ENGINEER;
    mission_supplies->units.PushBack(&unit_type);

    unit_type = SURVEYOR;
    mission_supplies->units.PushBack(&unit_type);

    UnitsManager_AddAxisMissionLoadout(player_team, mission_supplies->units);

    mission_supplies->cargos.Clear();

    cargo = 40;
    mission_supplies->cargos.PushBack(&cargo);

    cargo = 20;
    mission_supplies->cargos.PushBack(&cargo);

    for (int i = 2; i < mission_supplies->units.GetCount(); ++i) {
        cargo = 0;
        mission_supplies->cargos.PushBack(&cargo);
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

            ChooseInitialUpgrades(((((dos_rand() * 5) >> 15) + 4) * mission_supplies->team_gold) / 16);

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

                if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
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

            ChooseInitialUpgrades(((((dos_rand() * 5) >> 15) + 4) * mission_supplies->team_gold) / 16);

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

            ChooseInitialUpgrades(((((dos_rand() * 4) >> 15) + 1) * mission_supplies->team_gold) / 16);

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

            ChooseInitialUpgrades(((((dos_rand() * 4) >> 15) + 1) * mission_supplies->team_gold) / 16);

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

            ChooseInitialUpgrades(((((dos_rand() * 4) >> 15) + 2) * mission_supplies->team_gold) / 8);

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

            ChooseInitialUpgrades(((((dos_rand() * 6) >> 15) + 1) * mission_supplies->team_gold) / 8);

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

            ChooseInitialUpgrades(((((dos_rand() * 5) >> 15) + 3) * mission_supplies->team_gold) / 12);

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

            ChooseInitialUpgrades(((((dos_rand() * 10) >> 15) + 3) * mission_supplies->team_gold) / 16);

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

            ChooseInitialUpgrades(((((dos_rand() * 10) >> 11) + 2) * mission_supplies->team_gold) / 16);

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

void AiPlayer::AddBuildOrder(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type, int attribute) {
    if (Builder_IsBuildable(unit_type)) {
        BuildOrder order(attribute, unit_type);

        (*build_orders).PushBack(&order);
    }
}

void AiPlayer::CheckReconnaissanceNeeds(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type,
                                        unsigned short team, unsigned short enemy_team, bool mode) {
    int unit_range_max = 0;
    int unit_scan = UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type)->GetAttribute(ATTRIB_SCAN);
    CTInfo* team_info = &UnitsManager_TeamInfo[enemy_team];

    for (int i = 0; i < UNIT_END; ++i) {
        UnitValues* unit_values = UnitsManager_GetCurrentUnitValues(team_info, static_cast<ResourceID>(i));

        if (unit_values->GetAttribute(ATTRIB_ATTACK) > 0 && (mode || unit_values->GetAttribute(ATTRIB_SPEED)) &&
            Access_IsValidAttackTargetType(static_cast<ResourceID>(i), unit_type)) {
            unit_range_max = std::max(unit_range_max, unit_values->GetAttribute(ATTRIB_RANGE) + 1);
            if (team_info->team_units->GetBaseUnitValues(static_cast<ResourceID>(i))->GetAttribute(ATTRIB_RANGE) >
                unit_scan) {
                return;
            }
        }
    }

    if (unit_range_max >
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_SCAN)) {
        AddBuildOrder(build_orders, unit_type, ATTRIB_SCAN);
    }
}

SmartObjectArray<BuildOrder> AiPlayer::ChooseStrategicBuildOrders(bool mode) {
    SmartObjectArray<BuildOrder> build_orders;

    if (target_team < PLAYER_TEAM_RED) {
        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (target_team < PLAYER_TEAM_RED || UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER) {
                target_team = team;
            }
        }
    }

    if (target_team >= PLAYER_TEAM_RED) {
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
                AddBuildOrder(&build_orders, COMMANDO, ATTRIB_SCAN);
            }

            if (UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[target_team], CORVETTE)
                    ->GetAttribute(ATTRIB_SCAN) >=
                UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], SUBMARNE)
                    ->GetAttribute(ATTRIB_SCAN)) {
                AddBuildOrder(&build_orders, SUBMARNE, ATTRIB_SCAN);
            }

            ProcessBuildOrders(build_orders);
        }
    }

    if (!build_orders->GetCount()) {
        switch (strategy) {
            case AI_STRATEGY_DEFENSIVE: {
                AddBuildOrder(&build_orders, ANTIMSSL, ATTRIB_RANGE);
                AddBuildOrder(&build_orders, ANTIMSSL, ATTRIB_ATTACK);

                if (!mode) {
                    AddBuildOrder(&build_orders, RADAR, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, GUNTURRT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, GUNTURRT, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, GUNTURRT, ATTRIB_ARMOR);
                    AddBuildOrder(&build_orders, GUNTURRT, ATTRIB_HITS);
                    AddBuildOrder(&build_orders, GUNTURRT, ATTRIB_RANGE);

                    AddBuildOrder(&build_orders, ANTIAIR, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, ANTIAIR, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, ANTIAIR, ATTRIB_ATTACK);
                }

            } break;

            case AI_STRATEGY_MISSILES: {
                AddBuildOrder(&build_orders, MISSLLCH, ATTRIB_RANGE);

                if (!mode) {
                    AddBuildOrder(&build_orders, SCANNER, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, MISSLLCH, ATTRIB_ATTACK);

                    AddBuildOrder(&build_orders, SCANNER, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, MISSLLCH, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, MSSLBOAT, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, MSSLBOAT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, MSSLBOAT, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_SCAN);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_SPEED);
                }
            } break;

            case AI_STRATEGY_AIR: {
                AddBuildOrder(&build_orders, FIGHTER, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, FIGHTER, ATTRIB_RANGE);

                AddBuildOrder(&build_orders, BOMBER, ATTRIB_ATTACK);

                if (!mode) {
                    AddBuildOrder(&build_orders, FIGHTER, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, FIGHTER, ATTRIB_HITS);
                    AddBuildOrder(&build_orders, FIGHTER, ATTRIB_ARMOR);

                    AddBuildOrder(&build_orders, AWAC, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, BOMBER, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, BOMBER, ATTRIB_HITS);
                    AddBuildOrder(&build_orders, BOMBER, ATTRIB_ARMOR);
                }

            } break;

            case AI_STRATEGY_SEA: {
                AddBuildOrder(&build_orders, BATTLSHP, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, BATTLSHP, ATTRIB_ARMOR);
                AddBuildOrder(&build_orders, BATTLSHP, ATTRIB_HITS);

                AddBuildOrder(&build_orders, MSSLBOAT, ATTRIB_RANGE);

                if (!mode) {
                    AddBuildOrder(&build_orders, SUBMARNE, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, SUBMARNE, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, BATTLSHP, ATTRIB_ROUNDS);

                    AddBuildOrder(&build_orders, MSSLBOAT, ATTRIB_ATTACK);

                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_SPEED);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_SPEED);
                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_SCAN);
                }

            } break;

            case AI_STRATEGY_SCOUT_HORDE: {
                AddBuildOrder(&build_orders, SCOUT, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, SCOUT, ATTRIB_ARMOR);
                AddBuildOrder(&build_orders, SCOUT, ATTRIB_HITS);
                AddBuildOrder(&build_orders, SCOUT, ATTRIB_SPEED);
            } break;

            case AI_STRATEGY_TANK_HORDE: {
                AddBuildOrder(&build_orders, SCOUT, ATTRIB_SPEED);

                AddBuildOrder(&build_orders, TANK, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, TANK, ATTRIB_ROUNDS);
                AddBuildOrder(&build_orders, TANK, ATTRIB_HITS);
                AddBuildOrder(&build_orders, TANK, ATTRIB_ARMOR);
                AddBuildOrder(&build_orders, TANK, ATTRIB_SPEED);

                if (!mode) {
                    AddBuildOrder(&build_orders, TANK, ATTRIB_RANGE);
                }
            } break;

            case AI_STRATEGY_FAST_ATTACK: {
                AddBuildOrder(&build_orders, SCOUT, ATTRIB_SCAN);

                AddBuildOrder(&build_orders, ARTILLRY, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, ARTILLRY, ATTRIB_ROUNDS);
                AddBuildOrder(&build_orders, ARTILLRY, ATTRIB_RANGE);

                if (!mode) {
                    AddBuildOrder(&build_orders, ARTILLRY, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, SCOUT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, SCOUT, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, SP_FLAK, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, SP_FLAK, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, SP_FLAK, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, SP_FLAK, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, BOMBER, ATTRIB_ATTACK);

                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_SPEED);
                    AddBuildOrder(&build_orders, FASTBOAT, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_RANGE);
                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_SPEED);
                    AddBuildOrder(&build_orders, CORVETTE, ATTRIB_SCAN);
                }
            } break;

            case AI_STRATEGY_COMBINED_ARMS: {
                AddBuildOrder(&build_orders, MISSLLCH, ATTRIB_RANGE);

                AddBuildOrder(&build_orders, TANK, ATTRIB_ATTACK);
                AddBuildOrder(&build_orders, TANK, ATTRIB_ARMOR);
                AddBuildOrder(&build_orders, TANK, ATTRIB_HITS);

                if (!mode) {
                    AddBuildOrder(&build_orders, SCANNER, ATTRIB_SCAN);
                    AddBuildOrder(&build_orders, SCANNER, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, MISSLLCH, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, MISSLLCH, ATTRIB_SPEED);

                    AddBuildOrder(&build_orders, TANK, ATTRIB_ROUNDS);
                    AddBuildOrder(&build_orders, TANK, ATTRIB_SPEED);
                }
            } break;

            case AI_STRATEGY_ESPIONAGE: {
                AddBuildOrder(&build_orders, COMMANDO, ATTRIB_SCAN);
                AddBuildOrder(&build_orders, COMMANDO, ATTRIB_SPEED);

                AddBuildOrder(&build_orders, MISSLLCH, ATTRIB_RANGE);

                if (!mode) {
                    AddBuildOrder(&build_orders, SUBMARNE, ATTRIB_ATTACK);
                    AddBuildOrder(&build_orders, SUBMARNE, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, SCANNER, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, AWAC, ATTRIB_SCAN);

                    AddBuildOrder(&build_orders, MSSLBOAT, ATTRIB_RANGE);

                    AddBuildOrder(&build_orders, CLNTRANS, ATTRIB_SPEED);
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
        if ((*it).team == player_team && (*it).orders == ORDER_BUILD && (*it).state != ORDER_STATE_UNIT_READY) {
            ResourceID constructed_unit_type = (*it).GetConstructedUnitType();

            if (unit_types->Find(&constructed_unit_type) < 0) {
                unit_types.PushBack(&constructed_unit_type);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == player_team && (*it).orders == ORDER_BUILD && (*it).state != ORDER_STATE_UNIT_READY) {
            ResourceID constructed_unit_type = (*it).GetConstructedUnitType();

            if (unit_types->Find(&constructed_unit_type) < 0) {
                unit_types.PushBack(&constructed_unit_type);
            }
        }
    }

    for (int i = build_orders.GetCount() - 1; i >= 0; --i) {
        if (unit_types->Find(&build_orders[i]->unit_type) < 0) {
            build_orders.Remove(i);
        }
    }
}

SmartObjectArray<BuildOrder> AiPlayer::ChooseGenericBuildOrders() {
    SmartObjectArray<BuildOrder> result;

    AddBuildOrder(&result, ANTIMSSL, ATTRIB_RANGE);
    AddBuildOrder(&result, ANTIMSSL, ATTRIB_ATTACK);
    AddBuildOrder(&result, ANTIMSSL, ATTRIB_ROUNDS);

    AddBuildOrder(&result, RADAR, ATTRIB_SCAN);

    AddBuildOrder(&result, GUNTURRT, ATTRIB_ATTACK);
    AddBuildOrder(&result, GUNTURRT, ATTRIB_ROUNDS);
    AddBuildOrder(&result, GUNTURRT, ATTRIB_ARMOR);
    AddBuildOrder(&result, GUNTURRT, ATTRIB_HITS);
    AddBuildOrder(&result, GUNTURRT, ATTRIB_RANGE);

    AddBuildOrder(&result, ANTIAIR, ATTRIB_ROUNDS);
    AddBuildOrder(&result, ANTIAIR, ATTRIB_RANGE);
    AddBuildOrder(&result, ANTIAIR, ATTRIB_ATTACK);

    AddBuildOrder(&result, MISSLLCH, ATTRIB_RANGE);
    AddBuildOrder(&result, MISSLLCH, ATTRIB_ATTACK);
    AddBuildOrder(&result, MISSLLCH, ATTRIB_ROUNDS);

    AddBuildOrder(&result, SCANNER, ATTRIB_SCAN);

    AddBuildOrder(&result, MSSLBOAT, ATTRIB_RANGE);
    AddBuildOrder(&result, MSSLBOAT, ATTRIB_ATTACK);
    AddBuildOrder(&result, MSSLBOAT, ATTRIB_ROUNDS);

    AddBuildOrder(&result, FASTBOAT, ATTRIB_SCAN);
    AddBuildOrder(&result, FASTBOAT, ATTRIB_ATTACK);
    AddBuildOrder(&result, FASTBOAT, ATTRIB_ROUNDS);
    AddBuildOrder(&result, FASTBOAT, ATTRIB_RANGE);

    AddBuildOrder(&result, FIGHTER, ATTRIB_ATTACK);
    AddBuildOrder(&result, FIGHTER, ATTRIB_ROUNDS);
    AddBuildOrder(&result, FIGHTER, ATTRIB_RANGE);
    AddBuildOrder(&result, FIGHTER, ATTRIB_HITS);
    AddBuildOrder(&result, FIGHTER, ATTRIB_ARMOR);

    AddBuildOrder(&result, BOMBER, ATTRIB_ATTACK);
    AddBuildOrder(&result, BOMBER, ATTRIB_ROUNDS);
    AddBuildOrder(&result, BOMBER, ATTRIB_HITS);
    AddBuildOrder(&result, BOMBER, ATTRIB_ARMOR);

    AddBuildOrder(&result, AWAC, ATTRIB_SCAN);

    AddBuildOrder(&result, BATTLSHP, ATTRIB_ATTACK);
    AddBuildOrder(&result, BATTLSHP, ATTRIB_ARMOR);
    AddBuildOrder(&result, BATTLSHP, ATTRIB_RANGE);
    AddBuildOrder(&result, BATTLSHP, ATTRIB_HITS);
    AddBuildOrder(&result, BATTLSHP, ATTRIB_ROUNDS);

    AddBuildOrder(&result, SUBMARNE, ATTRIB_ATTACK);
    AddBuildOrder(&result, SUBMARNE, ATTRIB_SCAN);
    AddBuildOrder(&result, SUBMARNE, ATTRIB_ROUNDS);

    AddBuildOrder(&result, CORVETTE, ATTRIB_ATTACK);
    AddBuildOrder(&result, CORVETTE, ATTRIB_ROUNDS);
    AddBuildOrder(&result, CORVETTE, ATTRIB_SCAN);
    AddBuildOrder(&result, CORVETTE, ATTRIB_RANGE);

    AddBuildOrder(&result, SCOUT, ATTRIB_SPEED);
    AddBuildOrder(&result, SCOUT, ATTRIB_SCAN);

    AddBuildOrder(&result, TANK, ATTRIB_ATTACK);
    AddBuildOrder(&result, TANK, ATTRIB_ROUNDS);
    AddBuildOrder(&result, TANK, ATTRIB_HITS);
    AddBuildOrder(&result, TANK, ATTRIB_ARMOR);

    AddBuildOrder(&result, ARTILLRY, ATTRIB_ATTACK);
    AddBuildOrder(&result, ARTILLRY, ATTRIB_ROUNDS);
    AddBuildOrder(&result, ARTILLRY, ATTRIB_RANGE);
    AddBuildOrder(&result, ARTILLRY, ATTRIB_SPEED);

    AddBuildOrder(&result, SP_FLAK, ATTRIB_ATTACK);
    AddBuildOrder(&result, SP_FLAK, ATTRIB_ROUNDS);
    AddBuildOrder(&result, SP_FLAK, ATTRIB_RANGE);

    AddBuildOrder(&result, COMMANDO, ATTRIB_SCAN);
    AddBuildOrder(&result, COMMANDO, ATTRIB_SPEED);

    ProcessBuildOrders(result);

    return result;
}

void AiPlayer::ChooseInitialUpgrades(int team_gold) {
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
        SmartPointer<UnitValues> unit_vales(
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], build_order.unit_type));

        unit_vales = new (std::nothrow) UnitValues(*unit_vales);

        unit_vales->UpdateVersion();
        unit_vales->SetUnitsBuilt(0);

        unit_vales->GetAttributeAddress(build_order.primary_attribute)[0] +=
            TeamUnits_UpgradeOffsetFactor(player_team, build_order.unit_type, build_order.primary_attribute);

        UnitsManager_TeamInfo[player_team].team_units->SetCurrentUnitValues(build_order.unit_type, *unit_vales);

        if (Remote_IsNetworkGame) {
            Remote_SendNetPacket_10(player_team, build_order.unit_type);
        }
    }
}

AiPlayer::AiPlayer() : info_map(nullptr), mine_map(nullptr), target_location(1, 1) {}

AiPlayer::~AiPlayer() {
    if (info_map) {
        for (int i = 0; i < dimension_x; ++i) {
            delete[] info_map[i];
        }

        delete[] info_map;
    }

    if (mine_map) {
        for (int i = 0; i < dimension_x; ++i) {
            delete[] mine_map[i];
        }

        delete[] mine_map;
    }
}

int AiPlayer::GetStrategy() const { return strategy; }

signed short AiPlayer::GetTargetTeam() const { return target_team; }

unsigned char** AiPlayer::GetInfoMap() { return info_map; }

Point AiPlayer::GetTargetLocation() const { return target_location; }

unsigned short AiPlayer::GetField5() const { return field_5; }

signed char** AiPlayer::GetMineMap() { return mine_map; }

void AiPlayer::AddTransportOrder(TransportOrder* transport_order) { transport_orders.PushBack(*transport_order); }

Task* AiPlayer::FindManager(Point site) {
    SmartPointer<Task> task;

    if (task_list.GetCount() > 0) {
        Point position;
        Point location;
        SmartList<Task>::Iterator task_it = task_list.Begin();
        Rect bounds;
        int distance;
        int minimum_distance;

        task = *task_it;

        task->GetBounds(&bounds);

        position = site;

        position -= Point(bounds.ulx, bounds.uly);

        minimum_distance = position.x * position.x + position.y * position.y;

        for (++task_it; task_it; ++task_it) {
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

SmartList<SpottedUnit>::Iterator AiPlayer::GetSpottedUnitIterator() { return spotted_units.Begin(); }

int AiPlayer::GetMemoryUsage() {
    return task_list.GetMemorySize() + spotted_units.GetMemorySize() - 10 +
           (ResourceManager_MapSize.x * ResourceManager_MapSize.y) * sizeof(void*) - 10;
}

void AiPlayer::AddMilitaryUnit(UnitInfo* unit) {
    if (Access_IsValidAttackTargetType(unit->unit_type, FIGHTER)) {
        air_force.PushBack(*unit);

    } else {
        ground_forces.PushBack(*unit);
    }

    if (unit->shots > 0 && !need_init) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindAttack(*unit), true);
    }
}

void AiPlayer::BeginTurn() {
    if (UnitsManager_TeamInfo[player_team].team_type == TEAM_TYPE_COMPUTER) {
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

        int player_team_points = UnitsManager_TeamInfo[player_team].team_points;
        int team_building_counts[PLAYER_TEAM_MAX];
        int target_team_points;
        int enemy_team_points;

        memset(team_building_counts, 0, sizeof(team_building_counts));

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            ++team_building_counts[(*it).team];
        }

        if (target_team >= PLAYER_TEAM_RED) {
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

        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE && player_team != team && target_team != team) {
                enemy_team_points = UnitsManager_TeamInfo[team].team_points;

                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER) {
                    if (enemy_team_points < player_team_points) {
                        ++enemy_team_points;
                    } else {
                        enemy_team_points += 1000;
                    }
                }

                if (target_team < PLAYER_TEAM_RED || target_team_points < enemy_team_points ||
                    (target_team_points == enemy_team_points &&
                     team_building_counts[target_team] < team_building_counts[team])) {
                    target_team = team;
                    target_team_points = enemy_team_points;
                }
            }
        }

        if (need_init) {
            task_1 = new (std::nothrow) TaskCheckAssaults(player_team);

            TaskManager.AppendTask(*task_1);

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
                info_map = new (std::nothrow) unsigned char*[ResourceManager_MapSize.x];

                for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
                    info_map[i] = new (std::nothrow) unsigned char[ResourceManager_MapSize.y];

                    memset(info_map[i], 0, ResourceManager_MapSize.y);
                }
            }

            if (!mine_map) {
                mine_map = new (std::nothrow) signed char*[ResourceManager_MapSize.x];

                for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
                    mine_map[i] = new (std::nothrow) signed char[ResourceManager_MapSize.y];

                    memset(mine_map[i], 0, ResourceManager_MapSize.y);
                }
            }

            for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
                for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                    if (UnitsManager_TeamInfo[player_team].heat_map_complete[y * ResourceManager_MapSize.x + x]) {
                        info_map[x][y] = 1;
                    }
                }
            }

            if (position.x || position.y) {
                SmartPointer<Task> survey(new (std::nothrow) TaskSurvey(player_team, position));

                for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                     it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                    if ((*it).team == player_team && (*it).unit_type == SURVEYOR) {
                        survey->AddUnit(*it);
                    }
                }

                TaskManager.AppendTask(*survey);

                SmartPointer<Task> explore(new (std::nothrow) TaskExplore(player_team, position));

                TaskManager.AppendTask(*explore);
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
                 it != UnitsManager_GroundCoverUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).orders != ORDER_IDLE &&
                    ((*it).unit_type == WTRPLTFM || (*it).unit_type == BRIDGE)) {
                    AddBuilding(&*it);
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                 it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).orders != ORDER_IDLE) {
                    AddBuilding(&*it);
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).orders == ORDER_BUILD) {
                    AddBuilding(&*it);
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).orders == ORDER_IDLE && (*it).state == ORDER_STATE_4) {
                    SmartPointer<Task> move(new (std::nothrow) TaskMove(&*it, &MoveFinishedCallback));

                    TaskManager.AppendTask(*move);
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                 it != UnitsManager_MobileAirUnits.End(); ++it) {
                if ((*it).team == player_team) {
                    if ((*it).unit_type == AWAC || (*it).unit_type == BOMBER) {
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
                    if ((*it).unit_type == MISSLLCH || (*it).unit_type == SCANNER) {
                        SmartPointer<Task> tank_escort(new (std::nothrow) TaskEscort(&*it, TANK));
                        TaskManager.AppendTask(*tank_escort);

                        SmartPointer<Task> sp_flak_escort(new (std::nothrow) TaskEscort(&*it, SP_FLAK));
                        TaskManager.AppendTask(*sp_flak_escort);
                    }

                    if ((*it).unit_type == SP_FLAK || (*it).unit_type == FASTBOAT) {
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

            if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_EXPERT) {
                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team != player_team && ((*it).unit_type == MININGST || (*it).unit_type == GREENHSE)) {
                        Ai_UnitSpotted(&*it, player_team);
                    }
                }
            }

            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it; ++it) {
                if (IsKeyFacility((*it).GetUnit()->unit_type) && (*it).GetUnit()->team == target_team) {
                    DetermineAttack(&*it, 0x1F00);
                }
            }

            if (!task_3) {
                bool is_found = false;
                int grid_x;
                int grid_y;

                for (grid_x = 0; grid_x < ResourceManager_MapSize.x && !is_found; ++grid_x) {
                    for (grid_y = 0; grid_y < ResourceManager_MapSize.y && !is_found; ++grid_y) {
                        if (mine_map[grid_x][grid_y] > 0) {
                            is_found = true;
                        }
                    }
                }

                if (is_found) {
                    task_3 = new (std::nothrow) TaskFindMines(player_team, Point(grid_x, grid_y));

                    TaskManager.AppendTask(*task_3);
                }
            }

            SmartPointer<Task> mine_assistant(new (std::nothrow) TaskMineAssistant(player_team));
            TaskManager.AppendTask(*mine_assistant);

            SmartPointer<Task> update_terrain(new (std::nothrow) TaskUpdateTerrain(player_team));
            TaskManager.AppendTask(*update_terrain);

            if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_APPRENTICE) {
                SmartPointer<Task> scavenge(new (std::nothrow) TaskScavenge(player_team));
                TaskManager.AppendTask(*scavenge);

                SmartPointer<Task> assist_move(new (std::nothrow) TaskAssistMove(player_team));
                TaskManager.AppendTask(*assist_move);
            }

            need_init = false;
        }

        bool teams[PLAYER_TEAM_MAX];

        AiAttack_GetTargetTeams(player_team, teams);

        for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it; ++it) {
            if (!(*it).GetTask()) {
                UnitInfo* target = (*it).GetUnit();

                if (AiAttack_IsWithinReach(target, player_team, teams)) {
                    DetermineAttack(&*it, 0x1000);

                } else if (target->team == target_team || target->team == PLAYER_TEAM_ALIEN) {
                    if ((target->unit_type == CONSTRCT && target->orders == ORDER_BUILD) ||
                        IsKeyFacility(target->unit_type) ||
                        (target->team == PLAYER_TEAM_ALIEN &&
                         (target->flags & (MOBILE_AIR_UNIT | MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)))) {
                        DetermineAttack(&*it, 0x1F00);
                    }
                }
            }
        }

        int fuel_reserves = 0;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == player_team && (*it).unit_type == FDUMP) {
                fuel_reserves += (*it).storage;
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == player_team && (*it).unit_type == MININGST && (*it).orders != ORDER_POWER_ON &&
                (*it).orders != ORDER_IDLE) {
                UnitsManager_SetNewOrder(&*it, ORDER_POWER_ON, ORDER_STATE_0);
            }
        }

        if (fuel_reserves >= 10) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                 it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == player_team &&
                    ((*it).unit_type == COMMTWR || (*it).unit_type == GREENHSE || (*it).unit_type == RESEARCH) &&
                    (*it).orders != ORDER_POWER_ON && (*it).orders != ORDER_IDLE) {
                    UnitsManager_SetNewOrder(&*it, ORDER_POWER_ON, ORDER_STATE_0);
                }
            }
        }

        SmartObjectArray<BuildOrder> strategic_orders = ChooseStrategicBuildOrders(false);
        SmartObjectArray<BuildOrder> generic_orders = ChooseGenericBuildOrders();

        ChooseUpgrade(strategic_orders, generic_orders);

        int team_gold = UnitsManager_TeamInfo[player_team].team_units->GetGold();

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
        AccessMap access_map;
        SmartList<UnitInfo> units;
        Point site;

        for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
            for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
                info_map[site.x][site.y] &= 0xFB;
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
                access_map.GetMapColumn(walker.GetGridX())[walker.GetGridY()] = 0x01;
            } while (walker.FindNext());
        }

        for (int team = PLAYER_TEAM_MAX; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (team != player_team && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
                    for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
                        if (UnitsManager_TeamInfo[team]
                                .heat_map_complete[site.y * ResourceManager_MapSize.x + site.x]) {
                            access_map.GetMapColumn(site.x)[site.y] = 0x00;
                        }
                    }
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            CircumferenceWalker walker(Point((*it).grid_x, (*it).grid_y),
                                       (*it).GetBaseValues()->GetAttribute(ATTRIB_SCAN) + 10);

            do {
                if (!access_map.GetMapColumn(walker.GetGridX())[walker.GetGridY()]) {
                    UpdateAccessMap(*walker.GetGridXY(), Point((*it).grid_x, (*it).grid_y), access_map.GetMap());
                }
            } while (walker.FindNext());
        }
    }
}

void AiPlayer::PlanMinefields() {
    if (info_map && field_7 > 0) {
        AccessMap access_map;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == player_team) {
                if ((*it).unit_type == GUNTURRT || (*it).unit_type == ANTIMSSL || (*it).unit_type == ARTYTRRT) {
                    ZoneWalker walker(Point((*it).grid_x, (*it).grid_y),
                                      (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE));
                    int surface_type = Access_GetSurfaceType((*it).grid_x, (*it).grid_y);

                    do {
                        if (Access_GetSurfaceType(walker.GetGridX(), walker.GetGridY()) == surface_type) {
                            access_map.GetMapColumn(walker.GetGridX())[walker.GetGridY()] = 0x01;
                        }

                    } while (walker.FindNext());

                } else if ((*it).flags & BUILDING) {
                    ZoneWalker walker(Point((*it).grid_x, (*it).grid_y), 8);
                    int surface_type = Access_GetSurfaceType((*it).grid_x, (*it).grid_y);

                    do {
                        if (Access_GetSurfaceType(walker.GetGridX(), walker.GetGridY()) == surface_type) {
                            access_map.GetMapColumn(walker.GetGridX())[walker.GetGridY()] = 0x01;
                        }

                    } while (walker.FindNext());
                }
            }
        }

        for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it; ++it) {
            if ((*it).GetTeam() == player_team && (*it).GetType() == TaskType_TaskCreateBuilding) {
                TaskCreateBuilding* create_building = dynamic_cast<TaskCreateBuilding*>(&*it);

                if (create_building->Task_vfunc28()) {
                    if (UnitsManager_BaseUnits[create_building->GetUnitType()].flags & BUILDING) {
                        Point position = create_building->DeterminePosition();
                        ZoneWalker walker(position, 8);
                        int surface_type = Access_GetSurfaceType(position.x, position.y);

                        do {
                            if (Access_GetSurfaceType(walker.GetGridX(), walker.GetGridY()) == surface_type) {
                                access_map.GetMapColumn(walker.GetGridX())[walker.GetGridY()] = 0x01;
                            }

                        } while (walker.FindNext());
                    }
                }
            }
        }

        /// \todo The entire block is dead code due to the inner for loops.
        for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it; ++it) {
            if ((*it).GetTeam() == player_team && (*it).GetType() == TaskType_TaskCreateBuilding) {
                TaskCreateBuilding* create_building = dynamic_cast<TaskCreateBuilding*>(&*it);

                if (create_building->Task_vfunc28()) {
                    Rect bounds;

                    create_building->GetBounds(&bounds);

                    if (UnitsManager_BaseUnits[create_building->GetUnitType()].flags & BUILDING) {
                        bounds.ulx = std::max(0, bounds.ulx - 2);
                        bounds.uly = std::max(0, bounds.uly - 2);
                        bounds.lrx = std::min(static_cast<int>(ResourceManager_MapSize.x), bounds.lrx + 2);
                        bounds.lry = std::min(static_cast<int>(ResourceManager_MapSize.y), bounds.lry + 2);

                        for (int x = bounds.ulx; x < 0; ++x) {
                            for (int y = bounds.uly; y < 0; ++y) {
                                access_map.GetMapColumn(x)[y] = 0x00;
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

            if (UnitsManager_BaseUnits[(*it).unit_type].flags & BUILDING) {
                bounds.ulx = std::max(0, bounds.ulx - 2);
                bounds.uly = std::max(0, bounds.uly - 2);
                bounds.lrx = std::min(static_cast<int>(ResourceManager_MapSize.x), bounds.lrx + 2);
                bounds.lry = std::min(static_cast<int>(ResourceManager_MapSize.y), bounds.lry + 2);

                for (int x = bounds.ulx; x < 0; ++x) {
                    for (int y = bounds.uly; y < 0; ++y) {
                        access_map.GetMapColumn(x)[y] = 0x00;
                    }
                }
            }
        }

        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (team != player_team) {
                if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                    for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
                        for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                            if (UnitsManager_TeamInfo[team].heat_map_complete[y * ResourceManager_MapSize.x + x]) {
                                access_map.GetMapColumn(x)[y] = 0x00;
                            }
                        }
                    }
                }
            }
        }

        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
            for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                if (access_map.GetMapColumn(x)[y]) {
                    ++counter1;

                    if (info_map[x][y] & 0x02) {
                        ++counter2;

                        access_map.GetMapColumn(x)[y] = 0x00;
                    }

                } else {
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).team == player_team && ((*it).unit_type == LANDMINE || (*it).unit_type == SEAMINE)) {
                if (access_map.GetMapColumn((*it).grid_x)[(*it).grid_y]) {
                    ++counter3;

                    info_map[(*it).grid_x][(*it).grid_y] &= ~0x02;
                    access_map.GetMapColumn((*it).grid_x)[(*it).grid_y] = 0x00;
                }
            }
        }

        int probability = ((field_7 * counter1) / 100) - counter2 - counter3;

        if (probability >= 32 - counter2) {
            probability = 32 - counter2;
        }

        counter1 -= counter2 + counter3;

        if (probability > 0) {
            for (int x = 0; x < ResourceManager_MapSize.x && probability; ++x) {
                for (int y = 0; y < ResourceManager_MapSize.y && probability; ++y) {
                    if (access_map.GetMapColumn(x)[y]) {
                        if (((dos_rand() * counter1) >> 15) + 1 <= probability) {
                            info_map[x][y] |= 0x02;
                            --probability;
                        }

                        --counter1;
                    }
                }
            }

            if (!task_4) {
                task_4 = (new (std::nothrow) TaskPlaceMines(player_team));
                TaskManager.AppendTask(*task_4);
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
                TaskManager.ChangeFlagsSet(player_team);
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
    return dynamic_cast<TaskManageBuildings*>(FindManager(position))->CreateBuilding(unit_type, task, task->GetFlags());
}

void AiPlayer::Init(unsigned short team) {
    strategy = AI_STRATEGY_RANDOM;
    field_3 = 0;
    target_team = -1;
    field_5 = -1;
    field_7 = -1;
    task_3 = nullptr;
    task_4 = nullptr;

    if (info_map) {
        for (int x = 0; x < dimension_x; ++x) {
            delete[] info_map[x];
        }

        delete[] info_map;
        info_map = nullptr;
    }

    if (mine_map) {
        for (int x = 0; x < dimension_x; ++x) {
            delete[] mine_map[x];
        }

        delete[] mine_map;
        mine_map = nullptr;
    }

    for (int i = 0; i < AIPLAYER_THREAT_MAP_CACHE_ENTRIES; ++i) {
        AiPlayer_ThreatMaps[i].SetRiskLevel(0);
    }

    spotted_units.Clear();
    air_force.Clear();
    ground_forces.Clear();

    player_team = team;
    need_init = true;
    tasks_pending = true;

    task_list.Clear();

    task_clear_ground_zone = nullptr;
    task_clear_air_zone = nullptr;
    task_1 = nullptr;
    attack_reserve_task = nullptr;
    defense_reserve_task = nullptr;

    for (int i = 0; i < 3; ++i) {
        task_array[i] = nullptr;
    }

    int opponent = ini_get_setting(INI_OPPONENT);

    weight_table_ground_defense.Clear();

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        UnitWeight weight1(COMMANDO, 2);
        weight_table_ground_defense.PushBack(weight1);
    }

    UnitWeight weight2(TANK, 1);
    weight_table_ground_defense.PushBack(weight2);

    UnitWeight weight3(ARTILLRY, 1);
    weight_table_ground_defense.PushBack(weight3);

    UnitWeight weight4(ROCKTLCH, 1);
    weight_table_ground_defense.PushBack(weight4);

    UnitWeight weight5(MISSLLCH, 2);
    weight_table_ground_defense.PushBack(weight5);

    UnitWeight weight6(BATTLSHP, 1);
    weight_table_ground_defense.PushBack(weight6);

    UnitWeight weight7(MSSLBOAT, 2);
    weight_table_ground_defense.PushBack(weight7);

    UnitWeight weight8(BOMBER, 2);
    weight_table_ground_defense.PushBack(weight8);

    UnitWeight weight9(JUGGRNT, 1);
    weight_table_ground_defense.PushBack(weight9);

    UnitWeight weight10(ALNTANK, 1);
    weight_table_ground_defense.PushBack(weight10);

    UnitWeight weight11(ALNASGUN, 1);
    weight_table_ground_defense.PushBack(weight11);

    UnitWeight weight12(ALNPLANE, 1);
    weight_table_ground_defense.PushBack(weight12);

    weight_table_scout.Clear();

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        UnitWeight weight13(COMMANDO, 1);
        weight_table_scout.PushBack(weight13);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight14(ARTYTRRT, 1);
        weight_table_scout.PushBack(weight14);

        UnitWeight weight15(ANTIMSSL, 1);
        weight_table_scout.PushBack(weight15);
    }

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        UnitWeight weight16(GUNTURRT, 1);
        weight_table_scout.PushBack(weight16);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight17(SUBMARNE, 1);
        weight_table_scout.PushBack(weight17);
    }

    UnitWeight weight18(SCOUT, 2);
    weight_table_scout.PushBack(weight18);

    UnitWeight weight19(TANK, 1);
    weight_table_scout.PushBack(weight19);

    UnitWeight weight20(ARTILLRY, 2);
    weight_table_scout.PushBack(weight20);

    UnitWeight weight21(ROCKTLCH, 1);
    weight_table_scout.PushBack(weight21);

    UnitWeight weight22(MISSLLCH, 2);
    weight_table_scout.PushBack(weight22);

    UnitWeight weight23(BATTLSHP, 2);
    weight_table_scout.PushBack(weight23);

    UnitWeight weight24(MSSLBOAT, 2);
    weight_table_scout.PushBack(weight24);

    UnitWeight weight25(BOMBER, 3);
    weight_table_scout.PushBack(weight25);

    UnitWeight weight26(JUGGRNT, 1);
    weight_table_scout.PushBack(weight26);

    UnitWeight weight27(ALNTANK, 1);
    weight_table_scout.PushBack(weight27);

    UnitWeight weight28(ALNASGUN, 1);
    weight_table_scout.PushBack(weight28);

    UnitWeight weight29(ALNPLANE, 1);
    weight_table_scout.PushBack(weight29);

    weight_table_tanks.Clear();

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        UnitWeight weight30(GUNTURRT, 1);
        weight_table_tanks.PushBack(weight30);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight31(ANTIMSSL, 1);
        weight_table_tanks.PushBack(weight31);
    }

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        UnitWeight weight32(COMMANDO, 1);
        weight_table_tanks.PushBack(weight32);
    }

    UnitWeight weight33(TANK, 2);
    weight_table_tanks.PushBack(weight33);

    UnitWeight weight34(ARTILLRY, 1);
    weight_table_tanks.PushBack(weight34);

    UnitWeight weight35(ROCKTLCH, 1);
    weight_table_tanks.PushBack(weight35);

    UnitWeight weight36(MISSLLCH, 2);
    weight_table_tanks.PushBack(weight36);

    UnitWeight weight37(BATTLSHP, 2);
    weight_table_tanks.PushBack(weight37);

    UnitWeight weight38(MSSLBOAT, 2);
    weight_table_tanks.PushBack(weight38);

    UnitWeight weight39(BOMBER, 3);
    weight_table_tanks.PushBack(weight39);

    UnitWeight weight40(JUGGRNT, 1);
    weight_table_tanks.PushBack(weight40);

    UnitWeight weight41(ALNTANK, 1);
    weight_table_tanks.PushBack(weight41);

    UnitWeight weight42(ALNASGUN, 1);
    weight_table_tanks.PushBack(weight42);

    UnitWeight weight43(ALNPLANE, 1);
    weight_table_tanks.PushBack(weight43);

    weight_table_assault_guns.Clear();

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        UnitWeight weight44(COMMANDO, 1);
        weight_table_assault_guns.PushBack(weight44);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight45(ANTIMSSL, 1);
        weight_table_assault_guns.PushBack(weight45);
    }

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        UnitWeight weight46(GUNTURRT, 1);
        weight_table_assault_guns.PushBack(weight46);
    }

    UnitWeight weight47(TANK, 2);
    weight_table_assault_guns.PushBack(weight47);

    UnitWeight weight48(ARTILLRY, 1);
    weight_table_assault_guns.PushBack(weight48);

    UnitWeight weight49(ROCKTLCH, 1);
    weight_table_assault_guns.PushBack(weight49);

    UnitWeight weight50(MISSLLCH, 2);
    weight_table_assault_guns.PushBack(weight50);

    UnitWeight weight51(BATTLSHP, 2);
    weight_table_assault_guns.PushBack(weight51);

    UnitWeight weight52(MSSLBOAT, 2);
    weight_table_assault_guns.PushBack(weight52);

    UnitWeight weight53(BOMBER, 3);
    weight_table_assault_guns.PushBack(weight53);

    UnitWeight weight54(JUGGRNT, 1);
    weight_table_assault_guns.PushBack(weight54);

    UnitWeight weight55(ALNTANK, 1);
    weight_table_assault_guns.PushBack(weight55);

    UnitWeight weight56(ALNASGUN, 1);
    weight_table_assault_guns.PushBack(weight56);

    UnitWeight weight57(ALNPLANE, 1);
    weight_table_assault_guns.PushBack(weight57);

    weight_table_missile_launcher.Clear();

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        UnitWeight weight58(COMMANDO, 1);
        weight_table_missile_launcher.PushBack(weight58);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight59(ANTIMSSL, 1);
        weight_table_missile_launcher.PushBack(weight59);
    }

    UnitWeight weight60(TANK, 1);
    weight_table_missile_launcher.PushBack(weight60);

    UnitWeight weight61(ARTILLRY, 1);
    weight_table_missile_launcher.PushBack(weight61);

    UnitWeight weight62(ROCKTLCH, 1);
    weight_table_missile_launcher.PushBack(weight62);

    UnitWeight weight63(MISSLLCH, 2);
    weight_table_missile_launcher.PushBack(weight63);

    UnitWeight weight64(BATTLSHP, 1);
    weight_table_missile_launcher.PushBack(weight64);

    UnitWeight weight65(MSSLBOAT, 2);
    weight_table_missile_launcher.PushBack(weight65);

    UnitWeight weight66(BOMBER, 3);
    weight_table_missile_launcher.PushBack(weight66);

    UnitWeight weight67(JUGGRNT, 1);
    weight_table_missile_launcher.PushBack(weight67);

    UnitWeight weight68(ALNTANK, 1);
    weight_table_missile_launcher.PushBack(weight68);

    UnitWeight weight69(ALNASGUN, 1);
    weight_table_missile_launcher.PushBack(weight69);

    UnitWeight weight70(ALNPLANE, 1);
    weight_table_missile_launcher.PushBack(weight70);

    weight_table_air_defense.Clear();

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        UnitWeight weight71(COMMANDO, 2);
        weight_table_air_defense.PushBack(weight71);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight72(ANTIMSSL, 1);
        weight_table_air_defense.PushBack(weight72);
    }

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        UnitWeight weight73(GUNTURRT, 1);
        weight_table_air_defense.PushBack(weight73);
    }

    UnitWeight weight74(SCOUT, 1);
    weight_table_air_defense.PushBack(weight74);

    UnitWeight weight75(TANK, 1);
    weight_table_air_defense.PushBack(weight75);

    UnitWeight weight76(ARTILLRY, 1);
    weight_table_air_defense.PushBack(weight76);

    UnitWeight weight77(ROCKTLCH, 1);
    weight_table_air_defense.PushBack(weight77);

    UnitWeight weight78(MISSLLCH, 1);
    weight_table_air_defense.PushBack(weight78);

    UnitWeight weight79(BATTLSHP, 1);
    weight_table_air_defense.PushBack(weight79);

    UnitWeight weight80(MSSLBOAT, 1);
    weight_table_air_defense.PushBack(weight80);

    UnitWeight weight81(JUGGRNT, 1);
    weight_table_air_defense.PushBack(weight81);

    UnitWeight weight82(ALNTANK, 1);
    weight_table_air_defense.PushBack(weight82);

    UnitWeight weight83(ALNASGUN, 1);
    weight_table_air_defense.PushBack(weight83);

    weight_table_aircrafts.Clear();

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight84(ANTIAIR, 3);
        weight_table_aircrafts.PushBack(weight84);
    }

    UnitWeight weight85(SP_FLAK, 3);
    weight_table_aircrafts.PushBack(weight85);

    UnitWeight weight86(FASTBOAT, 3);
    weight_table_aircrafts.PushBack(weight86);

    UnitWeight weight87(FIGHTER, 1);
    weight_table_aircrafts.PushBack(weight87);

    UnitWeight weight88(ALNPLANE, 1);
    weight_table_aircrafts.PushBack(weight88);

    weight_table_bomber.Clear();

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight89(ANTIAIR, 1);
        weight_table_bomber.PushBack(weight89);
    }

    UnitWeight weight90(SP_FLAK, 1);
    weight_table_bomber.PushBack(weight90);

    UnitWeight weight91(FASTBOAT, 1);
    weight_table_bomber.PushBack(weight91);

    UnitWeight weight92(FIGHTER, 3);
    weight_table_bomber.PushBack(weight92);

    UnitWeight weight93(ALNPLANE, 3);
    weight_table_bomber.PushBack(weight93);

    weight_table_9.Clear();

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight94(ANTIAIR, 1);
        weight_table_9.PushBack(weight94);
    }

    UnitWeight weight95(SP_FLAK, 1);
    weight_table_9.PushBack(weight95);

    UnitWeight weight96(FASTBOAT, 1);
    weight_table_9.PushBack(weight96);

    UnitWeight weight97(FIGHTER, 1);
    weight_table_9.PushBack(weight97);

    UnitWeight weight98(ALNPLANE, 1);
    weight_table_9.PushBack(weight98);

    weight_table_fastboat.Clear();

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight99(ARTYTRRT, 1);
        weight_table_fastboat.PushBack(weight99);

        UnitWeight weight100(ANTIMSSL, 1);
        weight_table_fastboat.PushBack(weight100);

        UnitWeight weight101(SUBMARNE, 1);
        weight_table_fastboat.PushBack(weight101);
    }

    UnitWeight weight102(ARTILLRY, 1);
    weight_table_fastboat.PushBack(weight102);

    UnitWeight weight103(ROCKTLCH, 1);
    weight_table_fastboat.PushBack(weight103);

    UnitWeight weight104(MISSLLCH, 1);
    weight_table_fastboat.PushBack(weight104);

    UnitWeight weight105(CORVETTE, 1);
    weight_table_fastboat.PushBack(weight105);

    UnitWeight weight106(BATTLSHP, 1);
    weight_table_fastboat.PushBack(weight106);

    UnitWeight weight107(MSSLBOAT, 1);
    weight_table_fastboat.PushBack(weight107);

    UnitWeight weight108(JUGGRNT, 1);
    weight_table_fastboat.PushBack(weight108);

    UnitWeight weight109(ALNTANK, 1);
    weight_table_fastboat.PushBack(weight109);

    UnitWeight weight110(ALNASGUN, 1);
    weight_table_fastboat.PushBack(weight110);

    weight_table_corvette.Clear();

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight111(ARTYTRRT, 1);
        weight_table_corvette.PushBack(weight111);

        UnitWeight weight112(ANTIMSSL, 1);
        weight_table_corvette.PushBack(weight112);
    }

    UnitWeight weight113(ARTILLRY, 1);
    weight_table_corvette.PushBack(weight113);

    UnitWeight weight114(ROCKTLCH, 1);
    weight_table_corvette.PushBack(weight114);

    UnitWeight weight115(MISSLLCH, 1);
    weight_table_corvette.PushBack(weight115);

    UnitWeight weight116(CORVETTE, 1);
    weight_table_corvette.PushBack(weight116);

    UnitWeight weight117(BATTLSHP, 2);
    weight_table_corvette.PushBack(weight117);

    UnitWeight weight118(MSSLBOAT, 2);
    weight_table_corvette.PushBack(weight118);

    UnitWeight weight119(BOMBER, 3);
    weight_table_corvette.PushBack(weight119);

    UnitWeight weight120(JUGGRNT, 1);
    weight_table_corvette.PushBack(weight120);

    UnitWeight weight121(ALNASGUN, 1);
    weight_table_corvette.PushBack(weight121);

    UnitWeight weight122(ALNPLANE, 1);
    weight_table_corvette.PushBack(weight122);

    weight_table_submarine.Clear();

    UnitWeight weight123(CORVETTE, 1);
    weight_table_submarine.PushBack(weight123);

    weight_table_battleships.Clear();

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight124(SUBMARNE, 3);
        weight_table_battleships.PushBack(weight124);

        UnitWeight weight125(ANTIMSSL, 1);
        weight_table_battleships.PushBack(weight125);
    }

    UnitWeight weight126(ARTILLRY, 1);
    weight_table_battleships.PushBack(weight126);

    UnitWeight weight127(ROCKTLCH, 1);
    weight_table_battleships.PushBack(weight127);

    UnitWeight weight128(MISSLLCH, 1);
    weight_table_battleships.PushBack(weight128);

    UnitWeight weight129(CORVETTE, 1);
    weight_table_battleships.PushBack(weight129);

    UnitWeight weight130(BATTLSHP, 2);
    weight_table_battleships.PushBack(weight130);

    UnitWeight weight131(MSSLBOAT, 1);
    weight_table_battleships.PushBack(weight131);

    UnitWeight weight132(BOMBER, 3);
    weight_table_battleships.PushBack(weight132);

    UnitWeight weight133(JUGGRNT, 1);
    weight_table_battleships.PushBack(weight133);

    UnitWeight weight134(ALNASGUN, 1);
    weight_table_battleships.PushBack(weight134);

    UnitWeight weight135(ALNPLANE, 1);
    weight_table_battleships.PushBack(weight135);

    weight_table_support_ships.Clear();

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        UnitWeight weight136(GUNTURRT, 1);
        weight_table_support_ships.PushBack(weight136);
    }

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight137(ARTYTRRT, 1);
        weight_table_support_ships.PushBack(weight137);

        UnitWeight weight138(ANTIMSSL, 1);
        weight_table_support_ships.PushBack(weight138);

        UnitWeight weight139(SUBMARNE, 1);
        weight_table_support_ships.PushBack(weight139);
    }

    UnitWeight weight140(SCOUT, 1);
    weight_table_support_ships.PushBack(weight140);

    UnitWeight weight141(TANK, 1);
    weight_table_support_ships.PushBack(weight141);

    UnitWeight weight142(ARTILLRY, 1);
    weight_table_support_ships.PushBack(weight142);

    UnitWeight weight143(ROCKTLCH, 1);
    weight_table_support_ships.PushBack(weight143);

    UnitWeight weight144(MISSLLCH, 1);
    weight_table_support_ships.PushBack(weight144);

    UnitWeight weight145(SP_FLAK, 1);
    weight_table_support_ships.PushBack(weight145);

    UnitWeight weight146(CORVETTE, 1);
    weight_table_support_ships.PushBack(weight146);

    UnitWeight weight147(BATTLSHP, 1);
    weight_table_support_ships.PushBack(weight147);

    UnitWeight weight148(MSSLBOAT, 1);
    weight_table_support_ships.PushBack(weight148);

    UnitWeight weight149(BOMBER, 1);
    weight_table_support_ships.PushBack(weight149);

    UnitWeight weight150(JUGGRNT, 1);
    weight_table_support_ships.PushBack(weight150);

    UnitWeight weight151(ALNTANK, 1);
    weight_table_support_ships.PushBack(weight151);

    UnitWeight weight152(ALNASGUN, 1);
    weight_table_support_ships.PushBack(weight152);

    UnitWeight weight153(ALNPLANE, 1);
    weight_table_support_ships.PushBack(weight153);

    weight_table_missileboat.Clear();

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight154(ANTIMSSL, 1);
        weight_table_missileboat.PushBack(weight154);
    }

    UnitWeight weight155(MISSLLCH, 1);
    weight_table_missileboat.PushBack(weight155);

    UnitWeight weight156(CORVETTE, 1);
    weight_table_missileboat.PushBack(weight156);

    UnitWeight weight157(BATTLSHP, 1);
    weight_table_missileboat.PushBack(weight157);

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight158(SUBMARNE, 3);
        weight_table_missileboat.PushBack(weight158);
    }

    UnitWeight weight159(MSSLBOAT, 1);
    weight_table_missileboat.PushBack(weight159);

    UnitWeight weight160(BOMBER, 3);
    weight_table_missileboat.PushBack(weight160);

    UnitWeight weight161(JUGGRNT, 1);
    weight_table_missileboat.PushBack(weight161);

    UnitWeight weight162(ALNPLANE, 1);
    weight_table_missileboat.PushBack(weight162);

    weight_table_commando.Clear();

    UnitWeight weight163(INFANTRY, 1);
    weight_table_commando.PushBack(weight163);

    weight_table_infantry.Clear();

    if (opponent >= OPPONENT_TYPE_AVERAGE) {
        UnitWeight weight164(ANTIMSSL, 1);
        weight_table_infantry.PushBack(weight164);
    }

    if (opponent >= OPPONENT_TYPE_APPRENTICE) {
        UnitWeight weight165(GUNTURRT, 1);
        weight_table_infantry.PushBack(weight165);
    }

    UnitWeight weight166(TANK, 1);
    weight_table_infantry.PushBack(weight166);

    UnitWeight weight167(ARTILLRY, 1);
    weight_table_infantry.PushBack(weight167);

    UnitWeight weight168(ROCKTLCH, 1);
    weight_table_infantry.PushBack(weight168);

    UnitWeight weight169(MISSLLCH, 1);
    weight_table_infantry.PushBack(weight169);

    UnitWeight weight170(SP_FLAK, 1);
    weight_table_infantry.PushBack(weight170);

    UnitWeight weight171(INFANTRY, 1);
    weight_table_infantry.PushBack(weight171);

    UnitWeight weight172(BATTLSHP, 1);
    weight_table_infantry.PushBack(weight172);

    UnitWeight weight173(MSSLBOAT, 1);
    weight_table_infantry.PushBack(weight173);

    UnitWeight weight174(BOMBER, 1);
    weight_table_infantry.PushBack(weight174);

    UnitWeight weight175(JUGGRNT, 1);
    weight_table_infantry.PushBack(weight175);

    UnitWeight weight176(ALNTANK, 1);
    weight_table_infantry.PushBack(weight176);

    UnitWeight weight177(ALNASGUN, 1);
    weight_table_infantry.PushBack(weight177);

    UnitWeight weight178(ALNPLANE, 1);
    weight_table_infantry.PushBack(weight178);

    weight_table_generic.Clear();

    if (opponent >= OPPONENT_TYPE_EXPERT) {
        UnitWeight weight179(COMMANDO, 1);
        weight_table_generic.PushBack(weight179);

        UnitWeight weight180(INFANTRY, 1);
        weight_table_generic.PushBack(weight180);
    }

    UnitWeight weight181(SCOUT, 1);
    weight_table_generic.PushBack(weight181);

    UnitWeight weight182(TANK, 1);
    weight_table_generic.PushBack(weight182);

    UnitWeight weight183(ARTILLRY, 1);
    weight_table_generic.PushBack(weight183);

    UnitWeight weight184(ROCKTLCH, 1);
    weight_table_generic.PushBack(weight184);

    UnitWeight weight185(MISSLLCH, 1);
    weight_table_generic.PushBack(weight185);

    UnitWeight weight186(BATTLSHP, 1);
    weight_table_generic.PushBack(weight186);

    UnitWeight weight187(MSSLBOAT, 1);
    weight_table_generic.PushBack(weight187);

    UnitWeight weight188(BOMBER, 1);
    weight_table_generic.PushBack(weight188);

    UnitWeight weight189(JUGGRNT, 1);
    weight_table_generic.PushBack(weight189);

    UnitWeight weight190(ALNTANK, 1);
    weight_table_generic.PushBack(weight190);

    UnitWeight weight191(ALNASGUN, 1);
    weight_table_generic.PushBack(weight191);

    UnitWeight weight192(ALNPLANE, 1);
    weight_table_generic.PushBack(weight192);
}

int AiPlayer::GetPredictedAttack(UnitInfo* unit, int caution_level) {
    return GetTotalProjectedDamage(unit, caution_level, player_team, &UnitsManager_MobileLandSeaUnits) +
           GetTotalProjectedDamage(unit, caution_level, player_team, &UnitsManager_MobileAirUnits);
}

short** AiPlayer::GetDamagePotentialMap(UnitInfo* unit, int caution_level, bool is_for_attacking) {
    short** result;
    ThreatMap* threat_map = GetThreatMap(ThreatMap::GetRiskLevel(unit), caution_level, is_for_attacking);

    if (threat_map) {
        threat_map->Update(unit->GetBaseValues()->GetAttribute(ATTRIB_ARMOR));

        result = threat_map->damage_potential_map;

    } else {
        result = nullptr;
    }

    return result;
}

short** AiPlayer::GetDamagePotentialMap(ResourceID unit_type, int caution_level, bool is_for_attacking) {
    short** result;
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

int AiPlayer::GetDamagePotential(UnitInfo* unit, Point site, int caution_level, bool is_for_attacking) {
    int result;

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
            task_clear_air_zone = new (std::nothrow) TaskClearZone(player_team, 0x40);

            TaskManager.AppendTask(*task_clear_air_zone);
        }

        task_clear_air_zone->AddZone(zone);

    } else {
        if (!task_clear_ground_zone) {
            task_clear_ground_zone = new (std::nothrow) TaskClearZone(player_team, 0x180);
            TaskManager.AppendTask(*task_clear_ground_zone);
        }

        task_clear_ground_zone->AddZone(zone);
    }
}

WeightTable AiPlayer::GetFilteredWeightTable(ResourceID unit_type, unsigned short flags) {
    WeightTable table(GetWeightTable(unit_type), true);

    if (flags & 0x01) {
        for (int i = 0; i < table.GetCount(); ++i) {
            if (table[i].unit_type != INVALID_ID && (UnitsManager_BaseUnits[table[i].unit_type].flags & STATIONARY)) {
                table[i].weight = 0;
            }
        }
    }

    if (flags & 0x02) {
        for (int i = 0; i < table.GetCount(); ++i) {
            if (table[i].unit_type != INVALID_ID &&
                (UnitsManager_BaseUnits[table[i].unit_type].flags & REGENERATING_UNIT)) {
                table[i].weight = 0;
            }
        }
    }

    return table;
}

void AiPlayer::SetInfoMapPoint(Point site) {
    if (info_map) {
        info_map[site.x][site.y] |= 0x01;
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

signed char AiPlayer::GetMineMapEntry(Point site) {
    signed char result;

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
        unsigned short team = unit->team;
        int attack =
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], LANDMINE)->GetAttribute(ATTRIB_ATTACK);

        mine_map[position.x][position.y] = (attack + mine_map[position.x][position.y]) / 2;

        if (!task_3) {
            task_3 = new (std::nothrow) TaskFindMines(player_team, position);

            TaskManager.AppendTask(*task_3);
        }
    }
}

WeightTable AiPlayer::GetExtendedWeightTable(UnitInfo* target, unsigned char flags) {
    WeightTable table = GetFilteredWeightTable(target->unit_type, flags);
    TeamUnits* team_units = UnitsManager_TeamInfo[player_team].team_units;
    Point position(target->grid_x, target->grid_y);

    if (target->team == PLAYER_TEAM_ALIEN && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_EXPERT) {
        table.Clear();

        UnitWeight unit_weight(COMMANDO, 1);

        table.PushBack(unit_weight);

    } else {
        if (target->team == PLAYER_TEAM_ALIEN) {
            table = GetFilteredWeightTable(ADUMP, flags);
        }

        if (target->flags & STATIONARY) {
            bool is_water_present = false;
            Rect bounds;

            target->GetBounds(&bounds);

            for (int x = bounds.ulx; x < bounds.lrx; ++x) {
                for (int y = bounds.uly; y < bounds.lry; ++y) {
                    if (ResourceManager_MapSurfaceMap[y * ResourceManager_MapSize.x + x] == SURFACE_TYPE_WATER) {
                        is_water_present = true;
                    }
                }
            }

            if (is_water_present) {
                UnitWeight unit_weight(CORVETTE, 1);

                table.PushBack(unit_weight);
            }

            if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
                UnitWeight unit_weight(SUBMARNE, 1);

                table.PushBack(unit_weight);
            }
        }

        for (int i = 0; i < table.GetCount(); ++i) {
            if (table[i].unit_type != INVALID_ID) {
                int surface_type = UnitsManager_BaseUnits[table[i].unit_type].land_type;
                int unit_range = team_units->GetCurrentUnitValues(table[i].unit_type)->GetAttribute(ATTRIB_RANGE);

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
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], unit->unit_type);

    if (base_values != current_values) {
        if (build_order.unit_type != unit->unit_type) {
            if (!(unit->flags & REGENERATING_UNIT)) {
                if (unit->unit_type != CONSTRCT && unit->unit_type != ENGINEER && unit->unit_type != BULLDOZR &&
                    unit->unit_type != MINELAYR) {
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
        for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it; ++it) {
            if (unit == (*it).GetUnit()) {
                spotted_units.Remove(*it);
            }
        }
    }
}

void AiPlayer::UnitSpotted(UnitInfo* unit) {
    if (unit->team != player_team &&
        (!(unit->flags & GROUND_COVER) || unit->unit_type == LANDMINE || unit->unit_type == SEAMINE)) {
        SmartList<SpottedUnit>::Iterator spotted_unit;

        InvalidateThreatMaps();

        for (spotted_unit = spotted_units.Begin(); spotted_unit; ++spotted_unit) {
            if ((*spotted_unit).GetUnit() == unit) {
                break;
            }
        }

        if (spotted_unit) {
            (*spotted_unit).UpdatePosition();

        } else {
            SmartPointer<SpottedUnit> spotted_unit2(new (std::nothrow) SpottedUnit(unit, player_team));

            spotted_units.PushBack(*spotted_unit2);

            if (unit->unit_type == LANDMINE || unit->unit_type == SEAMINE) {
                MineSpotted(unit);
            }

            bool is_key_facility = false;

            for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it; ++it) {
                if (IsKeyFacility((*it).GetUnit()->unit_type)) {
                    is_key_facility = true;
                    break;
                }
            }

            if ((IsKeyFacility(unit->unit_type) || (!is_key_facility && unit->unit_type == CONSTRCT)) &&
                unit->team == target_team) {
                DetermineAttack(&*spotted_unit2, 0x1F00);

            } else if (!is_key_facility && unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) > 0) {
                if (unit->team == target_team) {
                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                         it != UnitsManager_StationaryUnits.End(); ++it) {
                        if ((*it).team == unit->team && IsKeyFacility((*it).unit_type) &&
                            TaskManager_GetDistance(unit, &*it) < 30) {
                            spotted_unit2 = new (std::nothrow) SpottedUnit(&*it, player_team);

                            spotted_unit2->SetPosition(Point(unit->grid_x, unit->grid_y));

                            spotted_units.PushBack(*spotted_unit2);

                            DetermineAttack(&*spotted_unit2, 0x1F00);

                            return;
                        }
                    }

                    for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it; ++it) {
                        if ((*it).GetUnit()->unit_type == CONSTRCT) {
                            return;
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                        if ((*it).team == unit->team && (*it).unit_type == CONSTRCT &&
                            TaskManager_GetDistance(unit, &*it) < 30) {
                            spotted_unit2 = new (std::nothrow) SpottedUnit(&*it, player_team);

                            spotted_unit2->SetPosition(Point(unit->grid_x, unit->grid_y));

                            spotted_units.PushBack(*spotted_unit2);

                            DetermineAttack(&*spotted_unit2, 0x1F00);

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
    int minimum_distance = request->GetMinimumDistance();
    Point site = request->GetDestination();
    int distance = TaskManager_GetDistance(position, site) / 2;
    bool result;

    distance -= minimum_distance;

    if (distance >= unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) {
        int transport_category = TransportOrder::DetermineCategory(unit->unit_type);
        bool flag = request->GetField31();
        TransportOrder* transport_order = nullptr;
        Point source;
        Point destination;
        int minimum_distance2;

        for (SmartList<TransportOrder>::Iterator it = transport_orders.Begin(); it != transport_orders.End(); ++it) {
            if ((*it).GetTransportCategory() == transport_category) {
                int distance1;
                int distance2;

                source = (*it).MatchStartPosition(position);
                distance1 = TaskManager_GetDistance(position, source) / 2;
                destination = (*it).MatchClosestPosition(site, source);
                distance2 = TaskManager_GetDistance(site, destination) / 2;
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
    AccessMap access_map;
    Point site;
    int continent_size = 0;
    unsigned short strategy_scores[AI_STRATEGY_MAX];

    for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
        for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
            switch (Access_GetSurfaceType(site.x, site.y)) {
                case SURFACE_TYPE_LAND: {
                    access_map.GetMapColumn(site.x)[site.y] = 0x02;
                } break;

                case SURFACE_TYPE_WATER:
                case SURFACE_TYPE_COAST: {
                    access_map.GetMapColumn(site.x)[site.y] = 0x01;
                } break;

                default: {
                    access_map.GetMapColumn(site.x)[site.y] = 0x00;
                } break;
            }
        }
    }

    for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
        for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
            if (access_map.GetMapColumn(site.x)[site.y] == 0x02) {
                SmartPointer<Continent> continent(new (std::nothrow)
                                                      Continent(access_map.GetMap(), continents.GetCount() + 3, site));

                if (continent->IsViableContinent(false, player_team)) {
                    continents.Insert(&*continent);
                }
            }
        }
    }

    int opponent_class = ini_get_setting(INI_OPPONENT);

    for (int i = 0; i < AI_STRATEGY_MAX; ++i) {
        strategy_scores[i] = 0;
    }

    if (opponent_class >= OPPONENT_TYPE_AVERAGE) {
        int continent_score = 0;
        continent_size = 0;

        for (int i = 0; i < continents.GetCount(); ++i) {
            if (continents[i].GetContinentSize() > continent_size) {
                continent_size = continents[i].GetContinentSize();
            }
        }

        for (int i = 0; i < continents.GetCount(); ++i) {
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

    for (int i = 0; i < continents.GetCount(); ++i) {
        continents[i].TestIsolated();
    }

    for (int i = continents.GetCount() - 1; i >= 0; --i) {
        if (continents[i].IsViableContinent(true, player_team)) {
            if (continents[i].GetContinentSize() > continent_size) {
                continent_size = continents[i].GetContinentSize();
            }

        } else {
            continents.Erase(i);
        }
    }

    if (continent_size > 80) {
        for (int i = continents.GetCount() - 1; i >= 0; --i) {
            if (continents[i].GetContinentSize() < 80) {
                continents.Erase(i);
            }
        }
    }

    int isolated_continents = 0;
    int continents_in_close_proximity = 0;

    for (int i = 0; i < continents.GetCount(); ++i) {
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

        if (ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_STRATEGY + player_team), strategy_string,
                                      sizeof(strategy_string))) {
            for (int i = 0; i < AI_STRATEGY_MAX; ++i) {
                if (!stricmp(strategy_string, strategy_strings[i])) {
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
        int total_score = 0;
        int potential_strategies = 0;

        for (int i = 0; i < AI_STRATEGY_MAX; ++i) {
            if (strategy_scores[i] > 0) {
                ++potential_strategies;
            }
        }

        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER && team != player_team &&
                AiPlayer_Teams[team].GetStrategy() != AI_STRATEGY_RANDOM) {
                --potential_strategies;
            }
        }

        if (potential_strategies > 0) {
            for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER && team != player_team &&
                    AiPlayer_Teams[team].GetStrategy() != AI_STRATEGY_RANDOM) {
                    strategy_scores[AiPlayer_Teams[team].GetStrategy()] = 0;
                }
            }
        }

        for (int i = 0; i < AI_STRATEGY_MAX; ++i) {
            total_score += strategy_scores[i];

            if (strategy_scores[i] > 0) {
                ++potential_strategies;
            }
        }

        total_score = ((dos_rand() * total_score) >> 15) + 1;

        int index = -1;

        do {
            ++index;
            total_score -= strategy_scores[index];
        } while (total_score > 0);

        strategy = index;
    }

    switch (strategy) {
        case AI_STRATEGY_SEA: {
            for (int i = continents.GetCount() - 1; i >= 0; --i) {
                if (!continents[i].IsIsolated()) {
                    continents.Erase(i);
                }
            }
        } break;

        case AI_STRATEGY_FAST_ATTACK:
        case AI_STRATEGY_COMBINED_ARMS: {
            for (int i = continents.GetCount() - 1; i >= 0; --i) {
                if (continents[i].IsCloseProximity()) {
                    continents.Erase(i);
                }
            }
        } break;

        case AI_STRATEGY_AIR: {
            for (int i = continents.GetCount() - 1; i >= 0; --i) {
                if (!continents[i].IsCloseProximity()) {
                    continents.Erase(i);
                }
            }
        } break;
    }

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (continents.GetCount() > 1 && team != player_team &&
            UnitsManager_TeamMissionSupplies[team].units.GetCount() > 0 &&
            UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            unsigned char filler = access_map.GetMapColumn(
                UnitsManager_TeamMissionSupplies[team]
                    .starting_position.x)[UnitsManager_TeamMissionSupplies[team].starting_position.y];

            for (int i = continents.GetCount() - 1; i >= 0 && continents.GetCount() > 1; --i) {
                if (continents[i].GetFiller() == filler) {
                    continents.Erase(i);
                }
            }
        }
    }

    Point map_center(ResourceManager_MapSize.x / 2, ResourceManager_MapSize.y / 2);

    if (strategy == AI_STRATEGY_DEFENSIVE || strategy == AI_STRATEGY_AIR || strategy == AI_STRATEGY_SEA ||
        strategy == AI_STRATEGY_ESPIONAGE) {
        int maximum_distance = 0;
        int distance;

        for (int i = 0; i < continents.GetCount(); ++i) {
            Point continent_center = continents[i].GetCenter();

            distance = TaskManager_GetDistance(map_center, continent_center);

            if (distance > maximum_distance) {
                maximum_distance = distance;
            }
        }

        for (int i = continents.GetCount() - 1; i >= 0; --i) {
            Point continent_center = continents[i].GetCenter();

            distance = TaskManager_GetDistance(map_center, continent_center);

            if (distance < maximum_distance / 2) {
                continents.Erase(i);
            }
        }
    }

    if (strategy == AI_STRATEGY_SCOUT_HORDE) {
        int minimum_distance = ResourceManager_MapSize.x + ResourceManager_MapSize.y;
        int distance;

        for (int i = 0; i < continents.GetCount(); ++i) {
            Point continent_center = continents[i].GetCenter();

            distance = TaskManager_GetDistance(map_center, continent_center);

            if (distance < minimum_distance) {
                minimum_distance = distance;
            }
        }

        for (int i = continents.GetCount() - 1; i >= 0; --i) {
            Point continent_center = continents[i].GetCenter();

            distance = TaskManager_GetDistance(map_center, continent_center);

            if (distance > minimum_distance * 2) {
                continents.Erase(i);
            }
        }
    }

    int total_continent_size = 0;
    bool result;

    for (int i = 0; i < continents.GetCount(); ++i) {
        total_continent_size += continents[i].GetContinentSize();
    }

    if (total_continent_size) {
        total_continent_size = ((dos_rand() * total_continent_size) >> 15) + 1;

        int index = -1;

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
    unsigned short item_count;

    file.Write(player_team);
    file.Write(strategy);
    file.Write(field_3);
    file.Write(field_5);
    file.Write(field_7);
    file.Write(target_team);

    file.WriteObjectCount(spotted_units.GetCount());

    for (SmartList<SpottedUnit>::Iterator it = spotted_units.Begin(); it; ++it) {
        (*it).FileSave(file);
    }

    item_count = info_map ? 1 : 0;

    file.Write(item_count);

    if (item_count) {
        for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
            file.Write(info_map[x], ResourceManager_MapSize.y);
        }
    }

    item_count = mine_map ? 1 : 0;

    file.Write(item_count);

    if (item_count) {
        for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
            file.Write(mine_map[x], ResourceManager_MapSize.y);
        }
    }

    file.Write(target_location);
}

void AiPlayer::FileLoad(SmartFileReader& file) {
    unsigned short item_count;

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

    for (int i = 0; i < item_count; ++i) {
        SmartPointer<SpottedUnit> spotted_unit(new (std::nothrow) SpottedUnit(file));

        spotted_units.PushBack(*spotted_unit);
    }

    SDL_assert(info_map == nullptr);

    item_count = file.ReadObjectCount();

    if (item_count) {
        info_map = new (std::nothrow) unsigned char*[ResourceManager_MapSize.x];

        for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
            info_map[x] = new (std::nothrow) unsigned char[ResourceManager_MapSize.y];

            file.Read(info_map[x], ResourceManager_MapSize.y);

            for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                info_map[x][y] &= 0x03;
            }
        }
    }

    SDL_assert(mine_map == nullptr);

    item_count = file.ReadObjectCount();

    if (item_count) {
        mine_map = new (std::nothrow) signed char*[ResourceManager_MapSize.x];

        for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
            mine_map[x] = new (std::nothrow) signed char[ResourceManager_MapSize.y];

            file.Read(mine_map[x], ResourceManager_MapSize.y);
        }
    }

    file.Read(target_location);
}

void AiPlayer::ChooseUpgrade(SmartObjectArray<BuildOrder> build_orders1, SmartObjectArray<BuildOrder> build_orders2) {
    upgrade_cost = INT16_MAX;
    build_order.unit_type = INVALID_ID;

    for (int i = 0; i < build_orders1.GetCount(); ++i) {
        int new_upgrade_cost =
            TeamUnits_GetUpgradeCost(player_team, build_orders1[i]->unit_type, build_orders1[i]->primary_attribute);

        if (new_upgrade_cost < upgrade_cost) {
            build_order = *build_orders1[i];
            upgrade_cost = new_upgrade_cost;
        }
    }

    int factor = 2;

    for (int i = 0; i < build_orders2.GetCount(); ++i) {
        int new_upgrade_cost =
            TeamUnits_GetUpgradeCost(player_team, build_orders2[i]->unit_type, build_orders2[i]->primary_attribute);

        if (new_upgrade_cost * factor < upgrade_cost) {
            build_order = *build_orders2[i];
            upgrade_cost = new_upgrade_cost;

            factor = 1;
        }
    }
}

int AiPlayer_CalculateProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int caution_level) {
    UnitValues* unit_values = friendly_unit->GetBaseValues();
    int range = unit_values->GetAttribute(ATTRIB_RANGE);
    int distance;
    int shots;
    int result;

    if (!enemy_unit->IsVisibleToTeam(friendly_unit->team)) {
        range = std::min(range, unit_values->GetAttribute(ATTRIB_SCAN));
    }

    distance = Access_GetDistance(friendly_unit, enemy_unit);
    shots = 0;

    if (AiPlayer_TerrainMap.TerrainMap_sub_690D6(Point(enemy_unit->grid_x, enemy_unit->grid_y),
                                                 UnitsManager_BaseUnits[friendly_unit->unit_type].land_type) <=
        range * range) {
        if (unit_values->GetAttribute(ATTRIB_MOVE_AND_FIRE) && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
            int distance2 = ((friendly_unit->speed / 2) + range);

            if (distance <= distance2 * distance2) {
                shots = friendly_unit->shots;
            }

        } else {
            int base_rounds = unit_values->GetAttribute(ATTRIB_ROUNDS);
            int base_speed = unit_values->GetAttribute(ATTRIB_SPEED);

            for (shots = friendly_unit->shots; shots > 0; --shots) {
                int distance2 = friendly_unit->speed - ((base_speed * shots) / base_rounds) + range;

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

int AiPlayer_GetProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int caution_level) {
    int result;

    if (friendly_unit->shots > 0 && Task_IsReadyToTakeOrders(friendly_unit) &&
        Access_IsValidAttackTarget(friendly_unit, enemy_unit)) {
        if (caution_level != CAUTION_LEVEL_AVOID_REACTION_FIRE ||
            friendly_unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
            if (!friendly_unit->GetTask() || friendly_unit->GetTask()->DeterminePriority(0x400) > 0) {
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
