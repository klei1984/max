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

#include "rect.h"
#include "svga.h"
#include "timer.h"

extern ScreenBlitFunc mouse_blit;

int GNW_mouse_init(void);
void GNW_mouse_exit(void);
int mouse_get_shape(unsigned char** buf, int* width, int* length, int* full, int* hotx, int* hoty, char* trans);
int mouse_set_shape(unsigned char* buf, int width, int length, int full, int hotx, int hoty, char trans);
int mouse_get_anim(unsigned char** frames, int* num_frames, int* width, int* length, int* hotx, int* hoty, char* trans,
                   TOCKS* speed);
int mouse_set_anim_frames(unsigned char* frames, int num_frames, int start_frame, int width, int length, int hotx,
                          int hoty, char trans, TOCKS speed);
void mouse_show(void);
void mouse_hide(void);
void mouse_info(void);
void mouse_simulate_input(int delta_x, int delta_y, int buttons);
int mouse_in(int ulx, int uly, int lrx, int lry);
int mouse_click_in(int ulx, int uly, int lrx, int lry);
void mouse_get_rect(Rect* m);
void mouse_get_position(int* x, int* y);
void mouse_set_position(int x, int y);
int mouse_get_buttons(void);
int mouse_hidden(void);
void mouse_get_hotspot(int* hotx, int* hoty);
void mouse_set_hotspot(int hotx, int hoty);
int mouse_query_exist(void);
void mouse_get_raw_state(int* x, int* y, int* buttons);
void mouse_disable(void);
void mouse_enable(void);
int mouse_is_disabled(void);
void mouse_set_sensitivity(double new_sensitivity);
double mouse_get_sensitivity(void);

#endif /* define MOUSE_H */
