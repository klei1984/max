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

#include "taskpathrequest.hpp"

#include "aiplayer.hpp"
#include "taskfindpath.hpp"

TaskPathRequest::TaskPathRequest(UnitInfo* unit, int32_t mode, Point point)
    : PathRequest(unit, mode, point), can_use_cached_paths(0) {}

TaskPathRequest::~TaskPathRequest() {}

bool TaskPathRequest::TryUseCachedPath() {
    bool result;

    if (can_use_cached_paths && find_path != nullptr && AiPlayer_Teams[find_path->GetTeam()].MatchPath(this)) {
        find_path = nullptr;
        client = nullptr;

        result = true;

    } else {
        result = false;
    }

    return result;
}

void TaskPathRequest::Cancel() {
    find_path->CancelRequest();
    find_path = nullptr;
    client = nullptr;
}

void TaskPathRequest::Finish(GroundPath* path) {
    Point position(client->grid_x, client->grid_y);

    find_path->Finish(position, path, false);
    find_path = nullptr;
    client = nullptr;
}

void TaskPathRequest::AssignPathFindTask(TaskFindPath* task) { find_path = task; }

void TaskPathRequest::EnableCachedPaths(bool allow_generic_transporter) {
    can_use_cached_paths = true;
    this->allow_generic_transporter = allow_generic_transporter;
}

bool TaskPathRequest::AllowsGenericTransporter() const { return allow_generic_transporter; }

void TaskPathRequest::Complete(Point position, GroundPath* path) {
    find_path->Finish(position, path, true);
    find_path = nullptr;
    client = nullptr;
}
