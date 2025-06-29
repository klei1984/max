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
#include "saveloadmenu.hpp"
#include "textedit.hpp"
#include "transport.hpp"

#define NETWORK_MENU_ITEM_COUNT 24
#define NETWORK_MENU_IMAGE_COUNT 14
#define NETWORK_MAX_HOSTS_PER_PAGE 8

class NetworkMenu;

struct NetworkMenuItem {
    int32_t r_value;
    int32_t event_code;
    void (NetworkMenu::*event_handler)();
};

class NetworkMenu {
    void ButtonInit(int32_t index);
    void DeleteButtons();
    void Reinit(int32_t palette_from_image);
    void InitPlayerPanel();
    void SetButtonState(int32_t button_index, bool state);
    void ReadIniSettings(int32_t game_state);
    void DrawTextLine(int32_t line_index, char *text, int32_t height, bool horizontal_align);
    void UpdateSaveSettings(struct SaveFileInfo *save_file_header);
    void SetClans(int32_t team_clan);
    void UpdateMenuButtonStates();
    int32_t IsAllowedToStartGame();
    void NetSync(int32_t player_team);
    void NetSync(struct SaveFileInfo &save_file_header);
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
    char multi_scenario_id;
    uint32_t rng_seed;
    int16_t minimap_world_index;
    Image *minimap_bg_image;
    uint8_t ini_world_index;
    char ini_play_mode;
    int16_t ini_timer;
    int16_t ini_endturn;
    char ini_opponent;
    int16_t ini_start_gold;
    char ini_raw_resource;
    char ini_fuel_resource;
    char ini_gold_resource;
    char ini_alien_derelicts;
    char ini_victory_type;
    int16_t ini_victory_limit;
    char is_map_changed;
    char is_multi_scenario;
    char player_name[30];
    char world_name[30];
    char default_team_names[TRANSPORT_MAX_TEAM_COUNT][30];
    char team_names[TRANSPORT_MAX_TEAM_COUNT][30];
    char text_buffer[120];
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
