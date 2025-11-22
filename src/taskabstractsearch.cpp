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

#include "ailog.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "task_manager.hpp"
#include "taskrepair.hpp"
#include "tasksearchdestination.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

void TaskAbstractSearch::FindDestination(UnitInfo& unit, int32_t radius) {
    AILOG(log, "Abstract Search: Find Destination");

    if (!Task_RetreatFromDanger(this, &unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE) && IsProcessingNeeded() && unit.speed) {
        SetProcessingNeeded(false);

        SmartPointer<Task> search_destination_task = new (std::nothrow) TaskSearchDestination(this, &unit, radius);

        TaskManager.AppendTask(*search_destination_task);
    }
}

TaskAbstractSearch::TaskAbstractSearch(uint16_t team_, Task* task, uint16_t priority_, Point point_)
    : Task(team_, task, priority_) {
    point = point_;
    requestors = 0;
}

TaskAbstractSearch::~TaskAbstractSearch() {}

Point TaskAbstractSearch::GetPoint() const { return point; }

void TaskAbstractSearch::AddUnit(UnitInfo& unit) {
    AILOG(log, "Abstract search: Add {}", ResourceManager_GetUnit(unit.GetUnitType()).GetSingularName().data());

    units.PushBack(unit);
    unit.AddTask(this);
    Task_RemindMoveFinished(&unit);
}

void TaskAbstractSearch::BeginTurn() { EndTurn(); }

void TaskAbstractSearch::ChildComplete(Task* task) {
    AILOG(log, "Abstract Search:: Child Complete.");

    if (task->GetType() == TaskType_TaskObtainUnits) {
        --requestors;

        if (requestors < 0) {
            requestors = 0;
        }

        AILOG_LOG(log, "Requestors open: {}", requestors);
    }
}

void TaskAbstractSearch::EndTurn() {
    AILOG(log, "Abstract search end turn");

    if (units.GetCount()) {
        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if ((*it).speed && (*it).IsReadyForOrders(this)) {
                if ((*it).hits <= ((*it).GetBaseValues()->GetAttribute(ATTRIB_HITS) / 2) &&
                    ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_APPRENTICE) {
                    (*it).RemoveTasks();

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
        TaskManager.ClearUnitTasksAndRemindAvailable(&*it);
    }

    units.Clear();
    m_parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskAbstractSearch::RemoveUnit(UnitInfo& unit) {
    AILOG(log, "Abstract search: Remove {} (requestors open: {})",
          ResourceManager_GetUnit(unit.GetUnitType()).GetSingularName().data(), requestors);

    units.Remove(unit);
}

void TaskAbstractSearch::TaskAbstractSearch_vfunc28(UnitInfo& unit) {
    if (units.GetCount() > 0 || requestors > 0) {
        TaskManager.ClearUnitTasksAndRemindAvailable(&unit);
    }
}
