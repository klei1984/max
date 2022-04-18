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

#ifndef GAMECONFIGMENU_HPP
#define GAMECONFIGMENU_HPP

#include "button.hpp"
#include "textedit.hpp"

#define GAME_CONFIG_MENU_ITEM_COUNT 51

enum {
    GAME_CONFIG_MENU_GAME_MODE_1 = 1,
    GAME_CONFIG_MENU_GAME_MODE_2 = 2,
    GAME_CONFIG_MENU_GAME_MODE_3 = 3,
    GAME_CONFIG_MENU_GAME_MODE_4 = 4,
};

class GameConfigMenu;

struct GameConfigMenuItem {
    int r_value;
    int event_code;
    void (GameConfigMenu::*event_handler)();
};

class GameConfigMenu {
    void ButtonInit(int index);
    void DrawBgPanel(int group_index);
    void DrawRadioButtons(int element_count, int start_index, int selection_index);
    void DrawPanelComputerOpponent();
    void DrawPanelTurnTimers();
    void DrawPanelPlayMode();
    void DrawPanelStartingCredit();
    void DrawPanelResourceLevels();
    void DrawPanelVictoryCondition();
    void DrawPanels();

public:
    WindowInfo *window;
    bool event_click_done;
    bool event_click_cancel;
    unsigned char game_mode;
    int key;
    GameConfigMenuItem menu_item[GAME_CONFIG_MENU_ITEM_COUNT];
    Button *buttons[GAME_CONFIG_MENU_ITEM_COUNT];
    TextEdit *text_edit;
    Image *bg_panels[6];
    char victory_limit_text[30];
    unsigned int field_867;
    unsigned char ini_play_mode;
    unsigned short ini_timer;
    unsigned short ini_endturn;
    unsigned char ini_opponent;
    unsigned short ini_start_gold;
    unsigned char ini_raw_resource;
    unsigned char ini_fuel_resource;
    unsigned char ini_gold_resource;
    unsigned char ini_alien_derelicts;
    unsigned char ini_victory_type;
    unsigned short ini_victory_limit;

    void Init();
    void Deinit();
    void EventComputerOpponent();
    void EventTurnTimer();
    void EventEndturn();
    void EventPlayMode();
    void EventStartingCredit();
    void EventResourceLevelsRaw();
    void EventResourceLevelsFuel();
    void EventResourceLevelsGold();
    void EventResourceLevelsAlien();
    void EventVictoryCondition();
    void EventVictoryConditionPrefs();
    void EventCancel();
    void EventHelp();
    void EventDone();
    void UpdateTextEdit(int mode);
};

extern struct MenuTitleItem game_config_menu_items[];

#endif /* GAMECONFIGMENU_HPP */
