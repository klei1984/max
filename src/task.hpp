/* Copyright (c) 2021 M.A.X. Port Team
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

#ifndef TASKS_HPP
#define TASKS_HPP

#include "resource_manager.hpp"
#include "smartobjectarray.hpp"
#include "smartpointer.hpp"
#include "zone.hpp"

enum : uint8_t {
    TaskType_TaskActivate = 0,
    TaskType_TaskAssistMove = 1,
    TaskType_TaskAttack = 2,
    TaskType_TaskAttackReserve = 3,
    TaskType_TaskAutoSurvey = 4,

    TaskType_TaskCheckAssaults = 6,
    TaskType_TaskClearZone = 7,
    TaskType_TaskCreateUnit = 8,
    TaskType_TaskCreateBuilding = 9,
    TaskType_TaskEscort = 10,
    TaskType_TaskExplore = 11,
    TaskType_TaskDefenseAssistant = 12,
    TaskType_TaskDefenseReserve = 13,
    TaskType_TaskDump = 14,
    TaskType_TaskFindMines = 15,
    TaskType_TaskFindPath = 16,
    TaskType_TaskFrontalAttack = 17,
    TaskType_TaskGetMaterials = 18,
    TaskType_TaskHabitatAssistant = 19,
    TaskType_TaskKillUnit = 20,
    TaskType_TaskManageBuildings = 21,
    TaskType_TaskMineAssisstant = 22,
    TaskType_TaskMove = 23,
    TaskType_TaskMoveHome = 24,
    TaskType_TaskObtainUnits = 25,
    TaskType_TaskPlaceMines = 26,
    TaskType_TaskConnectionAssistant = 27,
    TaskType_TaskRadarAssistant = 28,
    TaskType_TaskFrontierAssistant = 29,
    TaskType_TaskPowerAssistant = 30,

    TaskType_TaskReload = 33,
    TaskType_TaskRemoveMines = 34,
    TaskType_TaskRemoveRubble = 35,
    TaskType_TaskRendezvous = 36,
    TaskType_TaskRepair = 37,
    TaskType_TaskRetreat = 38,
    TaskType_TaskSearchDestination = 39,
    TaskType_TaskScavenge = 40,

    TaskType_TaskSupportAttack = 42,
    TaskType_TaskSurvey = 43,

    TaskType_TaskTransport = 45,
    TaskType_TaskUpdateTerrain = 46,
    TaskType_TaskUpgrade = 47,
    TaskType_TaskWaitToAttack = 48
};

class Complex;
class UnitInfo;

const char* Task_GetName(Task* task);
void Task_UpgradeStationaryUnit(UnitInfo* unit);
void Task_RemindMoveFinished(UnitInfo* unit, bool priority = false);
bool Task_IsReadyToTakeOrders(UnitInfo* unit);
void Task_RemoveMovementTasks(UnitInfo* unit);
bool Task_ShouldReserveShot(UnitInfo* unit, Point site);
bool Task_IsUnitDoomedToDestruction(UnitInfo* unit, int32_t caution_level);
bool Task_RetreatIfNecessary(Task* task, UnitInfo* unit, int32_t caution_level);
bool Task_RetreatFromDanger(Task* task, UnitInfo* unit, int32_t caution_level);
bool Task_IsAdjacent(UnitInfo* unit, int16_t grid_x, int16_t grid_y);
int32_t Task_EstimateTurnsTillMissionEnd();

int32_t Task_GetReadyUnitsCount(uint16_t team, ResourceID unit_type);

class Task : public SmartObject {
    static uint32_t task_count;
    static uint32_t task_id;

    bool needs_processing;
    bool scheduled_for_turn_start;
    bool scheduled_for_turn_end;

protected:
    uint32_t id;
    uint16_t team;
    uint16_t flags;

    SmartPointer<Task> parent;

public:
    Task(uint16_t team, Task* parent, uint16_t flags);
    Task(const Task& other);
    virtual ~Task();

    void RemindTurnEnd(bool priority = false);

    void RemindTurnStart(bool priority = false);

    bool IsProcessingNeeded() const;
    void SetProcessingNeeded(bool value);

    bool IsScheduledForTurnStart() const;
    void ChangeIsScheduledForTurnStart(bool value);

    bool IsScheduledForTurnEnd() const;
    void ChangeIsScheduledForTurnEnd(bool value);

    uint16_t GetTeam() const;
    uint32_t GetId() const;

    Task* GetParent();
    void SetParent(Task* task);

    void SetFlags(uint16_t flags);

    int16_t DeterminePriority(uint16_t task_flags);
    Point DeterminePosition();

    virtual bool Task_vfunc1(UnitInfo& unit);
    virtual bool IsUnitUsable(UnitInfo& unit);
    virtual int32_t GetCautionLevel(UnitInfo& unit);
    virtual uint16_t GetFlags() const;
    virtual char* WriteStatusLog(char* buffer) const = 0;
    virtual Rect* GetBounds(Rect* bounds);
    virtual uint8_t GetType() const = 0;
    virtual bool IsNeeded();
    virtual bool IsThinking();
    virtual void AddUnit(UnitInfo& unit);
    virtual void Begin();
    virtual void BeginTurn();
    virtual void ChildComplete(Task* task);
    virtual void EndTurn();
    virtual bool ExchangeOperator(UnitInfo& unit);
    virtual bool Execute(UnitInfo& unit);
    virtual void RemoveSelf();
    virtual bool CheckReactions();
    virtual void Task_vfunc20(UnitInfo& unit);
    virtual void RemoveUnit(UnitInfo& unit);
    virtual void EventCargoTransfer(UnitInfo& unit);
    virtual void EventUnitDestroyed(UnitInfo& unit);
    virtual void EventUnitLoaded(UnitInfo& unit1, UnitInfo& unit2);
    virtual void EventEnemyUnitSpotted(UnitInfo& unit);
    virtual void EventUnitUnloaded(UnitInfo& unit1, UnitInfo& unit2);
    virtual void EventZoneCleared(Zone* zone, bool status);
};

#endif /* TASKS_HPP */
