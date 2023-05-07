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

static int Paths_GetAngle(int x, int y);
static void Paths_DrawMissile(UnitInfo* unit, int position_x, int position_y);
static bool Paths_LoadUnit(UnitInfo* unit);
static void Paths_FinishMove(UnitInfo* unit);
static void Paths_TakeStep(UnitInfo* unit, int cost, int param);
static bool Paths_CalculateStep(UnitInfo* unit, int cost, int param, bool is_diagonal_step);

static unsigned short Paths_AirPath_TypeIndex;
static MAXRegisterClass Paths_AirPath_ClassRegister("AirPath", &Paths_AirPath_TypeIndex, &AirPath::Allocate);

static unsigned short Paths_GroundPath_TypeIndex;
static MAXRegisterClass Paths_GroundPath_ClassRegister("GroundPath", &Paths_GroundPath_TypeIndex,
                                                       &GroundPath::Allocate);

static unsigned short Paths_BuilderPath_TypeIndex;
static MAXRegisterClass Paths_BuilderPath_ClassRegister("BuilderPath", &Paths_BuilderPath_TypeIndex,
                                                        &BuilderPath::Allocate);

const Point Paths_8DirPointsArray[8] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

const short Paths_8DirPointsArrayX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
const short Paths_8DirPointsArrayY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

const Point Paths_8DirPointsArrayMarkerA[8] = {{0, -10}, {8, -8}, {10, 0},  {8, 8},
                                               {0, 10},  {-8, 8}, {-10, 0}, {-8, -8}};

const Point Paths_8DirPointsArrayMarkerB[8] = {{-10, 10}, {-12, 0}, {-10, -10}, {0, -12},
                                               {10, -10}, {12, 0},  {10, 10},   {0, 12}};

const Point Paths_8DirPointsArrayMarkerC[8] = {{10, 10},   {0, 12},  {-10, 10}, {-12, 0},
                                               {-10, -10}, {0, -12}, {10, -10}, {12, 0}};

unsigned int Paths_LastTimeStamp;
bool Paths_TimeBenchmarkDisable;
unsigned int Paths_TimeLimit = TIMER_FPS_TO_MS(30 / 1.1);

unsigned int Paths_DebugMode;

unsigned int Paths_EvaluatedTileCount;
unsigned int Paths_EvaluatorCallCount;
unsigned int Paths_SquareAdditionsCount;
unsigned int Paths_SquareInsertionsCount;
unsigned int Paths_EvaluatedSquareCount;
unsigned int Paths_MaxDepth;

UnitPath::UnitPath() : x_end(0), y_end(0), distance_x(0), distance_y(0), euclidean_distance(0) {}

UnitPath::UnitPath(int target_x, int target_y)
    : x_end(target_x), y_end(target_y), distance_x(0), distance_y(0), euclidean_distance(0) {}

UnitPath::UnitPath(int distance_x, int distance_y, int euclidean_distance, int target_x, int target_y)
    : x_end(target_x),
      y_end(target_y),
      distance_x(distance_x),
      distance_y(distance_y),
      euclidean_distance(euclidean_distance) {}

UnitPath::~UnitPath() {}

Point UnitPath::GetPosition(UnitInfo* unit) const { return Point(unit->grid_x, unit->grid_y); }

bool UnitPath::IsInPath(int grid_x, int grid_y) const { return false; }

void UnitPath::CancelMoment(UnitInfo* unit) {}

void UnitPath::UpdateUnitAngle(UnitInfo* unit) {}

bool UnitPath::IsEndStep() const { return false; }

void UnitPath::WritePacket(NetPacket& packet) {}

void UnitPath::ReadPacket(NetPacket& packet, int steps_count) {}

void UnitPath::Path_vfunc17(int distance_x, int distance_y) {}

short UnitPath::GetEndX() const { return x_end; }

short UnitPath::GetEndY() const { return y_end; }

int UnitPath::GetDistanceX() const { return distance_x; }

int UnitPath::GetDistanceY() const { return distance_y; }

short UnitPath::GetEuclideanDistance() const { return euclidean_distance; }

void UnitPath::SetEndXY(int target_x, int target_y) {
    x_end = target_x;
    y_end = target_y;
}

AirPath::AirPath()
    : length(0), angle(0), pixel_x_start(0), pixel_y_start(0), x_step(0), y_step(0), delta_x(0), delta_y(0) {}

AirPath::AirPath(UnitInfo* unit, int distance_x, int distance_y, int euclidean_distance, int target_x, int target_y)
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

FileObject* AirPath::Allocate() { return new (std::nothrow) AirPath(); }

unsigned short AirPath::GetTypeIndex() const { return Paths_AirPath_TypeIndex; }

void AirPath::FileLoad(SmartFileReader& file) {
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

void AirPath::FileSave(SmartFileWriter& file) {
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
    int pixel_x;
    int pixel_y;
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

void AirPath::CancelMoment(UnitInfo* unit) {
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

int AirPath::GetMovementCost(UnitInfo* unit) { return (length / unit->max_velocity) * 4; }

bool AirPath::Path_vfunc10(UnitInfo* unit) {
    bool team_visibility;
    SmartPointer<UnitInfo> target_unit;

    do {
        team_visibility = unit->IsVisibleToTeam(GameManager_PlayerTeam);

        if (team_visibility) {
            unit->RefreshScreen();
        }

        if (unit->flags & MOBILE_AIR_UNIT) {
            int speed = unit->speed;

            if (unit->group_speed) {
                speed = unit->group_speed - 1;
            }

            if (!speed || !unit->engine) {
                unit->state = ORDER_STATE_1;
                unit->MoveFinished();

                return false;
            }
        }

        if (!Paths_UpdateAngle(unit, angle)) {
            if (unit->flags & MOBILE_AIR_UNIT) {
                if (length == 1) {
                    int dx = labs(delta_x >> 16) >> 1;
                    int dy = labs(delta_y >> 16) >> 1;

                    if (dx || dy) {
                        ++length;
                    }

                    delta_x >>= 1;
                    delta_y >>= 1;

                } else if (GameManager_SelectedUnit == unit && unit->sound != SFX_TYPE_DRIVE) {
                    SoundManager.PlaySfx(unit, SFX_TYPE_DRIVE);
                }
            }

            x_step += delta_x;
            y_step += delta_y;

            int position_x = (x_step >> 16) + pixel_x_start;
            int position_y = (y_step >> 16) + pixel_y_start;
            int offset_x = position_x - unit->x;
            int offset_y = position_y - unit->y;
            int grid_x = (position_x >> 6) - unit->grid_x;
            int grid_y = (position_y >> 6) - unit->grid_y;

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

                    if (unit->unit_type == ALNMISSL || unit->unit_type == ALNTBALL || unit->unit_type == ALNABALL) {
                        unit->DrawSpriteFrame(unit->GetImageIndex() ^ 1);
                    }
                }

            } else {
                if (grid_x || grid_y) {
                    Access_UpdateMapStatus(unit, true);
                    Access_UpdateMapStatus(&*target_unit, false);

                    if (GameManager_SelectedUnit == unit && length == 1) {
                        SoundManager.PlaySfx(unit, SFX_TYPE_STOP);
                    }
                }

                unit->FollowUnit();

                if ((length % unit->max_velocity) == 0) {
                    Paths_TakeStep(unit, 1, 2);

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

    } while (unit->hits && unit->state == ORDER_STATE_5 && !Remote_IsNetworkGame && !team_visibility);

    return false;
}

void AirPath::Path_vfunc12(int unknown) {}

void AirPath::Draw(UnitInfo* unit, WindowInfo* window) {
    int steps;
    int grid_x;
    int grid_y;
    int step_x;
    int step_y;
    int max_speed;
    int base_speed;
    int scaled_grid_x;
    int scaled_grid_y;
    int color;

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

    for (int i = 0; i <= steps; ++i) {
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

GroundPath::GroundPath(int target_x, int target_y) : UnitPath(target_x, target_y), index(0) {}

GroundPath::~GroundPath() {}

FileObject* GroundPath::Allocate() { return new (std::nothrow) GroundPath(); }

unsigned short GroundPath::GetTypeIndex() const { return Paths_GroundPath_TypeIndex; }

void GroundPath::FileLoad(SmartFileReader& file) {
    int count;
    PathStep step;

    file.Read(x_end);
    file.Read(y_end);
    file.Read(index);

    count = file.ReadObjectCount();

    steps.Clear();

    for (int i = 0; i < count; ++i) {
        file.Read(step);
        steps.PushBack(&step);
    }
}

void GroundPath::FileSave(SmartFileWriter& file) {
    int count;
    PathStep step;

    file.Write(x_end);
    file.Write(y_end);
    file.Write(index);

    count = steps.GetCount();

    file.WriteObjectCount(count);

    for (int i = 0; i < count; ++i) {
        file.Write(*steps[i]);
    }
}

Point GroundPath::GetPosition(UnitInfo* unit) const {
    Point position(unit->grid_x, unit->grid_y);
    int speed = unit->speed;

    if (unit->orders != ORDER_BUILD && unit->state == ORDER_STATE_1 && speed > 0) {
        int move_fraction;
        Point point;
        PathStep* step;
        int step_cost;

        move_fraction = unit->move_fraction;

        for (int i = index; i < steps->GetCount(); ++i) {
            step = steps[i];

            point.x = position.x + step->x;
            point.y = position.y + step->y;

            if (point.x >= 0 && point.x < ResourceManager_MapSize.x && point.y >= 0 &&
                point.y < ResourceManager_MapSize.y) {
                step_cost = Access_IsAccessible(unit->unit_type, unit->team, point.x, point.y, 0);

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

bool GroundPath::IsInPath(int grid_x, int grid_y) const {
    Point point(0, 0);

    for (int i = index; i < steps.GetCount(); ++i) {
        if (point.x == grid_x && point.y == grid_y) {
            return true;
        }

        point.x += steps[i]->x;
        point.y += steps[i]->y;
    }

    return point.x == grid_x && point.y == grid_y;
}

void GroundPath::CancelMoment(UnitInfo* unit) {
    AiLog log("Ground path: emergency stop.");

    if (unit->state == ORDER_STATE_5) {
        log.Log("In turn state.");

        unit->BlockedOnPathRequest();

    } else {
        log.Log("Emptying steps.");

        index = 0;
        steps.Clear();
        AddStep(0, 0);
    }
}

int GroundPath::GetMovementCost(UnitInfo* unit) {
    int grid_x;
    int grid_y;
    int result;
    int step_cost;

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    result = 0;

    for (int i = index; i < steps.GetCount(); ++i) {
        PathStep* step = steps[i];

        if (step->x || step->y) {
            grid_x += step->x;
            grid_y += step->y;

            step_cost = Access_IsAccessible(unit->unit_type, unit->team, grid_x, grid_y, 0);

            if (step->x && step->y) {
                step_cost = (step_cost * 3) / 2;
            }

            result += step_cost;
        }
    }

    return result;
}

bool GroundPath::Path_vfunc10(UnitInfo* unit) {
    PathStep* step;
    int angle;
    int grid_x;
    int grid_y;
    int target_grid_x;
    int target_grid_y;
    int offset_x;
    int offset_y;
    int speed;
    int cost;
    bool is_diagonal_step;

    unit->target_grid_x = x_end;
    unit->target_grid_y = y_end;

    for (step = steps[index]; step->x == 0 && step->y == 0; step = steps[index]) {
        ++index;

        if (index >= steps.GetCount()) {
            SmartPointer<UnitPath> path(this);

            /// \todo Is this smart pointer really required?
            /// In case unit would hold the last reference to this path object, then
            /// postponing the destruction of the object till return from this virtual
            /// function would not make any difference as BlockedOnPathRequest() is
            /// the last function call before return.

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
            unit->state = ORDER_STATE_1;
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

    cost = 4;

    if (unit->orders == ORDER_MOVE_TO_UNIT) {
        SmartPointer<UnitInfo> receiver(Access_GetReceiverUnit(unit, target_grid_x, target_grid_y));

        if (receiver != nullptr) {
            if (receiver->GetBaseValues()->GetAttribute(ATTRIB_STORAGE) == Access_GetStoredUnitCount(&*receiver)) {
                unit->BlockedOnPathRequest();

            } else {
                unit->orders = ORDER_IDLE;

                if (receiver->unit_type == SEATRANS || receiver->unit_type == CLNTRANS) {
                    unit->state = ORDER_STATE_4;

                } else {
                    unit->state = ORDER_STATE_3;
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

    cost = Access_IsAccessible(unit->unit_type, unit->team, target_grid_x, target_grid_y, 0x1A);

    if (cost) {
        if (offset_x && offset_y) {
            is_diagonal_step = true;

        } else {
            is_diagonal_step = false;
        }

        if (Paths_CalculateStep(unit, cost, 2, is_diagonal_step)) {
            if ((unit->flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
                int image_index;

                int surface_type = Access_GetModifiedSurfaceType(target_grid_x, target_grid_y);

                if (unit->unit_type == CLNTRANS) {
                    image_index = 0;

                    if (surface_type == SURFACE_TYPE_WATER) {
                        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
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
                    if (unit->unit_type == CLNTRANS) {
                        unit->firing_image_base = image_index;

                    } else {
                        unit->firing_image_base = image_index + 16;
                    }

                    unit->UpdateSpriteFrame(image_index, unit->image_index_max);
                }
            }

            unit->MoveInTransitUnitInMapHash(target_grid_x, target_grid_y);
            unit->moved = 0;
            unit->state = ORDER_STATE_6;

            if (GameManager_SelectedUnit == unit) {
                GameManager_UpdateInfoDisplay(unit);
            }

            return true;

        } else {
            unit->state = ORDER_STATE_1;
            unit->MoveFinished();

            return false;
        }

    } else {
        if (!Access_SetUnitDestination(unit->grid_x, unit->grid_y, target_grid_x, target_grid_y, false)) {
            SmartPointer<UnitPath> path(this);

            if (unit->orders == ORDER_BUILD || UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_ELIMINATED ||
                (GameManager_GameState == GAME_STATE_9_END_TURN && GameManager_TurnTimerValue == 0)) {
                unit->BlockedOnPathRequest();

            } else if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_REMOTE || Paths_RequestPath(unit, 2)) {
                unit->state = ORDER_STATE_NEW_ORDER;
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

void GroundPath::Path_vfunc12(int unknown) {}

void GroundPath::Draw(UnitInfo* unit, WindowInfo* window) {
    int limited_speed;
    int unit_speed;
    int base_speed;
    int grid_x;
    int grid_y;
    int scaled_grid_x;
    int scaled_grid_y;
    int pixel_grid_x;
    int pixel_grid_y;
    int steps_count;
    int path_x = 0;
    int path_y = 0;
    int cost = 0;
    int angle;
    int color;
    int shots;

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

    for (int i = index; i <= steps_count; ++i) {
        if (i != steps_count) {
            path_x = steps[i]->x;
            path_y = steps[i]->y;

            if (limited_speed < 0) {
                limited_speed += base_speed;
            }

            grid_x += path_x;
            grid_y += path_y;

            cost = Access_IsAccessible(unit->unit_type, unit->team, grid_x, grid_y, 0x00);

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

                if (i > index && unit->shots != 0 && unit_speed >= 0) {
                    if (unit->GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
                        shots = unit->shots;

                    } else {
                        shots = (unit_speed * unit->GetBaseValues()->GetAttribute(ATTRIB_ROUNDS)) /
                                (unit->GetBaseValues()->GetAttribute(ATTRIB_SPEED) * 4);

                        shots = std::min(shots, static_cast<int>(unit->shots));
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
    unsigned short steps_count = steps.GetCount();

    packet << steps_count;

    for (int i = 0; i < steps_count; ++i) {
        packet << *steps[i];
    }
}

void GroundPath::ReadPacket(NetPacket& packet, int steps_count) {
    PathStep step;

    for (int i = 0; i < steps_count; ++i) {
        packet >> step;
        steps.PushBack(&step);
    }
}

void GroundPath::Path_vfunc17(int distance_x_, int distance_y_) {
    int step_x;
    int step_y;
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

void GroundPath::AddStep(int step_x, int step_y) {
    PathStep step;

    step.x = step_x;
    step.y = step_y;

    SDL_assert(step.x >= -1 && step.x <= 1 && step.y >= -1 && step.y <= 1);

    steps.PushBack(&step);
}

SmartObjectArray<PathStep> GroundPath::GetSteps() { return steps; }

unsigned short GroundPath::GetPathStepIndex() const { return index; }

bool Paths_RequestPath(UnitInfo* unit, int mode) {
    bool result;

    if (unit->orders == ORDER_MOVE_TO_ATTACK) {
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

        request->SetBoardTransport(unit->orders == ORDER_MOVE_TO_UNIT);

        if (mode & 1) {
            request->SetMinimumDistance(3);
        }

        if (unit->orders == ORDER_MOVE_TO_ATTACK) {
            int range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);

            request->SetMinimumDistance(range * range);
        }

        if (unit->state == ORDER_STATE_NEW_ORDER) {
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

        if (Remote_IsNetworkGame) {
            Remote_SendNetPacket_41(unit);
        }

        result = false;
    }

    return result;
}

AirPath* Paths_GetAirPath(UnitInfo* unit) {
    AirPath* result;

    int grid_x;
    int grid_y;
    int end_x;
    int end_y;
    int distance_x;
    int distance_y;
    int distance;
    int max_distance;
    int steps_distance;
    int target_grid_x;
    int target_grid_y;

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

FileObject* BuilderPath::Allocate() { return new (std::nothrow) BuilderPath(); }

unsigned short BuilderPath::GetTypeIndex() const { return Paths_BuilderPath_TypeIndex; }

void BuilderPath::FileLoad(SmartFileReader& file) {
    file.Read(x);
    file.Read(y);
}

void BuilderPath::FileSave(SmartFileWriter& file) {
    file.Write(x);
    file.Write(y);
}

int BuilderPath::GetMovementCost(UnitInfo* unit) { return SHRT_MAX; }

bool BuilderPath::Path_vfunc10(UnitInfo* unit) {
    bool result;
    int direction;

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

void BuilderPath::Path_vfunc12(int unknown) {}

void BuilderPath::Draw(UnitInfo* unit, WindowInfo* window) {}

int Paths_GetAngle(int x, int y) {
    int result;

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

bool Paths_UpdateAngle(UnitInfo* unit, int angle) {
    bool result;

    if (unit->angle != angle) {
        int delta_a = angle - unit->angle;
        int delta_b = unit->angle - angle;
        int direction;

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

void Paths_DrawMissile(UnitInfo* unit, int position_x, int position_y) {
    if (unit->unit_type == TORPEDO || unit->unit_type == ROCKET) {
        int index;
        int team;
        int grid_x;
        int grid_y;
        int delta_x;
        int delta_y;
        int scaled_x;
        int scaled_y;
        int offset_x;
        int offset_y;
        ResourceID unit_type;

        index = 3;
        team = unit->team;
        grid_x = unit->grid_x;
        grid_y = unit->grid_y;
        delta_x = 0;
        delta_y = 0;
        scaled_x = ((position_x - unit->x) << 16) / index;
        scaled_y = ((position_y - unit->y) << 16) / index;

        if (unit->unit_type == TORPEDO) {
            unit_type = TRPBUBLE;

        } else {
            unit_type = RKTSMOKE;
        }

        for (int i = 0; i < index; ++i) {
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
        if (shop->unit_type == LANDPAD && !Access_GetUnit2(unit->grid_x, unit->grid_y, unit->team)) {
            unit->SetParent(nullptr);
            unit->orders = ORDER_LAND;
            unit->state = ORDER_STATE_0;

            result = true;

        } else if (shop->unit_type == HANGAR && unit->orders == ORDER_MOVE_TO_UNIT) {
            if (Access_GetStoredUnitCount(shop) == shop->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                unit->orders = ORDER_AWAIT;
                unit->state = ORDER_STATE_1;

                if (GameManager_SelectedUnit == unit) {
                    MessageManager_DrawMessage(_(7c8d), 1, unit, Point(unit->grid_x, unit->grid_y));
                }

            } else {
                unit->SetParent(shop);
                unit->orders = ORDER_LAND;
                unit->state = ORDER_STATE_0;

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
    int grid_x;
    int grid_y;

    GameManager_RenderMinimapDisplay = true;

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    if (unit->flags & MISSILE_UNIT) {
        SmartPointer<UnitInfo> missile = unit->GetParent();

        missile->Attack(grid_x, grid_y);

        UnitsManager_DestroyUnit(unit);

    } else {
        unit->Redraw();

        if (unit->path->GetEndX() == grid_x && unit->path->GetEndY() == grid_y) {
            if (unit->path != nullptr) {
                unit->path = nullptr;
            }

            if (!Paths_LoadUnit(unit)) {
                if (unit->unit_type == AIRTRANS && unit->GetParent()) {
                    if (unit->storage < unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                        if (unit->GetParent() == Access_GetUnit4(grid_x, grid_y, unit->team, MOBILE_LAND_UNIT)) {
                            unit->orders = ORDER_LOAD;
                            unit->state = ORDER_STATE_0;

                            return;
                        }

                    } else {
                        unit->SetParent(nullptr);
                    }
                }

                unit->BlockedOnPathRequest();
            }

        } else {
            unit->state = ORDER_STATE_1;

            unit->path = Paths_GetAirPath(unit);

            unit->MoveFinished();
        }
    }
}

void Paths_TakeStep(UnitInfo* unit, int cost, int param) {
    int group_speed;
    UnitValues* base_values;

    if (GameManager_RealTime) {
        param = 0;
        cost = 0;
    }

    param = 0;

    unit->speed -= cost;

    if (unit->group_speed) {
        group_speed = unit->group_speed - cost;

        if (group_speed < 1) {
            group_speed = 1;
        }

        unit->group_speed = group_speed;
    }

    base_values = unit->GetBaseValues();

    if (!base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
        int shots;

        shots = (base_values->GetAttribute(ATTRIB_ROUNDS) * unit->speed) / base_values->GetAttribute(ATTRIB_SPEED);

        if (shots < unit->shots) {
            unit->shots = shots;
        }
    }
}

bool Paths_CalculateStep(UnitInfo* unit, int cost, int param, bool is_diagonal_step) {
    int max_velocity;
    int normalized_cost;
    bool result;

    if (GameManager_FastMovement) {
        max_velocity = 16;

    } else {
        max_velocity = 8;
    }

    unit->max_velocity = max_velocity;

    if (unit->unit_type == COMMANDO || unit->unit_type == INFANTRY) {
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
        param = (param * 3) / 2;
    }

    param = 0;

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
        Paths_TakeStep(unit, normalized_cost, param);

        unit->move_fraction = (normalized_cost * 4) - cost;

        result = true;
    }

    return result;
}

void Paths_DrawMarker(WindowInfo* window, int angle, int grid_x, int grid_y, int color) {
    int a_x;
    int a_y;
    int b_x;
    int b_y;
    int c_x;
    int c_y;

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

void Paths_DrawShots(WindowInfo* window, int grid_x, int grid_y, int shots) {
    if (shots && GameManager_DisplayButtonStatus) {
        int size;

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

                            for (int i = 0; i < shots; ++i) {
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

bool Paths_IsOccupied(int grid_x, int grid_y, int angle, int team) {
    for (SmartList<UnitInfo>::Iterator it = Hash_MapHash[Point(grid_x, grid_y)]; it != nullptr; ++it) {
        if (((*it).unit_type == SUBMARNE || (*it).unit_type == COMMANDO || (*it).unit_type == CLNTRANS) &&
            !(*it).IsVisibleToTeam(team)) {
            if (!(*it).AttemptSideStep(grid_x, grid_y, angle)) {
                (*it).SpotByTeam(team);
                return true;
            }
        }
    }

    return false;
}

bool Paths_HaveTimeToThink() { return (timer_get() - Paths_LastTimeStamp) <= Paths_TimeLimit; }
