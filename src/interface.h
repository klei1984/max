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
#include <stdint.h>

#include "rect.h"

typedef int32_t WinID;
typedef uint32_t TOCKS;

typedef void (*Trans_b2b)(uint8_t* src, int32_t width, int32_t length, int32_t full, uint8_t* dst, int32_t full2);

typedef struct GNW_Menu_s GNW_Menu;

typedef struct GNW_Window_s GNW_Window;

struct WindowInfo {
    Rect window;
    uint16_t width;
    WinID id;
    uint8_t* buffer;
};

typedef struct WindowInfo WindowInfo;

typedef void (*SelectFunc)(char**, int32_t);

int32_t win_list_select(char* title, char** list, int32_t num, SelectFunc select_func, int32_t ulx, int32_t uly,
                        int32_t color);
int32_t win_list_select_at(char* title, char** list, int32_t num, SelectFunc select_func, int32_t ulx, int32_t uly,
                           int32_t color, int32_t start);
int32_t win_get_str(char* str, int32_t limit, char* title, int32_t x, int32_t y);
int32_t win_output(char* title, char** list, int32_t num, int32_t ulx, int32_t uly, int32_t color, char* extra_button);
int32_t win_yes_no(char* question, int32_t ulx, int32_t uly, int32_t color);
int32_t win_msg(char* msg, int32_t ulx, int32_t uly, int32_t color);
int32_t win_pull_down(char** list, int32_t num, int32_t ulx, int32_t uly, int32_t color);
int32_t win_debug(const char* str);
int32_t win_register_menu_bar(WinID wid, int32_t ulx, int32_t uly, int32_t width, int32_t length, int32_t fore_color,
                              int32_t back_color);
int32_t win_register_menu_pulldown(WinID wid, int32_t offx, char* name, int32_t value, int32_t num, char** list,
                                   int32_t fore_color, int32_t back_color);
void win_delete_menu_bar(WinID wid);
int32_t GNW_process_menu(GNW_Menu* m, int32_t num_pd);
int32_t win_width_needed(char** list, int32_t num);
int32_t win_input_str(WinID id, char* str, int32_t limit, int32_t x, int32_t y, int32_t text_color, int32_t back_color);
int32_t win_get_num_i(int32_t* value, int32_t min, int32_t max, int32_t clear, char* title, int32_t x, int32_t y);
void GNW_intr_init(void);
void win_timed_msg_defaults(TOCKS persistence);
void GNW_intr_exit(void);
int32_t win_timed_msg(char* msg, int32_t color);

#endif /* INTERFACE_H */
