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
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "remote.hpp"
#include "task_manager.hpp"
#include "taskfindpath.hpp"
#include "taskpathrequest.hpp"
#include "units_manager.hpp"

void TaskClearZone::PathFindResultCallback(Task* task, PathRequest* request, Point point, GroundPath* path,
                                           char result) {
    TaskClearZone* clear_zone = dynamic_cast<TaskClearZone*>(task);

    if (path && (!clear_zone->moving_unit || clear_zone->moving_unit->IsReadyForOrders(clear_zone)) &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == clear_zone->team)) {
        SDL_assert(clear_zone->moving_unit != nullptr);

        clear_zone->state = CLEARZONE_STATE_MOVING_UNIT;

        if (clear_zone->moving_unit->flags & MOBILE_AIR_UNIT) {
            UnitsManager_SetNewOrder(&*clear_zone->moving_unit, ORDER_MOVE, ORDER_STATE_0);

        } else {
            clear_zone->moving_unit->path = path;
            clear_zone->moving_unit->Redraw();

            UnitsManager_SetNewOrder(&*clear_zone->moving_unit, ORDER_MOVE, ORDER_STATE_5);

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

TaskClearZone::TaskClearZone(unsigned short team, unsigned int flags_)
    : Task(team, nullptr, 0), unit_flags(flags_), state(CLEARZONE_STATE_WAITING) {}

TaskClearZone::~TaskClearZone() {}

int TaskClearZone::GetMemoryUse() const {
    return 4 + zones.GetCount() * (sizeof(Zone) + 4) + zone_squares.GetCount() * sizeof(ZoneSquare) +
           (points1.GetCount() + points2.GetCount()) * sizeof(Point);
}

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

unsigned char TaskClearZone::GetType() const { return TaskType_TaskClearZone; }

bool TaskClearZone::IsThinking() { return state != CLEARZONE_STATE_WAITING && state != CLEARZONE_STATE_MOVING_UNIT; }

void TaskClearZone::Begin() { RemindTurnEnd(); }

void TaskClearZone::BeginTurn() { EndTurn(); }

void TaskClearZone::EndTurn() {
    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
        if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
            if (state != CLEARZONE_STATE_EXAMINING_ZONES && state != CLEARZONE_STATE_WAITING_FOR_PATH) {
                if (state == CLEARZONE_STATE_MOVING_UNIT) {
                    state = CLEARZONE_STATE_WAITING;

                    if (moving_unit) {
                        moving_unit->RemoveTask(this);

                        if (!moving_unit->GetTask()) {
                            TaskManager.RemindAvailable(&*moving_unit);
                        }

                        moving_unit = nullptr;
                    }
                }

                if (state == CLEARZONE_STATE_WAITING) {
                    while (!ExamineZones()) {
                        if (timer_get_stamp32() - Paths_LastTimeStamp > Paths_TimeLimit) {
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

bool TaskClearZone::Task_vfunc17(UnitInfo& unit) {
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
        moving_unit = nullptr;
        state = CLEARZONE_STATE_WAITING;
    }
}

bool TaskClearZone::ExamineZones() {
    Point site;
    SmartPointer<Zone> zone;

    state = CLEARZONE_STATE_EXAMINING_ZONES;

    points1.Clear();
    zone_squares.Clear();
    points2.Clear();

    unsigned char** info_map = AiPlayer_Teams[team].GetInfoMap();

    if (info_map) {
        for (int i = 0; i < zones.GetCount(); ++i) {
            zone = zones[i];

            for (int j = 0; j < zone->points.GetCount(); ++j) {
                site = *zone->points[j];

                info_map[site.x][site.y] &= 0xF7;
            }
        }
    }

    for (int i = 0; i < zones.GetCount(); ++i) {
        bool is_found = false;

        zone = zones[i];

        for (int j = 0; j < zone->points.GetCount(); ++j) {
            site = *zone->points[j];

            if (info_map) {
                info_map[site.x][site.y] |= 0x08;
            }

            UnitInfo* unit = Access_GetUnit4(site.x, site.y, team, unit_flags);

            if (unit && zone->unit != unit) {
                is_found = true;
            }

            if (IsNewSite(site)) {
                if (unit) {
                    if (unit->team != team) {
                        zones.Erase(i);

                        zone->CallTaskVfunc27(false);

                        return false;
                    }

                    if (zone->unit == unit) {
                        points2.Append(&site);

                    } else {
                        if (zone->GetField30() && unit->speed < 2) {
                            zones.Erase(i);

                            zone->CallTaskVfunc27(false);

                            return false;
                        }

                        if (unit->orders == ORDER_AWAIT || unit->orders == ORDER_SENTRY) {
                            ZoneSquare zone_square(site.x, site.y, unit);

                            zone_squares.Append(&zone_square);

                        } else {
                            if ((unit->orders == ORDER_BUILD && unit->state != ORDER_STATE_UNIT_READY) ||
                                zone->GetField30()) {
                                zones.Erase(i);

                                zone->CallTaskVfunc27(false);

                                return false;
                            }

                            points2.Append(&site);
                        }
                    }

                } else {
                    if (Access_IsAccessible(zone->unit->unit_type, team, site.x, site.y, 0x02)) {
                        points1.Append(&site);

                    } else {
                        points2.Append(&site);
                    }
                }
            }
        }

        if (!is_found) {
            zones.Erase(i);

            zone->CallTaskVfunc27(true);

            return false;
        }
    }

    return true;
}

bool TaskClearZone::IsNewSite(Point site) {
    for (int i = 0; i < points2.GetCount(); ++i) {
        if (*points2[i] == site) {
            return false;
        }
    }

    for (int i = 0; i < zone_squares.GetCount(); ++i) {
        if (zone_squares[i]->point == site) {
            return false;
        }
    }

    return true;
}

void TaskClearZone::EvaluateSite(ZoneSquare* zone_square, Point site) {
    if (Task_IsReadyToTakeOrders(zone_square->unit)) {
        UnitInfo* unit = Access_GetUnit4(site.x, site.y, team, unit_flags);

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

            for (int i = 0; i < points1.GetCount(); ++i) {
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
                int unit_hits = zone_square->unit->hits;
                int caution_level = Ai_DetermineCautionLevel(zone_square->unit);

                if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
                    unit_hits = 1;
                }

                if (caution_level > CAUTION_LEVEL_NONE &&
                    AiPlayer_Teams[team].GetDamagePotential(zone_square->unit, site, caution_level, 0x01) >=
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
                            UnitsManager_SetNewOrder(&*moving_unit, ORDER_MOVE, ORDER_STATE_0);

                        } else {
                            SmartPointer<GroundPath> path = new (std::nothrow) GroundPath(site.x, site.y);

                            path->AddStep(position.x, position.y);

                            moving_unit->path = &*path;

                            moving_unit->Redraw();

                            UnitsManager_SetNewOrder(&*moving_unit, ORDER_MOVE, ORDER_STATE_5);

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

    while (zone_squares.GetCount() > 0 && state == CLEARZONE_STATE_SEARCHING_MAP &&
           timer_get_stamp32() - Paths_LastTimeStamp < Paths_TimeLimit) {
        ZoneSquare zone_square(*zone_squares[0]);

        zone_squares.Remove(0);

        points2.Append(&zone_square.point);

        for (int direction = 0; direction < 8 && state == CLEARZONE_STATE_SEARCHING_MAP; ++direction) {
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
    unsigned char** map;

    map = AiPlayer_Teams[team].GetInfoMap();

    zones.Insert(zone);

    if (map) {
        for (int i = 0; i < zone->points.GetCount(); ++i) {
            map[zone->points[i]->x][zone->points[i]->y] |= 0x08;
        }
    }

    RemindTurnEnd();
}
