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

#include "unitstats.hpp"

#include "builder.hpp"
#include "cargo.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "reportstats.hpp"
#include "resource_manager.hpp"
#include "taskdebugger.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

class UnitStatsMenu : public Window {
    WindowInfo window;
    TaskDebugger* task_debugger = nullptr;
    Button* button_done;
    Button* button_help;
    SmartPointer<UnitInfo> unit;
    bool event_release;

public:
    UnitStatsMenu(UnitInfo* unit);
    ~UnitStatsMenu();

    void Run();
};

static void UnitStats_DrawText(uint8_t* buffer, int32_t screen_width, const char* label, int32_t line_length,
                               int32_t resource_count, bool drawline);

static int32_t UnitStats_DrawIcon(uint8_t* buffer, int32_t screen_width, struct ImageSimpleHeader* image,
                                  int32_t original, int32_t current, int32_t width, int32_t height);

static int32_t UnitStats_DrawIcons(uint8_t* buffer, int32_t screen_width, int32_t max_width, ResourceID id_normal,
                                   ResourceID id_empty, int32_t current, int32_t original);

static uint8_t* UnitStats_DrawMilitary(uint8_t* buffer, int32_t screen_width, UnitValues* unit_values1,
                                       UnitValues* unit_values2, int32_t image_width);

static uint8_t* UnitStats_DrawCargo(uint8_t* buffer, int32_t screen_width, ResourceID unit_type, uint16_t team,
                                    UnitValues* unit_values, int32_t image_width);

static uint8_t* UnitStats_DrawUsage(uint8_t* buffer, int32_t screen_width, ResourceID unit_type, int32_t image_width);

static uint8_t* UnitStats_DrawCommon(uint8_t* buffer, int32_t screen_width, UnitValues* unit_values1,
                                     UnitValues* unit_values2, int32_t image_width);

static uint8_t* UnitStats_DrawSpeed(uint8_t* buffer, int32_t screen_width, UnitValues* unit_values1,
                                    UnitValues* unit_values2, int32_t image_width);

static void UnitStats_DrawCost(uint8_t* buffer, int32_t screen_width, ResourceID unit_type, int32_t value1,
                               int32_t value2, ResourceID id_normal, ResourceID id_empty, int32_t image_width);

void UnitStats_DrawImage(uint8_t* buffer, int32_t window_width, struct ImageSimpleHeader* image) {
    const uint8_t transparent_pixel = image->transparent_color;
    uint8_t* image_data = &image->transparent_color;
    int32_t offset = 0;

    for (int32_t i = 0; i < image->height; ++i) {
        for (int32_t j = 0; j < image->width; ++j) {
            if (transparent_pixel != image_data[j + i * image->width]) {
                buffer[offset + j + i * image->width] = image_data[j + i * image->width];
            }
        }

        offset += window_width - image->width;
    }
}

void UnitStats_DrawStats(uint8_t* buffer, int32_t window_width, ResourceID unit_type, uint16_t team,
                         UnitValues& unit_values, int32_t image_width, ResourceID icon_full, ResourceID icon_empty) {
    UnitValues* unit_values2;

    unit_values2 = UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type);

    if (unit_values2->GetAttribute(ATTRIB_ATTACK)) {
        buffer = UnitStats_DrawMilitary(buffer, window_width, unit_values2, &unit_values, image_width);
    }

    if (unit_values2->GetAttribute(ATTRIB_STORAGE)) {
        buffer = UnitStats_DrawCargo(buffer, window_width, unit_type, team, &unit_values, image_width);
    }

    buffer = UnitStats_DrawUsage(buffer, window_width, unit_type, image_width);
    buffer = UnitStats_DrawCommon(buffer, window_width, unit_values2, &unit_values, image_width);

    if (unit_values2->GetAttribute(ATTRIB_SPEED)) {
        buffer = UnitStats_DrawSpeed(buffer, window_width, unit_values2, &unit_values, image_width);
    }

    if (unit_values2->GetAttribute(ATTRIB_TURNS)) {
        UnitStats_DrawCost(buffer, window_width, unit_type, unit_values2->GetAttribute(ATTRIB_TURNS),
                           unit_values.GetAttribute(ATTRIB_TURNS), icon_full, icon_empty, image_width);
    }
}

void UnitStats_DrawText(uint8_t* buffer, int32_t screen_width, const char* label, int32_t line_length,
                        int32_t resource_count, bool drawline) {
    uint8_t* address;

    Text_SetFont(GNW_TEXT_FONT_5);
    address = &buffer[((19 - Text_GetHeight()) / 2) * screen_width + 24];

    if (resource_count) {
        ReportStats_DrawNumber(address, resource_count, 24, screen_width, COLOR_YELLOW);
    }

    Text_Blit(&address[4], label, 49, screen_width, 0xA2);

    if (drawline) {
        draw_line(buffer, screen_width, 2, 18, line_length - 2, 18, COLOR_RED);
    }
}

int32_t UnitStats_DrawIcon(uint8_t* buffer, int32_t screen_width, struct ImageSimpleHeader* image, int32_t original,
                           int32_t current, int32_t width, int32_t height) {
    int32_t offset = 0;

    for (int32_t i = 0; i < original; ++i) {
        int32_t reminder = i % 5;
        offset = width * (i / 5) + current * reminder;

        UnitStats_DrawImage(&buffer[offset + screen_width * reminder * height], screen_width, image);
    }

    return image->width + offset;
}

int32_t UnitStats_DrawIcons(uint8_t* buffer, int32_t screen_width, int32_t max_width, ResourceID id_normal,
                            ResourceID id_empty, int32_t current, int32_t original) {
    int32_t result;
    struct ImageSimpleHeader* image_normal;
    struct ImageSimpleHeader* image_empty;
    int32_t height;
    int32_t original_scaled;
    int32_t current_scaled;
    int32_t original_offset;
    int32_t current_offset;
    int32_t full_scaled;
    int32_t var_24;
    int32_t var_28;
    int32_t var_2C;
    int32_t width;
    int32_t var_34;
    uint8_t* address;

    image_normal = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(id_normal));
    image_empty = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(id_empty));

    if (image_normal) {
        if (!image_empty) {
            image_empty = image_normal;
        }

        original -= current;

        if (original) {
            max_width -= 5;
        }

        if (current >= 6) {
            height = (18 - image_normal->height) / 4;
        } else {
            height = 0;
        }

        address = &buffer[(((18 - (height * 4)) - image_normal->height) / 2) * screen_width];

        current_scaled = (current + 4) / 5;
        current_offset = current - ((current_scaled - 1) * 5);

        original_scaled = (original + 4) / 5;
        original_offset = original - ((original_scaled - 1) * 5);

        full_scaled = current_scaled + original_scaled;

        max_width -= image_normal->width * full_scaled;

        var_2C = full_scaled * 4 - (5 - current_offset);

        if (original_scaled) {
            var_2C -= 5 - original_offset;
        }

        if (var_2C) {
            if (max_width < var_2C) {
                var_34 = 1;

                if (original_scaled) {
                    --full_scaled;
                }

                SDL_assert(full_scaled != 1);

                var_28 = (full_scaled + (var_2C - max_width) - 2) / (full_scaled - 1);
            } else {
                SDL_assert(var_2C != 0);

                var_34 = max_width / var_2C;

                if (var_34 > image_normal->width) {
                    var_34 = image_normal->width;
                }

                var_28 = 0;
            }

        } else {
            var_34 = image_normal->width;
            var_28 = 0;
        }

        width = (var_34 * 4 + image_normal->width) - var_28;

        var_24 = UnitStats_DrawIcon(address, screen_width, image_normal, current, var_34, width, height);

        if (original) {
            draw_line(&buffer[var_24 + 2], screen_width, 0, 2, 0, 16, COLOR_RED);

            var_24 += 5;

            var_24 += UnitStats_DrawIcon(&address[var_24], screen_width, image_empty, original, var_34, width, height);
        }

        result = var_24;

    } else {
        result = 0;
    }

    return result;
}

uint8_t* UnitStats_DrawMilitary(uint8_t* buffer, int32_t screen_width, UnitValues* unit_values1,
                                UnitValues* unit_values2, int32_t image_width) {
    UnitStats_DrawText(buffer, screen_width, _(98a8), image_width, unit_values2->GetAttribute(ATTRIB_ATTACK), true);
    UnitStats_DrawIcons(&buffer[76], screen_width, image_width - 76, I_HRDATK, I_HRDATK,
                        unit_values1->GetAttribute(ATTRIB_ATTACK), unit_values2->GetAttribute(ATTRIB_ATTACK));

    buffer = &buffer[screen_width * 19];

    if (unit_values1->GetAttribute(ATTRIB_ROUNDS) > 0) {
        UnitStats_DrawText(buffer, screen_width, _(5e40), image_width,
                           std::min(unit_values2->GetAttribute(ATTRIB_ROUNDS), unit_values2->GetAttribute(ATTRIB_AMMO)),
                           true);
        UnitStats_DrawIcons(
            &buffer[76], screen_width, image_width - 76, I_SHOTS, I_SHOTS, unit_values1->GetAttribute(ATTRIB_ROUNDS),
            std::min(unit_values2->GetAttribute(ATTRIB_ROUNDS), unit_values2->GetAttribute(ATTRIB_AMMO)));

        buffer = &buffer[screen_width * 19];
    }

    if (unit_values1->GetAttribute(ATTRIB_RANGE)) {
        UnitStats_DrawText(buffer, screen_width, _(2540), image_width, unit_values2->GetAttribute(ATTRIB_RANGE), true);
        UnitStats_DrawIcons(&buffer[76], screen_width, image_width - 76, I_RANGE, I_RANGE,
                            unit_values1->GetAttribute(ATTRIB_RANGE), unit_values2->GetAttribute(ATTRIB_RANGE));

        buffer = &buffer[screen_width * 19];
    }

    if (unit_values1->GetAttribute(ATTRIB_AMMO)) {
        UnitStats_DrawText(buffer, screen_width, _(d510), image_width, unit_values2->GetAttribute(ATTRIB_AMMO), true);
        UnitStats_DrawIcons(&buffer[76], screen_width, image_width - 76, I_AMMO, I_AMMO,
                            unit_values1->GetAttribute(ATTRIB_AMMO), unit_values2->GetAttribute(ATTRIB_AMMO));

        buffer = &buffer[screen_width * 19];
    }

    return buffer;
}

uint8_t* UnitStats_DrawCargo(uint8_t* buffer, int32_t screen_width, ResourceID unit_type, uint16_t team,
                             UnitValues* unit_values, int32_t image_width) {
    ResourceID icon;

    switch (unit_type) {
        case ADUMP:
        case CARGOSHP:
        case SPLYTRCK:
        case ENGINEER:
        case CONSTRCT: {
            icon = I_RAW;
        } break;

        case FUELTRCK:
        case FDUMP: {
            icon = I_FUEL;
        } break;

        case GOLDSM:
        case GOLDTRCK: {
            icon = I_GOLD;
        } break;

        default: {
            return buffer;
        } break;
    }

    UnitStats_DrawText(buffer, screen_width, _(054b), image_width, unit_values->GetAttribute(ATTRIB_STORAGE), true);

    UnitStats_DrawIcons(
        &buffer[76], screen_width, image_width - 76, icon, icon,
        UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type)->GetAttribute(ATTRIB_STORAGE) / 10,
        unit_values->GetAttribute(ATTRIB_STORAGE) / 10);

    buffer = &buffer[screen_width * 19];

    return buffer;
}

uint8_t* UnitStats_DrawUsage(uint8_t* buffer, int32_t screen_width, ResourceID unit_type, int32_t image_width) {
    int32_t raw_material_usage;
    int32_t fuel_usage;
    int32_t power_usage;
    int32_t life_usage;
    int32_t gold_usage;

    raw_material_usage = Cargo_GetRawConsumptionRate(unit_type, 1);
    fuel_usage = Cargo_GetFuelConsumptionRate(unit_type);
    power_usage = Cargo_GetPowerConsumptionRate(unit_type);
    life_usage = Cargo_GetLifeConsumptionRate(unit_type);
    gold_usage = Cargo_GetGoldConsumptionRate(unit_type);

    if (power_usage < 0) {
        UnitStats_DrawText(buffer, screen_width, _(527f), image_width, -power_usage, true);
        UnitStats_DrawIcons(buffer + 76, screen_width, image_width - 76, I_POWER, I_POWER, -power_usage, -power_usage);

        buffer = &buffer[screen_width * 19];
    }

    if (life_usage < 0) {
        UnitStats_DrawText(buffer, screen_width, _(3641), image_width, -life_usage, true);
        UnitStats_DrawIcons(buffer + 76, screen_width, image_width - 76, I_LIFE, I_LIFE, -life_usage, -life_usage);

        buffer = &buffer[screen_width * 19];
    }

    if (raw_material_usage > 0 || fuel_usage > 0 || power_usage > 0 || life_usage > 0) {
        int32_t offset;

        UnitStats_DrawText(buffer, screen_width, _(9e61), image_width, 0, true);
        offset = 76;

        if (raw_material_usage > 0) {
            offset += UnitStats_DrawIcons(&buffer[offset], screen_width, image_width - offset, I_RAW, I_RAW,
                                          raw_material_usage, raw_material_usage);
        }

        if (fuel_usage > 0) {
            offset += UnitStats_DrawIcons(&buffer[offset], screen_width, image_width - offset, I_FUEL, I_FUEL,
                                          fuel_usage, fuel_usage);
        }

        if (power_usage > 0) {
            offset += UnitStats_DrawIcons(&buffer[offset], screen_width, image_width - offset, I_POWER, I_POWER,
                                          power_usage, power_usage);
        }

        if (life_usage > 0) {
            offset += UnitStats_DrawIcons(&buffer[offset], screen_width, image_width - offset, I_LIFE, I_LIFE,
                                          life_usage, life_usage);
        }

        if (gold_usage > 0) {
            UnitStats_DrawIcons(&buffer[offset], screen_width, image_width - offset, I_GOLD, I_GOLD, gold_usage,
                                gold_usage);
        }

        buffer = &buffer[screen_width * 19];
    }

    return buffer;
}

uint8_t* UnitStats_DrawCommon(uint8_t* buffer, int32_t screen_width, UnitValues* unit_values1, UnitValues* unit_values2,
                              int32_t image_width) {
    if (unit_values1->GetAttribute(ATTRIB_ARMOR)) {
        UnitStats_DrawText(buffer, screen_width, _(49d1), image_width, unit_values2->GetAttribute(ATTRIB_ARMOR), true);
        UnitStats_DrawIcons(&buffer[76], screen_width, image_width - 76, I_ARMOR, I_ARMOR,
                            unit_values1->GetAttribute(ATTRIB_ARMOR), unit_values2->GetAttribute(ATTRIB_ARMOR));
        buffer = &buffer[screen_width * 19];
    }

    if (unit_values1->GetAttribute(ATTRIB_HITS)) {
        UnitStats_DrawText(buffer, screen_width, _(be42), image_width, unit_values2->GetAttribute(ATTRIB_HITS), true);
        UnitStats_DrawIcons(&buffer[76], screen_width, image_width - 76, I_HITS, I_HITS,
                            unit_values1->GetAttribute(ATTRIB_HITS), unit_values2->GetAttribute(ATTRIB_HITS));
        buffer = &buffer[screen_width * 19];
    }

    if (unit_values1->GetAttribute(ATTRIB_SCAN)) {
        UnitStats_DrawText(buffer, screen_width, _(d10e), image_width, unit_values2->GetAttribute(ATTRIB_SCAN), true);
        UnitStats_DrawIcons(&buffer[76], screen_width, image_width - 76, I_SCAN, I_SCAN,
                            unit_values1->GetAttribute(ATTRIB_SCAN), unit_values2->GetAttribute(ATTRIB_SCAN));
        buffer = &buffer[screen_width * 19];
    }

    return buffer;
}

uint8_t* UnitStats_DrawSpeed(uint8_t* buffer, int32_t screen_width, UnitValues* unit_values1, UnitValues* unit_values2,
                             int32_t image_width) {
    UnitStats_DrawText(buffer, screen_width, _(6136), image_width, unit_values2->GetAttribute(ATTRIB_SPEED), true);
    UnitStats_DrawIcons(&buffer[76], screen_width, image_width - 76, I_SPEED, I_SPEED,
                        unit_values1->GetAttribute(ATTRIB_SPEED), unit_values2->GetAttribute(ATTRIB_SPEED));
    buffer = &buffer[screen_width * 19];

    return buffer;
}

void UnitStats_DrawCost(uint8_t* buffer, int32_t screen_width, ResourceID unit_type, int32_t value1, int32_t value2,
                        ResourceID id_normal, ResourceID id_empty, int32_t image_width) {
    int32_t level1;
    int32_t level2;

    level1 = Cargo_GetRawConsumptionRate(Builder_GetBuilderType(unit_type), 1) * value1;
    level2 = Cargo_GetRawConsumptionRate(Builder_GetBuilderType(unit_type), 1) * value2;

    if (level1 < 1) {
        level1 = 1;
    }

    if (level2 < 1) {
        level2 = 1;
    }

    UnitStats_DrawText(buffer, screen_width, _(7dfb), image_width, level2, false);
    UnitStats_DrawIcons(&buffer[76], screen_width, image_width - 76, id_normal, id_empty, level2, level1);
}

void UnitStats_Menu(UnitInfo* unit) {
    UnitStatsMenu menu(unit);

    menu.Run();
}

UnitStatsMenu::UnitStatsMenu(UnitInfo* unit_)
    : Window(STATS, GameManager_GetDialogWindowCenterMode()), unit(unit_), event_release(false) {
    WindowInfo debug_window;
    BaseUnit* base_unit;
    char text[40];

    Add();
    FillWindowInfo(&window);

    Cursor_SetCursor(CURSOR_HAND);

    GameManager_DisableMainMenu();

    Text_SetFont(GNW_TEXT_FONT_5);

    button_done = new (std::nothrow) Button(BLDONE_U, BLDONE_D, 484, 452);
    button_done->SetCaption(_(cf39));
    button_done->SetRValue(GNW_KB_KEY_KP_ENTER);
    button_done->SetPValue(GNW_KB_KEY_KP_ENTER + GNW_INPUT_PRESS);
    button_done->SetSfx(NDONE0);
    button_done->RegisterButton(window.id);

    button_help = new (std::nothrow) Button(BLDHLP_U, BLDHLP_D, 457, 452);
    button_help->SetRValue(1000);
    button_help->SetPValue(1000 + GNW_INPUT_PRESS);
    button_help->SetSfx(NHELP0);
    button_help->RegisterButton(window.id);

    base_unit = &UnitsManager_BaseUnits[unit->GetUnitType()];

    debug_window = window;

    debug_window.buffer = &window.buffer[11 + window.width * 13];
    debug_window.window.ulx = 11;
    debug_window.window.uly = 13;
    debug_window.window.lrx = 311;
    debug_window.window.lry = 253;

    if (TaskDebugger_GetDebugMode() && unit->GetTask()) {
        task_debugger = new (std::nothrow) TaskDebugger(&debug_window, unit->GetTask(), 2000, 2001, 2002);

    } else {
        if (!WindowManager_LoadBigImage(base_unit->portrait, &debug_window, window.width, false)) {
            flicsmgr_construct(base_unit->flics, &window, window.width, 100, 74, false, false);
        }
    }

    UnitStats_DrawStats(&window.buffer[11 + window.width * 293], window.width, unit->GetUnitType(), unit->team,
                        *unit->GetBaseValues(), 245, I_RAW, I_RAWE);

    Text_SetFont(GNW_TEXT_FONT_5);

    unit->GetDisplayName(text, sizeof(text));

    Text_TextBox(window.buffer, window.width, text, 336, 60, 287, 11, 0xA2, true);
    Text_TextBox(window.buffer, window.width, base_unit->description, 345, 90, 270, 117, 0xA2, false, false);

    Text_SetFont(GNW_TEXT_FONT_5);
    Text_TextBox(window.buffer, window.width, _(94b7), 327, 7, 158, 18, COLOR_GREEN, true);

    if (task_debugger) {
        task_debugger->DrawRows();
    }

    win_draw(window.id);
}

UnitStatsMenu::~UnitStatsMenu() {
    delete task_debugger;
    delete button_done;
    delete button_help;

    GameManager_EnableMainMenu(&*GameManager_SelectedUnit);
    GameManager_ProcessTick(true);
}

void UnitStatsMenu::Run() {
    int32_t key = 0;

    while (key != GNW_KB_KEY_ESCAPE && key != GNW_KB_KEY_KP_ENTER) {
        key = get_input();

        if (key > 0 && key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        if (key < GNW_INPUT_PRESS) {
            if (key == GNW_KB_KEY_LALT_P) {
                PauseMenu_Menu();
            }

            if (key == 1000 || key == '?') {
                HelpMenu_Menu(HELPMENU_STATS_SETUP, WINDOW_MAIN_WINDOW);
            }

            if (task_debugger && key >= 0) {
                task_debugger->ProcessKeyPress(key);
            }

            if (GameManager_RequestMenuExit) {
                key = GNW_KB_KEY_KP_ENTER;
            }

            GameManager_ProcessState(false);

        } else if (!event_release) {
            if (GNW_KB_KEY_KP_ENTER + GNW_INPUT_PRESS) {
                button_done->PlaySound();

            } else {
                button_help->PlaySound();
            }

            event_release = true;
        }
    }
}
