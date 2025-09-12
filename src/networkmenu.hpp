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

#ifndef NETWORKMENU_HPP
#define NETWORKMENU_HPP

#include "button.hpp"
#include "menu.hpp"
#include "saveloadmenu.hpp"
#include "textedit.hpp"
#include "transport.hpp"

#define NETWORK_MENU_PLANET_ITEM_COUNT 24
#define NETWORK_MENU_ITEM_COUNT 24
#define NETWORK_MENU_CONFIG_COUNT 4
#define NETWORK_MENU_TITLE_COUNT 14
#define NETWORK_MENU_IMAGE_COUNT 14
#define NETWORK_MAX_HOSTS_PER_PAGE 8

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    {{(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx)}

class NetworkMenu;

struct NetworkMenuItem {
    int32_t r_value;
    int32_t event_code;
    void (NetworkMenu::*event_handler)();
};

struct NetworkMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char *label;
    int32_t event_code;
    void (NetworkMenu::*event_handler)();
    ResourceID sfx;
};

class NetworkMenu {
    const char *menu_planet_names[NETWORK_MENU_PLANET_ITEM_COUNT] = {
        _(e43b), _(f588), _(c78b), _(895d), _(5f5f), _(e7b2), _(f3fe), _(8524), _(4bb8), _(f408), _(0935), _(7303),
        _(94ef), _(c46c), _(48ac), _(275a), _(ea47), _(fcf0), _(6426), _(7ea8), _(386d), _(41e5), _(bbcb), _(ba99)};

    struct MenuTitleItem network_menu_titles[NETWORK_MENU_TITLE_COUNT] = {
        MENU_TITLE_ITEM_DEF(42, 48, 134, 61, _(4869)),   MENU_TITLE_ITEM_DEF(42, 71, 185, 84, ""),
        MENU_TITLE_ITEM_DEF(359, 28, 568, 188, _(5939)), MENU_TITLE_ITEM_DEF(59, 131, 149, 143, ""),
        MENU_TITLE_ITEM_DEF(191, 131, 285, 143, ""),     MENU_TITLE_ITEM_DEF(59, 249, 149, 260, ""),
        MENU_TITLE_ITEM_DEF(191, 249, 285, 260, ""),     MENU_TITLE_ITEM_DEF(150, 377, 617, 392, ""),
        MENU_TITLE_ITEM_DEF(150, 408, 617, 423, ""),     MENU_TITLE_ITEM_DEF(75, 160, 132, 231, ""),
        MENU_TITLE_ITEM_DEF(208, 160, 263, 231, ""),     MENU_TITLE_ITEM_DEF(75, 278, 132, 349, ""),
        MENU_TITLE_ITEM_DEF(208, 278, 263, 349, ""),     MENU_TITLE_ITEM_DEF(355, 228, 469, 342, ""),
    };

    struct NetworkMenuControlItem network_menu_controls[NETWORK_MENU_ITEM_COUNT] = {
        MENU_CONTROL_DEF(42, 71, 185, 84, INVALID_ID, nullptr, 0, &NetworkMenu::EventEditPlayerName, MENUOP),
        MENU_CONTROL_DEF(211, 25, 0, 0, M_CLAN_U, nullptr, 0, &NetworkMenu::EventSelectClan, MENUOP),
        MENU_CONTROL_DEF(359, 28, 568, 48, INVALID_ID, nullptr, 0, &NetworkMenu::EventTextWindow, MENUOP),
        MENU_CONTROL_DEF(359, 48, 568, 68, INVALID_ID, nullptr, 0, &NetworkMenu::EventTextWindow, MENUOP),
        MENU_CONTROL_DEF(359, 68, 568, 88, INVALID_ID, nullptr, 0, &NetworkMenu::EventTextWindow, MENUOP),
        MENU_CONTROL_DEF(359, 88, 568, 108, INVALID_ID, nullptr, 0, &NetworkMenu::EventTextWindow, MENUOP),
        MENU_CONTROL_DEF(359, 108, 568, 128, INVALID_ID, nullptr, 0, &NetworkMenu::EventTextWindow, MENUOP),
        MENU_CONTROL_DEF(359, 128, 568, 148, INVALID_ID, nullptr, 0, &NetworkMenu::EventTextWindow, MENUOP),
        MENU_CONTROL_DEF(359, 148, 568, 168, INVALID_ID, nullptr, 0, &NetworkMenu::EventTextWindow, MENUOP),
        MENU_CONTROL_DEF(359, 168, 568, 188, INVALID_ID, nullptr, 0, &NetworkMenu::EventTextWindow, MENUOP),
        MENU_CONTROL_DEF(516, 232, 0, 0, SBLNK_UP, _(119e), 0, &NetworkMenu::EventMapButton, MENUOP),
        MENU_CONTROL_DEF(516, 268, 0, 0, SBLNK_UP, _(8a6b), 0, &NetworkMenu::EventLoadButton, MENUOP),
        MENU_CONTROL_DEF(516, 304, 0, 0, SBLNK_UP, _(72fa), 0, &NetworkMenu::EventScenarioButton, MENUOP),
        MENU_CONTROL_DEF(75, 160, 0, 0, CH_NON_U, nullptr, 0, &NetworkMenu::EventSetJar, MENUOP),
        MENU_CONTROL_DEF(208, 160, 0, 0, CH_NON_U, nullptr, 0, &NetworkMenu::EventSetJar, MENUOP),
        MENU_CONTROL_DEF(75, 278, 0, 0, CH_NON_U, nullptr, 0, &NetworkMenu::EventSetJar, MENUOP),
        MENU_CONTROL_DEF(208, 278, 0, 0, CH_NON_U, nullptr, 0, &NetworkMenu::EventSetJar, MENUOP),
        MENU_CONTROL_DEF(33, 369, 0, 0, SBLNK_UP, _(1baf), 0, &NetworkMenu::EventChat, MENUOP),
        MENU_CONTROL_DEF(150, 377, 617, 392, INVALID_ID, 0, 0, &NetworkMenu::EventChat, MENUOP),
        MENU_CONTROL_DEF(243, 438, 0, 0, MNUBTN3U, _(d0db), 0, &NetworkMenu::EventOptions, MENUOP),
        MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, _(808a), GNW_KB_KEY_ESCAPE, &NetworkMenu::EventCancel, NCANC0),
        MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, _(0b5a), GNW_KB_KEY_SHIFT_DIVIDE, &NetworkMenu::EventHelp, MENUOP),
        MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(5fa7), GNW_KB_KEY_RETURN, &NetworkMenu::EventReady, NDONE0),
        MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(e4bb), GNW_KB_KEY_RETURN, &NetworkMenu::EventStart, NDONE0),
    };

    struct MenuTitleItem network_config_menu_items[NETWORK_MENU_CONFIG_COUNT] = {
        MENU_TITLE_ITEM_DEF(229, 385, 409, 397, _(48bd)),
        MENU_TITLE_ITEM_DEF(351, 292, 413, 304, _(dca9)),
        MENU_TITLE_ITEM_DEF(351, 327, 413, 339, _(eac9)),
        MENU_TITLE_ITEM_DEF(351, 362, 413, 374, _(0c4a)),
    };

    void ButtonInit(int32_t index);
    void DeleteButtons();
    void Reinit(int32_t palette_from_image);
    void InitPlayerPanel();
    void SetButtonState(int32_t button_index, bool state);
    void ReadIniSettings(int32_t game_state);
    void DrawTextLine(int32_t line_index, char *text, int32_t height, bool horizontal_align);
    void UpdateSaveSettings(struct SaveFileInfo *save_file_info);
    void SetClans(int32_t team_clan);
    void UpdateMenuButtonStates();
    int32_t IsAllowedToStartGame();
    void NetSync(int32_t player_team);
    void NetSync(struct SaveFileInfo &save_file_info);
    TextEdit *CreateTextEdit(int32_t index);
    void DrawTextWindow();
    void DrawJars();
    void DrawJoinScreen();
    void InitJoinScreen();

public:
    WindowInfo *window;
    int32_t key;
    char is_host_mode;
    char connection_state;
    char client_state;
    uint16_t host_node;
    char is_gui_update_needed;
    char remote_player_count;
    int8_t player_team;
    char player_clan;
    char is_incompatible_save_file;
    uint16_t player_node;
    Button *buttons[NETWORK_MENU_ITEM_COUNT];
    NetworkMenuItem menu_items[NETWORK_MENU_ITEM_COUNT];
    Image *images[NETWORK_MENU_IMAGE_COUNT];
    TextEdit *text_edit1;
    TextEdit *text_edit2;
    TextEdit *text_edit3;
    uint16_t team_nodes[TRANSPORT_MAX_TEAM_COUNT];
    int16_t team_clans[TRANSPORT_MAX_TEAM_COUNT];
    char team_jar_in_use[TRANSPORT_MAX_TEAM_COUNT];
    int32_t multi_scenario_id;
    uint32_t rng_seed;
    int16_t minimap_world_index;
    Image *minimap_bg_image;
    int32_t ini_world_index;
    int32_t ini_play_mode;
    int32_t ini_timer;
    int32_t ini_endturn;
    int32_t ini_opponent;
    int32_t ini_start_gold;
    int32_t ini_raw_resource;
    int32_t ini_fuel_resource;
    int32_t ini_gold_resource;
    int32_t ini_alien_derelicts;
    int32_t ini_victory_type;
    int32_t ini_victory_limit;
    char is_map_changed;
    char is_multi_scenario;
    std::string player_name;
    char world_name[30];
    char default_team_names[TRANSPORT_MAX_TEAM_COUNT][30];
    char team_names[TRANSPORT_MAX_TEAM_COUNT][30];
    char text_buffer[150];
    char chat_input_buffer[150];
    char chat_message_buffer[150];

    void Init();
    void Deinit();
    void DrawScreen();
    int32_t SetupScenario(int32_t mode);
    void LeaveGame(uint16_t team_node);
    void EventEditPlayerName();
    void EventSelectClan();
    void EventTextWindow();
    void EventMapButton();
    void EventLoadButton();
    void EventScenarioButton();
    void EventSetJar();
    void ResetJar(int32_t team);
    void EventChat();
    void EventOptions();
    void EventCancel();
    void EventHelp();
    void EventReady();
    void EventStart();
    void LeaveEditField();
};

bool NetworkMenu_MenuLoop(bool is_host_mode);

#endif /* NETWORKMENU_HPP */
