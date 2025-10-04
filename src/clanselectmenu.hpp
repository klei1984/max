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
#include "menu.hpp"

#define CLAN_SELECT_MENU_ITEM_COUNT 12
#define CLAN_SELECT_MENU_ICONS_COUNT 8
#define CLAN_SELECT_MENU_TITLE_COUNT 2

class ClanSelectMenu;

struct ClanSelectMenuItem {
    int32_t r_value;
    int32_t event_code;
    void (ClanSelectMenu::*event_handler)();
};

struct ClanSelectMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char* label;
    int32_t event_code;
    void (ClanSelectMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    {{(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx)}

class ClanSelectMenu {
    struct MenuTitleItem clan_select_menu_screen_title = {{230, 6, 410, 26}, _(897f)};
    struct MenuTitleItem clan_select_menu_screen_text[CLAN_SELECT_MENU_TITLE_COUNT] = {
        MENU_TITLE_ITEM_DEF(41, 350, 610, 409, ""),
        MENU_TITLE_ITEM_DEF(330, 350, 610, 409, ""),
    };

    struct MenuTitleItem clan_select_menu_clan_icons[CLAN_SELECT_MENU_ICONS_COUNT] = {
        MENU_TITLE_ITEM_DEF(32, 140, 142, 160, ""),  MENU_TITLE_ITEM_DEF(188, 140, 298, 160, ""),
        MENU_TITLE_ITEM_DEF(344, 140, 454, 160, ""), MENU_TITLE_ITEM_DEF(497, 140, 607, 160, ""),
        MENU_TITLE_ITEM_DEF(32, 290, 142, 310, ""),  MENU_TITLE_ITEM_DEF(188, 290, 298, 310, ""),
        MENU_TITLE_ITEM_DEF(344, 290, 454, 310, ""), MENU_TITLE_ITEM_DEF(497, 290, 607, 310, ""),
    };

    struct ClanSelectMenuControlItem clan_select_menu_controls[CLAN_SELECT_MENU_ITEM_COUNT] = {
        MENU_CONTROL_DEF(46, 44, 0, 0, CH_CN1_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CCHOS0),
        MENU_CONTROL_DEF(201, 44, 0, 0, CH_CN2_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CCRIM0),
        MENU_CONTROL_DEF(355, 44, 0, 0, CH_CN3_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CVONG0),
        MENU_CONTROL_DEF(510, 44, 0, 0, CH_CN4_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CAYER0),
        MENU_CONTROL_DEF(46, 194, 0, 0, CH_CN5_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CMUSA0),
        MENU_CONTROL_DEF(201, 194, 0, 0, CH_CN6_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CSACR0),
        MENU_CONTROL_DEF(355, 194, 0, 0, CH_CN7_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CKNIG0),
        MENU_CONTROL_DEF(510, 194, 0, 0, CH_CN8_U, nullptr, 0, &ClanSelectMenu::ClanSelection, CAXIS0),
        MENU_CONTROL_DEF(243, 438, 0, 0, MNUBTN3U, _(7e91), 0, &ClanSelectMenu::EventRandom, CRAND0),
        MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, _(f5ee), GNW_KB_KEY_SHIFT_ESCAPE, &ClanSelectMenu::EventCancel,
                         CCANC0),
        MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, _(eb1b), GNW_KB_KEY_SHIFT_DIVIDE, &ClanSelectMenu::EventHelp,
                         CHELP0),
        MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(aff0), GNW_KB_KEY_SHIFT_RETURN, &ClanSelectMenu::EventDone,
                         CDONE0),
    };

    void ButtonInit(int32_t index, int32_t mode);
    void SelectMenuItems();
    void DrawClanUpgrades(const char* text, int32_t index, int32_t color);

public:
    WindowInfo* window;

    bool event_click_done_cancel_random;
    IniParameter team_clan_ini_id;
    int32_t team_clan;
    int32_t team_clan_selection;
    int32_t key;
    Image* image;

    Button* buttons[CLAN_SELECT_MENU_ITEM_COUNT];
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
