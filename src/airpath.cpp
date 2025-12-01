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

#include "airpath.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "paths.hpp"
#include "registerarray.hpp"
#include "remote.hpp"
#include "sound_manager.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"

static void Paths_DrawMissile(UnitInfo* unit, int32_t position_x, int32_t position_y);
static void Paths_FinishMove(UnitInfo* unit);
static void Paths_TakeStep(UnitInfo* unit, int32_t cost);

static uint32_t Paths_AirPath_TypeIndex;
static RegisterClass Paths_AirPath_ClassRegister("AirPath", &Paths_AirPath_TypeIndex, &AirPath::Allocate);

AirPath::AirPath()
    : m_length(0), m_angle(0), m_start_x(0), m_start_y(0), m_step_x(0), m_step_y(0), m_delta_x(0), m_delta_y(0) {}

AirPath::AirPath(UnitInfo* unit, int32_t distance_x, int32_t distance_y, int32_t euclidean_distance, int32_t target_x,
                 int32_t target_y)
    : UnitPath(distance_x, distance_y, euclidean_distance, target_x, target_y),
      m_length(euclidean_distance),
      m_start_x(unit->x),
      m_start_y(unit->y),
      m_step_x(0),
      m_step_y(0),
      m_delta_x((distance_x << 22) / euclidean_distance),
      m_delta_y((distance_y << 22) / euclidean_distance) {
    if (unit->flags & MOBILE_AIR_UNIT) {
        m_angle = UnitsManager_GetTargetAngle(distance_x, distance_y);

    } else {
        m_angle = unit->angle;
    }
}

AirPath::~AirPath() {}

FileObject* AirPath::Allocate() noexcept { return new (std::nothrow) AirPath(); }

uint32_t AirPath::GetTypeIndex() const { return Paths_AirPath_TypeIndex; }

void AirPath::FileLoad(SmartFileReader& file) noexcept {
    file.Read(m_length);
    file.Read(m_angle);
    file.Read(m_start_x);
    file.Read(m_start_y);
    file.Read(m_end_x);
    file.Read(m_end_y);
    file.Read(m_step_x);
    file.Read(m_step_y);
    file.Read(m_delta_x);
    file.Read(m_delta_y);
}

void AirPath::FileSave(SmartFileWriter& file) noexcept {
    file.Write(m_length);
    file.Write(m_angle);
    file.Write(m_start_x);
    file.Write(m_start_y);
    file.Write(m_end_x);
    file.Write(m_end_y);
    file.Write(m_step_x);
    file.Write(m_step_y);
    file.Write(m_delta_x);
    file.Write(m_delta_y);
}

Point AirPath::GetPosition(UnitInfo* unit) const {
    int32_t pixel_x;
    int32_t pixel_y;
    Point point(unit->grid_x, unit->grid_y);

    if (unit->flags & MOBILE_AIR_UNIT) {
        pixel_x = m_length * m_delta_x + m_step_x;
        pixel_x >>= 16;
        pixel_x += m_start_x;
        pixel_x >>= 6;
        point.x = pixel_x;

        pixel_y = m_length * m_delta_y + m_step_y;
        pixel_y >>= 16;
        pixel_y += m_start_y;
        pixel_y >>= 6;
        point.y = pixel_y;
    }

    return point;
}

void AirPath::CancelMovement(UnitInfo* unit) {
    AILOG(log, "Airpath: emergency stop.");

    if (m_length) {
        if (unit->angle == m_angle) {
            if (m_delta_x > 0) {
                m_end_x = (unit->x + 32) / 64;

            } else {
                m_end_x = (unit->x) / 64;
            }

            if (m_delta_y > 0) {
                m_end_y = (unit->y + 32) / 64;

            } else {
                m_end_y = (unit->y) / 64;
            }

            m_start_x = unit->x;
            m_start_y = unit->y;

            m_step_x = 0;
            m_step_y = 0;

            m_length %= unit->max_velocity;

            if (m_length == 0) {
                ++m_length;
                ++unit->speed;
            }

            m_delta_x = m_end_x * 64 - unit->x + 32;
            m_delta_y = m_end_y * 64 - unit->y + 32;

            m_delta_x = (m_delta_x << 16) / m_length;
            m_delta_y = (m_delta_y << 16) / m_length;

            AILOG_LOG(log, "Recalculated path to [{},{}], length {}.", m_end_x + 1, m_end_y + 1, m_length);

        } else {
            AILOG_LOG(log, "Haven't finished turning.");

            unit->Redraw();

            unit->path = nullptr;

            unit->BlockedOnPathRequest();
        }

    } else {
        AILOG_LOG(log, "Length is zero.");
    }
}

int32_t AirPath::GetMovementCost(UnitInfo* unit) { return (m_length / unit->max_velocity) * 4; }

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

        if (!Paths_UpdateAngle(unit, m_angle)) {
            if (unit->flags & MOBILE_AIR_UNIT) {
                if (m_length == 1) {
                    int32_t dx = labs(m_delta_x >> 16) >> 1;
                    int32_t dy = labs(m_delta_y >> 16) >> 1;

                    if (dx || dy) {
                        ++m_length;
                    }

                    m_delta_x >>= 1;
                    m_delta_y >>= 1;

                } else if (GameManager_SelectedUnit == unit && unit->GetSfxType() != Unit::SFX_TYPE_DRIVE) {
                    SoundManager_PlaySfx(unit, Unit::SFX_TYPE_DRIVE);
                }
            }

            m_step_x += m_delta_x;
            m_step_y += m_delta_y;

            int32_t position_x = (m_step_x >> 16) + m_start_x;
            int32_t position_y = (m_step_y >> 16) + m_start_y;
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

            --m_length;

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

                    if (GameManager_SelectedUnit == unit && m_length == 1) {
                        SoundManager_PlaySfx(unit, Unit::SFX_TYPE_STOP);
                    }
                }

                unit->FollowUnit();

                if ((m_length % unit->max_velocity) == 0) {
                    Paths_TakeStep(unit, 1);

                    if (GameManager_SelectedUnit == unit) {
                        GameManager_UpdateInfoDisplay(unit);
                    }
                }
            }

            if (m_length == 0) {
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

    steps = m_length / unit->max_velocity;

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    grid_x = (grid_x << 6) + 32;
    grid_y = (grid_y << 6) + 32;

    step_x = m_step_x;
    step_y = m_step_y;

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

            Paths_DrawMarker(window, m_angle, scaled_grid_x, scaled_grid_y, color);
        }

        if (max_speed == 0) {
            max_speed = base_speed;
        }

        --max_speed;

        step_x += unit->max_velocity * m_delta_x;
        step_y += unit->max_velocity * m_delta_y;

        grid_x = (step_x >> 16) + m_start_x;
        grid_y = (step_y >> 16) + m_start_y;
    }
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

            if (!UnitsManager_LoadUnit(unit)) {
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
