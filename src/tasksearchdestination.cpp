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

#include "tasksearchdestination.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "aiplayer.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "transportermap.hpp"
#include "units_manager.hpp"

TaskSearchDestination::TaskSearchDestination(Task* task, UnitInfo* unit_, int radius_)
    : Task(task->GetTeam(), task, task->GetFlags()) {
    field_62 = 0;
    unit = unit_;
    point1.x = unit->grid_x;
    point1.y = unit->grid_y;
    search_task = dynamic_cast<TaskAbstractSearch*>(task);
    search_radius = radius_;

    valid_sites = 0;
    searched_sites = 0;
    enterable_sites = 0;
}

TaskSearchDestination::~TaskSearchDestination() {}

int TaskSearchDestination::GetMemoryUse() const { return 4; }

char* TaskSearchDestination::WriteStatusLog(char* buffer) const {
    sprintf(buffer, "Find search square (radius %i)", radius);

    return buffer;
}

unsigned char TaskSearchDestination::GetType() const { return TaskType_TaskSearchDestination; }

void TaskSearchDestination::Begin() {
    Point position;

    unit->PushFrontTask1List(this);
    point3 = search_task->GetPoint();
    position.x = unit->grid_x;
    position.y = unit->grid_y;

    if (point3 == position) {
        --point3.y;
    }

    point2 = position;

    field_57 = 0;
    field_61 = 1;

    SearchNextCircle();
}

void TaskSearchDestination::EndTurn() {
    if (unit && unit->GetTask() == this) {
        ResumeSearch();
    }
}

void TaskSearchDestination::RemoveSelf() {
    if (unit) {
        unit->RemoveTask(this);
    }

    unit = nullptr;
    search_task = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskSearchDestination::FinishSearch() {
    if (unit) {
        unit->RemoveTask(this);
    }

    unit = nullptr;
    search_task = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskSearchDestination::SearchNextCircle() {
    Point position(unit->grid_x, unit->grid_y);
    int direction;

    while (field_61) {
        direction = UnitsManager_GetTargetAngle(point3.x - point2.x, point3.y - point2.y);

        point2 += Paths_8DirPointsArray[direction];

        if (point2 == point3) {
            point3.x = point3.x * 2 - position.x;
            point3.y = point3.y * 2 - position.y;
        }

        radius = TaskManager_GetDistance(position, point2) / 2;

        field_61 = true;

        if (field_57 != radius) {
            field_57 = radius;

            points[0] = point2;
            points[1] = point2;

            directions[0] = (direction + 2) & 0x7;
            directions[1] = (direction + 6) & 0x7;

            index = 0;
            field_59 = 1;
            field_61 = true;

            if (Search()) {
                return;
            }
        }
    }

    if (search_task) {
        search_task->TaskAbstractSearch_vfunc28(*unit);
    }

    FinishSearch();
}

bool TaskSearchDestination::Search() {
    Point position;
    Point site;
    Rect bounds;
    int loop_count = 0;
    ResourceID unit_type = INVALID_ID;
    short** damage_potential_map = nullptr;
    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    TransporterMap map(&*unit, 1, field_62 ? CAUTION_LEVEL_NONE : CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                       UnitsManager_BaseUnits[unit->unit_type].land_type & SURFACE_TYPE_WATER ? INVALID_ID : AIRTRANS);

    if (!field_62) {
        damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE, 1);
    }

    position.x = unit->grid_x;
    position.y = unit->grid_y;

    for (;;) {
        index = 1 - index;
        field_59 = -field_59;

        directions[index] = (directions[index] - (field_59 * 2)) & 0x7;

        for (int direction = 0;;) {
            directions[index] = (directions[index] + field_59) & 0x7;

            site = points[index];
            site += Paths_8DirPointsArray[directions[index]];

            if (direction++ < 8) {
                if (TaskManager_GetDistance(site, position) / 2 == radius) {
                    break;
                }

            } else {
                return false;
            }
        }

        points[index] = site;

        ++loop_count;

        if (Access_IsInsideBounds(&bounds, &points[index])) {
            field_61 = true;

            if (!damage_potential_map || damage_potential_map[points[index].x][points[index].y] == 0) {
                if (sub_3DFCF(&*unit, points[index])) {
                    if (map.Search(points[index])) {
                        SearchTrySite();

                        return true;
                    }
                }
            }
        }

        if (timer_get_stamp32() - Paths_LastTimeStamp < Paths_TimeLimit || loop_count <= 20) {
            if (points[index] == point2 || points[0] == points[1]) {
                return false;
            }

        } else {
            if (!GetField8()) {
                TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
            }

            return true;
        }
    }
}

void TaskSearchDestination::SearchTrySite() {
    Point Point_object1;
    Point site;
    Point best_site;
    Rect bounds;
    int direction;
    SmartList<UnitInfo>* units;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);
    Point_object1 = search_task->GetPoint();
    direction = UnitsManager_GetTargetAngle(points[index].x - Point_object1.x, points[index].y - Point_object1.y);
    site = points[index];
    best_site = site;

    site += Paths_8DirPointsArray[direction];

    for (int i = search_radius;
         i > 0 && Access_IsInsideBounds(&bounds, &site) &&
         (field_62 || !Ai_IsDangerousLocation(&*unit, site, CAUTION_LEVEL_AVOID_ALL_DAMAGE, 1)) &&
         sub_3DFCF(&*unit, site);
         --i) {
        best_site = site;
        site += Paths_8DirPointsArray[direction];
    }

    if (unit->flags & MOBILE_AIR_UNIT) {
        units = &UnitsManager_MobileAirUnits;

    } else {
        units = &UnitsManager_MobileLandSeaUnits;
    }

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).hits > 0 && (*it).unit_type == unit->unit_type && unit != (*it) &&
            ((*it).orders == ORDER_AWAIT || (*it).orders == ORDER_SENTRY || (*it).orders == ORDER_MOVE ||
             (*it).orders == ORDER_MOVE_TO_UNIT) &&
            Access_GetDistance(&*unit, best_site) > Access_GetDistance(&*it, best_site)) {
            bool flag = false;

            if ((*it).GetTask()) {
                if ((*it).GetTask()->DeterminePriority(flags) > 0) {
                    flag = unit->GetTask()->Task_vfunc1(*it);
                }

            } else {
                flag = true;
            }

            if (flag) {
                SmartPointer<UnitInfo> backup = unit;
                unit = &*it;
                unit->ClearFromTaskLists();
                search_task->AddUnit(*unit);
                unit->PushFrontTask1List(this);
                backup->RemoveTask(this, false);

                TaskManager.RemindAvailable(&*backup);
            }
        }
    }

    SmartPointer<Task> move_task =
        new (std::nothrow) TaskMove(&*unit, this, 0, field_62 ? CAUTION_LEVEL_NONE : CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                    best_site, &CloseMoveFinishedCallback);

    TaskManager.AppendTask(*move_task);
}

bool TaskSearchDestination::sub_3DFCF(UnitInfo* unit_, Point point) {
    bool result;

    ++searched_sites;

    if (!search_task->IsVisited(*unit_, point)) {
        ++enterable_sites;
        result = Access_IsAccessible(unit_->unit_type, team, point.x, point.y, 1) > 0;

    } else {
        result = false;
    }

    return result;
}

void TaskSearchDestination::ResumeSearch() {
    if (unit) {
        if (unit->IsReadyForOrders(this) && unit->speed > 0) {
            if (!field_62) {
                if (Task_RetreatFromDanger(this, &*unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                    FinishSearch();

                    return;

                } else if (Task_IsUnitDoomedToDestruction(&*unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                    field_62 = true;
                }
            }

            if (field_62 && (unit->grid_x != point1.x || unit->grid_y != point1.y)) {
                FinishSearch();

                return;
            }

            if ((points[index].x == point2.x && points[index].y == point2.y) ||
                (points[0].x == points[1].x && points[0].y == points[1].y) || !Search()) {
                SearchNextCircle();
            }

        } else {
            FinishSearch();
        }
    }
}

void TaskSearchDestination::CloseMoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskSearchDestination* search_task = dynamic_cast<TaskSearchDestination*>(task);

    if (result == TASKMOVE_RESULT_BLOCKED) {
        SmartPointer<Task> move_task = new (std::nothrow)
            TaskMove(unit, search_task, search_task->search_radius * search_task->search_radius,
                     CAUTION_LEVEL_AVOID_ALL_DAMAGE, search_task->points[search_task->index], &FarMoveFinishedCallback);

        TaskManager.AppendTask(*move_task);

    } else {
        search_task->FinishSearch();
    }
}

void TaskSearchDestination::FarMoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskSearchDestination* search_task = dynamic_cast<TaskSearchDestination*>(task);

    if (result == TASKMOVE_RESULT_BLOCKED) {
        search_task->ResumeSearch();

    } else {
        search_task->FinishSearch();
    }
}

void TaskSearchDestination::RemoveUnit(UnitInfo& unit_) {
    if (unit == unit_) {
        FinishSearch();
    }
}
