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

#include "taskautosurvey.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "game_manager.hpp"
#include "task_manager.hpp"
#include "ticktimer.hpp"
#include "transportermap.hpp"
#include "units_manager.hpp"

TaskAutoSurvey::TaskAutoSurvey(UnitInfo* unit_) : Task(unit_->team, nullptr, 0x2200) {
    unit = unit_;
    central_site.x = unit_->grid_x;
    central_site.y = unit_->grid_y;
    radius = 0;

    direction = 0;
    direction2 = 0;

    continue_search = false;
    location_found = false;

    current_radius = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == MININGST) {
            central_site.x = (*it).grid_x;
            central_site.y = (*it).grid_y;
        }
    }
}

TaskAutoSurvey::~TaskAutoSurvey() {}

char* TaskAutoSurvey::WriteStatusLog(char* buffer) const {
    sprintf(buffer, "Auto Survey (radius %i)", radius);

    return buffer;
}

uint8_t TaskAutoSurvey::GetType() const { return TaskType_TaskAutoSurvey; }

void TaskAutoSurvey::Begin() { unit->AddTask(this); }

bool TaskAutoSurvey::Execute(UnitInfo& unit_) {
    bool result;

    if (unit == unit_ && unit->IsReadyForOrders(this) &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) && unit->speed > 0) {
        AiLog log("Auto survey: Move %s at [%i,%i]", UnitsManager_BaseUnits[unit->GetUnitType()].singular_name,
                  unit->grid_x + 1, unit->grid_y + 1);

        uint16_t hash_team_id = UnitsManager_TeamInfo[team].team_units->hash_team_id;
        TransporterMap map(&*unit, 2, CAUTION_LEVEL_AVOID_ALL_DAMAGE);

        if (radius) {
            int32_t loop_count = 0;
            Point Point_object1;
            int32_t target_direction =
                UnitsManager_GetTargetAngle(unit->grid_x - central_site.x, unit->grid_y - central_site.y);

            while (TickTimer_HaveTimeToThink() || loop_count < 20) {
                ++loop_count;

                position += Paths_8DirPointsArray[direction];
                ++current_radius;

                if (current_radius >= radius * 2) {
                    current_radius = 0;
                    direction += 2;

                    if (direction >= 8) {
                        if (location_found) {
                            unit->target_grid_x = destination.x;
                            unit->target_grid_y = destination.y;

                            radius = 0;

                            UnitsManager_SetNewOrder(&*unit, ORDER_MOVE, ORDER_STATE_INIT);

                            result = true;

                            return result;

                        } else if (continue_search) {
                            direction = 0;
                            continue_search = false;
                            ++radius;
                            --position.x;
                            ++position.y;

                        } else {
                            unit->auto_survey = false;
                            unit->RemoveTasks();

                            result = false;

                            return result;
                        }
                    }
                }

                if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                    position.y < ResourceManager_MapSize.y) {
                    continue_search = true;

                    if (!(ResourceManager_CargoMap[ResourceManager_MapSize.x * position.y + position.x] &
                          hash_team_id)) {
                        int32_t direction_difference =
                            UnitsManager_GetTargetAngle(position.x - unit->grid_x, position.y - unit->grid_y) -
                            target_direction;

                        direction_difference = (direction_difference + 8) & 7;

                        if (direction_difference > 4) {
                            direction_difference = 8 - direction_difference;
                        }

                        if (!location_found || direction_difference > direction2) {
                            if (map.Search(position)) {
                                Point backup_position;

                                location_found = true;
                                destination = position;
                                direction2 = direction_difference;
                                backup_position = position;

                                direction_difference = UnitsManager_GetTargetAngle(position.x - central_site.x,
                                                                                   position.y - central_site.y);

                                backup_position += Paths_8DirPointsArray[direction_difference];

                                if (backup_position.x >= 0 && backup_position.x < ResourceManager_MapSize.x &&
                                    backup_position.y >= 0 && backup_position.y < ResourceManager_MapSize.y) {
                                    if (map.Search(backup_position)) {
                                        destination = backup_position;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            log.Log("Search paused, %i msecs since frame update", TickTimer_GetElapsedTime());

            result = false;

        } else {
            Rect bounds;
            int32_t distance;
            int32_t minimum_distance{INT32_MAX};

            bounds.ulx = std::max(1, unit->grid_x - 5);
            bounds.lrx = std::min(ResourceManager_MapSize.x - 1, unit->grid_x + 6);
            bounds.uly = std::max(1, unit->grid_y - 5);
            bounds.lry = std::min(ResourceManager_MapSize.y - 1, unit->grid_y + 6);

            location_found = false;

            for (position.x = bounds.ulx; position.x < bounds.lrx; ++position.x) {
                for (position.y = bounds.uly; position.y < bounds.lry; ++position.y) {
                    uint16_t cargo_at_site =
                        ResourceManager_CargoMap[ResourceManager_MapSize.x * position.y + position.x];

                    if ((cargo_at_site & hash_team_id) && (cargo_at_site & 0x1F)) {
                        distance = Access_GetDistance(&*unit, position);

                        if (!location_found || minimum_distance > distance) {
                            for (int32_t index_x = position.x - 1; index_x <= position.x + 1; ++index_x) {
                                for (int32_t index_y = position.y - 1; index_y <= position.y + 1; ++index_y) {
                                    cargo_at_site =
                                        ResourceManager_CargoMap[ResourceManager_MapSize.x * index_y + index_x];

                                    if (!(cargo_at_site & hash_team_id) &&
                                        Access_GetSurfaceType(index_x, index_y) != SURFACE_TYPE_AIR) {
                                        if (map.Search(position)) {
                                            location_found = true;
                                            destination = position;
                                            minimum_distance = distance;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (location_found) {
                unit->target_grid_x = destination.x;
                unit->target_grid_y = destination.y;

                radius = 0;

                UnitsManager_SetNewOrder(&*unit, ORDER_MOVE, ORDER_STATE_INIT);

            } else {
                position.x = unit->grid_x - 1;
                position.y = unit->grid_y + 1;
                radius = 1;
                direction = 0;
                current_radius = 0;
                continue_search = false;
            }

            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskAutoSurvey::RemoveSelf() {
    if (unit) {
        unit->RemoveTasks();
    }

    unit = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskAutoSurvey::RemoveUnit(UnitInfo& unit_) {
    if (unit == unit_) {
        unit = nullptr;

        TaskManager.RemoveTask(*this);
    }
}
