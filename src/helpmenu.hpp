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

#ifndef HELPMENU_HPP
#define HELPMENU_HPP

#include "button.hpp"
#include "smartstring.hpp"
#include "window.hpp"

enum HelpSectionId {
    HELPMENU_NEW_GAME_SETUP = 0x0,
    HELPMENU_CLAN_SETUP = 0x1,
    HELPMENU_PLANET_SETUP = 0x2,
    HELPMENU_MULTI_PLAYER_SETUP = 0x3,
    HELPMENU_STATS_SETUP = 0x4,
    HELPMENU_UPGRADES_SETUP = 0x5,
    HELPMENU_CARGO_SETUP = 0x6,
    HELPMENU_LOAD_SETUP = 0x7,
    HELPMENU_SAVELOAD_SETUP = 0x8,
    HELPMENU_GAME_SCREEN_SETUP = 0x9,
    HELPMENU_ALLOCATE_SETUP = 0xA,
    HELPMENU_DEPOT_SETUP = 0xB,
    HELPMENU_HANGAR_SETUP = 0xC,
    HELPMENU_DOCK_SETUP = 0xD,
    HELPMENU_TRANSPORT_SETUP = 0xE,
    HELPMENU_TRANSFER_SETUP = 0xF,
    HELPMENU_SITE_SELECT_SETUP = 0x10,
    HELPMENU_RESEARCH_SETUP = 0x11,
    HELPMENU_FACTORY_BUILD_SETUP = 0x12,
    HELPMENU_CONSTRUCTOR_BUILD_SETUP = 0x13,
    HELPMENU_SERIAL_MENU_SETUP = 0x14,
    HELPMENU_MODEM_MENU_SETUP = 0x15,
    HELPMENU_CHAT_MENU_SETUP = 0x16,
    HELPMENU_PREFS_MENU_SETUP = 0x17,
    HELPMENU_SETUP_MENU_SETUP = 0x18,
    HELPMENU_HOT_SEAT_SETUP = 0x19,
    HELPMENU_TRAINING_MENU_SETUP = 0x1A,
    HELPMENU_STAND_ALONE_MENU_SETUP = 0x1B,
    HELPMENU_OPTIONS_SETUP = 0x1C,
    HELPMENU_REPORTS_SETUP = 0x1D,
    HELPMENU_CAMPAIGN_MENU = 0x1E,
    HELPMENU_MULTI_SCENARIO_MENU = 0x1F,
    HELPMENU_BARRACKS_SETUP = 0x20,
};

class HelpMenu : public Window {
    WindowInfo window;
    bool field_49;
    bool field_50;
    Button *button_done;
    Button *button_keys;
    Button *button_up;
    Button *button_down;
    unsigned short field_67;
    unsigned short help_cache_use_count;
    char **help_cache[10];
    unsigned char buffer[1000];
    unsigned char section;
    unsigned char window_id;
    FILE *file;
    int file_size;
    bool field_1121;
    Image *canvas;
    unsigned short field_1126;
    unsigned short field_1128;
    unsigned short field_1130;
    SmartString *strings;
    bool field_1136;

public:
    HelpMenu(unsigned char section, int cursor_x, int cursor_y, int window_id);
    ~HelpMenu();

    bool Run();
};

void HelpMenu_Menu(HelpSectionId section_id, int window_index, bool mode = false);

#endif /* HELPMENU_HPP */
