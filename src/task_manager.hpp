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
#include "tasks.hpp"
#include "unitinfo.hpp"

int Taskmanager_sub_45F65(int distance_x, int distance_y);
int TaskManager_sub_4601A(UnitInfo* unit1, UnitInfo* unit2);

class TaskManager {
    SmartList<Task> tasklist;
    SmartList<TaskObtainUnits> taskobtainunitslist;
    SmartList<Reminder> reminderlist_0;
    SmartList<Reminder> reminderlist_1;
    SmartList<UnitInfo> unitinfolist;
    unsigned short field_44;

public:
    TaskManager();
    ~TaskManager();

    void AddReminder(Reminder* reminder, bool priority = false);
    void AddTask(Task& task);
    void RemoveTask(Task& task);

    void TaskManager_sub_44954(ResourceID unit_type, unsigned short team, Point point, Task* task);
    void TaskManager_sub_449D0(ResourceID unit_type, unsigned short team, Point point, Task* task);
    void TaskManager_sub_44A73(ResourceID unit_type, unsigned short team, int requested_amount, Task* task,
                               Point point);
};

extern class TaskManager TaskManager;

#endif /* TASK_MANAGER_HPP */