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

#include "taskplacemines.hpp"

#include "access.hpp"
#include "aiplayer.hpp"
#include "game_manager.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskgetmaterials.hpp"
#include "taskmove.hpp"
#include "taskobtainunits.hpp"
#include "units_manager.hpp"

void TaskPlaceMines::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    if (result == TASKMOVE_RESULT_SUCCESS) {
        Task_RemindMoveFinished(unit);
    }
}

TaskPlaceMines::TaskPlaceMines(unsigned short team_) : Task(team_, nullptr, 0x1A00) {
    mine_layer_count = 0;
    sea_mine_layer_count = 0;
}

TaskPlaceMines::~TaskPlaceMines() {}

bool TaskPlaceMines::IsUnitUsable(UnitInfo& unit) { return unit.unit_type == MINELAYR || unit.unit_type == SEAMNLYR; }

int TaskPlaceMines::GetMemoryUse() const { return units.GetMemorySize() - 6; }

char* TaskPlaceMines::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Place mines");

    return buffer;
}

Rect* TaskPlaceMines::GetBounds(Rect* bounds) {
    unsigned char** info_map = AiPlayer_Teams[team].GetInfoMap();

    if (info_map) {
        for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
            for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                if (info_map[x][y] & 2) {
                    bounds->ulx = x;
                    bounds->uly = y;
                    bounds->lrx = x + 1;
                    bounds->lry = y + 1;

                    return bounds;
                }
            }
        }
    }

    return Task::GetBounds(bounds);
}

unsigned char TaskPlaceMines::GetType() const { return TaskType_TaskPlaceMines; }

bool TaskPlaceMines::IsNeeded() {
    bool has_mine_layer = false;
    bool has_sea_mine_layer = false;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).unit_type == MINELAYR) {
            has_mine_layer = true;

        } else {
            has_sea_mine_layer = true;
        }
    }

    return !has_mine_layer || !has_sea_mine_layer;
}

void TaskPlaceMines::AddUnit(UnitInfo& unit) {
    units.PushBack(unit);
    unit.AddTask(this);

    if (unit.unit_type == MINELAYR) {
        mine_layer_count = 0;

    } else {
        sea_mine_layer_count = 0;
    }

    Task_RemindMoveFinished(&unit);
}

void TaskPlaceMines::BeginTurn() {
    bool has_mine_layer = mine_layer_count > 0;
    bool has_sea_mine_layer = sea_mine_layer_count > 0;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).unit_type == MINELAYR) {
            has_mine_layer = true;

        } else {
            has_sea_mine_layer = true;
        }
    }

    if (!has_mine_layer || !has_sea_mine_layer) {
        SmartPointer<TaskObtainUnits> obtain_unit_task(new (std::nothrow) TaskObtainUnits(this, DeterminePosition()));

        if (!has_mine_layer) {
            obtain_unit_task->AddUnit(MINELAYR);
            ++mine_layer_count;
        }

        if (!has_sea_mine_layer) {
            obtain_unit_task->AddUnit(SEAMNLYR);
            ++sea_mine_layer_count;
        }

        TaskManager.AppendTask(*obtain_unit_task);
    }

    EndTurn();
}

void TaskPlaceMines::EndTurn() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (Execute(*it)) {
            return;
        }
    }
}

bool TaskPlaceMines::Execute(UnitInfo& unit) {
    bool result;

    if (unit.speed > 0 && unit.IsReadyForOrders(this)) {
        if (Task_RetreatFromDanger(this, &unit, CAUTION_LEVEL_AVOID_ALL_DAMAGE)) {
            result = true;

        } else if (unit.storage > 0) {
            unsigned char** info_map = AiPlayer_Teams[team].GetInfoMap();

            if (info_map) {
                if (info_map[unit.grid_x][unit.grid_y] & 2) {
                    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
                        UnitsManager_SetNewOrder(&unit, ORDER_LAY_MINE, ORDER_STATE_PLACING_MINES);

                        info_map[unit.grid_x][unit.grid_y] &= ~0x02;
                    }

                    result = true;

                } else if (unit.GetLayingState() == 2) {
                    UnitsManager_SetNewOrder(&unit, ORDER_LAY_MINE, ORDER_STATE_INACTIVE);

                    result = true;

                } else {
                    Point position;
                    int surface_type;
                    int distance;
                    int minimum_distance{INT32_MAX};
                    bool is_found = false;

                    if (unit.unit_type == MINELAYR) {
                        surface_type = SURFACE_TYPE_LAND;

                    } else {
                        surface_type = SURFACE_TYPE_WATER;
                    }

                    for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
                        for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                            if ((info_map[x][y] & 2) && !(info_map[x][y] & 8) &&
                                Access_GetSurfaceType(x, y) == surface_type) {
                                distance = TaskManager_GetDistance(unit.grid_x - x, unit.grid_y - y);

                                if (!is_found || distance < minimum_distance) {
                                    position.x = x;
                                    position.y = y;

                                    is_found = true;

                                    minimum_distance = distance;
                                }
                            }
                        }
                    }

                    if (is_found) {
                        SmartPointer<TaskMove> move_task(new (std::nothrow) TaskMove(
                            &unit, this, 0, CAUTION_LEVEL_AVOID_ALL_DAMAGE, position, &MoveFinishedCallback));

                        TaskManager.AppendTask(*move_task);

                        result = true;

                    } else {
                        result = false;
                    }
                }

            } else {
                result = false;
            }

        } else {
            SmartPointer<Task> get_materials_task(
                new (std::nothrow) TaskGetMaterials(this, &unit, unit.GetBaseValues()->GetAttribute(ATTRIB_STORAGE)));

            TaskManager.AppendTask(*get_materials_task);

            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskPlaceMines::RemoveSelf() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        TaskManager.RemindAvailable(&*it);
    }

    units.Clear();

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskPlaceMines::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }
