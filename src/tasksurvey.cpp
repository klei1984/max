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

#include "tasksurvey.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "taskobtainunits.hpp"
#include "units_manager.hpp"

void TaskSurvey::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    if (result == TASKMOVE_RESULT_BLOCKED) {
        dynamic_cast<TaskSurvey*>(task)->FindDestination(*unit, 1);

    } else if (result == TASKMOVE_RESULT_SUCCESS) {
        Task_RemindMoveFinished(unit);
    }
}

TaskSurvey::TaskSurvey(unsigned short team_, Point point_) : TaskAbstractSearch(team_, nullptr, 0x2200, point_) {}

TaskSurvey::~TaskSurvey() {}

int TaskSurvey::GetMemoryUse() const { return units.GetMemorySize() - 6; }

char* TaskSurvey::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Survey");

    return buffer;
}

unsigned char TaskSurvey::GetType() const { return TaskType_TaskSurvey; }

bool TaskSurvey::Execute(UnitInfo& unit) {
    bool result;

    AiLog log("Survey: move finished.");

    if (unit.IsReadyForOrders(this)) {
        if (unit.speed) {
            bool location_found = false;
            Point best_site;
            Point site;
            unsigned short hash_team_id = UnitsManager_TeamInfo[team].team_units->hash_team_id;
            short** damage_potential_map = nullptr;
            unsigned short cargo_at_site;
            Rect bounds;
            int distance;
            int minimum_distance{INT32_MAX};

            bounds.ulx = std::max(1, unit.grid_x - 5);
            bounds.lrx = std::min(ResourceManager_MapSize.x - 1, unit.grid_x + 6);
            bounds.uly = std::max(1, unit.grid_y - 5);
            bounds.lry = std::min(ResourceManager_MapSize.y - 1, unit.grid_y + 6);

            damage_potential_map =
                AiPlayer_Teams[team].GetDamagePotentialMap(&unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true);

            for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                    cargo_at_site = ResourceManager_CargoMap[ResourceManager_MapSize.x * site.y + site.x];

                    if ((cargo_at_site & hash_team_id) && (cargo_at_site & 0x1F)) {
                        distance = Access_GetDistance(&unit, site);

                        if (!location_found || distance < minimum_distance) {
                            if (damage_potential_map[site.x][site.y] <= 0) {
                                if (Access_IsAccessible(SURVEYOR, team, site.x, site.y, 1)) {
                                    for (int index_x = site.x - 1; index_x <= site.x + 1; ++index_x) {
                                        for (int index_y = site.y - 1; index_y <= site.y + 1; ++index_y) {
                                            cargo_at_site =
                                                ResourceManager_CargoMap[ResourceManager_MapSize.x * index_y + index_x];

                                            if (!(cargo_at_site & hash_team_id) &&
                                                Access_GetSurfaceType(index_x, index_y) != SURFACE_TYPE_AIR) {
                                                location_found = true;
                                                best_site = site;
                                                minimum_distance = distance;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (location_found) {
                SmartPointer<Task> move_task = new (std::nothrow)
                    TaskMove(&unit, this, 0, CAUTION_LEVEL_AVOID_ALL_DAMAGE, best_site, &MoveFinishedCallback);

                TaskManager.AppendTask(*move_task);

            } else {
                FindDestination(unit, 1);
            }

            result = true;

        } else {
            result = false;
        }

    } else {
        log.Log("Not ready for orders.");

        result = false;
    }

    return result;
}

bool TaskSurvey::IsVisited(UnitInfo& unit, Point point) {
    return ResourceManager_CargoMap[ResourceManager_MapSize.x * point.y + point.x] &
           UnitsManager_TeamInfo[team].team_units->hash_team_id;
}

void TaskSurvey::ObtainUnit() {
    SmartPointer<TaskObtainUnits> obtain_units_task = new (std::nothrow) TaskObtainUnits(this, point);
    obtain_units_task->AddUnit(SURVEYOR);

    TaskManager.AppendTask(*obtain_units_task);
}
