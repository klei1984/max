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

#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "window_manager.hpp"

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

static struct MenuTitleItem choose_player_menu_titles[] = {
    MENU_TITLE_ITEM_DEF(230, 6, 410, 26, nullptr),  MENU_TITLE_ITEM_DEF(61, 31, 141, 51, _(5985)),
    MENU_TITLE_ITEM_DEF(162, 31, 241, 51, _(9ffa)), MENU_TITLE_ITEM_DEF(272, 31, 351, 51, _(76ed)),
    MENU_TITLE_ITEM_DEF(380, 31, 459, 51, _(dc93)), MENU_TITLE_ITEM_DEF(497, 31, 576, 51, _(bda5)),
};

static struct ChoosePlayerMenuControlItem choose_player_menu_controls[] = {
    MENU_CONTROL_DEF(175, 67, 0, 0, CH_HUM_U, nullptr, 0, &ChoosePlayerMenu::EventSelectHuman, NHUMN0),
    MENU_CONTROL_DEF(175, 159, 0, 0, CH_HUM_U, nullptr, 0, &ChoosePlayerMenu::EventSelectHuman, NHUMN0),
    MENU_CONTROL_DEF(175, 251, 0, 0, CH_HUM_U, nullptr, 0, &ChoosePlayerMenu::EventSelectHuman, NHUMN0),
    MENU_CONTROL_DEF(175, 343, 0, 0, CH_HUM_U, nullptr, 0, &ChoosePlayerMenu::EventSelectHuman, NHUMN0),
    MENU_CONTROL_DEF(285, 67, 0, 0, CH_CMP_U, nullptr, 0, &ChoosePlayerMenu::EventSelectComputer, NCOMP0),
    MENU_CONTROL_DEF(285, 159, 0, 0, CH_CMP_U, nullptr, 0, &ChoosePlayerMenu::EventSelectComputer, NCOMP0),
    MENU_CONTROL_DEF(285, 251, 0, 0, CH_CMP_U, nullptr, 0, &ChoosePlayerMenu::EventSelectComputer, NCOMP0),
    MENU_CONTROL_DEF(285, 343, 0, 0, CH_CMP_U, nullptr, 0, &ChoosePlayerMenu::EventSelectComputer, NCOMP0),
    MENU_CONTROL_DEF(394, 67, 0, 0, CH_NON_U, nullptr, 0, &ChoosePlayerMenu::EventSelectNone, NNONE0),
    MENU_CONTROL_DEF(394, 159, 0, 0, CH_NON_U, nullptr, 0, &ChoosePlayerMenu::EventSelectNone, NNONE0),
    MENU_CONTROL_DEF(394, 251, 0, 0, CH_NON_U, nullptr, 0, &ChoosePlayerMenu::EventSelectNone, NNONE0),
    MENU_CONTROL_DEF(394, 343, 0, 0, CH_NON_U, nullptr, 0, &ChoosePlayerMenu::EventSelectNone, NNONE0),
    MENU_CONTROL_DEF(495, 61, 0, 0, CH_TM1_U, nullptr, 0, &ChoosePlayerMenu::EventSelectClan, NCLAN0),
    MENU_CONTROL_DEF(495, 153, 0, 0, CH_TM2_U, nullptr, 0, &ChoosePlayerMenu::EventSelectClan, NCLAN0),
    MENU_CONTROL_DEF(495, 245, 0, 0, CH_TM3_U, nullptr, 0, &ChoosePlayerMenu::EventSelectClan, NCLAN0),
    MENU_CONTROL_DEF(495, 337, 0, 0, CH_TM4_U, nullptr, 0, &ChoosePlayerMenu::EventSelectClan, NCLAN0),
    MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, _(4042), GNW_KB_KEY_ESCAPE, &ChoosePlayerMenu::EventCancel, NCANC0),
    MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, _(9605), GNW_KB_KEY_SHIFT_DIVIDE, &ChoosePlayerMenu::EventHelp, NHELP0),
    MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(ef0a), GNW_KB_KEY_RETURN, &ChoosePlayerMenu::EventDone, NDONE0),
};

static_assert(CHOOSE_PLAYER_MENU_ITEM_COUNT ==
              sizeof(choose_player_menu_controls) / sizeof(ChoosePlayerMenuControlItem));

void ChoosePlayerMenu::ButtonSetState(int team, int rest_state) {
    int team_player_ini_setting;

    team_player_ini_setting = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team)) - 1;

    if (team_player_ini_setting < 0) {
        team_player_ini_setting = TEAM_TYPE_COMPUTER;
    }

    buttons[team_player_ini_setting * 4 + team]->SetRestState(rest_state);
}

void ChoosePlayerMenu::Init() {
    const int team_count = 4;
    const int num_buttons = 3;
    ButtonID button_list[num_buttons];

    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    event_click_done = false;
    event_click_cancel = false;

    mouse_hide();
    WindowManager_LoadBigImage(CHOSPLYR, window, window->width, false, false, -1, -1, true);

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

void ChoosePlayerMenu::EventSelectHuman() {
    if (game_type) {
        for (int i = 0; i < PLAYER_TEAM_ALIEN; ++i) {
            if (ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + i)) == TEAM_TYPE_PLAYER) {
                ButtonSetState(i, 0);
                ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + i),
                                ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + key_press)));
                ButtonSetState(i, 1);
                break;
            }
        }
    }

    ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + key_press), TEAM_TYPE_PLAYER);
    UpdateButtons();
}

void ChoosePlayerMenu::EventSelectComputer() {
    ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + key_press - 4), TEAM_TYPE_COMPUTER);
    UpdateButtons();
}

void ChoosePlayerMenu::EventSelectNone() {
    ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + key_press - 8), TEAM_TYPE_NONE);
    UpdateButtons();
}

void ChoosePlayerMenu::EventSelectClan() {
    Deinit();
    menu_clan_select_menu_loop(key_press - 12);
    Init();
}

void ChoosePlayerMenu::EventCancel() { event_click_cancel = true; }

void ChoosePlayerMenu::EventHelp() {
    HelpMenu_Menu(game_type == 0 ? HELPMENU_HOT_SEAT_SETUP : HELPMENU_NEW_GAME_SETUP, WINDOW_MAIN_WINDOW);
}

void ChoosePlayerMenu::ButtonInit(int index, int mode) {
    ChoosePlayerMenuControlItem* control;

    control = &choose_player_menu_controls[index];

    Text_SetFont(GNW_TEXT_FONT_1);

    if (index >= 12 && index < 16) {
        ResourceID image_id;
        ResourceID clan_logo_id;

        image_id = static_cast<ResourceID>(control->image_id + 1);
        clan_logo_id = CLN0LOGO;

        if (ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + index - 12)) == TEAM_TYPE_PLAYER) {
            if (ini_get_setting(INI_GAME_FILE_TYPE) != GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                image_id = control->image_id;
            }

            if (game_type || ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                clan_logo_id = static_cast<ResourceID>(
                    CLN0LOGO + ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + index - 12)));
            }
        }

        buttons[index] = new (std::nothrow) Button(image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   WindowManager_ScaleUlx(window, control->bounds.ulx),
                                                   WindowManager_ScaleUly(window, control->bounds.uly));

        buttons[index]->Copy(clan_logo_id, 41, 40);
    } else if (control->image_id == INVALID_ID) {
        buttons[index] = new (std::nothrow) Button(
            WindowManager_ScaleUlx(window, control->bounds.ulx), WindowManager_ScaleUly(window, control->bounds.uly),
            control->bounds.lrx - control->bounds.ulx, control->bounds.lry - control->bounds.uly);
    } else {
        buttons[index] = new (std::nothrow) Button(control->image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   WindowManager_ScaleUlx(window, control->bounds.ulx),
                                                   WindowManager_ScaleUly(window, control->bounds.uly));

        if (control->label) {
            buttons[index]->SetCaption(control->label);
        }
    }

    if (mode) {
        buttons[index]->SetFlags(5);
        buttons[index]->SetPValue(1000 + index);
    } else {
        buttons[index]->SetFlags(0);
        buttons[index]->SetRValue(1000 + index);
        buttons[index]->SetPValue(GNW_INPUT_PRESS + index);
    }

    buttons[index]->SetSfx(control->sfx);
    buttons[index]->RegisterButton(window->id);

    menu_item[index].r_value = 1000 + index;
    menu_item[index].event_code = control->event_code;
    menu_item[index].event_handler = control->event_handler;
}

void ChoosePlayerMenu::UpdateButtons() {
    char buffer[100];

    for (int i = 12; i < 16; ++i) {
        delete buttons[i];

        ButtonInit(i, false);
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    if (game_type) {
        strcpy(buffer, _(5512));
    } else {
        strcpy(buffer, _(55c8));
    }

    choose_player_menu_titles[0].title = buffer;

    for (int i = 0; i < 6; ++i) {
        menu_draw_menu_title(window, &choose_player_menu_titles[i], COLOR_GREEN, true);
    }

    for (int i = 0; i < CHOOSE_PLAYER_MENU_ITEM_COUNT; ++i) {
        buttons[i]->Enable();

        if (i >= 12 && i < 16 &&
            (ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + i - 12)) != TEAM_TYPE_PLAYER ||
             ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_MULTI_PLAYER_SCENARIO)) {
            buttons[i]->Disable();
        }
    }

    win_draw(window->id);
}

void ChoosePlayerMenu::EventDone() {
    int human_player_count;
    int computer_player_count;

    human_player_count = 0;
    computer_player_count = 0;

    for (int i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
        int team_type;

        team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + i));

        if (team_type == TEAM_TYPE_PLAYER) {
            ++human_player_count;
        } else if (team_type == TEAM_TYPE_COMPUTER) {
            ++computer_player_count;
        }
    }

    if (game_type) {
        if (human_player_count + computer_player_count < 2) {
            MessageManager_DrawMessage(_(bc94), 0, 1);
        } else {
            event_click_done = true;
        }

    } else if (human_player_count < 2) {
        MessageManager_DrawMessage(_(06ec), 0, 1);
    } else {
        event_click_done = true;
    }
}
