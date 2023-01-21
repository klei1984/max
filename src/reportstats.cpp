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

#include "reportstats.hpp"

#include "cargo.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "inifile.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "unitstats.hpp"

struct InterfaceMeta {
    unsigned short divisor;
    ColorIndex* color_index_table;
    unsigned short brightness;
    ResourceID icon;
    unsigned char* buffer;
    unsigned short width;
    Rect bounds;
    unsigned short ulx;
    unsigned short uly;
    unsigned short frame_index;
};

static void ReportStats_RenderSprite(struct InterfaceMeta* data);
static void ReportStats_DrawIcons(WindowInfo* window, ResourceID icon_normal, ResourceID icon_empty, int current_value,
                                  int base_value);
static void ReportStats_DrawRowEx(const char* text, WinID id, Rect* bounds, int row_id, ResourceID icon_normal,
                                  ResourceID icon_empty, int current_value, int base_value, int factor, bool drawline);
static void ReportStats_DrawCommonUnit(UnitInfo* unit, WinID id, Rect* bounds);
static void ReportStats_DrawStorageUnit(UnitInfo* unit, WinID id, Rect* bounds);
static void ReportStats_DrawPointsUnit(UnitInfo* unit, WinID id, Rect* bounds);

const ResourceID ReportStats_CargoIcons[] = {
    INVALID_ID, INVALID_ID, SI_RAW,  EI_RAW, SI_FUEL, EI_FUEL, SI_GOLD,
    EI_GOLD,    SI_LAND,    EI_LAND, SI_SEA, EI_SEA,  SI_AIR,  EI_AIR,
};

void ReportStats_RenderSprite(struct InterfaceMeta* data) {
    unsigned int scaling_divisor_factor;
    unsigned int data_size;
    unsigned int map_scaling_factor;
    unsigned char* image_frame;
    int map_window_ulx;
    int map_window_uly;

    scaling_divisor_factor = 1 << data->divisor;

    Gfx_ResourceBuffer = ResourceManager_GetBuffer(data->icon);

    if (!Gfx_ResourceBuffer) {
        Gfx_ResourceBuffer = ResourceManager_LoadResource(data->icon);

        if (!Gfx_ResourceBuffer) {
            return;
        }

        Gfx_ResourceBuffer = Gfx_RescaleSprite(Gfx_ResourceBuffer, &data_size, 0, scaling_divisor_factor);
        ResourceManager_Realloc(data->icon, Gfx_ResourceBuffer, data_size);
    }

    image_frame = &Gfx_ResourceBuffer[reinterpret_cast<unsigned int*>(
        &Gfx_ResourceBuffer[sizeof(unsigned short)])[data->frame_index]];

    map_scaling_factor = Gfx_MapScalingFactor;
    map_window_ulx = Gfx_MapWindowUlx;
    map_window_uly = Gfx_MapWindowUly;
    Gfx_MapScalingFactor = 0x10000;
    Gfx_MapWindowUlx = 0;
    Gfx_MapWindowUly = 0;
    Gfx_MapWindowWidth = data->width;

    if (Gfx_DecodeSpriteSetup(Point(data->ulx, data->uly), image_frame, 2, &data->bounds)) {
        Gfx_SpriteRowAddresses = reinterpret_cast<unsigned int*>(&image_frame[sizeof(short) * 4]);
        Gfx_TeamColorIndexBase = 0;
        Gfx_ColorIndices = data->color_index_table;
        Gfx_UnitBrightnessBase = data->brightness;
        Gfx_MapWindowBuffer = data->buffer;

        Gfx_DecodeSprite();
    }

    Gfx_MapScalingFactor = map_scaling_factor;
    Gfx_MapWindowWidth = 640;
    Gfx_MapWindowUlx = map_window_ulx;
    Gfx_MapWindowUly = map_window_uly;
}

void ReportStats_DrawIcons(WindowInfo* window, ResourceID icon_normal, ResourceID icon_empty, int current_value,
                           int base_value) {
    struct ImageSimpleHeader* image;
    int width;
    int height;
    int width_offset;
    int height_offset;
    unsigned char* buffer;
    int buffer_position;
    int base_value_scaled;
    int base_value_diff;
    int factor;
    int offset;
    int ulx;
    int uly;
    int icon_index;

    width = window->window.lrx - window->window.ulx;
    height = window->window.lry - window->window.uly;

    if (current_value) {
        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(icon_normal));

    } else {
        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(icon_empty));
    }

    if (image) {
        if (base_value > 5) {
            height_offset = (height - 1 - image->height) / 4;

        } else {
            height_offset = 0;
        }

        buffer = &window->buffer[((((height - 1) - (height_offset * 4)) - image->height) / 2) * window->width];

        base_value_scaled = (base_value + 4) / 5;

        base_value_diff = base_value - ((base_value_scaled - 1) * 5);

        width -= image->width * base_value_scaled;

        width_offset = base_value_scaled * 4 - (5 - base_value_diff);

        if (width_offset) {
            if (width >= width_offset) {
                factor = width / width_offset;

                if (factor > image->width) {
                    factor = image->width;
                }

                offset = 0;

            } else {
                factor = 1;
                offset = (base_value_scaled + (width_offset - width) - 2) / (base_value_scaled - 1);
            }

        } else {
            factor = image->width;
            offset = 0;
        }

        ulx = (factor * 4 + image->width) - offset;
        buffer_position = 0;

        for (int i = 0; i < base_value; ++i) {
            icon_index = i % 5;
            buffer_position = ulx * (i / 5) + (factor * icon_index);
            uly = icon_index * height_offset;

            UnitStats_DrawImage(&buffer[buffer_position + uly * window->width], window->width, image);
            --current_value;

            if (current_value == 0) {
                image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(icon_empty));
            }
        }
    }
}

void ReportStats_DrawRowEx(const char* text, WinID id, Rect* bounds, int row_id, ResourceID icon_normal,
                           ResourceID icon_empty, int current_value, int base_value, int factor, bool drawline) {
    Rect new_bounds;
    int height;

    height = bounds->lry - bounds->uly;

    new_bounds.ulx = bounds->ulx;
    new_bounds.lrx = bounds->lrx;

    new_bounds.uly = ((height * row_id) / 4) + bounds->uly + 1;
    new_bounds.lry = ((height * (row_id + 1)) / 4) + bounds->uly - 1;

    ReportStats_DrawRow(text, id, &new_bounds, icon_normal, icon_empty, current_value, base_value, factor, drawline);
}

void ReportStats_DrawRow(const char* text, WinID id, Rect* bounds, ResourceID icon_normal, ResourceID icon_empty,
                         int current_value, int base_value, int factor, bool drawline) {
    WindowInfo window;
    int width;
    int height;
    int color;
    int size;
    char string[10];

    width = bounds->lrx - bounds->ulx;
    height = bounds->lry - bounds->uly;

    window.id = id;
    window.width = win_width(id);
    window.buffer = &win_get_buf(id)[window.width * bounds->uly + bounds->ulx];
    window.window = *bounds;

    if (drawline) {
        draw_line(window.buffer, window.width, 0, height, width - 1, height, 0x3C);
    }

    if (base_value) {
        current_value = std::min(current_value, base_value);

        if (current_value > base_value / 4) {
            if (current_value > base_value / 2) {
                color = COLOR_GREEN;

            } else {
                if (icon_normal == SI_HITSB) {
                    icon_normal = SI_HITSY;
                    icon_empty = EI_HITSY;
                }

                color = COLOR_YELLOW;
            }

        } else {
            if (icon_normal == SI_HITSB) {
                icon_normal = SI_HITSR;
                icon_empty = EI_HITSR;
            }

            color = COLOR_RED;
        }

        text_font(GNW_TEXT_FONT_2);
        sprintf(string, "%i/%i", current_value, base_value);

        if (icon_normal == SI_FUEL || icon_normal == SI_GOLD) {
            size = 20;
        } else {
            size = 25;
        }

        if (((base_value + factor - 1) / factor) > size) {
            factor = (base_value + size - 1) / size;
        }

        Text_TextBox(window.buffer, window.width, string, 0, 0, 45, height, color, true);

        current_value = (current_value + factor - 1) / factor;
        base_value = (base_value + factor - 1) / factor;

        Text_TextBox(window.buffer, window.width, text, 45, 0, 30, height, 0xA2);

        window.buffer = &window.buffer[75];
        window.window.ulx += 75;

        ReportStats_DrawIcons(&window, icon_normal, icon_empty, current_value, base_value);
    }
}

void ReportStats_DrawListItemIcon(unsigned char* buffer, int width, ResourceID unit_type, unsigned short team, int ulx,
                                  int uly) {
    InterfaceMeta data;

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        data.divisor = 2;
    } else {
        data.divisor = 1;
    }

    data.buffer = buffer;
    data.width = width;
    data.frame_index = 0;
    data.brightness = 0xFF;
    data.color_index_table = UnitsManager_TeamInfo[team].team_units->color_index_table;
    data.icon = UnitsManager_BaseUnits[unit_type].icon;
    data.ulx = ulx;
    data.uly = uly;

    data.bounds.ulx = ulx - 16;
    data.bounds.uly = uly - 16;
    data.bounds.lrx = data.bounds.ulx + 32;
    data.bounds.lry = data.bounds.uly + 32;

    ReportStats_RenderSprite(&data);

    if (UnitsManager_BaseUnits[unit_type].flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        struct BaseUnitDataFile* data_file =
            reinterpret_cast<struct BaseUnitDataFile*>(UnitsManager_BaseUnits[unit_type].data_buffer);

        data.ulx += data_file->angle_offsets[2].x >> data.divisor;
        data.uly += data_file->angle_offsets[2].y >> data.divisor;
        ++data.frame_index;

        ReportStats_RenderSprite(&data);
    }
}

void ReportStats_DrawListItem(unsigned char* buffer, int width, ResourceID unit_type, int ulx, int uly, int full,
                              int color) {
    ReportStats_DrawListItemIcon(buffer, width, unit_type, GameManager_PlayerTeam, ulx + 16, uly + 16);
    Text_TextBox(buffer, width, UnitsManager_BaseUnits[unit_type].singular_name, ulx + 35, uly, full - 35, 32, color,
                 false);
}

void ReportStats_DrawNumber(unsigned char* buffer, int number, int width, int full, int color) {
    char text_buffer[10];

    snprintf(text_buffer, sizeof(text_buffer), "%i", number);
    ReportStats_DrawText(buffer, text_buffer, width, full, color);
}

void ReportStats_DrawText(unsigned char* buffer, char* text, int width, int full, int color) {
    int length;

    length = text_width(text);

    if (length > width) {
        length = width;
    }

    text_to_buf(&buffer[-length], text, width, full, color);
}

void ReportStats_DrawCommonUnit(UnitInfo* unit, WinID id, Rect* bounds) {
    int power_consumption_base;
    int power_consumption_current;

    power_consumption_base = Cargo_GetPowerConsumptionRate(unit->unit_type);

    if (power_consumption_base && GameManager_PlayerTeam == unit->team &&
        !Cargo_GetLifeConsumptionRate(unit->unit_type)) {
        int current_value;
        int base_value;
        Cargo cargo;

        power_consumption_current = power_consumption_base;
        current_value = 0;
        base_value = 0;

        Cargo_GetCargoDemand(unit, &cargo);
        power_consumption_current = cargo.power;

        if (power_consumption_base >= 0) {
            ReportStats_DrawRowEx("Usage", id, bounds, 1, SI_POWER, EI_POWER, -power_consumption_current,
                                  power_consumption_base, 1, false);
        } else {
            ReportStats_DrawRowEx("Power", id, bounds, 1, SI_POWER, EI_POWER, power_consumption_current,
                                  -power_consumption_base, 1, false);
        }

        power_consumption_base = 0;
        power_consumption_current = 0;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).GetComplex() == unit->GetComplex()) {
                cargo.power = Cargo_GetPowerConsumptionRate((*it).unit_type);

                if (cargo.power >= 0) {
                    base_value += cargo.power;
                } else {
                    power_consumption_base -= cargo.power;
                }

                Cargo_GetCargoDemand(&*it, &cargo);

                if (cargo.power >= 0) {
                    power_consumption_current += cargo.power;
                } else {
                    current_value -= cargo.power;
                }
            }
        }

        if (Cargo_GetPowerConsumptionRate(unit->unit_type) > 0) {
            ReportStats_DrawRowEx("Total", id, bounds, 2, SI_POWER, EI_POWER, current_value, base_value, 1, false);
        } else {
            ReportStats_DrawRowEx("Total", id, bounds, 2, SI_POWER, EI_POWER, power_consumption_current,
                                  power_consumption_base, 1, false);
            ReportStats_DrawRowEx("Usage", id, bounds, 3, SI_POWER, EI_POWER, current_value, base_value, 1, false);
        }
    }
}

void ReportStats_DrawStorageUnit(UnitInfo* unit, WinID id, Rect* bounds) {
    int life_need;

    life_need = Cargo_GetLifeConsumptionRate(unit->unit_type);

    if (life_need && GameManager_PlayerTeam == unit->team) {
        int current_life_need;
        int current_value;
        int base_value;
        Cargo cargo;

        current_life_need = life_need;
        current_value = 0;
        base_value = 0;

        Cargo_GetCargoDemand(unit, &cargo);

        current_life_need = cargo.life;

        if (life_need >= 0) {
            ReportStats_DrawRowEx("Usage", id, bounds, 1, SI_WORK, EI_WORK, -current_life_need, life_need, 1, false);
        } else {
            ReportStats_DrawRowEx("Teams", id, bounds, 1, SI_WORK, EI_WORK, current_life_need, -life_need, 1, false);
        }

        life_need = 0;
        current_life_need = 0;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).GetComplex() == unit->GetComplex()) {
                cargo.life = Cargo_GetLifeConsumptionRate((*it).unit_type);

                if (cargo.life >= 0) {
                    base_value += cargo.life;

                } else {
                    life_need -= cargo.life;
                }

                Cargo_GetCargoDemand(&*it, &cargo);

                if (cargo.life >= 0) {
                    current_life_need += cargo.life;

                } else {
                    current_value -= cargo.life;
                }
            }
        }

        if (Cargo_GetLifeConsumptionRate(unit->unit_type) > 0) {
            ReportStats_DrawRowEx("Total", id, bounds, 2, SI_WORK, EI_WORK, current_value, base_value, 1, false);
        } else {
            ReportStats_DrawRowEx("Total", id, bounds, 2, SI_WORK, EI_WORK, current_life_need, life_need, 1, false);
            ReportStats_DrawRowEx("Usage", id, bounds, 3, SI_WORK, EI_WORK, current_value, base_value, 1, false);
        }
    }
}

void ReportStats_DrawPointsUnit(UnitInfo* unit, WinID id, Rect* bounds) {
    unsigned int victory_limit;
    unsigned int victory_limit_scaled;

    if (ini_setting_victory_type == VICTORY_TYPE_SCORE) {
        victory_limit = ini_setting_victory_limit;
    } else {
        victory_limit = 0;

        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE &&
                UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_ELIMINATED) {
                victory_limit = std::max(victory_limit, UnitsManager_TeamInfo[unit->team].team_points);
            }
        }
    }

    if (victory_limit < 1) {
        victory_limit = 10;
    }

    victory_limit_scaled = (victory_limit + 14) / 15;

    ReportStats_DrawRowEx("points", id, bounds, 1, SI_WORK, EI_WORK, unit->storage, unit->storage, victory_limit_scaled,
                          false);
    ReportStats_DrawRowEx("Total", id, bounds, 2, SI_WORK, EI_WORK, UnitsManager_TeamInfo[unit->team].team_points,
                          victory_limit, victory_limit_scaled, false);
}

void ReportStats_Draw(UnitInfo* unit, WinID id, Rect* bounds) {
    if (unit->IsVisibleToTeam(GameManager_PlayerTeam)) {
        SmartPointer<UnitValues> unit_values(unit->GetBaseValues());

        ReportStats_DrawRowEx("Hits", id, bounds, 0, SI_HITSB, EI_HITSB, unit->hits,
                              unit_values->GetAttribute(ATTRIB_HITS), 4, true);

        if (unit_values->GetAttribute(ATTRIB_ATTACK)) {
            ReportStats_DrawRowEx("Ammo", id, bounds, 1, SI_AMMO, EI_AMMO, unit->ammo,
                                  unit->team == GameManager_PlayerTeam ? unit_values->GetAttribute(ATTRIB_AMMO) : 0, 1,
                                  true);
        } else {
            int cargo_type;
            int cargo_value;

            cargo_type = UnitsManager_BaseUnits[unit->unit_type].cargo_type;
            cargo_value = 3;

            if (unit_values->GetAttribute(ATTRIB_STORAGE) > 4) {
                if (unit_values->GetAttribute(ATTRIB_STORAGE) <= 8) {
                    cargo_value = 2;
                }

            } else {
                cargo_value = 1;
            }

            if (cargo_type < CARGO_TYPE_LAND) {
                cargo_value = 10;
            }

            if (unit->team != GameManager_PlayerTeam) {
                cargo_value = 0;
            }

            ReportStats_DrawRowEx("Cargo", id, bounds, 1, ReportStats_CargoIcons[cargo_type * 2],
                                  ReportStats_CargoIcons[cargo_type * 2 + 1], unit->storage,
                                  unit_values->GetAttribute(ATTRIB_STORAGE), cargo_value, true);
        }

        ReportStats_DrawRowEx("Speed", id, bounds, 2, SI_SPEED, EI_SPEED, unit->speed,
                              unit_values->GetAttribute(ATTRIB_SPEED), 1, true);
        ReportStats_DrawRowEx("Shots", id, bounds, 3, SI_SHOTS, EI_SHOTS, unit->shots,
                              unit_values->GetAttribute(ATTRIB_ROUNDS), 1, false);

        if (!unit_values->GetAttribute(ATTRIB_STORAGE)) {
            ReportStats_DrawCommonUnit(unit, id, bounds);
        }

        if (unit->unit_type == GREENHSE && unit->team == GameManager_PlayerTeam) {
            ReportStats_DrawPointsUnit(unit, id, bounds);
        } else {
            ReportStats_DrawStorageUnit(unit, id, bounds);
        }

        if (unit->team == GameManager_PlayerTeam && unit_values->GetAttribute(ATTRIB_STORAGE) &&
            (unit->flags & STATIONARY) && UnitsManager_BaseUnits[unit->unit_type].cargo_type >= CARGO_TYPE_RAW &&
            UnitsManager_BaseUnits[unit->unit_type].cargo_type <= CARGO_TYPE_GOLD) {
            Cargo materials;
            Cargo capacity;
            int cargo_type;
            int cargo_value;
            int current_value;
            int base_value;

            unit->GetComplex()->GetCargoInfo(materials, capacity);
            cargo_type = UnitsManager_BaseUnits[unit->unit_type].cargo_type;

            switch (cargo_type) {
                case CARGO_TYPE_RAW: {
                    current_value = materials.raw;
                    base_value = capacity.raw;
                } break;

                case CARGO_TYPE_FUEL: {
                    current_value = materials.fuel;
                    base_value = capacity.fuel;
                } break;

                case CARGO_TYPE_GOLD: {
                    current_value = materials.gold;
                    base_value = capacity.gold;
                } break;

                default: {
                    SDL_assert(0);
                } break;
            }

            cargo_value = 10;

            if ((base_value / cargo_value) > 25) {
                cargo_value = base_value / 25;
            }

            ReportStats_DrawRowEx("Total", id, bounds, 2,
                                  ReportStats_CargoIcons[UnitsManager_BaseUnits[unit->unit_type].cargo_type * 2],
                                  ReportStats_CargoIcons[UnitsManager_BaseUnits[unit->unit_type].cargo_type * 2 + 1],
                                  current_value, base_value, cargo_value, false);
        }
    }
}
