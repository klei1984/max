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

#include "chooseplayermenu.hpp"

#include "gwindow.hpp"
#include "helpmenu.hpp"

struct ChoosePlayerMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char* label;
    int event_code;
    void (ChoosePlayerMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    { {(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx) }

static struct ChoosePlayerMenuControlItem choose_player_menu_controls[] = {
    MENU_CONTROL_DEF(175, 67, 0, 0, CH_HUM_U, nullptr, 0, ChoosePlayerMenu::EventSelectHuman, NHUMN0),
    MENU_CONTROL_DEF(175, 159, 0, 0, CH_HUM_U, nullptr, 0, ChoosePlayerMenu::EventSelectHuman, NHUMN0),
    MENU_CONTROL_DEF(175, 251, 0, 0, CH_HUM_U, nullptr, 0, ChoosePlayerMenu::EventSelectHuman, NHUMN0),
    MENU_CONTROL_DEF(175, 343, 0, 0, CH_HUM_U, nullptr, 0, ChoosePlayerMenu::EventSelectHuman, NHUMN0),
    MENU_CONTROL_DEF(285, 67, 0, 0, CH_CMP_U, nullptr, 0, ChoosePlayerMenu::EventSelectComputer, NCOMP0),
    MENU_CONTROL_DEF(285, 159, 0, 0, CH_CMP_U, nullptr, 0, ChoosePlayerMenu::EventSelectComputer, NCOMP0),
    MENU_CONTROL_DEF(285, 251, 0, 0, CH_CMP_U, nullptr, 0, ChoosePlayerMenu::EventSelectComputer, NCOMP0),
    MENU_CONTROL_DEF(285, 343, 0, 0, CH_CMP_U, nullptr, 0, ChoosePlayerMenu::EventSelectComputer, NCOMP0),
    MENU_CONTROL_DEF(394, 67, 0, 0, CH_NON_U, nullptr, 0, ChoosePlayerMenu::EventSelectNone, NNONE0),
    MENU_CONTROL_DEF(394, 159, 0, 0, CH_NON_U, nullptr, 0, ChoosePlayerMenu::EventSelectNone, NNONE0),
    MENU_CONTROL_DEF(394, 251, 0, 0, CH_NON_U, nullptr, 0, ChoosePlayerMenu::EventSelectNone, NNONE0),
    MENU_CONTROL_DEF(394, 343, 0, 0, CH_NON_U, nullptr, 0, ChoosePlayerMenu::EventSelectNone, NNONE0),
    MENU_CONTROL_DEF(495, 61, 0, 0, CH_TM1_U, nullptr, 0, ChoosePlayerMenu::EventSelectClan, NCLAN0),
    MENU_CONTROL_DEF(495, 153, 0, 0, CH_TM2_U, nullptr, 0, ChoosePlayerMenu::EventSelectClan, NCLAN0),
    MENU_CONTROL_DEF(495, 245, 0, 0, CH_TM3_U, nullptr, 0, ChoosePlayerMenu::EventSelectClan, NCLAN0),
    MENU_CONTROL_DEF(495, 337, 0, 0, CH_TM4_U, nullptr, 0, ChoosePlayerMenu::EventSelectClan, NCLAN0),
    MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, "Cancel", GNW_KB_KEY_ESCAPE, ChoosePlayerMenu::EventCancel, NCANC0),
    MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, "?", GNW_KB_KEY_SHIFT_DIVIDE, ChoosePlayerMenu::EventHelp, NHELP0),
    MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, "Done", GNW_KB_KEY_RETURN, ChoosePlayerMenu::EventDone, NDONE0),
};

static_assert(CHOOSE_PLAYER_MENU_ITEM_COUNT ==
              sizeof(choose_player_menu_controls) / sizeof(ChoosePlayerMenuControlItem));

void ChoosePlayerMenu::ButtonSetState(int team, int rest_state) {}

void ChoosePlayerMenu::Init() {
    ButtonID button_list[3];
    const int team_count = 4;
    const int num_buttons = 3;

    window = gwin_get_window(GWINDOW_MAIN_WINDOW);
    event_click_done = false;
    event_click_cancel = false;

    mouse_hide();
    gwin_load_image(CHOSPLYR, window, window->width, false, false);

    for (int i = 0; i < CHOOSE_PLAYER_MENU_ITEM_COUNT; ++i) {
        buttons[i] = nullptr;

        ButtonInit(i, i < 12);
    }

    UpdateButtons();

    for (int i = 0; i < team_count; ++i) {
        for (int j = 0; j < num_buttons; ++j) {
            button_list[j] = buttons[j * team_count + i]->GetId();
        }

        win_group_radio_buttons(num_buttons, button_list);
        ButtonSetState(i, 1);
    }

    mouse_show();
}

void ChoosePlayerMenu::Deinit() {
    for (int i = 0; i < CHOOSE_PLAYER_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }
}

void ChoosePlayerMenu::EventSelectHuman() {}

void ChoosePlayerMenu::EventSelectComputer() {}

void ChoosePlayerMenu::EventSelectNone() {}

void ChoosePlayerMenu::EventSelectClan() {
    Deinit();
    menu_clan_select_menu_loop(key_press - 12);
    Init();
}

void ChoosePlayerMenu::EventCancel() { event_click_cancel = true; }

void ChoosePlayerMenu::EventHelp() {
    HelpMenu_Menu(game_type == 0 ? HELPMENU_HOT_SEAT_SETUP : HELPMENU_NEW_GAME_SETUP, GWINDOW_MAIN_WINDOW);
}

void ChoosePlayerMenu::EventDone() {}
