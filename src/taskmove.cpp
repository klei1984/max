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
#include "ailog.hpp"
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

TaskMove::TaskMove(UnitInfo* unit, Task* task, uint16_t minimum_distance_, uint8_t caution_level_,
                   Point destination, void (*result_callback_)(Task* task, UnitInfo* unit, char result))
    : Task(unit->team, task, task ? task->GetFlags() : 0x1000) {
    path_result = TASKMOVE_RESULT_SUCCESS;
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

    destination_waypoint = destination;
    passenger_waypoint = destination;
    transport_waypoint = destination;
    passenger_destination = destination;

    transporter_unit_type = INVALID_ID;

    result_callback = result_callback_;

    field_68 = false;
    field_69 = false;
    field_70 = false;
}

TaskMove::TaskMove(UnitInfo* unit, void (*result_callback_)(Task* task, UnitInfo* unit, char result))
    : Task(unit->team, nullptr, 0x2900) {
    passenger = unit;
    path_result = TASKMOVE_RESULT_SUCCESS;
    minimum_distance = 0;
    caution_level = CAUTION_LEVEL_AVOID_ALL_DAMAGE;

    destination_waypoint.x = -1;
    destination_waypoint.y = -1;
    transport_waypoint.x = -1;
    transport_waypoint.y = -1;
    passenger_waypoint.x = -1;
    passenger_waypoint.y = -1;
    passenger_destination.x = -1;
    passenger_destination.y = -1;

    transporter_unit_type = unit->GetParent()->unit_type;

    result_callback = result_callback_;

    field_68 = false;
    field_69 = false;
    field_70 = false;
}

TaskMove::~TaskMove() {}

int32_t TaskMove::GetCautionLevel(UnitInfo& unit) { return caution_level; }

char* TaskMove::WriteStatusLog(char* buffer) const {
    if (passenger) {
        if (planned_path.GetCount()) {
            Point waypoint = *planned_path[planned_path.GetCount() - 1];

            if (transporter_unit_type == INVALID_ID) {
                sprintf(buffer, "Move to [%i,%i]", waypoint.x + 1, waypoint.y + 1);

            } else {
                sprintf(buffer, "Move to [%i,%i] via [%i,%i] using %s", passenger_destination.x + 1,
                        passenger_destination.y + 1, waypoint.x + 1, waypoint.y + 1,
                        UnitsManager_BaseUnits[transporter_unit_type].singular_name);
            }

        } else if (transporter_unit_type == INVALID_ID) {
            strcpy(buffer, "Move: calculating.");

        } else {
            sprintf(buffer, "Move to [%i,%i] using %s", passenger_destination.x + 1, passenger_destination.y + 1,
                    UnitsManager_BaseUnits[transporter_unit_type].singular_name);
        }

    } else {
        strcpy(buffer, "Move: finished.");
    }

    return buffer;
}

uint8_t TaskMove::GetType() const { return TaskType_TaskMove; }

void TaskMove::SetField68(bool value) { field_68 = value; }

void TaskMove::SetField69(bool value) { field_69 = value; }

UnitInfo* TaskMove::GetPassenger() { return &*passenger; }

void TaskMove::SetDestination(Point site) { passenger_destination = site; }

ResourceID TaskMove::GetTransporterType() const { return transporter_unit_type; }

Point TaskMove::GetTransporterWaypoint() const { return transport_waypoint; }

void TaskMove::RemoveTransport() { transporter_unit_type = INVALID_ID; }

void TaskMove::AddUnit(UnitInfo& unit) {
    AiLog log("Move: add %s.", UnitsManager_BaseUnits[unit.unit_type].singular_name);

    if (passenger == &unit && unit.GetTask() == this) {
        Task_RemindMoveFinished(&*passenger, true);
    }
}

void TaskMove::Begin() {
    Task_RemoveMovementTasks(&*passenger);
    passenger->AddTask(this);

    if (passenger->orders == ORDER_IDLE) {
        RemindTurnStart(true);
    }

    if (transporter_unit_type == INVALID_ID && passenger->orders != ORDER_IDLE) {
        Search(true);
    }
}

void TaskMove::BeginTurn() {
    if (passenger) {
        AiLog log("Move %s: Begin Turn", UnitsManager_BaseUnits[passenger->unit_type].singular_name);

        if (passenger->orders == ORDER_IDLE && passenger->state == ORDER_STATE_4 &&
            (passenger_destination.x < 0 || passenger_destination.y < 0)) {
            SmartPointer<UnitInfo> transport(passenger->GetParent());

            SDL_assert(transport != nullptr);

            passenger_destination.x = transport->grid_x;
            passenger_destination.y = transport->grid_y;

            FindWaypointUsingTransport();

            Task_RemindMoveFinished(&*transport, true);
        }

        if (!zone && passenger->IsReadyForOrders(this)) {
            if (transporter_unit_type == INVALID_ID || planned_path.GetCount()) {
                if (passenger->speed > 0) {
                    Task_RemindMoveFinished(&*passenger, true);
                }

            } else {
                log.Log("Checking if direct route remains blocked.");

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
            Execute(*passenger);
        }
    }
}

bool TaskMove::Execute(UnitInfo& unit) {
    bool result;

    if (passenger == &unit) {
        AiLog log("Move finished for %s at [%i,%i].", UnitsManager_BaseUnits[unit.unit_type].singular_name,
                  unit.grid_x + 1, unit.grid_y + 1);

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
                                    int32_t unit_shots = passenger->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);
                                    int32_t unit_speed = passenger->GetBaseValues()->GetAttribute(ATTRIB_SPEED);

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
                                    log.Log("Find Direct path.");

                                    PathRequest* request =
                                        new (std::nothrow) TaskPathRequest(&*passenger, 2, passenger_waypoint);

                                    request->SetMaxCost(passenger->GetBaseValues()->GetAttribute(ATTRIB_SPEED) * 4);
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
                passenger->RemoveTasks();
                RemoveSelf();

                result = false;
            }

        } else {
            log.Log("Not ready for orders.");

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

    AiLog("Move: Remove passenger (self).");

    passenger = nullptr;
    zone = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskMove::RemoveUnit(UnitInfo& unit) {
    if (passenger == &unit) {
        AiLog log("Move: Remove %s.", UnitsManager_BaseUnits[unit.unit_type].singular_name);

        Finished(TASKMOVE_RESULT_CANCELLED);
    }
}

void TaskMove::EventZoneCleared(Zone* zone_, bool status) {
    AiLog log("Move: Zone Cleared");

    if (zone == zone_) {
        zone = nullptr;
    }

    if (passenger) {
        if (!status && field_69) {
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
            int32_t team_id;

            for (team_id = PLAYER_TEAM_RED; team_id < PLAYER_TEAM_MAX - 1; ++team_id) {
                if (UnitsManager_TeamInfo[team_id].team_type != TEAM_TYPE_NONE) {
                    if (team_id != team) {
                        if (UnitsManager_TeamInfo[team_id]
                                .heat_map_complete[ResourceManager_MapSize.x * passenger->grid_y + passenger->grid_x] >
                            0) {
                            break;
                        }
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
    AiLog log("Attempt %s.", UnitsManager_BaseUnits[unit_type].singular_name);

    PathRequest* request = new (std::nothrow) TaskPathRequest(&*passenger, 0, destination_waypoint);

    request->SetMinimumDistance(minimum_distance);
    request->SetCautionLevel(caution_level);
    request->CreateTransport(unit_type);

    SmartPointer<Task> find_path(new (std::nothrow)
                                     TaskFindPath(this, request, &PathResultCallback, &PathCancelCallback));

    TaskManager.AppendTask(*find_path);
}

void TaskMove::FindWaypointUsingTransport() {
    bool keep_searching = true;
    Point site = passenger_destination;
    Rect bounds;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (Access_IsAccessible(passenger->unit_type, team, site.x, site.y, 2) &&
        !Ai_IsDangerousLocation(&*passenger, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, true)) {
        destination_waypoint = passenger_destination;
    }

    for (int32_t range_limit = 2; keep_searching; range_limit += 2) {
        --site.x;
        ++site.y;
        keep_searching = false;

        for (int32_t direction = 0; direction < 8; direction += 2) {
            for (int32_t range = 0; range < range_limit; ++range) {
                site += Paths_8DirPointsArray[direction];

                if (Access_IsInsideBounds(&bounds, &site)) {
                    keep_searching = true;

                    if (Access_IsAccessible(passenger->unit_type, team, site.x, site.y, 1) &&
                        !Ai_IsDangerousLocation(&*passenger, site, CAUTION_LEVEL_AVOID_NEXT_TURNS_FIRE, true)) {
                        passenger_destination = site;
                        destination_waypoint = site;

                        return;
                    }
                }
            }
        }
    }
}

void TaskMove::Search(bool mode) {
    AiLog log("Move %s: find path.", UnitsManager_BaseUnits[passenger->unit_type].singular_name);

    Point line_distance(destination_waypoint.x - passenger->grid_x, destination_waypoint.y - passenger->grid_y);

    if (line_distance.x * line_distance.x + line_distance.y * line_distance.y > minimum_distance) {
        TaskPathRequest* request = new (std::nothrow) TaskPathRequest(&*passenger, 1, destination_waypoint);
        int32_t distance;

        path_result = TASKMOVE_RESULT_SUCCESS;

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

void TaskMove::Finished(int32_t result) {
    if (passenger) {
        const char* result_codes[] = {"success", "already in range", "blocked", "cancelled"};

        AiLog log("Move %s: finished (%s)", UnitsManager_BaseUnits[passenger->unit_type].singular_name,
                  result_codes[result]);

        passenger->RemoveTask(this, false);

        if (parent) {
            result_callback(&*parent, &*passenger, result);
        }
    }

    AiLog("Move: Remove passenger (finish).");

    passenger = nullptr;
    zone = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskMove::PathResultCallback(Task* task, PathRequest* path_request, Point destination_, GroundPath* path,
                                  uint8_t result) {
    SmartPointer<TaskMove> move(dynamic_cast<TaskMove*>(task));

    if (path) {
        move->transporter_unit_type = path_request->GetTransporter()->unit_type;
        move->path_result = result;

        move->TranscribeTransportPath(destination_, path);

    } else {
        if (path_request->GetTransporter()->unit_type != AIRTRANS && Builder_IsBuildable(AIRTRANS) &&
            (FindUnit(&UnitsManager_MobileAirUnits, move->GetTeam(), AIRTRANS) ||
             FindUnit(&UnitsManager_StationaryUnits, move->GetTeam(), AIRPLT))) {
            move->AttemptTransportType(AIRTRANS);

        } else {
            move->Finished(TASKMOVE_RESULT_BLOCKED);
        }
    }
}

void TaskMove::FullPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                      uint8_t result) {
    AiLog log("Move full path result.");

    SmartPointer<TaskMove> move(dynamic_cast<TaskMove*>(task));

    if (path) {
        move->path_result = result;

        if (path_request->GetTransporter()) {
            move->transporter_unit_type = path_request->GetTransporter()->unit_type;

        } else {
            move->transporter_unit_type = INVALID_ID;
        }

        move->transport_waypoint = move->destination_waypoint;
        move->passenger_destination = move->destination_waypoint;

        if (result == 0 && path_request->GetCautionLevel() == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
            if ((TaskManager_GetDistance(move->passenger->grid_x - move->destination_waypoint.x,
                                         move->passenger->grid_y - move->destination_waypoint.y) /
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
            new (std::nothrow) TaskPathRequest(path_request->GetClient(), 1, path_request->GetDestination());

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
                                        uint8_t result) {
    SmartPointer<TaskMove> move = dynamic_cast<TaskMove*>(task);

    AiLog log("Direct path result.");

    if (move == dynamic_cast<TaskMove*>(move->passenger->GetTask())) {
        if (path) {
            if (move->caution_level > CAUTION_LEVEL_NONE && (move->passenger->flags & MOBILE_AIR_UNIT)) {
                log.Log("Adjusting path.");

                AdjustRequest* request =
                    new (std::nothrow) AdjustRequest(&*move->passenger, 2, move->passenger_waypoint, path);

                SmartPointer<Task> find_path(
                    new (std::nothrow) TaskFindPath(&*move, request, &ActualPathResultCallback, &PathCancelCallback));

                TaskManager.AppendTask(*find_path);

            } else {
                move->MoveUnit(path);
            }

        } else {
            log.Log("Looking for alternate path.");

            PathRequest* request = new (std::nothrow) TaskPathRequest(&*move->passenger, 1, move->passenger_waypoint);

            request->SetMaxCost(move->passenger->GetBaseValues()->GetAttribute(ATTRIB_SPEED) * 4);
            request->SetCautionLevel(move->caution_level);

            SmartPointer<Task> find_path(
                new (std::nothrow) TaskFindPath(&*move, request, &BlockedPathResultCallback, &PathCancelCallback));

            TaskManager.AppendTask(*find_path);
        }

    } else {
        log.Log("Unit currently belongs to another task.");
    }
}

void TaskMove::BlockedPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                         uint8_t result) {
    SmartPointer<TaskMove> move = dynamic_cast<TaskMove*>(task);

    AiLog log("Blocked path result.");

    if (move == dynamic_cast<TaskMove*>(move->passenger->GetTask())) {
        if (path) {
            SmartPointer<UnitInfo> unit;
            SmartObjectArray<PathStep> steps = path->GetSteps();
            bool flag = false;
            Point site;
            SmartPointer<Zone> local_zone = new (std::nothrow) Zone(&*move->passenger, &*move);
            uint32_t unit_flags;

            if (move->passenger->flags & MOBILE_AIR_UNIT) {
                unit_flags = MOBILE_AIR_UNIT;

            } else {
                unit_flags = MOBILE_SEA_UNIT | MOBILE_LAND_UNIT;
            }

            site.x = move->passenger->grid_x;
            site.y = move->passenger->grid_y;

            local_zone->Add(&site);

            for (int32_t i = 0; i < steps.GetCount(); ++i) {
                site.x += steps[i]->x;
                site.y += steps[i]->y;

                local_zone->Add(&site);

                unit = Access_GetUnit4(site.x, site.y, move->team, unit_flags);

                if (unit) {
                    log.Log("Blocked by %s at [%i,%i].", UnitsManager_BaseUnits[unit->unit_type].singular_name,
                            site.x + 1, site.y + 1);

                    flag = true;

                    if (unit == move->passenger) {
                        log.Log("Unit blocks itself!");

                        move->Finished(TASKMOVE_RESULT_BLOCKED);

                        return;
                    }
                }
            }

            if (flag) {
                move->zone = local_zone;
                move->zone->SetField30(move->field_69);

                AiPlayer_Teams[move->team].ClearZone(&*local_zone);

            } else {
                move->passenger_waypoint = site;
                move->MoveUnit(path);
            }

        } else {
            move->RetryMove();
        }

    } else {
        log.Log("Unit currently belongs to another task.");
    }
}

void TaskMove::ActualPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                        uint8_t result) {
    SmartPointer<TaskMove> move = dynamic_cast<TaskMove*>(task);
    AdjustRequest* request = dynamic_cast<AdjustRequest*>(path_request);

    AiLog log("Actual path result.");

    if (move == dynamic_cast<TaskMove*>(move->passenger->GetTask())) {
        if (path) {
            SmartObjectArray<PathStep> adjusted_steps = request->GetPath()->GetSteps();
            SmartObjectArray<PathStep> steps = path->GetSteps();
            Point position(move->passenger->grid_x, move->passenger->grid_y);
            int32_t count = adjusted_steps->GetCount();
            int32_t step_index;

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

            move->passenger_waypoint = position;

            if (step_index != count) {
                SmartPointer<GroundPath> ground_path = new (std::nothrow) GroundPath(position.x, position.y);

                for (int32_t i = 0; i <= step_index; ++i) {
                    ground_path->AddStep(adjusted_steps[i]->x, adjusted_steps[i]->y);
                }

                move->MoveUnit(&*ground_path);

            } else {
                move->MoveUnit(path);
            }

        } else {
            log.Log("Problem: actual path is null.");

            move->Finished(TASKMOVE_RESULT_BLOCKED);
        }

    } else {
        log.Log("Unit currently belongs to another task.");
    }
}

void TaskMove::PathCancelCallback(Task* task, PathRequest* path_request) {
    dynamic_cast<TaskMove*>(task)->Finished(TASKMOVE_RESULT_CANCELLED);
}

void TaskMove::ProcessPath(Point site, GroundPath* path) {
    SmartObjectArray<PathStep> steps = path->GetSteps();
    Point line_distance;
    int32_t distance_minimum = minimum_distance;

    if (transporter_unit_type != INVALID_ID) {
        distance_minimum = 0;
    }

    field_70 = false;

    for (int32_t i = 0; i < steps.GetCount(); ++i) {
        site.x += steps[i]->x;
        site.y += steps[i]->y;

        planned_path.Append(&site);

        line_distance.x = transport_waypoint.x - site.x;
        line_distance.y = transport_waypoint.y - site.y;

        if (line_distance.x * line_distance.x + line_distance.y * line_distance.y <= distance_minimum) {
            break;
        }
    }

    if (planned_path.GetCount() > 0) {
        Task_RemindMoveFinished(&*passenger, true);

    } else if (transporter_unit_type == INVALID_ID) {
        AiLog log("Result had no steps.");

        Finished(TASKMOVE_RESULT_BLOCKED);

    } else {
        Task_RemindMoveFinished(&*passenger);
    }
}

void TaskMove::RetryMove() {
    if (field_70) {
        Finished(TASKMOVE_RESULT_SUCCESS);

    } else if (path_result) {
        AiLog log("Re-trying move without precalculated paths.");

        planned_path.Clear();
        transport_waypoint = destination_waypoint;
        transporter_unit_type = INVALID_ID;

        Search(false);

    } else {
        Finished(TASKMOVE_RESULT_BLOCKED);
    }
}

void TaskMove::TranscribeTransportPath(Point site, GroundPath* path) {
    Point position = site;
    SmartObjectArray<PathStep> steps = path->GetSteps();
    int32_t block_count = 0;
    bool flag1 = false;
    bool needs_transport = false;

    AiLog log("Transcribe transport path.");

    for (int32_t i = 0; i < steps.GetCount(); ++i) {
        position.x += steps[i]->x;
        position.y += steps[i]->y;

        if (Access_IsAccessible(passenger->unit_type, team, position.x, position.y, 0x00)) {
            if (flag1) {
                passenger_destination = position;
                flag1 = false;

                break;
            }

        } else {
            if (!flag1) {
                transport_waypoint.x = position.x - steps[i]->x;
                transport_waypoint.y = position.y - steps[i]->y;

                flag1 = true;
                needs_transport = true;
            }

            ++block_count;
        }
    }

    if (needs_transport) {
        if (flag1) {
            log.Log("No landing spot.");

            if (path_result) {
                Search(false);

            } else {
                Finished(TASKMOVE_RESULT_BLOCKED);
            }

        } else {
            if (!path_result) {
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

                    for (int32_t i = 0; i < steps.GetCount(); ++i) {
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

            if (site == transport_waypoint) {
                Execute(*passenger);

            } else {
                ProcessPath(site, path);
            }
        }

    } else {
        log.Log("Transport not needed?!");

        ProcessPath(site, path);
    }
}

bool TaskMove::CheckAirPath() {
    bool result;

    AiLog log("Check air path.");

    if (caution_level == CAUTION_LEVEL_NONE) {
        result = true;

    } else {
        int16_t** damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*passenger, caution_level, true);

        if (damage_potential_map) {
            int32_t line_distance_x = passenger_waypoint.x - passenger->grid_x;
            int32_t line_distance_y = passenger_waypoint.y - passenger->grid_y;
            Point site;
            int32_t distance = line_distance_x * line_distance_x + line_distance_y * line_distance_y;
            int32_t range_limit = sqrt(distance) * 4.0 + 0.5;
            int32_t unit_hits = passenger->hits;

            if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
                unit_hits = 1;
            }

            if (range_limit) {
                line_distance_x = (line_distance_x << 22) / range_limit;
                line_distance_y = (line_distance_y << 22) / range_limit;

                for (int32_t i = 0; i < range_limit; ++i) {
                    site.x = (((i * line_distance_x) >> 16) + passenger->x) >> 6;
                    site.y = (((i * line_distance_y) >> 16) + passenger->y) >> 6;

                    if (damage_potential_map[site.x][site.y] >= unit_hits) {
                        log.Log("[%i,%i] is too dangerous", site.x + 1, site.y + 1);

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
        AiLog log("Move air unit.");

        if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
            SmartPointer<UnitInfo> unit;
            bool flag = false;
            int32_t step_index;

            for (step_index = planned_path.GetCount() - 1; step_index >= 0; --step_index) {
                if (passenger_waypoint == *planned_path[step_index]) {
                    break;
                }
            }

            for (; step_index >= 0;) {
                passenger_waypoint = *planned_path[step_index];

                --step_index;

                if (CheckAirPath()) {
                    flag = true;

                    const auto units = Hash_MapHash[passenger_waypoint];

                    if (units) {
                        for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
                            if ((*it).flags & MOBILE_AIR_UNIT) {
                                log.Log("Blocked by %s at [%i,%i]",
                                        UnitsManager_BaseUnits[(*it).unit_type].singular_name, (*it).grid_x + 1,
                                        (*it).grid_y + 1);

                                if ((*it).orders == ORDER_MOVE && (*it).state != ORDER_STATE_1) {
                                    log.Log("Blocker is moving.");

                                    class RemindTurnEnd* reminder = new (std::nothrow) class RemindTurnEnd(*this);

                                    TaskManager.AppendReminder(reminder);

                                    return;

                                } else if ((*it).team == team && (*it).speed >= 2) {
                                    log.Log("Attempting to displace blocker.");

                                    if (!zone) {
                                        zone = new (std::nothrow) Zone(&*passenger, this);

                                        zone->Add(&passenger_waypoint);
                                        zone->SetField30(field_69);

                                        AiPlayer_Teams[team].ClearZone(&*zone);
                                    }

                                } else {
                                    log.Log("Can't move blocker.");

                                    if (step_index) {
                                        flag = false;

                                        break;

                                    } else {
                                        return;
                                    }
                                }
                            }
                        }
                    }

                    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
                         it != UnitsManager_MobileAirUnits.End(); ++it) {
                        if ((*it).orders == ORDER_MOVE && (*it).state != ORDER_STATE_1 && (*it).team == team &&
                            (*it).target_grid_x == passenger_waypoint.x &&
                            (*it).target_grid_y == passenger_waypoint.y) {
                            log.Log("[%i,%i] is a destination for %s at [%i,%i].", passenger_waypoint.x + 1,
                                    passenger_waypoint.y + 1, UnitsManager_BaseUnits[(*it).unit_type].singular_name,
                                    (*it).grid_x + 1, (*it).grid_y + 1);

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

            log.Log("Air move issued to [%i,%i].", passenger_waypoint.x + 1, passenger_waypoint.y + 1);

            passenger->target_grid_x = passenger_waypoint.x;
            passenger->target_grid_y = passenger_waypoint.y;

            field_70 = true;

            passenger->ChangeField221(1, false);

            UnitsManager_SetNewOrder(&*passenger, ORDER_MOVE, ORDER_STATE_0);
        }
    }
}

bool TaskMove::IsPathClear() {
    bool result;

    AiLog log("Check if path is clear.");

    if (Access_GetDistance(&*passenger, *planned_path[0]) > 2) {
        log.Log("[%i,%i] is too far away.", planned_path[0]->x + 1, planned_path[0]->y + 1);

        result = false;

    } else {
        int32_t step_index;

        for (step_index = 0; step_index < planned_path.GetCount(); ++step_index) {
            if (Access_IsAccessible(passenger->unit_type, team, planned_path[step_index]->x,
                                    planned_path[step_index]->y, 2)) {
                if (passenger_waypoint == *planned_path[step_index]) {
                    break;
                }

            } else {
                log.Log("Can't enter [%i,%i]", planned_path[step_index]->x + 1, planned_path[step_index]->y + 1);

                result = false;

                return result;
            }
        }

        if (planned_path.GetCount() != step_index) {
            SmartPointer<GroundPath> path = new (std::nothrow) GroundPath(passenger_waypoint.x, passenger_waypoint.y);

            if (passenger->grid_x != planned_path[0]->x || passenger->grid_y != planned_path[0]->y) {
                path->AddStep(planned_path[0]->x - passenger->grid_x, planned_path[0]->y - passenger->grid_y);
            }

            for (int32_t i = 1; i <= step_index; ++i) {
                path->AddStep(planned_path[i]->x - planned_path[i - 1]->x, planned_path[i]->y - planned_path[i - 1]->y);
            }

            passenger_destination = destination_waypoint;

            SDL_assert(path->GetSteps()->GetCount() > 0);

            MoveUnit(&*path);

            result = true;

        } else {
            log.Log("Waypoint isn't on path!");

            result = false;
        }
    }

    return result;
}

void TaskMove::FindCurrentLocation() {
    AiLog log("Find current location.");

    if (planned_path.GetCount()) {
        Point position(passenger->grid_x, passenger->grid_y);
        Point path_step;
        int16_t** damage_potential_map = nullptr;
        int32_t distance;
        int32_t minimum_distance;
        int32_t step_index;
        int32_t minimum_distance_step_index = 0;
        int32_t unit_hits;

        minimum_distance = TaskManager_GetDistance(position, *planned_path[0]) / 2;

        if (caution_level > CAUTION_LEVEL_NONE) {
            damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*passenger, caution_level, true);
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
            log.Log("Removing [%i,%i]", planned_path[minimum_distance_step_index]->x + 1,
                    planned_path[minimum_distance_step_index]->y + 1);

            planned_path.Remove(minimum_distance_step_index);
        }

        for (int32_t i = 0; i < minimum_distance_step_index; ++i) {
            log.Log("Removing [%i,%i]", planned_path[0]->x + 1, planned_path[0]->y + 1);

            planned_path.Remove(0);
        }
    }
}

bool TaskMove::FindWaypoint() {
    bool result;
    int32_t unit_speed = 0;
    int32_t step_cost = 0;
    int32_t unit_hits = passenger->hits;

    AiLog log("Find waypoint.");

    if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
        unit_hits = 1;
    }

    SDL_assert(planned_path.GetCount() != 0);

    unit_speed = passenger->speed * 4;

    passenger_waypoint.x = passenger->grid_x;
    passenger_waypoint.y = passenger->grid_y;

    for (int32_t i = 0; i < planned_path.GetCount() && unit_speed > 0; ++i) {
        step_cost = Access_IsAccessible(passenger->unit_type, team, planned_path[i]->x, planned_path[i]->y, 1);

        if (planned_path[i]->x && planned_path[i]->y) {
            step_cost = (step_cost * 3) / 2;
        }

        unit_speed -= step_cost;

        passenger_waypoint = *planned_path[i];
    }

    if (step_cost) {
        int16_t** damage_potential_map = nullptr;

        if (caution_level > CAUTION_LEVEL_NONE) {
            damage_potential_map = AiPlayer_Teams[team].GetDamagePotentialMap(&*passenger, caution_level, true);
        }

        if (damage_potential_map && damage_potential_map[passenger_waypoint.x][passenger_waypoint.y] >= unit_hits) {
            log.Log("[%i,%i] is too dangerous.", passenger_waypoint.x + 1, passenger_waypoint.y + 1);

            result = false;

        } else {
            log.Log("Found [%i,%i].", passenger_waypoint.x + 1, passenger_waypoint.y + 1);

            passenger->target_grid_x = passenger_waypoint.x;
            passenger->target_grid_y = passenger_waypoint.y;

            result = true;
        }

    } else {
        log.Log("Can't enter [%i,%i].", passenger_waypoint.x + 1, passenger_waypoint.y + 1);

        result = false;
    }

    return result;
}

void TaskMove::MoveUnit(GroundPath* path) {
    if (passenger->IsReadyForOrders(this) && path &&
        (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team)) {
        AiLog log("Moving unit to [%i,%i].", passenger_waypoint.x + 1, passenger_waypoint.y + 1);

        if (caution_level > CAUTION_LEVEL_NONE && Task_ShouldReserveShot(&*passenger, passenger_waypoint)) {
            int32_t unit_shots = passenger->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);
            int32_t unit_speed = passenger->GetBaseValues()->GetAttribute(ATTRIB_SPEED);
            int32_t speed = ((unit_speed + unit_shots - 1) / unit_shots);
            int32_t velocity = (passenger->speed - speed) * 4;
            Point position(passenger->grid_x, passenger->grid_y);
            Point site;
            int32_t step_cost = 0;
            SmartObjectArray<PathStep> steps = path->GetSteps();
            int32_t step_index;

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
                if ((position.x != passenger_waypoint.x || position.y != passenger_waypoint.y) &&
                    !(passenger->flags & MOBILE_AIR_UNIT) && steps.GetCount() > step_index) {
                    path = new (std::nothrow) GroundPath(position.x, position.y);

                    for (int32_t i = 0; i <= step_index; ++i) {
                        path->AddStep(steps[i]->x, steps[i]->y);
                    }
                }

                log.Log("New waypoint [%i,%i].", position.x + 1, position.y + 1);

                passenger_waypoint = position;

            } else if (step_cost) {
                passenger->speed = speed;

                return;
            }
        }

        log.Log("Move order issued to [%i,%i]", passenger_waypoint.x + 1, passenger_waypoint.y + 1);

        passenger->target_grid_x = passenger_waypoint.x;
        passenger->target_grid_y = passenger_waypoint.y;
        field_70 = true;
        passenger->ChangeField221(1, false);

        if (passenger->flags & MOBILE_AIR_UNIT) {
            UnitsManager_SetNewOrder(&*passenger, ORDER_MOVE, ORDER_STATE_0);

        } else {
            passenger->path = path;
            UnitsManager_SetNewOrder(&*passenger, ORDER_MOVE, ORDER_STATE_5);
        }
    }
}

UnitInfo* TaskMove::FindUnit(SmartList<UnitInfo>* units, uint16_t team, ResourceID unit_type) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type) {
            return &*it;
        }
    }

    return nullptr;
}

void TaskMove::FindTransport(ResourceID unit_type) {
    TaskTransport* transport = nullptr;
    int32_t distance;
    int32_t minimum_distance{INT32_MAX};

    AiLog log("Find transport.");

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
        transport->AddClient(this);

    } else {
        TaskTransport* task = new (std::nothrow) TaskTransport(this, transporter_unit_type);

        TaskManager.AppendTask(*task);
    }
}

bool TaskMove::IsReadyForTransport() {
    return transporter_unit_type != INVALID_ID && passenger->grid_x == transport_waypoint.x &&
           passenger->grid_y == transport_waypoint.y;
}

Point TaskMove::GetDestination() { return passenger_destination; }
