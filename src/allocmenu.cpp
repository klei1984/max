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

#include "allocmenu.hpp"

#include <array>

#include "complex.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "scrollbar.hpp"
#include "survey.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

#define ALLOCATION_MENU_CONTROL_ITEM_COUNT 11

struct Allocator {
    uint32_t field_0;
    SmartPointer<Complex> complex;
    int32_t cargo_demand;
    int32_t cargo_material_type;
    int32_t material_mining;

    bool Optimize(int32_t cargo_type, uint8_t *cargo1, int16_t cargo_value, uint8_t *cargo2);
};

struct CargoBar {
    WindowInfo *window;
    uint8_t *buffer;
    int16_t width;
    int16_t uly;
    int16_t cargo_1;
    int16_t cargo_2;
    int16_t cargo_3;
    int16_t cargo_4;
    int16_t cargo_5;
    int16_t cargo_6;
    ResourceID bar_id;
    ResourceID tape_id;
    uint8_t *color_index_map;

    void Draw();
};

struct CargoBarData {
    WindowInfo *window;
    uint8_t *buffer;
    int16_t width;
    Rect rect;
    int16_t cargo_1;
    int16_t cargo_2;
    int16_t cargo_3;
    int32_t flag;
    char *caption;
    ResourceID bar_id;
    ResourceID tape_id;
    uint8_t *color_index_map;

    void Draw();
};

class AllocMenu : public Window {
    WindowInfo window;
    uint8_t *cargo_bars_buffer;
    Button *buttons[ALLOCATION_MENU_CONTROL_ITEM_COUNT];
    SmartPointer<UnitInfo> unit;
    SmartPointer<Complex> complex;
    uint8_t color_index_map[256];
    Cargo production;
    Cargo cargo_2;
    Cargo capacity;
    Cargo usage;
    Cargo complex_materials;
    Cargo complex_capacity;

    void Deinit();
    void InitButton(int32_t index);
    void DrawCargoBars();
    static int32_t Balance(Complex *complex, int32_t cargo_type, int32_t material1, int32_t material2);
    int32_t GetHorizontalBarClickValue(WinID wid, int32_t material);
    void UpdateRawAllocation(int32_t material);
    void UpdateFuelAllocation(int32_t material);
    void UpdateGoldAllocation(int32_t material);
    void OnClickRawBar();
    void OnClickFuelBar();
    void OnClickGoldBar();

public:
    AllocMenu(UnitInfo *unit);
    ~AllocMenu();

    void Run();
};

struct AllocMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char *label;
    int32_t event_code;
    void *event_handler;
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    { {(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx) }

static struct AllocMenuControlItem allocation_menu_controls[ALLOCATION_MENU_CONTROL_ITEM_COUNT] = {
    MENU_CONTROL_DEF(139, 70, 0, 0, LFARO_OF, nullptr, 1002, nullptr, KCARG0),
    MENU_CONTROL_DEF(421, 70, 0, 0, RTARO_OF, nullptr, 1001, nullptr, KCARG0),
    MENU_CONTROL_DEF(139, 190, 0, 0, LFARO_OF, nullptr, 1005, nullptr, KCARG0),
    MENU_CONTROL_DEF(421, 190, 0, 0, RTARO_OF, nullptr, 1004, nullptr, KCARG0),
    MENU_CONTROL_DEF(139, 310, 0, 0, LFARO_OF, nullptr, 1008, nullptr, KCARG0),
    MENU_CONTROL_DEF(421, 310, 0, 0, RTARO_OF, nullptr, 1007, nullptr, KCARG0),
    MENU_CONTROL_DEF(174, 70, 440, 99, INVALID_ID, nullptr, 1003, nullptr, KCARG0),
    MENU_CONTROL_DEF(174, 190, 440, 219, INVALID_ID, nullptr, 1006, nullptr, KCARG0),
    MENU_CONTROL_DEF(174, 310, 440, 339, INVALID_ID, nullptr, 1009, nullptr, KCARG0),
    MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, _(eb74), 1000, nullptr, NHELP0),
    MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(40c7), GNW_KB_KEY_RETURN, nullptr, NDONE0),
};

static const struct MenuTitleItem allocation_menu_titles[] = {
    MENU_TITLE_ITEM_DEF(230, 6, 410, 26, _(c771)),   MENU_TITLE_ITEM_DEF(42, 74, 120, 93, _(8481)),
    MENU_TITLE_ITEM_DEF(42, 111, 120, 130, _(1e19)), MENU_TITLE_ITEM_DEF(42, 148, 120, 167, _(5ae6)),
    MENU_TITLE_ITEM_DEF(42, 194, 120, 213, _(a80f)), MENU_TITLE_ITEM_DEF(42, 231, 120, 250, _(5432)),
    MENU_TITLE_ITEM_DEF(42, 268, 120, 287, _(04dd)), MENU_TITLE_ITEM_DEF(42, 315, 120, 333, _(dff1)),
    MENU_TITLE_ITEM_DEF(42, 353, 120, 370, _(e187)), MENU_TITLE_ITEM_DEF(42, 389, 120, 407, _(89a3)),
};

AllocMenu::AllocMenu(UnitInfo *unit) : Window(ALLOCFRM, GameManager_GetDialogWindowCenterMode()), unit(unit) {
    Add();
    FillWindowInfo(&window);

    Text_SetFont(GNW_TEXT_FONT_5);

    Cursor_SetCursor(CURSOR_HAND);

    complex = unit->GetComplex();

    Color_GenerateIntensityTable1(0, 0, 63, 1, 2, color_index_map);

    cargo_bars_buffer = new (std::nothrow) uint8_t[4 * sizeof(uint16_t) + 240 * 344];

    struct ImageSimpleHeader *image = reinterpret_cast<struct ImageSimpleHeader *>(cargo_bars_buffer);

    image->width = 240;
    image->height = 344;
    image->ulx = 0;
    image->uly = 0;

    buf_to_buf(&window.buffer[window.width * 70 + 174], image->width, image->height, window.width,
               &image->transparent_color, image->width);

    for (int32_t i = 0; i < std::size(allocation_menu_titles); ++i) {
        Text_TextBox(window.buffer, window.width, allocation_menu_titles[i].title, allocation_menu_titles[i].bounds.ulx,
                     allocation_menu_titles[i].bounds.uly,
                     allocation_menu_titles[i].bounds.lrx - allocation_menu_titles[i].bounds.ulx,
                     allocation_menu_titles[i].bounds.lry - allocation_menu_titles[i].bounds.uly, COLOR_GREEN, true,
                     true);
    }

    for (int32_t i = 0; i < std::size(allocation_menu_controls); ++i) {
        InitButton(i);
    }

    complex->GetCargoMining(production, capacity);
    complex->GetCargoMinable(usage);
    complex->GetCargoInfo(complex_materials, complex_capacity);

    DrawCargoBars();

    win_draw(window.id);
}

AllocMenu::~AllocMenu() {}

void AllocMenu::InitButton(int32_t index) {
    struct AllocMenuControlItem *control;

    control = &allocation_menu_controls[index];

    Text_SetFont(GNW_TEXT_FONT_1);

    if (control->image_id == INVALID_ID) {
        buttons[index] = new (std::nothrow)
            Button(control->bounds.ulx, control->bounds.uly, control->bounds.lrx - control->bounds.ulx,
                   control->bounds.lry - control->bounds.uly);

    } else {
        buttons[index] = new (std::nothrow) Button(control->image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   control->bounds.ulx, control->bounds.uly);

        if (control->label) {
            buttons[index]->SetCaption(control->label);
        }
    }

    if (index > 8) {
        buttons[index]->SetRValue(control->event_code);
        buttons[index]->SetPValue(index + GNW_INPUT_PRESS);

    } else {
        buttons[index]->SetPValue(control->event_code);
    }

    buttons[index]->SetFlags(0x00);
    buttons[index]->SetSfx(control->sfx);
    buttons[index]->RegisterButton(window.id);
}

void CargoBarData::Draw() {
    struct ImageSimpleHeader *image;
    uint8_t *address;
    int32_t rect_width;
    int32_t rect_height;
    int32_t offset_x;
    int32_t offset_y;
    int32_t offset;
    int32_t str_width;
    Rect bounds;

    image = reinterpret_cast<struct ImageSimpleHeader *>(buffer);
    address = &window->buffer[rect.ulx + width * rect.uly];

    rect_width = rect.lrx - rect.ulx;
    rect_height = rect.lry - rect.uly;

    uint8_t *image_data = &image->transparent_color;

    buf_to_buf(&image_data[image->width * (rect.uly - 70)], rect_width, rect_height, image->width, address, width);

    if (cargo_3) {
        offset_y = (cargo_1 * rect_width) / cargo_3;

        if (flag) {
            offset_x = ((cargo_3 - cargo_2) * rect_width) / cargo_3;

        } else {
            offset_x = 0;
        }

    } else {
        offset_y = 0;

        if (flag) {
            offset_x = rect_width;

        } else {
            offset_x = 0;
        }
    }

    LoadHorizontalBar(address, width, rect_height, offset_y, bar_id);

    LoadHorizontalTape(&address[rect_width - offset_x], width, rect_height, offset_x, tape_id);

    Text_SetFont(GNW_TEXT_FONT_1);

    str_width = Text_GetWidth(caption);

    if (str_width > rect_width) {
        str_width = rect_width;
    }

    bounds.ulx = (rect_width - str_width) / 2 + rect.ulx - 2;
    bounds.uly = (rect_height - Text_GetHeight()) / 2 + rect.uly;
    bounds.lrx = bounds.ulx + str_width + 4;
    bounds.lry = bounds.uly + Text_GetHeight();

    Color_RecolorPixels(&window->buffer[bounds.ulx + bounds.uly * width], width, str_width + 4, Text_GetHeight(),
                        color_index_map);

    offset = ((rect_height - Text_GetHeight()) / 2) + rect.uly;

    offset *= width;

    Text_Blit(&window->buffer[((rect_width - str_width) / 2) + rect.ulx + offset], caption, rect_width, width,
              GNW_TEXT_OUTLINE | 0xFF);

    win_draw_rect(window->id, &rect);
}

void CargoBar::Draw() {
    CargoBarData data;
    char text[100];

    data.caption = text;

    data.window = window;
    data.width = width;
    data.buffer = buffer;

    data.rect.ulx = 174;
    data.rect.uly = uly;
    data.rect.lrx = 414;
    data.rect.lry = uly + 30;

    data.cargo_1 = cargo_1;
    data.cargo_2 = cargo_2;
    data.cargo_3 = cargo_3;

    data.flag = true;

    data.bar_id = bar_id;
    data.tape_id = tape_id;
    data.color_index_map = color_index_map;

    if (data.cargo_3 < cargo_4) {
        data.cargo_3 = cargo_4;
    }

    sprintf(text, "%i", cargo_1);

    data.Draw();

    data.rect.uly += 37;
    data.rect.lry += 37;

    data.cargo_1 = cargo_4;

    data.flag = false;

    sprintf(text, _(592f), cargo_4, cargo_1 - cargo_4);

    data.Draw();

    data.rect.uly += 37;
    data.rect.lry += 37;

    data.cargo_1 = cargo_5;
    data.cargo_3 = cargo_6;

    sprintf(text, "%i", cargo_5);

    data.Draw();
}

void AllocMenu::DrawCargoBars() {
    CargoBar bar;

    cargo_2.raw = Balance(&*complex, CARGO_MATERIALS, production.raw, capacity.raw);
    cargo_2.fuel = Balance(&*complex, CARGO_FUEL, production.fuel, capacity.fuel);
    cargo_2.gold = Balance(&*complex, CARGO_GOLD, production.gold, capacity.gold);

    bar.window = &window;
    bar.width = window.width;
    bar.buffer = cargo_bars_buffer;
    bar.uly = 70;
    bar.cargo_1 = production.raw;
    bar.cargo_2 = cargo_2.raw;
    bar.cargo_3 = capacity.raw;
    bar.cargo_4 = usage.raw;
    bar.cargo_5 = complex_materials.raw;
    bar.cargo_6 = complex_capacity.raw;
    bar.bar_id = BARRAW;
    bar.tape_id = BARTAPE;
    bar.color_index_map = color_index_map;

    bar.Draw();

    bar.window = &window;
    bar.width = window.width;
    bar.uly = 190;
    bar.cargo_1 = production.fuel;
    bar.cargo_2 = cargo_2.fuel;
    bar.cargo_3 = capacity.fuel;
    bar.cargo_4 = usage.fuel;
    bar.cargo_5 = complex_materials.fuel;
    bar.cargo_6 = complex_capacity.fuel;
    bar.bar_id = BARFUEL;
    bar.tape_id = BARTAPE;

    bar.Draw();

    bar.window = &window;
    bar.width = window.width;
    bar.uly = 310;
    bar.cargo_1 = production.gold;
    bar.cargo_2 = cargo_2.gold;
    bar.cargo_3 = capacity.gold;
    bar.cargo_4 = usage.gold;
    bar.cargo_5 = complex_materials.gold;
    bar.cargo_6 = complex_capacity.gold;
    bar.bar_id = BARGOLD;
    bar.tape_id = BARTAPE;

    bar.Draw();
}

bool Allocator::Optimize(int32_t cargo_type, uint8_t *cargo1, int16_t cargo_value, uint8_t *cargo2) {
    if (!(cargo_material_type & cargo_type) && *cargo2) {
        int32_t demand;

        demand = cargo_demand;

        if (*cargo1 + demand > cargo_value) {
            demand = cargo_value - *cargo1;
        }

        if (*cargo2 < demand) {
            demand = *cargo2;
        }

        demand = AllocMenu_Optimize(&*complex, cargo_type, demand, cargo_material_type);

        *cargo2 -= demand;
        *cargo1 += demand;
        material_mining += demand;
        cargo_demand -= demand;
    }

    return cargo_demand == 0;
}

int32_t AllocMenu_Optimize(Complex *complex, int32_t cargo_type1, int32_t material, int32_t cargo_type2) {
    int32_t result;

    if (material) {
        Allocator alloc;
        int16_t raw;
        int16_t fuel;
        int16_t gold;

        alloc.complex = complex;
        alloc.cargo_demand = material;
        alloc.material_mining = 0;
        alloc.cargo_material_type = cargo_type1 | cargo_type2;

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).GetComplex() == complex && (*it).GetUnitType() == MININGST &&
                (*it).GetOrder() != ORDER_POWER_OFF && (*it).GetOrder() != ORDER_DISABLE &&
                (*it).GetOrder() != ORDER_IDLE) {
                Survey_GetResourcesInArea((*it).grid_x, (*it).grid_y, 1, 16, &raw, &gold, &fuel, true, (*it).team);

                uint8_t *cargo{nullptr};
                int16_t cargo_value{0};

                switch (cargo_type1) {
                    case CARGO_GOLD: {
                        cargo = &(*it).gold_mining;
                        cargo_value = gold;
                    } break;

                    case CARGO_MATERIALS: {
                        cargo = &(*it).raw_mining;
                        cargo_value = raw;
                    } break;

                    case CARGO_FUEL: {
                        cargo = &(*it).fuel_mining;
                        cargo_value = fuel;
                    } break;

                    default: {
                        SDL_assert(0);
                    } break;
                };

                if (*cargo < cargo_value) {
                    if ((*it).raw_mining + (*it).fuel_mining + (*it).gold_mining < 16) {
                        int32_t remaining_capacity;

                        remaining_capacity = 16 - ((*it).raw_mining + (*it).fuel_mining + (*it).gold_mining);

                        if (remaining_capacity > alloc.cargo_demand) {
                            remaining_capacity = alloc.cargo_demand;
                        }

                        if (cargo_value - *cargo < remaining_capacity) {
                            remaining_capacity = cargo_value - *cargo;
                        }

                        *cargo += remaining_capacity;
                        (*it).total_mining += remaining_capacity;
                        alloc.material_mining += remaining_capacity;
                        alloc.cargo_demand -= remaining_capacity;

                        if (alloc.cargo_demand == 0) {
                            break;
                        }
                    }

                    if (alloc.Optimize(CARGO_MATERIALS, cargo, cargo_value, &(*it).raw_mining) ||
                        alloc.Optimize(CARGO_FUEL, cargo, cargo_value, &(*it).fuel_mining) ||
                        alloc.Optimize(CARGO_GOLD, cargo, cargo_value, &(*it).gold_mining)) {
                        break;
                    }
                }
            }
        }

        result = alloc.material_mining;

    } else {
        result = 0;
    }

    return result;
}

void AllocMenu_ReduceProduction(Complex *complex, int32_t cargo_type, int32_t amount) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetComplex() == complex && (*it).GetUnitType() == MININGST && (*it).GetOrder() != ORDER_POWER_OFF &&
            (*it).GetOrder() != ORDER_DISABLE && (*it).GetOrder() != ORDER_IDLE) {
            uint8_t *production{nullptr};

            switch (cargo_type) {
                case CARGO_GOLD: {
                    production = &(*it).gold_mining;
                } break;

                case CARGO_MATERIALS: {
                    production = &(*it).raw_mining;
                } break;

                case CARGO_FUEL: {
                    production = &(*it).fuel_mining;
                } break;

                default: {
                    SDL_assert(0);
                } break;
            };

            if (*production < amount) {
                if (*production != 0) {
                    amount -= *production;
                    (*it).total_mining -= *production;
                    *production = 0;
                }

            } else {
                *production -= amount;
                (*it).total_mining -= amount;

                return;
            }
        }
    }
}

int32_t AllocMenu::Balance(Complex *complex, int32_t cargo_type, int32_t material1, int32_t material2) {
    int32_t result;

    if (material1 >= material2) {
        result = material2;

    } else {
        result = AllocMenu_Optimize(complex, cargo_type, material2 - material1, cargo_type);

        AllocMenu_ReduceProduction(complex, cargo_type, result);

        result += material1;
    }

    return result;
}

void AllocMenu::Deinit() {
    for (int32_t i = 0; i < std::size(allocation_menu_controls); ++i) {
        delete buttons[i];
    }

    delete[] cargo_bars_buffer;
}

int32_t AllocMenu::GetHorizontalBarClickValue(WinID wid, int32_t material) {
    int32_t x;
    int32_t y;
    Rect bounds;

    get_input_position(&x, &y);

    win_get_rect(wid, &bounds);

    x -= bounds.ulx + 174;

    return (material * x + 120) / 240;
}

void AllocMenu::UpdateRawAllocation(int32_t material) {
    buttons[6]->PlaySound();

    production.raw += material;
    production.free_capacity -= material;

    complex->material += material;

    DrawCargoBars();
}

void AllocMenu::UpdateFuelAllocation(int32_t material) {
    buttons[7]->PlaySound();

    production.fuel += material;
    production.free_capacity -= material;

    complex->fuel += material;

    DrawCargoBars();
}

void AllocMenu::UpdateGoldAllocation(int32_t material) {
    buttons[8]->PlaySound();

    production.gold += material;
    production.free_capacity -= material;

    complex->gold += material;

    DrawCargoBars();
}

void AllocMenu::OnClickRawBar() {
    int32_t materials;

    materials = GetHorizontalBarClickValue(window.id, capacity.raw) - production.raw;

    if (production.free_capacity < materials) {
        materials = production.free_capacity;
    }

    if (materials < 0) {
        AllocMenu_ReduceProduction(&*complex, CARGO_MATERIALS, -materials);
        UpdateRawAllocation(materials);

    } else if (materials > 0) {
        UpdateRawAllocation(AllocMenu_Optimize(&*complex, CARGO_MATERIALS, materials, CARGO_MATERIALS));
    }
}

void AllocMenu::OnClickFuelBar() {
    int32_t materials;

    materials = GetHorizontalBarClickValue(window.id, capacity.fuel) - production.fuel;

    if (production.free_capacity < materials) {
        materials = production.free_capacity;
    }

    if (materials < 0) {
        AllocMenu_ReduceProduction(&*complex, CARGO_FUEL, -materials);
        UpdateFuelAllocation(materials);

    } else if (materials > 0) {
        UpdateFuelAllocation(AllocMenu_Optimize(&*complex, CARGO_FUEL, materials, CARGO_FUEL));
    }
}

void AllocMenu::OnClickGoldBar() {
    int32_t materials;

    materials = GetHorizontalBarClickValue(window.id, capacity.gold) - production.gold;

    if (production.free_capacity < materials) {
        materials = production.free_capacity;
    }

    if (materials < 0) {
        AllocMenu_ReduceProduction(&*complex, CARGO_GOLD, -materials);
        UpdateGoldAllocation(materials);

    } else if (materials > 0) {
        UpdateGoldAllocation(AllocMenu_Optimize(&*complex, CARGO_GOLD, materials, CARGO_GOLD));
    }
}

void AllocMenu::Run() {
    bool exit_loop = false;
    bool event_release = false;

    while (!exit_loop) {
        int32_t key = get_input();

        if (key > 0 && key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        if (GameManager_RequestMenuExit || unit->GetOrder() == ORDER_DISABLE) {
            key = GNW_KB_KEY_KP_ENTER;
        }

        switch (key) {
            case GNW_KB_KEY_ESCAPE:
            case GNW_KB_KEY_KP_ENTER: {
                exit_loop = true;
            } break;

            case GNW_KB_KEY_LALT_P: {
                PauseMenu_Menu();
            } break;

            case GNW_KB_KEY_SHIFT_DIVIDE:
            case 1000: {
                HelpMenu_Menu(HELPMENU_ALLOCATE_SETUP, WINDOW_MAIN_WINDOW);
            } break;

            case 1001: {
                if (production.free_capacity && production.raw < cargo_2.raw) {
                    UpdateRawAllocation(AllocMenu_Optimize(&*complex, CARGO_MATERIALS, 1, CARGO_MATERIALS));
                }
            } break;

            case 1002: {
                if (production.raw) {
                    AllocMenu_ReduceProduction(&*complex, CARGO_MATERIALS, 1);
                    UpdateRawAllocation(-1);
                }
            } break;

            case 1003: {
                OnClickRawBar();
            } break;

            case 1004: {
                if (production.free_capacity && production.fuel < cargo_2.fuel) {
                    UpdateFuelAllocation(AllocMenu_Optimize(&*complex, CARGO_FUEL, 1, CARGO_FUEL));
                }
            } break;

            case 1005: {
                if (production.fuel) {
                    AllocMenu_ReduceProduction(&*complex, CARGO_FUEL, 1);
                    UpdateFuelAllocation(-1);
                }
            } break;

            case 1006: {
                OnClickFuelBar();
            } break;

            case 1007: {
                if (production.free_capacity && production.gold < cargo_2.gold) {
                    UpdateGoldAllocation(AllocMenu_Optimize(&*complex, CARGO_GOLD, 1, CARGO_GOLD));
                }
            } break;

            case 1008: {
                if (production.gold) {
                    AllocMenu_ReduceProduction(&*complex, CARGO_GOLD, 1);
                    UpdateGoldAllocation(-1);
                }
            } break;

            case 1009: {
                OnClickGoldBar();
            } break;

            default: {
                if (key >= GNW_INPUT_PRESS) {
                    if (!event_release) {
                        buttons[key - GNW_INPUT_PRESS]->PlaySound();
                    }

                    event_release = true;
                }
            } break;
        }

        GameManager_ProcessState(false);
    }

    Deinit();

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_11(unit->team, unit->GetComplex());
    }

    GameManager_EnableMainMenu(&*unit);
    GameManager_ProcessTick(true);
}

void AllocMenu_Menu(UnitInfo *unit) { AllocMenu(unit).Run(); }
