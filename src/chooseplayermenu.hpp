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

#ifndef CHOOSEPLAYERMENU_HPP
#define CHOOSEPLAYERMENU_HPP

#include "button.hpp"

#define CHOOSE_PLAYER_MENU_ITEM_COUNT 19

class ChoosePlayerMenu;

struct ChoosePlayerMenuItem {
    int32_t r_value;
    int32_t event_code;
    void (ChoosePlayerMenu::*event_handler)();
};

class ChoosePlayerMenu {
    void ButtonInit(int32_t index, int32_t mode);
    void UpdateButtons();
    void ButtonSetState(int32_t team, int32_t rest_state);

public:
    WindowInfo *window;
    uint32_t event_click_done;
    uint32_t event_click_cancel;
    uint32_t game_type;
    int32_t key_press;
    Button *buttons[CHOOSE_PLAYER_MENU_ITEM_COUNT];
    ChoosePlayerMenuItem menu_item[CHOOSE_PLAYER_MENU_ITEM_COUNT];

    void Init();
    void Deinit();
    void EventSelectHuman();
    void EventSelectComputer();
    void EventSelectNone();
    void EventSelectClan();
    void EventCancel();
    void EventHelp();
    void EventDone();
};

#endif /* CHOOSEPLAYERMENU_HPP */
