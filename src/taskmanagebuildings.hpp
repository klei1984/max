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
        signed char d1;
        signed char d2;
        signed char d3;
        signed char d4;
    };

    void FillMap(unsigned short** construction_map, int ulx, int uly, int lrx, int lry, int fill_value);
    unsigned short** CreateMap();
    void DeleteMap(unsigned short** construction_map);
    void MarkMiningAreas(unsigned short** construction_map);
    void MarkBuildingAreas(unsigned short** construction_map, int area_expanse, int area_offset);
    void ClearBuildingAreas(unsigned short** construction_map, TaskCreateBuilding* task);
    void ClearPathways(unsigned short** construction_map, Rect bounds, int unit_size);
    void ClearPlannedBuildings(unsigned short** construction_map, TaskCreateBuilding* task, ResourceID unit_type,
                               unsigned short task_flags);
    static bool FillBlockMap(int ul1, int lr1, int ul2, int lr2, signed char* address_a, signed char* address_b);
    void ClearBlocks(unsigned short** construction_map, struct Block** block_map, Rect bounds, int area_marker,
                     int unit_size);
    void LimitBlockSize(unsigned short** construction_map, int unit_size);
    int EvaluateSiteValue(unsigned short** construction_map, Point site, ResourceID unit_type);
    bool IsFavorableMiningSite(Point site);
    bool IsViableMiningSite(unsigned short** construction_map, int ulx, int uly, int lrx, int lry);
    bool IsSafeSite(unsigned short** construction_map, Point site, ResourceID unit_type);
    bool EvaluateSite(unsigned short** construction_map, ResourceID unit_type, Point& site);
    bool FindSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site, unsigned short task_flags);
    int GetUnitCount(ResourceID unit_type, unsigned short task_flags);
    bool IsSupremeTeam(unsigned short team);
    int GetHighestGreenHouseCount(unsigned short team);
    bool CreateBuildings(int building_demand, ResourceID unit_type, unsigned short task_flags);
    bool PlanNextBuildJob();
    static void UpdateCargoDemand(short* limit, short* material, int max_mining);
    void UpdateMiningNeeds();
    void MakeConnectors(int ulx, int uly, int lrx, int lry, Task* task);
    bool CheckNeeds();
    void ClearAreasNearBuildings(unsigned char** access_map, int area_expanse, TaskCreateBuilding* task);
    void EvaluateDangers(unsigned char** access_map);
    void MarkDefenseSites(unsigned short** construction_map, unsigned char** access_map, TaskCreateBuilding* task,
                          int value);
    void ClearDefenseSites(unsigned char** access_map, ResourceID unit_type, TaskCreateBuilding* task,
                           unsigned short task_flags);
    bool IsSiteWithinRadarRange(Point site, int unit_range, TaskCreateBuilding* task);
    void UpdateAccessMap(unsigned char** access_map, TaskCreateBuilding* task);
    bool EvaluateNeedForRadar(unsigned char** access_map, TaskCreateBuilding* task);
    bool MarkBuildings(unsigned char** access_map, Point& site);
    void MarkConnections(unsigned char** access_map, Point site, int value);
    void UpdateConnectors(unsigned char** access_map, int ulx, int uly, int lrx, int lry);
    int GetConnectionDistance(unsigned char** access_map, Point& site1, Point site2, unsigned short team, int value);
    bool ConnectBuilding(unsigned char** access_map, Point site, int value);
    bool ReconnectBuilding(unsigned char** access_map, Rect* bounds, int value);
    static bool FindMarkedSite(unsigned char** access_map, Rect* bounds);

public:
    TaskManageBuildings(unsigned short team, Point site);
    ~TaskManageBuildings();

    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    bool IsNeeded();
    void AddUnit(UnitInfo& unit);
    void Begin();
    void BeginTurn();
    void ChildComplete(Task* task);
    void EndTurn();
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);
    void EventUnitDestroyed(UnitInfo& unit);

    void AddCreateOrder(TaskCreateBuilding* task);
    bool CheckPower();
    void CheckWorkers();
    bool FindDefenseSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site, int value,
                         unsigned short task_flags);
    bool FindSiteForRadar(TaskCreateBuilding* task, Point& site);
    bool ChangeSite(TaskCreateBuilding* task, Point& site);
    void BuildBridge(Point site, Task* task);
    bool CreateBuilding(ResourceID unit_type, Task* task, unsigned short task_flags);

    bool ReconnectBuildings();
};

#endif /* TASKMANAGEBUILDINGS_HPP */
