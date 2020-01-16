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

#include "mouse.h"

#include "svga.h"
#include "vcr.h"

int GNW_mouse_init(void);
extern int have_mouse;
extern int mouse_disabled;
extern int _mouse_is_hidden;
extern ScreenBlitFunc mouse_blit;
extern int mouse_x;
extern int mouse_y;
extern int last_buttons;

extern int vcr_state;
extern int vcr_terminate_flags;
extern int vcr_terminated_condition;
void vcr_stop(void);
void mouse_simulate_input(int delta_x, int delta_y, int buttons);

static inline int wrap_mouse_set_shape(unsigned char *buf, int width, int length, int full, int hotx, int hoty,
                                       char trans) {
    int result;

    __asm__ __volatile__(
        "	pushl	%7\n"
        "	pushl	%6\n"
        "	pushl	%5\n"
        "	call	mouse_set_shape\n"
        : /* out  */ "=a"(result) /* return value is EDX:EAX (%0) */
        : /* in   */ "a"(buf), "d"(width), "b"(length), "c"(full), "g"(hotx), "g"(hoty), "g"(trans)
        : /* clob */);

    return result;
}

int GNW_mouse_init(void) {
    SDL_ShowCursor(SDL_DISABLE);
    if (SDL_SetRelativeMouseMode(SDL_FALSE) != 0) {
        SDL_Log("SDL_SetRelativeMouseMode failed: %s\n", SDL_GetError());
    }

    {
        int result;

        have_mouse = 0;
        mouse_disabled = 0;
        _mouse_is_hidden = 1;
        mouse_blit = scr_blit;

        asm volatile("call mouse_colorize\n");

        result = wrap_mouse_set_shape(NULL, 0, 0, 0, 0, 0, 0);

        if (result != -1) {
            mouse_x = scr_size.ulx / 2;
            mouse_y = scr_size.uly / 2;

            have_mouse = 1;
            result = 0;
        }

        return result;
    }
}

void mouse_info(void) {
    unsigned int button_status = 0;
    signed int horizontal_movement;
    signed int vertical_movement;

    if (have_mouse && !_mouse_is_hidden && !mouse_disabled) {
        unsigned int button_bitmask;

        SDL_PumpEvents();
        button_bitmask = SDL_GetRelativeMouseState(&horizontal_movement, &vertical_movement);

        if (SDL_BUTTON(SDL_BUTTON_LEFT) & button_bitmask) {
            button_status |= 1;
        }

        if (SDL_BUTTON(SDL_BUTTON_RIGHT) & button_bitmask) {
            button_status |= 2;
        }

        if (SDL_BUTTON(SDL_BUTTON_MIDDLE) & button_bitmask) {
            button_status |= 4;
        }

        if (vcr_state == 1) {
            if (((vcr_terminate_flags & 4) && button_status) ||
                ((vcr_terminate_flags & 2) && (horizontal_movement || vertical_movement))) {
                vcr_terminated_condition = 2;
                asm volatile("call vcr_stop\n" ::);
                return;
            }
            asm volatile("call mouse_simulate_input\n" ::"a"(0), "d"(0), "b"(last_buttons));
        } else {
            asm volatile("call mouse_simulate_input\n" ::"a"(horizontal_movement), "d"(vertical_movement),
                         "b"(button_status));
        }
    }
}
