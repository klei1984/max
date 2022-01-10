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
#include "text.hpp"
#include "units_manager.hpp"

void TransferMenu_GetUnitCargoInfo(UnitInfo *source_unit, UnitInfo *destination_unit, short &materials,
                                   short &capacity) {
    Complex *complex;

    complex = source_unit->GetComplex();

    if (complex && complex != destination_unit->GetComplex()) {
        Cargo complex_materials;
        Cargo complex_capacity;

        source_unit->GetComplex()->GetCargoInfo(complex_materials, complex_capacity);

        switch (UnitsManager_BaseUnits[source_unit->unit_type].cargo_type) {
            case MATERIALS:
                materials = complex_materials.raw;
                capacity = complex_capacity.raw;
                break;
            case FUEL:
                materials = complex_materials.fuel;
                capacity = complex_capacity.fuel;
                break;
            case GOLD:
                materials = complex_materials.gold;
                capacity = complex_capacity.gold;
                break;
            default:
                SDL_assert(0);
                break;
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

    text_font(5);
    snprintf(total, sizeof(total), "%ld", labs(total_materials_transferred));
    snprintf(source, sizeof(source), "%ld", labs(unit1_materials));
    snprintf(target, sizeof(target), "%ld", labs(unit2_materials));

    xfer_amount->Write(&window);
    Text_TextBox(reinterpret_cast<char *>(window.buffer), window.width, total, 141, 15, 29, 20, 0xFF, true);

    source_cargo->Write(&window);
    Text_TextBox(reinterpret_cast<char *>(window.buffer), window.width, source, 28, 28, 60, 16, 0xFF, true);

    target_cargo->Write(&window);
    Text_TextBox(reinterpret_cast<char *>(window.buffer), window.width, target, 223, 28, 60, 16, 0xFF, true);

    button_arrow->Enable();
    button_arrow->SetRestState(total_materials_transferred >= 0);
    button_arrow->Disable();

    win_draw_rect(window.id, &window.window);
}

bool TransferMenu::ProcessKey(int key) {
    if (key > 0 && key < 0x7000) {
        event_release = false;
    }

    if (key == 0x0D) {
        event_click_done = true;
    } else if (key == 0x1B) {
        total_materials_transferred = 0;
        event_click_done = true;
    } else if (key == 0x3F) {
        /// \todo Implement functions
        //            HelpMenu_Menu(15, 38);
    } else if (key == 0x119) {
        //            PauseMenu_Menu();
    } else if (key < 0x7000) {
        if (scrollbar->ProcessKey(key)) {
            int old_value = total_materials_transferred;
            total_materials_transferred = scrollbar->GetValue();

            if (old_value != total_materials_transferred) {
                button_right->PlaySound();
                UpdateIndicators();
            }
        }
    } else if (!event_release) {
        event_release = true;

        if (key == 0x700D) {
            button_done->PlaySound();
        } else if (key == 0x701B) {
            button_cancel->PlaySound();
        } else if (key == 0x703F) {
            button_help->PlaySound();
        }
    }

    return true;
}

TransferMenu::TransferMenu(UnitInfo *unit) : Window(XFERPIC, 38) {
    short source_unit_capacity;
    short target_unit_capacity;

    team = unit->team;

    source_unit = unit;
    target_unit = unit->GetParent();

    event_click_done = false;
    event_release = false;

    Cursor_SetCursor(CURSOR_HAND);
    text_font(5);

    TransferMenu_GetUnitCargoInfo(source_unit, target_unit, unit1_materials, source_unit_capacity);
    TransferMenu_GetUnitCargoInfo(target_unit, source_unit, unit2_materials, target_unit_capacity);

    total_materials_transferred = target_unit_capacity - unit2_materials;
    total_materials_transferred = std::min(total_materials_transferred, unit1_materials);

    unit1_free_capacity = source_unit_capacity - unit1_materials;
    unit1_free_capacity = std::min(unit1_free_capacity, unit2_materials);

    switch (UnitsManager_BaseUnits[source_unit->unit_type].cargo_type) {
        case MATERIALS:
            material_id = SMBRRAW;
            break;
        case FUEL:
            material_id = SMBRFUEL;
            break;
        case GOLD:
            material_id = SMBRGOLD;
            break;
        default:
            SDL_assert(0);
            break;
    }

    unit2_free_capacity = total_materials_transferred;

    Add();
    FillWindowInfo(&window);
    /// \todo Implement function
    //    disable_main_menu();

    xfer_amount = new (std::nothrow) Image(141, 15, 29, 20);
    xfer_amount->Copy(&window);

    source_cargo = new (std::nothrow) Image(28, 28, 60, 16);
    source_cargo->Copy(&window);

    target_cargo = new (std::nothrow) Image(223, 28, 60, 16);
    target_cargo->Copy(&window);

    Rect slider_bounds = {44, 89, 267, 105};
    Rect value_bounds = {141, 15, 170, 35};
    scrollbar =
        new (std::nothrow) LimitedScrollbar(this, &slider_bounds, &value_bounds, SMBRRAW, 0x3E8, 0x3E9, 0x3EA, 0);

    button_right = new (std::nothrow) Button(XFRRGT_U, XFRRGT_D, 278, 89);
    button_right->SetPValue(0x3E8);
    button_right->SetSfx(KCARG0);
    button_right->RegisterButton(window.id);

    button_left = new (std::nothrow) Button(XFRLFT_U, XFRLFT_D, 17, 89);
    button_left->SetPValue(0x3E9);
    button_left->SetSfx(KCARG0);
    button_left->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(XFRCAN_U, XFRCAN_D, 82, 125);
    button_cancel->SetCaption("Cancel", 1, 2);
    button_cancel->SetRValue(0x1B);
    button_cancel->SetPValue(0x701B);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    button_done = new (std::nothrow) Button(XFRDNE_U, XFRDNE_D, 174, 125);
    button_done->SetCaption("Done", 1, 2);
    button_done->SetRValue(0x0D);
    button_done->SetPValue(0x700D);
    button_done->SetSfx(NDONE0);
    button_done->RegisterButton(window.id);

    button_help = new (std::nothrow) Button(XFRHLP_U, XFRHLP_D, 147, 125);
    button_help->SetCaption("Done", 1, 2);
    button_help->SetRValue(0x3F);
    button_help->SetPValue(0x703F);
    button_help->SetSfx(NHELP0);
    button_help->RegisterButton(window.id);

    button_arrow = new (std::nothrow) Button(XFRLFARO, XFRRTARO, 141, 43);
    button_help->SetFlags(0x01);
    button_help->RegisterButton(window.id);
    button_help->CopyUpDisabled(XFRLFARO);
    button_help->CopyDownDisabled(XFRRTARO);

    scrollbar->Register();
    scrollbar->SetFreeCapacity(target_unit_capacity - unit2_materials);
    scrollbar->SetZeroOffset(-unit2_materials);
    scrollbar->SetXferGiveMax(unit2_free_capacity);
    scrollbar->SetXferTakeMax(-unit1_free_capacity);
    scrollbar->SetValue(unit2_free_capacity);
    scrollbar->SetMaterialBar(material_id);
    scrollbar->RefreshScreen();

    /// \todo Implement functions
    //    sub_CB6AF(window.buffer, width, source_unit->unit_type, GUI_PlayerTeamIndex, 104, 36);

    Text_TextBox(reinterpret_cast<char *>(window.buffer), window.width,
                 UnitsManager_BaseUnits[source_unit->unit_type].singular_name, 10, 52, 110, 30, 0, true);

    //    sub_CB6AF(window.buffer, width, unit2->unit_type, GUI_PlayerTeamIndex, 207, 36);

    Text_TextBox(reinterpret_cast<char *>(window.buffer), window.width,
                 UnitsManager_BaseUnits[target_unit->unit_type].singular_name, 191, 52, 110, 30, 0, true);

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

    delete scrollbar;

    /// \todo Implement functions
    //    enable_main_menu(*selected_unit);
    //    sub_A0EFE(1);
}

void TransferMenu::Run() {
    event_click_done = false;

    while (!event_click_done) {
        int key = get_input();

        /// \todo Implement missing stuff
        if (/* byte_1737D2 || */ source_unit->orders == ORDER_DISABLED || target_unit->orders == ORDER_DISABLED
            /* || source_unit->team != GUI_PlayerTeamIndex || target_unit->team != GUI_PlayerTeamIndex */) {
            key = 0x1B;
        }

        ProcessKey(key);
        // sub_A0E32(1);
    }
}

short TransferMenu::GetCargoTransferred() const { return total_materials_transferred; }
