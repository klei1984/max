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

#include "paths.hpp"

#include <cmath>

#include "access.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "message_manager.hpp"
#include "paths_manager.hpp"
#include "resource_manager.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

SmartObjectArray<Point> Paths_SiteReservations;

static constexpr Point Paths_8DirPointsArrayMarkerA[DIRECTION_COUNT] = {{0, -10}, {8, -8}, {10, 0},  {8, 8},
                                                                        {0, 10},  {-8, 8}, {-10, 0}, {-8, -8}};

static constexpr Point Paths_8DirPointsArrayMarkerB[DIRECTION_COUNT] = {{-10, 10}, {-12, 0}, {-10, -10}, {0, -12},
                                                                        {10, -10}, {12, 0},  {10, 10},   {0, 12}};

static constexpr Point Paths_8DirPointsArrayMarkerC[DIRECTION_COUNT] = {{10, 10},   {0, 12},  {-10, 10}, {-12, 0},
                                                                        {-10, -10}, {0, -12}, {10, -10}, {12, 0}};

bool Paths_RequestPath(UnitInfo* unit, int32_t mode) {
    bool result;

    if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK) {
        UnitInfo* enemy = unit->GetEnemy();

        if (enemy) {
            unit->move_to_grid_x = enemy->grid_x;
            unit->move_to_grid_y = enemy->grid_y;
        }
    }

    if (unit->move_to_grid_x >= 0 && unit->move_to_grid_x < ResourceManager_MapSize.x && unit->move_to_grid_y >= 0 &&
        unit->move_to_grid_y < ResourceManager_MapSize.y) {
        SmartPointer<PathRequest> request(
            new (std::nothrow) PathRequest(unit, mode, Point(unit->move_to_grid_x, unit->move_to_grid_y)));

        request->SetBoardTransport(unit->GetOrder() == ORDER_MOVE_TO_UNIT);

        if (mode & 1) {
            request->SetMinimumDistance(3);
        }

        if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK) {
            int32_t range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

            request->SetMinimumDistance(range * range);
        }

        if (unit->GetOrderState() == ORDER_STATE_NEW_ORDER) {
            ResourceManager_GetPathsManager().RemoveRequest(unit);
        }

        if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
            ResourceManager_GetPathsManager().PushBack(*request);

        } else {
            ResourceManager_GetPathsManager().PushFront(*request);
        }

        result = true;

    } else {
        unit->BlockedOnPathRequest(false);

        result = false;
    }

    return result;
}

AirPath* Paths_GetAirPath(UnitInfo* unit) {
    AirPath* result;

    int32_t grid_x;
    int32_t grid_y;
    int32_t end_x;
    int32_t end_y;
    int32_t distance_x;
    int32_t distance_y;
    int32_t distance;
    int32_t max_distance;
    int32_t steps_distance;
    int32_t target_grid_x;
    int32_t target_grid_y;

    unit->Redraw();

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    if (unit->path != nullptr) {
        end_x = unit->path->GetEndX();
        end_y = unit->path->GetEndY();

        unit->path = nullptr;

    } else {
        end_x = unit->move_to_grid_x;
        end_y = unit->move_to_grid_y;
    }

    distance_x = end_x - grid_x;
    distance_y = end_y - grid_y;

    distance = sqrt(distance_x * distance_x + distance_y * distance_y) * 4.0 + 0.5;

    if (distance) {
        distance -= unit->move_fraction;

        if (distance < 2) {
            distance = 2;
        }

        if (GameManager_RealTime) {
            max_distance = distance;

        } else {
            max_distance = unit->speed;

            if (unit->group_speed) {
                max_distance = unit->group_speed - 1;
            }

            if (max_distance) {
                max_distance *= 4;

            } else {
                max_distance = SHRT_MAX;
            }
        }

        if (max_distance < distance) {
            target_grid_x = (distance_x * max_distance) / distance + grid_x;
            target_grid_y = (distance_y * max_distance) / distance + grid_y;

            distance_x = target_grid_x - grid_x;
            distance_y = target_grid_y - grid_y;

            distance = sqrt(distance_x * distance_x + distance_y * distance_y) * 4.0 + 0.5;

            if (distance > max_distance) {
                distance = max_distance;
            }

            steps_distance = max_distance / 4;

            unit->move_fraction = max_distance - distance;

            if (unit->move_fraction > 3) {
                unit->move_fraction = 3;
            }

            if (steps_distance == 0 || distance == 0) {
                unit->speed = 0;

                if (unit->group_speed > 1) {
                    unit->group_speed = 1;
                }

                distance_x = end_x - grid_x;
                distance_y = end_y - grid_y;

                distance = sqrt(distance_x * distance_x + distance_y * distance_y) * 4.0 + 0.5;

                steps_distance = (distance + 3) / 4;
            }

        } else {
            steps_distance = (distance + 3) / 4;

            if (max_distance != SHRT_MAX) {
                unit->move_fraction = distance % 4;
            }
        }

        if (GameManager_FastMovement) {
            unit->max_velocity = 2;

        } else {
            unit->max_velocity = 4;
        }

        result =
            new (std::nothrow) AirPath(unit, distance_x, distance_y, steps_distance * unit->max_velocity, end_x, end_y);

    } else {
        result = nullptr;
    }

    return result;
}

int32_t Paths_GetAngle(int32_t x, int32_t y) {
    int32_t result;

    if (x) {
        if (x <= 0) {
            if (y <= 0) {
                if (y) {
                    result = 7;

                } else {
                    result = 6;
                }

            } else {
                result = 5;
            }

        } else if (y >= 0) {
            if (y) {
                result = 3;

            } else {
                result = 2;
            }

        } else {
            result = 1;
        }

    } else if (y >= 0) {
        result = 4;

    } else {
        result = 0;
    }

    return result;
}

bool Paths_UpdateAngle(UnitInfo* unit, int32_t angle) {
    bool result;

    if (unit->angle != angle) {
        int32_t delta_a = angle - unit->angle;
        int32_t delta_b = unit->angle - angle;
        int32_t direction;

        if (delta_a < 0) {
            delta_a += 8;
        }

        if (delta_b < 0) {
            delta_b += 8;
        }

        if (delta_a >= delta_b) {
            direction = -1;

        } else {
            direction = 1;
        }

        unit->UpdateAngle((unit->angle + direction) & 7);

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool Paths_CalculateStep(UnitInfo* unit, int32_t cost, bool is_diagonal_step) {
    int32_t max_velocity;
    int32_t normalized_cost;
    bool result;

    if (GameManager_FastMovement) {
        max_velocity = 16;

    } else {
        max_velocity = 8;
    }

    unit->max_velocity = max_velocity;

    if (unit->GetUnitType() == COMMANDO || unit->GetUnitType() == INFANTRY) {
        unit->max_velocity >>= 1;
        unit->velocity = unit->max_velocity;
    }

    if (cost > 4) {
        unit->max_velocity /= 2;

    } else if (cost < 4) {
        unit->max_velocity *= 2;
    }

    if (is_diagonal_step) {
        cost = (cost * 3) / 2;
    }

    if (unit->move_fraction > cost) {
        unit->move_fraction = cost;
    }

    cost -= unit->move_fraction;

    normalized_cost = (cost + 3) / 4;

    if (normalized_cost > unit->speed) {
        unit->move_fraction = unit->speed * 4;
        unit->speed = 0;

        if (unit->group_speed != 0) {
            unit->group_speed = 1;
        }

        result = false;

    } else {
        if (GameManager_RealTime) {
            normalized_cost = 0;
        }

        unit->speed -= normalized_cost;

        if (unit->group_speed) {
            int32_t group_speed = unit->group_speed - normalized_cost;

            if (group_speed < 1) {
                group_speed = 1;
            }

            unit->group_speed = group_speed;
        }

        auto base_values = unit->GetBaseValues();

        if (!base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
            int32_t shots =
                (base_values->GetAttribute(ATTRIB_ROUNDS) * unit->speed) / base_values->GetAttribute(ATTRIB_SPEED);

            if (shots < unit->shots) {
                unit->shots = shots;
            }
        }

        unit->move_fraction = (normalized_cost * 4) - cost;

        result = true;
    }

    return result;
}

void Paths_DrawMarker(WindowInfo* window, int32_t angle, int32_t grid_x, int32_t grid_y, int32_t color) {
    int32_t a_x;
    int32_t a_y;
    int32_t b_x;
    int32_t b_y;
    int32_t c_x;
    int32_t c_y;

    a_x = (Paths_8DirPointsArrayMarkerA[angle].x << 16) / Gfx_MapScalingFactor + grid_x;
    a_y = (Paths_8DirPointsArrayMarkerA[angle].y << 16) / Gfx_MapScalingFactor + grid_y;
    b_x = (Paths_8DirPointsArrayMarkerB[angle].x << 16) / Gfx_MapScalingFactor + grid_x;
    b_y = (Paths_8DirPointsArrayMarkerB[angle].y << 16) / Gfx_MapScalingFactor + grid_y;
    c_x = (Paths_8DirPointsArrayMarkerC[angle].x << 16) / Gfx_MapScalingFactor + grid_x;
    c_y = (Paths_8DirPointsArrayMarkerC[angle].y << 16) / Gfx_MapScalingFactor + grid_y;

    a_x = std::min(std::max(0, a_x), WindowManager_MapWidth - 1);
    a_y = std::min(std::max(0, a_y), WindowManager_MapHeight - 1);
    b_x = std::min(std::max(0, b_x), WindowManager_MapWidth - 1);
    b_y = std::min(std::max(0, b_y), WindowManager_MapHeight - 1);
    c_x = std::min(std::max(0, c_x), WindowManager_MapWidth - 1);
    c_y = std::min(std::max(0, c_y), WindowManager_MapHeight - 1);

    draw_line(window->buffer, window->width, a_x, a_y, b_x, b_y, color);
    draw_line(window->buffer, window->width, b_x, b_y, c_x, c_y, color);
    draw_line(window->buffer, window->width, c_x, c_y, a_x, a_y, color);
}

void Paths_DrawShots(WindowInfo* window, int32_t grid_x, int32_t grid_y, int32_t shots) {
    if (shots && GameManager_DisplayButtonStatus) {
        int32_t size;

        size = 0x400000 / Gfx_MapScalingFactor - 2;

        if (size >= 0) {
            struct ImageSimpleHeader* image;

            image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(IL_SHOTS));

            if (image->height <= size) {
                shots = std::min(size / image->width, shots);

                if (shots > 0) {
                    grid_y += (size / 2) - image->height;

                    if (grid_y >= 0) {
                        if (image->width + grid_y < WindowManager_MapHeight) {
                            grid_x += (size / 2) - (image->width * shots);

                            for (int32_t i = 0; i < shots; ++i) {
                                if (grid_x >= 0 && (image->width + grid_x) < WindowManager_MapWidth) {
                                    WindowManager_DecodeSimpleImage(image, grid_x, grid_y, true, window);
                                }

                                grid_x += image->width;
                            }
                        }
                    }
                }
            }
        }
    }
}

bool Paths_IsOccupied(int32_t grid_x, int32_t grid_y, int32_t angle, int32_t team) {
    const auto units = Hash_MapHash[Point(grid_x, grid_y)];

    if (units) {
        // the end node must be cached in case Hash_MapHash.Remove() deletes the list
        for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
            if (((*it).GetUnitType() == SUBMARNE || (*it).GetUnitType() == COMMANDO ||
                 (*it).GetUnitType() == CLNTRANS) &&
                !(*it).IsVisibleToTeam(team)) {
                if (!(*it).AttemptSideStep(grid_x, grid_y, angle)) {
                    (*it).SpotByTeam(team);
                    return true;
                }
            }
        }
    }

    return false;
}

void Paths_ReserveSite(const Point site) noexcept { Paths_SiteReservations.PushBack(&site); }

void Paths_RemoveSiteReservation(const Point site) noexcept {
    const int64_t position{Paths_SiteReservations->Find(&site)};

    SDL_assert(position != -1);

    if (position != -1) {
        Paths_SiteReservations.Remove(position);
    }
}

void Paths_ClearSiteReservations() noexcept { Paths_SiteReservations.Clear(); }

[[nodiscard]] bool Paths_IsSiteReserved(const Point site) noexcept { return Paths_SiteReservations->Find(&site) != -1; }
