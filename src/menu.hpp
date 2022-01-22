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

#ifndef MENU_HPP
#define MENU_HPP

extern "C" {
#include "gnw.h"
}

struct MenuTitleItem {
    Rect bounds;
    const char* title;
};

#define MENU_TITLE_ITEM_DEF(ulx, uly, lrx, lry, text) \
    { {(ulx), (uly), (lrx), (lry)}, (text) }

const char* menu_planet_descriptions[];
extern const char* menu_planet_names[];

void menu_draw_menu_title(WindowInfo* window, MenuTitleItem* menu_item, int color, bool horizontal_align = false,
                          bool vertical_align = true);

void menu_draw_logo(ResourceID resource_id, int time_limit);

int Menu_LoadPlanetMinimap(int planet_index, char* buffer, int width);

int menu_clan_select_menu_loop(int team);

void menu_update_resource_levels();

void menu_draw_menu_portrait_frame(WindowInfo* window);

void draw_menu_title(WindowInfo* window, const char* caption);

int menu_options_menu_loop(int game_mode);
int GameSetupMenu_menu_loop(int game_file_type, bool flag1 = true, bool flag2 = true);
int menu_planet_select_menu_loop();

#endif /* MENU_HPP */
