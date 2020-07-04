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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <assert.h>

#include "rect.h"

typedef int WinID;
typedef unsigned int TOCKS;

typedef void (*Trans_b2b)(unsigned char*, int, int, int, unsigned char*, int);

typedef struct GNW_Menu_s GNW_Menu;

typedef struct GNW_Window_s GNW_Window;

struct __attribute__((packed)) Window_s {
    Rect window;
    unsigned short unknown;
    WinID id;
    unsigned char* buffer;
};

static_assert(sizeof(struct Window_s) == 26, "The structure needs to be packed.");

typedef struct Window_s Window;

typedef void (*SelectFunc)(char**, int);

int win_list_select(char* title, char** list, int num, SelectFunc select_func, int ulx, int uly, int color);
int win_list_select_at(char* title, char** list, int num, SelectFunc select_func, int ulx, int uly, int color,
                       int start);
int win_get_str(char* str, int limit, char* title, int x, int y);
int win_output(char* title, char** list, int num, int ulx, int uly, int color, char* extra_button);
int win_yes_no(char* question, int ulx, int uly, int color);
int win_msg(char* msg, int ulx, int uly, int color);
int win_pull_down(char** list, int num, int ulx, int uly, int color);
int win_debug(char* str);
int win_register_menu_bar(WinID wid, int ulx, int uly, int width, int length, int fore_color, int back_color);
int win_register_menu_pulldown(WinID wid, int offx, char* name, int value, int num, char** list, int fore_color,
                               int back_color);
void win_delete_menu_bar(WinID wid);
int GNW_process_menu(GNW_Menu* m, int num_pd);
int win_width_needed(char** list, int num);
int win_input_str(WinID id, char* str, int limit, int x, int y, int text_color, int back_color);
int win_get_num_i(int* value, int min, int max, int clear, char* title, int x, int y);
void GNW_intr_init(void);
void win_timed_msg_defaults(TOCKS persistence);
void GNW_intr_exit(void);
int win_timed_msg(char* msg, int color);

#endif /* INTERFACE_H */
