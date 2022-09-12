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

#include "taskabstractsearch.hpp"

#include "inifile.hpp"
#include "task_manager.hpp"
#include "taskrepair.hpp"
#include "tasksearchdestination.hpp"

void TaskAbstractSearch::FindDestination(UnitInfo& unit, int radius) {
    if (!Task_RetreatFromDanger(this, &unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE) && field_6 && unit.speed) {
        field_6 = false;

        SmartPointer<Task> search_destination_task = new (std::nothrow) TaskSearchDestination(this, &unit, radius);

        TaskManager.AppendTask(*search_destination_task);
    }
}

TaskAbstractSearch::TaskAbstractSearch(unsigned short team_, Task* task, unsigned short flags_, Point point_)
    : Task(team_, task, flags_) {
    point = point_;
    requestors = 0;
}

TaskAbstractSearch::~TaskAbstractSearch() {}

Point TaskAbstractSearch::GetPoint() const { return point; }

void TaskAbstractSearch::Task_vfunc11(UnitInfo& unit) {
    units.PushBack(unit);
    unit.PushFrontTask1List(this);
    Task_RemindMoveFinished(&unit);
}

void TaskAbstractSearch::BeginTurn() { EndTurn(); }

void TaskAbstractSearch::ChildComplete(Task* task) {
    if (task->GetType() == TaskType_TaskObtainUnits) {
        --requestors;

        if (requestors < 0) {
            requestors = 0;
        }
    }
}

void TaskAbstractSearch::EndTurn() {
    if (units.GetCount()) {
        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if ((*it).speed && (*it).IsReadyForOrders(this)) {
                if ((*it).hits <= ((*it).GetBaseValues()->GetAttribute(ATTRIB_HITS) / 2) &&
                    ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_APPRENTICE) {
                    (*it).ClearFromTaskLists();

                    SmartPointer<Task> repair_task = new (std::nothrow) TaskRepair(&*it);

                    TaskManager.AppendTask(*repair_task);

                } else {
                    Task_RemindMoveFinished(&*it);

                    if (!(*it).IsReadyForOrders(this)) {
                        break;
                    }
                }
            }
        }

    } else {
        if (requestors == 0) {
            ++requestors;

            ObtainUnit();
        }
    }
}

void TaskAbstractSearch::RemoveSelf() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units.Clear();
    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskAbstractSearch::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }

void TaskAbstractSearch::TaskAbstractSearch_vfunc28(UnitInfo& unit) {
    if (units.GetCount() > 0 || requestors > 0) {
        TaskManager.RemindAvailable(&unit);
    }
}
