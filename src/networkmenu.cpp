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

            NetworkIpxMenu_sub_B417F(false);
            NetworkIpxMenu_sub_B390D();
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
                        NetworkIpxMenu_sub_B2206();
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
                            network_menu.buttons[17]->PlaySound();
                        }

                    } else if (network_menu.key == 1000) {
                        continue;
                    }

                    network_menu.text_edit3->ProcessKeyPress(GNW_KB_KEY_RETURN);
                    NetworkIpxMenu_sub_B2206();

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
                        if ((i >= 13 && i <= 16) || i == 17) {
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
            Remote_SendNetPacket_13();

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

    text_font((index < 19) ? 5 : 1);

    if (index == 1) {
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

    if (index >= 13 && index <= 16) {
        buttons[index]->CopyUp(CH_NON_D);
        buttons[index]->CopyUpDisabled(CH_NON_D);
        buttons[index]->CopyDown(CH_HUM_D);
        buttons[index]->CopyDownDisabled(CH_HUM_D);
        buttons[index]->SetFlags(1);
        buttons[index]->SetPValue(1000 + index);

    } else if (index == 17) {
        buttons[index]->SetFlags(1);
        buttons[index]->SetRValue(1000 + index);
        buttons[index]->SetPValue(1000 + index);

    } else {
        buttons[index]->SetFlags(0);
        buttons[index]->SetRValue(1000 + index);
        buttons[index]->SetPValue(GNW_INPUT_PRESS + index);
    }

    if (index == 23 || index == 22 || index == 17) {
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

    NetworkIpxMenu_sub_B4119(ini_get_setting(INI_PLAYER_CLAN));
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

    network_menu_titles[1].title = player_name;

    for (int i = 0; i < 8; ++i) {
        host_node_ids[i] = 0;
        host_names[i][0] = '\0';

        if (i < 4) {
            team_nodes[i] = 0;
            team_names[i][0] = '\0';
            team_jar_in_use[i] = 0;
            network_menu_titles[i + 3].title = team_names[i];
        }
    }

    chat_input_buffer[0] = '\0';
    network_menu_titles[7].title = chat_input_buffer;

    chat_message_buffer[0] = '\0';
    network_menu_titles[8].title = chat_message_buffer;

    text_edit1 = NetworkIpxMenu_create_text_edit_object(0);
    text_edit2 = NetworkIpxMenu_create_text_edit_object(18);
    text_edit3 = nullptr;

    if (is_host_mode) {
        ReadIniSettings(GAME_STATE_6);

        for (int i = 0; i < 4; ++i) {
            default_team_name[i][0] = '\0';
        }

        host_node = (dos_rand() * 31991) >> 15 + 10;
        node = host_node;

    } else {
        host_node = 0;
    }

    minimap_world_index = -1;
    NetworkIpxMenu_sub_B390D();
    Remote_sub_CAC41();

    if (is_host_mode) {
        NetworkIpxMenu_sub_B3D56(0);
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

void NetworkMenu::EventEditPlayerName() {}

void NetworkMenu::EventSelectClan() {}

void NetworkMenu::EventTextWindow() {}

void NetworkMenu::EventMapButton() {}

void NetworkMenu::EventLoadButton() {}

void NetworkMenu::EventScenarioButton() {}

void NetworkMenu::EventSetJar() {}

void NetworkMenu::EventChat() {}

void NetworkMenu::EventOptions() {}

void NetworkMenu::EventCancel() {
    if (node) {
        Remote_SendNetPacket_28_31(31, host_node, 0);
    }

    GUI_GameState = GAME_STATE_3_MAIN_MENU;
    key = GNW_KB_KEY_ESCAPE;
}

void NetworkMenu::EventHelp() { HelpMenu_Menu(HELPMENU_MULTI_PLAYER_SETUP, GWINDOW_MAIN_WINDOW); }

void NetworkMenu::EventReady() {}

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

        buffer_position =
            &window->buffer[network_menu_titles[13].bounds.ulx + network_menu_titles[13].bounds.uly * window->width];
        Menu_LoadPlanetMinimap(minimap_world_index, buffer_position, window->width);
    }

    UpdateMenuButtonStates();

    win_draw(window->id);

    if (is_host_mode || node) {
        for (int i = 0; i < 4; ++i) {
            int button_index = i + 13;

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

    menu_draw_menu_title(window, &network_menu_titles[0], 0xA2);
    menu_draw_menu_title(window, &network_menu_titles[1], 0xA2, true);

    SetButtonState(0, true);

    delete buttons[1];
    buttons[1] = nullptr;

    SetButtonState(1, true);

    if (is_multi_scenario) {
        buttons[1]->Disable(false);
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
    MenuTitleItem *menu_item = &network_menu_titles[2];
    int ulx;
    int uly;
    int width;

    ulx = menu_item->bounds.ulx + 5;
    uly = line_index * height + menu_item->bounds.uly;
    width = menu_item->bounds.lrx - ulx;

    Text_TextBox(window->buffer, window->width, text, ulx, uly, width, height, 0x2, horizontal_align);
}

void NetworkMenu::EventStart() {}

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
