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

#ifndef TASKMANAGEBUILDINGS_HPP
#define TASKMANAGEBUILDINGS_HPP

#include "cargo.hpp"
#include "taskcreatebuilding.hpp"

class TaskManageBuildings : public Task {
    Point building_site;
    SmartList<UnitInfo> units;
    SmartList<TaskCreateBuilding> tasks;
    Cargo cargo_demand;

    struct Block {
        int8_t d1;
        int8_t d2;
        int8_t d3;
        int8_t d4;
    };

    void FillMap(uint16_t** construction_map, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t fill_value);
    uint16_t** CreateMap();
    void DeleteMap(uint16_t** construction_map);
    void MarkMiningAreas(uint16_t** construction_map);
    void MarkBuildingAreas(uint16_t** construction_map, int32_t area_expanse, int32_t area_offset);
    void ClearBuildingAreas(uint16_t** construction_map, TaskCreateBuilding* task);
    void ClearPathways(uint16_t** construction_map, Rect bounds, int32_t unit_size);
    void ClearPlannedBuildings(uint16_t** construction_map, TaskCreateBuilding* task, ResourceID unit_type,
                               uint16_t task_priority);
    static bool FillBlockMap(int32_t ul1, int32_t lr1, int32_t ul2, int32_t lr2, int8_t* address_a, int8_t* address_b);
    void ClearBlocks(uint16_t** construction_map, struct Block** block_map, Rect bounds, int32_t area_marker,
                     int32_t unit_size);
    void LimitBlockSize(uint16_t** construction_map, int32_t unit_size);
    int32_t EvaluateSiteValue(uint16_t** construction_map, Point site, ResourceID unit_type);
    bool IsFavorableMiningSite(Point site);
    bool IsViableMiningSite(uint16_t** construction_map, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
    bool IsSafeSite(uint16_t** construction_map, Point site, ResourceID unit_type);
    bool EvaluateSite(uint16_t** construction_map, ResourceID unit_type, Point& site);
    bool FindSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site, uint16_t task_priority);
    int32_t GetUnitCount(ResourceID unit_type, uint16_t task_priority);
    bool IsSupremeTeam(uint16_t team);
    int32_t GetHighestGreenHouseCount(uint16_t team);
    bool CreateBuildings(int32_t building_demand, ResourceID unit_type, uint16_t task_priority);
    bool PlanNextBuildJob();
    static void UpdateCargoDemand(int16_t* limit, int16_t* material, int32_t max_mining);
    void UpdateMiningNeeds();
    void MakeConnectors(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, Task* task);
    bool CheckNeeds();
    void ClearAreasNearBuildings(uint8_t** access_map, int32_t area_expanse, TaskCreateBuilding* task);
    void EvaluateDangers(uint8_t** access_map);
    void MarkDefenseSites(uint16_t** construction_map, uint8_t** access_map, TaskCreateBuilding* task, int32_t value);
    void ClearDefenseSites(uint8_t** access_map, ResourceID unit_type, TaskCreateBuilding* task,
                           uint16_t task_priority);
    bool IsSiteWithinRadarRange(Point site, int32_t unit_range, TaskCreateBuilding* task);
    void UpdateAccessMap(uint8_t** access_map, TaskCreateBuilding* task);
    bool EvaluateNeedForRadar(uint8_t** access_map, TaskCreateBuilding* task);
    bool MarkBuildings(uint8_t** access_map, Point& site);
    void MarkConnections(uint8_t** access_map, Point site, int32_t value);
    void UpdateConnectors(uint8_t** access_map, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
    int32_t GetConnectionDistance(uint8_t** access_map, Point& site1, Point site2, uint16_t team, int32_t value);
    bool ConnectBuilding(uint8_t** access_map, Point site, int32_t value);
    bool ReconnectBuilding(uint8_t** access_map, Rect* bounds, int32_t value);
    static bool FindMarkedSite(uint8_t** access_map, Rect* bounds);

public:
    TaskManageBuildings(uint16_t team, Point site);
    ~TaskManageBuildings();

    char* WriteStatusLog(char* buffer) const;
    uint8_t GetType() const;
    bool IsNeeded();
    void AddUnit(UnitInfo& unit);
    void Init();
    void BeginTurn();
    void ChildComplete(Task* task);
    void EndTurn();
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);
    void EventUnitDestroyed(UnitInfo& unit);

    void AddCreateOrder(TaskCreateBuilding* task);
    bool CheckPower();
    void CheckWorkers();
    bool FindDefenseSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site, int32_t value,
                         uint16_t task_priority);
    bool FindSiteForRadar(TaskCreateBuilding* task, Point& site);
    bool ChangeSite(TaskCreateBuilding* task, Point& site);
    void BuildBridge(Point site, Task* task);
    bool CreateBuilding(ResourceID unit_type, Task* task, uint16_t task_priority);

    bool ReconnectBuildings();
};

#endif /* TASKMANAGEBUILDINGS_HPP */
