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

#include "taskactivate.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TaskActivate::TaskActivate(UnitInfo* unit, Task* task, UnitInfo* parent_) : Task(unit->team, task, task->GetFlags()) {
    unit_to_activate = unit;
    unit_parent = parent_;
}

TaskActivate::~TaskActivate() {}

void TaskActivate::Activate() {
    if (unit_to_activate != nullptr) {
        if (GameManager_PlayMode != PLAY_MODE_TURN_BASED || GameManager_ActiveTurnTeam == team) {
            if (GameManager_PlayMode != PLAY_MODE_UNKNOWN) {
                if (unit_to_activate->GetTask() == this && zone == nullptr) {
                    if (unit_to_activate->orders == ORDER_IDLE || unit_to_activate->orders == ORDER_BUILD ||
                        unit_to_activate->orders == ORDER_AWAIT) {
                        if (unit_parent != nullptr) {
                            if (unit_to_activate->orders == ORDER_AWAIT) {
                                Rect bounds;

                                rect_init(&bounds, 0, 0, 0, 0);

                                unit_parent->GetBounds(&bounds);

                                Point position(unit_to_activate->grid_x, unit_to_activate->grid_y);

                                if (!Access_IsInsideBounds(&bounds, &position)) {
                                    RemindTurnEnd(true);
                                    return;
                                }
                            }

                            AiLog log("Activate %s.",
                                      UnitsManager_BaseUnits[unit_to_activate->unit_type].singular_name);

                            if (unit_to_activate->orders != ORDER_IDLE || unit_parent->orders == ORDER_BUILD ||
                                unit_parent->orders == ORDER_AWAIT) {
                                Point position(unit_parent->grid_x - 1, unit_parent->grid_y);
                                int unit_size;

                                if (unit_parent->flags & BUILDING) {
                                    unit_size = 2;

                                } else {
                                    unit_size = 1;
                                }

                                position.y += unit_size;

                                for (int direction = 0; direction < 8; direction += 2) {
                                    for (int range = 0; range < unit_size + 1; ++range) {
                                        position += Paths_8DirPointsArray[direction];

                                        if (Access_IsAccessible(unit_to_activate->unit_type, team, position.x,
                                                                position.y, 0x02)) {
                                            log.Log("Open square: [%i,%i].", position.x + 1, position.y + 1);

                                            switch (unit_to_activate->orders) {
                                                case ORDER_BUILD: {
                                                    unit_to_activate->target_grid_x = position.x;
                                                    unit_to_activate->target_grid_y = position.y;

                                                    UnitsManager_SetNewOrder(&*unit_to_activate, ORDER_ACTIVATE,
                                                                             ORDER_STATE_6);
                                                } break;

                                                case ORDER_IDLE: {
                                                    if (unit_to_activate->prior_orders == ORDER_BUILD &&
                                                        unit_to_activate->prior_state == ORDER_STATE_UNIT_READY &&
                                                        unit_parent->prior_state == ORDER_STATE_BUILDING_READY) {
                                                        SDL_assert(false);
                                                    }

                                                    unit_parent->target_grid_x = position.x;
                                                    unit_parent->target_grid_y = position.y;

                                                    unit_parent->SetParent(&*unit_to_activate);

                                                    UnitsManager_SetNewOrder(&*unit_parent, ORDER_ACTIVATE,
                                                                             ORDER_STATE_1);

                                                } break;

                                                case ORDER_AWAIT: {
                                                    unit_to_activate->target_grid_x = position.x;
                                                    unit_to_activate->target_grid_y = position.y;

                                                    UnitsManager_SetNewOrder(&*unit_to_activate, ORDER_MOVE,
                                                                             ORDER_STATE_0);
                                                } break;
                                            }

                                            return;
                                        }
                                    }
                                }

                                for (int direction = 0; direction < 8; direction += 2) {
                                    for (int range = 0; range < unit_size + 1; ++range) {
                                        position += Paths_8DirPointsArray[direction];

                                        if (Access_IsAccessible(unit_to_activate->unit_type, team, position.x,
                                                                position.y, 0x01)) {
                                            log.Log("Clearing square: [%i,%i].", position.x + 1, position.y + 1);

                                            zone = new (std::nothrow) Zone(&*unit_to_activate, this);

                                            zone->Add(&position);

                                            AiPlayer_Teams[team].ClearZone(&*zone);

                                            return;
                                        }
                                    }
                                }

                            } else {
                                log.Log("%s is not ready for orders.",
                                        UnitsManager_BaseUnits[unit_parent->unit_type].singular_name);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool TaskActivate::Task_vfunc1(UnitInfo& unit) { return unit_to_activate != unit; }

char* TaskActivate::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Activate unit");

    return buffer;
}

Rect* TaskActivate::GetBounds(Rect* bounds) {
    if (unit_to_activate != nullptr) {
        if (unit_to_activate->GetParent()) {
            unit_to_activate->GetParent()->GetBounds(bounds);

        } else {
            unit_to_activate->GetBounds(bounds);
        }
    }

    return bounds;
}

unsigned char TaskActivate::GetType() const { return TaskType_TaskActivate; }

void TaskActivate::AddUnit(UnitInfo& unit) {
    if (&*unit_to_activate == &unit) {
        Task_RemindMoveFinished(&*unit_to_activate, true);
    }
}

void TaskActivate::Begin() {
    unit_to_activate->AddTask(this);
    Activate();
}

void TaskActivate::EndTurn() {
    if (unit_to_activate != nullptr) {
        Execute(*unit_to_activate);
    }
}

bool TaskActivate::Execute(UnitInfo& unit) {
    bool result;

    if (unit_to_activate != nullptr && unit_to_activate == unit) {
        if (unit_to_activate->orders != ORDER_IDLE && unit_to_activate->orders != ORDER_BUILD &&
            unit_to_activate->orders != ORDER_EXPLODE && unit_to_activate->orders != ORDER_AWAIT_SCALING &&
            unit_to_activate->state != ORDER_STATE_14) {
            SmartPointer<Task> task(this);
            Rect bounds;

            rect_init(&bounds, 0, 0, 0, 0);

            unit_parent->GetBounds(&bounds);

            Point position(unit_to_activate->grid_x, unit_to_activate->grid_y);

            if (!Access_IsInsideBounds(&bounds, &position)) {
                AiLog log("Completed activation of %s.",
                          UnitsManager_BaseUnits[unit_to_activate->unit_type].singular_name);

                unit_to_activate->RemoveTask(this);

                if (parent->GetType() == TaskType_TaskCreateUnit) {
                    parent->AddUnit(*unit_to_activate);
                }

                if (!unit_to_activate->GetTask()) {
                    TaskManager.RemindAvailable(&*unit_to_activate);
                }

                unit_to_activate = nullptr;
                unit_parent = nullptr;

                TaskManager.RemoveTask(*this);

                result = false;

                return result;
            }
        }

        Activate();

        result = true;

    } else {
        result = false;
    }

    return result;
}

void TaskActivate::RemoveSelf() {
    if (unit_to_activate != nullptr) {
        unit_to_activate->RemoveTask(this);
    }

    unit_to_activate = nullptr;
    unit_parent = nullptr;
    parent = nullptr;
    zone = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskActivate::RemoveUnit(UnitInfo& unit) {
    if (unit_to_activate == unit) {
        AiLog log("Removing %s from Activate Unit.", UnitsManager_BaseUnits[unit_to_activate->unit_type].singular_name);

        unit_to_activate = nullptr;
        zone = nullptr;
        parent = nullptr;
        unit_parent = nullptr;

        TaskManager.RemoveTask(*this);
    }
}

void TaskActivate::EventZoneCleared(Zone* zone_, bool status) {
    if (zone == zone_) {
        zone = nullptr;
        Activate();
    }
}
