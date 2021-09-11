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

#ifndef GBUTTON_H
#define GBUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gimage.h"
#include "gnw.h"
#include "resrcmgr.h"

struct __attribute__((packed)) GButton {
    ButtonID bid;
    unsigned short ulx;
    unsigned short uly;
    unsigned short lrx;
    unsigned short lry;
    GImage *up;
    GImage *down;
    GImage *up_disabled;
    GImage *down_disabled;
    int p_value;
    int r_value;
    unsigned int flags;
    char enable;
    ButtonFunc p_func;
    ButtonFunc r_func;
    WinID wid;
    unsigned short sfx; /* GAME_RESOURCE enum */
    char rest_state;
};

typedef struct GButton GButton;

/* static */ void gbutton_alloc(GButton *b);
void gbutton_init(GButton *b);
GButton *gbutton_init_rect(GButton *b, int ulx, int uly, int lrx, int lry);
GButton *gbutton_init_texture(GButton *b, GAME_RESOURCE up, GAME_RESOURCE down, int ulx, int uly);
GButton *gbutton_delete(GButton *b);
void gbutton_copy_from_window(GButton *b, WinID wid);
void gbutton_copy_from_resource(GButton *b, GAME_RESOURCE id, int ulx, int uly);
void gbutton_copy_disabled_from_window(GButton *b, WindowInfo *w);
void gbutton_copy_up_from_resource(GButton *b, GAME_RESOURCE id);
void gbutton_copy_down_from_resource(GButton *b, GAME_RESOURCE id);
void gbutton_copy_up_disabled_from_resource(GButton *b, GAME_RESOURCE id);
void gbutton_copy_down_disabled_from_resource(GButton *b, GAME_RESOURCE id);
void gbutton_copy_up_from_buffer(GButton *b, unsigned char *buffer);
void gbutton_copy_down_from_buffer(GButton *b, unsigned char *buffer);
void gbutton_copy_up_disabled_from_buffer(GButton *b, unsigned char *buffer);
void gbutton_copy_down_disabled_from_buffer(GButton *b, unsigned char *buffer);
void gbutton_set_p_func(GButton *b, ButtonFunc p_func, int p_value);
void gbutton_set_r_func(GButton *b, ButtonFunc r_func, int r_value);
void gbutton_register_button(GButton *b, WinID wid);
void gbutton_enable(GButton *b, char enable, char redraw);
void gbutton_disable(GButton *b, char redraw);
void gbutton_set_rest_state(GButton *b, char rest_state);
void gbutton_play_sound(GButton *b);
void gbutton_set_sfx(GButton *b, GAME_RESOURCE id);
ButtonID gbutton_get_button_id(GButton *b);
void gbutton_set_r_value(GButton *b, int r_value);
void gbutton_set_p_value(GButton *b, int p_value);
void gbutton_set_flags(GButton *b, int flags);

#ifdef __cplusplus
}
#endif

#endif /* GBUTTON_H */
