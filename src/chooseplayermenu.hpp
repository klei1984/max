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
#include "menu.hpp"

#define CHOOSE_PLAYER_MENU_ITEM_COUNT 19
#define CHOOSE_PLAYER_MENU_TITLE_COUNT 6

class ChoosePlayerMenu;

struct ChoosePlayerMenuItem {
    int32_t r_value;
    int32_t event_code;
    void (ChoosePlayerMenu::*event_handler)();
};

struct ChoosePlayerMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char *label;
    int32_t event_code;
    void (ChoosePlayerMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    {{(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx)}

class ChoosePlayerMenu {
    struct MenuTitleItem choose_player_menu_titles[CHOOSE_PLAYER_MENU_TITLE_COUNT] = {
        MENU_TITLE_ITEM_DEF(230, 6, 410, 26, ""),       MENU_TITLE_ITEM_DEF(61, 31, 141, 51, _(5985)),
        MENU_TITLE_ITEM_DEF(162, 31, 241, 51, _(9ffa)), MENU_TITLE_ITEM_DEF(272, 31, 351, 51, _(76ed)),
        MENU_TITLE_ITEM_DEF(380, 31, 459, 51, _(dc93)), MENU_TITLE_ITEM_DEF(497, 31, 576, 51, _(bda5)),
    };

    struct ChoosePlayerMenuControlItem choose_player_menu_controls[CHOOSE_PLAYER_MENU_ITEM_COUNT] = {
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
        MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, _(9605), GNW_KB_KEY_SHIFT_DIVIDE, &ChoosePlayerMenu::EventHelp,
                         NHELP0),
        MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(ef0a), GNW_KB_KEY_RETURN, &ChoosePlayerMenu::EventDone, NDONE0),
    };

    void ButtonInit(int32_t index, int32_t mode);
    void UpdateButtons();
    void ButtonSetState(int32_t team, int32_t rest_state);

public:
    WindowInfo *window;
    uint32_t event_click_done;
    uint32_t event_click_cancel;
    int32_t key_press;
    Button *buttons[CHOOSE_PLAYER_MENU_ITEM_COUNT];
    ChoosePlayerMenuItem menu_item[CHOOSE_PLAYER_MENU_ITEM_COUNT];
    bool is_single_player;

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
