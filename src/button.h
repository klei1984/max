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

typedef int ButtonID;

typedef void (*ButtonFunc)(ButtonID, int);
typedef void (*CheckButtonFunc)(ButtonID);

typedef struct GNW_buttondata* GNW_ButtonPtr;

typedef struct GNW_ButtonGroup_s {
    int max_checked;
    int curr_checked;
    CheckButtonFunc func;
    int num_buttons;
    GNW_ButtonPtr list[64];
} GNW_ButtonGroup;

static_assert(sizeof(struct GNW_ButtonGroup_s) == 272, "The structure needs to be packed.");

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
    ButtonFunc on_func;
    ButtonFunc off_func;
    ButtonFunc p_func;
    ButtonFunc r_func;
    ButtonFunc rp_func;
    ButtonFunc rr_func;
    GNW_ButtonGroup* group;
    GNW_ButtonPtr prev;
    GNW_ButtonPtr next;
};

static_assert(sizeof(struct GNW_buttondata) == 116, "The structure needs to be packed.");

ButtonID win_register_button(WinID id, int ulx, int uly, int width, int length, int on_value, int off_value,
                             int p_value, int r_value, unsigned char* up, unsigned char* down, unsigned char* hover,
                             int flags);
ButtonID win_register_text_button(WinID id, int ulx, int uly, int on_value, int off_value, int p_value, int r_value,
                                  char* name, int flags);
int win_register_button_disable(ButtonID bid, unsigned char* disabled_up, unsigned char* disabled_down,
                                unsigned char* disabled_hover);
int win_register_button_image(ButtonID bid, unsigned char* up, unsigned char* down, unsigned char* hover, int draw);
int win_register_button_func(ButtonID bid, ButtonFunc on_func, ButtonFunc off_func, ButtonFunc p_func,
                             ButtonFunc r_func);
int win_register_right_button(ButtonID bid, int p_value, int r_value, ButtonFunc p_func, ButtonFunc r_func);
int win_register_button_mask(ButtonID bid, char* mask);
int win_button_down(ButtonID bid);
int GNW_check_buttons(GNW_Window* w, int* press);
WinID win_button_winID(ButtonID bid);
WinID win_last_button_winID(void);
int win_delete_button(ButtonID bid);
void GNW_delete_button(GNW_ButtonPtr b);
void win_delete_button_win(ButtonID bid, int button_value);
ButtonID button_new_id(void);
int win_enable_button(ButtonID bid);
int win_disable_button(ButtonID bid);
int win_set_button_rest_state(ButtonID bid, int rest_down, int flags);
int win_group_check_buttons(int num_buttons, ButtonID* button_list, int max_checked, CheckButtonFunc func);
int win_group_radio_buttons(int num_buttons, ButtonID* button_list);
void GNW_button_refresh(GNW_Window* w, Rect* r);
int win_button_press_and_release(ButtonID bid);

#endif /* BUTTON_H */
