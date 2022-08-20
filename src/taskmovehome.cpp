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

#include "taskmovehome.hpp"

#include "accessmap.hpp"
#include "ai_player.hpp"
#include "paths_manager.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "units_manager.hpp"
#include "zonewalker.hpp"

TaskMoveHome::TaskMoveHome(UnitInfo* unit_, Task* task) : Task(unit->team, task, task->GetFlags()) { unit = unit_; }

TaskMoveHome::~TaskMoveHome() {}

void TaskMoveHome::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    unit->RemoveTask(task, false);
    TaskManager.RemoveTask(*task);
}

void TaskMoveHome::PopulateTeamZones(unsigned char** map) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team) {
            Point site;
            Rect bounds;

            (*it).GetBounds(&bounds);

            bounds.ulx = std::max(0, bounds.ulx - 2);
            bounds.lrx = std::min(static_cast<int>(ResourceManager_MapSize.x), bounds.lrx + 2);
            bounds.uly = std::max(0, bounds.uly - 2);
            bounds.lry = std::min(static_cast<int>(ResourceManager_MapSize.y), bounds.lry + 2);

            for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                    map[site.x][site.y] = 1;
                }
            }
        }
    }
}

void TaskMoveHome::PopulateDefenses(unsigned char** map, ResourceID unit_type) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type && (*it).orders != ORDER_IDLE &&
            (*it).ammo > (*it).GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) &&
            (*it).hits == (*it).GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
            int unit_range = (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE);

            if (unit_range > 0) {
                ZoneWalker walker(Point((*it).grid_x, (*it).grid_y), unit_range);
                int damage_potential = (*it).GetBaseValues()->GetAttribute(ATTRIB_ROUNDS) *
                                       (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK);

                do {
                    unsigned char* map_site = &map[walker.GetGridX()][walker.GetGridY()];

                    if (map_site[0]) {
                        if (map_site[0] + damage_potential < 0x1F) {
                            map_site[0] += damage_potential;

                        } else {
                            map_site[0] = 0x1F;
                        }
                    }

                } while (walker.FindNext());
            }
        }
    }
}

void TaskMoveHome::PopulateOccupiedSites(unsigned char** map, SmartList<UnitInfo>* units) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).orders != ORDER_IDLE && (&*it) != &*unit) {
            Point site;
            Rect bounds;

            (*it).GetBounds(&bounds);

            for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                    map[site.x][site.y] = 0;
                }
            }
        }
    }
}

int TaskMoveHome::GetMemoryUse() const { return 4; }

char* TaskMoveHome::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Move back to complex");

    return buffer;
}

unsigned char TaskMoveHome::GetType() const { return TaskType_TaskMoveHome; }

void TaskMoveHome::AddReminder() {
    unit->PushFrontTask1List(this);
    Task_RemindMoveFinished(&*unit);
}

void TaskMoveHome::EndTurn() {
    if (unit && unit->IsReadyForOrders(this)) {
        unit->RemoveTask(this);
        unit = nullptr;
        TaskManager.RemoveTask(*this);
    }
}

bool TaskMoveHome::Task_vfunc17(UnitInfo& unit_) {
    bool result;

    if (unit == unit_ && unit->IsReadyForOrders(this) && unit->speed > 0) {
        AccessMap map1;
        AccessMap map2;

        PopulateTeamZones(map1.GetMap());

        if (unit->flags & MOBILE_AIR_UNIT) {
            PopulateDefenses(map1.GetMap(), ANTIAIR);

            PopulateOccupiedSites(map1.GetMap(), &UnitsManager_MobileAirUnits);

        } else {
            PopulateDefenses(map1.GetMap(), GUNTURRT);
            PopulateDefenses(map1.GetMap(), ARTYTRRT);
            PopulateDefenses(map1.GetMap(), ANTIMSSL);

            PopulateOccupiedSites(map1.GetMap(), &UnitsManager_MobileLandSeaUnits);
            PopulateOccupiedSites(map1.GetMap(), &UnitsManager_StationaryUnits);
        }

        {
            Point destination(unit->grid_x, unit->grid_y);
            Point site;
            int surface_type = UnitsManager_BaseUnits[unit->unit_type].land_type;
            unsigned char** info_map = AiPlayer_Teams[team].GetInfoMap();
            int safety;
            int maximum_safety = map1.GetMapColumn(unit->grid_x)[unit->grid_y];
            int distance;
            int minimum_distance = 0;

            PathsManager_InitAccessMap(&*unit, map2.GetMap(), 2, CAUTION_LEVEL_AVOID_ALL_DAMAGE);

            for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
                for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
                    safety = map1.GetMapColumn(site.x)[site.y];

                    if (safety >= maximum_safety && map2.GetMapColumn(site.x)[site.y] > 0) {
                        if (!(info_map[site.x][site.y] & 8)) {
                            distance = TaskManager_GetDistance(unit->grid_x - site.x, unit->grid_y - site.y);

                            if (safety > maximum_safety || distance < minimum_distance) {
                                minimum_distance = distance;
                                destination = site;
                                maximum_safety = safety;
                            }
                        }
                    }
                }
            }

            if (unit->grid_x != destination.x || unit->grid_y != destination.y) {
                SmartPointer<Task> move_task = new (std::nothrow)
                    TaskMove(&*unit, this, 0, CAUTION_LEVEL_AVOID_ALL_DAMAGE, destination, &MoveFinishedCallback);

                TaskManager.AddTask(*move_task);

                result = true;

            } else {
                result = false;
            }
        }

    } else {
        result = false;
    }

    return result;
}

void TaskMoveHome::RemoveSelf() {
    if (unit) {
        TaskManager.RemindAvailable(&*unit);

        unit = nullptr;
        parent = nullptr;
        TaskManager.RemoveTask(*this);
    }
}

void TaskMoveHome::Remove(UnitInfo& unit_) {
    if (unit == unit_) {
        unit = nullptr;
        TaskManager.RemoveTask(*this);
    }
}
