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

#ifndef GNW_H
#define GNW_H

#include <assert.h>
#include <limits.h>

#ifdef __unix__
#include <linux/limits.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "button.h"
#include "color.h"
#include "dos.h"
#include "grbuf.h"
#include "input.h"
#include "interface.h"
#include "kb.h"
#include "mouse.h"
#include "rect.h"
#include "text.h"

typedef int32_t (*SetModeFunc)(void);
typedef void (*ResetModeFunc)(void);

typedef struct GNW_PD_s {
    Rect r;
    int32_t value;
    int32_t num_list;
    char** list;
    int32_t fcolor;
    int32_t bcolor;
} GNW_PD;

typedef struct GNW_Menu_s {
    WinID wid;
    Rect m;
    int32_t num_pds;
    GNW_PD pd[15];
    int32_t fcolor;
    int32_t bcolor;
} GNW_Menu;

typedef struct GNW_Window_s {
    WinID id;
    uint32_t flags;
    Rect w;
    int32_t width;
    int32_t length;
    int32_t color;
    int32_t tx;
    int32_t ty;
    uint8_t* buf;
    GNW_ButtonPtr button_list;
    GNW_ButtonPtr last_over;
    GNW_ButtonPtr last_click;
    GNW_Menu* menu;
    Trans_b2b trans_b2b;
} GNW_Window;

extern uint8_t* GNW_texture;
extern int32_t GNW_win_init_flag;
extern ColorRGB GNW_wcolor[6];

int32_t win_init(SetModeFunc set_mode_func, ResetModeFunc reset_mode_func, int32_t flags);
int32_t win_reinit(SetModeFunc set_mode_func);
int32_t win_active(void);
void win_exit(void);
WinID win_add(int32_t ulx, int32_t uly, int32_t width, int32_t length, int32_t color, int32_t flags);
void win_delete(WinID id);
void win_buffering(int32_t state);
void win_border(WinID id);
void win_set_bk_color(int32_t color);
void win_print(WinID id, const char* str, int32_t field_width, int32_t x, int32_t y, int32_t color);
void win_text(WinID id, char** list, int32_t num, int32_t field_width, int32_t x, int32_t y, int32_t color);
void win_line(WinID id, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);
void win_box(WinID id, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t color);
void win_shaded_box(WinID id, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, int32_t color1, int32_t color2);
void win_fill(WinID id, int32_t ulx, int32_t uly, int32_t width, int32_t length, int32_t color);
void win_show(WinID id);
void win_hide(WinID id);
void win_move(WinID id, int32_t ulx, int32_t uly);
void win_draw(WinID id);
void win_draw_rect(WinID id, Rect* bound);
void GNW_win_refresh(GNW_Window* w, Rect* bound, uint8_t* scr_buf);
void win_refresh_all(Rect* bound);
void win_drag(WinID id);
void win_get_mouse_buf(uint8_t* buf);
GNW_Window* GNW_find(WinID id);
uint8_t* win_get_buf(WinID id);
WinID win_get_top_win(int32_t x, int32_t y);
int32_t win_width(WinID id);
int32_t win_height(WinID id);
int32_t win_get_rect(WinID id, Rect* r);
int32_t win_check_all_buttons(void);
GNW_ButtonPtr GNW_find_button(ButtonID id, GNW_Window** w);
int32_t GNW_check_menu_bars(int32_t input);
void win_set_trans_b2b(WinID id, Trans_b2b trans_b2b);
uint32_t GNWSystemError(char* errStr);

#ifdef __cplusplus
}
#endif

#endif /* GNW_H */
