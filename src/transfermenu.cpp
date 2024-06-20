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

#include "transfermenu.hpp"

#include <algorithm>

#include "cargo.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "reportstats.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

void TransferMenu_GetUnitCargoInfo(UnitInfo *source_unit, UnitInfo *destination_unit, int16_t &materials,
                                   int16_t &capacity) {
    Complex *complex;

    complex = source_unit->GetComplex();

    if (complex && complex != destination_unit->GetComplex()) {
        Cargo complex_materials;
        Cargo complex_capacity;

        source_unit->GetComplex()->GetCargoInfo(complex_materials, complex_capacity);

        switch (UnitsManager_BaseUnits[source_unit->GetUnitType()].cargo_type) {
            case MATERIALS: {
                materials = complex_materials.raw;
                capacity = complex_capacity.raw;
            } break;

            case FUEL: {
                materials = complex_materials.fuel;
                capacity = complex_capacity.fuel;
            } break;

            case GOLD: {
                materials = complex_materials.gold;
                capacity = complex_capacity.gold;
            } break;

            default: {
                SDL_assert(0);
            } break;
        }
    } else {
        materials = source_unit->storage;
        capacity = source_unit->GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
    }
}

void TransferMenu::UpdateIndicators() {
    char total[8];
    char source[8];
    char target[8];

    Text_SetFont(GNW_TEXT_FONT_5);
    snprintf(total, sizeof(total), "%ld", labs(total_materials_transferred));
    snprintf(source, sizeof(source), "%ld", labs(source_unit_materials - total_materials_transferred));
    snprintf(target, sizeof(target), "%ld", labs(target_unit_materials + total_materials_transferred));

    xfer_amount->Write(&window);
    Text_TextBox(window.buffer, window.width, total, 141, 15, 29, 20, 0xFF, true);

    source_cargo->Write(&window);
    Text_TextBox(window.buffer, window.width, source, 28, 28, 60, 16, 0xFF, true);

    target_cargo->Write(&window);
    Text_TextBox(window.buffer, window.width, target, 223, 28, 60, 16, 0xFF, true);

    button_arrow->Enable();
    button_arrow->SetRestState(total_materials_transferred >= 0);
    button_arrow->Disable();

    win_draw_rect(window.id, &window.window);
}

bool TransferMenu::ProcessKey(int32_t key) {
    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_release = false;
    }

    if (key == GNW_KB_KEY_RETURN) {
        event_click_done = true;

    } else if (key == GNW_KB_KEY_ESCAPE) {
        total_materials_transferred = 0;
        event_click_done = true;

    } else if (key == GNW_KB_KEY_SHIFT_DIVIDE) {
        HelpMenu_Menu(HELPMENU_TRANSFER_SETUP, WINDOW_MAIN_MAP);

    } else if (key == GNW_KB_KEY_LALT_P) {
        PauseMenu_Menu();

    } else if (key < GNW_INPUT_PRESS) {
        if (scrollbar->ProcessKey(key)) {
            int32_t old_value = total_materials_transferred;
            total_materials_transferred = scrollbar->GetValue();

            if (old_value != total_materials_transferred) {
                button_right->PlaySound();
                UpdateIndicators();
            }
        }

    } else if (!event_release) {
        event_release = true;
        key -= GNW_INPUT_PRESS;

        if (key == GNW_KB_KEY_RETURN) {
            button_done->PlaySound();

        } else if (key == GNW_KB_KEY_ESCAPE) {
            button_cancel->PlaySound();

        } else if (key == GNW_KB_KEY_SHIFT_DIVIDE) {
            button_help->PlaySound();
        }
    }

    return true;
}

TransferMenu::TransferMenu(UnitInfo *unit) : Window(XFERPIC, GameManager_GetDialogWindowCenterMode()) {
    int16_t source_unit_capacity;
    int16_t target_unit_capacity;

    team = unit->team;

    source_unit = unit;
    target_unit = unit->GetParent();

    event_click_done = false;
    event_release = false;

    Cursor_SetCursor(CURSOR_HAND);
    Text_SetFont(GNW_TEXT_FONT_5);

    TransferMenu_GetUnitCargoInfo(source_unit, target_unit, source_unit_materials, source_unit_capacity);
    TransferMenu_GetUnitCargoInfo(target_unit, source_unit, target_unit_materials, target_unit_capacity);

    total_materials_transferred = target_unit_capacity - target_unit_materials;
    total_materials_transferred = std::min(total_materials_transferred, source_unit_materials);

    source_unit_free_capacity = source_unit_capacity - source_unit_materials;
    source_unit_free_capacity = std::min(source_unit_free_capacity, target_unit_materials);

    switch (UnitsManager_BaseUnits[source_unit->GetUnitType()].cargo_type) {
        case MATERIALS: {
            material_id = SMBRRAW;
        } break;

        case FUEL: {
            material_id = SMBRFUEL;
        } break;

        case GOLD: {
            material_id = SMBRGOLD;
        } break;

        default: {
            SDL_assert(0);
        } break;
    }

    target_unit_free_capacity = total_materials_transferred;

    Add();
    FillWindowInfo(&window);
    GameManager_DisableMainMenu();

    xfer_amount = new (std::nothrow) Image(141, 15, 29, 20);
    xfer_amount->Copy(&window);

    source_cargo = new (std::nothrow) Image(28, 28, 60, 16);
    source_cargo->Copy(&window);

    target_cargo = new (std::nothrow) Image(223, 28, 60, 16);
    target_cargo->Copy(&window);

    Rect slider_bounds = {44, 89, 267, 105};
    Rect value_bounds = {141, 15, 170, 35};
    scrollbar = new (std::nothrow) LimitedScrollbar(this, &slider_bounds, &value_bounds, SMBRRAW, 1000, 1001, 1002, 0);

    button_right = new (std::nothrow) Button(XFRRGT_U, XFRRGT_D, 278, 89);
    button_right->SetPValue(1000);
    button_right->SetSfx(KCARG0);
    button_right->RegisterButton(window.id);

    button_left = new (std::nothrow) Button(XFRLFT_U, XFRLFT_D, 17, 89);
    button_left->SetPValue(1001);
    button_left->SetSfx(KCARG0);
    button_left->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(XFRCAN_U, XFRCAN_D, 82, 125);
    button_cancel->SetCaption(_(36f9), 1, 2);
    button_cancel->SetRValue(GNW_KB_KEY_CTRL_ESCAPE);
    button_cancel->SetPValue(GNW_KB_KEY_CTRL_ESCAPE + GNW_INPUT_PRESS);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    button_done = new (std::nothrow) Button(XFRDNE_U, XFRDNE_D, 174, 125);
    button_done->SetCaption(_(c366), 1, 2);
    button_done->SetRValue(GNW_KB_KEY_KP_ENTER);
    button_done->SetPValue(GNW_KB_KEY_KP_ENTER + GNW_INPUT_PRESS);
    button_done->SetSfx(NDONE0);
    button_done->RegisterButton(window.id);

    button_help = new (std::nothrow) Button(XFRHLP_U, XFRHLP_D, 147, 125);
    button_help->SetRValue(GNW_KB_KEY_SHIFT_DIVIDE);
    button_help->SetPValue(GNW_KB_KEY_SHIFT_DIVIDE + GNW_INPUT_PRESS);
    button_help->SetSfx(NHELP0);
    button_help->RegisterButton(window.id);

    button_arrow = new (std::nothrow) Button(XFRLFARO, XFRRTARO, 141, 43);
    button_arrow->SetFlags(0x01);
    button_arrow->RegisterButton(window.id);
    button_arrow->CopyUpDisabled(XFRLFARO);
    button_arrow->CopyDownDisabled(XFRRTARO);

    scrollbar->Register();
    scrollbar->SetFreeCapacity(target_unit_capacity - target_unit_materials);
    scrollbar->SetZeroOffset(-target_unit_materials);
    scrollbar->SetXferGiveMax(target_unit_free_capacity);
    scrollbar->SetXferTakeMax(-source_unit_free_capacity);
    scrollbar->SetValue(target_unit_free_capacity);
    scrollbar->SetMaterialBar(material_id);
    scrollbar->RefreshScreen();

    ReportStats_DrawListItemIcon(window.buffer, window.width, source_unit->GetUnitType(), GameManager_PlayerTeam, 104,
                                 36);

    Text_TextBox(window.buffer, window.width, UnitsManager_BaseUnits[source_unit->GetUnitType()].singular_name, 10, 52,
                 110, 30, COLOR_BLACK, true);

    ReportStats_DrawListItemIcon(window.buffer, window.width, target_unit->GetUnitType(), GameManager_PlayerTeam, 207,
                                 36);

    Text_TextBox(window.buffer, window.width, UnitsManager_BaseUnits[target_unit->GetUnitType()].singular_name, 191, 52,
                 110, 30, COLOR_BLACK, true);

    UpdateIndicators();
}

TransferMenu::~TransferMenu() {
    delete xfer_amount;
    delete source_cargo;
    delete target_cargo;

    delete button_right;
    delete button_left;
    delete button_cancel;
    delete button_done;
    delete button_help;
    delete button_arrow;

    delete scrollbar;

    GameManager_EnableMainMenu(&*GameManager_SelectedUnit);
    GameManager_ProcessTick(true);
}

void TransferMenu::Run() {
    event_click_done = false;

    while (!event_click_done) {
        int32_t key = get_input();

        if (GameManager_RequestMenuExit || source_unit->GetOrder() == ORDER_DISABLE ||
            target_unit->GetOrder() == ORDER_DISABLE || source_unit->team != GameManager_PlayerTeam ||
            target_unit->team != GameManager_PlayerTeam) {
            key = GNW_KB_KEY_ESCAPE;
        }

        ProcessKey(key);
        GameManager_ProcessState(true);
    }
}

int16_t TransferMenu::GetCargoTransferred() const { return total_materials_transferred; }

int32_t TransferMenu_Menu(UnitInfo *unit) {
    TransferMenu menu(unit);

    menu.Run();

    return menu.GetCargoTransferred();
}
