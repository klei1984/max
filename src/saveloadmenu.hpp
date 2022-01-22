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

#ifndef SAVELOADMENU_HPP
#define SAVELOADMENU_HPP

struct __attribute__((packed)) SaveFormatHeader {
    unsigned short version;
    char save_game_type;
    char save_name[30];
    char world;
    unsigned short mission_index;
    char team_name[4][30];
    char team_type[5];
    char team_clan[5];
    unsigned int rng_seed;
    char opponent;
    unsigned short turn_timer_time;
    unsigned short endturn_time;
    char play_mode;
};

extern const char* save_file_tpyes[];

int SaveLoadMenu_GetSavedGameInfo(int save_slot, int game_file_type, struct SaveFormatHeader& save_file_header,
                                  bool load_ini_options = true);

int SaveLoadMenu_menu_loop(int is_saving_allowed, int is_text_mode);

#endif /* SAVELOADMENU_HPP */
