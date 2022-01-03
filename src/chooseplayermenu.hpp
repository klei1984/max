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
    int r_value;
    int event_code;
    void (ChoosePlayerMenu::*event_handler)();
};

class ChoosePlayerMenu {
    void ButtonInit(int index, int mode);
    void UpdateButtons();
    void ButtonSetState(int team, int rest_state);

public:
    WindowInfo *window;
    unsigned int event_click_done;
    unsigned int event_click_cancel;
    unsigned int game_type;
    unsigned int key_press;
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
