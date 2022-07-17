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

#include "access.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "message_manager.hpp"
#include "registerarray.hpp"
#include "remote.hpp"
#include "sound_manager.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"

static int Paths_GetAngle(int x, int y);
static void Paths_DrawMissile(UnitInfo* unit, int position_x, int position_y);
static bool Paths_LoadUnit(UnitInfo* unit);
static void Paths_FinishMove(UnitInfo* unit);
static void Paths_TakeStep(UnitInfo* unit, int cost, int param);

static unsigned short Paths_AirPath_TypeIndex;
static MAXRegisterClass Paths_AirPath_ClassRegister("AirPath", &Paths_AirPath_TypeIndex, &AirPath::Allocate);

static unsigned short Paths_GroundPath_TypeIndex;
static MAXRegisterClass Paths_GroundPath_ClassRegister("GroundPath", &Paths_GroundPath_TypeIndex,
                                                       &GroundPath::Allocate);

static unsigned short Paths_BuilderPath_TypeIndex;
static MAXRegisterClass Paths_BuilderPath_ClassRegister("BuilderPath", &Paths_BuilderPath_TypeIndex,
                                                        &BuilderPath::Allocate);

Point Paths_8DirPointsArray[8] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

short Paths_8DirPointsArrayX[8] = {0, 1, 1, 1, 0, -1, -1, -1};
short Paths_8DirPointsArrayY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

unsigned int Paths_LastTimeStamp;
bool Path_EnableTimeBenchmark;
unsigned int Paths_TimeLimit = 43750;

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

void UnitPath::Path_vfunc8(UnitInfo* unit) {}

void UnitPath::UpdateUnitAngle(UnitInfo* unit) {}

bool UnitPath::IsEndStep() const { return false; }

int UnitPath::WritePacket(char* buffer) { return 0; }

void UnitPath::Path_vfunc16(int unknown1, int unknown2) {}

void UnitPath::Path_vfunc17(int unknown1, int unknown2) {}

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
      delta_x(distance_x << 22 / euclidean_distance),
      delta_y(distance_y << 22 / euclidean_distance) {
    if (unit->flags & MOBILE_AIR_UNIT) {
        /// \todo Implement missing stuff
        // angle = UnitsManager_GetTargetAngle(distance_x, distance_y);
    } else {
        angle = unit->angle;
    }
}

AirPath::~AirPath() {}

TextFileObject* AirPath::Allocate() { return new (std::nothrow) AirPath(); }

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

void AirPath::TextLoad(TextStructure& object) {
    length = object.ReadInt("length");
    angle = object.ReadInt("angle");
    pixel_x_start = object.ReadInt("pixel_x_start");
    pixel_y_start = object.ReadInt("pixel_y_start");
    x_end = object.ReadInt("x_end");
    y_end = object.ReadInt("y_end");
    x_step = object.ReadInt("x_step");
    y_step = object.ReadInt("y_step");
    delta_x = object.ReadInt("delta_x");
    delta_y = object.ReadInt("delta_y");
}

void AirPath::TextSave(SmartTextfileWriter& file) {
    file.WriteInt("length", length);
    file.WriteInt("angle", angle);
    file.WriteInt("pixel_x_start", pixel_x_start);
    file.WriteInt("pixel_y_start", pixel_y_start);
    file.WriteInt("x_end", x_end);
    file.WriteInt("y_end", y_end);
    file.WriteInt("x_step", x_step);
    file.WriteInt("y_step", y_step);
    file.WriteInt("delta_x", delta_x);
    file.WriteInt("delta_y", delta_y);
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

void AirPath::Path_vfunc8(UnitInfo* unit) {
    if (length) {
        if (unit->angle == angle) {
            if (delta_x > 0) {
                x_end = (unit->x + 32) / 64;
            } else {
                x_end = (unit->x) / 64;
            }

            if (delta_y > 0) {
                y_end = (unit->x + 32) / 64;
            } else {
                y_end = (unit->x) / 64;
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
        } else {
            /// \todo Implement missing stuff
            // unit->unitinfo_sub_F48BA();
            unit->path = nullptr;
            // unit->unitinfo_sub_EF017();
        }
    }
}

int AirPath::GetMovementCost(UnitInfo* unit) { return (length / unit->max_velocity) * 4; }

bool AirPath::Path_vfunc10(UnitInfo* unit) {
    bool team_visibility;
    SmartPointer<UnitInfo> target_unit;

    /// \todo Implement missing stuff
    /*
        do {
            team_visibility = unit->IsVisibleToTeam(GUI_PlayerTeamIndex);

            if (team_visibility) {
                unit->RefreshScreen();
            }

            if (unit->flags & MOBILE_AIR_UNIT) {
                int speed = unit->speed;

                if (unit->field_79) {
                    speed = unit->field_79 - 1;
                }

                if (!speed || !unit->engine) {
                    unit->state = 1;
                    unit->unitinfo_sub_EEEA8();

                    return false;
                }
            }

            if (!unit->UpdateAngle(angle)) {
                if (unit->flags & MOBILE_AIR_UNIT) {
                    if (length == 1) {
                        int dx = labs(delta_x >> 16) >> 1;
                        int dy = labs(delta_y >> 16) >> 1;

                        if (dx | dy) {
                            ++length;
                        }

                        delta_x >>= 1;
                        delta_y >>= 1;

                    } else if (unit == selected_unit && unit->sound != SFX_TYPE_DRIVE) {
                        soundmgr.PlaySfx(unit, SFX_TYPE_DRIVE);
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

                if (grid_x | grid_y) {
                    if ((unit->flags & MISSILE_UNIT) && team_visibility) {
                        AirPath_sub_B8457(unit, position_x, position_y);
                    } else {
                        target_unit = unit->MakeCopy();
                    }

                    Hash_MapHash.Remove(unit);
                }

                unit->OffsetDrawZones(offset_x, offset_y);
                unit->grid_x = unit->x >> 6;
                unit->grid_y = unit->y >> 6;

                --length;

                if (grid_x | grid_y) {
                    Hash_MapHash.Add(unit);
                }

                if (unit->flags & MISSILE_UNIT) {
                    if (grid_x | grid_y) {
                        unit->unitinfo_sub_11838(true);

                        if (unit->unit_type == ALNMISSL || unit->unit_type == ALNTBALL || unit->unit_type == ALNABALL) {
                            unit->unitinfo_draw_unit(unit->GetImageIndex() ^ 1);
                        }
                    }
                } else {
                    if (grid_x | grid_y) {
                        unit->unitinfo_sub_11838(true);
                        target_unit->unitinfo_sub_11838(false);

                        if (unit == selected_unit && length == 1) {
                            soundmgr.PlaySfx(unit, SFX_TYPE_STOP);
                        }
                    }

                    unit->unitinfo_sub_EF12B();

                    if ((length % unit->max_velocity) == 0) {
                        unit->UnitInfo_sub_B890D(1, 2);

                        if (unit == selected_unit) {
                            unit->UnitInfo_sub_A094D();
                        }
                    }
                }

                if (length == 0) {
                    AirPath_sub_B8758(unit);
                }

                if (unit->IsVisibleToTeam(GUI_PlayerTeamIndex)) {
                    unit->RefreshScreen();
                    team_visibility = true;
                }
            }

        } while (unit->hits && unit->state == 5 && dword_175624 == 0 && !team_visibility);
    */
    return false;
}

int AirPath::Path_vfunc12(int unknown) {}

bool AirPath::Draw(UnitInfo* unit, WindowInfo* window) {
    /// \todo Implement method
}

GroundPath::GroundPath() : index(0) {}

GroundPath::GroundPath(int target_x, int target_y) : UnitPath(target_x, target_y), index(0) {}

GroundPath::~GroundPath() {}

TextFileObject* GroundPath::Allocate() { return new (std::nothrow) GroundPath(); }

unsigned short GroundPath::GetTypeIndex() const { return Paths_GroundPath_TypeIndex; }

void GroundPath::FileLoad(SmartFileReader& file) {}

void GroundPath::FileSave(SmartFileWriter& file) {}

void GroundPath::TextLoad(TextStructure& object) {}

void GroundPath::TextSave(SmartTextfileWriter& file) {}

Point GroundPath::GetPosition(UnitInfo* unit) const {}

bool GroundPath::IsInPath(int grid_x, int grid_y) const {}

void GroundPath::Path_vfunc8(UnitInfo* unit) {
    if (unit->state == ORDER_STATE_5) {
        unit->BlockedOnPathRequest();

    } else {
        index = 0;
        steps.Clear();
        AddStep(0, 0);
    }
}

int GroundPath::GetMovementCost(UnitInfo* unit) {}

bool GroundPath::Path_vfunc10(UnitInfo* unit) {}

void GroundPath::UpdateUnitAngle(UnitInfo* unit) {}

int GroundPath::Path_vfunc12(int unknown) {}

bool GroundPath::Draw(UnitInfo* unit, WindowInfo* window) {}

bool GroundPath::IsEndStep() const {}

int GroundPath::WritePacket(char* buffer) {}

void GroundPath::Path_vfunc16(int unknown1, int unknown2) {}

void GroundPath::Path_vfunc17(int unknown1, int unknown2) {}

void GroundPath::AddStep(int step_x, int step_y) {
    PathStep step;

    step.x = step_x;
    step.y = step_y;

    steps.PushBack(&step);
}

bool Paths_RequestPath(UnitInfo* unit, int mode) {
    /// \todo
}

AirPath* Paths_GetAirPath(UnitInfo* unit) {
    /// \todo
    return nullptr;
}

BuilderPath::BuilderPath() : UnitPath(0, 0), x(1), y(1) {}

BuilderPath::~BuilderPath() {}

TextFileObject* BuilderPath::Allocate() { return new (std::nothrow) BuilderPath(); }

unsigned short BuilderPath::GetTypeIndex() const { return Paths_BuilderPath_TypeIndex; }

void BuilderPath::FileLoad(SmartFileReader& file) {
    file.Read(x);
    file.Read(y);
}

void BuilderPath::FileSave(SmartFileWriter& file) {
    file.Write(x);
    file.Write(y);
}

void BuilderPath::TextLoad(TextStructure& object) {
    x = object.ReadInt("x");
    y = object.ReadInt("y");
}

void BuilderPath::TextSave(SmartTextfileWriter& file) {
    file.WriteInt("x", x);
    file.WriteInt("y", y);
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

int BuilderPath::Path_vfunc12(int unknown) {}

bool BuilderPath::Draw(UnitInfo* unit, WindowInfo* window) {}

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
            unit->orders = ORDER_LANDING;
            unit->state = ORDER_STATE_0;

            result = true;

        } else if (shop->unit_type == HANGAR && unit->orders == ORDER_MOVING_27) {
            if (Access_GetStoredUnitCount(shop) == shop->GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                unit->orders = ORDER_AWAITING;
                unit->state = ORDER_STATE_1;

                if (GameManager_SelectedUnit == unit) {
                    MessageManager_DrawMessage("Unable to move unit into holding area.", 1, unit,
                                               Point(unit->grid_x, unit->grid_y));
                }

            } else {
                unit->SetParent(shop);
                unit->orders = ORDER_LANDING;
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
                            unit->orders = ORDER_LOADING;
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

            Paths_GetAirPath(unit);

            unit->path = nullptr;

            unit->MoveFinished();
        }
    }
}

void Paths_TakeStep(UnitInfo* unit, int cost, int param) {}

void Paths_DrawMarker(WindowInfo* window, int angle, int grid_x, int grid_y, int color) {}

void Paths_DrawShots(WindowInfo* window, int grid_x, int grid_y, int shots) {}
