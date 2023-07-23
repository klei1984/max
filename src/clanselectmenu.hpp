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

#ifndef CLANSELECTMENU_HPP
#define CLANSELECTMENU_HPP

#include "button.hpp"
#include "inifile.hpp"

#define CLAN_SELECT_MENU_ITEM_COUNT 12

class ClanSelectMenu;

struct ClanSelectMenuItem {
    int32_t r_value;
    int32_t event_code;
    void (ClanSelectMenu::*event_handler)();
};

class ClanSelectMenu {
    void ButtonInit(int32_t index, int32_t mode);
    void SelectMenuItems();
    void DrawClanUpgrades(const char *text, int32_t index, int32_t color);

public:
    WindowInfo *window;

    bool event_click_done_cancel_random;
    IniParameter team_clan_ini_id;
    int32_t team_clan;
    int32_t team_clan_selection;
    int32_t key;
    Image *image;

    Button *buttons[CLAN_SELECT_MENU_ITEM_COUNT];
    ClanSelectMenuItem menu_item[CLAN_SELECT_MENU_ITEM_COUNT];

    void Init(int32_t team);
    void Deinit();
    void ClanSelection();
    void EventRandom();
    void EventCancel();
    void EventHelp();
    void EventDone();
};

#endif /* CLANSELECTMENU_HPP */
