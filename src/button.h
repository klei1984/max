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

#ifndef BUTTON_H
#define BUTTON_H

#include "interface.h"
#include "rect.h"
#include "vcr.h"

typedef int ButtonID;

typedef void (*ButtonFunc)(ButtonID, int);

typedef struct GNW_ButtonGroup_s* GNW_ButtonGroup;
typedef struct GNW_buttondata* GNW_ButtonPtr;

struct GNW_buttondata {
    ButtonID id;
    unsigned int flags;
    Rect b;
    int on_value;
    int off_value;
    int p_value;
    int r_value;
    int rp_value;
    int rr_value;
    char* up;
    char* down;
    char* hover;
    char* dis_up;
    char* dis_down;
    char* dis_hover;
    char* last_image;
    char* mask;
    void* on_func;
    void* off_func;
    void* p_func;
    void* r_func;
    void* rp_func;
    void* rr_func;
    void* p_sound_func;
    GNW_ButtonPtr next;
    GNW_ButtonGroup group;
};

ButtonID win_register_button(WinID id, int ulx, int uly, int width, int length, int on_value, int off_value,
                             int p_value, int r_value, char* up, char* down, char* hover, int flags);

// win_register_text_button
// win_register_button_disable
// win_register_button_func

int win_register_right_button(ButtonID bid, int p_value, int r_value, ButtonFunc p_func, ButtonFunc r_func);

// win_register_button_sound_func
// win_register_button_mask

GNW_ButtonPtr button_create(WinID id, int ulx, int uly, int width, int length, int on_value, int off_value, int p_value,
                            int r_value, int flags, char* up, char* down, char* hover);

int win_button_down(ButtonID bid);

int GNW_check_buttons(GNW_Window* w, int* press);

// button_under_mouse
// win_button_winID
// win_last_button_winID
// win_delete_button

void GNW_delete_button(GNW_ButtonPtr b);

// win_delete_button_win
// button_new_id
// win_enable_button
// win_disable_button
// win_set_button_rest_state
// win_group_check_buttons
// win_group_radio_buttons

int button_check_group(GNW_ButtonPtr b);

void button_draw(GNW_ButtonPtr b, GNW_Window* w, char*, int, int);

// GNW_button_refresh
// win_button_press_and_release

#endif /* BUTTON_H */
