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
#include <format>

#include "game_manager.hpp"
#include "gameconfigmenu.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "message_manager.hpp"
#include "missionmanager.hpp"
#include "randomizer.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "text.hpp"
#include "window_manager.hpp"

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

bool NetworkMenu_MenuLoop(bool is_host_mode) {
    NetworkMenu network_menu;
    uint32_t time_stamp;
    bool event_release;

    event_release = false;
    time_stamp = timer_get();

    network_menu.is_host_mode = is_host_mode;
    network_menu.Init();

    ResourceManager_GetMissionManager()->LoadMission(MISSION_CATEGORY_MULTI, "MISSION_CATEGORY_MULTI");

    do {
        if (!network_menu.is_host_mode) {
            if (timer_elapsed_time(time_stamp) > 3000) {
                Remote_SendNetPacket_28(0);
                time_stamp = timer_get();
            }
        }

        if (Remote_UiProcessNetPackets()) {
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

            for (int32_t i = 0; i < NETWORK_MENU_ITEM_COUNT; ++i) {
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
                        (network_menu.*network_menu.menu_items[i].event_handler)();
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
            while (GameManager_GameState == GAME_STATE_3_MAIN_MENU) {
                Remote_ProcessNetPackets();

                if (get_input() == GNW_KB_KEY_ESCAPE) {
                    return false;
                }
            }
        }
    }

    return network_menu.connection_state;
}

void NetworkMenu::ButtonInit(int32_t index) {
    NetworkMenuControlItem* control;

    control = &network_menu_controls[index];

    Text_SetFont((index < MENU_CONTROL_OPTIONS_BUTTON) ? GNW_TEXT_FONT_5 : GNW_TEXT_FONT_1);

    if (index == MENU_CONTROL_CLAN_BUTTON) {
        ResourceID clan_logo;
        ResourceID image_id;

        clan_logo = static_cast<ResourceID>(CLN0LOGO + player_clan);
        image_id = control->image_id;

        if (is_multi_scenario) {
            image_id = static_cast<ResourceID>(image_id + 1);
        }

        buttons[index] = new (std::nothrow) Button(image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   WindowManager_ScaleUlx(window, control->bounds.ulx),
                                                   WindowManager_ScaleUly(window, control->bounds.uly));
        buttons[index]->Copy(clan_logo, 41, 40);

    } else if (control->image_id == INVALID_ID) {
        buttons[index] = new (std::nothrow) Button(
            WindowManager_ScaleUlx(window, control->bounds.ulx), WindowManager_ScaleUly(window, control->bounds.uly),
            control->bounds.lrx - control->bounds.ulx, control->bounds.lry - control->bounds.uly);

    } else {
        buttons[index] = new (std::nothrow) Button(control->image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   WindowManager_ScaleUlx(window, control->bounds.ulx),
                                                   WindowManager_ScaleUly(window, control->bounds.uly));

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

    Text_SetFont(GNW_TEXT_FONT_5);
}

void NetworkMenu::Init() {
    Randomizer_SetSeed(time(nullptr));
    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);

    connection_state = false;
    client_state = false;
    host_node = 0;
    is_gui_update_needed = 0;
    remote_player_count = 0;
    player_team = -1;
    is_incompatible_save_file = 0;
    is_multi_scenario = false;
    is_map_changed = 0;

    SetClans(ini_get_setting(INI_PLAYER_CLAN));
    mouse_hide();
    WindowManager_LoadBigImage(MULTGAME, window, window->width, false, false, -1, -1, true);

    Text_SetFont(GNW_TEXT_FONT_1);
    Text_TextBox(window, _(6637), WindowManager_ScaleUlx(window, 28), WindowManager_ScaleUly(window, 403), 106, 25,
                 true);

    Text_SetFont(GNW_TEXT_FONT_5);

    for (int32_t i = 0; i < NETWORK_MENU_IMAGE_COUNT; ++i) {
        MenuTitleItem* menu_item = &network_menu_titles[i];

        images[i] = new (std::nothrow)
            Image(WindowManager_ScaleUlx(window, menu_item->bounds.ulx),
                  WindowManager_ScaleUly(window, menu_item->bounds.uly),
                  menu_item->bounds.lrx - menu_item->bounds.ulx + 1, menu_item->bounds.lry - menu_item->bounds.uly + 1);

        images[i]->Copy(window);
    }

    for (int32_t i = 0; i < NETWORK_MENU_ITEM_COUNT; ++i) {
        buttons[i] = nullptr;
    }

    {
        char ini_player_name[30];

        ini_config.GetStringValue(INI_PLAYER_NAME, ini_player_name, sizeof(ini_player_name));

        player_name = ini_player_name;
    }

    for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        team_nodes[i] = 0;
        team_names[i][0] = '\0';
        team_jar_in_use[i] = 0;
    }

    chat_input_buffer[0] = '\0';
    chat_message_buffer[0] = '\0';

    text_edit1 = CreateTextEdit(0);
    text_edit2 = CreateTextEdit(18);
    text_edit3 = nullptr;

    if (is_host_mode) {
        ReadIniSettings(GAME_STATE_6);

        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            default_team_names[i][0] = '\0';
        }

        player_node = Randomizer_Generate(31991) + 10;
        host_node = player_node;

    } else {
        player_node = 0;
    }

    minimap_bg_image = new (std::nothrow) Image(
        WindowManager_ScaleUlx(window, network_menu_titles[MENU_ITEM_MAP_PICT].bounds.ulx),
        WindowManager_ScaleUly(window, network_menu_titles[MENU_ITEM_MAP_PICT].bounds.uly),
        network_menu_titles[MENU_ITEM_MAP_PICT].bounds.lrx - network_menu_titles[MENU_ITEM_MAP_PICT].bounds.ulx + 1,
        network_menu_titles[MENU_ITEM_MAP_PICT].bounds.lry - network_menu_titles[MENU_ITEM_MAP_PICT].bounds.uly + 1);

    minimap_bg_image->Copy(window);
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

    for (int32_t i = 0; i < NETWORK_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }

    for (int32_t i = 0; i < NETWORK_MENU_IMAGE_COUNT; ++i) {
        delete images[i];
    }

    delete minimap_bg_image;
}

void NetworkMenu::EventEditPlayerName() {
    SDL_strlcpy(text_buffer, player_name.c_str(), sizeof(text_buffer));

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
    int32_t key_press;
    NetNode* host_node_pointer;

    host_node_pointer = Remote_Hosts[key - 2];
    SDL_assert(host_node_pointer);

    Remote_SendNetPacket_29(host_node_pointer->entity_id);
    images[2]->Write(window);
    win_draw(window->id);

    do {
        if (host_node) {
            break;
        }

        Remote_UiProcessNetPackets();
        key_press = get_input();

    } while (key_press != GNW_KB_KEY_ESCAPE && key_press != 1020);

    DrawScreen();

    if (host_node && !SetupScenario(true) && !is_incompatible_save_file) {
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
        ResourceManager_GetMissionManager()->LoadMission(MISSION_CATEGORY_MULTI, "MISSION_CATEGORY_MULTI");
        UpdateSaveSettings(nullptr);
        SetClans(ini_get_setting(INI_PLAYER_CLAN));
    }

    Reinit(true);
}

void NetworkMenu::EventLoadButton() {
    SaveFileInfo save_file_info;
    int32_t save_slot;

    DeleteButtons();

    save_slot = SaveLoadMenu_MenuLoop(MISSION_CATEGORY_MULTI, false);

    if (save_slot) {
        multi_scenario_id = save_slot;

        ini_set_setting(INI_GAME_FILE_NUMBER, save_slot);

        if (SaveLoad_GetSaveFileInfo(MISSION_CATEGORY_MULTI, save_slot, save_file_info, true)) {
            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                team_clans[i] = save_file_info.team_clan[i];
            }

            ReadIniSettings(GAME_STATE_10);
            UpdateSaveSettings(&save_file_info);

            Reinit(true);
            NetSync(save_file_info);
            DrawScreen();

        } else {
            Reinit(true);
        }

    } else {
        Reinit(true);
    }
}

void NetworkMenu::EventScenarioButton() {
    DeleteButtons();

    if (GameSetupMenu_Menu(MISSION_CATEGORY_MULTI_PLAYER_SCENARIO, false)) {
        const auto team_index = player_team;
        const auto save_slot = ini_get_setting(INI_GAME_FILE_NUMBER);
        SaveFileInfo save_file_info;

        multi_scenario_id = save_slot;

        if (SaveLoad_GetSaveFileInfo(MISSION_CATEGORY_MULTI_PLAYER_SCENARIO, save_slot, save_file_info, true)) {
            save_file_info.save_name = std::format("{} #{}", _(2954), save_slot);

            ini_set_setting(INI_PLAY_MODE, ini_play_mode);
            ini_set_setting(INI_TIMER, ini_timer);
            ini_set_setting(INI_ENDTURN, ini_endturn);

            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                team_clans[i] = save_file_info.team_clan[i];
            }

            ReadIniSettings(GAME_STATE_10);
            UpdateSaveSettings(&save_file_info);

            Reinit(true);
            NetSync(team_index);
            DrawScreen();

        } else {
            Reinit(true);
        }

    } else {
        Reinit(true);
    }
}

void NetworkMenu::EventSetJar() {
    int32_t team_index;
    int32_t button_index;

    team_index = key - MENU_CONTROL_JAR_1;
    button_index = key;

    if (is_incompatible_save_file) {
        MessageManager_DrawMessage(_(cdfc), 0, 1);

        buttons[button_index]->SetRestState(false);

    } else {
        if (player_team != -1) {
            ResetJar(player_team);
            --remote_player_count;
        }

        SDL_strlcpy(team_names[team_index], player_name.c_str(), sizeof(team_names[team_index]));
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
    menu_options_menu_loop(GameManager_GameState == GAME_STATE_6 ? 3 : 4);
    ReadIniSettings(GameManager_GameState);
    Remote_SendNetPacket_37();
    Reinit(false);
}

void NetworkMenu::EventCancel() {
    if (host_node) {
        Remote_SendNetPacket_31(player_node);
    }

    GameManager_GameState = GAME_STATE_3_MAIN_MENU;
    key = GNW_KB_KEY_ESCAPE;
}

void NetworkMenu::EventHelp() {
    HelpMenu_Menu(is_host_mode ? "MULTI_PLAYER_SETUP" : "MULTI_PLAYER_SETUP_CLIENT", WINDOW_MAIN_WINDOW);
}

void NetworkMenu::EventReady() {
    client_state = 2;

    Remote_SendNetPacket_35();
    DrawScreen();
}

void NetworkMenu::DrawScreen() {
    Text_SetFont(GNW_TEXT_FONT_5);
    InitPlayerPanel();

    if (is_host_mode || host_node) {
        DrawTextWindow();
        DrawJars();
    } else {
        InitJoinScreen();
        DrawJoinScreen();
    }

    if (host_node && minimap_world_index != ini_world_index) {
        minimap_bg_image->Write(window);
        minimap_world_index = ini_world_index;

        auto buffer_position =
            &window->buffer[WindowManager_ScaleUlx(window, network_menu_titles[MENU_ITEM_MAP_PICT].bounds.ulx) +
                            WindowManager_ScaleUly(window, network_menu_titles[MENU_ITEM_MAP_PICT].bounds.uly) *
                                window->width];

        Menu_LoadPlanetMinimap(minimap_world_index, buffer_position, window->width);
    }

    UpdateMenuButtonStates();

    win_draw(window->id);

    if (is_host_mode || host_node) {
        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            int32_t button_index = MENU_CONTROL_JAR_1 + i;

            if (team_nodes[i] && buttons[button_index]) {
                buttons[button_index]->SetRestState(true);
                buttons[button_index]->Enable();
                buttons[button_index]->Disable();
            }
        }
    }
}

void NetworkMenu::InitPlayerPanel() {
    images[1]->Write(window);

    network_menu_titles[MENU_ITEM_NAME_FIELD].title = player_name;

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

void NetworkMenu::SetButtonState(int32_t button_index, bool state) {
    if (state) {
        if (!buttons[button_index]) {
            ButtonInit(button_index);
        }

        buttons[button_index]->Enable();
    } else if (buttons[button_index]) {
        buttons[button_index]->Disable();
    }
}

void NetworkMenu::ReadIniSettings(int32_t game_state) {
    GameManager_GameState = game_state;

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

    if (GameManager_GameState == GAME_STATE_6) {
        strcpy(world_name, menu_planet_names[ini_world_index]);
    }
}

void NetworkMenu::DrawTextLine(int32_t line_index, char* text, int32_t height, bool horizontal_align) {
    MenuTitleItem* menu_item = &network_menu_titles[MENU_ITEM_JOIN_TEXT_WINDOW];
    int32_t ulx;
    int32_t uly;
    int32_t width;

    ulx = menu_item->bounds.ulx + 5;
    uly = line_index * height + menu_item->bounds.uly;
    width = menu_item->bounds.lrx - ulx;

    Text_TextBox(window->buffer, window->width, text, WindowManager_ScaleUlx(window, ulx),
                 WindowManager_ScaleUly(window, uly), width, height, COLOR_GREEN, horizontal_align);
}

void NetworkMenu::ResetJar(int32_t team) {
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

void NetworkMenu::UpdateSaveSettings(struct SaveFileInfo* save_file_info) {
    if (save_file_info) {
        is_multi_scenario = true;

        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            SDL_utf8strlcpy(default_team_names[i], save_file_info->team_names[i].c_str(), 30);

            if (!strlen(default_team_names[i]) && (save_file_info->team_type[i] == TEAM_TYPE_PLAYER ||
                                                   save_file_info->team_type[i] == TEAM_TYPE_REMOTE)) {
                strcpy(default_team_names[i], _(0cb7));
            }
        }

        rng_seed = save_file_info->random_seed;
        SDL_utf8strlcpy(world_name, save_file_info->save_name.c_str(), 30);

    } else {
        is_multi_scenario = false;

        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            default_team_names[i][0] = '\0';
        }
    }

    is_map_changed = true;

    Remote_SendNetPacket_37();
}

void NetworkMenu::SetClans(int32_t team_clan) {
    player_clan = team_clan;

    if (!is_multi_scenario) {
        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            team_clans[i] = team_clan;
        }
    }
}

int32_t NetworkMenu::SetupScenario(int32_t mode) {
    SaveFileInfo save_file_info;
    int32_t result;

    if (!is_multi_scenario) {
        SetClans(ini_get_setting(INI_PLAYER_CLAN));
        is_incompatible_save_file = false;

        return false;
    }

    if (!mode && !is_map_changed) {
        return false;
    }

    {
        is_incompatible_save_file = true;

        auto file_exists = SaveLoad_GetSaveFileInfo(MISSION_CATEGORY_MULTI, multi_scenario_id, save_file_info, true);

        if (file_exists && rng_seed == save_file_info.random_seed) {
            /// \todo language
            auto mission = std::make_shared<Mission>();

            if (mission && mission->LoadBinaryBuffer(save_file_info.script)) {
                if (ResourceManager_GetMissionManager()->LoadMission(mission)) {
                    ResourceManager_GetMissionManager()->GetMission()->SetMission(save_file_info.file_path);

                    is_incompatible_save_file = false;
                }
            }
        }

        if (is_incompatible_save_file) {
            file_exists = SaveLoad_GetSaveFileInfo(MISSION_CATEGORY_MULTI_PLAYER_SCENARIO, multi_scenario_id,
                                                   save_file_info, true);

            if (file_exists && rng_seed == save_file_info.random_seed) {
                if (ResourceManager_GetMissionManager()->LoadMission(
                        static_cast<MissionCategory>(save_file_info.save_file_category), multi_scenario_id - 1)) {
                    is_incompatible_save_file = false;
                }
            }
        }

        if (is_incompatible_save_file) {
            const auto file_list = ResourceManager_GetFileList(ResourceManager_FilePathGameData, ".mts");

            for (const auto& file_path : file_list) {
                file_exists = SaveLoad_GetSaveFileInfo(file_path, save_file_info, true);

                if (file_exists && rng_seed == save_file_info.random_seed) {
                    const auto mission_hash = Mission::Mission_Sha256(file_path);

                    if (ResourceManager_GetMissionManager()->LoadMission(MISSION_CATEGORY_MULTI_PLAYER_SCENARIO,
                                                                         mission_hash)) {
                        ResourceManager_GetMissionManager()->GetMission()->SetMission(file_path);

                        is_incompatible_save_file = false;
                    }

                    break;
                }
            }
        }
    }

    if (is_incompatible_save_file) {
        MessageManager_DrawMessage(_(4ba6), 0, 1);

        client_state = 0;

        result = false;

    } else {
        const auto mission_category = ResourceManager_GetMissionManager()->GetMission()->GetCategory();

        if (is_map_changed) {
            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                ResetJar(i);
            }

            is_map_changed = false;
        }

        for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
            team_clans[i] = save_file_info.team_clan[i];
        }

        if (mission_category == MISSION_CATEGORY_MULTI) {
            NetSync(save_file_info);

        } else {
            int32_t player_team_local;

            player_team_local = player_team;

            if (player_team_local == -1) {
                player_team_local = 0;
            }

            NetSync(player_team_local);
        }

        result = true;
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

int32_t NetworkMenu::IsAllowedToStartGame() {
    if (player_team == -1 || remote_player_count < 2) {
        return false;
    }

    for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        if (team_nodes[i] && !team_jar_in_use[i]) {
            return false;
        }
    }

    return true;
}

void NetworkMenu::LeaveGame(uint16_t team_node) {
    Remote_Hosts.Remove(team_node);

    for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        if (team_nodes[i] == team_node) {
            ResetJar(i);

            if (is_host_mode) {
                --remote_player_count;
            }

            buttons[MENU_CONTROL_JAR_1 + i]->CopyDown(CH_HUM_D);
            buttons[MENU_CONTROL_JAR_1 + i]->CopyDownDisabled(CH_HUM_D);
        }
    }

    is_gui_update_needed = true;
}

void NetworkMenu::NetSync(int32_t team) {
    bool game_found;

    Remote_UiProcessNetPackets();

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
            Remote_UiProcessNetPackets();

            if (!host_node) {
                break;
            }

            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                if (team_nodes[i] == host_node) {
                    game_found = true;
                }
            }
        }
    }

    if (game_found) {
        int32_t index = TRANSPORT_MAX_TEAM_COUNT;

        if (is_multi_scenario) {
            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                if (!SDL_strcasecmp(player_name.c_str(), default_team_names[i]) && !team_nodes[i]) {
                    index = i;
                    break;
                }
            }
        }

        if (index == TRANSPORT_MAX_TEAM_COUNT && !team_nodes[team] &&
            (!is_multi_scenario || default_team_names[team][0] != '\0')) {
            index = team;
        }

        if (index == TRANSPORT_MAX_TEAM_COUNT) {
            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                if (!team_nodes[i] && (!is_multi_scenario || default_team_names[i][0] != '\0')) {
                    index = i;
                    break;
                }
            }
        }

        if (index != TRANSPORT_MAX_TEAM_COUNT) {
            key = MENU_CONTROL_JAR_1 + index;
            EventSetJar();
        }
    }
}

void NetworkMenu::NetSync(struct SaveFileInfo& save_file_info) {
    int32_t index;

    for (index = 0; index < 3 && save_file_info.team_type[index] != TEAM_TYPE_PLAYER; ++index) {
    }

    NetSync(index);
}

void NetworkMenu::EventStart() {
    Remote_SetupConnection();

    key = GNW_KB_KEY_ESCAPE;
}

void NetworkMenu::DeleteButtons() {
    for (int32_t i = 0; i < NETWORK_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
        buttons[i] = nullptr;
    }
}

void NetworkMenu::Reinit(int32_t palette_from_image) {
    mouse_hide();
    WindowManager_LoadBigImage(MULTGAME, window, window->width, palette_from_image, false, -1, -1, true);
    Text_SetFont(GNW_TEXT_FONT_1);
    Text_TextBox(window, _(90a9), WindowManager_ScaleUlx(window, 28), WindowManager_ScaleUly(window, 403), 106, 25,
                 true);
    minimap_world_index = -1;
    DrawScreen();
    mouse_show();
}

TextEdit* NetworkMenu::CreateTextEdit(int32_t index) {
    NetworkMenuControlItem* control;
    TextEdit* text_edit;

    control = &network_menu_controls[index];

    text_buffer[0] = '\0';
    text_edit = new (std::nothrow)
        TextEdit(window, text_buffer, index == 18 ? 120 : 30, WindowManager_ScaleUlx(window, control->bounds.ulx),
                 WindowManager_ScaleUly(window, control->bounds.uly), control->bounds.lrx - control->bounds.ulx + 1,
                 control->bounds.lry - control->bounds.uly + 1, 0xA2, GNW_TEXT_FONT_5);
    text_edit->LoadBgImage();

    return text_edit;
}

void NetworkMenu::DrawTextWindow() {
    int32_t line_index;
    int32_t height;
    char text[90];
    char text_ini_timer[10];
    char text_ini_endturn[10];

    line_index = 0;
    height = 10;

    images[2]->Write(window);
    strcpy(text, _(a661));

    DrawTextLine(line_index, text, height, true);
    ++line_index;

    if (is_multi_scenario) {
        sprintf(text, _(b764), menu_planet_names[ini_world_index]);
        DrawTextLine(line_index, text, height, false);
        ++line_index;

        sprintf(text, _(6c04), world_name);
        DrawTextLine(line_index, text, height, false);
        ++line_index;

    } else {
        sprintf(text, _(c56b), world_name);
        DrawTextLine(line_index, text, height, false);
        ++line_index;

        sprintf(text, _(1c1d), "");
        DrawTextLine(line_index, text, height, false);
        ++line_index;
    }

    sprintf(text, _(af71), ini_play_mode == 0 ? _(ec45) : _(3d0b));
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    snprintf(text_ini_timer, 10, "%d", ini_timer);
    snprintf(text_ini_endturn, 10, "%d", ini_endturn);

    sprintf(text, _(fc1c), text_ini_timer, text_ini_endturn);
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, _(1c7d), ini_start_gold);
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, _(d895), network_config_menu_items[ini_raw_resource].title.c_str());
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, _(85cd), network_config_menu_items[ini_fuel_resource].title.c_str());
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, _(d46a), network_config_menu_items[ini_gold_resource].title.c_str());
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, _(dd95), network_config_menu_items[ini_alien_derelicts].title.c_str());
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    sprintf(text, _(961e), ini_victory_limit, ini_victory_type ? _(e878) : _(97c6));
    DrawTextLine(line_index, text, height, false);
    ++line_index;

    if (is_host_mode) {
        SetButtonState(MENU_CONTROL_OPTIONS_BUTTON, true);
        SetButtonState(MENU_CONTROL_MAP_BUTTON, true);
        SetButtonState(MENU_CONTROL_LOAD_BUTTON, true);
        SetButtonState(MENU_CONTROL_SCENARIO_BUTTON, true);
    }

    for (int32_t i = MENU_CONTROL_TEXT_WINDOW; i < MENU_CONTROL_TEXT_WINDOW + 8; ++i) {
        SetButtonState(i, false);
    }
}

void NetworkMenu::DrawJars() {
    int32_t button_index;

    if (is_map_changed) {
        is_map_changed = false;

        if (is_multi_scenario) {
            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                ResetJar(i);
            }

            remote_player_count = 0;
            player_team = -1;

        } else {
            for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
                if (!team_nodes[i]) {
                    team_names[i][0] = '\0';
                }
            }
        }
    }

    for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
        button_index = MENU_CONTROL_JAR_1 + i;

        images[3 + i]->Write(window);
        images[9 + i]->Write(window);

        if (is_multi_scenario && default_team_names[i][0] == '\0') {
            delete buttons[button_index];
            buttons[button_index] = nullptr;

        } else {
            network_menu_titles[MENU_ITEM_TEXT_WINDOW + i].title = team_names[i];

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

    network_menu_titles[MENU_ITEM_CHAT_BAR_ONE].title = chat_input_buffer;

    menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_CHAT_BAR_ONE], 0xA2, true);

    images[8]->Write(window);

    network_menu_titles[MENU_ITEM_CHAT_BAR_TWO].title = chat_message_buffer;

    menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_CHAT_BAR_TWO], 0x2);

    SetButtonState(MENU_CONTROL_CHAT_BAR_ONE, true);
    SetButtonState(MENU_CONTROL_CHAT_BAR_TWO, true);
}

void NetworkMenu::DrawJoinScreen() {
    int32_t index;

    images[2]->Write(window);

    if (Remote_Hosts.GetCount()) {
        for (int32_t i = 0; i < NETWORK_MAX_HOSTS_PER_PAGE; ++i) {
            index = MENU_CONTROL_TEXT_WINDOW + i;
            NetNode* host_node_pointer = Remote_Hosts[i];

            if (host_node_pointer) {
                DrawTextLine(i, host_node_pointer->name, 20, false);
                SetButtonState(index, true);
            } else {
                SetButtonState(index, false);
            }

            if (i < NETWORK_MAX_HOSTS_PER_PAGE - 1) {
                struct NetworkMenuControlItem* control = &network_menu_controls[index];

                draw_line(window->buffer, window->width, WindowManager_ScaleUlx(window, control->bounds.ulx),
                          WindowManager_ScaleUly(window, control->bounds.lry),
                          WindowManager_ScaleLrx(window, control->bounds.ulx, control->bounds.lrx),
                          WindowManager_ScaleLry(window, control->bounds.uly, control->bounds.lry), COLOR_GREEN);
            }
        }

    } else {
        menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_JOIN_TEXT_WINDOW], 0xA2, true);

        for (int32_t i = MENU_CONTROL_TEXT_WINDOW; i < MENU_CONTROL_TEXT_WINDOW + NETWORK_MAX_HOSTS_PER_PAGE; ++i) {
            SetButtonState(i, false);
        }
    }
}

void NetworkMenu::InitJoinScreen() {
    int32_t button_index;

    for (int32_t i = 0; i < TRANSPORT_MAX_TEAM_COUNT; ++i) {
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

    for (int32_t i = 2; i < 14; ++i) {
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
            player_name = text_buffer;

            if (player_name.empty()) {
                player_name = _(0cb7);
            }

            images[1]->Write(window);

            network_menu_titles[MENU_ITEM_NAME_FIELD].title = player_name;

            menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_NAME_FIELD], 0xA2, true);

            {
                char ini_player_name[30];

                SDL_strlcpy(ini_player_name, player_name.c_str(), sizeof(ini_player_name));

                ini_config.SetStringValue(INI_PLAYER_NAME, ini_player_name);
            }

            if (player_team != -1) {
                images[3 + player_team]->Write(window);
                SDL_strlcpy(team_names[player_team], player_name.c_str(), sizeof(team_names[player_team]));

                network_menu_titles[MENU_ITEM_TEXT_WINDOW + player_team].title = team_names[player_team];

                menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_TEXT_WINDOW + player_team], 0xA2, false);

                Remote_SendNetPacket_36();
            }

        } else {
            if (key == GNW_KB_KEY_ESCAPE) {
                text_buffer[0] = '\0';
            }

            images[7]->Write(window);
            strcpy(chat_input_buffer, text_buffer);

            network_menu_titles[MENU_ITEM_CHAT_BAR_ONE].title = chat_input_buffer;

            menu_draw_menu_title(window, &network_menu_titles[MENU_ITEM_CHAT_BAR_ONE], 0xA2, true);

            Remote_SendNetPacket_33();

            buttons[MENU_CONTROL_CHAT_BAR_ONE]->SetRestState(false);
        }

        text_edit3 = nullptr;
        win_draw(window->id);
    }
}
