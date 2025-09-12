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

#include "missionmanager.hpp"
#include "resource_manager.hpp"

struct MenuTitleItem {
    Rect bounds;
    std::string title;
};

#define MENU_TITLE_ITEM_DEF(ulx, uly, lrx, lry, text) {{(ulx), (uly), (lrx), (lry)}, (text)}

bool menu_check_end_game_conditions(int32_t turn_counter, int32_t turn_counter_session_start, bool is_demo_mode);

void menu_draw_menu_title(WindowInfo* window, MenuTitleItem* menu_item, int32_t color, bool horizontal_align = false,
                          bool vertical_align = true);

void menu_draw_logo(ResourceID resource_id, uint32_t time_limit);

int32_t Menu_LoadPlanetMinimap(int32_t planet_index, uint8_t* buffer, int32_t width);

int32_t menu_clan_select_menu_loop(int32_t team);

void menu_update_resource_levels();

void menu_draw_menu_portrait_frame(WindowInfo* window);

void draw_menu_title(WindowInfo* window, const char* caption);

int32_t menu_options_menu_loop(int32_t game_mode);
void menu_preferences_window(uint16_t team);
int32_t GameSetupMenu_Menu(const MissionCategory mission_category, bool flag1 = true,
                           const bool is_single_player = true);
bool OKCancelMenu_Menu(const char* caption);
void DialogMenu_Menu(const char* label);
bool DesyncMenu_Menu();
void PauseMenu_Menu();
int32_t menu_planet_select_menu_loop();
void menu_draw_exit_logos();
void main_menu();

#endif /* MENU_HPP */
