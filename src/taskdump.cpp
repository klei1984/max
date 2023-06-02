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

#include "taskdump.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "paths.hpp"
#include "taskfindpath.hpp"
#include "taskpathrequest.hpp"
#include "units_manager.hpp"

TaskDump::TaskDump(TaskTransport* task_transport, TaskMove* task_move_, UnitInfo* transporter)
    : Task(task_move_->GetTeam(), task_transport, task_move_->GetFlags()),
      transporter_unit(transporter),
      task_move(task_move_) {
    steps_limit = 2;
    steps_counter = 2;
    direction = 0;

    destination.x = transporter_unit->grid_x - 1;
    destination.y = transporter_unit->grid_y + 1;

    keep_searching = 0;
}

TaskDump::~TaskDump() {}

void TaskDump::TaskDump_PathResultCallback(Task* task, PathRequest* request, Point destination_, GroundPath* path,
                                           unsigned char result) {
    AiLog log("Dump: path result.");

    TaskDump* dump_task = dynamic_cast<TaskDump*>(task);

    if (path) {
        dump_task->task_move->SetDestination(dump_task->destination);
        dump_task->RemoveTask();

    } else {
        dump_task->Search();
    }
}

void TaskDump::TaskDump_PathCancelCallback(Task* task, PathRequest* request) {
    TaskDump* dump_task = dynamic_cast<TaskDump*>(task);

    dump_task->RemoveTask();
}

void TaskDump::Search() {
    Rect bounds;
    SmartPointer<UnitInfo> passenger;
    int minimum_distance;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (transporter_unit->unit_type == AIRTRANS) {
        minimum_distance = 0;

    } else {
        minimum_distance = 2;
    }

    passenger = task_move->GetPassenger();

    AiLog log("Dump %s: search for position.", UnitsManager_BaseUnits[passenger->unit_type].singular_name);

    for (;;) {
        if (--steps_counter < 0) {
            direction += 2;

            if (direction >= 8) {
                if (!keep_searching) {
                    RemoveTask();

                    return;
                }

                steps_limit += 2;
                keep_searching = false;
                direction = 0;
            }

            steps_counter = steps_limit;
        }

        destination += Paths_8DirPointsArray[direction];

        if (Access_IsInsideBounds(&bounds, &destination)) {
            keep_searching = true;

            if (Access_IsAccessible(passenger->unit_type, team, destination.x, destination.y, 0x01)) {
                if (!Ai_IsDangerousLocation(&*passenger, destination, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, true)) {
                    TaskPathRequest* request = new (std::nothrow) TaskPathRequest(&*transporter_unit, 1, destination);

                    request->SetMinimumDistance(minimum_distance);
                    request->SetCautionLevel(CAUTION_LEVEL_AVOID_ALL_DAMAGE);
                    request->SetOptimizeFlag(false);

                    SmartPointer<Task> find_path(new (std::nothrow) TaskFindPath(
                        this, request, &TaskDump_PathResultCallback, &TaskDump_PathCancelCallback));

                    TaskManager.AppendTask(*find_path);

                    return;
                }
            }
        }
    }
}

void TaskDump::RemoveTask() {
    transporter_unit->RemoveTask(this);
    transporter_unit = nullptr;
    task_move = nullptr;
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

int TaskDump::GetMemoryUse() const { return 4; }

char* TaskDump::WriteStatusLog(char* buffer) const {
    if (task_move == nullptr) {
        strcpy(buffer, "Dump unit task, null move.");

    } else if (task_move->GetPassenger()) {
        strcpy(buffer, "Find a place to unload ");
        strcat(buffer, UnitsManager_BaseUnits[task_move->GetPassenger()->unit_type].singular_name);

    } else {
        strcpy(buffer, "Dump unit task, null unit.");
    }

    return buffer;
}

unsigned char TaskDump::GetType() const { return TaskType_TaskDump; }

void TaskDump::Begin() {
    transporter_unit->AddTask(this);
    steps_limit = 2;
    steps_counter = 2;
    direction = 0;

    destination.x = transporter_unit->grid_x - 1;
    destination.y = transporter_unit->grid_y + 1;

    keep_searching = 0;

    Search();
}

void TaskDump::RemoveSelf() {
    if (transporter_unit != nullptr) {
        TaskManager.RemindAvailable(&*transporter_unit);
    }

    transporter_unit = nullptr;
    task_move = nullptr;
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskDump::RemoveUnit(UnitInfo& unit_) {
    if (transporter_unit == &unit_ || (task_move && task_move->GetPassenger() == &unit_)) {
        RemoveTask();
    }
}
