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
#include "menu.hpp"
#include "textedit.hpp"

#define GAME_CONFIG_MENU_ITEM_COUNT 51
#define GAME_CONFIG_MENU_TITLE_COUNT 71
#define GAME_CONFIG_MENU_DIFFICULTY_COUNT 6

enum {
    GAME_CONFIG_MENU_GAME_MODE_1 = 1,
    GAME_CONFIG_MENU_GAME_MODE_2 = 2,
    GAME_CONFIG_MENU_GAME_MODE_3 = 3,
    GAME_CONFIG_MENU_GAME_MODE_4 = 4,
};

class GameConfigMenu;

struct GameConfigMenuItem {
    int32_t r_value;
    int32_t event_code;
    void (GameConfigMenu::*event_handler)();
};

struct GameConfigMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char* label;
    int32_t event_code;
    void (GameConfigMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    {{(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx)}

class GameConfigMenu {
    struct MenuTitleItem game_config_menu_items[GAME_CONFIG_MENU_TITLE_COUNT] = {
        MENU_TITLE_ITEM_DEF(230, 5, 409, 25, _(8448)),    MENU_TITLE_ITEM_DEF(19, 50, 199, 70, _(de1b)),
        MENU_TITLE_ITEM_DEF(230, 50, 409, 70, _(e96a)),   MENU_TITLE_ITEM_DEF(439, 50, 619, 70, _(4599)),
        MENU_TITLE_ITEM_DEF(19, 245, 199, 265, _(1d15)),  MENU_TITLE_ITEM_DEF(230, 245, 409, 265, _(e1e4)),
        MENU_TITLE_ITEM_DEF(439, 245, 619, 265, _(3b7b)), MENU_TITLE_ITEM_DEF(56, 85, 156, 100, _(fde2)),
        MENU_TITLE_ITEM_DEF(56, 100, 156, 115, _(e644)),  MENU_TITLE_ITEM_DEF(56, 115, 156, 130, _(2d88)),
        MENU_TITLE_ITEM_DEF(56, 130, 156, 145, _(8b31)),  MENU_TITLE_ITEM_DEF(56, 145, 156, 160, _(a2bb)),
        MENU_TITLE_ITEM_DEF(56, 160, 156, 175, _(9c54)),  MENU_TITLE_ITEM_DEF(227, 85, 307, 105, _(779f)),
        MENU_TITLE_ITEM_DEF(331, 85, 411, 105, _(99b8)),  MENU_TITLE_ITEM_DEF(227, 105, 307, 120, _(366c)),
        MENU_TITLE_ITEM_DEF(227, 120, 307, 135, _(03d2)), MENU_TITLE_ITEM_DEF(227, 135, 307, 150, "120"),
        MENU_TITLE_ITEM_DEF(227, 150, 307, 165, "180"),   MENU_TITLE_ITEM_DEF(227, 165, 307, 180, "240"),
        MENU_TITLE_ITEM_DEF(227, 180, 307, 195, "300"),   MENU_TITLE_ITEM_DEF(227, 195, 307, 210, "360"),
        MENU_TITLE_ITEM_DEF(331, 105, 411, 120, _(8f89)), MENU_TITLE_ITEM_DEF(331, 120, 411, 135, _(218f)),
        MENU_TITLE_ITEM_DEF(331, 135, 411, 150, "30"),    MENU_TITLE_ITEM_DEF(331, 150, 411, 165, "45"),
        MENU_TITLE_ITEM_DEF(331, 165, 411, 180, "60"),    MENU_TITLE_ITEM_DEF(331, 180, 411, 195, "75"),
        MENU_TITLE_ITEM_DEF(331, 195, 411, 210, "90"),    MENU_TITLE_ITEM_DEF(480, 90, 580, 110, _(f7e4)),
        MENU_TITLE_ITEM_DEF(480, 115, 580, 145, _(0ba9)), MENU_TITLE_ITEM_DEF(24, 285, 114, 300, _(dbc7)),
        MENU_TITLE_ITEM_DEF(24, 305, 114, 320, _(f793)),  MENU_TITLE_ITEM_DEF(24, 325, 114, 340, _(e512)),
        MENU_TITLE_ITEM_DEF(24, 345, 114, 360, _(a724)),  MENU_TITLE_ITEM_DEF(24, 365, 114, 380, _(c4eb)),
        MENU_TITLE_ITEM_DEF(24, 385, 114, 400, _(3ade)),  MENU_TITLE_ITEM_DEF(144, 285, 194, 300, "0"),
        MENU_TITLE_ITEM_DEF(144, 305, 194, 320, "50"),    MENU_TITLE_ITEM_DEF(144, 325, 194, 340, "100"),
        MENU_TITLE_ITEM_DEF(144, 345, 194, 360, "150"),   MENU_TITLE_ITEM_DEF(144, 365, 194, 380, "200"),
        MENU_TITLE_ITEM_DEF(144, 385, 194, 400, "250"),   MENU_TITLE_ITEM_DEF(229, 280, 409, 292, _(d558)),
        MENU_TITLE_ITEM_DEF(229, 315, 409, 327, _(10c4)), MENU_TITLE_ITEM_DEF(229, 350, 409, 362, _(4ef9)),
        MENU_TITLE_ITEM_DEF(229, 385, 409, 397, _(48bd)), MENU_TITLE_ITEM_DEF(225, 292, 287, 304, _(94df)),
        MENU_TITLE_ITEM_DEF(288, 292, 352, 304, _(8e58)), MENU_TITLE_ITEM_DEF(351, 292, 413, 304, _(dca9)),
        MENU_TITLE_ITEM_DEF(225, 327, 287, 339, _(8c28)), MENU_TITLE_ITEM_DEF(288, 327, 352, 339, _(ccfe)),
        MENU_TITLE_ITEM_DEF(351, 327, 413, 339, _(eac9)), MENU_TITLE_ITEM_DEF(225, 362, 287, 374, _(ec35)),
        MENU_TITLE_ITEM_DEF(288, 362, 352, 374, _(9c1d)), MENU_TITLE_ITEM_DEF(351, 362, 413, 374, _(0c4a)),
        MENU_TITLE_ITEM_DEF(225, 397, 287, 409, _(2484)), MENU_TITLE_ITEM_DEF(288, 397, 352, 409, _(291d)),
        MENU_TITLE_ITEM_DEF(351, 397, 413, 409, _(3099)), MENU_TITLE_ITEM_DEF(427, 285, 529, 305, _(298d)),
        MENU_TITLE_ITEM_DEF(529, 285, 633, 305, _(9689)), MENU_TITLE_ITEM_DEF(447, 305, 507, 320, _(f5d6)),
        MENU_TITLE_ITEM_DEF(447, 325, 507, 340, _(9427)), MENU_TITLE_ITEM_DEF(447, 345, 507, 360, _(d1d8)),
        MENU_TITLE_ITEM_DEF(551, 305, 611, 320, _(76b7)), MENU_TITLE_ITEM_DEF(551, 325, 611, 340, _(b459)),
        MENU_TITLE_ITEM_DEF(551, 345, 611, 360, _(4065)), MENU_TITLE_ITEM_DEF(447, 370, 507, 390, ""),
        MENU_TITLE_ITEM_DEF(551, 370, 611, 390, ""),      MENU_TITLE_ITEM_DEF(447, 390, 507, 410, _(a07a)),
        MENU_TITLE_ITEM_DEF(551, 390, 611, 410, _(7015)),
    };

    struct GameConfigMenuControlItem game_config_menu_controls[GAME_CONFIG_MENU_ITEM_COUNT] = {
        MENU_CONTROL_DEF(56, 85, 156, 100, INVALID_ID, nullptr, 0, &GameConfigMenu::EventComputerOpponent, KCARG0),
        MENU_CONTROL_DEF(56, 100, 156, 115, INVALID_ID, nullptr, 0, &GameConfigMenu::EventComputerOpponent, KCARG0),
        MENU_CONTROL_DEF(56, 115, 156, 130, INVALID_ID, nullptr, 0, &GameConfigMenu::EventComputerOpponent, KCARG0),
        MENU_CONTROL_DEF(56, 130, 156, 145, INVALID_ID, nullptr, 0, &GameConfigMenu::EventComputerOpponent, KCARG0),
        MENU_CONTROL_DEF(56, 145, 156, 160, INVALID_ID, nullptr, 0, &GameConfigMenu::EventComputerOpponent, KCARG0),
        MENU_CONTROL_DEF(56, 160, 156, 175, INVALID_ID, nullptr, 0, &GameConfigMenu::EventComputerOpponent, KCARG0),
        MENU_CONTROL_DEF(227, 105, 307, 120, INVALID_ID, nullptr, 0, &GameConfigMenu::EventTurnTimer, KCARG0),
        MENU_CONTROL_DEF(227, 120, 307, 135, INVALID_ID, nullptr, 0, &GameConfigMenu::EventTurnTimer, KCARG0),
        MENU_CONTROL_DEF(227, 135, 307, 150, INVALID_ID, nullptr, 0, &GameConfigMenu::EventTurnTimer, KCARG0),
        MENU_CONTROL_DEF(227, 150, 307, 165, INVALID_ID, nullptr, 0, &GameConfigMenu::EventTurnTimer, KCARG0),
        MENU_CONTROL_DEF(227, 165, 307, 180, INVALID_ID, nullptr, 0, &GameConfigMenu::EventTurnTimer, KCARG0),
        MENU_CONTROL_DEF(227, 180, 307, 195, INVALID_ID, nullptr, 0, &GameConfigMenu::EventTurnTimer, KCARG0),
        MENU_CONTROL_DEF(227, 195, 307, 210, INVALID_ID, nullptr, 0, &GameConfigMenu::EventTurnTimer, KCARG0),
        MENU_CONTROL_DEF(331, 105, 411, 120, INVALID_ID, nullptr, 0, &GameConfigMenu::EventEndturn, KCARG0),
        MENU_CONTROL_DEF(331, 120, 411, 135, INVALID_ID, nullptr, 0, &GameConfigMenu::EventEndturn, KCARG0),
        MENU_CONTROL_DEF(331, 135, 411, 150, INVALID_ID, nullptr, 0, &GameConfigMenu::EventEndturn, KCARG0),
        MENU_CONTROL_DEF(331, 150, 411, 165, INVALID_ID, nullptr, 0, &GameConfigMenu::EventEndturn, KCARG0),
        MENU_CONTROL_DEF(331, 165, 411, 180, INVALID_ID, nullptr, 0, &GameConfigMenu::EventEndturn, KCARG0),
        MENU_CONTROL_DEF(331, 180, 411, 195, INVALID_ID, nullptr, 0, &GameConfigMenu::EventEndturn, KCARG0),
        MENU_CONTROL_DEF(331, 195, 411, 210, INVALID_ID, nullptr, 0, &GameConfigMenu::EventEndturn, KCARG0),
        MENU_CONTROL_DEF(480, 90, 580, 110, INVALID_ID, nullptr, 0, &GameConfigMenu::EventPlayMode, KCARG0),
        MENU_CONTROL_DEF(480, 115, 580, 145, INVALID_ID, nullptr, 0, &GameConfigMenu::EventPlayMode, KCARG0),
        MENU_CONTROL_DEF(144, 285, 194, 300, INVALID_ID, nullptr, 0, &GameConfigMenu::EventStartingCredit, KCARG0),
        MENU_CONTROL_DEF(144, 305, 194, 320, INVALID_ID, nullptr, 0, &GameConfigMenu::EventStartingCredit, KCARG0),
        MENU_CONTROL_DEF(144, 325, 194, 340, INVALID_ID, nullptr, 0, &GameConfigMenu::EventStartingCredit, KCARG0),
        MENU_CONTROL_DEF(144, 345, 194, 360, INVALID_ID, nullptr, 0, &GameConfigMenu::EventStartingCredit, KCARG0),
        MENU_CONTROL_DEF(144, 365, 194, 380, INVALID_ID, nullptr, 0, &GameConfigMenu::EventStartingCredit, KCARG0),
        MENU_CONTROL_DEF(144, 385, 194, 400, INVALID_ID, nullptr, 0, &GameConfigMenu::EventStartingCredit, KCARG0),
        MENU_CONTROL_DEF(225, 292, 287, 304, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsRaw, KCARG0),
        MENU_CONTROL_DEF(288, 292, 351, 304, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsRaw, KCARG0),
        MENU_CONTROL_DEF(351, 292, 413, 304, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsRaw, KCARG0),
        MENU_CONTROL_DEF(225, 327, 287, 339, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsFuel, KCARG0),
        MENU_CONTROL_DEF(288, 327, 351, 339, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsFuel, KCARG0),
        MENU_CONTROL_DEF(351, 327, 413, 339, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsFuel, KCARG0),
        MENU_CONTROL_DEF(225, 362, 287, 374, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsGold, KCARG0),
        MENU_CONTROL_DEF(288, 362, 351, 374, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsGold, KCARG0),
        MENU_CONTROL_DEF(351, 362, 413, 374, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsGold, KCARG0),
        MENU_CONTROL_DEF(225, 397, 287, 409, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsAlien, KCARG0),
        MENU_CONTROL_DEF(288, 397, 351, 409, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsAlien, KCARG0),
        MENU_CONTROL_DEF(351, 397, 413, 409, INVALID_ID, nullptr, 0, &GameConfigMenu::EventResourceLevelsAlien, KCARG0),
        MENU_CONTROL_DEF(447, 305, 507, 320, INVALID_ID, nullptr, 0, &GameConfigMenu::EventVictoryCondition, KCARG0),
        MENU_CONTROL_DEF(447, 325, 507, 340, INVALID_ID, nullptr, 0, &GameConfigMenu::EventVictoryCondition, KCARG0),
        MENU_CONTROL_DEF(447, 345, 507, 360, INVALID_ID, nullptr, 0, &GameConfigMenu::EventVictoryCondition, KCARG0),
        MENU_CONTROL_DEF(551, 305, 611, 320, INVALID_ID, nullptr, 0, &GameConfigMenu::EventVictoryCondition, KCARG0),
        MENU_CONTROL_DEF(551, 325, 611, 340, INVALID_ID, nullptr, 0, &GameConfigMenu::EventVictoryCondition, KCARG0),
        MENU_CONTROL_DEF(551, 345, 611, 360, INVALID_ID, nullptr, 0, &GameConfigMenu::EventVictoryCondition, KCARG0),
        MENU_CONTROL_DEF(447, 370, 507, 390, PREFEDIT, nullptr, 0, &GameConfigMenu::EventVictoryConditionPrefs, KCARG0),
        MENU_CONTROL_DEF(551, 370, 611, 390, PREFEDIT, nullptr, 0, &GameConfigMenu::EventVictoryConditionPrefs, KCARG0),
        MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, _(a80a), GNW_KB_KEY_ESCAPE, &GameConfigMenu::EventCancel, NCANC0),
        MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, _(8610), GNW_KB_KEY_SHIFT_DIVIDE, &GameConfigMenu::EventHelp,
                         NHELP0),
        MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(da62), GNW_KB_KEY_RETURN, &GameConfigMenu::EventDone, NDONE0),
    };

    const char* game_config_menu_difficulty_descriptions[GAME_CONFIG_MENU_DIFFICULTY_COUNT] = {
        _(1dc3), _(eb1c), _(0e00), _(2de0), _(e22c), _(7cb8),
    };

    void ButtonInit(int32_t index);
    void DrawBgPanel(int32_t group_index);
    void DrawRadioButtons(int32_t element_count, int32_t start_index, int32_t selection_index);
    void DrawPanelComputerOpponent();
    void DrawPanelTurnTimers();
    void DrawPanelPlayMode();
    void DrawPanelStartingCredit();
    void DrawPanelResourceLevels();
    void DrawPanelVictoryCondition();
    void DrawPanels();

public:
    WindowInfo* window;
    bool event_click_done;
    bool event_click_cancel;
    uint8_t game_mode;
    int32_t key;
    GameConfigMenuItem menu_item[GAME_CONFIG_MENU_ITEM_COUNT];
    Button* buttons[GAME_CONFIG_MENU_ITEM_COUNT];
    TextEdit* text_edit;
    Image* bg_panels[6];
    char victory_limit_text[30];
    uint32_t active_text_edit_index;
    uint8_t ini_play_mode;
    uint16_t ini_timer;
    uint16_t ini_endturn;
    uint8_t ini_opponent;
    uint16_t ini_start_gold;
    uint8_t ini_raw_resource;
    uint8_t ini_fuel_resource;
    uint8_t ini_gold_resource;
    uint8_t ini_alien_derelicts;
    uint8_t ini_victory_type;
    uint16_t ini_victory_limit;

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
    void UpdateTextEdit(int32_t mode);
};

#endif /* GAMECONFIGMENU_HPP */
