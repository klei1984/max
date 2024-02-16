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

#include "taskretreat.hpp"

#include "access.hpp"
#include "aiplayer.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "ticktimer.hpp"
#include "transportermap.hpp"

TaskRetreat::TaskRetreat(UnitInfo* unit, Task* task, uint16_t flags, int32_t caution_level_)
    : Task(unit->team, task, flags), unit_to_retreat(unit), caution_level(caution_level_) {}

TaskRetreat::~TaskRetreat() {}

bool TaskRetreat::Task_vfunc1(UnitInfo& unit) { return unit_to_retreat != unit; }

char* TaskRetreat::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Retreat from danger");

    return buffer;
}

uint8_t TaskRetreat::GetType() const { return TaskType_TaskRetreat; }

void TaskRetreat::Begin() {
    field_23 = 0;
    position.x = unit_to_retreat->grid_x;
    position.y = unit_to_retreat->grid_y;
    direction = 6;
    field_31 = 0;
    field_34 = 1;

    for (SmartList<Task>::Iterator it = unit_to_retreat->GetTasks().Begin(); it != unit_to_retreat->GetTasks().End();
         ++it) {
        if ((*it).GetType() == TaskType_TaskRetreat) {
            TaskManager.RemoveTask(*this);
            return;
        }
    }

    unit_to_retreat->AddTask(this);

    RemindTurnEnd(true);
}

void TaskRetreat::EndTurn() {
    if (unit_to_retreat != nullptr && unit_to_retreat->IsReadyForOrders(this)) {
        Point site(unit_to_retreat->grid_x, unit_to_retreat->grid_y);

        if (AiPlayer_Teams[team].GetDamagePotential(&*unit_to_retreat, site, caution_level, false)) {
            Search();

        } else {
            RemoveTask();
        }
    }
}

void TaskRetreat::RemoveSelf() {
    if (unit_to_retreat != nullptr) {
        unit_to_retreat->RemoveTask(this);
        unit_to_retreat = nullptr;
        parent = nullptr;

        TaskManager.RemoveTask(*this);
    }
}

void TaskRetreat::RemoveTask() {
    if (unit_to_retreat != nullptr) {
        unit_to_retreat->RemoveTask(this, false);
        unit_to_retreat = nullptr;
        parent = nullptr;

        TaskManager.RemoveTask(*this);
    }
}

void TaskRetreat::Search() {
    int16_t** damage_potential_map;
    Rect bounds;
    int32_t unit_hits;
    int32_t index;

    damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*unit_to_retreat, caution_level, false);

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    unit_hits = unit_to_retreat->hits;

    TransporterMap transporter_map(&*unit_to_retreat, 0x01, CAUTION_LEVEL_NONE);

    index = 0;

    if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
        unit_hits = 1;
    }

    if (damage_potential_map) {
        while (TickTimer_HaveTimeToThink() || index < 20) {
            ++field_31;

            if (field_31 >= field_23) {
                direction += 2;

                if (direction >= 8) {
                    field_23 += 2;

                    if (field_34 == 0 || field_23 > unit_to_retreat->GetBaseValues()->GetAttribute(ATTRIB_SPEED) * 2) {
                        unit_to_retreat->ChangeField221(0x01, true);
                        RemoveTask();

                        return;
                    }

                    --position.x;
                    ++position.y;

                    direction = 0;

                    field_34 = 0;
                }

                field_31 = 0;
            }

            position += Paths_8DirPointsArray[direction];

            ++index;

            if (Access_IsInsideBounds(&bounds, &position)) {
                field_34 = 1;

                if (damage_potential_map[position.x][position.y] < unit_hits) {
                    if (Access_IsAccessible(unit_to_retreat->unit_type, team, position.x, position.y, 0x02)) {
                        if (transporter_map.Search(position)) {
                            SmartPointer<TaskMove> task_move(new (std::nothrow) TaskMove(
                                &*unit_to_retreat, this, 0, CAUTION_LEVEL_NONE, position, &TaskMoveResultCallback));
                            task_move->SetField68(true);

                            TaskManager.AppendTask(*task_move);

                            return;
                        }
                    }
                }
            }
        }

        RemindTurnEnd(true);

    } else {
        RemoveTask();
    }
}

void TaskRetreat::TaskMoveResultCallback(Task* task, UnitInfo* unit, char result) {
    TaskRetreat* retreat_task = dynamic_cast<TaskRetreat*>(task);

    if (retreat_task->unit_to_retreat != nullptr) {
        if (result == 3 || result == 0) {
            retreat_task->RemoveTask();

        } else {
            retreat_task->Search();
        }
    }
}

void TaskRetreat::RemoveUnit(UnitInfo& unit) { RemoveTask(); }
