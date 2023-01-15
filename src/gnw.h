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

#include "assoc.h"
#include "button.h"
#include "color.h"
#include "db.h"
#include "dos.h"
#include "grbuf.h"
#include "input.h"
#include "interface.h"
#include "kb.h"
#include "lzss.h"
#include "mouse.h"
#include "rect.h"
#include "text.h"
#include "vcr.h"

typedef int (*SetModeFunc)(void);
typedef void (*ResetModeFunc)(void);

typedef struct GNW_PD_s {
    Rect r;
    int value;
    int num_list;
    char** list;
    int fcolor;
    int bcolor;
} GNW_PD;

typedef struct GNW_Menu_s {
    WinID wid;
    Rect m;
    int num_pds;
    GNW_PD pd[15];
    int fcolor;
    int bcolor;
} GNW_Menu;

static_assert(sizeof(struct GNW_Menu_s) == 572, "The structure needs to be packed.");

typedef struct GNW_Window_s {
    WinID id;
    unsigned int flags;
    Rect w;
    int width;
    int length;
    int color;
    int tx;
    int ty;
    unsigned char* buf;
    GNW_ButtonPtr button_list;
    GNW_ButtonPtr last_over;
    GNW_ButtonPtr last_click;
    GNW_Menu* menu;
    Trans_b2b trans_b2b;
} GNW_Window;

extern unsigned char* GNW_texture;
extern int GNW_win_init_flag;
extern ColorRGB GNW_wcolor[6];

int win_init(SetModeFunc set_mode_func, ResetModeFunc reset_mode_func, int flags);
int win_reinit(SetModeFunc set_mode_func);
int win_active(void);
void win_exit(void);
WinID win_add(int ulx, int uly, int width, int length, int color, int flags);
void win_delete(WinID id);
void win_buffering(int state);
void win_border(WinID id);
void win_no_texture(void);
void win_texture(char* fname);
void win_set_bk_color(int color);
void win_print(WinID id, char* str, int field_width, int x, int y, int color);
void win_text(WinID id, char** list, int num, int field_width, int x, int y, int color);
void win_line(WinID id, int x1, int y1, int x2, int y2, int color);
void win_box(WinID id, int ulx, int uly, int lrx, int lry, int color);
void win_shaded_box(WinID id, int ulx, int uly, int lrx, int lry, int color1, int color2);
void win_fill(WinID id, int ulx, int uly, int width, int length, int color);
void win_show(WinID id);
void win_hide(WinID id);
void win_move(WinID id, int ulx, int uly);
void win_draw(WinID id);
void win_draw_rect(WinID id, Rect* bound);
void GNW_win_refresh(GNW_Window* w, Rect* bound, unsigned char* scr_buf);
void win_refresh_all(Rect* bound);
void win_drag(WinID id);
void win_get_mouse_buf(unsigned char* buf);
GNW_Window* GNW_find(WinID id);
unsigned char* win_get_buf(WinID id);
WinID win_get_top_win(int x, int y);
int win_width(WinID id);
int win_height(WinID id);
int win_get_rect(WinID id, Rect* r);
int win_check_all_buttons(void);
GNW_ButtonPtr GNW_find_button(ButtonID id, GNW_Window** w);
int GNW_check_menu_bars(int input);
void win_set_trans_b2b(WinID id, Trans_b2b trans_b2b);
unsigned long GNWSystemError(char* errStr);

#ifdef __cplusplus
}
#endif

#endif /* GNW_H */
