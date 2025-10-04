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

#ifndef TASKCREATEBUILDING_HPP
#define TASKCREATEBUILDING_HPP

#include "taskcreate.hpp"

class TaskManageBuildings;

class TaskCreateBuilding : public TaskCreate {
    SmartPointer<UnitInfo> building;
    SmartPointer<TaskManageBuildings> manager;
    Point site;
    uint8_t op_state;
    SmartPointer<Zone> zone;
    bool field_42;
    SmartList<Task> tasks;

    void RequestBuilder();
    void AbandonSite();
    bool BuildRoad();
    void BeginBuilding();
    void Abort();
    void Finish();
    bool RequestMineRemoval();
    bool RequestRubbleRemoval();
    bool RequestWaterPlatform();
    void Activate();
    void FindBuildSite();
    bool CheckMaterials();
    void BuildBoardwalks();
    void BuildBridges();
    void MarkBridgeAreas(uint8_t** map);
    void PopulateMap(uint8_t** map);
    bool FindBridgePath(uint8_t** map, int32_t value);

    static void MoveFinishedCallback(Task* task, UnitInfo* unit, char result);
    static bool SearchPathStep(uint8_t** map, Point position, int32_t* direction, uint16_t* best_unit_count,
                               int32_t value);

public:
    TaskCreateBuilding(Task* task, uint16_t flags, ResourceID unit_type, Point site, TaskManageBuildings* manager);
    TaskCreateBuilding(UnitInfo* unit, TaskManageBuildings* manager);
    ~TaskCreateBuilding();

    char* WriteStatusLog(char* buffer) const;
    Rect* GetBounds(Rect* bounds);
    uint8_t GetType() const;
    bool IsNeeded();
    void AddUnit(UnitInfo& unit);
    void Begin();
    void BeginTurn();
    void ChildComplete(Task* task);
    void EndTurn();
    bool ExchangeOperator(UnitInfo& unit);
    bool Execute(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);
    void EventZoneCleared(Zone* zone, bool status);
    bool Task_vfunc28();
    bool Task_vfunc29();

    int32_t EstimateBuildTime();
};

#endif /* TASKCREATEBUILDING_HPP */
