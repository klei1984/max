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
#include "menu.hpp"
#include "missionmanager.hpp"
#include "smartstring.hpp"

#define GAME_SETUP_MENU_ITEM_COUNT 19
#define GAME_SETUP_MENU_TITLE_COUNT 2

class GameSetupMenu;

struct GameSetupMenuItem {
    int32_t r_value;
    int32_t event_code;
    void (GameSetupMenu::*event_handler)();
};

struct GameSetupMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char* label;
    int32_t event_code;
    void (GameSetupMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    {{(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx)}

class GameSetupMenu {
    struct MenuTitleItem game_setup_menu_titles[GAME_SETUP_MENU_TITLE_COUNT] = {
        MENU_TITLE_ITEM_DEF(400, 174, 580, 195, ""),
        MENU_TITLE_ITEM_DEF(354, 215, 620, 416, ""),
    };

    struct GameSetupMenuControlItem game_setup_menu_controls[GAME_SETUP_MENU_ITEM_COUNT] = {
        MENU_CONTROL_DEF(18, 184, 312, 203, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 204, 312, 223, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 224, 312, 243, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 244, 312, 263, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 264, 312, 283, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 284, 312, 303, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 304, 312, 323, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 324, 312, 343, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 344, 312, 363, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 364, 312, 383, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 384, 312, 403, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(18, 404, 312, 423, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
        MENU_CONTROL_DEF(16, 438, 0, 0, MNUUAROU, nullptr, 0, &GameSetupMenu::EventScrollButton, MBUTT0),
        MENU_CONTROL_DEF(48, 438, 0, 0, MNUDAROU, nullptr, 0, &GameSetupMenu::EventScrollButton, MBUTT0),
        MENU_CONTROL_DEF(563, 438, 0, 0, MNUUAROU, nullptr, 0, &GameSetupMenu::EventBriefingButton, MBUTT0),
        MENU_CONTROL_DEF(596, 438, 0, 0, MNUDAROU, nullptr, 0, &GameSetupMenu::EventBriefingButton, MBUTT0),
        MENU_CONTROL_DEF(200, 438, 0, 0, MNUBTN4U, _(639d), GNW_KB_KEY_ESCAPE, &GameSetupMenu::EventCancel, NCANC0),
        MENU_CONTROL_DEF(312, 438, 0, 0, MNUBTN5U, _(da8e), GNW_KB_KEY_SHIFT_DIVIDE, &GameSetupMenu::EventHelp, NHELP0),
        MENU_CONTROL_DEF(361, 438, 0, 0, MNUBTN6U, _(f0a3), GNW_KB_KEY_RETURN, &GameSetupMenu::EventStart, NDONE0),
    };

    void ButtonInit(int32_t index);
    void DrawMissionList();
    void LoadMissionDescription();
    void DrawMissionDescription();
    void DrawSaveFileTitle(int32_t game_slot, int32_t color);
    void DrawDescriptionPanel();
    int32_t FindNextValidFile(int32_t game_slot);

public:
    WindowInfo* window;
    bool event_click_done;
    bool event_click_cancel;
    int32_t key;
    Button* buttons[GAME_SETUP_MENU_ITEM_COUNT];
    GameSetupMenuItem menu_item[GAME_SETUP_MENU_ITEM_COUNT];
    std::string mission_titles[GAME_SETUP_MENU_ITEM_COUNT];
    int32_t game_index_first_on_page;
    int32_t game_count;
    int32_t game_file_number;
    SmartString* strings;
    int32_t string_row_count;
    int16_t string_row_index;
    int16_t rows_per_page;

    MissionCategory m_mission_category;

    void Init(int32_t palette_from_image);
    void Deinit();
    void EventSelectItem();
    void EventScrollButton();
    void EventStepButton();
    void EventBriefingButton();
    void EventCancel();
    void EventHelp();
    void EventStart();
    MissionCategory GetMissionCategory() const;
    void SetMissionCategory(const MissionCategory mission_category);
};

#endif /* GAMESETUPMENU_HPP */
