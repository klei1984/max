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

#include "ailog.hpp"
#include "paths_manager.hpp"
#include "task_manager.hpp"
#include "taskpathrequest.hpp"
#include "units_manager.hpp"

TaskFindPath::TaskFindPath(Task* parent, PathRequest* request,
                           void (*result_callback_)(Task* task, PathRequest* path_request, Point destination_,
                                                    GroundPath* path, uint8_t result),
                           void (*cancel_callback_)(Task* task, PathRequest* path_request))
    : Task(parent->GetTeam(), parent, parent->GetFlags()) {
    result_callback = result_callback_;
    cancel_callback = cancel_callback_;

    dynamic_cast<TaskPathRequest*>(request)->AssignPathFindTask(this);

    path_request = request;
}

TaskFindPath::~TaskFindPath() {}

char* TaskFindPath::WriteStatusLog(char* buffer) const {
    if (path_request) {
        Point destination(path_request->GetDestination());
        char string[50];

        strcpy(buffer, "Find Path to ");
        sprintf(string, "[%i,%i].", destination.x + 1, destination.y + 1);
        strcat(buffer, string);

    } else {
        strcpy(buffer, "Find path (finished)");
    }

    return buffer;
}

uint8_t TaskFindPath::GetType() const { return TaskType_TaskFindPath; }

bool TaskFindPath::IsThinking() { return path_request != nullptr; }

void TaskFindPath::Begin() {
    AILOG(log, "Task find path for {}: begin.",
          UnitsManager_BaseUnits[path_request->GetClient()->GetUnitType()].GetSingularName());

    path_request->GetClient()->AddTask(this);
    PathsManager_RemoveRequest(path_request->GetClient());
    PathsManager_PushBack(*path_request);
}

void TaskFindPath::EndTurn() {}

void TaskFindPath::RemoveSelf() {
    if (path_request) {
        AILOG(log, "Task find path for {}: parent complete.",
              UnitsManager_BaseUnits[path_request->GetClient()->GetUnitType()].GetSingularName());

        PathsManager_RemoveRequest(&*path_request);

        path_request = nullptr;
    }

    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskFindPath::RemoveUnit(UnitInfo& unit) {
    if (path_request) {
        AILOG(log, "Task find path for {}: remove {}.",
              UnitsManager_BaseUnits[path_request->GetClient()->GetUnitType()].GetSingularName(),
              UnitsManager_BaseUnits[unit.GetUnitType()].GetSingularName());

        SDL_assert(path_request->GetClient() == &unit);

        RemoveSelf();
    }
}

void TaskFindPath::CancelRequest() {
    if (path_request) {
        AILOG(log, "Task find path for {}: cancelled.",
              UnitsManager_BaseUnits[path_request->GetClient()->GetUnitType()].GetSingularName());

        path_request->GetClient()->RemoveTask(this, false);

        cancel_callback(&*parent, &*path_request);

        path_request = nullptr;
        parent = nullptr;
        TaskManager.RemoveTask(*this);
    }
}

void TaskFindPath::Finish(Point position, GroundPath* path, bool result) {
    AILOG(log, "Task find path for {}: finished ({}).",
          UnitsManager_BaseUnits[path_request->GetClient()->GetUnitType()].GetSingularName(),
          result ? "True" : "False");

    path_request->GetClient()->RemoveTask(this, false);

    result_callback(&*parent, &*path_request, position, path, result);

    path_request = nullptr;
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}
