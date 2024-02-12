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

#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

#include "input.h"
#include "rect.h"
#include "svga.h"

enum {
    MOUSE_PRESS_LEFT = 0x1,
    MOUSE_PRESS_RIGHT = 0x2,
    MOUSE_LONG_PRESS_LEFT = 0x4,
    MOUSE_LONG_PRESS_RIGHT = 0x8,
    MOUSE_RELEASE_LEFT = 0x10,
    MOUSE_RELEASE_RIGHT = 0x20,
};

enum {
    MOUSE_LOCK_UNLOCKED,
    MOUSE_LOCK_FOCUSED,
    MOUSE_LOCK_LOCKED,
};

extern ScreenBlitFunc mouse_blit;

int32_t GNW_mouse_init(void);
void GNW_mouse_exit(void);
int32_t mouse_get_shape(uint8_t** buf, int32_t* width, int32_t* length, int32_t* full, int32_t* hotx, int32_t* hoty,
                        char* trans);
int32_t mouse_set_shape(uint8_t* buf, int32_t width, int32_t length, int32_t full, int32_t hotx, int32_t hoty,
                        char trans);
int32_t mouse_get_anim(uint8_t** frames, int32_t* num_frames, int32_t* width, int32_t* length, int32_t* hotx,
                       int32_t* hoty, char* trans, TOCKS* speed);
int32_t mouse_set_anim_frames(uint8_t* frames, int32_t num_frames, int32_t start_frame, int32_t width, int32_t length,
                              int32_t hotx, int32_t hoty, char trans, TOCKS speed);
void mouse_show(void);
void mouse_hide(void);
void mouse_info(void);
void mouse_simulate_input(int32_t delta_x, int32_t delta_y, uint32_t buttons);
int32_t mouse_in(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
int32_t mouse_click_in(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
void mouse_get_rect(Rect* m);
void mouse_get_position(int32_t* x, int32_t* y);
void mouse_set_position(int32_t x, int32_t y);
uint32_t mouse_get_buttons(void);
int32_t mouse_hidden(void);
void mouse_get_hotspot(int32_t* hotx, int32_t* hoty);
void mouse_set_hotspot(int32_t hotx, int32_t hoty);
int32_t mouse_query_exist(void);
void mouse_get_raw_state(int32_t* x, int32_t* y, int32_t* buttons);
void mouse_disable(void);
void mouse_enable(void);
int32_t mouse_is_disabled(void);
void mouse_set_sensitivity(double new_sensitivity);
double mouse_get_sensitivity(void);
int32_t mouse_get_lock(void);
void mouse_set_lock(int32_t state);

#endif /* define MOUSE_H */
