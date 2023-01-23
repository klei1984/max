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

#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#include "reminders.hpp"
#include "smartlist.hpp"
#include "task.hpp"
#include "taskobtainunits.hpp"
#include "unitinfo.hpp"

int TaskManager_GetDistance(int distance_x, int distance_y);
int TaskManager_GetDistance(Point point1, Point point2);
int TaskManager_GetDistance(UnitInfo* unit1, UnitInfo* unit2);

bool TaskManager_NeedToReserveRawMaterials(unsigned short team);

class TaskManager {
    SmartList<Task> tasks;
    SmartList<TaskObtainUnits> unit_requests;
    SmartList<Reminder> normal_reminders;
    SmartList<Reminder> priority_reminders;
    SmartList<UnitInfo> units;
    unsigned short reminder_counter;

    bool IsUnitNeeded(ResourceID unit_type, unsigned short team, unsigned short flags);

public:
    TaskManager();
    ~TaskManager();

    unsigned int CalcMemoryUsage();
    bool CheckTasksThinking(unsigned short team);
    void CheckComputerReactions();
    void EnumeratePotentialAttackTargets(UnitInfo* unit);
    void AppendUnit(UnitInfo& unit);
    void CreateBuilding(ResourceID unit_type, unsigned short team, Point site, Task* task);
    void CreateUnit(ResourceID unit_type, unsigned short team, Point site, Task* task);
    void ManufactureUnits(ResourceID unit_type, unsigned short team, int requested_amount, Task* task, Point site);
    void AppendTask(Task& task);
    void AppendReminder(Reminder* reminder, bool priority = false);
    bool ExecuteReminders();
    void BeginTurn(unsigned short team);
    void EndTurn(unsigned short team);
    void ChangeFlagsSet(unsigned short team);
    void Clear();
    void RemindAvailable(UnitInfo* unit, bool priority = false);
    void FindTaskForUnit(UnitInfo* unit);
    void RemoveTask(Task& task);
    void ProcessTasks1(UnitInfo* unit);
    void ProcessTasks2(UnitInfo* unit);
    int GetRemindersCount() const;
    SmartList<Task>& GetTaskList();
};

extern class TaskManager TaskManager;
extern unsigned short TaskManager_word_1731C0;

#endif /* TASK_MANAGER_HPP */
