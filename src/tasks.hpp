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

#include "enums.hpp"
#include "point.hpp"
#include "smartobjectarray.hpp"
#include "smartpointer.hpp"
#include "zone.hpp"

enum : unsigned char {
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
    TaskType_TaskFrontierAssistant = 22,
    TaskType_TaskMove = 23,
    TaskType_TaskMoveHome = 24,
    TaskType_TaskObtainUnits = 25,
    TaskType_TaskPlaceMines = 26,
    TaskType_TaskConnectionAssistant = 27,
    TaskType_TaskRadarAssistant = 28,

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

extern "C" {
#include "gnw.h"
}

void Task_RemindMoveFinished(UnitInfo* unit, bool priority = false);

class Task : public SmartObject {
    static unsigned short task_id;
    static unsigned short task_count;

    bool field_6;
    bool field_7;
    bool field_8;

protected:
    unsigned short team;
    unsigned short id;
    unsigned short flags;

    SmartPointer<Task> parent;

public:
    Task(unsigned short team, Task* parent, unsigned short flags);
    Task(const Task& other);
    virtual ~Task();

    void RemindTurnEnd(bool priority);

    void RemindTurnStart(bool priority);

    bool GetField6() const;
    void SetField6(bool value);

    bool GetField7() const;
    void SetField7(bool value);

    bool GetField8() const;
    void SetField8(bool value);

    unsigned short GetTeam() const;
    short Task_sub_42BC4(unsigned short task_flags);
    Point Task_sub_42D3D();

    virtual bool Task_vfunc1(UnitInfo& unit);
    virtual bool Task_vfunc2(UnitInfo& unit);
    virtual int Task_vfunc3(UnitInfo& unit);
    virtual int Task_vfunc4() const = 0;
    virtual unsigned short GetFlags() const;
    virtual char* WriteStatusLog(char* buffer) const = 0;
    virtual Rect* GetBounds(Rect* bounds);
    virtual unsigned char GetType() const = 0;
    virtual bool Task_vfunc9();
    virtual bool Task_vfunc10();
    virtual void Task_vfunc11(UnitInfo& unit);
    virtual void AddReminder();
    virtual void Task_vfunc13();
    virtual void Task_vfunc14(Task* task);
    virtual void Task_vfunc15();
    virtual bool Task_vfunc16(UnitInfo& unit);
    virtual bool Task_vfunc17(UnitInfo& unit);
    virtual void RemoveSelf();
    virtual bool Task_vfunc19();
    virtual void Task_vfunc20(int unknown);
    virtual void Task_vfunc21(UnitInfo& unit);
    virtual void Task_vfunc22(UnitInfo& unit);
    virtual void Task_vfunc23(UnitInfo& unit);
    virtual void Task_vfunc24(UnitInfo& unit1, UnitInfo& unit2);
    virtual void Task_vfunc25(int unknown);
    virtual void Task_vfunc26(UnitInfo& unit1, UnitInfo& unit2);
    virtual void Task_vfunc27(Zone* zone, char mode);
};

class TaskObtainUnits : public Task {
    SmartObjectArray<ResourceID> units;
    Point point;
    bool field_27;
    bool field_28;

    bool TaskObtainUnits_sub_464C6(UnitInfo* unit, bool mode);

public:
    TaskObtainUnits(Task* task, Point point);
    ~TaskObtainUnits();

    unsigned short CountInstancesOfUnitType(ResourceID unit_type);
    UnitInfo* TaskObtainUnits_sub_465F4(ResourceID unit_type, bool mode);

    int Task_vfunc4() const;
    unsigned short GetFlags() const;
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    bool Task_vfunc9();
    void Task_vfunc11(UnitInfo& unit);
    void AddReminder();
    void Task_vfunc13();
    void Task_vfunc15();
    void RemoveSelf();
};

class TaskGetResource : public Task {
    SmartPointer<UnitInfo> unit1;
    SmartPointer<UnitInfo> unit2;
    SmartPointer<UnitInfo> unit3;

    void TaskGetResource_sub_46DF3();
    void TaskGetResource_sub_471A4(int unknown, char mode);

protected:
    UnitInfo* TaskGetResource_sub_46D29(Complex* complex);
    void TaskGetResource_sub_471F8();

public:
    TaskGetResource(Task* task, UnitInfo& unit);
    ~TaskGetResource();

    virtual void AddReminder();
    virtual void Task_vfunc15();
    virtual void RemoveSelf();
    virtual void Task_vfunc21(UnitInfo& unit);

    virtual void Task_vfunc28() = 0;
    virtual UnitInfo* Task_vfunc29() = 0;
    virtual void Task_vfunc30() = 0;
};

class TaskRendezvous : public Task {
    SmartPointer<UnitInfo> unit1;
    SmartPointer<UnitInfo> unit2;
    void (TaskGetResource::*function)(int unknown, char mode);

    void TaskRendezvous_sub_54D3B();

public:
    TaskRendezvous(UnitInfo* unit1, UnitInfo* unit2, Task* task,
                   void (TaskGetResource::*function)(int unknown, char mode));
    ~TaskRendezvous();

    int Task_vfunc4() const;
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    void AddReminder();
    void Task_vfunc13();
    void Task_vfunc15();
    bool Task_vfunc17(UnitInfo& unit);
    void RemoveSelf();
    void Task_vfunc21(UnitInfo& unit);
};

class TaskClearZone : public Task {
    unsigned char state;
    SmartArray<Zone> zones;
    ObjectArray<ZoneSquare> zone_squares;
    ObjectArray<Point> points1;
    ObjectArray<Point> points2;
    SmartPointer<UnitInfo> moving_unit;
    unsigned int flags;

public:
    TaskClearZone(unsigned short team, unsigned int flags);
    ~TaskClearZone();

    int Task_vfunc4() const;
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    bool Task_vfunc10();
    void AddReminder();
    void Task_vfunc13();
    void Task_vfunc15();
    bool Task_vfunc17(UnitInfo& unit);
    void RemoveSelf();
    void Task_vfunc21(UnitInfo& unit);
};

#endif /* TASKS_HPP */
