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
#include "menu.hpp"
#include "message_manager.hpp"
#include "missionmanager.hpp"
#include "settings.hpp"
#include "window_manager.hpp"

void ChoosePlayerMenu::ButtonSetState(int32_t team, int32_t rest_state) {
    int32_t team_player_type = ResourceManager_GetSettings()->GetNumericValue(menu_team_player_setting[team]) - 1;

    if (team_player_type < 0) {
        team_player_type = TEAM_TYPE_COMPUTER;
    }

    buttons[team_player_type * 4 + team]->SetRestState(rest_state);
}

void ChoosePlayerMenu::Init() {
    const int32_t team_count = 4;
    const int32_t num_buttons = 3;
    ButtonID button_list[num_buttons];

    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    event_click_done = false;
    event_click_cancel = false;

    mouse_hide();
    WindowManager_LoadBigImage(CHOSPLYR, window, window->width, false, false, -1, -1, true);

    for (int32_t i = 0; i < CHOOSE_PLAYER_MENU_ITEM_COUNT; ++i) {
        buttons[i] = nullptr;

        ButtonInit(i, i < 12);
    }

    UpdateButtons();

    for (int32_t i = 0; i < team_count; ++i) {
        for (int32_t j = 0; j < num_buttons; ++j) {
            button_list[j] = buttons[j * team_count + i]->GetId();
        }

        win_group_radio_buttons(num_buttons, button_list);
        ButtonSetState(i, 1);
    }

    mouse_show();
}

void ChoosePlayerMenu::Deinit() {
    for (int32_t i = 0; i < CHOOSE_PLAYER_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }
}

void ChoosePlayerMenu::EventSelectHuman() {
    if (is_single_player) {
        for (int32_t i = 0; i < PLAYER_TEAM_ALIEN; ++i) {
            if (ResourceManager_GetSettings()->GetNumericValue(menu_team_player_setting[i]) == TEAM_TYPE_PLAYER) {
                ButtonSetState(i, 0);
                ResourceManager_GetSettings()->SetNumericValue(
                    menu_team_player_setting[i],
                    ResourceManager_GetSettings()->GetNumericValue(menu_team_player_setting[key_press]));
                ButtonSetState(i, 1);
                break;
            }
        }
    }

    ResourceManager_GetSettings()->SetNumericValue(menu_team_player_setting[key_press], TEAM_TYPE_PLAYER);
    UpdateButtons();
}

void ChoosePlayerMenu::EventSelectComputer() {
    ResourceManager_GetSettings()->SetNumericValue(menu_team_player_setting[key_press - 4], TEAM_TYPE_COMPUTER);
    UpdateButtons();
}

void ChoosePlayerMenu::EventSelectNone() {
    ResourceManager_GetSettings()->SetNumericValue(menu_team_player_setting[key_press - 8], TEAM_TYPE_NONE);
    UpdateButtons();
}

void ChoosePlayerMenu::EventSelectClan() {
    Deinit();
    menu_clan_select_menu_loop(key_press - 12);
    Init();
}

void ChoosePlayerMenu::EventCancel() { event_click_cancel = true; }

void ChoosePlayerMenu::EventHelp() {
    HelpMenu_Menu(is_single_player == false ? "HOT_SEAT_SETUP" : "NEW_GAME_SETUP", WINDOW_MAIN_WINDOW);
}

void ChoosePlayerMenu::ButtonInit(int32_t index, int32_t mode) {
    ChoosePlayerMenuControlItem* control;

    control = &choose_player_menu_controls[index];

    Text_SetFont(GNW_TEXT_FONT_1);

    if (index >= 12 && index < 16) {
        ResourceID image_id;
        ResourceID clan_logo_id;

        image_id = static_cast<ResourceID>(control->image_id + 1);
        clan_logo_id = CLN0LOGO;

        if (ResourceManager_GetSettings()->GetNumericValue(menu_team_player_setting[index - 12]) == TEAM_TYPE_PLAYER) {
            const auto mission_category = ResourceManager_GetMissionManager()->GetMission()->GetCategory();

            if (mission_category != MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
                image_id = control->image_id;
            }

            if (is_single_player || mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
                clan_logo_id = static_cast<ResourceID>(
                    CLN0LOGO + ResourceManager_GetSettings()->GetNumericValue(menu_team_clan_setting[index - 12]));
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
    const auto mission_category = ResourceManager_GetMissionManager()->GetMission()->GetCategory();

    for (int32_t i = 12; i < 16; ++i) {
        delete buttons[i];

        ButtonInit(i, false);
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    choose_player_menu_titles[0].title = is_single_player ? _(5512) : _(55c8);

    for (int32_t i = 0; i < 6; ++i) {
        menu_draw_menu_title(window, &choose_player_menu_titles[i], COLOR_GREEN, true);
    }

    for (int32_t i = 0; i < CHOOSE_PLAYER_MENU_ITEM_COUNT; ++i) {
        buttons[i]->Enable();

        if (i >= 12 && i < 16 &&
            (ResourceManager_GetSettings()->GetNumericValue(menu_team_player_setting[i - 12]) != TEAM_TYPE_PLAYER ||
             mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO)) {
            buttons[i]->Disable();
        }
    }

    win_draw(window->id);
}

void ChoosePlayerMenu::EventDone() {
    int32_t human_player_count;
    int32_t computer_player_count;

    human_player_count = 0;
    computer_player_count = 0;

    for (int32_t i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
        int32_t team_type;

        team_type = ResourceManager_GetSettings()->GetNumericValue(menu_team_player_setting[i]);

        if (team_type == TEAM_TYPE_PLAYER) {
            ++human_player_count;
        } else if (team_type == TEAM_TYPE_COMPUTER) {
            ++computer_player_count;
        }
    }

    if (is_single_player) {
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
