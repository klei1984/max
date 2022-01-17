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
#include "textedit.hpp"

#define NETWORK_MENU_ITEM_COUNT 24
#define NETWORK_MENU_IMAGE_COUNT 14

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

public:
    WindowInfo *window;
    int key;
    char is_host_mode;
    char connection_state;
    char client_state;
    short node;
    char is_gui_update_needed;
    char remote_player_count;
    char hosts_online;
    char player_team;
    char player_clan;
    char is_incompatible_save_file;
    short host_node;
    Button *buttons[NETWORK_MENU_ITEM_COUNT];
    NetworkMenuItem menu_items[NETWORK_MENU_ITEM_COUNT];
    TextEdit *text_edit1;
    TextEdit *text_edit2;
    TextEdit *text_edit3;
    char text_buffer[120];
    char player_name[30];
    char host_names[8][30];
    short host_node_ids[8];
    char team_names[4][30];
    char chat_input_buffer[150];
    char chat_message_buffer[150];
    short team_nodes[4];
    short team_clan[4];
    char team_jar_in_use[4];
    char world_name[30];
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
    char default_team_name[4][30];
    char multi_scenario_id;
    unsigned int rng_seed;
    Image *images[NETWORK_MENU_IMAGE_COUNT];
    short minimap_world_index;

    void Init();
    void Deinit();
    void DrawScreen();
    void EventEditPlayerName();
    void EventSelectClan();
    void EventTextWindow();
    void EventMapButton();
    void EventLoadButton();
    void EventScenarioButton();
    void EventSetJar();
    void EventChat();
    void EventOptions();
    void EventCancel();
    void EventHelp();
    void EventReady();
    void EventStart();
};

bool NetworkMenu_MenuLoop(bool is_host_mode);

#endif /* NETWORKMENU_HPP */
