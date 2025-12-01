/* Copyright (c) 2021 M.A.X. Port Team
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

#include "groundpath.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "net_packet.hpp"
#include "paths.hpp"
#include "paths_manager.hpp"
#include "registerarray.hpp"
#include "resource_manager.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"

static uint32_t Paths_GroundPath_TypeIndex;
static RegisterClass Paths_GroundPath_ClassRegister("GroundPath", &Paths_GroundPath_TypeIndex, &GroundPath::Allocate);

GroundPath::GroundPath() : m_step_index(0) {}

GroundPath::GroundPath(int32_t target_x, int32_t target_y) : UnitPath(target_x, target_y), m_step_index(0) {}

GroundPath::~GroundPath() {}

FileObject* GroundPath::Allocate() noexcept { return new (std::nothrow) GroundPath(); }

uint32_t GroundPath::GetTypeIndex() const { return Paths_GroundPath_TypeIndex; }

void GroundPath::FileLoad(SmartFileReader& file) noexcept {
    uint32_t count;
    PathStep step;

    file.Read(m_end_x);
    file.Read(m_end_y);

    if (file.GetFormat() == SmartFileFormat::V70) {
        uint16_t local_index;

        file.Read(local_index);

        m_step_index = local_index;

    } else {
        file.Read(m_step_index);
    }

    count = file.ReadObjectCount();

    m_steps.Clear();

    for (uint32_t i = 0; i < count; ++i) {
        file.Read(step);
        m_steps.PushBack(&step);
    }
}

void GroundPath::FileSave(SmartFileWriter& file) noexcept {
    uint32_t count;
    PathStep step;

    file.Write(m_end_x);
    file.Write(m_end_y);
    file.Write(m_step_index);

    count = m_steps.GetCount();

    file.WriteObjectCount(count);

    for (uint32_t i = 0; i < count; ++i) {
        file.Write(*m_steps[i]);
    }
}

Point GroundPath::GetPosition(UnitInfo* unit) const {
    Point position(unit->grid_x, unit->grid_y);
    int32_t speed = unit->speed;

    if (unit->GetOrder() != ORDER_BUILD && unit->GetOrderState() == ORDER_STATE_EXECUTING_ORDER && speed > 0) {
        int32_t move_fraction;
        Point point;
        PathStep* step;
        int32_t step_cost;

        move_fraction = unit->move_fraction;

        for (uint32_t i = m_step_index; i < m_steps->GetCount(); ++i) {
            step = m_steps[i];

            point.x = position.x + step->x;
            point.y = position.y + step->y;

            if (point.x >= 0 && point.x < ResourceManager_MapSize.x && point.y >= 0 &&
                point.y < ResourceManager_MapSize.y) {
                step_cost =
                    Access_IsAccessible(unit->GetUnitType(), unit->team, point.x, point.y, AccessModifier_NoModifiers);

                if (step->x && step->y) {
                    step_cost = (step_cost * 3) / 2;
                }

                if (move_fraction > step_cost) {
                    move_fraction = step_cost;
                }

                step_cost -= move_fraction;

                move_fraction = 0;

                if (step_cost <= speed) {
                    position = point;

                } else {
                    break;
                }
            }
        }
    }

    return position;
}

bool GroundPath::IsInPath(int32_t grid_x, int32_t grid_y) const {
    Point point(0, 0);

    for (uint32_t i = m_step_index; i < m_steps.GetCount(); ++i) {
        if (point.x == grid_x && point.y == grid_y) {
            return true;
        }

        point.x += m_steps[i]->x;
        point.y += m_steps[i]->y;
    }

    return point.x == grid_x && point.y == grid_y;
}

void GroundPath::CancelMovement(UnitInfo* unit) {
    AILOG(log, "Ground path: emergency stop.");

    if (unit->GetOrderState() == ORDER_STATE_IN_PROGRESS) {
        AILOG_LOG(log, "In turn state.");

        unit->BlockedOnPathRequest();

    } else {
        AILOG_LOG(log, "Emptying steps.");

        m_step_index = 0;
        m_steps.Clear();
        AddStep(0, 0);
    }
}

int32_t GroundPath::GetMovementCost(UnitInfo* unit) {
    int32_t grid_x;
    int32_t grid_y;
    int32_t result;
    int32_t step_cost;

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    result = 0;

    for (uint32_t i = m_step_index; i < m_steps.GetCount(); ++i) {
        PathStep* step = m_steps[i];

        if (step->x || step->y) {
            grid_x += step->x;
            grid_y += step->y;

            step_cost =
                Access_IsAccessible(unit->GetUnitType(), unit->team, grid_x, grid_y, AccessModifier_NoModifiers);

            if (step->x && step->y) {
                step_cost = (step_cost * 3) / 2;
            }

            result += step_cost;
        }
    }

    return result;
}

bool GroundPath::Execute(UnitInfo* unit) {
    PathStep* step;
    int32_t angle;
    int32_t grid_x;
    int32_t grid_y;
    int32_t target_grid_x;
    int32_t target_grid_y;
    int32_t offset_x;
    int32_t offset_y;
    int32_t speed;
    int32_t cost;
    bool is_diagonal_step;

    unit->move_to_grid_x = m_end_x;
    unit->move_to_grid_y = m_end_y;

    for (step = m_steps[m_step_index]; step->x == 0 && step->y == 0; step = m_steps[m_step_index]) {
        ++m_step_index;

        if (m_step_index >= m_steps.GetCount()) {
            unit->BlockedOnPathRequest();

            return false;
        }
    }

    angle = Paths_GetAngle(step->x, step->y);

    offset_x = DIRECTION_OFFSETS[angle].x;
    offset_y = DIRECTION_OFFSETS[angle].y;

    speed = unit->speed;

    if (unit->group_speed > 0) {
        speed = unit->group_speed - 1;
    }

    if (speed == 0 || unit->engine == 0) {
        if (unit->engine) {
            unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
            unit->MoveFinished();

        } else {
            unit->ClearUnitList();
            unit->BlockedOnPathRequest();
        }

        return false;
    }

    if (Paths_UpdateAngle(unit, angle)) {
        return false;
    }

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    target_grid_x = grid_x + offset_x;
    target_grid_y = grid_y + offset_y;

    if (target_grid_x < 0 || target_grid_x >= ResourceManager_MapSize.x || target_grid_y < 0 ||
        target_grid_y >= ResourceManager_MapSize.y) {
        unit->BlockedOnPathRequest();
        return false;
    }

    cost = 4;

    if (unit->GetOrder() == ORDER_MOVE_TO_UNIT) {
        SmartPointer<UnitInfo> receiver(Access_GetReceiverUnit(unit, target_grid_x, target_grid_y));

        if (receiver != nullptr) {
            const int32_t stored_units = Access_GetStoredUnitCount(&*receiver);
            const int32_t storable_units = receiver->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            SDL_assert(stored_units <= storable_units && receiver->storage == stored_units);

            if (stored_units >= storable_units) {
                unit->BlockedOnPathRequest();

            } else {
                unit->SetOrder(ORDER_IDLE);

                if (receiver->GetUnitType() == SEATRANS || receiver->GetUnitType() == CLNTRANS) {
                    unit->SetOrderState(ORDER_STATE_PREPARE_STORE);

                } else {
                    unit->SetOrderState(ORDER_STATE_STORE);
                }

                unit->SetParent(&*receiver);

                unit->path = nullptr;

                UnitsManager_ScaleUnit(unit, ORDER_STATE_SHRINK);

                if (GameManager_SelectedUnit == unit) {
                    GameManager_MenuUnitSelect(&*receiver);
                }
            }

            return false;
        }
    }

    Paths_IsOccupied(target_grid_x, target_grid_y, unit->angle, unit->team);

    cost = Access_IsAccessible(
        unit->GetUnitType(), unit->team, target_grid_x, target_grid_y,
        AccessModifier_IgnoreVisibility | AccessModifier_MovesUnderBridge | AccessModifier_SameClassBlocks);

    if (cost) {
        if (offset_x && offset_y) {
            is_diagonal_step = true;

        } else {
            is_diagonal_step = false;
        }

        if (Paths_CalculateStep(unit, cost, is_diagonal_step)) {
            if ((unit->flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
                int32_t image_index;

                int32_t surface_type = Access_GetModifiedSurfaceType(target_grid_x, target_grid_y);

                if (unit->GetUnitType() == CLNTRANS) {
                    image_index = 0;

                    if (surface_type == SURFACE_TYPE_WATER) {
                        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                            if (unit->team != team && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
                                if (UnitsManager_TeamInfo[team]
                                        .heat_map_stealth_sea[target_grid_y * ResourceManager_MapSize.x +
                                                              target_grid_x]) {
                                    image_index = 8;
                                    break;

                                } else if (unit->IsDetectedByTeam(team) &&
                                           UnitsManager_TeamInfo[team]
                                               .heat_map_complete[target_grid_y * ResourceManager_MapSize.x +
                                                                  target_grid_x]) {
                                    image_index = 8;
                                    break;
                                }
                            }
                        }

                    } else {
                        image_index = 8;
                    }

                } else {
                    if (surface_type == SURFACE_TYPE_WATER) {
                        image_index = 8;

                    } else {
                        image_index = 0;
                    }
                }

                if (image_index != unit->image_base) {
                    if (unit->GetUnitType() == CLNTRANS) {
                        unit->firing_image_base = image_index;

                    } else {
                        unit->firing_image_base = image_index + 16;
                    }

                    unit->UpdateSpriteFrame(image_index, unit->image_index_max);
                }
            }

            unit->MoveInTransitUnitInMapHash(target_grid_x, target_grid_y);
            unit->moved = 0;
            unit->SetOrderState(ORDER_STATE_IN_TRANSITION);

            if (GameManager_SelectedUnit == unit) {
                GameManager_UpdateInfoDisplay(unit);
            }

            return true;

        } else {
            unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
            unit->MoveFinished();

            return false;
        }

    } else {
        if (!Access_SetUnitDestination(unit->grid_x, unit->grid_y, target_grid_x, target_grid_y,
                                       !(unit->flags & MOBILE_SEA_UNIT))) {
            SmartPointer<UnitPath> path(this);

            if (unit->GetOrder() == ORDER_BUILD ||
                UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_ELIMINATED ||
                (GameManager_GameState == GAME_STATE_9_END_TURN && GameManager_TurnTimerValue == 0)) {
                unit->BlockedOnPathRequest();

            } else if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_REMOTE ||
                       Paths_RequestPath(unit, AccessModifier_SameClassBlocks)) {
                unit->SetOrderState(ORDER_STATE_NEW_ORDER);
            }
        }

        return false;
    }
}

void GroundPath::UpdateUnitAngle(UnitInfo* unit) {
    if (m_step_index + 1 < m_steps.GetCount()) {
        unit->UpdateAngle(Paths_GetAngle(m_steps[m_step_index + 1]->x, m_steps[m_step_index + 1]->y));
    }
}

void GroundPath::Draw(UnitInfo* unit, WindowInfo* window) {
    int32_t limited_speed;
    int32_t unit_speed;
    int32_t base_speed;
    int32_t grid_x;
    int32_t grid_y;
    int32_t scaled_grid_x;
    int32_t scaled_grid_y;
    int32_t pixel_grid_x;
    int32_t pixel_grid_y;
    uint32_t steps_count;
    int32_t path_x = 0;
    int32_t path_y = 0;
    int32_t cost = 0;
    int32_t angle;
    int32_t color;
    int32_t shots;

    limited_speed = unit->speed;

    unit_speed = unit->speed * 4 + unit->move_fraction;

    if (unit->group_speed) {
        limited_speed = unit->group_speed - 1;
    }

    limited_speed = limited_speed * 4 + unit->move_fraction;

    base_speed = unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED) * 4;

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    pixel_grid_x = grid_x * 64 + 32;
    pixel_grid_y = grid_y * 64 + 32;

    steps_count = m_steps->GetCount();

    for (uint32_t i = m_step_index; i <= steps_count; ++i) {
        if (i != steps_count) {
            path_x = m_steps[i]->x;
            path_y = m_steps[i]->y;

            if (limited_speed < 0) {
                limited_speed += base_speed;
            }

            grid_x += path_x;
            grid_y += path_y;

            cost = Access_IsAccessible(unit->GetUnitType(), unit->team, grid_x, grid_y, AccessModifier_NoModifiers);

            if (path_x && path_y) {
                cost = (cost * 3) / 2;
            }

            limited_speed -= cost;

        } else {
            limited_speed = -1;
        }

        if (pixel_grid_x < GameManager_MapWindowDrawBounds.lrx && pixel_grid_x > GameManager_MapWindowDrawBounds.ulx &&
            pixel_grid_y < GameManager_MapWindowDrawBounds.lry && pixel_grid_y > GameManager_MapWindowDrawBounds.uly) {
            if (path_x || path_y) {
                scaled_grid_x = (pixel_grid_x << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
                scaled_grid_y = (pixel_grid_y << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

                if (path_x == 0 && path_y < 0) {
                    angle = 0;

                } else if (path_x > 0 && path_y < 0) {
                    angle = 1;

                } else if (path_x > 0 && path_y == 0) {
                    angle = 2;

                } else if (path_x > 0 && path_y > 0) {
                    angle = 3;

                } else if (path_x == 0 && path_y > 0) {
                    angle = 4;

                } else if (path_x < 0 && path_y > 0) {
                    angle = 5;

                } else if (path_x < 0 && path_y == 0) {
                    angle = 6;

                } else {
                    angle = 7;
                }

                if (limited_speed < 0) {
                    color = COLOR_GREEN;

                } else {
                    color = COLOR_BLUE;
                }

                Paths_DrawMarker(window, angle, scaled_grid_x, scaled_grid_y, color);

                if (i > m_step_index && unit->shots > 0 && unit_speed >= 0) {
                    if (unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                        shots = unit->shots;

                    } else {
                        shots = (unit_speed * unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS)) /
                                (unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED) * 4);

                        shots = std::min(shots, static_cast<int32_t>(unit->shots));
                    }

                    Paths_DrawShots(window, scaled_grid_x, scaled_grid_y, shots);
                }
            }
        }

        unit_speed -= cost;

        pixel_grid_x += path_x * 64;
        pixel_grid_y += path_y * 64;
    }
}

bool GroundPath::IsEndStep() const { return m_step_index + 1 == m_steps.GetCount(); }

void GroundPath::WritePacket(NetPacket& packet) {
    uint16_t steps_count = m_steps.GetCount();

    packet << steps_count;

    for (int32_t i = 0; i < steps_count; ++i) {
        packet << *m_steps[i];
    }
}

void GroundPath::ReadPacket(NetPacket& packet, int32_t steps_count) {
    PathStep step;

    for (int32_t i = 0; i < steps_count; ++i) {
        packet >> step;
        m_steps.PushBack(&step);
    }
}

void GroundPath::AppendLinearSteps(int32_t distance_x_, int32_t distance_y_) {
    int32_t step_x;
    int32_t step_y;
    PathStep step;

    while (distance_x_ || distance_y_) {
        if (distance_x_ > 0) {
            step_x = 1;

        } else if (distance_x_ == 0) {
            step_x = 0;

        } else {
            step_x = -1;
        }

        step.x = step_x;

        if (distance_y_ > 0) {
            step_y = 1;

        } else if (distance_y_ == 0) {
            step_y = 0;

        } else {
            step_y = -1;
        }

        step.y = step_y;

        m_steps.PushBack(&step);

        distance_x_ -= step_x;
        distance_y_ -= step_y;
    }
}

void GroundPath::AddStep(int32_t step_x, int32_t step_y) {
    PathStep step;

    step.x = step_x;
    step.y = step_y;

    SDL_assert(step.x >= -1 && step.x <= 1 && step.y >= -1 && step.y <= 1);

    m_steps.PushBack(&step);
}

SmartObjectArray<PathStep> GroundPath::GetSteps() { return m_steps; }

uint32_t GroundPath::GetStepIndex() const { return m_step_index; }
