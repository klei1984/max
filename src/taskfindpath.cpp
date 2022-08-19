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

#include "taskfindpath.hpp"

TaskFindPath::TaskFindPath(Task* parent, PathRequest* request,
                           void (*result_callback)(Task* task, PathRequest* request, Point destination_,
                                                   GroundPath* path, char result),
                           void (*cancel_callback)(Task* task, PathRequest* request))
    : Task(parent->GetTeam(), parent, parent->GetFlags()) {}

TaskFindPath::~TaskFindPath() {}

int TaskFindPath::GetMemoryUse() const {}

char* TaskFindPath::WriteStatusLog(char* buffer) const {}

unsigned char TaskFindPath::GetType() const {}

bool TaskFindPath::Task_vfunc10() {}

void TaskFindPath::AddReminder() {}

void TaskFindPath::EndTurn() {}

void TaskFindPath::RemoveSelf() {}

void TaskFindPath::Remove(UnitInfo& unit) {}

void TaskFindPath::CancelRequest() {}

void TaskFindPath::Finish(Point position, GroundPath* path, bool mode) {}
