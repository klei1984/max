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

#ifndef TASKTRANSPORT_HPP
#define TASKTRANSPORT_HPP

#include "taskmove.hpp"
#include "taskobtainunits.hpp"
#include "transportermap.hpp"

class TaskTransport : public Task {
    ResourceID transporter_unit_type;
    SmartPointer<UnitInfo> unit_transporter;
    SmartList<TaskMove> move_tasks;
    SmartPointer<TaskMove> task_move;
    SmartPointer<TaskObtainUnits> task_obtain_units;

    void AddClients(SmartList<TaskMove>* list);
    bool ChooseNewTask();
    void RemoveClient(TaskMove* move);

    static void MoveFinishedCallback1(Task* task, UnitInfo* unit, char result);
    static void MoveFinishedCallback2(Task* task, UnitInfo* unit, char result);

    bool LoadUnit(UnitInfo* unit);
    void UnloadUnit(UnitInfo* unit);

public:
    TaskTransport(TaskMove* task_move, ResourceID transporter);
    ~TaskTransport();

    bool IsUnitTransferable(UnitInfo& unit);
    char* WriteStatusLog(char* buffer) const;
    Rect* GetBounds(Rect* bounds);
    uint8_t GetType() const;
    bool IsNeeded();
    void AddUnit(UnitInfo& unit);
    void Init();
    void BeginTurn();
    void ChildComplete(Task* task);
    void EndTurn();
    bool Execute(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);
    void EventUnitLoaded(UnitInfo& unit1, UnitInfo& unit2);
    void EventUnitUnloaded(UnitInfo& unit1, UnitInfo& unit2);
    void EventZoneCleared(Zone* zone, bool status);

    ResourceID GetTransporterType() const;
    bool WillTransportNewClient(TaskMove* task);
    bool IsClient(TaskMove* move);
    void AddClient(TaskMove* move);
};

bool TaskTransport_Search(UnitInfo* transporter, UnitInfo* client, TransporterMap* map);
void TaskTransport_MoveFinishedCallback(Task* task, UnitInfo* unit, char result);
void TaskTransport_FinishTransport(Task* const task, UnitInfo* const transporter, UnitInfo* const client, Point site);

#endif /* TASKTRANSPORT_HPP */
