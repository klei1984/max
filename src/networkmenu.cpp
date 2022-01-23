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

#include "networkmenu.hpp"

#include <ctime>

#include "gameconfigmenu.hpp"
#include "gui.hpp"
#include "gwindow.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"

struct NetworkMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char *label;
    int event_code;
    void (NetworkMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    { {(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx) }

#define MENU_ITEM_NAME_LABEL 0
#define MENU_ITEM_NAME_FIELD 1
#define MENU_ITEM_JOIN_TEXT_WINDOW 2
#define MENU_ITEM_TEXT_WINDOW 3
#define MENU_ITEM_CHAT_BAR_ONE 7
#define MENU_ITEM_CHAT_BAR_TWO 8
#define MENU_ITEM_MAP_PICT 13

#define MENU_CONTROL_NAME_FIELD 0
#define MENU_CONTROL_CLAN_BUTTON 1
#define MENU_CONTROL_TEXT_WINDOW 2
#define MENU_CONTROL_MAP_BUTTON 10
#define MENU_CONTROL_LOAD_BUTTON 11
#define MENU_CONTROL_SCENARIO_BUTTON 12
#define MENU_CONTROL_JAR_1 13
#define MENU_CONTROL_JAR_2 14
#define MENU_CONTROL_JAR_3 15
#define MENU_CONTROL_JAR_4 16
#define MENU_CONTROL_CHAT_BAR_ONE 17
#define MENU_CONTROL_CHAT_BAR_TWO 18
#define MENU_CONTROL_OPTIONS_BUTTON 19
#define MENU_CONTROL_CANCEL_BUTTON 20
#define MENU_CONTROL_HELP_BUTTON 21
#define MENU_CONTROL_READY_BUTTON 22
#define MENU_CONTROL_START_BUTTON 23

static struct MenuTitleItem network_menu_titles[] = {
    MENU_TITLE_ITEM_DEF(42, 48, 134, 61, "Player Name:"),
    MENU_TITLE_ITEM_DEF(42, 71, 185, 84, nullptr),
    MENU_TITLE_ITEM_DEF(359, 28, 568, 188, "Searching for hosts..."),
    MENU_TITLE_ITEM_DEF(59, 131, 149, 143, nullptr),
    MENU_TITLE_ITEM_DEF(191, 131, 285, 143, nullptr),
    MENU_TITLE_ITEM_DEF(59, 249, 149, 260, nullptr),
    MENU_TITLE_ITEM_DEF(191, 249, 285, 260, nullptr),
    MENU_TITLE_ITEM_DEF(150, 377, 617, 392, nullptr),
    MENU_TITLE_ITEM_DEF(150, 408, 617, 423, nullptr),
    MENU_TITLE_ITEM_DEF(75, 160, 132, 231, nullptr),
    MENU_TITLE_ITEM_DEF(208, 160, 263, 231, nullptr),
    MENU_TITLE_ITEM_DEF(75, 278, 132, 349, nullptr),
    MENU_TITLE_ITEM_DEF(208, 278, 263, 349, nullptr),
    MENU_TITLE_ITEM_DEF(355, 228, 469, 342, nullptr),
};

static struct NetworkMenuControlItem network_menu_controls[] = {
    MENU_CONTROL_DEF(42, 71, 185, 84, INVALID_ID, nullptr, 0, NetworkMenu::EventEditPlayerName, MENUOP),
    MENU_CONTROL_DEF(211, 25, 0, 0, M_CLAN_U, nullptr, 0, NetworkMenu::EventSelectClan, MENUOP),
    MENU_CONTROL_DEF(359, 28, 568, 48, INVALID_ID, nullptr, 0, NetworkMenu::EventTextWindow, MENUOP),
    MENU_CONTROL_DEF(359, 48, 568, 68, INVALID_ID, nullptr, 0, NetworkMenu::EventTextWindow, MENUOP),
    MENU_CONTROL_DEF(359, 68, 568, 88, INVALID_ID, nullptr, 0, NetworkMenu::EventTextWindow, MENUOP),
    MENU_CONTROL_DEF(359, 88, 568, 108, INVALID_ID, nullptr, 0, NetworkMenu::EventTextWindow, MENUOP),
    MENU_CONTROL_DEF(359, 108, 568, 128, INVALID_ID, nullptr, 0, NetworkMenu::EventTextWindow, MENUOP),
    MENU_CONTROL_DEF(359, 128, 568, 148, INVALID_ID, nullptr, 0, NetworkMenu::EventTextWindow, MENUOP),
    MENU_CONTROL_DEF(359, 148, 568, 168, INVALID_ID, nullptr, 0, NetworkMenu::EventTextWindow, MENUOP),
    MENU_CONTROL_DEF(359, 168, 568, 188, INVALID_ID, nullptr, 0, NetworkMenu::EventTextWindow, MENUOP),
    MENU_CONTROL_DEF(516, 232, 0, 0, SBLNK_UP, "Maps", 0, NetworkMenu::EventMapButton, MENUOP),
    MENU_CONTROL_DEF(516, 268, 0, 0, SBLNK_UP, "Load", 0, NetworkMenu::EventLoadButton, MENUOP),
    MENU_CONTROL_DEF(516, 304, 0, 0, SBLNK_UP, "Scenarios", 0, NetworkMenu::EventScenarioButton, MENUOP),
    MENU_CONTROL_DEF(75, 160, 0, 0, CH_NON_U, nullptr, 0, NetworkMenu::EventSetJar, MENUOP),
    MENU_CONTROL_DEF(208, 160, 0, 0, CH_NON_U, nullptr, 0, NetworkMenu::EventSetJar, MENUOP),
    MENU_CONTROL_DEF(75, 278, 0, 0, CH_NON_U, nullptr, 0, NetworkMenu::EventSetJar, MENUOP),
    MENU_CONTROL_DEF(208, 278, 0, 0, CH_NON_U, nullptr, 0, NetworkMenu::EventSetJar, MENUOP),
    MENU_CONTROL_DEF(33, 369, 0, 0, SBLNK_UP, "Chat", 0, NetworkMenu::EventChat, MENUOP),
    MENU_CONTROL_DEF(150, 377, 617, 392, INVALID_ID, 0, 0, NetworkMenu::EventChat, MENUOP),
    MENU_CONTROL_DEF(243, 438, 0, 0, MNUBTN3U, "Options", 0, NetworkMenu::EventOptions, MENUOP),
    MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, "Cancel", GNW_KB_KEY_ESCAPE, NetworkMenu::EventCancel, NCANC0),
    MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, "?", GNW_KB_KEY_SHIFT_DIVIDE, NetworkMenu::EventHelp, MENUOP),
    MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, "Ready", GNW_KB_KEY_RETURN, NetworkMenu::EventReady, NDONE0),
    MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, "Start", GNW_KB_KEY_RETURN, NetworkMenu::EventStart, NDONE0),
};

bool NetworkMenu_MenuLoop(bool is_host_mode) {
    NetworkMenu network_menu;
    unsigned int time_stamp;
    bool event_release;

    event_release = false;
    time_stamp = timer_get_stamp32();

    network_menu.is_host_mode = is_host_mode;
    network_menu.Init();

    do {
        if (network_menu.is_host_mode) {
            if (timer_elapsed_time_ms(time_stamp) > 3000) {
                Remote_SendNetPacket_44();
                time_stamp = timer_get_stamp32();
            }
        }

        if (Remote_sub_CAC94()) {
            network_menu.is_gui_update_needed = false;

            network_menu.SetupScenario(false);
            network_menu.DrawScreen();
        }

        network_menu.key = get_input();

        if (network_menu.key > 0 && network_menu.key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        if (network_menu.connection_state == 1) {
            network_menu.key = GNW_KB_KEY_ESCAPE;
            continue;
        }

        if (Remote_GameState != 2) {
            if (network_menu.text_edit3) {
                if (network_menu.text_edit3->ProcessKeyPress(network_menu.key)) {
                    if (network_menu.key == GNW_KB_KEY_ESCAPE || network_menu.key == GNW_KB_KEY_RETURN) {
                        network_menu.LeaveEditField();
                    }

                    network_menu.key = 0;
                    continue;
                }

                if (network_menu.key > 0) {
                    if (network_menu.text_edit3 == network_menu.text_edit2) {
                        if (network_menu.key == 1018 || network_menu.key == GNW_INPUT_PRESS + 18) {
                            continue;
                        }

                        if (network_menu.key == 1017) {
                            network_menu.key = 0;
                            network_menu.buttons[MENU_CONTROL_CHAT_BAR_ONE]->PlaySound();
                        }

                    } else if (network_menu.key == 1000) {
                        continue;
                    }

                    network_menu.text_edit3->ProcessKeyPress(GNW_KB_KEY_RETURN);
                    network_menu.LeaveEditField();

                    if (!network_menu.key) {
                        continue;
                    }
                }
            }

            for (int i = 0; i < NETWORK_MENU_ITEM_COUNT; ++i) {
                if (network_menu.buttons[i]) {
                    if (network_menu.key == GNW_INPUT_PRESS + i) {
                        if (!event_release) {
                            network_menu.buttons[i]->PlaySound();
                        }

                        event_release = 1;
                        break;
                    }

                    if (network_menu.key == network_menu.menu_items[i].event_code) {
                        network_menu.key = network_menu.menu_items[i].r_value;
                    }

                    if (network_menu.key == network_menu.menu_items[i].r_value) {
                        if ((i >= MENU_CONTROL_JAR_1 && i <= MENU_CONTROL_JAR_4) || i == MENU_CONTROL_CHAT_BAR_ONE) {
                            network_menu.buttons[i]->PlaySound();
                        }

                        network_menu.key -= 1000;
                        network_menu.menu_items[i].event_handler();
                        break;
                    }
                }
            }
        }

    } while (network_menu.key != GNW_KB_KEY_ESCAPE);

    network_menu.Deinit();

    if (network_menu.connection_state) {
        if (is_host_mode) {
            Remote_RngSeed = time(nullptr);
            Remote_SendNetPacket_13(Remote_RngSeed);

        } else {
            while (GUI_GameState == GAME_STATE_3_MAIN_MENU) {
                Remote_ProcessNetPackets();

                if (get_input() == GNW_KB_KEY_ESCAPE) {
                    return false;
                }
            }
        }
    }

    return network_menu.connection_state;
}

void NetworkMenu::ButtonInit(int index) {
    NetworkMenuControlItem *control;

    control = &network_menu_controls[index];

    text_font((index < MENU_CONTROL_OPTIONS_BUTTON) ? 5 : 1);

    if (index == MENU_CONTROL_CLAN_BUTTON) {
        ResourceID clan_logo;
        ResourceID image_id;

        clan_logo = CLN0LOGO + player_clan;
        image_id = control->image_id;

        if (is_multi_scenario) {
            ++image_id;
        }

        buttons[index] =
            new (std::nothrow) Button(image_id, control->image_id + 1, control->bounds.ulx, control->bounds.uly);
        buttons[index]->Copy(clan_logo, 41, 40);
    } else if (control->image_id == INVALID_ID) {
        buttons[index] = new (std::nothrow)
            Button(control->bounds.ulx, control->bounds.uly, control->bounds.lrx - control->bounds.ulx,
                   control->bounds.lry - control->bounds.uly);
    } else {
        buttons[index] = new (std::nothrow)
            Button(control->image_id, control->image_id + 1, control->bounds.ulx, control->bounds.uly);

        if (control->label) {
            buttons[index]->SetCaption(control->label);
        }
    }

    if (index >= MENU_CONTROL_JAR_1 && index <= MENU_CONTROL_JAR_4) {
        buttons[index]->CopyUp(CH_NON_D);
        buttons[index]->CopyUpDisabled(CH_NON_D);
        buttons[index]->CopyDown(CH_HUM_D);
        buttons[index]->CopyDownDisabled(CH_HUM_D);
        buttons[index]->SetFlags(1);
        buttons[index]->SetPValue(1000 + index);

    } else if (index == MENU_CONTROL_CHAT_BAR_ONE) {
        buttons[index]->SetFlags(1);
        buttons[index]->SetRValue(1000 + index);
        buttons[index]->SetPValue(1000 + index);

    } else {
        buttons[index]->SetFlags(0);
        buttons[index]->SetRValue(1000 + index);
        buttons[index]->SetPValue(GNW_INPUT_PRESS + index);
    }

    if (index == MENU_CONTROL_START_BUTTON || index == MENU_CONTROL_READY_BUTTON ||
        index == MENU_CONTROL_CHAT_BAR_ONE) {
        buttons[index]->CopyDisabled(window);
    }

    buttons[index]->SetSfx(control->sfx);
    buttons[index]->RegisterButton(window->id);

    menu_items[index].r_value = 1000 + index;
    menu_items[index].event_code = control->event_code;
    menu_items[index].event_handler = control->event_handler;

    text_font(5);
}

void NetworkMenu::Init() {
    dos_srand(time(nullptr));
    window = gwin_get_window(GWINDOW_MAIN_WINDOW);

    connection_state = false;
    client_state = false;
    node = 0;
    is_gui_update_needed = 0;
    hosts_online = 0;
    remote_player_count = 0;
    player_team = -1;
    is_incompatible_save_file = 0;
    is_multi_scenario = false;
    is_map_changed = 0;

    SetClans(ini_get_setting(INI_PLAYER_CLAN));
    mouse_hide();
    gwin_load_image(MULTGAME, window, window->width, false, false);

    text_font(1);
    Text_TextBox(window, "Messages:", 28, 403, 106, 25, true);

    text_font(5);

    for (int i = 0; i < NETWORK_MENU_IMAGE_COUNT; ++i) {
        MenuTitleItem *menu_item = &network_menu_titles[i];

        images[i] = new (std::nothrow)
            Image(menu_item->bounds.ulx, menu_item->bounds.uly, menu_item->bounds.lrx - menu_item->bounds.ulx + 1,
                  menu_item->bounds.lry - menu_item->bounds.uly + 1);

        images[i]->Copy(window);
    }

    for (int i = 0; i < NETWORK_MENU_ITEM_COUNT; ++i) {
        buttons[i] = nullptr;
    }

    ini_config.GetStringValue(INI_PLAYER_NAME, player_name, 30);

    network_menu_titles[MENU_ITEM_NAME_FIELD].title = player_name;

    for (int i = 0; i < 8; ++i) {
        host_node_ids[i] = 0;
        host_names[i][0] = '\0';

        if (i < 4) {
            team_nodes[i] = 0;
            team_names[i][0] = '\0';
            team_jar_in_use[i] = 0;
            network_menu_titles[MENU_ITEM_TEXT_WINDOW + i].title = team_names[i];
        }
    }

    chat_input_buffer[0] = '\0';
    network_menu_titles[MENU_ITEM_CHAT_BAR_ONE].title = chat_input_buffer;

    chat_message_buffer[0] = '\0';
    network_menu_titles[MENU_ITEM_CHAT_BAR_TWO].title = chat_message_buffer;

    text_edit1 = CreateTextEdit(0);
    text_edit2 = CreateTextEdit(18);
    text_edit3 = nullptr;

    if (is_host_mode) {
        ReadIniSettings(GAME_STATE_6);

        for (int i = 0; i < 4; ++i) {
            default_team_names[i][0] = '\0';
        }

        player_node = (dos_rand() * 31991) >> 15 + 10;
        node = player_node;

    } else {
        player_node = 0;
    }

    minimap_world_index = -1;
    DrawScreen();
    Remote_RegisterMenu(this);

    if (is_host_mode) {
        NetSync(0);
    }

    mouse_show();
}

void NetworkMenu::Deinit() {
    delete text_edit1;
    delete text_edit2;

    for (int i = 0; i < NETWORK_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }

    for (int i = 0; i < NETWORK_MENU_IMAGE_COUNT; ++i) {
        delete images[i];
    }
}

void NetworkMenu::EventEditPlayerName() {
    strcpy(text_buffer, player_name);

    text_edit3 = text_edit1;
    text_edit3->SetEditedText(text_buffer);
    text_edit3->DrawFullText();
    text_edit3->EnterTextEditField();
}

void NetworkMenu::EventSelectClan() {
    DeleteButtons();
    ini_set_setting(INI_PLAYER_CLAN, menu_clan_select_menu_loop(0));
    SetClans(ini_get_setting(INI_PLAYER_CLAN));
    Reinit(false);
}

void NetworkMenu::EventTextWindow() {
    int key_press;

    Remote_SendNetPacket_29(host_node_ids[key - 2]);
    images[2]->Write(window);
    win_draw(window->id);

    do {
        if (node) {
            break;
        }

        Remote_sub_CAC94();
        key_press = get_input();

    } while (key_press != GNW_KB_KEY_ESCAPE && key_press != 1020);

    DrawScreen();

    if (node && !SetupScenario(true) && !is_incompatible_save_file) {
        NetSync(0);
    }
}

void NetworkMenu::EventMapButton() {
    DeleteButtons();

    if (menu_planet_select_menu_loop()) {
        ini_world_index = ini_get_setting(INI_WORLD);
        strcpy(world_name, menu_planet_names[ini_world_index]);
        ini_setting_victory_type = ini_get_setting(INI_VICTORY_TYPE);
        ini_setting_victory_limit = ini_get_setting(INI_VICTORY_LIMIT);
        ReadIniSettings(GAME_STATE_6);
        UpdateSaveSettings(nullptr);
        SetClans(ini_get_setting(INI_PLAYER_CLAN));
    }

    Reinit(true);
}

void NetworkMenu::EventLoadButton() {
    int save_slot;
    SaveFormatHeader save_file_header;

    DeleteButtons();

    save_slot = SaveLoadMenu_menu_loop(false, false);

    if (save_slot) {
        multi_scenario_id = save_slot;

        ini_set_setting(INI_GAME_FILE_NUMBER, save_slot);
        ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_MULTI);

        SaveLoadMenu_GetSavedGameInfo(save_slot, GAME_TYPE_MULTI, save_file_header);

        for (int i = 0; i < 4; ++i) {
            team_clans[i] = save_file_header.team_clan[i];
        }

        ReadIniSettings(GAME_STATE_10);
        UpdateSaveSettings(&save_file_header);

        Reinit(true);
        NetSync(save_file_header);
        DrawScreen();

    } else {
        Reinit(true);
    }
}

void NetworkMenu::EventScenarioButton() {
    int team_index;
    int ini_game_file_number;
    SaveFormatHeader save_file_header;

    DeleteButtons();

    if (GameSetupMenu_menu_loop(GAME_TYPE_MULTI_PLAYER_SCENARIO, false)) {
    } else {
        Reinit(true);

        team_index = player_team;
        ini_game_file_number = ini_get_setting(INI_GAME_FILE_NUMBER);
        multi_scenario_id = ini_game_file_number;
        SaveLoadMenu_GetSavedGameInfo(ini_game_file_number, GAME_TYPE_MULTI_PLAYER_SCENARIO, save_file_header);

        ini_set_setting(INI_PLAY_MODE, ini_play_mode);
        ini_set_setting(INI_TIMER, ini_timer);
        ini_set_setting(INI_ENDTURN, ini_endturn);

        for (int i = 0; i < 4; ++i) {
            team_clans[i] = save_file_header.team_clan[i];
        }

        ReadIniSettings(GAME_STATE_10);
        UpdateSaveSettings(&save_file_header);

        Reinit(true);
        NetSync(team_index);
        DrawScreen();
    }
}

void NetworkMenu::EventSetJar() {
    int team_index;
    int button_index;

    team_index = key - MENU_CONTROL_JAR_1;
    button_index = key;

    if (is_incompatible_save_file) {
        MessageManager_DrawMessage(
            "The host has selected a load file that is not compatible with the load file on your system. You will not "
            "be able to start this session until the host selects another load file or selects a new world map.",
            0, 1);

        buttons[button_index]->SetRestState(false);

    } else {
        if (player_team != -1) {
            ResetJar(player_team);
            --remote_player_count;
        }

        strcpy(team_names[team_index], player_name);
        team_nodes[team_index] = player_node;
        team_jar_in_use[team_index] = true;

        player_team = team_index;
        player_clan = team_clans[team_index];
        ++remote_player_count;

        if (!client_state) {
            client_state = 1;
        }

        Remote_SendNetPacket_35();
        DrawScreen();
    }
}

void NetworkMenu::EventChat() {
    strcpy(text_buffer, chat_input_buffer);

    text_edit3 = text_edit2;
    text_edit3->SetEditedText(text_buffer);
    text_edit3->DrawFullText();
    text_edit3->EnterTextEditField();

    buttons[MENU_CONTROL_CHAT_BAR_ONE]->SetRestState(true);
}

void NetworkMenu::EventOptions() {
    DeleteButtons();
    menu_options_menu_loop(GUI_GameState == GAME_STATE_6 ? 3 : 4);
    ReadIniSettings(GUI_GameState);
    Remote_SendNetPacket_37();
    Reinit(false);
}

void NetworkMenu::EventCancel() {
    if (node) {
        Remote_SendNetPacket_31(player_node);
    }

    GUI_GameState = GAME_STATE_3_MAIN_MENU;
    key = GNW_KB_KEY_ESCAPE;
}

void NetworkMenu::EventHelp() { HelpMenu_Menu(HELPMENU_MULTI_PLAYER_SETUP, GWINDOW_MAIN_WINDOW); }

void NetworkMenu::EventReady() {
    client_state = 2;

    Remote_SendNetPacket_35();
    DrawScreen();
}

void NetworkMenu::DrawScreen() {
    text_font(5);
    InitPlayerPanel();

    if (is_host_mode || node) {
        DrawTextWindow();
        DrawJars();
    } else {
        InitJoinScreen();
        DrawJoinScreen();
    }

    if (node && minimap_world_index != ini_world_index) {
        char *buffer_position;

        minimap_world_index = ini_world_index;

        buffer_position = &window->buffer[network_menu_titles[MENU_ITEM_MAP_PICT].bounds.ulx +
                                          network_menu_titles[MENU_ITEM_MAP_PICT].bounds.uly * window->width];
        Menu_LoadPlanetMinimap(minimap_world_index, buffer_position, window->width);
    }

    UpdateMenuButtonStates();

    win_draw(window->id);

    if (is_host_mode || node) {
        for (int i = 0; i < 4; ++i) {
            int button_index = MENU_CONTROL_JAR_1 + i;

            if (team_nodes[i]) {
                buttons[button_index]->SetRestState(true);
                buttons[button_index]->Enable();
                buttons[button_index]->Disable();
            }
        }
    }
}

void NetworkMenu::InitPlayerPanel() {
    images[1]->Write(window);

    menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_NAME_LABEL], 0xA2);
    menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_NAME_FIELD], 0xA2, true);

    SetButtonState(MENU_CONTROL_NAME_FIELD, true);

    delete buttons[MENU_CONTROL_CLAN_BUTTON];
    buttons[MENU_CONTROL_CLAN_BUTTON] = nullptr;

    SetButtonState(MENU_CONTROL_CLAN_BUTTON, true);

    if (is_multi_scenario) {
        buttons[MENU_CONTROL_CLAN_BUTTON]->Disable(false);
    }
}

void NetworkMenu::SetButtonState(int button_index, bool state) {
    if (state) {
        if (!buttons[button_index]) {
            ButtonInit(button_index);
        }

        buttons[button_index]->Enable();
    } else if (buttons[button_index]) {
        buttons[button_index]->Disable();
    }
}

void NetworkMenu::ReadIniSettings(int game_state) {
    GUI_GameState = game_state;

    ini_play_mode = ini_get_setting(INI_PLAY_MODE);
    ini_timer = ini_get_setting(INI_TIMER);
    ini_endturn = ini_get_setting(INI_ENDTURN);
    ini_start_gold = ini_get_setting(INI_START_GOLD);
    ini_opponent = ini_get_setting(INI_OPPONENT);
    ini_victory_type = ini_setting_victory_type;
    ini_victory_limit = ini_setting_victory_limit;
    ini_raw_resource = ini_get_setting(INI_RAW_RESOURCE);
    ini_fuel_resource = ini_get_setting(INI_FUEL_RESOURCE);
    ini_gold_resource = ini_get_setting(INI_GOLD_RESOURCE);
    ini_alien_derelicts = ini_get_setting(INI_ALIEN_DERELICTS);
    ini_world_index = ini_get_setting(INI_WORLD);

    if (GUI_GameState == GAME_STATE_6) {
        strcpy(world_name, menu_planet_names[ini_world_index]);
    }
}

void NetworkMenu::DrawTextLine(int line_index, char *text, int height, bool horizontal_align) {
    MenuTitleItem *menu_item = &network_menu_titles[MENU_ITEM_JOIN_TEXT_WINDOW];
    int ulx;
    int uly;
    int width;

    ulx = menu_item->bounds.ulx + 5;
    uly = line_index * height + menu_item->bounds.uly;
    width = menu_item->bounds.lrx - ulx;

    Text_TextBox(window->buffer, window->width, text, ulx, uly, width, height, 0x2, horizontal_align);
}

void NetworkMenu::ResetJar(int team) {
    if (default_team_names[team][0]) {
        strcpy(team_names[team], "<");
        strcat(team_names[team], default_team_names[team]);
        strcat(team_names[team], ">");
    } else {
        strcpy(team_names[team], default_team_names[team]);
    }

    team_nodes[team] = 0;
    team_jar_in_use[team] = false;
}

void NetworkMenu::UpdateSaveSettings(struct SaveFormatHeader *save_file_header) {
    if (save_file_header) {
        is_multi_scenario = true;

        for (int i = 0; i < 4; ++i) {
            strcpy(default_team_names[i], team_names[i]);
        }

        rng_seed = save_file_header->rng_seed;
        strcpy(world_name, save_file_header->save_name);

    } else {
        is_multi_scenario = false;

        for (int i = 0; i < 4; ++i) {
            default_team_names[i][0] = '\0';
        }
    }

    is_map_changed = true;

    Remote_SendNetPacket_37();
}

void NetworkMenu::SetClans(int team_clan) {
    player_clan = team_clan;

    if (!is_multi_scenario) {
        for (int i = 0; i < 4; ++i) {
            team_clans[i] = team_clan;
        }
    }
}

int NetworkMenu::SetupScenario(int mode) {
    SaveFormatHeader save_file_header;
    int result;

    if (!is_multi_scenario) {
        SetClans(ini_get_setting(INI_PLAYER_CLAN));
        is_incompatible_save_file = false;

        return false;
    }

    if (!mode && !is_map_changed) {
        return false;
    }

    ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_MULTI);

    SaveLoadMenu_GetSavedGameInfo(multi_scenario_id, GAME_TYPE_MULTI, save_file_header);

    if (rng_seed != save_file_header.rng_seed) {
        ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_MULTI_PLAYER_SCENARIO);

        SaveLoadMenu_GetSavedGameInfo(multi_scenario_id, GAME_TYPE_MULTI_PLAYER_SCENARIO, save_file_header);
    }

    if (rng_seed == save_file_header.rng_seed) {
        is_incompatible_save_file = false;

        if (is_map_changed) {
            for (int i = 0; i < 4; ++i) {
                ResetJar(i);
            }

            is_map_changed = false;
        }

        for (int i = 0; i < 4; ++i) {
            team_clans[i] = save_file_header.team_clan[i];
        }

        if (ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_MULTI) {
            NetSync(save_file_header);
        } else {
            int player_team_local;

            player_team_local = player_team;

            if (player_team_local == -1) {
                player_team_local = 0;
            }

            NetSync(player_team_local);
        }

        result = true;

    } else {
        MessageManager_DrawMessage(
            "The host has selected a load file that is not compatible with the load file on your system. You will not "
            "be able to start this session until the host selects another load file or selects a new world map.",
            0, 1);

        is_incompatible_save_file = true;
        client_state = 0;

        result = false;
    }

    return result;
}

void NetworkMenu::UpdateMenuButtonStates() {
    SetButtonState(MENU_CONTROL_CANCEL_BUTTON, 1);
    SetButtonState(MENU_CONTROL_HELP_BUTTON, 1);

    if (is_host_mode) {
        SetButtonState(MENU_CONTROL_START_BUTTON, IsAllowedToStartGame());
    } else {
        SetButtonState(MENU_CONTROL_READY_BUTTON, client_state == 1);
    }
}

int NetworkMenu::IsAllowedToStartGame() {
    if (player_team == -1 || remote_player_count < 2) {
        return false;
    }

    for (int i = 0; i < 4; ++i) {
        if (team_nodes[i] && !team_jar_in_use[i]) {
            return false;
        }
    }

    return true;
}

void NetworkMenu::NetSync(int team) {
    bool game_found;

    Remote_sub_CAC94();

    if (player_team != -1) {
        ResetJar(player_team);
        player_team = -1;
        --remote_player_count;
    }

    if (team >= 4) {
        team = 3;
    }

    if (is_host_mode) {
        game_found = true;
    } else {
        game_found = false;

        while (!game_found) {
            Remote_sub_CAC94();

            if (!node) {
                break;
            }

            for (int i = 0; i < 4; ++i) {
                if (team_nodes[i] == node) {
                    game_found = true;
                }
            }
        }
    }

    if (game_found) {
        int index;

        index = 4;

        if (is_multi_scenario) {
            for (int i = 0; i < 4; ++i) {
                if (!stricmp(player_name, default_team_names[i]) && !team_nodes[i]) {
                    index = i;
                    break;
                }
            }
        }

        if (index == 4 && !team_nodes[team] && (!is_multi_scenario || default_team_names[team][0] != '\0')) {
            index = team;
        }

        if (index == 4) {
            for (int i = 0; i < 4; ++i) {
                if (!team_nodes[i] && (!is_multi_scenario || default_team_names[i][0] != '\0')) {
                    index = i;
                    break;
                }
            }
        }

        if (index != 4) {
            key = MENU_CONTROL_JAR_1 + index;
            EventSetJar();
        }
    }
}

void NetworkMenu::NetSync(struct SaveFormatHeader &save_file_header) {
    int index;

    for (index = 0; index < 3 && save_file_header.team_type[index] != TEAM_TYPE_PLAYER; ++index) {
    }

    NetSync(index);
}

void NetworkMenu::EventStart() {
    Remote_SetupConnection();

    key = GNW_KB_KEY_ESCAPE;
}

void NetworkMenu::DeleteButtons() {
    for (int i = 0; i < NETWORK_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
        buttons[i] = nullptr;
    }
}

void NetworkMenu::Reinit(int palette_from_image) {
    mouse_hide();
    gwin_load_image(MULTGAME, window, window->width, palette_from_image, false);
    text_font(1);
    Text_TextBox(window, "Messages:", 28, 403, 106, 25, true);
    DrawScreen();
    mouse_show();
}

TextEdit *NetworkMenu::CreateTextEdit(int index) {
    NetworkMenuControlItem *control;
    TextEdit *text_edit;

    control = &network_menu_controls[index];

    text_buffer[0] = '\0';
    text_edit = new (std::nothrow)
        TextEdit(window, text_buffer, index == 18 ? 120 : 30, control->bounds.ulx, control->bounds.uly,
                 control->bounds.lrx - control->bounds.ulx + 1, control->bounds.lry - control->bounds.uly + 1, 0xA2, 5);
    text_edit->LoadBgImage();

    return text_edit;
}

void NetworkMenu::DrawTextWindow() {
    int line_index;
    int height;
    char text[90];
    char text_ini_timer[10];
    char text_ini_endturn[10];

    line_index = 0;
    height = 10;

    images[2]->Write(window);
    strcpy(text, "Game Setup:");

    DrawTextLine(line_index, text, height, true);
    ++line_index;

    if (is_multi_scenario) {
        sprintf(text, "Map: %s", world_name);
        DrawTextLine(line_index, text, height, false);
        ++line_index;

        sprintf(text, "File: %s", "");
        DrawTextLine(line_index, text, height, false);
        ++line_index;

    } else {
        sprintf(text, "Map: %s", menu_planet_names[ini_world_index]);
        DrawTextLine(line_index, text, height, false);
        ++line_index;

        sprintf(text, "File: %s", world_name);
        DrawTextLine(line_index, text, height, false);
        ++line_index;
    }

    sprintf(text, "Play Mode: %s", ini_play_mode == 0 ? "Turn Based" : "Simultaneous");
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    snprintf(text_ini_timer, 10, "%d", ini_timer);
    snprintf(text_ini_endturn, 10, "%d", ini_endturn);

    sprintf(text, "Turn Time: %s - End Turn: %s", text_ini_timer, text_ini_endturn);
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, "Starting Credit: %i", ini_start_gold);
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, "Raw Resources: %s", game_config_menu_items[47 + ini_raw_resource].title);
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, "Fuel Resources: %s", game_config_menu_items[50 + ini_fuel_resource].title);
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, "Gold Resources: %s", game_config_menu_items[53 + ini_gold_resource].title);
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, "Alien Derelicts: %s", game_config_menu_items[56 + ini_alien_derelicts].title);
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, "Victory: %i %s", ini_victory_limit, ini_victory_type ? "points" : "turns");
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    if (is_host_mode) {
        SetButtonState(MENU_CONTROL_OPTIONS_BUTTON, true);
        SetButtonState(MENU_CONTROL_MAP_BUTTON, true);
        SetButtonState(MENU_CONTROL_LOAD_BUTTON, true);
        SetButtonState(MENU_CONTROL_SCENARIO_BUTTON, true);
    }

    for (int i = MENU_CONTROL_TEXT_WINDOW; i < MENU_CONTROL_TEXT_WINDOW + 8; ++i) {
        SetButtonState(i, false);
    }
}

void NetworkMenu::DrawJars() {
    int button_index;

    if (is_map_changed) {
        is_map_changed = false;

        if (is_multi_scenario) {
            for (int i = 0; i < 4; ++i) {
                ResetJar(i);
            }

            remote_player_count = 0;
            player_team = -1;

        } else {
            for (int i = 0; i < 4; ++i) {
                if (!team_nodes[i]) {
                    team_names[i][0] = '\0';
                }
            }
        }
    }

    for (int i = 0; i < 4; ++i) {
        button_index = MENU_CONTROL_JAR_1 + i;

        images[3 + i]->Write(window);
        images[9 + i]->Write(window);

        if (is_multi_scenario && default_team_names[i][0] != '\0') {
            delete buttons[button_index];
            buttons[button_index] = nullptr;

        } else {
            menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_TEXT_WINDOW + i], 0xA2, false);

            if (!buttons[button_index]) {
                ButtonInit(button_index);
            }

            if (team_nodes[i]) {
                if (team_nodes[i] >= 1 && team_nodes[i] <= 4) {
                    buttons[button_index]->CopyDown(CH_CMP_D);
                    buttons[button_index]->CopyDownDisabled(CH_CMP_D);
                }

            } else {
                buttons[button_index]->Enable();
                buttons[button_index]->SetRestState(false);
            }
        }
    }

    images[7]->Write(window);
    menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_CHAT_BAR_ONE], 0xA2, true);

    images[8]->Write(window);
    menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_CHAT_BAR_TWO], 0x2);

    SetButtonState(MENU_CONTROL_CHAT_BAR_ONE, true);
    SetButtonState(MENU_CONTROL_CHAT_BAR_TWO, true);
}

void NetworkMenu::DrawJoinScreen() {
    int index;

    images[2]->Write(window);

    if (hosts_online) {
        for (int i = 0; i < 8; ++i) {
            index = MENU_CONTROL_TEXT_WINDOW + i;

            DrawTextLine(i, host_names[i], 20, false);

            if (i) {
                draw_line(window->buffer, 640, network_menu_controls[index].bounds.ulx,
                          network_menu_controls[index].bounds.uly, network_menu_controls[index].bounds.lrx,
                          network_menu_controls[index].bounds.lry, 0x2);
            }

            SetButtonState(index, host_node_ids[i] != 0);
        }

    } else {
        menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_JOIN_TEXT_WINDOW], 0xA2, true);

        for (int i = MENU_CONTROL_TEXT_WINDOW; i < MENU_CONTROL_TEXT_WINDOW + 8; ++i) {
            SetButtonState(i, false);
        }
    }
}

void NetworkMenu::InitJoinScreen() {
    int button_index;

    for (int i = 0; i < 4; ++i) {
        button_index = MENU_CONTROL_JAR_1 + i;

        SetButtonState(button_index, false);
        if (buttons[button_index]) {
            buttons[button_index]->CopyDown(CH_HUM_D);
            buttons[button_index]->CopyDownDisabled(CH_HUM_D);
        }
    }

    if (buttons[MENU_CONTROL_CHAT_BAR_ONE]) {
        buttons[MENU_CONTROL_CHAT_BAR_ONE]->Disable();
    }

    if (buttons[MENU_CONTROL_CHAT_BAR_TWO]) {
        buttons[MENU_CONTROL_CHAT_BAR_TWO]->Disable();
    }

    for (int i = 2; i < 14; ++i) {
        images[i]->Write(window);
    }

    chat_input_buffer[0] = '\0';
    chat_message_buffer[0] = '\0';
    minimap_world_index = -1;
}

void NetworkMenu::LeaveEditField() {
    if (text_edit3) {
        text_edit3->AcceptEditedText();
        text_edit3->LeaveTextEditField();

        if (text_edit1 == text_edit3) {
            strcpy(player_name, text_buffer);

            if (!strlen(player_name)) {
                strcpy(player_name, "No Name");
            }

            images[1]->Write(window);
            menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_NAME_FIELD], 0x0A2, true);
            ini_config.SetStringValue(INI_PLAYER_NAME, player_name);

            if (player_team != -1) {
                images[3 + player_team]->Write(window);
                strcpy(team_names[player_team], player_name);
                menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_TEXT_WINDOW + player_team], 0x0A2, false);

                Remote_SendNetPacket_36();
            }

        } else {
            if (key == GNW_KB_KEY_ESCAPE) {
                text_buffer[0] = '\0';
            }

            images[7]->Write(window);
            strcpy(chat_input_buffer, text_buffer);
            menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_CHAT_BAR_ONE], 0x0A2, true);

            Remote_SendNetPacket_33();

            buttons[MENU_CONTROL_CHAT_BAR_ONE]->SetRestState(false);
        }

        text_edit3 = nullptr;
        win_draw(window->id);
    }
}
