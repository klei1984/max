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

#ifndef TASKDUMP_HPP
#define TASKDUMP_HPP

#include "pathrequest.hpp"
#include "task_manager.hpp"
#include "tasktransport.hpp"

class TaskDump : public Task {
    SmartPointer<TaskMove> task_move;
    SmartPointer<UnitInfo> unit;
    short field_27;
    short direction;
    short field_31;
    Point destination;
    char field_37;

    static void TaskDump_PathResultCallback(Task* task, PathRequest* request, Point destination, GroundPath* path,
                                            char result);
    static void TaskDump_PathCancelCallback(Task* task, PathRequest* request);

    void Search();
    void RemoveTask();

public:
    TaskDump(TaskTransport* task_transport, TaskMove* task_move, UnitInfo* unit);
    ~TaskDump();

    int GetMemoryUse() const;
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    void AddReminder();
    void RemoveSelf();
    void Remove(UnitInfo& unit);
};

#endif /* TASKDUMP_HPP */