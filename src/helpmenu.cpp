/* Copyright (c) 2021 M.A.X. Port Team
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

#include "helpmenu.hpp"

#include "access.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "mouseevent.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "unitstats.hpp"
#include "window_manager.hpp"

const char* help_menu_sections[] = {
    "NEW_GAME_SETUP",    "CLAN_SETUP",        "PLANET_SETUP",        "MULTI_PLAYER_SETUP",
    "STATS_SETUP",       "UPGRADES_SETUP",    "CARGO_SETUP",         "LOAD_SETUP",
    "SAVELOAD_SETUP",    "GAME_SCREEN_SETUP", "ALLOCATE_SETUP",      "DEPOT_SETUP",
    "HANGAR_SETUP",      "DOCK_SETUP",        "TRANSPORT_SETUP",     "TRANSFER_SETUP",
    "SITE_SELECT_SETUP", "RESEARCH_SETUP",    "FACTORY_BUILD_SETUP", "CONSTRUCTOR_BUILD_SETUP",
    "SERIAL_MENU_SETUP", "MODEM_MENU_SETUP",  "CHAT_MENU_SETUP",     "PREFS_MENU_SETUP",
    "SETUP_MENU_SETUP",  "HOT_SEAT_SETUP",    "TRAINING_MENU_SETUP", "STAND_ALONE_MENU_SETUP",
    "OPTIONS_SETUP",     "REPORTS_SETUP",     "CAMPAIGN_MENU",       "MULTI_SCENARIO_MENU",
    "BARRACKS_SETUP",
};

const char* help_menu_keyboard_reference = _(ceb6);

HelpMenu::HelpMenu(uint8_t section, int32_t cursor_x, int32_t cursor_y, int32_t window_id)
    : Window(HELPFRAM, window_id),
      section(section),
      help_cache_use_count(0),
      button_done(nullptr),
      button_keys(nullptr),
      button_up(nullptr),
      button_down(nullptr),
      strings(nullptr),
      string_row_index(0),
      string_row_count(0),
      canvas(nullptr),
      event_click_cancel(false),
      event_click_help(false),
      keys_mode(false),
      window_id(window_id) {
    bool entry_loaded;

    entry_loaded = false;
    file_size = ResourceManager_GetResourceSize(HELP_ENG);
    file = ResourceManager_GetFileHandle(HELP_ENG);

    if (file) {
        WindowManager_ScaleCursor(section, window_id, cursor_x, cursor_y);

        if (SeekSection() && SeekEntry(cursor_x, cursor_y)) {
            if (!ReadEntry()) {
                event_click_cancel = true;
                return;
            }

            entry_loaded = true;
        }
    }

    if (entry_loaded) {
        Cursor_SetCursor(CURSOR_HAND);
        Text_SetFont(GNW_TEXT_FONT_5);
        Add();
        FillWindowInfo(&window);

        canvas = new (std::nothrow) Image(0, 0, window.window.lrx, window.window.lry);
        canvas->Copy(&window);

        if (section == HELPMENU_GAME_SCREEN_SETUP) {
            button_done = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 85, 193);
            button_keys = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 155, 193);

            button_keys->SetCaption(_(f0d6), 2, 2);
            button_keys->SetRValue(1000);
            button_keys->SetPValue(1000);
            button_keys->SetFlags(1);
            button_keys->RegisterButton(window.id);
        } else {
            button_done = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 120, 193);
        }

        button_done->SetCaption(_(4bfa), 2, 2);
        button_done->SetRValue(GNW_KB_KEY_RETURN);
        button_done->SetPValue(GNW_INPUT_PRESS);
        button_done->SetSfx(NDONE0);
        button_done->RegisterButton(window.id);

        help_cache_index = 0;
        ProcessText(help_cache[help_cache_index]);

    } else {
        event_click_help = true;
        event_click_cancel = true;
    }
}

HelpMenu::~HelpMenu() {
    delete button_done;
    delete button_keys;
    delete button_up;
    delete button_down;
    delete canvas;
    delete[] strings;

    while (help_cache_use_count) {
        delete help_cache[--help_cache_use_count];
    }
}

void HelpMenu::GetHotZone(const char* chunk, Rect* hot_zone) const {
    while (*chunk != '{') {
        chunk++;
    }

    chunk++;

    hot_zone->ulx = strtol(chunk, nullptr, 10);

    while (*chunk != ',') {
        chunk++;
    }

    chunk++;

    hot_zone->uly = strtol(chunk, nullptr, 10);

    while (*chunk != ',') {
        chunk++;
    }

    chunk++;

    hot_zone->lrx = strtol(chunk, nullptr, 10);

    while (*chunk != ',') {
        chunk++;
    }

    chunk++;

    hot_zone->lry = strtol(chunk, nullptr, 10);
}

int32_t HelpMenu::ReadNextChunk() {
    int32_t result;
    int32_t size;

    if (file_size > 0) {
        size = file_size + 1;
        if (size > 1000) {
            size = 1000;
        }

        if (fgets(buffer, size, file)) {
            file_size -= strlen(buffer);
            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

int32_t HelpMenu::SeekSection() {
    do {
        if (!ReadNextChunk()) {
            return 0;
        }

    } while (!strstr(buffer, help_menu_sections[section]));

    return 1;
}

int32_t HelpMenu::SeekEntry(int32_t cursor_x, int32_t cursor_y) {
    Rect hot_zone;

    for (;;) {
        if (!ReadNextChunk()) {
            return 0;
        }

        if (strstr(buffer, "[")) {
            return 0;
        }

        if (strstr(buffer, "{")) {
            GetHotZone(buffer, &hot_zone);

            if (cursor_x >= hot_zone.ulx && cursor_x <= hot_zone.lrx && cursor_y >= hot_zone.uly &&
                cursor_y <= hot_zone.lry) {
                break;
            }
        }
    }

    return 1;
}

int32_t HelpMenu::ReadEntry() {
    char* text;

    for (;;) {
        text = new (std::nothrow) char[5000];
        if (text) {
            help_cache[help_cache_use_count] = text;
            ++help_cache_use_count;

            text[0] = '\0';

            for (;;) {
                if (!ReadNextChunk() || strstr(buffer, "[") || strstr(buffer, "{")) {
                    return help_cache[help_cache_use_count - 1] != text;
                } else if (strstr(buffer, "\\p")) {
                    break;
                } else {
                    strcat(text, buffer);
                    text += strlen(buffer) - 2;
                    text[0] = '\0';
                }
            };

        } else {
            return false;
        }
    }
}

void HelpMenu::ProcessText(const char* text) {
    if (strings) {
        delete[] strings;
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    rows_per_page = 160 / Text_GetHeight();

    strings = Text_SplitText(text, rows_per_page * 10, 265, &string_row_count);

    if (string_row_count <= rows_per_page) {
        delete button_up;
        delete button_down;

        button_up = nullptr;
        button_down = nullptr;
    } else if (!button_up && !button_down) {
        button_up = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 20, 197);

        button_up->SetRValue(1001);
        button_up->SetPValue(GNW_INPUT_PRESS + 1001);
        button_up->SetSfx(KCARG0);
        button_up->RegisterButton(window.id);

        button_down = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 40, 197);

        button_down->SetRValue(1002);
        button_down->SetPValue(GNW_INPUT_PRESS + 1002);
        button_down->SetSfx(KCARG0);
        button_down->RegisterButton(window.id);
    }

    DrawText();
}

void HelpMenu::DrawText() {
    uint8_t* buffer_position;
    int32_t text_full_height;
    int32_t row_index_max;

    canvas->Write(&window);

    buffer_position = &window.buffer[window.width * 20 + 20];
    text_full_height = string_row_count * Text_GetHeight();

    if (text_full_height < 160) {
        buffer_position = &buffer_position[window.width * ((160 - text_full_height) / 2)];
    }

    row_index_max = string_row_index + rows_per_page;

    if (row_index_max > string_row_count) {
        row_index_max = string_row_count;
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    for (int32_t row_index = string_row_index; row_index < row_index_max; ++row_index) {
        Text_Blit(&buffer_position[window.width * (row_index - string_row_index) * Text_GetHeight()],
                  strings[row_index].GetCStr(), 265, window.width, GNW_TEXT_OUTLINE | 0xFF);
    }

    win_draw_rect(window.id, &window.window);
}

bool HelpMenu::ProcessKey(int32_t key) {
    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_click_done = false;
    }

    switch (key) {
        case 1001:
        case GNW_KB_KEY_PAGEUP: {
            if (string_row_index) {
                int32_t row_index;
                uint32_t time_stamp;

                button_up->PlaySound();

                row_index = string_row_index - rows_per_page;

                if (row_index < 0) {
                    row_index = 0;
                }

                do {
                    time_stamp = timer_get();

                    --string_row_index;
                    DrawText();

                    while ((timer_get() - time_stamp) < TIMER_FPS_TO_MS(96)) {
                    }

                } while (string_row_index != row_index);
            }

        } break;

        case 1002:
        case GNW_KB_KEY_PAGEDOWN: {
            if ((string_row_index + rows_per_page) < string_row_count) {
                int32_t row_index;
                uint32_t time_stamp;

                button_down->PlaySound();

                row_index = string_row_index + rows_per_page;

                do {
                    time_stamp = timer_get();

                    ++string_row_index;
                    DrawText();

                    while ((timer_get() - time_stamp) < TIMER_FPS_TO_MS(96)) {
                    }

                } while (string_row_index != row_index);
            }

        } break;

        case GNW_INPUT_PRESS: {
            if (!event_click_done) {
                button_done->PlaySound();
            }

            event_click_done = true;

        } break;

        case 1000: {
            keys_mode = !keys_mode;
            button_done->PlaySound();
            string_row_index = 0;

            if (keys_mode) {
                ProcessText(help_menu_keyboard_reference);
            } else {
                ProcessText(help_cache[help_cache_index]);
            }

        } break;

        case GNW_KB_KEY_SHIFT_DIVIDE: {
            event_click_help = true;
            event_click_cancel = true;
        } break;

        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();
        } break;

        case GNW_KB_KEY_RETURN:
        case GNW_KB_KEY_CTRL_ESCAPE: {
            event_click_cancel = true;
        } break;
    }

    return true;
}

bool HelpMenu::Run(int32_t mode) {
    int32_t key;

    event_click_done = false;

    while (!event_click_cancel) {
        key = get_input();

        if (GameManager_RequestMenuExit) {
            key = GNW_KB_KEY_RETURN;
        }

        ProcessKey(key);

        if (!mode) {
            if (window_id == WINDOW_MAIN_MAP) {
                GameManager_ProcessState(true, false);
            } else if (GameManager_GameState == GAME_STATE_8_IN_GAME ||
                       GameManager_GameState == GAME_STATE_9_END_TURN) {
                GameManager_ProcessState(false, false);
            } else if (Remote_GameState) {
                Remote_NetSync();

                if (Remote_GameState == 2) {
                    event_click_cancel = true;
                }

            } else if (Remote_IsNetworkGame) {
                if (Remote_ProcessFrame()) {
                    UnitsManager_ProcessRemoteOrders();
                }
            }
        }
    }

    return event_click_help;
}

void HelpMenu_Menu(HelpSectionId section_id, int32_t window_index, bool mode) {
    MouseEvent mouse_event;
    WinID window_id;

    Cursor_SetCursor(CURSOR_HELP);
    window_id = win_add(0, 0, 1, 1, COLOR_BLACK, 0x10);
    MouseEvent::Clear();

    while (get_input() != GNW_KB_KEY_CTRL_ESCAPE) {
        if (MouseEvent::PopFront(mouse_event)) {
            if (mouse_event.buttons & MOUSE_RELEASE_RIGHT) {
                break;
            }

            if (mouse_event.buttons & MOUSE_RELEASE_LEFT) {
                SoundManager.PlaySfx(KCARG0);

                if (section_id == HELPMENU_GAME_SCREEN_SETUP &&
                    HelpMenu_UnitReport(mouse_event.point.x, mouse_event.point.y)) {
                    break;
                }

                {
                    HelpMenu help_menu(section_id, mouse_event.point.x, mouse_event.point.y, window_index);

                    if (!help_menu.Run(mode)) {
                        break;
                    }
                }

                Cursor_SetCursor(CURSOR_HELP);
            }

            if (!mode) {
                if (window_index == WINDOW_MAIN_MAP) {
                    GameManager_ProcessState(true, false);
                } else if (GameManager_GameState == GAME_STATE_8_IN_GAME ||
                           GameManager_GameState == GAME_STATE_9_END_TURN) {
                    GameManager_ProcessState(false, false);
                } else if (Remote_GameState) {
                    Remote_NetSync();
                    if (Remote_GameState == 2) {
                        break;
                    }

                } else if (Remote_IsNetworkGame) {
                    if (Remote_ProcessFrame()) {
                        UnitsManager_ProcessRemoteOrders();
                    }
                }
            }
        }
    }

    win_delete(window_id);
    Cursor_SetCursor(CURSOR_HAND);
    MouseEvent::Clear();
}

bool HelpMenu_UnitReport(int32_t mouse_x, int32_t mouse_y) {
    WindowInfo* window;
    UnitInfo* unit;
    bool result;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (mouse_x < window->window.ulx || mouse_x > window->window.lrx || mouse_y < window->window.uly ||
        mouse_y > window->window.lry) {
        result = false;

    } else {
        mouse_x =
            (GameManager_MapWindowDrawBounds.ulx + ((Gfx_MapScalingFactor * (mouse_x - window->window.ulx)) >> 16)) >>
            6;
        mouse_y =
            (GameManager_MapWindowDrawBounds.uly + ((Gfx_MapScalingFactor * (mouse_y - window->window.uly)) >> 16)) >>
            6;

        unit = Access_GetTeamUnit(mouse_x, mouse_y, GameManager_PlayerTeam, SELECTABLE);

        if (!unit) {
            unit = Access_GetEnemyUnit(GameManager_PlayerTeam, mouse_x, mouse_y, SELECTABLE);
        }

        if (!unit) {
            result = false;

        } else {
            UnitStats_Menu(unit);
            result = true;
        }
    }

    return result;
}
