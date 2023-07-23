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

#include "enums.hpp"
#include "gnw.h"

struct MenuTitleItem {
    Rect bounds;
    const char* title;
};

#define MENU_TITLE_ITEM_DEF(ulx, uly, lrx, lry, text) \
    { {(ulx), (uly), (lrx), (lry)}, (text) }

extern const char* menu_planet_descriptions[];
extern const char* menu_planet_names[];
extern const char* menu_team_names[];

bool menu_check_end_game_conditions(int32_t global_turn, int32_t local_turn, bool is_demo_mode);

void menu_draw_menu_title(WindowInfo* window, MenuTitleItem* menu_item, int32_t color, bool horizontal_align = false,
                          bool vertical_align = true);

void menu_draw_logo(ResourceID resource_id, int32_t time_limit);

int32_t Menu_LoadPlanetMinimap(int32_t planet_index, uint8_t* buffer, int32_t width);

int32_t menu_clan_select_menu_loop(int32_t team);

void menu_update_resource_levels();

void menu_draw_menu_portrait_frame(WindowInfo* window);

void draw_menu_title(WindowInfo* window, const char* caption);

int32_t menu_options_menu_loop(int32_t game_mode);
void menu_preferences_window(uint16_t team);
int32_t GameSetupMenu_Menu(int32_t game_file_type, bool flag1 = true, bool flag2 = true);
bool OKCancelMenu_Menu(const char* caption);
void DialogMenu_Menu(const char* label);
bool DesyncMenu_Menu();
void PauseMenu_Menu();
int32_t menu_planet_select_menu_loop();
void menu_draw_exit_logos();
void main_menu();

#endif /* MENU_HPP */
