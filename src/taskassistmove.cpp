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

#include "taskassistmove.hpp"

#include "access.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "game_manager.hpp"
#include "remote.hpp"
#include "task_manager.hpp"
#include "taskcreateunit.hpp"
#include "taskmove.hpp"
#include "tasktransport.hpp"
#include "units_manager.hpp"

TaskAssistMove::TaskAssistMove(unsigned short team_) : Task(team_, nullptr, 0x2800) {}

TaskAssistMove::~TaskAssistMove() {}

void TaskAssistMove::RequestTransport(UnitInfo* unit1, UnitInfo* unit2) {
    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
        if (unit1->unit_type == AIRTRANS) {
            unit1->SetParent(unit2);
            UnitsManager_SetNewOrder(unit1, ORDER_LOAD, ORDER_STATE_0);

        } else {
            unit2->target_grid_x = unit1->grid_x;
            unit2->target_grid_y = unit1->grid_y;

            if (unit2->speed < 2) {
                unit2->speed = 2;
            }

            SmartPointer<GroundPath> path = new (std::nothrow) GroundPath(unit1->grid_x, unit1->grid_y);

            path->AddStep(unit1->grid_x - unit2->grid_x, unit1->grid_y - unit2->grid_y);

            unit2->path = &*path;
            unit2->Redraw();

            UnitsManager_SetNewOrder(unit2, ORDER_MOVE_TO_UNIT, ORDER_STATE_5);

            if (Remote_IsNetworkGame) {
                Remote_SendNetPacket_38(unit2);
            }
        }
    }
}

void TaskAssistMove::CompleteTransport(UnitInfo* unit1, UnitInfo* unit2, Point site) {
    if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
        if (Access_GetUnit4(site.x, site.y, team, MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
            SmartPointer<Zone> zone = new (std::nothrow) Zone(unit2, this);

            zone->Add(&site);

            AiPlayer_Teams[team].ClearZone(&*zone);

        } else {
            unit1->target_grid_x = site.x;
            unit1->target_grid_y = site.y;

            unit1->SetParent(unit2);

            SDL_assert(GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team);

            if (unit1->unit_type == AIRTRANS) {
                UnitsManager_SetNewOrder(unit1, ORDER_UNLOAD, ORDER_STATE_0);

            } else {
                UnitsManager_SetNewOrder(unit1, ORDER_ACTIVATE, ORDER_STATE_1);
            }
        }
    }
}

bool TaskAssistMove::Task_vfunc1(UnitInfo& unit) { return unit.storage == 0; }

bool TaskAssistMove::IsUnitUsable(UnitInfo& unit) {
    return unit.unit_type == AIRTRANS || unit.unit_type == SEATRANS || unit.unit_type == CLNTRANS;
}

int TaskAssistMove::GetMemoryUse() const { return units.GetMemorySize() - 6; }

char* TaskAssistMove::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Use transports to speed movement");

    return buffer;
}

unsigned char TaskAssistMove::GetType() const { return TaskType_TaskAssistMove; }

void TaskAssistMove::Task_vfunc11(UnitInfo& unit) {
    if (unit.unit_type == AIRTRANS || unit.unit_type == SEATRANS || unit.unit_type == CLNTRANS) {
        units.PushBack(unit);
        unit.PushFrontTask1List(this);
        Task_RemindMoveFinished(&unit);
    }
}

void TaskAssistMove::BeginTurn() {
    SmartPointer<UnitInfo> local_unit;
    int unit_count_transport = 0;
    int unit_count_client = 0;
    bool is_found;

    if (Builder_IsBuildable(AIRTRANS)) {
        is_found = false;

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if ((*it).unit_type == AIRTRANS && (*it).storage == 0) {
                is_found = true;
            }
        }

        if (!is_found) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team && ((*it).flags & MOBILE_LAND_UNIT)) {
                    ++unit_count_client;
                }
            }

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                 it != UnitsManager_MobileAirUnits.End(); ++it) {
                if ((*it).team == team && (*it).unit_type == AIRTRANS) {
                    ++unit_count_transport;
                }
            }

            for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin();
                 it != TaskManager.GetTaskList().End(); ++it) {
                if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskCreateUnit &&
                    dynamic_cast<TaskCreate*>(&*it)->GetUnitType() == AIRTRANS) {
                    ++unit_count_transport;
                }
            }

            if (unit_count_transport < (unit_count_client + 10) / 20) {
                TaskCreateUnit* create_unit_task = new (std::nothrow)
                    TaskCreateUnit(AIRTRANS, this, Point(ResourceManager_MapSize.x / 2, ResourceManager_MapSize.y / 2));

                TaskManager.AddTask(*create_unit_task);
            }
        }
    }

    if (Builder_IsBuildable(CLNTRANS)) {
        unit_count_client = 0;
        unit_count_transport = 0;
        is_found = false;

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if ((*it).unit_type == CLNTRANS && (*it).storage == 0) {
                is_found = true;
            }
        }

        if (!is_found) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team) {
                    if ((*it).unit_type == INFANTRY || (*it).unit_type == COMMANDO) {
                        ++unit_count_client;

                    } else if ((*it).unit_type == CLNTRANS) {
                        ++unit_count_transport;
                    }
                }
            }

            for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin();
                 it != TaskManager.GetTaskList().End(); ++it) {
                if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskCreateUnit &&
                    dynamic_cast<TaskCreate*>(&*it)->GetUnitType() == CLNTRANS) {
                    ++unit_count_transport;
                }
            }

            if (unit_count_transport < (unit_count_client + 8) / 10) {
                TaskCreateUnit* create_unit_task = new (std::nothrow)
                    TaskCreateUnit(CLNTRANS, this, Point(ResourceManager_MapSize.x / 2, ResourceManager_MapSize.y / 2));

                TaskManager.AddTask(*create_unit_task);
            }
        }
    }
}

void TaskAssistMove::EndTurn() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (Task_vfunc17((*it))) {
            return;
        }
    }
}

bool TaskAssistMove::Task_vfunc17(UnitInfo& unit) {
    UnitInfo* UnitInfo_object2;
    Point Point_object1;
    Point Point_object2;
    int distance;
    int minimum_distance;
    bool result;

    if (unit.IsReadyForOrders(this)) {
        if (unit.storage < unit.GetBaseValues()->GetAttribute(ATTRIB_STORAGE) && unit.speed > 0) {
            TransporterMap* map = new (std::nothrow) TransporterMap(
                &unit, 1,
                unit.unit_type == CLNTRANS ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE : CAUTION_LEVEL_AVOID_ALL_DAMAGE);

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == team && ((*it).flags & MOBILE_LAND_UNIT) && (*it).unit_type != SURVEYOR) {
                    if ((*it).GetTask() && (*it).GetTask()->GetType() == TaskType_TaskMove &&
                        Task_IsReadyToTakeOrders(&*it)) {
                        if (dynamic_cast<TaskMove*>((*it).GetTask())->TaskMove_sub_4D247() ||
                            unit.speed == 0) {
                            distance = Access_GetDistance(&*it, &unit);

                            if (UnitInfo_object2 == nullptr || distance < minimum_distance) {
                                if (TaskTransport_Search(&unit, &*it, map)) {
                                    UnitInfo_object2 = &*it;
                                    minimum_distance = distance;
                                    Point_object1.x = (*it).grid_x;
                                    Point_object1.y = (*it).grid_y;
                                }
                            }
                        }
                    }
                }
            }

            if (UnitInfo_object2 &&
                Access_GetDistance(&unit, Point_object1) <= ((unit.unit_type == AIRTRANS) ? 0 : 3)) {
                RequestTransport(&unit, UnitInfo_object2);

                result = true;

                return result;
            }
        }

        if (unit.storage > 0) {
            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).orders == ORDER_IDLE && (*it).GetParent() == &unit && (*it).team == team) {
                    if (!(*it).GetTask() || (*it).GetTask()->GetType() != TaskType_TaskMove) {
                        TaskMove* task_move = new (std::nothrow) TaskMove(&*it, &TaskTransport_MoveFinishedCallback);

                        TaskManager.AddTask(*task_move);
                    }

                    SDL_assert((*it).GetTask()->GetType() == TaskType_TaskMove);

                    Point_object2 = dynamic_cast<TaskMove*>((*it).GetTask())->GetDestination();

                    distance = Access_GetDistance(&unit, Point_object2);

                    if (Point_object2.x >= 0 && (UnitInfo_object2 == nullptr || distance < minimum_distance)) {
                        UnitInfo_object2 = &*it;
                        minimum_distance = distance;
                        Point_object1 = Point_object2;
                    }
                }
            }
        }

        if (UnitInfo_object2) {
            if (Access_GetDistance(&unit, Point_object1) <= ((unit.unit_type == AIRTRANS) ? 0 : 3)) {
                CompleteTransport(&unit, UnitInfo_object2, Point_object1);

            } else {
                SmartPointer<TaskMove> task_move = new (std::nothrow) TaskMove(
                    &unit, this, distance,
                    unit.unit_type == CLNTRANS ? CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE : CAUTION_LEVEL_AVOID_ALL_DAMAGE,
                    Point_object1, &TaskTransport_MoveFinishedCallback);

                TaskManager.AddTask(*task_move);
            }

            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskAssistMove::RemoveSelf() {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        (*it).RemoveTask(this);
    }

    units.Clear();

    parent = nullptr;
    TaskManager.RemoveTask(*this);
}

void TaskAssistMove::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }

void TaskAssistMove::Task_vfunc24(UnitInfo& unit1, UnitInfo& unit2) { Task_RemindMoveFinished(&unit1); }

void TaskAssistMove::Task_vfunc26(UnitInfo& unit1, UnitInfo& unit2) {
    if (unit2.GetTask() && unit2.GetTask()->GetType() == TaskType_TaskMove) {
        dynamic_cast<TaskMove*>(unit2.GetTask())->TaskMove_sub_4C66B(true);
    }

    Task_RemindMoveFinished(&unit1);

    unit2.AddReminders(true);
}
