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

#include "taskscavenge.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "task_manager.hpp"
#include "taskremoverubble.hpp"
#include "units_manager.hpp"

TaskScavenge::TaskScavenge(unsigned short team_) : Task(team_, nullptr, 0x2100) { wait_for_unit = false; }

TaskScavenge::~TaskScavenge() {}

bool TaskScavenge::IsUnitUsable(UnitInfo& unit) { return unit.unit_type == BULLDOZR; }

int TaskScavenge::GetMemoryUse() const { return 4; }

char* TaskScavenge::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Scavenge materials");

    return buffer;
}

unsigned char TaskScavenge::GetType() const { return TaskType_TaskScavenge; }

bool TaskScavenge::Task_vfunc9() { return units.GetCount() == 0; }

void TaskScavenge::AddUnit(UnitInfo& unit) {
    units.PushBack(unit);
    unit.AddTask(this);
    Task_RemindMoveFinished(&unit);
}

void TaskScavenge::BeginTurn() {
    if (units.GetCount() == 0 && !wait_for_unit) {
        SmartPointer<UnitInfo> dummy_unit(new (std::nothrow) UnitInfo(BULLDOZR, team, 0xFFFF));

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).IsVisibleToTeam(team) && ((*it).unit_type == SMLRUBLE || (*it).unit_type == LRGRUBLE) &&
                !Ai_IsDangerousLocation(&*dummy_unit, Point((*it).grid_x, (*it).grid_y), CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                        true)) {
                SmartPointer<TaskObtainUnits> obtain_task(new (std::nothrow)
                                                              TaskObtainUnits(this, Point((*it).grid_x, (*it).grid_y)));
                obtain_task->AddUnit(BULLDOZR);
                wait_for_unit = true;

                TaskManager.AppendTask(*obtain_task);

                return;
            }
        }
    }

    if (!GetField8()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
    }
}

void TaskScavenge::ChildComplete(Task* task) {
    if (task->GetType() == TaskType_TaskObtainUnits) {
        wait_for_unit = false;
    }
}

void TaskScavenge::EndTurn() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (Execute(*it)) {
            break;
        }
    }
}

bool TaskScavenge::Execute(UnitInfo& unit) {
    bool result;

    if (unit.IsReadyForOrders(this)) {
        UnitInfo* target = nullptr;
        int distance;
        int minimum_distance;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).IsVisibleToTeam(team) && ((*it).unit_type == SMLRUBLE || (*it).unit_type == LRGRUBLE)) {
                Point position((*it).grid_x, (*it).grid_y);

                distance = Access_GetDistance(&unit, &*it);

                if ((!target || distance < minimum_distance) &&
                    !Ai_IsDangerousLocation(&unit, position, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true) &&
                    Access_GetRemovableRubble(team, (*it).grid_x, (*it).grid_y)) {
                    target = &*it;
                    minimum_distance = distance;
                }
            }
        }

        if (target) {
            SmartPointer<Task> remove_rubble_task(new (std::nothrow) TaskRemoveRubble(this, target, 0x2100));

            remove_rubble_task->AddUnit(unit);

            TaskManager.AppendTask(*remove_rubble_task);

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskScavenge::RemoveSelf() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units.Clear();
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskScavenge::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }
