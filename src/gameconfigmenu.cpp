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

#include "gameconfigmenu.hpp"

#include <format>

#include "helpmenu.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "text.hpp"
#include "window_manager.hpp"

static void DrawCaption(WindowInfo* window, MenuTitleItem* menu_item, FontColor color = Fonts_BrightYellowColor,
                        bool horizontal_align = false) {
    if (menu_item->title.length() > 0) {
        Text_TextBox(window, menu_item->title.c_str(), WindowManager_ScaleUlx(window, menu_item->bounds.ulx),
                     WindowManager_ScaleUly(window, menu_item->bounds.uly),
                     menu_item->bounds.lrx - menu_item->bounds.ulx, menu_item->bounds.lry - menu_item->bounds.uly,
                     horizontal_align, true, color);
    }
}

static void DrawSelector(WindowInfo* window, MenuTitleItem* menu_item, int32_t color) {
    draw_box(window->buffer, window->width, WindowManager_ScaleUlx(window, menu_item->bounds.ulx),
             WindowManager_ScaleUly(window, menu_item->bounds.uly),
             WindowManager_ScaleUlx(window, menu_item->bounds.lrx),
             WindowManager_ScaleUly(window, menu_item->bounds.lry), color);
}

void GameConfigMenu::Init() {
    ini_play_mode = ini_get_setting(INI_PLAY_MODE);
    ini_timer = ini_get_setting(INI_TIMER);
    ini_endturn = ini_get_setting(INI_ENDTURN);
    ini_start_gold = ini_get_setting(INI_START_GOLD);
    ini_opponent = ini_get_setting(INI_OPPONENT);

    if (game_mode != GAME_CONFIG_MENU_GAME_MODE_3 && game_mode != GAME_CONFIG_MENU_GAME_MODE_4) {
        ini_victory_type = ini_get_setting(INI_VICTORY_TYPE);
        ini_victory_limit = ini_get_setting(INI_VICTORY_LIMIT);
    }

    ini_raw_resource = ini_get_setting(INI_RAW_RESOURCE);
    ini_fuel_resource = ini_get_setting(INI_FUEL_RESOURCE);
    ini_gold_resource = ini_get_setting(INI_GOLD_RESOURCE);
    ini_alien_derelicts = ini_get_setting(INI_ALIEN_DERELICTS);

    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    event_click_done = false;
    event_click_cancel = false;

    victory_limit_text[0] = '\0';
    text_edit = nullptr;
    field_867 = 0;

    mouse_hide();
    WindowManager_LoadBigImage(OPTNFRM, window, window->width, false, false, -1, -1, true);

    for (int32_t i = 0; i < GAME_CONFIG_MENU_ITEM_COUNT; ++i) {
        buttons[i] = nullptr;

        if ((i > 5 || (game_mode != GAME_CONFIG_MENU_GAME_MODE_3 && game_mode != GAME_CONFIG_MENU_GAME_MODE_4)) &&
            (i <= 21 || i >= 48 ||
             (game_mode != GAME_CONFIG_MENU_GAME_MODE_2 && game_mode != GAME_CONFIG_MENU_GAME_MODE_4))) {
            ButtonInit(i);
        }
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    bg_panels[0] =
        new (std::nothrow) Image(WindowManager_ScaleUlx(window, 6), WindowManager_ScaleUly(window, 38), 200, 190);
    bg_panels[1] =
        new (std::nothrow) Image(WindowManager_ScaleUlx(window, 216), WindowManager_ScaleUly(window, 38), 200, 190);
    bg_panels[2] =
        new (std::nothrow) Image(WindowManager_ScaleUlx(window, 426), WindowManager_ScaleUly(window, 38), 200, 190);
    bg_panels[3] =
        new (std::nothrow) Image(WindowManager_ScaleUlx(window, 6), WindowManager_ScaleUly(window, 233), 200, 190);
    bg_panels[4] =
        new (std::nothrow) Image(WindowManager_ScaleUlx(window, 216), WindowManager_ScaleUly(window, 233), 200, 190);
    bg_panels[5] =
        new (std::nothrow) Image(WindowManager_ScaleUlx(window, 426), WindowManager_ScaleUly(window, 233), 200, 190);

    for (int32_t i = 0; i < 6; ++i) {
        bg_panels[i]->Copy(window);
    }

    menu_draw_menu_title(window, &game_config_menu_items[0], GNW_TEXT_OUTLINE | COLOR_GREEN, true);
    DrawPanels();
    mouse_show();
}

void GameConfigMenu::Deinit() {
    for (int32_t i = 0; i < GAME_CONFIG_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }

    for (int32_t i = 0; i < 6; ++i) {
        delete bg_panels[i];
    }

    delete text_edit;
}

void GameConfigMenu::EventComputerOpponent() {
    ini_set_setting(INI_OPPONENT, key);
    DrawPanelComputerOpponent();
    win_draw(window->id);
}

void GameConfigMenu::EventTurnTimer() {
    ini_set_setting(INI_TIMER, strtol(game_config_menu_items[key + 9].title.c_str(), nullptr, 10));
    DrawPanelTurnTimers();
    win_draw(window->id);
}

void GameConfigMenu::EventEndturn() {
    ini_set_setting(INI_ENDTURN, strtol(game_config_menu_items[key + 9].title.c_str(), nullptr, 10));
    DrawPanelTurnTimers();
    win_draw(window->id);
}

void GameConfigMenu::EventPlayMode() {
    ini_set_setting(INI_PLAY_MODE, key - 20);
    DrawPanelPlayMode();
    win_draw(window->id);
}

void GameConfigMenu::EventStartingCredit() {
    ini_set_setting(INI_START_GOLD, strtol(game_config_menu_items[key + 15].title.c_str(), nullptr, 10));
    DrawPanelStartingCredit();
    win_draw(window->id);
}

void GameConfigMenu::EventResourceLevelsRaw() {
    ini_set_setting(INI_RAW_RESOURCE, key - 28);
    DrawPanelResourceLevels();
    win_draw(window->id);
}

void GameConfigMenu::EventResourceLevelsFuel() {
    ini_set_setting(INI_FUEL_RESOURCE, key - 31);
    DrawPanelResourceLevels();
    win_draw(window->id);
}

void GameConfigMenu::EventResourceLevelsGold() {
    ini_set_setting(INI_GOLD_RESOURCE, key - 34);
    DrawPanelResourceLevels();
    win_draw(window->id);
}

void GameConfigMenu::EventResourceLevelsAlien() {
    ini_set_setting(INI_ALIEN_DERELICTS, key - 37);
    DrawPanelResourceLevels();
    win_draw(window->id);
}

void GameConfigMenu::EventVictoryCondition() {
    int32_t victory_type{0};
    int32_t victory_limit{0};
    int32_t index;

    index = key - 40;

    switch (index) {
        case 0: {
            victory_type = 0;
            victory_limit = 100;
        } break;

        case 1: {
            victory_type = 0;
            victory_limit = 200;
        } break;

        case 2: {
            victory_type = 0;
            victory_limit = 400;
        } break;

        case 3: {
            victory_type = 1;
            victory_limit = 200;
        } break;

        case 4: {
            victory_type = 1;
            victory_limit = 400;
        } break;

        case 5: {
            victory_type = 1;
            victory_limit = 800;
        } break;

        default: {
            SDL_assert(0);
        } break;
    }

    ini_set_setting(INI_VICTORY_TYPE, victory_type);
    ini_set_setting(INI_VICTORY_LIMIT, victory_limit);

    DrawPanelVictoryCondition();
    win_draw(window->id);
}

void GameConfigMenu::EventVictoryConditionPrefs() {
    GameConfigMenuControlItem* control;
    int32_t victory_type;
    int32_t victory_limit;

    field_867 = key;

    DrawPanelVictoryCondition();
    win_draw(window->id);

    victory_type = ini_get_setting(INI_VICTORY_TYPE);
    victory_limit = ini_get_setting(INI_VICTORY_LIMIT);

    if (field_867 == 46 && victory_type) {
        victory_limit = 0;
    } else if (field_867 == 47 && victory_type != 1) {
        victory_limit = 0;
    }

    snprintf(victory_limit_text, 30, "%d", victory_limit);

    control = &game_config_menu_controls[field_867];

    text_edit = new (std::nothrow)
        TextEdit(window, victory_limit_text, 5, WindowManager_ScaleUlx(window, control->bounds.ulx + 10),
                 WindowManager_ScaleUly(window, control->bounds.uly), control->bounds.lrx - control->bounds.ulx - 20,
                 control->bounds.lry - control->bounds.uly, 0xA2, GNW_TEXT_FONT_5);

    text_edit->SetMode(1);
    text_edit->LoadBgImage();
    text_edit->SetEditedText(victory_limit_text);
    text_edit->DrawFullText();
    text_edit->EnterTextEditField();
}

void GameConfigMenu::EventCancel() {
    ini_set_setting(INI_PLAY_MODE, ini_play_mode);
    ini_set_setting(INI_TIMER, ini_timer);
    ini_set_setting(INI_ENDTURN, ini_endturn);
    ini_set_setting(INI_START_GOLD, ini_start_gold);
    ini_set_setting(INI_OPPONENT, ini_opponent);

    if (game_mode != GAME_CONFIG_MENU_GAME_MODE_3 && game_mode != GAME_CONFIG_MENU_GAME_MODE_4) {
        ini_set_setting(INI_VICTORY_TYPE, ini_victory_type);
        ini_set_setting(INI_VICTORY_LIMIT, ini_victory_limit);
    }

    ini_set_setting(INI_RAW_RESOURCE, ini_raw_resource);
    ini_set_setting(INI_FUEL_RESOURCE, ini_fuel_resource);
    ini_set_setting(INI_GOLD_RESOURCE, ini_gold_resource);
    ini_set_setting(INI_ALIEN_DERELICTS, ini_alien_derelicts);

    event_click_cancel = true;
}

void GameConfigMenu::EventHelp() { HelpMenu_Menu("OPTIONS_SETUP", WINDOW_MAIN_WINDOW); }

void GameConfigMenu::ButtonInit(int32_t index) {
    GameConfigMenuControlItem* control;

    control = &game_config_menu_controls[index];

    Text_SetFont((index < 48) ? GNW_TEXT_FONT_5 : GNW_TEXT_FONT_1);

    if (control->image_id != INVALID_ID && control->label) {
        buttons[index] = new (std::nothrow) Button(control->image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   WindowManager_ScaleUlx(window, control->bounds.ulx),
                                                   WindowManager_ScaleUly(window, control->bounds.uly));
        buttons[index]->SetCaption(control->label);
    } else {
        buttons[index] = new (std::nothrow) Button(
            WindowManager_ScaleUlx(window, control->bounds.ulx), WindowManager_ScaleUly(window, control->bounds.uly),
            control->bounds.lrx - control->bounds.ulx, control->bounds.lry - control->bounds.uly);

        if (control->image_id != INVALID_ID) {
            WindowManager_LoadSimpleImage(control->image_id, WindowManager_ScaleUlx(window, control->bounds.ulx),
                                          WindowManager_ScaleUly(window, control->bounds.uly), true, window);
        }
    }

    buttons[index]->SetFlags(0);
    buttons[index]->SetRValue(1000 + index);
    buttons[index]->SetPValue(GNW_INPUT_PRESS + index);
    buttons[index]->SetSfx(control->sfx);
    buttons[index]->RegisterButton(window->id);
    buttons[index]->Enable();

    menu_item[index].r_value = 1000 + index;
    menu_item[index].event_code = control->event_code;
    menu_item[index].event_handler = control->event_handler;
}

void GameConfigMenu::DrawPanels() {
    if (game_mode == GAME_CONFIG_MENU_GAME_MODE_1 || game_mode == GAME_CONFIG_MENU_GAME_MODE_2) {
        DrawPanelComputerOpponent();
    }

    DrawPanelTurnTimers();
    DrawPanelPlayMode();

    if (game_mode == GAME_CONFIG_MENU_GAME_MODE_1 || game_mode == GAME_CONFIG_MENU_GAME_MODE_3) {
        DrawPanelStartingCredit();
        DrawPanelResourceLevels();
        DrawPanelVictoryCondition();
    }

    win_draw(window->id);
}

void GameConfigMenu::DrawPanelComputerOpponent() {
    const char* text;
    int32_t opponent;

    DrawBgPanel(1);
    opponent = ini_get_setting(INI_OPPONENT);
    DrawRadioButtons(6, 7, opponent);
    text = game_config_menu_difficulty_descriptions[opponent];
    Text_TextBox(window->buffer, window->width, text, WindowManager_ScaleUlx(window, 19),
                 WindowManager_ScaleUly(window, 170), 180, 60, GNW_TEXT_OUTLINE | COLOR_YELLOW, false);
}

void GameConfigMenu::DrawPanelTurnTimers() {
    int32_t timer_setting;
    int32_t timer_value;
    int32_t timer_index;
    int32_t endturn_setting;
    int32_t endturn_value;
    int32_t endturn_index;

    DrawBgPanel(2);
    DrawCaption(window, &game_config_menu_items[13], Fonts_BrightYellowColor, true);
    DrawCaption(window, &game_config_menu_items[14], Fonts_BrightYellowColor, true);

    timer_setting = ini_get_setting(INI_TIMER);

    for (timer_index = 6; timer_index > 0; --timer_index) {
        timer_value = strtol(game_config_menu_items[timer_index + 15].title.c_str(), nullptr, 10);

        if (timer_setting >= timer_value) {
            break;
        }
    }

    endturn_setting = ini_get_setting(INI_ENDTURN);

    for (endturn_index = 6; endturn_index > 0; --endturn_index) {
        endturn_value = strtol(game_config_menu_items[endturn_index + 22].title.c_str(), nullptr, 10);

        if (endturn_setting >= endturn_value) {
            break;
        }
    }

    DrawRadioButtons(7, 15, timer_index);
    DrawRadioButtons(7, 22, endturn_index);
}

void GameConfigMenu::DrawPanelPlayMode() {
    DrawBgPanel(3);
    DrawRadioButtons(2, 29, ini_get_setting(INI_PLAY_MODE));
}

void GameConfigMenu::DrawPanelStartingCredit() {
    int32_t starting_credits;
    int32_t credits;
    int32_t index;

    DrawBgPanel(4);

    for (int32_t i = 0; i < 6; ++i) {
        DrawCaption(window, &game_config_menu_items[i + 31]);
    }

    starting_credits = ini_get_setting(INI_START_GOLD);

    for (index = 5; index > 0; --index) {
        credits = strtol(game_config_menu_items[index + 37].title.c_str(), nullptr, 10);

        if (starting_credits >= credits) {
            break;
        }
    }

    DrawRadioButtons(6, 37, index);
}

void GameConfigMenu::DrawPanelResourceLevels() {
    DrawBgPanel(5);

    DrawCaption(window, &game_config_menu_items[43]);
    DrawCaption(window, &game_config_menu_items[44]);
    DrawCaption(window, &game_config_menu_items[45]);
    DrawCaption(window, &game_config_menu_items[46]);

    DrawRadioButtons(3, 47, ini_get_setting(INI_RAW_RESOURCE));
    DrawRadioButtons(3, 50, ini_get_setting(INI_FUEL_RESOURCE));
    DrawRadioButtons(3, 53, ini_get_setting(INI_GOLD_RESOURCE));
    DrawRadioButtons(3, 56, ini_get_setting(INI_ALIEN_DERELICTS));
}

void GameConfigMenu::DrawPanelVictoryCondition() {
    int32_t menu_index;
    int32_t victory_limit;

    DrawBgPanel(6);

    DrawCaption(window, &game_config_menu_items[59], Fonts_BrightYellowColor, true);
    DrawCaption(window, &game_config_menu_items[60], Fonts_BrightYellowColor, true);

    victory_limit = ini_get_setting(INI_VICTORY_LIMIT);

    const auto text = std::format("{}", victory_limit);

    if (ini_get_setting(INI_VICTORY_TYPE)) {
        game_config_menu_items[67].title.clear();

        if (field_867) {
            game_config_menu_items[68].title.clear();
        } else {
            game_config_menu_items[68].title = text;
        }

        if (victory_limit > 200) {
            if (victory_limit > 400) {
                menu_index = 45;
            } else {
                menu_index = 44;
            }
        } else {
            menu_index = 43;
        }

    } else {
        game_config_menu_items[68].title.clear();

        if (field_867) {
            game_config_menu_items[67].title.clear();
        } else {
            game_config_menu_items[67].title = text;
        }

        if (victory_limit > 100) {
            if (victory_limit > 200) {
                menu_index = 42;
            } else {
                menu_index = 41;
            }
        } else {
            menu_index = 40;
        }
    }

    menu_index -= 40;

    DrawRadioButtons(8, 61, menu_index);
    menu_draw_menu_title(window, &game_config_menu_items[67], GNW_TEXT_OUTLINE | 0xFF, true);
    menu_draw_menu_title(window, &game_config_menu_items[68], GNW_TEXT_OUTLINE | 0xFF, true);

    DrawCaption(window, &game_config_menu_items[69], Fonts_BrightYellowColor, true);
    DrawCaption(window, &game_config_menu_items[70], Fonts_BrightYellowColor, true);
}

void GameConfigMenu::DrawBgPanel(int32_t group_index) {
    bg_panels[group_index - 1]->Write(window);
    menu_draw_menu_title(window, &game_config_menu_items[group_index], GNW_TEXT_OUTLINE | COLOR_GREEN, true);
}

void GameConfigMenu::DrawRadioButtons(int32_t element_count, int32_t start_index, int32_t selection_index) {
    for (int32_t i = 0; i < element_count; ++i) {
        DrawCaption(window, &game_config_menu_items[start_index + i], Fonts_BrightBrownColor, true);

        if (selection_index == i) {
            DrawSelector(window, &game_config_menu_items[start_index + i], 0xFF);
        }
    }
}

void GameConfigMenu::UpdateTextEdit(int32_t mode) {
    if (text_edit) {
        if (mode) {
            ini_set_setting(INI_VICTORY_TYPE, field_867 - 46);
            ini_set_setting(INI_VICTORY_LIMIT, strtol(victory_limit_text, nullptr, 10));
        }

        text_edit->LeaveTextEditField();
        delete text_edit;
        text_edit = nullptr;

        field_867 = 0;
        DrawPanelVictoryCondition();

        win_draw(window->id);
    }
}

void GameConfigMenu::EventDone() {
    menu_update_resource_levels();

    if (game_mode != GAME_CONFIG_MENU_GAME_MODE_4) {
        ini_setting_victory_type = ini_get_setting(INI_VICTORY_TYPE);
        ini_setting_victory_limit = ini_get_setting(INI_VICTORY_LIMIT);
    }

    event_click_done = true;
}
