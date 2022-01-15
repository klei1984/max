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

#ifndef GAMESETUPMENU_HPP
#define GAMESETUPMENU_HPP

#include "button.hpp"
#include "smartstring.hpp"

#define GAME_CONFIG_MENU_ITEM_COUNT 19

class GameSetupMenu;

struct GameSetupMenuItem {
    int r_value;
    int event_code;
    void (GameSetupMenu::*event_handler)();
};

class GameSetupMenu {
    void ButtonInit(int index);
    void DrawMissionList();
    void LoadMissionDescription();
    void DrawMissionDescription();
    void DrawSaveFileTitle(int game_slot, int color);
    void DrawDescriptionPanel();
    int FindNextValidFile(int game_slot);

public:
    WindowInfo *window;
    bool event_click_done;
    bool event_click_cancel;
    int key;
    Button *buttons[GAME_CONFIG_MENU_ITEM_COUNT];
    GameSetupMenuItem menu_item[GAME_CONFIG_MENU_ITEM_COUNT];
    int game_index_first_on_page;
    int game_count;
    int game_file_number;
    int game_file_type;
    SmartString *strings;
    int string_row_count;
    short string_row_index;
    short rows_per_page;

    void Init(int palette_from_image);
    void Deinit();
    void EventSelectItem();
    void EventScrollButton();
    void EventStepButton();
    void EventBriefingButton();
    void EventCancel();
    void EventHelp();
    void EventStart();
};

#endif /* GAMESETUPMENU_HPP */
