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
    uint8_t primary_attribute;
    ResourceID unit_type;

    BuildOrder() : primary_attribute(0), unit_type(INVALID_ID) {}
    BuildOrder(uint8_t primary_attribute, ResourceID unit_type)
        : primary_attribute(primary_attribute), unit_type(unit_type) {}
};

class AiPlayer {
    static constexpr uint8_t AttackTaskLimit{3};

    uint16_t player_team;
    uint8_t strategy;
    int16_t field_3;
    int16_t field_5;
    int16_t field_7;
    BuildOrder build_order;
    int16_t upgrade_cost;
    bool need_init;
    bool tasks_pending;
    uint8_t field_16;

    SmartPointer<TaskClearZone> task_clear_ground_zone;
    SmartPointer<TaskClearZone> task_clear_air_zone;

    SmartList<Task> task_list;

    SmartPointer<Task> check_assaults_task;
    SmartPointer<Task> defense_reserve_task;
    SmartPointer<Task> find_mines_task;
    SmartPointer<Task> place_mines_task;
    SmartPointer<TaskAttackReserve> attack_reserve_task;
    SmartPointer<Task> attack_tasks[AttackTaskLimit];

    SmartList<SpottedUnit> spotted_units;

    SmartList<UnitInfo> air_force;
    SmartList<UnitInfo> ground_forces;

    int16_t dimension_x;

    uint8_t** info_map;
    int8_t** mine_map;

    uint8_t field_107[2];

    SmartList<TransportOrder> transport_orders;

    Point target_location;
    int16_t target_team;

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
    void RebuildWeightTable(WeightTable table, ResourceID unit_type, int32_t factor);
    void RebuildWeightTables(ResourceID unit_type, int32_t factor);
    void UpdateWeightTables();
    static void MoveFinishedCallback(Task* task, UnitInfo* unit, char result);
    Point GetUnitClusterCoordinates(uint16_t team, SmartList<UnitInfo>* units);
    Point GetTeamClusterCoordinates(uint16_t team);
    void DetermineAttack(SpottedUnit* spotted_unit, uint16_t task_flags);
    void UpdatePriorityTasks();
    void DetermineTargetLocation(Point position);
    bool IsKeyFacility(ResourceID unit_type);
    void DetermineResearchProjects();
    bool IsPotentialSpotter(uint16_t team, UnitInfo* unit);
    void UpdateAccessMap(Point point1, Point point2, uint8_t** access_map);
    bool CheckAttacks();
    void RegisterReadyAndAbleUnits(SmartList<UnitInfo>* units);
    bool AreActionsPending();
    bool IsDemoMode();
    void RegisterIdleUnits();
    static int32_t GetTotalProjectedDamage(UnitInfo* unit, int32_t caution_level, uint16_t team,
                                           SmartList<UnitInfo>* units);
    static void UpdateMap(int16_t** map, Point position, int32_t range, int32_t damage_potential, bool normalize);
    static void UpdateThreatMaps(ThreatMap* threat_map, UnitInfo* unit, Point position, int32_t range, int32_t attack,
                                 int32_t shots, int32_t& ammo, bool normalize);
    void InvalidateThreatMaps();
    static void DetermineDefenses(SmartList<UnitInfo>* units, int16_t** map);
    static void DetermineThreats(UnitInfo* unit, Point position, int32_t caution_level, bool* teams,
                                 ThreatMap* air_force_map, ThreatMap* ground_forces_map);
    static void SumUpMaps(int16_t** map1, int16_t** map2);
    static void NormalizeThreatMap(ThreatMap* threat_map);
    static bool IsAbleToAttack(UnitInfo* attacker, ResourceID target_type, uint16_t team);
    ThreatMap* GetThreatMap(int32_t risk_level, int32_t caution_level, bool is_for_attacking);
    WeightTable GetWeightTable(ResourceID unit_type);
    void AddThreatToMineMap(int32_t grid_x, int32_t grid_y, int32_t range, int32_t damage_potential, int32_t factor);
    void MineSpotted(UnitInfo* unit);
    static bool IsSurfaceTypePresent(Point site, int32_t range, int32_t surface_type);
    int32_t SelectTeamClan();
    void RollField3();
    void RollField5();
    void RollField7();
    bool AddUnitToTeamMissionSupplies(ResourceID unit_type, uint16_t supplies);
    static int32_t GetVictoryConditionsFactor();
    void RollTeamMissionSupplies(int32_t clan);
    static void AddBuildOrder(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type, int32_t attribute);
    static void CheckReconnaissanceNeeds(SmartObjectArray<BuildOrder>* build_orders, ResourceID unit_type,
                                         uint16_t team, uint16_t enemy_team, bool mode);
    SmartObjectArray<BuildOrder> ChooseStrategicBuildOrders(bool mode);
    void ProcessBuildOrders(SmartObjectArray<BuildOrder> build_orders);
    SmartObjectArray<BuildOrder> ChooseGenericBuildOrders();
    void ChooseInitialUpgrades(int32_t team_gold);
    void UpgradeUnitType();

public:
    AiPlayer();
    ~AiPlayer();

    int32_t GetStrategy() const;
    int16_t GetTargetTeam() const;
    uint8_t** GetInfoMap();
    Point GetTargetLocation() const;
    uint16_t GetField5() const;
    int8_t** GetMineMap();
    void AddTransportOrder(TransportOrder* transport_order);
    Task* FindManager(Point site);
    SmartList<SpottedUnit>& GetSpottedUnits();
    void AddMilitaryUnit(UnitInfo* unit);
    bool IsTargetTeamDefined() const;
    bool IsTargetTeam(const uint8_t team) const;
    void DetermineTargetTeam();
    void BeginTurn();
    void GuessEnemyAttackDirections();
    void PlanMinefields();
    void ChangeTasksPendingFlag(bool value);
    bool CheckEndTurn();
    bool CreateBuilding(ResourceID unit_type, Point position, Task* task);
    void Init(uint16_t team);
    int32_t GetPredictedAttack(UnitInfo* unit, int32_t caution_level);
    int16_t** GetDamagePotentialMap(UnitInfo* unit, int32_t caution_level, bool is_for_attacking);
    int16_t** GetDamagePotentialMap(ResourceID unit_type, int32_t caution_level, bool is_for_attacking);
    int32_t GetDamagePotential(UnitInfo* unit, Point site, int32_t caution_level, bool is_for_attacking);
    void ClearZone(Zone* zone);
    WeightTable GetFilteredWeightTable(ResourceID unit_type, uint16_t flags);
    void SetInfoMapPoint(Point site);
    void UpdateMineMap(Point site);
    void MarkMineMapPoint(Point site);
    int8_t GetMineMapEntry(Point site);
    void FindMines(UnitInfo* unit);
    WeightTable GetExtendedWeightTable(UnitInfo* target, uint8_t flags);
    bool ShouldUpgradeUnit(UnitInfo* unit);
    void RemoveUnit(UnitInfo* unit);
    void UnitSpotted(UnitInfo* unit);
    bool MatchPath(TaskPathRequest* request);
    bool SelectStrategy();
    void FileSave(SmartFileWriter& file);
    void FileLoad(SmartFileReader& file);
    void ChooseUpgrade(SmartObjectArray<BuildOrder> build_orders1, SmartObjectArray<BuildOrder> build_orders2);
};

int32_t AiPlayer_CalculateProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int32_t caution_level);
int32_t AiPlayer_GetProjectedDamage(UnitInfo* friendly_unit, UnitInfo* enemy_unit, int32_t caution_level);

extern AiPlayer AiPlayer_Teams[PLAYER_TEAM_MAX - 1];
extern TerrainMap AiPlayer_TerrainMap;

#endif /* AI_PLAYER_HPP */
