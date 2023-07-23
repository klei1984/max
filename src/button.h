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

typedef int32_t ButtonID;

typedef void (*ButtonFunc)(ButtonID, int32_t);
typedef void (*CheckButtonFunc)(ButtonID);

typedef struct GNW_buttondata* GNW_ButtonPtr;

typedef struct GNW_ButtonGroup_s {
    int32_t max_checked;
    int32_t curr_checked;
    CheckButtonFunc func;
    int32_t num_buttons;
    GNW_ButtonPtr list[64];
} GNW_ButtonGroup;

struct GNW_buttondata {
    ButtonID id;
    uint32_t flags;
    Rect b;
    int32_t on_value;
    int32_t off_value;
    int32_t p_value;
    int32_t r_value;
    int32_t rp_value;
    int32_t rr_value;
    uint8_t* up;
    uint8_t* down;
    uint8_t* hover;
    uint8_t* dis_up;
    uint8_t* dis_down;
    uint8_t* dis_hover;
    uint8_t* last_image;
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

ButtonID win_register_button(WinID id, int32_t ulx, int32_t uly, int32_t width, int32_t length, int32_t on_value, int32_t off_value,
                             int32_t p_value, int32_t r_value, uint8_t* up, uint8_t* down, uint8_t* hover,
                             uint32_t flags);
ButtonID win_register_text_button(WinID id, int32_t ulx, int32_t uly, int32_t on_value, int32_t off_value, int32_t p_value, int32_t r_value,
                                  char* name, int32_t flags);
int32_t win_register_button_disable(ButtonID bid, uint8_t* disabled_up, uint8_t* disabled_down,
                                uint8_t* disabled_hover);
int32_t win_register_button_image(ButtonID bid, uint8_t* up, uint8_t* down, uint8_t* hover, int32_t draw);
int32_t win_register_button_func(ButtonID bid, ButtonFunc on_func, ButtonFunc off_func, ButtonFunc p_func,
                             ButtonFunc r_func);
int32_t win_register_right_button(ButtonID bid, int32_t p_value, int32_t r_value, ButtonFunc p_func, ButtonFunc r_func);
int32_t win_register_button_mask(ButtonID bid, char* mask);
int32_t win_button_down(ButtonID bid);
int32_t GNW_check_buttons(GNW_Window* w, int32_t* press);
WinID win_button_winID(ButtonID bid);
WinID win_last_button_winID(void);
int32_t win_delete_button(ButtonID bid);
void GNW_delete_button(GNW_ButtonPtr b);
void win_delete_button_win(ButtonID bid, int32_t button_value);
ButtonID button_new_id(void);
int32_t win_enable_button(ButtonID bid);
int32_t win_disable_button(ButtonID bid);
int32_t win_set_button_rest_state(ButtonID bid, int32_t rest_down, int32_t flags);
int32_t win_group_check_buttons(int32_t num_buttons, ButtonID* button_list, int32_t max_checked, CheckButtonFunc func);
int32_t win_group_radio_buttons(int32_t num_buttons, ButtonID* button_list);
void GNW_button_refresh(GNW_Window* w, Rect* r);
int32_t win_button_press_and_release(ButtonID bid);

#endif /* BUTTON_H */
