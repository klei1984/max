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
#include "inifile.hpp"
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
#include "taskscavenge.hpp"
#include "tasksurvey.hpp"
#include "taskupdateterrain.hpp"
#include "units_manager.hpp"
#include "zonewalker.hpp"

AiPlayer AiPlayer_Teams[4];
TerrainMap AiPlayer_TerrainMap;
ThreatMap AiPlayer_ThreatMaps[10];

void AiPlayer::AddBuilding(UnitInfo* unit) { FindManager(Point(unit->grid_x, unit->grid_y))->AddUnit(*unit); }

void AiPlayer::RebuildWeightTable(WeightTable table, ResourceID unit_type, int factor) {
    for (int i = 0; i < table.GetCount(); ++i) {
        if (table[i].unit_type == unit_type) {
            table[i].weight *= factor;
        }
    }
}

void AiPlayer::RebuildWeightTables(ResourceID unit_type, int factor) {
    RebuildWeightTable(weight_table_1, unit_type, factor);
    RebuildWeightTable(weight_table_2, unit_type, factor);
    RebuildWeightTable(weight_table_3, unit_type, factor);
    RebuildWeightTable(weight_table_4, unit_type, factor);
    RebuildWeightTable(weight_table_milliselauncher, unit_type, factor);
    RebuildWeightTable(weight_table_6, unit_type, factor);
    RebuildWeightTable(weight_table_fighter_alienplane_air_transport_awac, unit_type, factor);
    RebuildWeightTable(weight_table_8, unit_type, factor);
    RebuildWeightTable(weight_table_9, unit_type, factor);
    RebuildWeightTable(weight_table_fastboat, unit_type, factor);
    RebuildWeightTable(weight_table_corvette, unit_type, factor);
    RebuildWeightTable(weight_table_submarine, unit_type, factor);
    RebuildWeightTable(weight_table_battleship, unit_type, factor);
    RebuildWeightTable(weight_table_missileboat, unit_type, factor);
    RebuildWeightTable(weight_table_seatransport_cargoshop, unit_type, factor);
    RebuildWeightTable(weight_table_16, unit_type, factor);
    RebuildWeightTable(weight_table_17, unit_type, factor);
    RebuildWeightTable(weight_table_18, unit_type, factor);
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
            weight_table_3.PushBack(weight1);

            UnitWeight weight2(SCOUT, 2);
            weight_table_4.PushBack(weight2);

            UnitWeight weight3(SCOUT, 1);
            weight_table_1.PushBack(weight3);

            UnitWeight weight4(SCOUT, 1);
            weight_table_milliselauncher.PushBack(weight4);

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

void AiPlayer::UpdateAccessMap(Point point1, Point point2, unsigned char** access_map) {}

bool AiPlayer::CheckAttacks() {}

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
        }
    }

    return true;
}

void AiPlayer::RegisterIdleUnits() {}

int AiPlayer::GetProjectedDamage(UnitInfo* friendly, UnitInfo* enemy, int caution_level) {}

int AiPlayer::GetTotalProjectedDamage(UnitInfo* unit, int caution_level, unsigned short team,
                                      SmartList<UnitInfo>* units) {
    int result = 0;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team) {
            result += GetProjectedDamage(&*it, unit, caution_level);
        }
    }

    return result;
}

void AiPlayer::UpdateMap(short** map, Point position, int range, int damage_potential, bool normalize) {}

void AiPlayer::UpdateThreatMaps(ThreatMap* threat_map, UnitInfo* unit, Point position, int range, int attack, int shots,
                                int* ammo, bool normalize) {}

void AiPlayer::DetermineThreats(UnitInfo* unit, Point position, int caution_level, bool* teams, ThreatMap* threat_map1,
                                ThreatMap* threat_map2) {}

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

bool AiPlayer::IsAbleToAttack(UnitInfo* unit, ResourceID unit_type, unsigned short team) {}

ThreatMap* AiPlayer::GetThreatMap(int risk_level, int caution_level, unsigned char flags) {}

WeightTable AiPlayer::GetWeightTable(ResourceID unit_type) {}

void AiPlayer::AddThreatToMineMap(int grid_x, int grid_y, int range, int damage_potential, int factor) {}

void AiPlayer::MineSpotted(UnitInfo* unit) {}

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

bool AiPlayer::AddUnitToTeamMissionSupplies(ResourceID unit_type, int supplies) {}

int AiPlayer::GetVictoryConditionsFactor() {}

void AiPlayer::RollTeamMissionSupplies(int clan) {}

void AiPlayer::AddBuildOrder(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type, int attribute) {}

void AiPlayer::CheckReconnaissanceNeeds(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type,
                                        unsigned short team, unsigned short target_team, bool mode) {}

SmartObjectArray<BuildOrder> AiPlayer::ChooseStrategicBuildOrders(bool mode) {}

void AiPlayer::ProcessBuildOrders(SmartObjectArray<BuildOrder> build_orders) {}

SmartObjectArray<BuildOrder> AiPlayer::ChooseGenericBuildOrders() {}

void AiPlayer::ChooseInitialUpgrades(int team_gold) {}

void AiPlayer::UpgradeUnitType() {}

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

unsigned short AiPlayer::GetTargetTeam() const { return target_team; }

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

void AiPlayer::AddThreat(UnitInfo* unit) {
    if (Access_IsValidAttackTargetType(unit->unit_type, FIGHTER)) {
        unitinfo_list1.PushBack(*unit);

    } else {
        unitinfo_list2.PushBack(*unit);
    }

    if (unit->shots > 0 && !need_init) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindAttack(*unit), true);
    }
}

void AiPlayer::BeginTurn() {
    if (UnitsManager_TeamInfo[player_team].team_type == TEAM_TYPE_COMPUTER) {
        for (int i = 0; i < 10; ++i) {
            AiPlayer_ThreatMaps[i].SetRiskLevel(0);
        }

        field_16 = 0;

        transport_orders.Clear();

        if (GameManager_PlayMode == PLAY_MODE_TURN_BASED) {
            const char* team_colors[PLAYER_TEAM_MAX - 1] = {"Red", "Green", "Blue", "Gray"};
            char message[80];

            sprintf(message, "%s Computer Turn.", team_colors[player_team]);

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
                }

                if ((*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0) {
                    AddThreat(&*it);
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
                        AddThreat(&*it);
                    }
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                 it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == player_team && (*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0) {
                    AddThreat(&*it);
                }
            }

            if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_EXPERT) {
                for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
                     it != UnitsManager_StationaryUnits.End(); ++it) {
                    if ((*it).team != player_team && ((*it).unit_type == MININGST || (*it).unit_type == GREENHSE)) {
                        Ai_ProcessUnitTasks(&*it, player_team);
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

        while (build_order.unit_type != INVALID_ID && field_12 <= team_gold) {
            team_gold -= field_12;
            UnitsManager_TeamInfo[player_team].team_units->SetGold(team_gold);
            UnitsManager_TeamInfo[player_team].stats_gold_spent_on_upgrades += field_12;

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

void AiPlayer::GuessEnemyAttackDirections() {}

void AiPlayer::PlanMinefields() {}

void AiPlayer::ChangeTasksPendingFlag(bool value) { tasks_pending = value; }

bool AiPlayer::CheckEndTurn() {}

bool AiPlayer::CreateBuilding(ResourceID unit_type, Point position, Task* task) {
    return dynamic_cast<TaskManageBuildings*>(FindManager(position))->CreateBuilding(unit_type, task, task->GetFlags());
}

void AiPlayer::Init(unsigned short team) {}

int AiPlayer::GetPredictedAttack(UnitInfo* unit, int caution_level) {
    return GetTotalProjectedDamage(unit, caution_level, player_team, &UnitsManager_MobileLandSeaUnits) +
           GetTotalProjectedDamage(unit, caution_level, player_team, &UnitsManager_MobileAirUnits);
}

short** AiPlayer::GetDamagePotentialMap(UnitInfo* unit, int caution_level, unsigned char flags) {
    short** result;
    ThreatMap* threat_map = GetThreatMap(ThreatMap::GetRiskLevel(unit), caution_level, flags);

    if (threat_map) {
        threat_map->Update(unit->GetBaseValues()->GetAttribute(ATTRIB_ARMOR));

        result = threat_map->damage_potential_map;

    } else {
        result = nullptr;
    }

    return result;
}

short** AiPlayer::GetDamagePotentialMap(ResourceID unit_type, int caution_level, unsigned char flags) {
    short** result;
    ThreatMap* threat_map = GetThreatMap(ThreatMap::GetRiskLevel(unit_type), caution_level, flags);

    if (threat_map) {
        threat_map->Update(UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[player_team], unit_type)
                               ->GetAttribute(ATTRIB_ARMOR));

        result = threat_map->damage_potential_map;

    } else {
        result = nullptr;
    }

    return result;
}

int AiPlayer::GetDamagePotential(UnitInfo* unit, Point site, int caution_level, unsigned char flags) {
    int result;

    if (caution_level == CAUTION_LEVEL_NONE) {
        result = 0;

    } else {
        ThreatMap* threat_map = GetThreatMap(ThreatMap::GetRiskLevel(unit), caution_level, flags);

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

WeightTable AiPlayer::GetFilteredWeightTable(ResourceID unit_type, unsigned char flags) {}

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

void AiPlayer::FindMines(UnitInfo* unit) {}

WeightTable AiPlayer::GetExtendedWeightTable(UnitInfo* target, unsigned char flags) {}

bool AiPlayer::IsUpgradeNeeded(UnitInfo* unit) {}

void AiPlayer::RemoveUnit(UnitInfo* unit) {}

void AiPlayer::UnitSpotted(UnitInfo* unit) {}

bool AiPlayer::MatchPath(TaskPathRequest* request) {}

bool AiPlayer::SelectStrategy() {}

void AiPlayer::FileSave(SmartFileWriter& file) {}

void AiPlayer::FileLoad(SmartFileReader& file) {}

void AiPlayer::ChooseUpgrade(SmartObjectArray<BuildOrder> build_orders1, SmartObjectArray<BuildOrder> build_orders2) {}

int AiPlayer_CalculateProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int caution_level) {}

int AiPlayer_GetProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int caution_level) {}
