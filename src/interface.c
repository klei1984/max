/* Copyright (c) 2020 M.A.X. Port Team
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

#include "interface.h"

#include "gnw.h"

static WinID create_pull_down(char** list, int num, int ulx, int uly, int fcolor, int bcolor, Rect* r);
static int process_pull_down(WinID id, Rect* r, char** list, int num, int fcolor, int bcolor, GNW_Menu* m, int num_pd);
static void win_debug_delete(ButtonID bid, int button_value);
static int find_first_letter(int c, char** list, int num);
static int calc_max_field_chars_wcursor(int min, int max);
static int get_num_i(WinID id, int* value, int max_chars_wcursor, char clear, char allow_negative, int x, int y);
static void tm_watch_msgs(void);
static void tm_kill_msg(void);
static void tm_kill_out_of_order(int queue_index);
static void tm_click_response(ButtonID b_id, int b_value);
static int tm_index_active(int queue_index);
