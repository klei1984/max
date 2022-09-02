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

#include "task_manager.hpp"

class TaskManager TaskManager;

int TaskManager_GetDistance(int distance_x, int distance_y) {
    int result;

    distance_x = labs(distance_x);
    distance_y = labs(distance_y);

    if (distance_x >= distance_y) {
        result = 2 * distance_x + distance_y;
    } else {
        result = 2 * distance_y + distance_x;
    }

    return result;
}

int TaskManager_GetDistance(Point point1, Point point2) {
    return TaskManager_GetDistance(point1.x - point2.x, point1.y - point2.y);
}

int TaskManager_GetDistance(UnitInfo* unit1, UnitInfo* unit2) {
    return TaskManager_GetDistance(unit1->grid_x - unit2->grid_x, unit1->grid_y - unit2->grid_y);
}

TaskManager::TaskManager() : field_44(0) {}

TaskManager::~TaskManager() {}

void TaskManager::AddReminder(Reminder* reminder, bool priority) {
    if (priority) {
        reminderlist_1.PushBack(*reminder);
    } else {
        reminderlist_0.PushBack(*reminder);
    }
}

void TaskManager::AddTask(Task& task) {
    tasklist.PushBack(task);

    if (task.GetType() == TaskType_TaskObtainUnits) {
        taskobtainunitslist.PushBack(*dynamic_cast<TaskObtainUnits*>(&task));
    }

    task.Begin();
}

void TaskManager::RemoveTask(Task& task) {
    if (task.GetType() == TaskType_TaskObtainUnits) {
        taskobtainunitslist.Remove(*dynamic_cast<TaskObtainUnits*>(&task));
    }

    tasklist.Remove(task);
}

void TaskManager::RemindAvailable(UnitInfo* unit, bool priority) {
    /// \todo
}

void TaskManager::CreateBuilding(ResourceID unit_type, unsigned short team, Point point, Task* task) {
    /// \todo
}

void TaskManager::CreateUnit(ResourceID unit_type, unsigned short team, Point point, Task* task) {
    /// \todo
}

void TaskManager::ManufactureUnit(ResourceID unit_type, unsigned short team, int requested_amount, Task* task,
                                  Point point) {
    /// \todo
}
