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

#include "paths_manager.hpp"
#include "task_manager.hpp"
#include "taskpathrequest.hpp"

TaskFindPath::TaskFindPath(Task* parent, PathRequest* request,
                           void (*result_callback_)(Task* task, PathRequest* path_request, Point destination_,
                                                    GroundPath* path, char result),
                           void (*cancel_callback_)(Task* task, PathRequest* path_request))
    : Task(parent->GetTeam(), parent, parent->GetFlags()) {
    result_callback = result_callback_;
    cancel_callback = cancel_callback_;

    dynamic_cast<TaskPathRequest*>(request)->AssignPathFindTask(this);

    path_request = request;
}

TaskFindPath::~TaskFindPath() {}

int TaskFindPath::GetMemoryUse() const { return 4; }

char* TaskFindPath::WriteStatusLog(char* buffer) const {
    if (path_request) {
        Point destination(path_request->GetPoint());
        char string[20];

        strcpy(buffer, "Find Path to ");
        sprintf(string, "[%i,%i], c.l. %i", destination.x + 1, destination.y + 1, path_request->GetCautionLevel());
        strcat(buffer, string);

    } else {
        strcpy(buffer, "Find path (finished)");
    }

    return buffer;
}

unsigned char TaskFindPath::GetType() const { return TaskType_TaskFindPath; }

bool TaskFindPath::IsThinking() { return path_request; }

void TaskFindPath::Begin() {
    path_request->GetUnit1()->AddTask(this);
    PathsManager_RemoveRequest(path_request->GetUnit1());
    PathsManager_PushBack(*path_request);
}

void TaskFindPath::EndTurn() {}

void TaskFindPath::RemoveSelf() {
    if (path_request) {
        PathsManager_RemoveRequest(&*path_request);

        path_request = nullptr;
    }

    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskFindPath::RemoveUnit(UnitInfo& unit) {
    if (path_request) {
        SDL_assert(path_request->GetUnit1() == &unit);

        RemoveSelf();
    }
}

void TaskFindPath::CancelRequest() {
    if (path_request) {
        path_request->GetUnit1()->RemoveTask(this, false);

        cancel_callback(&*parent, &*path_request);

        path_request = nullptr;
        parent = nullptr;
        TaskManager.RemoveTask(*this);
    }
}

void TaskFindPath::Finish(Point position, GroundPath* path, bool result) {
    path_request->GetUnit1()->RemoveTask(this, false);

    result_callback(&*parent, &*path_request, position, path, result);

    path_request = nullptr;
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}
