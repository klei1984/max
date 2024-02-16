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

#include "taskclearzone.hpp"

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "remote.hpp"
#include "task_manager.hpp"
#include "taskfindpath.hpp"
#include "taskpathrequest.hpp"
#include "ticktimer.hpp"
#include "units_manager.hpp"

void TaskClearZone::PathFindResultCallback(Task* task, PathRequest* request, Point point, GroundPath* path,
                                           uint8_t result) {
    TaskClearZone* clear_zone = dynamic_cast<TaskClearZone*>(task);
    AiLog log("Clear zone: path result.");

    if (path && (!clear_zone->moving_unit || clear_zone->moving_unit->IsReadyForOrders(clear_zone)) &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == clear_zone->team)) {
        SDL_assert(clear_zone->moving_unit != nullptr);

        clear_zone->state = CLEARZONE_STATE_MOVING_UNIT;

        if (clear_zone->moving_unit->flags & MOBILE_AIR_UNIT) {
            UnitsManager_SetNewOrder(&*clear_zone->moving_unit, ORDER_MOVE, ORDER_STATE_INIT);

        } else {
            clear_zone->moving_unit->path = path;
            clear_zone->moving_unit->Redraw();

            UnitsManager_SetNewOrder(&*clear_zone->moving_unit, ORDER_MOVE, ORDER_STATE_IN_PROGRESS);

            if (Remote_IsNetworkGame) {
                Remote_SendNetPacket_38(&*clear_zone->moving_unit);
            }
        }

    } else {
        if (clear_zone->moving_unit) {
            clear_zone->moving_unit->RemoveTask(clear_zone);

            if (!clear_zone->moving_unit->GetTask()) {
                TaskManager.RemindAvailable(&*clear_zone->moving_unit);
            }
        }

        clear_zone->moving_unit = nullptr;

        clear_zone->state = CLEARZONE_STATE_WAITING;

        clear_zone->RemindTurnEnd();
    }
}

void TaskClearZone::PathFindCancelCallback(Task* task, PathRequest* request) {
    TaskClearZone* clear_zone = dynamic_cast<TaskClearZone*>(task);
    AiLog log("Clear zone: path cancelled.");

    if (clear_zone->moving_unit) {
        clear_zone->moving_unit->RemoveTask(clear_zone);

        if (!clear_zone->moving_unit->GetTask()) {
            TaskManager.RemindAvailable(&*clear_zone->moving_unit);
        }

        clear_zone->moving_unit = nullptr;

        clear_zone->state = CLEARZONE_STATE_WAITING;

        clear_zone->RemindTurnEnd();
    }
}

TaskClearZone::TaskClearZone(uint16_t team, uint32_t flags_)
    : Task(team, nullptr, 0), unit_flags(flags_), state(CLEARZONE_STATE_WAITING) {}

TaskClearZone::~TaskClearZone() {}

char* TaskClearZone::WriteStatusLog(char* buffer) const {
    if (unit_flags == MOBILE_AIR_UNIT) {
        strcpy(buffer, "Clear air zones: ");

    } else {
        strcpy(buffer, "Clear land / sea zones: ");
    }

    switch (state) {
        case CLEARZONE_STATE_WAITING: {
            strcat(buffer, "waiting");
        } break;

        case CLEARZONE_STATE_EXAMINING_ZONES: {
            strcat(buffer, "examining zones");
        } break;

        case CLEARZONE_STATE_SEARCHING_MAP: {
            strcat(buffer, "searching map");
        } break;

        case CLEARZONE_STATE_WAITING_FOR_PATH: {
            strcat(buffer, "waiting for path");
        } break;

        case CLEARZONE_STATE_MOVING_UNIT: {
            strcat(buffer, "moving ");
            strcat(buffer, UnitsManager_BaseUnits[moving_unit->unit_type].singular_name);
        } break;
    }

    return buffer;
}

uint8_t TaskClearZone::GetType() const { return TaskType_TaskClearZone; }

bool TaskClearZone::IsThinking() { return state != CLEARZONE_STATE_WAITING && state != CLEARZONE_STATE_MOVING_UNIT; }

void TaskClearZone::Begin() {
    AiLog log("Clear Zone: Begin.");

    RemindTurnEnd();
}

void TaskClearZone::BeginTurn() {
    AiLog log("Clear Zone: Begin Turn.");

    EndTurn();
}

void TaskClearZone::EndTurn() {
    AiLog log("Clear Zone: End Turn.");

    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
        if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
            if (state != CLEARZONE_STATE_EXAMINING_ZONES && state != CLEARZONE_STATE_WAITING_FOR_PATH) {
                if (state == CLEARZONE_STATE_MOVING_UNIT) {
                    state = CLEARZONE_STATE_WAITING;

                    if (moving_unit) {
                        log.Log("Clear Zone: Move Finished for %s at [%i,%i].",
                                UnitsManager_BaseUnits[moving_unit->unit_type].singular_name, moving_unit->grid_x + 1,
                                moving_unit->grid_y + 1);

                        moving_unit->RemoveTask(this);

                        if (!moving_unit->GetTask()) {
                            TaskManager.RemindAvailable(&*moving_unit);
                        }

                        moving_unit = nullptr;
                    }
                }

                if (state == CLEARZONE_STATE_WAITING) {
                    while (!ExamineZones()) {
                        if (TickTimer_HaveTimeToThink()) {
                            state = CLEARZONE_STATE_WAITING;

                            RemindTurnEnd(true);

                            return;
                        }
                    }
                }

                SearchMap();
            }
        }
    }
}

bool TaskClearZone::Execute(UnitInfo& unit) {
    if (moving_unit == unit) {
        EndTurn();
    }

    return true;
}

void TaskClearZone::RemoveSelf() {
    zones.Release();

    if (moving_unit != nullptr) {
        moving_unit->RemoveTask(this);
        moving_unit = nullptr;
    }

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskClearZone::RemoveUnit(UnitInfo& unit) {
    if (moving_unit == unit) {
        AiLog log("Clear Zone: Remove %s at [%i,%i].", UnitsManager_BaseUnits[unit.unit_type].singular_name,
                  unit.grid_x + 1, unit.grid_y + 1);

        moving_unit = nullptr;
        state = CLEARZONE_STATE_WAITING;
    }
}

bool TaskClearZone::ExamineZones() {
    Point site;
    SmartPointer<Zone> zone;
    AiLog log("Clear Zone: Examine Zones.");

    state = CLEARZONE_STATE_EXAMINING_ZONES;

    points1.Clear();
    zone_squares.Clear();
    points2.Clear();

    uint8_t** info_map = AiPlayer_Teams[team].GetInfoMap();

    if (info_map) {
        for (int32_t i = 0; i < zones.GetCount(); ++i) {
            zone = zones[i];

            for (int32_t j = 0; j < zone->points.GetCount(); ++j) {
                site = *zone->points[j];

                info_map[site.x][site.y] &= ~0x08;
            }
        }
    }

    for (int32_t i = 0; i < zones.GetCount(); ++i) {
        bool is_found = false;

        zone = zones[i];

        for (int32_t j = 0; j < zone->points.GetCount(); ++j) {
            site = *zone->points[j];

            if (info_map) {
                info_map[site.x][site.y] |= 0x08;
            }

            UnitInfo* unit = Access_GetTeamUnit(site.x, site.y, team, unit_flags);

            if (unit && zone->unit != unit) {
                is_found = true;
            }

            if (IsNewSite(site)) {
                if (unit) {
                    if (unit->team != team) {
                        zones.Erase(i);

                        zone->Finished(false);

                        return false;
                    }

                    if (zone->unit == unit) {
                        points2.Append(&site);

                    } else {
                        if (zone->GetField30() && unit->speed < 2) {
                            zones.Erase(i);

                            zone->Finished(false);

                            return false;
                        }

                        if (unit->orders == ORDER_AWAIT || unit->orders == ORDER_SENTRY) {
                            ZoneSquare zone_square(site.x, site.y, unit);

                            zone_squares.Append(&zone_square);

                        } else {
                            if ((unit->orders == ORDER_BUILD && unit->state != ORDER_STATE_UNIT_READY) ||
                                zone->GetField30()) {
                                zones.Erase(i);

                                zone->Finished(false);

                                return false;
                            }

                            points2.Append(&site);
                        }
                    }

                } else {
                    if (Access_IsAccessible(zone->unit->unit_type, team, site.x, site.y, 0x02)) {
                        points1.Append(&site);

                    } else {
                        log.Log("Anomalous block at [%i,%i].", site.x + 1, site.y + 1);

                        points2.Append(&site);
                    }
                }
            }
        }

        if (!is_found) {
            zones.Erase(i);

            zone->Finished(true);

            return false;
        }
    }

    return true;
}

bool TaskClearZone::IsNewSite(Point site) {
    for (int32_t i = 0; i < points2.GetCount(); ++i) {
        if (*points2[i] == site) {
            return false;
        }
    }

    for (int32_t i = 0; i < zone_squares.GetCount(); ++i) {
        if (zone_squares[i]->point == site) {
            return false;
        }
    }

    return true;
}

void TaskClearZone::EvaluateSite(ZoneSquare* zone_square, Point site) {
    if (Task_IsReadyToTakeOrders(zone_square->unit)) {
        UnitInfo* unit = Access_GetTeamUnit(site.x, site.y, team, unit_flags);

        if (unit) {
            if (unit->shots > 0 && Task_ShouldReserveShot(unit, site)) {
                points2.Append(&site);

            } else if (unit->orders == ORDER_AWAIT || unit->orders == ORDER_SENTRY) {
                ZoneSquare local(site.x, site.y, unit);

                zone_squares.Append(&local);

            } else {
                points2.Append(&site);
            }

        } else if (Access_IsAccessible(zone_square->unit->unit_type, team, site.x, site.y, 0x02)) {
            bool is_found = false;

            for (int32_t i = 0; i < points1.GetCount(); ++i) {
                if (*points1[i] == site) {
                    is_found = true;
                    points1.Remove(i);

                    break;
                }
            }

            if (is_found) {
                ZoneSquare local(site.x, site.y, zone_square->unit);

                zone_squares.Append(&local);

            } else {
                int32_t unit_hits = zone_square->unit->hits;
                int32_t caution_level = Ai_DetermineCautionLevel(zone_square->unit);

                if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
                    unit_hits = 1;
                }

                if (caution_level > CAUTION_LEVEL_NONE &&
                    AiPlayer_Teams[team].GetDamagePotential(zone_square->unit, site, caution_level, true) >=
                        unit_hits) {
                } else {
                    moving_unit = zone_square->unit;

                    moving_unit->target_grid_x = site.x;
                    moving_unit->target_grid_y = site.y;

                    moving_unit->AddTask(this);

                    Point position(site.x - moving_unit->grid_x, site.y - moving_unit->grid_y);

                    if (position.x >= -1 && position.x <= 1 && position.y >= -1 && position.y <= 1) {
                        state = CLEARZONE_STATE_MOVING_UNIT;

                        if (moving_unit->flags & MOBILE_AIR_UNIT) {
                            UnitsManager_SetNewOrder(&*moving_unit, ORDER_MOVE, ORDER_STATE_INIT);

                        } else {
                            SmartPointer<GroundPath> path = new (std::nothrow) GroundPath(site.x, site.y);

                            path->AddStep(position.x, position.y);

                            moving_unit->path = &*path;

                            moving_unit->Redraw();

                            UnitsManager_SetNewOrder(&*moving_unit, ORDER_MOVE, ORDER_STATE_IN_PROGRESS);

                            if (Remote_IsNetworkGame) {
                                Remote_SendNetPacket_38(&*moving_unit);
                            }
                        }

                    } else {
                        state = CLEARZONE_STATE_WAITING_FOR_PATH;

                        SmartPointer<TaskPathRequest> path_request =
                            new (std::nothrow) TaskPathRequest(&*moving_unit, 2, site);

                        path_request->SetOptimizeFlag(false);

                        SmartPointer<TaskFindPath> find_path = new (std::nothrow)
                            TaskFindPath(this, &*path_request, &PathFindResultCallback, &PathFindCancelCallback);

                        TaskManager.AppendTask(*find_path);
                    }
                }
            }
        }
    }
}

void TaskClearZone::SearchMap() {
    Rect bounds;
    Point position;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    state = CLEARZONE_STATE_SEARCHING_MAP;

    while (zone_squares.GetCount() > 0 && state == CLEARZONE_STATE_SEARCHING_MAP && TickTimer_HaveTimeToThink()) {
        ZoneSquare zone_square(*zone_squares[0]);

        zone_squares.Remove(0);

        points2.Append(&zone_square.point);

        for (int32_t direction = 0; direction < 8 && state == CLEARZONE_STATE_SEARCHING_MAP; ++direction) {
            position = zone_square.point;
            position += Paths_8DirPointsArray[direction];

            if (Access_IsInsideBounds(&bounds, &position) && IsNewSite(position)) {
                EvaluateSite(&zone_square, position);
            }
        }
    }

    if (!zone_squares.GetCount()) {
        points2.Clear();

        if (state == CLEARZONE_STATE_SEARCHING_MAP) {
            state = CLEARZONE_STATE_WAITING;
        }
    }

    if (state == CLEARZONE_STATE_SEARCHING_MAP) {
        RemindTurnEnd();
    }
}

void TaskClearZone::AddZone(Zone* zone) {
    uint8_t** map = AiPlayer_Teams[team].GetInfoMap();

    AiLog log("Clear Zone: Add Zone for %s at [%i,%i].", UnitsManager_BaseUnits[zone->unit->unit_type].singular_name,
              zone->unit->grid_x + 1, zone->unit->grid_y + 1);

    zones.Insert(zone);

    if (map) {
        for (int32_t i = 0; i < zone->points.GetCount(); ++i) {
            map[zone->points[i]->x][zone->points[i]->y] |= 0x08;
        }
    }

    RemindTurnEnd();
}
