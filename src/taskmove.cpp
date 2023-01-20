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

#include "taskmove.hpp"

#include "access.hpp"
#include "adjustrequest.hpp"
#include "ai.hpp"
#include "aiattack.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "hash.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskfindpath.hpp"
#include "taskmanagebuildings.hpp"
#include "tasktransport.hpp"
#include "transportorder.hpp"
#include "units_manager.hpp"

TaskMove::TaskMove(UnitInfo* unit, Task* task, unsigned short minimum_distance_, unsigned char caution_level_,
                   Point destination_, void (*result_callback_)(Task* task, UnitInfo* unit, char result))
    : Task(unit->team, task, task ? task->GetFlags() : 0x1000) {
    field_72 = 0;
    field_69 = false;
    passenger = unit;
    minimum_distance = minimum_distance_;

    SDL_assert(unit->team == team);

    caution_level = caution_level_;

    if (passenger->GetField221() & 0x01) {
        caution_level = CAUTION_LEVEL_NONE;
    }

    if (passenger->ammo == 0 && passenger->GetBaseValues()->GetAttribute(ATTRIB_AMMO) > 0 &&
        caution_level > CAUTION_LEVEL_NONE) {
        caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;
    }

    point3 = destination_;
    point1 = destination_;
    point2 = destination_;
    destination = destination_;

    transporter_unit_type = INVALID_ID;

    result_callback = result_callback_;

    field_68 = false;
    field_70 = 0;
}

TaskMove::TaskMove(UnitInfo* unit, void (*result_callback_)(Task* task, UnitInfo* unit, char result))
    : Task(unit->team, nullptr, 0x2900) {
    passenger = unit;
    field_69 = false;
    field_71 = 0;
    minimum_distance = 0;
    caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;

    point3.x = -1;
    point3.y = -1;
    point2 = point3;
    point1 = point3;
    destination.x = -1;
    destination.y = -1;

    transporter_unit_type = unit->GetParent()->unit_type;

    result_callback = result_callback_;

    field_68 = false;
    field_70 = 0;
}

TaskMove::~TaskMove() {}

int TaskMove::GetCautionLevel(UnitInfo& unit) { return caution_level; }

int TaskMove::GetMemoryUse() const { return planned_path.GetCount() * 4 + 4; }

char* TaskMove::WriteStatusLog(char* buffer) const {
    if (passenger) {
        if (planned_path.GetCount()) {
            Point waypoint = *planned_path[planned_path.GetCount() - 1];

            if (transporter_unit_type == INVALID_ID) {
                sprintf(buffer, "Move to [%i,%i]", waypoint.x + 1, waypoint.y + 1);

            } else {
                sprintf(buffer, "Move to [%i,%i] via [%i,%i] using %s", destination.x + 1, destination.y + 1,
                        waypoint.x + 1, waypoint.y + 1, UnitsManager_BaseUnits[transporter_unit_type].singular_name);
            }

        } else if (transporter_unit_type == INVALID_ID) {
            strcpy(buffer, "Move: calculating.");

        } else {
            sprintf(buffer, "Move to [%i,%i] using %s", destination.x + 1, destination.y + 1,
                    UnitsManager_BaseUnits[transporter_unit_type].singular_name);
        }

    } else {
        strcpy(buffer, "Move: finished.");
    }

    return buffer;
}

unsigned char TaskMove::GetType() const { return TaskType_TaskMove; }

void TaskMove::SetField68(bool value) { field_68 = value; }

void TaskMove::SetField69(bool value) { field_69 = value; }

UnitInfo* TaskMove::GetPassenger() { return &*passenger; }

void TaskMove::SetDestination(Point site) { destination = site; }

ResourceID TaskMove::GetTransporterType() const { return transporter_unit_type; }

Point TaskMove::GetPoint2() const { return point2; }

void TaskMove::TaskMove_sub_4C66B(bool mode) {
    if (transporter_unit_type == CLNTRANS && !mode) {
        AttemptTransportType(AIRTRANS);

    } else {
        transporter_unit_type = INVALID_ID;
    }
}

void TaskMove::AddUnit(UnitInfo& unit) {
    if (passenger == &unit && unit.GetTask() == this) {
        Task_RemindMoveFinished(&*passenger, true);
    }
}

void TaskMove::Begin() {
    Task_RemoveMovementTasks(&*passenger);
    passenger->PushFrontTask1List(this);

    if (passenger->orders == ORDER_IDLE) {
        RemindTurnStart(true);
    }

    if (transporter_unit_type == INVALID_ID && passenger->orders != ORDER_IDLE) {
        Search(true);
    }
}

void TaskMove::BeginTurn() {
    if (passenger) {
        if (passenger->orders == ORDER_IDLE && passenger->state == ORDER_STATE_4 && destination.x < 0) {
            SmartPointer<UnitInfo> transport(passenger->GetParent());

            SDL_assert(transport != nullptr);

            destination.x = transport->grid_x;
            destination.y = transport->grid_y;

            TaskMove_sub_4C6BA();

            Task_RemindMoveFinished(&*transport, true);
        }

        if (!zone && passenger->IsReadyForOrders(this)) {
            if (transporter_unit_type == INVALID_ID || planned_path.GetCount()) {
                if (passenger->speed > 0) {
                    Task_RemindMoveFinished(&*passenger, true);
                }

            } else {
                Search(true);
            }
        }
    }
}

void TaskMove::EndTurn() {
    if (passenger && passenger->IsReadyForOrders(this)) {
        if (field_68 && passenger->speed == 0 && transporter_unit_type == INVALID_ID) {
            Finished(TASKMOVE_RESULT_SUCCESS);

        } else if (passenger->speed && !zone) {
            Task_vfunc17(*passenger);
        }
    }
}

bool TaskMove::Task_vfunc17(UnitInfo& unit) {
    bool result;

    if (passenger == &unit) {
        if (unit.IsReadyForOrders(this)) {
            if (UnitsManager_TeamInfo[passenger->team].team_type == TEAM_TYPE_COMPUTER) {
                if (AiAttack_EvaluateAttack(&*passenger, caution_level > CAUTION_LEVEL_NONE)) {
                    result = true;

                } else if (!passenger) {
                    result = true;

                } else {
                    FindCurrentLocation();

                    if (planned_path.GetCount()) {
                        if (passenger->speed) {
                            if (caution_level > CAUTION_LEVEL_NONE) {
                                if (Task_ShouldReserveShot(&*passenger, Point(passenger->grid_x, passenger->grid_y))) {
                                    int unit_shots = passenger->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);
                                    int unit_speed = passenger->GetBaseValues()->GetAttribute(ATTRIB_SPEED);

                                    if (passenger->speed <= ((unit_speed + unit_shots - 1) / unit_shots)) {
                                        result = false;

                                        return result;
                                    }
                                }
                            }

                            if (FindWaypoint()) {
                                if (passenger->flags & MOBILE_AIR_UNIT) {
                                    MoveAirUnit();

                                    result = true;

                                } else if (IsPathClear()) {
                                    result = true;

                                } else {
                                    PathRequest* request = new (std::nothrow) TaskPathRequest(&*passenger, 2, point1);

                                    request->SetMaxCost(passenger->speed * 4);
                                    request->SetCautionLevel(caution_level);
                                    request->SetOptimizeFlag(caution_level > CAUTION_LEVEL_NONE);

                                    SmartPointer<Task> find_path(new (std::nothrow) TaskFindPath(
                                        this, request, &DirectPathResultCallback, &PathCancelCallback));

                                    TaskManager.AppendTask(*find_path);

                                    result = true;
                                }

                            } else {
                                RetryMove();

                                result = true;
                            }

                        } else {
                            if (field_68 && transporter_unit_type == INVALID_ID) {
                                Finished(TASKMOVE_RESULT_SUCCESS);
                            }

                            result = false;
                        }

                    } else {
                        if (transporter_unit_type == INVALID_ID) {
                            RetryMove();
                        }

                        result = true;
                    }
                }

            } else {
                passenger->ClearFromTaskLists();
                RemoveSelf();

                result = false;
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskMove::RemoveSelf() {
    if (passenger) {
        passenger->RemoveTask(this);
    }

    passenger = nullptr;
    zone = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskMove::RemoveUnit(UnitInfo& unit) {
    if (passenger == &unit) {
        Finished(TASKMOVE_RESULT_CANCELLED);
    }
}

void TaskMove::Task_vfunc27(Zone* zone_, char mode) {
    if (zone == zone_) {
        zone = nullptr;
    }

    if (passenger) {
        if (!mode && field_69) {
            Finished(TASKMOVE_RESULT_SUCCESS);

        } else {
            Task_RemindMoveFinished(&*passenger, true);
        }
    }
}

void TaskMove::AttemptTransport() {
    if (UnitsManager_BaseUnits[passenger->unit_type].land_type & SURFACE_TYPE_WATER) {
        Finished(TASKMOVE_RESULT_BLOCKED);

    } else {
        if ((passenger->unit_type == COMMANDO || passenger->unit_type == INFANTRY) &&
            (FindUnit(&UnitsManager_MobileLandSeaUnits, team, CLNTRANS) ||
             FindUnit(&UnitsManager_StationaryUnits, team, LIGHTPLT))) {
            int team_id;

            for (team_id = PLAYER_TEAM_RED; team_id < PLAYER_TEAM_MAX - 1; ++team_id) {
                if (team_id != team) {
                    if (UnitsManager_TeamInfo[team_id]
                            .heat_map_complete[ResourceManager_MapSize.x * passenger->grid_y + passenger->grid_x] > 0) {
                        break;
                    }
                }
            }

            if (team_id == PLAYER_TEAM_MAX - 1) {
                AttemptTransportType(CLNTRANS);

                return;
            }
        }

        if (Builder_IsBuildable(AIRTRANS) && FindUnit(&UnitsManager_MobileAirUnits, team, AIRTRANS)) {
            AttemptTransportType(AIRTRANS);

        } else {
            AttemptTransportType(SEATRANS);
        }
    }
}

void TaskMove::AttemptTransportType(ResourceID unit_type) {
    PathRequest* request = new (std::nothrow) TaskPathRequest(&*passenger, 0, point3);

    request->SetMinimumDistance(minimum_distance);
    request->SetCautionLevel(caution_level);
    request->CreateTransport(unit_type);

    SmartPointer<Task> find_path(new (std::nothrow)
                                     TaskFindPath(this, request, &PathResultCallback, &PathCancelCallback));

    TaskManager.AppendTask(*find_path);
}

void TaskMove::TaskMove_sub_4C6BA() {
    bool keep_searching = true;
    Point site = destination;
    Rect bounds;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (Access_IsAccessible(passenger->unit_type, team, site.x, site.y, 2) &&
        !Ai_IsDangerousLocation(&*passenger, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, 1)) {
        point3 = destination;
    }

    for (int range_limit = 2; keep_searching; range_limit += 2) {
        --site.x;
        ++site.y;
        keep_searching = false;

        for (int direction = 0; direction < 8; direction += 2) {
            for (int range = 0; range < range_limit; ++range) {
                site += Paths_8DirPointsArray[direction];

                if (Access_IsInsideBounds(&bounds, &site)) {
                    keep_searching = true;

                    if (Access_IsAccessible(passenger->unit_type, team, site.x, site.y, 1) &&
                        !Ai_IsDangerousLocation(&*passenger, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, 1)) {
                        destination = site;
                        point3 = destination;

                        return;
                    }
                }
            }
        }
    }
}

void TaskMove::Search(bool mode) {
    Point line_distance(point3.x - passenger->grid_x, point3.y - passenger->grid_y);

    if (line_distance.x * line_distance.x + line_distance.y * line_distance.y > minimum_distance) {
        TaskPathRequest* request = new (std::nothrow) TaskPathRequest(&*passenger, 1, point3);
        int distance;

        field_71 = 0;

        distance = TaskManager_GetDistance(line_distance.x, line_distance.y) / 2;

        request->SetMinimumDistance(minimum_distance);

        if (distance >= passenger->GetBaseValues()->GetAttribute(ATTRIB_SPEED) && transporter_unit_type == INVALID_ID) {
            request->SetCautionLevel(CAUTION_LEVEL_AVOID_ALL_DAMAGE);

        } else {
            request->SetCautionLevel(caution_level);
        }

        if (mode) {
            request->SetField31(transporter_unit_type == INVALID_ID);
        }

        SmartPointer<Task> find_path(new (std::nothrow)
                                         TaskFindPath(this, request, &FullPathResultCallback, &PathCancelCallback));

        TaskManager.AppendTask(*find_path);

    } else {
        Finished(TASKMOVE_RESULT_ALREADY_IN_RANGE);
    }
}

void TaskMove::Finished(int result) {
    if (passenger) {
        passenger->RemoveTask(this, false);

        result_callback(&*parent, &*passenger, result);
    }

    passenger = nullptr;
    zone = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskMove::PathResultCallback(Task* task, PathRequest* path_request, Point destination_, GroundPath* path,
                                  char result) {
    SmartPointer<TaskMove> move(dynamic_cast<TaskMove*>(task));

    if (path) {
        move->transporter_unit_type = path_request->GetUnit2()->unit_type;
        move->field_71 = result;

        move->TranscribeTransportPath(destination_, path);

    } else {
        if (path_request->GetUnit2()->unit_type != AIRTRANS && Builder_IsBuildable(AIRTRANS) &&
            (FindUnit(&UnitsManager_MobileAirUnits, move->GetTeam(), AIRTRANS) ||
             FindUnit(&UnitsManager_StationaryUnits, move->GetTeam(), AIRPLT))) {
            move->AttemptTransportType(AIRTRANS);

        } else {
            move->Finished(TASKMOVE_RESULT_BLOCKED);
        }
    }
}

void TaskMove::FullPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                      char result) {
    SmartPointer<TaskMove> move(dynamic_cast<TaskMove*>(task));

    if (path) {
        move->field_71 = result;

        if (path_request->GetUnit2()) {
            move->transporter_unit_type = path_request->GetUnit2()->unit_type;

        } else {
            move->transporter_unit_type = INVALID_ID;
        }

        move->point2 = move->point3;
        move->destination = move->point3;

        if (result == 0 && path_request->GetCautionLevel() == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
            if ((TaskManager_GetDistance(move->passenger->grid_x - move->point3.x,
                                         move->passenger->grid_y - move->point3.y) /
                 2) > move->passenger->GetBaseValues()->GetAttribute(ATTRIB_SPEED)) {
                AiPlayer_Teams[move->team].AddTransportOrder(
                    new (std::nothrow) TransportOrder(&*move->passenger, move->transporter_unit_type, path));
            }
        }

        if (move->transporter_unit_type == INVALID_ID) {
            move->ProcessPath(destination, path);

        } else {
            move->TranscribeTransportPath(destination, path);
        }

    } else if (path_request->GetCautionLevel() != move->caution_level) {
        SmartPointer<Task> find_path;
        PathRequest* request =
            new (std::nothrow) TaskPathRequest(path_request->GetUnit1(), 1, path_request->GetPoint());

        request->SetMinimumDistance(move->minimum_distance);
        request->SetCautionLevel(move->caution_level);

        find_path = new (std::nothrow) TaskFindPath(&*move, request, &FullPathResultCallback, &PathCancelCallback);

        TaskManager.AppendTask(*find_path);

    } else {
        if (move->transporter_unit_type == INVALID_ID) {
            move->AttemptTransport();
        }
    }
}
void TaskMove::DirectPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                        char result) {
    SmartPointer<TaskMove> move = dynamic_cast<TaskMove*>(task);

    if (move == dynamic_cast<TaskMove*>(move->passenger->GetTask())) {
        if (path) {
            if (move->caution_level > CAUTION_LEVEL_NONE && (move->passenger->flags & MOBILE_AIR_UNIT)) {
                AdjustRequest* request = new (std::nothrow) AdjustRequest(&*move->passenger, 2, move->point1, path);

                SmartPointer<Task> find_path(
                    new (std::nothrow) TaskFindPath(&*move, request, &ActualPathResultCallback, &PathCancelCallback));

                TaskManager.AppendTask(*find_path);

            } else {
                move->MoveUnit(path);
            }

        } else {
            PathRequest* request = new (std::nothrow) TaskPathRequest(&*move->passenger, 1, move->point1);

            request->SetMaxCost(move->passenger->speed * 4);
            request->SetCautionLevel(move->caution_level);

            SmartPointer<Task> find_path(
                new (std::nothrow) TaskFindPath(&*move, request, &BlockedPathResultCallback, &PathCancelCallback));

            TaskManager.AppendTask(*find_path);
        }
    }
}

void TaskMove::BlockedPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                         char result) {
    SmartPointer<TaskMove> move = dynamic_cast<TaskMove*>(task);

    if (move == dynamic_cast<TaskMove*>(move->passenger->GetTask())) {
        if (path) {
            SmartPointer<UnitInfo> unit;
            SmartObjectArray<PathStep> steps = path->GetSteps();
            bool flag = false;
            Point site;
            SmartPointer<Zone> zone = new (std::nothrow) Zone(&*move->passenger, &*move);
            unsigned int unit_flags;

            if (move->passenger->flags & MOBILE_AIR_UNIT) {
                unit_flags = MOBILE_AIR_UNIT;

            } else {
                unit_flags = MOBILE_SEA_UNIT | MOBILE_LAND_UNIT;
            }

            site.x = move->passenger->grid_x;
            site.y = move->passenger->grid_y;

            zone->Add(&site);

            for (int i = 0; i < steps.GetCount(); ++i) {
                site.x += steps[i]->x;
                site.y += steps[i]->y;

                zone->Add(&site);

                unit = Access_GetUnit4(site.x, site.y, move->team, unit_flags);

                if (unit) {
                    flag = true;

                    if (unit == move->passenger) {
                        move->Finished(TASKMOVE_RESULT_BLOCKED);

                        return;
                    }
                }
            }

            if (flag) {
                move->zone = zone;
                move->zone->SetField30(move->field_69);

                AiPlayer_Teams[move->team].ClearZone(&*zone);

            } else {
                move->point1 = site;
                move->MoveUnit(path);
            }

        } else {
            move->RetryMove();
        }
    }
}

void TaskMove::ActualPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                        char result) {
    SmartPointer<TaskMove> move = dynamic_cast<TaskMove*>(task);
    AdjustRequest* request = dynamic_cast<AdjustRequest*>(path_request);

    if (move == dynamic_cast<TaskMove*>(move->passenger->GetTask())) {
        if (path) {
            SmartObjectArray<PathStep> adjusted_steps = request->GetPath()->GetSteps();
            SmartObjectArray<PathStep> steps = path->GetSteps();
            Point position(move->passenger->grid_x, move->passenger->grid_y);
            int count = adjusted_steps->GetCount();
            int step_index;

            SDL_assert(count > 0);

            if (steps->GetCount() < count) {
                count = steps->GetCount();
            }

            SDL_assert(count > 0);

            for (step_index = 0; step_index < count; ++step_index) {
                position.x += adjusted_steps[step_index]->x;
                position.y += adjusted_steps[step_index]->y;

                if (steps[step_index]->x != adjusted_steps[step_index]->x ||
                    steps[step_index]->y != adjusted_steps[step_index]->y) {
                    break;
                }
            }

            move->point1 = position;

            if (step_index != count) {
                SmartPointer<GroundPath> ground_path = new (std::nothrow) GroundPath(position.x, position.y);

                for (int i = 0; i <= step_index; ++i) {
                    ground_path->AddStep(adjusted_steps[i]->x, adjusted_steps[i]->y);
                }

                move->MoveUnit(&*ground_path);

            } else {
                move->MoveUnit(path);
            }

        } else {
            move->Finished(TASKMOVE_RESULT_BLOCKED);
        }
    }
}

void TaskMove::PathCancelCallback(Task* task, PathRequest* path_request) {
    dynamic_cast<TaskMove*>(task)->Finished(TASKMOVE_RESULT_CANCELLED);
}

void TaskMove::ProcessPath(Point site, GroundPath* path) {
    SmartObjectArray<PathStep> steps = path->GetSteps();
    Point line_distance;
    int distance_minimum = minimum_distance;

    if (transporter_unit_type != INVALID_ID) {
        distance_minimum = 0;
    }

    field_70 = 0;

    for (int i = 0; i < steps.GetCount(); ++i) {
        site.x += steps[i]->x;
        site.y += steps[i]->y;

        planned_path.Append(&site);

        line_distance.x = point2.x - site.x;
        line_distance.y = point2.y - site.y;

        if (line_distance.x * line_distance.x + line_distance.y * line_distance.y <= distance_minimum) {
            break;
        }
    }

    if (planned_path.GetCount() > 0) {
        Task_RemindMoveFinished(&*passenger, true);

    } else if (transporter_unit_type == INVALID_ID) {
        Finished(TASKMOVE_RESULT_BLOCKED);

    } else {
        Task_RemindMoveFinished(&*passenger);
    }
}

void TaskMove::RetryMove() {
    if (field_70) {
        Finished(TASKMOVE_RESULT_SUCCESS);

    } else if (field_71) {
        planned_path.Clear();
        point2 = point3;
        transporter_unit_type = INVALID_ID;

        Search(false);

    } else {
        Finished(TASKMOVE_RESULT_BLOCKED);
    }
}

void TaskMove::TranscribeTransportPath(Point site, GroundPath* path) {
    Point position = site;
    SmartObjectArray<PathStep> steps = path->GetSteps();
    int block_count = 0;
    bool flag1 = false;
    bool needs_transport = false;

    for (int i = 0; i < steps.GetCount(); ++i) {
        position.x += steps[i]->x;
        position.y += steps[i]->y;

        if (Access_IsAccessible(passenger->unit_type, team, position.x, position.y, 0x00)) {
            if (flag1) {
                destination = position;
                flag1 = false;

                break;
            }

        } else {
            if (!flag1) {
                point2.x = position.x - steps[i]->x;
                point2.y = position.y - steps[i]->y;

                flag1 = true;
                needs_transport = true;
            }

            ++block_count;
        }
    }

    if (needs_transport) {
        if (flag1) {
            if (field_71) {
                Search(false);

            } else {
                Finished(TASKMOVE_RESULT_BLOCKED);
            }

        } else {
            if (!field_71) {
                AiPlayer_Teams[team].AddTransportOrder(new (std::nothrow)
                                                           TransportOrder(&*passenger, transporter_unit_type, path));
            }

            if (transporter_unit_type == SEATRANS && !FindUnit(&UnitsManager_MobileLandSeaUnits, team, SEATRANS)) {
                if (!FindUnit(&UnitsManager_StationaryUnits, team, SHIPYARD)) {
                    block_count -= 20;
                }

                if (block_count < 10) {
                    TaskManageBuildings* manager =
                        dynamic_cast<TaskManageBuildings*>(AiPlayer_Teams[team].FindManager(site));

                    position = site;

                    for (int i = 0; i < steps.GetCount(); ++i) {
                        position.x += steps[i]->x;
                        position.y += steps[i]->y;

                        if (!Access_IsAccessible(passenger->unit_type, team, position.x, position.y, 0x00) &&
                            Access_IsAccessible(BRIDGE, team, position.x, position.y, 0x00)) {
                            manager->BuildBridge(position, this);
                        }
                    }
                }
            }

            FindTransport(transporter_unit_type);

            if (site == point2) {
                Task_vfunc17(*passenger);

            } else {
                ProcessPath(site, path);
            }
        }

    } else {
        ProcessPath(site, path);
    }
}

bool TaskMove::CheckAirPath() {
    bool result;

    if (caution_level == CAUTION_LEVEL_NONE) {
        result = true;

    } else {
        short** damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*passenger, caution_level, 0x01);

        if (damage_potential_map) {
            int line_distance_x = point1.x - passenger->grid_x;
            int line_distance_y = point1.y - passenger->grid_y;
            Point site;
            int distance = line_distance_x * line_distance_x + line_distance_y * line_distance_y;
            int range_limit = sqrt(distance) * 4.0 + 0.5;
            int unit_hits = passenger->hits;

            if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
                unit_hits = 1;
            }

            if (range_limit) {
                line_distance_x = (line_distance_x << 22) / range_limit;
                line_distance_y = (line_distance_y << 22) / range_limit;

                for (int i = 0; i < range_limit; ++i) {
                    site.x = (((i * line_distance_x) >> 16) + passenger->x) >> 6;
                    site.y = (((i * line_distance_y) >> 16) + passenger->y) >> 6;

                    if (damage_potential_map[site.x][site.y] >= unit_hits) {
                        return false;
                    }
                }

                result = true;

            } else {
                result = true;
            }

        } else {
            result = true;
        }
    }

    return result;
}

void TaskMove::MoveAirUnit() {
    if (passenger->IsReadyForOrders(this)) {
        if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
            SmartPointer<UnitInfo> unit;
            bool flag = false;
            int step_index;

            for (step_index = planned_path.GetCount() - 1; step_index >= 0; --step_index) {
                if (point1 == *planned_path[step_index]) {
                    break;
                }
            }

            for (; step_index >= 0;) {
                point1 = *planned_path[step_index];

                --step_index;

                if (CheckAirPath()) {
                    flag = true;

                    for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(point1.x, point1.y)]; it != nullptr;
                         ++it) {
                        if ((*it).flags & MOBILE_AIR_UNIT) {
                            if ((*it).orders == ORDER_MOVE && (*it).state != ORDER_STATE_1) {
                                class RemindTurnEnd* reminder = new (std::nothrow) class RemindTurnEnd(*this);

                                TaskManager.AppendReminder(reminder);

                                return;

                            } else if ((*it).team == team && (*it).speed >= 2) {
                                if (!zone) {
                                    zone = new (std::nothrow) Zone(&*passenger, this);

                                    zone->Add(&point1);
                                    zone->SetField30(field_69);

                                    AiPlayer_Teams[team].ClearZone(&*zone);
                                }

                            } else if (step_index) {
                                flag = false;

                                break;

                            } else {
                                return;
                            }
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                         it != UnitsManager_MobileAirUnits.End(); ++it) {
                        if ((*it).orders == ORDER_MOVE && (*it).state != ORDER_STATE_1 && (*it).team == team &&
                            (*it).target_grid_x == point1.x && (*it).target_grid_y == point1.y) {
                            if (step_index) {
                                flag = false;

                                break;

                            } else {
                                return;
                            }
                        }
                    }

                    if (flag) {
                        break;
                    }
                }
            }

            passenger->target_grid_x = point1.x;
            passenger->target_grid_y = point1.y;

            field_70 = 1;

            passenger->ChangeField221(1, false);

            UnitsManager_SetNewOrder(&*passenger, ORDER_MOVE, ORDER_STATE_0);
        }
    }
}

bool TaskMove::IsPathClear() {
    bool result;

    if (Access_GetDistance(&*passenger, *planned_path[0]) > 2) {
        result = false;

    } else {
        int step_index;

        for (step_index = 0; step_index < planned_path.GetCount(); ++step_index) {
            if (Access_IsAccessible(passenger->unit_type, team, planned_path[step_index]->x,
                                    planned_path[step_index]->y, 2)) {
                if (point1 == *planned_path[step_index]) {
                    break;
                }

            } else {
                result = false;

                return result;
            }
        }

        if (planned_path.GetCount() != step_index) {
            SmartPointer<GroundPath> path = new (std::nothrow) GroundPath(point1.x, point1.y);

            if (passenger->grid_x != planned_path[0]->x || passenger->grid_y != planned_path[0]->y) {
                path->AddStep(planned_path[0]->x - passenger->grid_x, planned_path[0]->y - passenger->grid_y);
            }

            for (int i = 1; i <= step_index; ++i) {
                path->AddStep(planned_path[i]->x - planned_path[i - 1]->x, planned_path[i]->y - planned_path[i - 1]->y);
            }

            destination = point3;

            MoveUnit(&*path);

            result = true;

        } else {
            result = false;
        }
    }

    return result;
}

void TaskMove::FindCurrentLocation() {
    if (planned_path.GetCount()) {
        Point position(passenger->grid_x, passenger->grid_y);
        Point path_step;
        short** damage_potential_map = nullptr;
        int distance;
        int minimum_distance;
        int step_index;
        int minimum_distance_step_index = 0;
        int unit_hits;

        minimum_distance = TaskManager_GetDistance(position, *planned_path[0]) / 2;

        if (caution_level > CAUTION_LEVEL_NONE) {
            damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*passenger, caution_level, 1);
        }

        unit_hits = passenger->hits;

        if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
            unit_hits = 1;
        }

        for (step_index = 0; step_index < planned_path.GetCount(); ++step_index) {
            path_step = *planned_path[step_index];

            if (damage_potential_map && damage_potential_map[path_step.x][path_step.y] >= unit_hits) {
                planned_path.Clear();
                transporter_unit_type = INVALID_ID;

                return;
            }

            distance = TaskManager_GetDistance(position, path_step) / 2;

            if (distance <= minimum_distance) {
                minimum_distance = distance;
                minimum_distance_step_index = step_index;
            }
        }

        if (minimum_distance == 0) {
            planned_path.Remove(minimum_distance_step_index);
        }

        for (int i = 0; i < minimum_distance_step_index; ++i) {
            planned_path.Remove(0);
        }
    }
}

bool TaskMove::FindWaypoint() {
    bool result;
    int unit_speed = 0;
    int step_cost;
    int unit_hits = passenger->hits;
    short** damage_potential_map = nullptr;

    if (caution_level >= CAUTION_LEVEL_NONE) {
        damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*passenger, caution_level, 1);
    }

    if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
        unit_hits = 1;
    }

    SDL_assert(planned_path.GetCount() != 0);

    unit_speed = passenger->speed * 4;

    if (field_72) {
        unit_speed = 1;
    }

    point1.x = passenger->grid_x;
    point1.y = passenger->grid_y;

    for (int i = 0; i < planned_path.GetCount() && unit_speed > 0; ++i) {
        step_cost = Access_IsAccessible(passenger->unit_type, team, planned_path[i]->x, planned_path[i]->y, 1);

        if (planned_path[i]->x && planned_path[i]->y) {
            step_cost = (step_cost * 3) / 2;
        }

        unit_speed -= step_cost;

        point1 = *planned_path[i];
    }

    if (step_cost) {
        if (caution_level > CAUTION_LEVEL_NONE && damage_potential_map &&
            damage_potential_map[point1.x][point1.y] >= unit_hits) {
            result = false;

        } else {
            passenger->target_grid_x = point1.x;
            passenger->target_grid_y = point1.y;

            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskMove::MoveUnit(GroundPath* path) {
    if (passenger->IsReadyForOrders(this) && path &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team)) {
        if (caution_level > CAUTION_LEVEL_NONE && Task_ShouldReserveShot(&*passenger, point1)) {
            int unit_shots = passenger->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);
            int unit_speed = passenger->GetBaseValues()->GetAttribute(ATTRIB_SPEED);
            int speed = ((unit_speed + unit_shots - 1) / unit_shots);
            int velocity = (passenger->speed - speed) * 4;
            Point position(passenger->grid_x, passenger->grid_y);
            Point site;
            int step_cost = 0;
            SmartObjectArray<PathStep> steps = path->GetSteps();
            int step_index;

            for (step_index = 0; step_index < steps.GetCount(); ++step_index) {
                site.x = position.x + steps[step_index]->x;
                site.y = position.y + steps[step_index]->y;

                step_cost = Access_IsAccessible(passenger->unit_type, team, site.x, site.y, 2);

                if (site.x != position.x && site.y != position.y) {
                    step_cost = (step_cost * 3) / 2;
                }

                velocity -= step_cost;

                if (step_cost == 0 || velocity < 0) {
                    break;
                }

                position = site;
            }

            if (step_index > 0) {
                if ((position.x != point1.x || position.y != point1.y) && !(passenger->flags & MOBILE_AIR_UNIT) &&
                    steps.GetCount() > step_index) {
                    path = new (std::nothrow) GroundPath(position.x, position.y);

                    for (int i = 0; i <= step_index; ++i) {
                        path->AddStep(steps[i]->x, steps[i]->y);
                    }
                }

                point1 = position;

            } else if (step_cost) {
                passenger->speed = speed;

                return;
            }
        }

        passenger->target_grid_x = point1.x;
        passenger->target_grid_y = point1.y;
        field_70 = 1;
        passenger->ChangeField221(1, false);

        if (passenger->flags & MOBILE_AIR_UNIT) {
            UnitsManager_SetNewOrder(&*passenger, ORDER_MOVE, ORDER_STATE_0);

        } else {
            passenger->path = path;
            UnitsManager_SetNewOrder(&*passenger, ORDER_MOVE, ORDER_STATE_5);
        }
    }
}

UnitInfo* TaskMove::FindUnit(SmartList<UnitInfo>* units, unsigned short team, ResourceID unit_type) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type) {
            return &*it;
        }
    }

    return nullptr;
}

void TaskMove::FindTransport(ResourceID unit_type) {
    TaskTransport* transport = nullptr;
    int distance;
    int minimum_distance;

    transporter_unit_type = unit_type;

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
         ++it) {
        if ((*it).GetTeam() == team && (*it).GetType() == TaskType_TaskTransport) {
            TaskTransport* task = dynamic_cast<TaskTransport*>(&*it);

            if (task->GetTransporterType() == unit_type) {
                distance =
                    TaskManager_GetDistance(task->DeterminePosition(), Point(passenger->grid_x, passenger->grid_y));

                if (!transport || distance < minimum_distance) {
                    if (task->WillTransportNewClient(this)) {
                        transport = task;
                        minimum_distance = distance;
                    }
                }
            }
        }
    }

    if (transport) {
        transport->AddMove(this);

    } else {
        TaskTransport* task = new (std::nothrow) TaskTransport(this, transporter_unit_type);

        TaskManager.AppendTask(*task);
    }
}

bool TaskMove::TaskMove_sub_4D247() {
    return transporter_unit_type != INVALID_ID && passenger->grid_x == point2.x && passenger->grid_y == point2.y;
}

Point TaskMove::GetDestination() { return destination; }
