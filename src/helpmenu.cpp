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

#include "cursor.hpp"
#include "game_manager.hpp"
#include "menu.hpp"
#include "mouseevent.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
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

const char* help_menu_keyboard_reference =
    "Enter: End Turn\n"
    "\n"
    "F: Find Selected Unit. Currently selected unit will be centered on screen.\n"
    "\n"
    "-,+: Zoom In, Out\n"
    "\n"
    "G: Turns on Grid Display\n"
    "\n"
    "F1: Centers on Tagged Unit\n"
    "\n"
    "Arrow Keys: Scrolls the map\n"
    "\n"
    "ALT-P: Pause the game\n"
    "\n"
    "ALT-F: Opens the Load/Save Game Menu\n"
    "\n"
    "ALT-L: Use to quick load a game\n"
    "\n"
    "ALT-S: Use to quick save a game\n"
    "\n"
    "ALT-X: Exits the Game to the Main Menu\n"
    "\n"
    "ALT-F5, ALT-F6, ALT-F7, ALT-F8: Saves the current window position\n"
    "\n"
    "F5, F6, F7, F8: Jumps to a previously saved window position\n"
    "\n"
    "?: Initiates HELP mode.  The cursor changes to a question mark, clicking on screen items will display help text.\n"
    "\n"
    "ALT-C: Saves a screen shot of the game in PCX format.\n"
    "\n"
    "Shift: Hold the Shift key while selecting units to create groups.\n"
    "\n"
    "Shift-Done: Holding the Shift key while clicking on the Done button will start in motion all units that are "
    "waiting to move along a path.\n"
    "\n"
    "Hot keys for unit commands:\n"
    "\n"
    "Press 1 for these functions:\n"
    "...Activate\n"
    "...Allocate\n"
    "...Auto-Survey\n"
    "...Build\n"
    "...Buy Upgrade\n"
    "...Disable\n"
    "...Place Mines\n"
    "...Reload\n"
    "...Repair\n"
    "...Research\n"
    "\n"
    "Press 2 for these functions:\n"
    "...Load\n"
    "...Start\n"
    "...Steal\n"
    "\n"
    "Press 3 for these functions:\n"
    "...Attack\n"
    "...Transfer\n"
    "\n"
    "Press 4 for the Manual function.\n"
    "\n"
    "Press 5 for these functions:\n"
    "...Enter\n"
    "...Upgrade\n"
    "\n"
    "Press 6 for the Upgrade All function.\n"
    "\n"
    "Press 7 for the Stop function.\n"
    "\n"
    "Press 8 for the Sentry function.\n"
    "\n"
    "Press 9 for the Done function.\n"
    "\n"
    "Press 0 for the Remove function.\n";

HelpMenu::HelpMenu(unsigned char section, int cursor_x, int cursor_y, int window_id)
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
        text_font(5);
        Add();
        FillWindowInfo(&window);

        canvas = new (std::nothrow) Image(0, 0, window.window.lrx, window.window.lry);
        canvas->Copy(&window);

        if (section == HELPMENU_GAME_SCREEN_SETUP) {
            button_done = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 85, 193);
            button_keys = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 155, 193);

            button_keys->SetCaption("Keys", 2, 2);
            button_keys->SetRValue(1000);
            button_keys->SetPValue(1000);
            button_keys->SetFlags(1);
            button_keys->RegisterButton(window.id);
        } else {
            button_done = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 120, 193);
        }

        button_done->SetCaption("Done", 2, 2);
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

int HelpMenu::ReadNextChunk() {
    int result;
    int size;

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

int HelpMenu::SeekSection() {
    do {
        if (!ReadNextChunk()) {
            return 0;
        }

    } while (!strstr(buffer, help_menu_sections[section]));

    return 1;
}

int HelpMenu::SeekEntry(int cursor_x, int cursor_y) {
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

int HelpMenu::ReadEntry() {
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

    text_font(5);

    rows_per_page = 160 / text_height();

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
    unsigned char* buffer_position;
    int text_full_height;
    int row_index_max;

    canvas->Write(&window);

    buffer_position = &window.buffer[window.width * 20 + 20];
    text_full_height = string_row_count * text_height();

    if (text_full_height < 160) {
        buffer_position = &buffer_position[window.width * ((160 - text_full_height) / 2)];
    }

    row_index_max = string_row_index + rows_per_page;

    if (row_index_max > string_row_count) {
        row_index_max = string_row_count;
    }

    text_font(5);

    for (int row_index = string_row_index; row_index < row_index_max; ++row_index) {
        text_to_buf(&buffer_position[window.width * (row_index - string_row_index) * text_height()],
                    strings[row_index].GetCStr(), 265, window.width, 0x100FF);
    }

    win_draw_rect(window.id, &window.window);
}

bool HelpMenu::ProcessKey(int key) {
    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_click_done = false;
    }

    switch (key) {
        case 1001:
        case GNW_KB_KEY_PAGEUP: {
            if (string_row_index) {
                int row_index;
                unsigned int time_stamp;

                row_index = string_row_index - rows_per_page;

                if (row_index < 0) {
                    row_index = 0;
                }

                do {
                    time_stamp = timer_get_stamp32();

                    --string_row_index;
                    DrawText();

                    while ((timer_get_stamp32() - time_stamp) < TIMER_FPS_TO_TICKS(96)) {
                    }

                } while (string_row_index != row_index);
            }

        } break;

        case 1002:
        case GNW_KB_KEY_PAGEDOWN: {
            if ((string_row_index + rows_per_page) < string_row_count) {
                int row_index;
                unsigned int time_stamp;

                row_index = string_row_index + rows_per_page;

                do {
                    time_stamp = timer_get_stamp32();

                    ++string_row_index;
                    DrawText();

                    while ((timer_get_stamp32() - time_stamp) < TIMER_FPS_TO_TICKS(96)) {
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

bool HelpMenu::Run(int mode) {
    int key;

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
            } else if (GameManager_GameState == GAME_STATE_8_IN_GAME || GameManager_GameState == GAME_STATE_9) {
                GameManager_ProcessState(false, false);
            } else if (Remote_GameState) {
                Remote_NetSync();

                if (Remote_GameState == 2) {
                    event_click_cancel = true;
                }

            } else if (Remote_IsNetworkGame) {
                if (Remote_sub_C8835()) {
                    /// \todo sub_102CB8();
                }
            }
        }
    }

    return event_click_help;
}

void HelpMenu_Menu(HelpSectionId section_id, int window_index, bool mode) {
    MouseEvent mouse_event;
    WinID window_id;

    Cursor_SetCursor(CURSOR_HELP);
    window_id = win_add(0, 0, 1, 1, 0x0, 0x10);
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
                } else if (GameManager_GameState == GAME_STATE_8_IN_GAME || GameManager_GameState == GAME_STATE_9) {
                    GameManager_ProcessState(false, false);
                } else if (Remote_GameState) {
                    Remote_NetSync();
                    if (Remote_GameState == 2) {
                        break;
                    }

                } else if (Remote_IsNetworkGame) {
                    if (Remote_sub_C8835()) {
                        /// \todo sub_102CB8();
                    }
                }
            }
        }
    }

    win_delete(window_id);
    Cursor_SetCursor(CURSOR_HAND);
    MouseEvent::Clear();
}

bool HelpMenu_UnitReport(int mouse_x, int mouse_y) {
    WindowInfo* window;
    UnitInfo* unit;
    bool result;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    if (mouse_x < window->window.ulx || mouse_x > window->window.lrx || mouse_y < window->window.uly ||
        mouse_y > window->window.lry) {
        result = false;

    } else {
        /** \todo Implement missing stuff
          mouse_x = (draw_bounds.ulx + ((dword_1738F4 * (mouse_x - window->window.ulx)) >> 16)) >> 6;
          mouse_y = (draw_bounds.uly + ((dword_1738F4 * (mouse_y - window->window.uly)) >> 16)) >> 6;

          unit = get_unit_based_on_grid_pos(mouse_x, mouse_y, GUI_PlayerTeamIndex, 0x400000);

          if (!unit) {
              unit = sub_1459A(GUI_PlayerTeamIndex, mouse_x, mouse_y, 0x400000);
          }

          if (!unit) {
              result = false;
          } else {
              menu_draw_unit_stats_menu(unit);
              result = true;
          }
        */
    }

    return result;
}
