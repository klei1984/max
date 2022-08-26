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

TaskPathRequest::TaskPathRequest(UnitInfo* unit, int mode, Point point) : PathRequest(unit, mode, point), field_30(0) {}

TaskPathRequest::~TaskPathRequest() {}

bool TaskPathRequest::PathRequest_Vfunc1() {
    bool result;

    if (field_30 && find_path != nullptr && AiPlayer_Teams[find_path->GetTeam()].MatchPath(this)) {
        find_path = nullptr;
        unit1 = nullptr;

        result = true;

    } else {
        result = false;
    }

    return result;
}

void TaskPathRequest::Cancel() {
    find_path->CancelRequest();
    find_path = nullptr;
    unit1 = nullptr;
}

void TaskPathRequest::Finish(GroundPath* path) {
    Point position(unit1->grid_x, unit1->grid_y);

    find_path->Finish(position, path, false);
    find_path = nullptr;
    unit1 = nullptr;
}

void TaskPathRequest::AssignPathFindTask(TaskFindPath* task) { find_path = task; }

void TaskPathRequest::SetField31(bool value) {
    field_30 = true;
    field_31 = value;
}

bool TaskPathRequest::GetField31() const { return field_31; }

void TaskPathRequest::Complete(Point position, GroundPath* path) {
    find_path->Finish(position, path, true);
    find_path = nullptr;
    unit1 = nullptr;
}
