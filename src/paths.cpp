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
#include "ailog.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "message_manager.hpp"
#include "net_packet.hpp"
#include "paths_manager.hpp"
#include "registerarray.hpp"
#include "remote.hpp"
#include "sound_manager.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

static int32_t Paths_GetAngle(int32_t x, int32_t y);
static void Paths_DrawMissile(UnitInfo* unit, int32_t position_x, int32_t position_y);
static bool Paths_LoadUnit(UnitInfo* unit);
static void Paths_FinishMove(UnitInfo* unit);
static void Paths_TakeStep(UnitInfo* unit, int32_t cost);
static bool Paths_CalculateStep(UnitInfo* unit, int32_t cost, bool is_diagonal_step);

SmartObjectArray<Point> Paths_SiteReservations;

static uint16_t Paths_AirPath_TypeIndex;
static RegisterClass Paths_AirPath_ClassRegister("AirPath", &Paths_AirPath_TypeIndex, &AirPath::Allocate);

static uint16_t Paths_GroundPath_TypeIndex;
static RegisterClass Paths_GroundPath_ClassRegister("GroundPath", &Paths_GroundPath_TypeIndex, &GroundPath::Allocate);

static uint16_t Paths_BuilderPath_TypeIndex;
static RegisterClass Paths_BuilderPath_ClassRegister("BuilderPath", &Paths_BuilderPath_TypeIndex,
                                                     &BuilderPath::Allocate);

const Point Paths_8DirPointsArray[8] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

const int16_t Paths_8DirPointsArrayX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
const int16_t Paths_8DirPointsArrayY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

const Point Paths_8DirPointsArrayMarkerA[8] = {{0, -10}, {8, -8}, {10, 0},  {8, 8},
                                               {0, 10},  {-8, 8}, {-10, 0}, {-8, -8}};

const Point Paths_8DirPointsArrayMarkerB[8] = {{-10, 10}, {-12, 0}, {-10, -10}, {0, -12},
                                               {10, -10}, {12, 0},  {10, 10},   {0, 12}};

const Point Paths_8DirPointsArrayMarkerC[8] = {{10, 10},   {0, 12},  {-10, 10}, {-12, 0},
                                               {-10, -10}, {0, -12}, {10, -10}, {12, 0}};

uint32_t Paths_DebugMode;

uint32_t Paths_EvaluatedTileCount;
uint32_t Paths_EvaluatorCallCount;
uint32_t Paths_SquareAdditionsCount;
uint32_t Paths_SquareInsertionsCount;
uint32_t Paths_EvaluatedSquareCount;
uint32_t Paths_MaxDepth;

UnitPath::UnitPath() : x_end(0), y_end(0), distance_x(0), distance_y(0), euclidean_distance(0) {}

UnitPath::UnitPath(int32_t target_x, int32_t target_y)
    : x_end(target_x), y_end(target_y), distance_x(0), distance_y(0), euclidean_distance(0) {}

UnitPath::UnitPath(int32_t distance_x, int32_t distance_y, int32_t euclidean_distance, int32_t target_x,
                   int32_t target_y)
    : x_end(target_x),
      y_end(target_y),
      distance_x(distance_x),
      distance_y(distance_y),
      euclidean_distance(euclidean_distance) {}

UnitPath::~UnitPath() {}

Point UnitPath::GetPosition(UnitInfo* unit) const { return Point(unit->grid_x, unit->grid_y); }

bool UnitPath::IsInPath(int32_t grid_x, int32_t grid_y) const { return false; }

void UnitPath::CancelMovement(UnitInfo* unit) {}

void UnitPath::UpdateUnitAngle(UnitInfo* unit) {}

bool UnitPath::IsEndStep() const { return false; }

void UnitPath::WritePacket(NetPacket& packet) {}

void UnitPath::ReadPacket(NetPacket& packet, int32_t steps_count) {}

void UnitPath::Path_vfunc17(int32_t distance_x, int32_t distance_y) {}

int16_t UnitPath::GetEndX() const { return x_end; }

int16_t UnitPath::GetEndY() const { return y_end; }

int32_t UnitPath::GetDistanceX() const { return distance_x; }

int32_t UnitPath::GetDistanceY() const { return distance_y; }

int16_t UnitPath::GetEuclideanDistance() const { return euclidean_distance; }

void UnitPath::SetEndXY(int32_t target_x, int32_t target_y) {
    x_end = target_x;
    y_end = target_y;
}

AirPath::AirPath()
    : length(0), angle(0), pixel_x_start(0), pixel_y_start(0), x_step(0), y_step(0), delta_x(0), delta_y(0) {}

AirPath::AirPath(UnitInfo* unit, int32_t distance_x, int32_t distance_y, int32_t euclidean_distance, int32_t target_x,
                 int32_t target_y)
    : UnitPath(distance_x, distance_y, euclidean_distance, target_x, target_y),
      length(euclidean_distance),
      pixel_x_start(unit->x),
      pixel_y_start(unit->y),
      x_step(0),
      y_step(0),
      delta_x((distance_x << 22) / euclidean_distance),
      delta_y((distance_y << 22) / euclidean_distance) {
    if (unit->flags & MOBILE_AIR_UNIT) {
        angle = UnitsManager_GetTargetAngle(distance_x, distance_y);

    } else {
        angle = unit->angle;
    }
}

AirPath::~AirPath() {}

FileObject* AirPath::Allocate() noexcept { return new (std::nothrow) AirPath(); }

uint16_t AirPath::GetTypeIndex() const { return Paths_AirPath_TypeIndex; }

void AirPath::FileLoad(SmartFileReader& file) noexcept {
    file.Read(length);
    file.Read(angle);
    file.Read(pixel_x_start);
    file.Read(pixel_y_start);
    file.Read(x_end);
    file.Read(y_end);
    file.Read(x_step);
    file.Read(y_step);
    file.Read(delta_x);
    file.Read(delta_y);
}

void AirPath::FileSave(SmartFileWriter& file) noexcept {
    file.Write(length);
    file.Write(angle);
    file.Write(pixel_x_start);
    file.Write(pixel_y_start);
    file.Write(x_end);
    file.Write(y_end);
    file.Write(x_step);
    file.Write(y_step);
    file.Write(delta_x);
    file.Write(delta_y);
}

Point AirPath::GetPosition(UnitInfo* unit) const {
    int32_t pixel_x;
    int32_t pixel_y;
    Point point(unit->grid_x, unit->grid_y);

    if (unit->flags & MOBILE_AIR_UNIT) {
        pixel_x = length * delta_x + x_step;
        pixel_x >>= 16;
        pixel_x += pixel_x_start;
        pixel_x >>= 6;
        point.x = pixel_x;

        pixel_y = length * delta_y + y_step;
        pixel_y >>= 16;
        pixel_y += pixel_y_start;
        pixel_y >>= 6;
        point.y = pixel_y;
    }

    return point;
}

void AirPath::CancelMovement(UnitInfo* unit) {
    AiLog log("Airpath: emergency stop.");

    if (length) {
        if (unit->angle == angle) {
            if (delta_x > 0) {
                x_end = (unit->x + 32) / 64;

            } else {
                x_end = (unit->x) / 64;
            }

            if (delta_y > 0) {
                y_end = (unit->y + 32) / 64;

            } else {
                y_end = (unit->y) / 64;
            }

            pixel_x_start = unit->x;
            pixel_y_start = unit->y;

            x_step = 0;
            y_step = 0;

            length %= unit->max_velocity;

            if (length == 0) {
                ++length;
                ++unit->speed;
            }

            delta_x = x_end * 64 - unit->x + 32;
            delta_y = y_end * 64 - unit->y + 32;

            delta_x = (delta_x << 16) / length;
            delta_y = (delta_y << 16) / length;

            log.Log("Recalculated path to [%i,%i], length %i.", x_end + 1, y_end + 1, length);

        } else {
            log.Log("Haven't finished turning.");

            unit->Redraw();

            unit->path = nullptr;

            unit->BlockedOnPathRequest();
        }

    } else {
        log.Log("Length is zero.");
    }
}

int32_t AirPath::GetMovementCost(UnitInfo* unit) { return (length / unit->max_velocity) * 4; }

bool AirPath::Execute(UnitInfo* unit) {
    bool team_visibility;
    SmartPointer<UnitInfo> target_unit;

    do {
        team_visibility = unit->IsVisibleToTeam(GameManager_PlayerTeam);

        if (team_visibility) {
            unit->RefreshScreen();
        }

        if (unit->flags & MOBILE_AIR_UNIT) {
            int32_t speed = unit->speed;

            if (unit->group_speed > 0) {
                speed = unit->group_speed - 1;
            }

            if (speed == 0 || unit->engine == 0) {
                unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);
                unit->MoveFinished();

                return false;
            }
        }

        if (!Paths_UpdateAngle(unit, angle)) {
            if (unit->flags & MOBILE_AIR_UNIT) {
                if (length == 1) {
                    int32_t dx = labs(delta_x >> 16) >> 1;
                    int32_t dy = labs(delta_y >> 16) >> 1;

                    if (dx || dy) {
                        ++length;
                    }

                    delta_x >>= 1;
                    delta_y >>= 1;

                } else if (GameManager_SelectedUnit == unit && unit->GetSfxType() != SFX_TYPE_DRIVE) {
                    SoundManager_PlaySfx(unit, SFX_TYPE_DRIVE);
                }
            }

            x_step += delta_x;
            y_step += delta_y;

            int32_t position_x = (x_step >> 16) + pixel_x_start;
            int32_t position_y = (y_step >> 16) + pixel_y_start;
            int32_t offset_x = position_x - unit->x;
            int32_t offset_y = position_y - unit->y;
            int32_t grid_x = (position_x >> 6) - unit->grid_x;
            int32_t grid_y = (position_y >> 6) - unit->grid_y;

            if (grid_x || grid_y) {
                if ((unit->flags & MISSILE_UNIT) && team_visibility) {
                    Paths_DrawMissile(unit, position_x, position_y);

                } else {
                    target_unit = unit->MakeCopy();
                }

                Hash_MapHash.Remove(unit);
            }

            unit->OffsetDrawZones(offset_x, offset_y);
            unit->grid_x = unit->x >> 6;
            unit->grid_y = unit->y >> 6;

            --length;

            if (grid_x || grid_y) {
                Hash_MapHash.Add(unit);
            }

            if (unit->flags & MISSILE_UNIT) {
                if (grid_x || grid_y) {
                    Access_UpdateMapStatus(unit, true);

                    if (unit->GetUnitType() == ALNMISSL || unit->GetUnitType() == ALNTBALL ||
                        unit->GetUnitType() == ALNABALL) {
                        unit->DrawSpriteFrame(unit->GetImageIndex() ^ 1);
                    }
                }

            } else {
                if (grid_x || grid_y) {
                    Access_UpdateMapStatus(unit, true);
                    Access_UpdateMapStatus(&*target_unit, false);

                    if (GameManager_SelectedUnit == unit && length == 1) {
                        SoundManager_PlaySfx(unit, SFX_TYPE_STOP);
                    }
                }

                unit->FollowUnit();

                if ((length % unit->max_velocity) == 0) {
                    Paths_TakeStep(unit, 1);

                    if (GameManager_SelectedUnit == unit) {
                        GameManager_UpdateInfoDisplay(unit);
                    }
                }
            }

            if (length == 0) {
                Paths_FinishMove(unit);
            }

            if (unit->IsVisibleToTeam(GameManager_PlayerTeam)) {
                unit->RefreshScreen();
                team_visibility = true;
            }
        }

    } while (unit->hits && unit->GetOrderState() == ORDER_STATE_IN_PROGRESS && !Remote_IsNetworkGame &&
             !team_visibility);

    return false;
}

void AirPath::Path_vfunc12(int32_t unknown) {}

void AirPath::Draw(UnitInfo* unit, WindowInfo* window) {
    int32_t steps;
    int32_t grid_x;
    int32_t grid_y;
    int32_t step_x;
    int32_t step_y;
    int32_t max_speed;
    int32_t base_speed;
    int32_t scaled_grid_x;
    int32_t scaled_grid_y;
    int32_t color;

    steps = length / unit->max_velocity;

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    grid_x = (grid_x << 6) + 32;
    grid_y = (grid_y << 6) + 32;

    step_x = x_step;
    step_y = y_step;

    max_speed = unit->speed;

    if (unit->group_speed) {
        max_speed = unit->group_speed - 1;
    }

    base_speed = unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED);

    for (int32_t i = 0; i <= steps; ++i) {
        if (grid_x < GameManager_MapWindowDrawBounds.lrx && grid_x > GameManager_MapWindowDrawBounds.ulx &&
            grid_y < GameManager_MapWindowDrawBounds.lry && grid_y > GameManager_MapWindowDrawBounds.uly) {
            scaled_grid_x = (grid_x << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
            scaled_grid_y = (grid_y << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

            if (max_speed) {
                color = COLOR_BLUE;

            } else {
                color = COLOR_GREEN;
            }

            Paths_DrawMarker(window, angle, scaled_grid_x, scaled_grid_y, color);
        }

        if (max_speed == 0) {
            max_speed = base_speed;
        }

        --max_speed;

        step_x += unit->max_velocity * delta_x;
        step_y += unit->max_velocity * delta_y;

        grid_x = (step_x >> 16) + pixel_x_start;
        grid_y = (step_y >> 16) + pixel_y_start;
    }
}

GroundPath::GroundPath() : index(0) {}

GroundPath::GroundPath(int32_t target_x, int32_t target_y) : UnitPath(target_x, target_y), index(0) {}

GroundPath::~GroundPath() {}

FileObject* GroundPath::Allocate() noexcept { return new (std::nothrow) GroundPath(); }

uint16_t GroundPath::GetTypeIndex() const { return Paths_GroundPath_TypeIndex; }

void GroundPath::FileLoad(SmartFileReader& file) noexcept {
    int32_t count;
    PathStep step;

    file.Read(x_end);
    file.Read(y_end);
    file.Read(index);

    count = file.ReadObjectCount();

    steps.Clear();

    for (int32_t i = 0; i < count; ++i) {
        file.Read(step);
        steps.PushBack(&step);
    }
}

void GroundPath::FileSave(SmartFileWriter& file) noexcept {
    int32_t count;
    PathStep step;

    file.Write(x_end);
    file.Write(y_end);
    file.Write(index);

    count = steps.GetCount();

    file.WriteObjectCount(count);

    for (int32_t i = 0; i < count; ++i) {
        file.Write(*steps[i]);
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

        for (int32_t i = index; i < steps->GetCount(); ++i) {
            step = steps[i];

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

    for (int32_t i = index; i < steps.GetCount(); ++i) {
        if (point.x == grid_x && point.y == grid_y) {
            return true;
        }

        point.x += steps[i]->x;
        point.y += steps[i]->y;
    }

    return point.x == grid_x && point.y == grid_y;
}

void GroundPath::CancelMovement(UnitInfo* unit) {
    AiLog log("Ground path: emergency stop.");

    if (unit->GetOrderState() == ORDER_STATE_IN_PROGRESS) {
        log.Log("In turn state.");

        unit->BlockedOnPathRequest();

    } else {
        log.Log("Emptying steps.");

        index = 0;
        steps.Clear();
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

    for (int32_t i = index; i < steps.GetCount(); ++i) {
        PathStep* step = steps[i];

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

    unit->target_grid_x = x_end;
    unit->target_grid_y = y_end;

    for (step = steps[index]; step->x == 0 && step->y == 0; step = steps[index]) {
        ++index;

        if (index >= steps.GetCount()) {
            unit->BlockedOnPathRequest();

            return false;
        }
    }

    angle = Paths_GetAngle(step->x, step->y);

    offset_x = Paths_8DirPointsArrayX[angle];
    offset_y = Paths_8DirPointsArrayY[angle];

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
    if (index + 1 < steps.GetCount()) {
        unit->UpdateAngle(Paths_GetAngle(steps[index + 1]->x, steps[index + 1]->y));
    }
}

void GroundPath::Path_vfunc12(int32_t unknown) {}

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
    int32_t steps_count;
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

    steps_count = steps->GetCount();

    for (int32_t i = index; i <= steps_count; ++i) {
        if (i != steps_count) {
            path_x = steps[i]->x;
            path_y = steps[i]->y;

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

                if (i > index && unit->shots > 0 && unit_speed >= 0) {
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

bool GroundPath::IsEndStep() const { return index + 1 == steps.GetCount(); }

void GroundPath::WritePacket(NetPacket& packet) {
    uint16_t steps_count = steps.GetCount();

    packet << steps_count;

    for (int32_t i = 0; i < steps_count; ++i) {
        packet << *steps[i];
    }
}

void GroundPath::ReadPacket(NetPacket& packet, int32_t steps_count) {
    PathStep step;

    for (int32_t i = 0; i < steps_count; ++i) {
        packet >> step;
        steps.PushBack(&step);
    }
}

void GroundPath::Path_vfunc17(int32_t distance_x_, int32_t distance_y_) {
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

        steps.PushBack(&step);

        distance_x_ -= step_x;
        distance_y_ -= step_y;
    }
}

void GroundPath::AddStep(int32_t step_x, int32_t step_y) {
    PathStep step;

    step.x = step_x;
    step.y = step_y;

    SDL_assert(step.x >= -1 && step.x <= 1 && step.y >= -1 && step.y <= 1);

    steps.PushBack(&step);
}

SmartObjectArray<PathStep> GroundPath::GetSteps() { return steps; }

uint16_t GroundPath::GetPathStepIndex() const { return index; }

bool Paths_RequestPath(UnitInfo* unit, int32_t mode) {
    bool result;

    if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK) {
        UnitInfo* enemy = unit->GetEnemy();

        if (enemy) {
            unit->target_grid_x = enemy->grid_x;
            unit->target_grid_y = enemy->grid_y;
        }
    }

    if (unit->target_grid_x >= 0 && unit->target_grid_x < ResourceManager_MapSize.x && unit->target_grid_y >= 0 &&
        unit->target_grid_y < ResourceManager_MapSize.y) {
        SmartPointer<PathRequest> request(new (std::nothrow)
                                              PathRequest(unit, mode, Point(unit->target_grid_x, unit->target_grid_y)));

        request->SetBoardTransport(unit->GetOrder() == ORDER_MOVE_TO_UNIT);

        if (mode & 1) {
            request->SetMinimumDistance(3);
        }

        if (unit->GetOrder() == ORDER_MOVE_TO_ATTACK) {
            int32_t range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

            request->SetMinimumDistance(range * range);
        }

        if (unit->GetOrderState() == ORDER_STATE_NEW_ORDER) {
            PathsManager_RemoveRequest(unit);
        }

        if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
            PathsManager_PushBack(*request);

        } else {
            PathsManager_PushFront(*request);
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
        end_x = unit->target_grid_x;
        end_y = unit->target_grid_y;
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

BuilderPath::BuilderPath() : UnitPath(0, 0), x(1), y(1) {}

BuilderPath::~BuilderPath() {}

FileObject* BuilderPath::Allocate() noexcept { return new (std::nothrow) BuilderPath(); }

uint16_t BuilderPath::GetTypeIndex() const { return Paths_BuilderPath_TypeIndex; }

void BuilderPath::FileLoad(SmartFileReader& file) noexcept {
    file.Read(x);
    file.Read(y);
}

void BuilderPath::FileSave(SmartFileWriter& file) noexcept {
    file.Write(x);
    file.Write(y);
}

int32_t BuilderPath::GetMovementCost(UnitInfo* unit) { return SHRT_MAX; }

bool BuilderPath::Execute(UnitInfo* unit) {
    bool result;
    int32_t direction;

    if (ini_get_setting(INI_EFFECTS)) {
        unit->RefreshScreen();
    }

    if (Remote_IsNetworkGame || unit->IsVisibleToTeam(GameManager_PlayerTeam)) {
        x = Paths_8DirPointsArrayX[(unit->angle + 2) & 7];
        y = Paths_8DirPointsArrayY[(unit->angle + 2) & 7];

        direction = Paths_GetAngle(x, y);

        Paths_UpdateAngle(unit, direction);

        /// \todo Is this a defect? Paths_UpdateAngle() returns the correct status
        result = false;

    } else {
        result = false;
    }

    return result;
}

void BuilderPath::Path_vfunc12(int32_t unknown) {}

void BuilderPath::Draw(UnitInfo* unit, WindowInfo* window) {}

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

void Paths_DrawMissile(UnitInfo* unit, int32_t position_x, int32_t position_y) {
    if (unit->GetUnitType() == TORPEDO || unit->GetUnitType() == ROCKET) {
        int32_t index;
        int32_t team;
        int32_t grid_x;
        int32_t grid_y;
        int32_t delta_x;
        int32_t delta_y;
        int32_t scaled_x;
        int32_t scaled_y;
        int32_t offset_x;
        int32_t offset_y;
        ResourceID unit_type;

        index = 3;
        team = unit->team;
        grid_x = unit->grid_x;
        grid_y = unit->grid_y;
        delta_x = 0;
        delta_y = 0;
        scaled_x = ((position_x - unit->x) << 16) / index;
        scaled_y = ((position_y - unit->y) << 16) / index;

        if (unit->GetUnitType() == TORPEDO) {
            unit_type = TRPBUBLE;

        } else {
            unit_type = RKTSMOKE;
        }

        for (int32_t i = 0; i < index; ++i) {
            SmartPointer<UnitInfo> particle =
                UnitsManager_DeployUnit(unit_type, team, nullptr, grid_x, grid_y, 0, true);

            offset_x = (unit->x + (delta_x >> 16)) - particle->x;
            offset_y = (unit->y + (delta_y >> 16)) - particle->y;

            particle->OffsetDrawZones(offset_x, offset_y);
            particle->RefreshScreen();

            delta_x += scaled_x;
            delta_y += scaled_y;
        }
    }
}

bool Paths_LoadUnit(UnitInfo* unit) {
    UnitInfo* shop;
    bool result;

    shop = Access_GetReceiverUnit(unit, unit->grid_x, unit->grid_y);

    if (shop) {
        if (shop->GetUnitType() == LANDPAD && !Access_GetUnit2(unit->grid_x, unit->grid_y, unit->team)) {
            unit->SetParent(nullptr);
            unit->SetOrder(ORDER_LAND);
            unit->SetOrderState(ORDER_STATE_INIT);

            result = true;

        } else if (shop->GetUnitType() == HANGAR && unit->GetOrder() == ORDER_MOVE_TO_UNIT) {
            const int32_t stored_units = Access_GetStoredUnitCount(shop);
            const int32_t storable_units = shop->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            SDL_assert(stored_units <= storable_units && shop->storage == stored_units);

            if (stored_units == storable_units) {
                unit->SetOrder(ORDER_AWAIT);
                unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

                if (GameManager_SelectedUnit == unit) {
                    MessageManager_DrawMessage(_(7c8d), 1, unit, Point(unit->grid_x, unit->grid_y));
                }

            } else {
                unit->SetParent(shop);
                unit->SetOrder(ORDER_LAND);
                unit->SetOrderState(ORDER_STATE_INIT);

                if (GameManager_SelectedUnit == unit) {
                    GameManager_MenuUnitSelect(shop);
                }
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

void Paths_FinishMove(UnitInfo* unit) {
    int32_t grid_x = unit->grid_x;
    int32_t grid_y = unit->grid_y;

    GameManager_RenderMinimapDisplay = true;

    if (unit->flags & MISSILE_UNIT) {
        SmartPointer<UnitInfo> parent = unit->GetParent();

        parent->Attack(grid_x, grid_y);

        UnitsManager_DestroyUnit(unit);

    } else {
        unit->Redraw();

        if (unit->path->GetEndX() == grid_x && unit->path->GetEndY() == grid_y) {
            if (unit->path != nullptr) {
                unit->path = nullptr;
            }

            if (!Paths_LoadUnit(unit)) {
                if (unit->GetUnitType() == AIRTRANS && unit->GetParent()) {
                    if (unit->storage < unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                        if (unit->GetParent() == Access_GetTeamUnit(grid_x, grid_y, unit->team, MOBILE_LAND_UNIT)) {
                            unit->SetOrder(ORDER_LOAD);
                            unit->SetOrderState(ORDER_STATE_INIT);

                            return;
                        }

                    } else {
                        unit->SetParent(nullptr);
                    }
                }

                unit->BlockedOnPathRequest();
            }

        } else {
            unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

            unit->path = Paths_GetAirPath(unit);

            unit->MoveFinished();
        }
    }
}

void Paths_TakeStep(UnitInfo* unit, int32_t cost) {
    if (GameManager_RealTime) {
        cost = 0;
    }

    unit->speed -= cost;

    if (unit->group_speed) {
        int32_t group_speed = unit->group_speed - cost;

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
        Paths_TakeStep(unit, normalized_cost);

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
    const int32_t position{Paths_SiteReservations->Find(&site)};

    SDL_assert(position != -1);

    if (position != -1) {
        Paths_SiteReservations.Remove(position);
    }
}

void Paths_ClearSiteReservations() noexcept { Paths_SiteReservations.Clear(); }

[[nodiscard]] bool Paths_IsSiteReserved(const Point site) noexcept { return Paths_SiteReservations->Find(&site) != -1; }
