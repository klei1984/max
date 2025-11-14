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
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "ticktimer.hpp"
#include "transportermap.hpp"
#include "units_manager.hpp"

TaskSearchDestination::TaskSearchDestination(Task* task, UnitInfo* unit_, int32_t radius_)
    : Task(task->GetTeam(), task, task->GetPriority()) {
    index = 0;
    radius = 0;
    field_57 = 0;
    field_59 = 0;
    field_61 = 0;
    is_doomed = 0;
    unit = unit_;
    point1.x = unit_->grid_x;
    point1.y = unit_->grid_y;
    search_task = dynamic_cast<TaskAbstractSearch*>(task);
    search_radius = radius_;

    valid_sites = 0;
    searched_sites = 0;
    enterable_sites = 0;
}

TaskSearchDestination::~TaskSearchDestination() {}

char* TaskSearchDestination::WriteStatusLog(char* buffer) const {
    sprintf(buffer, "Find search square (radius %i)", radius);

    return buffer;
}

uint8_t TaskSearchDestination::GetType() const { return TaskType_TaskSearchDestination; }

void TaskSearchDestination::Init() {
    Point position;

    AILOG(log, "Search destination begin.");

    unit->AddTask(this);
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
    AILOG(log, "Search destination: finished.");

    AILOG_LOG(log, "Calls: valid {}, searched {}, can enter {}", valid_sites, searched_sites, enterable_sites);

    if (unit) {
        unit->RemoveTask(this);

    } else {
        AILOG_LOG(log, "Unit is null.");
    }

    unit = nullptr;
    search_task = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskSearchDestination::SearchNextCircle() {
    Point position(unit->grid_x, unit->grid_y);
    int32_t direction;

    AILOG(log, "Search destination: NextCircle");

    while (field_61) {
        direction = UnitsManager_GetTargetAngle(point3.x - point2.x, point3.y - point2.y);

        point2 += Paths_8DirPointsArray[direction];

        if (point2 == point3) {
            point3.x = point3.x * 2 - position.x;
            point3.y = point3.y * 2 - position.y;
        }

        radius = Access_GetApproximateDistance(position, point2) / 2;

        AILOG_LOG(log, "Radius {}", radius);

        field_61 = true;

        if (field_57 != radius) {
            field_57 = radius;

            points[0] = point2;
            points[1] = point2;

            directions[0] = (direction + 2) & 0x7;
            directions[1] = (direction + 6) & 0x7;

            index = 0;
            field_59 = 1;
            field_61 = false;

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
    int32_t loop_count = 0;
    ResourceID unit_type = INVALID_ID;
    int16_t** damage_potential_map = nullptr;
    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    TransporterMap map(
        &*unit, 1, is_doomed ? CAUTION_LEVEL_NONE : CAUTION_LEVEL_AVOID_ALL_DAMAGE,
        UnitsManager_BaseUnits[unit->GetUnitType()].land_type & SURFACE_TYPE_WATER ? INVALID_ID : AIRTRANS);

    if (!is_doomed) {
        damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true);
    }

    position.x = unit->grid_x;
    position.y = unit->grid_y;

    for (;;) {
        index = 1 - index;
        field_59 = -field_59;

        directions[index] = (directions[index] - (field_59 * 2)) & 0x7;

        for (int32_t direction = 0;;) {
            directions[index] = (directions[index] + field_59) & 0x7;

            site = points[index];
            site += Paths_8DirPointsArray[directions[index]];

            if (direction++ < 8) {
                if (Access_GetApproximateDistance(site, position) / 2 == radius) {
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

            if (!damage_potential_map || damage_potential_map[points[index].x][points[index].y] < 1) {
                if (sub_3DFCF(&*unit, points[index])) {
                    if (map.Search(points[index])) {
                        SearchTrySite();

                        return true;
                    }
                }
            }
        }

        if (TickTimer_HaveTimeToThink() || loop_count <= 20) {
            if (points[index] == point2 || points[0] == points[1]) {
                return false;
            }

        } else {
            AILOG(log, "Search paused, {} msecs since frame update", TickTimer_GetElapsedTime());

            if (!IsScheduledForTurnEnd()) {
                AILOG_LOG(log, "Adding end turn reminder");

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
    int32_t direction;
    SmartList<UnitInfo>* units;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    AILOG(log, "Search destination: try [{},{}]", points[index].x + 1, points[index].y + 1);

    Point_object1 = search_task->GetPoint();
    direction = UnitsManager_GetTargetAngle(points[index].x - Point_object1.x, points[index].y - Point_object1.y);
    site = points[index];
    best_site = site;

    site += Paths_8DirPointsArray[direction];

    for (int32_t i = search_radius;
         i > 0 && Access_IsInsideBounds(&bounds, &site) &&
         (is_doomed || !Ai_IsDangerousLocation(&*unit, site, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true)) &&
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
        if ((*it).team == team && (*it).hits > 0 && (*it).GetUnitType() == unit->GetUnitType() && unit != (*it) &&
            ((*it).GetOrder() == ORDER_AWAIT || (*it).GetOrder() == ORDER_SENTRY || (*it).GetOrder() == ORDER_MOVE ||
             (*it).GetOrder() == ORDER_MOVE_TO_UNIT) &&
            Access_GetSquaredDistance(&*unit, best_site) > Access_GetSquaredDistance(&*it, best_site)) {
            bool flag = false;

            if ((*it).GetTask()) {
                if ((*it).GetTask()->ComparePriority(base_priority) > 0) {
                    flag = unit->GetTask()->IsUnitTransferable(*it);
                }

            } else {
                flag = true;
            }

            if (flag) {
                SmartPointer<UnitInfo> backup = unit;
                unit = &*it;

                AILOG_LOG(log, "Swapping for {} at [{},{}]",
                          UnitsManager_BaseUnits[unit->GetUnitType()].GetSingularName(), unit->grid_x + 1,
                          unit->grid_y + 1);

                unit->RemoveTasks();
                search_task->AddUnit(*unit);
                unit->AddTask(this);
                backup->RemoveTask(this, false);

                TaskManager.ClearUnitTasksAndRemindAvailable(&*backup);
            }
        }
    }

    SmartPointer<Task> move_task =
        new (std::nothrow) TaskMove(&*unit, this, 0, is_doomed ? CAUTION_LEVEL_NONE : CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                                    best_site, &CloseMoveFinishedCallback);

    TaskManager.AppendTask(*move_task);
}

bool TaskSearchDestination::sub_3DFCF(UnitInfo* unit_, Point point) {
    bool result;

    ++searched_sites;

    if (!search_task->IsVisited(*unit_, point)) {
        ++enterable_sites;
        result =
            Access_IsAccessible(unit_->GetUnitType(), team, point.x, point.y, AccessModifier_EnemySameClassBlocks) > 0;

    } else {
        result = false;
    }

    return result;
}

void TaskSearchDestination::ResumeSearch() {
    AILOG(log, "Search destination: resume search.");

    if (unit) {
        if (unit->IsReadyForOrders(this) && unit->speed > 0) {
            AILOG_LOG(log, "Client unit: {} at [{},{}]", UnitsManager_BaseUnits[unit->GetUnitType()].GetSingularName(),
                      unit->grid_x + 1, unit->grid_y + 1);

            if (!is_doomed) {
                if (Task_RetreatFromDanger(this, &*unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                    AILOG_LOG(log, "Retreating from danger.");

                    FinishSearch();

                    return;

                } else if (Task_IsUnitDoomedToDestruction(&*unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
                    is_doomed = true;
                }
            }

            if (is_doomed) {
                AILOG_LOG(log, "Unit is doomed to destruction.");
            }

            if (is_doomed && (unit->grid_x != point1.x || unit->grid_y != point1.y)) {
                AILOG_LOG(log, "Unit has moved.");

                FinishSearch();

                return;
            }

            if ((points[index].x == point2.x && points[index].y == point2.y) ||
                (points[0].x == points[1].x && points[0].y == points[1].y) || !Search()) {
                SearchNextCircle();
            }

        } else {
            AILOG_LOG(log, "Not ready for orders, or no movement left.");

            FinishSearch();
        }

    } else {
        AILOG_LOG(log, "Unit is null.");
    }
}

void TaskSearchDestination::CloseMoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    TaskSearchDestination* search_task = dynamic_cast<TaskSearchDestination*>(task);

    AILOG(log, "Search destination: close result");

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

    AILOG(log, "Search destination: far result");

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
