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
    int r_value;
    int event_code;
    void (NetworkMenu::*event_handler)();
};

class NetworkMenu {
    void ButtonInit(int index);
    void DeleteButtons();
    void Reinit(int palette_from_image);
    void InitPlayerPanel();
    void SetButtonState(int button_index, bool state);
    void ReadIniSettings(int game_state);
    void DrawTextLine(int line_index, char *text, int height, bool horizontal_align);
    void UpdateSaveSettings(struct SaveFormatHeader *save_file_header);
    void SetClans(int team_clan);
    void UpdateMenuButtonStates();
    int IsAllowedToStartGame();
    void NetSync(int player_team);
    void NetSync(struct SaveFormatHeader &save_file_header);
    TextEdit *CreateTextEdit(int index);
    void DrawTextWindow();
    void DrawJars();
    void DrawJoinScreen();
    void InitJoinScreen();

public:
    WindowInfo *window;
    int key;
    char is_host_mode;
    char connection_state;
    char client_state;
    unsigned short host_node;
    char is_gui_update_needed;
    char remote_player_count;
    char player_team;
    char player_clan;
    char is_incompatible_save_file;
    unsigned short player_node;
    Button *buttons[NETWORK_MENU_ITEM_COUNT];
    NetworkMenuItem menu_items[NETWORK_MENU_ITEM_COUNT];
    Image *images[NETWORK_MENU_IMAGE_COUNT];
    TextEdit *text_edit1;
    TextEdit *text_edit2;
    TextEdit *text_edit3;
    unsigned short team_nodes[TRANSPORT_MAX_TEAM_COUNT];
    short team_clans[TRANSPORT_MAX_TEAM_COUNT];
    char team_jar_in_use[TRANSPORT_MAX_TEAM_COUNT];
    char multi_scenario_id;
    unsigned int rng_seed;
    short minimap_world_index;
    char ini_world_index;
    char ini_play_mode;
    short ini_timer;
    short ini_endturn;
    char ini_opponent;
    short ini_start_gold;
    char ini_raw_resource;
    char ini_fuel_resource;
    char ini_gold_resource;
    char ini_alien_derelicts;
    char ini_victory_type;
    short ini_victory_limit;
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
    int SetupScenario(int mode);
    void LeaveGame(unsigned short team_node);
    void EventEditPlayerName();
    void EventSelectClan();
    void EventTextWindow();
    void EventMapButton();
    void EventLoadButton();
    void EventScenarioButton();
    void EventSetJar();
    void ResetJar(int team);
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
