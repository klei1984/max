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

#ifndef AI_PLAYER_HPP
#define AI_PLAYER_HPP

#include "spottedunit.hpp"
#include "taskattackreserve.hpp"
#include "taskclearzone.hpp"
#include "taskpathrequest.hpp"
#include "terrainmap.hpp"
#include "threatmap.hpp"
#include "transportorder.hpp"
#include "unitinfo.hpp"
#include "weighttable.hpp"

struct BuildOrder {
    unsigned char primary_attribute;
    ResourceID unit_type;

    BuildOrder() : primary_attribute(0), unit_type(INVALID_ID) {}
    BuildOrder(unsigned char primary_attribute, ResourceID unit_type)
        : primary_attribute(primary_attribute), unit_type(unit_type) {}
};

class AiPlayer {
    unsigned short player_team;
    unsigned char strategy;
    signed short field_3;
    signed short field_5;
    signed short field_7;
    BuildOrder build_order;
    signed short upgrade_cost;
    bool need_init;
    bool tasks_pending;
    unsigned char field_16;

    SmartPointer<TaskClearZone> task_clear_ground_zone;
    SmartPointer<TaskClearZone> task_clear_air_zone;

    SmartPointer<Task> task_1;
    SmartPointer<Task> defense_reserve_task;
    SmartPointer<Task> task_3;
    SmartPointer<Task> task_4;

    SmartList<Task> task_list;

    SmartPointer<TaskAttackReserve> attack_reserve_task;

    SmartPointer<Task> task_array[3];

    SmartList<SpottedUnit> spotted_units;

    SmartList<UnitInfo> unitinfo_list1;
    SmartList<UnitInfo> unitinfo_list2;

    short dimension_x;

    unsigned char** info_map;
    signed char** mine_map;

    unsigned char field_107[2];

    SmartList<TransportOrder> transport_orders;

    Point target_location;
    signed short target_team;

    WeightTable weight_table_ground_defense;
    WeightTable weight_table_scout;
    WeightTable weight_table_tanks;
    WeightTable weight_table_assault_guns;
    WeightTable weight_table_missile_launcher;
    WeightTable weight_table_air_defense;
    WeightTable weight_table_aircrafts;
    WeightTable weight_table_bomber;
    WeightTable weight_table_9;
    WeightTable weight_table_fastboat;
    WeightTable weight_table_corvette;
    WeightTable weight_table_submarine;
    WeightTable weight_table_battleships;
    WeightTable weight_table_missileboat;
    WeightTable weight_table_support_ships;
    WeightTable weight_table_commando;
    WeightTable weight_table_infantry;
    WeightTable weight_table_generic;

    void AddBuilding(UnitInfo* unit);
    void RebuildWeightTable(WeightTable table, ResourceID unit_type, int factor);
    void RebuildWeightTables(ResourceID unit_type, int factor);
    void UpdateWeightTables();
    static void MoveFinishedCallback(Task* task, UnitInfo* unit, char result);
    Point GetUnitClusterCoordinates(unsigned short team, SmartList<UnitInfo>* units);
    Point GetTeamClusterCoordinates(unsigned short team);
    void DetermineAttack(SpottedUnit* spotted_unit, unsigned short task_flags);
    void UpdatePriorityTasks();
    void DetermineTargetLocation(Point position);
    bool IsKeyFacility(ResourceID unit_type);
    void DetermineResearchProjects();
    bool IsPotentialSpotter(unsigned short team, UnitInfo* unit);
    void UpdateAccessMap(Point point1, Point point2, unsigned char** access_map);
    bool CheckAttacks();
    void RegisterReadyAndAbleUnits(SmartList<UnitInfo>* units);
    bool AreActionsPending();
    bool IsDemoMode();
    void RegisterIdleUnits();
    static int GetTotalProjectedDamage(UnitInfo* unit, int caution_level, unsigned short team,
                                       SmartList<UnitInfo>* units);
    static void UpdateMap(short** map, Point position, int range, int damage_potential, bool normalize);
    static void UpdateThreatMaps(ThreatMap* threat_map, UnitInfo* unit, Point position, int range, int attack,
                                 int shots, int* ammo, bool normalize);
    static void DetermineThreats(UnitInfo* unit, Point position, int caution_level, bool* teams, ThreatMap* threat_map1,
                                 ThreatMap* threat_map2);
    static void SumUpMaps(short** map1, short** map2);
    static void NormalizeThreatMap(ThreatMap* threat_map);
    static bool IsAbleToAttack(UnitInfo* unit, ResourceID unit_type, unsigned short team);
    ThreatMap* GetThreatMap(int risk_level, int caution_level, unsigned char flags);
    WeightTable GetWeightTable(ResourceID unit_type);
    void AddThreatToMineMap(int grid_x, int grid_y, int range, int damage_potential, int factor);
    void MineSpotted(UnitInfo* unit);
    static bool IsSurfaceTypePresent(Point site, int range, int surface_type);
    int SelectTeamClan();
    void RollField3();
    void RollField5();
    void RollField7();
    bool AddUnitToTeamMissionSupplies(ResourceID unit_type, unsigned short supplies);
    static int GetVictoryConditionsFactor();
    void RollTeamMissionSupplies(int clan);
    static void AddBuildOrder(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type, int attribute);
    static void CheckReconnaissanceNeeds(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type,
                                         unsigned short team, unsigned short enemy_team, bool mode);
    SmartObjectArray<BuildOrder> ChooseStrategicBuildOrders(bool mode);
    void ProcessBuildOrders(SmartObjectArray<BuildOrder> build_orders);
    SmartObjectArray<BuildOrder> ChooseGenericBuildOrders();
    void ChooseInitialUpgrades(int team_gold);
    void UpgradeUnitType();

public:
    AiPlayer();
    ~AiPlayer();

    int GetStrategy() const;
    unsigned short GetTargetTeam() const;
    unsigned char** GetInfoMap();
    Point GetTargetLocation() const;
    unsigned short GetField5() const;
    signed char** GetMineMap();
    void AddTransportOrder(TransportOrder* transport_order);
    Task* FindManager(Point site);
    SmartList<SpottedUnit>::Iterator GetSpottedUnitIterator();
    int GetMemoryUsage();
    void AddThreat(UnitInfo* unit);
    void BeginTurn();
    void GuessEnemyAttackDirections();
    void PlanMinefields();
    void ChangeTasksPendingFlag(bool value);
    bool CheckEndTurn();
    bool CreateBuilding(ResourceID unit_type, Point position, Task* task);
    void Init(unsigned short team);
    int GetPredictedAttack(UnitInfo* unit, int caution_level);
    short** GetDamagePotentialMap(UnitInfo* unit, int caution_level, unsigned char flags);
    short** GetDamagePotentialMap(ResourceID unit_type, int caution_level, unsigned char flags);
    int GetDamagePotential(UnitInfo* unit, Point site, int caution_level, unsigned char flags);
    void ClearZone(Zone* zone);
    WeightTable GetFilteredWeightTable(ResourceID unit_type, unsigned short flags);
    void SetInfoMapPoint(Point site);
    void UpdateMineMap(Point site);
    void MarkMineMapPoint(Point site);
    signed char GetMineMapEntry(Point site);
    void FindMines(UnitInfo* unit);
    WeightTable GetExtendedWeightTable(UnitInfo* target, unsigned char flags);
    bool IsUpgradeNeeded(UnitInfo* unit);
    void RemoveUnit(UnitInfo* unit);
    void UnitSpotted(UnitInfo* unit);
    bool MatchPath(TaskPathRequest* request);
    bool SelectStrategy();
    void FileSave(SmartFileWriter& file);
    void FileLoad(SmartFileReader& file);
    void ChooseUpgrade(SmartObjectArray<BuildOrder> build_orders1, SmartObjectArray<BuildOrder> build_orders2);
};

int AiPlayer_CalculateProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int caution_level);
int AiPlayer_GetProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int caution_level);

extern AiPlayer AiPlayer_Teams[PLAYER_TEAM_MAX - 1];
extern TerrainMap AiPlayer_TerrainMap;
extern ThreatMap AiPlayer_ThreatMaps[10];

#endif /* AI_PLAYER_HPP */
