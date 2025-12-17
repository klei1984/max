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

#include <format>

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "ticktimer.hpp"
#include "transportermap.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

TaskSearchDestination::TaskSearchDestination(Task* task, UnitInfo* unit_, int32_t radius_)
    : Task(task->GetTeam(), task, task->GetPriority()) {
    index = 0;
    radius = 0;
    previous_radius = 0;
    spiral_direction = 0;
    continue_search_at_radius = 0;
    is_doomed = 0;
    unit = unit_;
    initial_position.x = unit_->grid_x;
    initial_position.y = unit_->grid_y;
    search_task = dynamic_cast<TaskAbstractSearch*>(task);
    search_radius = radius_;

    valid_sites = 0;
    searched_sites = 0;
    enterable_sites = 0;
}

TaskSearchDestination::~TaskSearchDestination() {}

std::string TaskSearchDestination::WriteStatusLog() const {
    return std::format("Find search square (radius {})", radius);
}

uint8_t TaskSearchDestination::GetType() const { return TaskType_TaskSearchDestination; }

void TaskSearchDestination::Init() {
    Point position;

    AILOG(log, "Search destination begin.");

    unit->AddTask(this);
    spiral_target = search_task->GetPoint();
    position.x = unit->grid_x;
    position.y = unit->grid_y;

    if (spiral_target == position) {
        --spiral_target.y;
    }

    spiral_center = position;

    previous_radius = 0;
    continue_search_at_radius = 1;

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
    m_parent = nullptr;

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
    m_parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskSearchDestination::SearchNextCircle() {
    Point position(unit->grid_x, unit->grid_y);
    int32_t direction;

    AILOG(log, "Search destination: NextCircle");

    while (continue_search_at_radius) {
        direction = UnitsManager_GetTargetAngle(spiral_target.x - spiral_center.x, spiral_target.y - spiral_center.y);

        spiral_center += DIRECTION_OFFSETS[direction];

        if (spiral_center == spiral_target) {
            spiral_target.x = spiral_target.x * 2 - position.x;
            spiral_target.y = spiral_target.y * 2 - position.y;
        }

        radius = Access_GetApproximateDistance(position, spiral_center) / 2;

        AILOG_LOG(log, "Radius {}", radius);

        continue_search_at_radius = true;

        if (previous_radius != radius) {
            previous_radius = radius;

            spiral_cursors[0] = spiral_center;
            spiral_cursors[1] = spiral_center;

            directions[0] = (direction + 2) & 0x7;
            directions[1] = (direction + 6) & 0x7;

            index = 0;
            spiral_direction = 1;
            continue_search_at_radius = false;

            if (Search()) {
                return;
            }
        }
    }

    if (search_task) {
        search_task->OnUnitReleased(*unit);
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
        ResourceManager_GetUnit(unit->GetUnitType()).GetLandType() & SURFACE_TYPE_WATER ? INVALID_ID : AIRTRANS);

    if (!is_doomed) {
        damage_potential_map =
            AiPlayer_Teams[m_team].GetDamagePotentialMap(&*unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true);
    }

    position.x = unit->grid_x;
    position.y = unit->grid_y;

    for (;;) {
        index = 1 - index;
        spiral_direction = -spiral_direction;

        directions[index] = (directions[index] - (spiral_direction * 2)) & 0x7;

        for (int32_t direction = 0;;) {
            directions[index] = (directions[index] + spiral_direction) & 0x7;

            site = spiral_cursors[index];
            site += DIRECTION_OFFSETS[directions[index]];

            if (direction++ < 8) {
                if (Access_GetApproximateDistance(site, position) / 2 == radius) {
                    break;
                }

            } else {
                return false;
            }
        }

        spiral_cursors[index] = site;

        ++loop_count;

        if (Access_IsInsideBounds(&bounds, &spiral_cursors[index])) {
            continue_search_at_radius = true;

            if (!damage_potential_map || damage_potential_map[spiral_cursors[index].x][spiral_cursors[index].y] < 1) {
                if (sub_3DFCF(&*unit, spiral_cursors[index])) {
                    if (map.Search(spiral_cursors[index])) {
                        SearchTrySite();

                        return true;
                    }
                }
            }
        }

        if (TickTimer_HaveTimeToThink() || loop_count <= 20) {
            if (spiral_cursors[index] == spiral_center || spiral_cursors[0] == spiral_cursors[1]) {
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

    AILOG(log, "Search destination: try [{},{}]", spiral_cursors[index].x + 1, spiral_cursors[index].y + 1);

    Point_object1 = search_task->GetPoint();
    direction = UnitsManager_GetTargetAngle(spiral_cursors[index].x - Point_object1.x,
                                            spiral_cursors[index].y - Point_object1.y);
    site = spiral_cursors[index];
    best_site = site;

    site += DIRECTION_OFFSETS[direction];

    for (int32_t i = search_radius;
         i > 0 && Access_IsInsideBounds(&bounds, &site) &&
         (is_doomed || !Ai_IsDangerousLocation(&*unit, site, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true)) &&
         sub_3DFCF(&*unit, site);
         --i) {
        best_site = site;
        site += DIRECTION_OFFSETS[direction];
    }

    if (unit->flags & MOBILE_AIR_UNIT) {
        units = &UnitsManager_MobileAirUnits;

    } else {
        units = &UnitsManager_MobileLandSeaUnits;
    }

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == m_team && (*it).hits > 0 && (*it).GetUnitType() == unit->GetUnitType() && unit != (*it) &&
            ((*it).GetOrder() == ORDER_AWAIT || (*it).GetOrder() == ORDER_SENTRY || (*it).GetOrder() == ORDER_MOVE ||
             (*it).GetOrder() == ORDER_MOVE_TO_UNIT) &&
            Access_GetSquaredDistance(&*unit, best_site) > Access_GetSquaredDistance(&*it, best_site)) {
            bool flag = false;

            if ((*it).GetTask()) {
                if ((*it).GetTask()->ComparePriority(m_base_priority) > 0) {
                    flag = unit->GetTask()->IsUnitTransferable(*it);
                }

            } else {
                flag = true;
            }

            if (flag) {
                SmartPointer<UnitInfo> backup = unit;
                unit = &*it;

                AILOG_LOG(log, "Swapping for {} at [{},{}]",
                          ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data(), unit->grid_x + 1,
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
        result = Access_IsAccessible(unit_->GetUnitType(), m_team, point.x, point.y,
                                     AccessModifier_EnemySameClassBlocks) > 0;

    } else {
        result = false;
    }

    return result;
}

void TaskSearchDestination::ResumeSearch() {
    AILOG(log, "Search destination: resume search.");

    if (unit) {
        if (unit->IsReadyForOrders(this) && unit->speed > 0) {
            AILOG_LOG(log, "Client unit: {} at [{},{}]",
                      ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data(), unit->grid_x + 1,
                      unit->grid_y + 1);

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

            if (is_doomed && (unit->grid_x != initial_position.x || unit->grid_y != initial_position.y)) {
                AILOG_LOG(log, "Unit has moved.");

                FinishSearch();

                return;
            }

            if ((spiral_cursors[index].x == spiral_center.x && spiral_cursors[index].y == spiral_center.y) ||
                (spiral_cursors[0].x == spiral_cursors[1].x && spiral_cursors[0].y == spiral_cursors[1].y) ||
                !Search()) {
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
        SmartPointer<Task> move_task = new (std::nothrow) TaskMove(
            unit, search_task, search_task->search_radius * search_task->search_radius, CAUTION_LEVEL_AVOID_ALL_DAMAGE,
            search_task->spiral_cursors[search_task->index], &FarMoveFinishedCallback);

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
