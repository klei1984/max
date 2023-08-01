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

#include <SDL.h>

#include "gnw.h"

#define GNW_MOUSE_BUTTON_LEFT 1
#define GNW_MOUSE_BUTTON_RIGHT 2
#define GNW_MOUSE_BUTTON_MIDDLE 4

static void mouse_anim(void);
static void mouse_colorize(void);
static void mouse_clip(void);

static double mouse_sensitivity = 1.0;
static uint8_t or_mask[64] = {1,  1,  1,  1,  1,  1,  1,  0, 1, 15, 15, 15, 15, 15, 1,  0,  1, 15, 15, 15, 15, 1,
                              1,  0,  1,  15, 15, 15, 15, 1, 1, 0,  1,  15, 15, 15, 15, 15, 1, 1,  1,  15, 1,  1,
                              15, 15, 15, 1,  1,  1,  1,  1, 1, 15, 15, 1,  0,  0,  0,  0,  1, 1,  1,  1};
static uint32_t last_buttons;
static uint8_t *mouse_fptr;
static uint8_t *mouse_shape;
static int32_t mouse_is_hidden;
static int32_t mouse_length;
static int32_t mouse_disabled;
static int32_t mouse_buttons;
static TOCKS mouse_speed;
static int32_t mouse_curr_frame;
static uint8_t *mouse_buf;
static int32_t mouse_full;
static int32_t mouse_num_frames;
static int32_t mouse_hotx;
static int32_t mouse_hoty;
static int32_t mouse_x;
static int32_t mouse_y;
static int32_t have_mouse;
static int32_t mouse_width;
static char mouse_trans;

ScreenBlitFunc mouse_blit;

int32_t GNW_mouse_init(void) {
    int32_t result;

    have_mouse = 0;
    mouse_disabled = 0;
    mouse_is_hidden = 1;

    mouse_blit = scr_blit;

    mouse_colorize();

    result = mouse_set_shape(NULL, 0, 0, 0, 0, 0, 0);

    if (result != -1) {
        SDL_ShowCursor(SDL_DISABLE);
        if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
            SDL_Log("SDL_SetRelativeMouseMode failed: %s\n", SDL_GetError());
            result = -1;
        }

        mouse_x = scr_size.ulx / 2;
        mouse_y = scr_size.uly / 2;

        have_mouse = 1;
        result = 0;
    }

    return result;
}

void GNW_mouse_exit(void) {
    if (mouse_buf) {
        free(mouse_buf);
        mouse_buf = NULL;
    }

    if (mouse_fptr) {
        remove_bk_process(mouse_anim);
        mouse_fptr = NULL;
    }
}

void mouse_colorize(void) {
    for (int32_t i = 0; i < 64; i++) {
        if (or_mask[i] == 15) {
            or_mask[i] = RGB2Color(0x7FFF);
        } else if (or_mask[i] == 1) {
            or_mask[i] = RGB2Color(0x2108);
        } else if (!or_mask[i]) {
            or_mask[i] = RGB2Color(0);
        }
    }
}

int32_t mouse_get_shape(uint8_t **buf, int32_t *width, int32_t *length, int32_t *full, int32_t *hotx, int32_t *hoty,
                        char *trans) {
    *buf = mouse_shape;
    *width = mouse_width;
    *length = mouse_length;
    *full = mouse_full;
    *hotx = mouse_hotx;
    *hoty = mouse_hoty;
    *trans = mouse_trans;

    return 0;
}

int32_t mouse_set_shape(uint8_t *buf, int32_t width, int32_t length, int32_t full, int32_t hotx, int32_t hoty,
                        char trans) {
    int32_t mh;

    if (!buf) {
        return mouse_set_shape(or_mask, 8, 8, 8, 1, 1, RGB2Color(0));
    }

    mh = mouse_is_hidden;

    if (!mouse_is_hidden) {
        mouse_hide();
    }

    if (width != mouse_width || length != mouse_length) {
        uint8_t *temp;

        temp = (uint8_t *)malloc(length * width);
        if (!temp) {
            if (!mh) {
                mouse_show();
            }

            return -1;
        }

        if (mouse_buf) {
            free(mouse_buf);
        }

        mouse_buf = temp;
    }

    mouse_shape = buf;
    mouse_width = width;
    mouse_length = length;
    mouse_full = full;
    mouse_trans = trans;

    if (mouse_fptr) {
        remove_bk_process(mouse_anim);
    }

    mouse_fptr = NULL;

    mouse_x += mouse_hotx - hotx;
    mouse_y += mouse_hoty - hoty;
    mouse_hotx = hotx;
    mouse_hoty = hoty;

    mouse_clip();

    if (!mh) {
        mouse_show();
    }

    return 0;
}

int32_t mouse_get_anim(uint8_t **frames, int32_t *num_frames, int32_t *width, int32_t *length, int32_t *hotx,
                       int32_t *hoty, char *trans, TOCKS *speed) {
    int32_t result;

    if (mouse_fptr) {
        *frames = mouse_fptr;
        *num_frames = mouse_num_frames;
        *width = mouse_width;
        *length = mouse_length;
        *hotx = mouse_hotx;
        *hoty = mouse_hoty;
        *trans = mouse_trans;
        *speed = mouse_speed;
        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int32_t mouse_set_anim_frames(uint8_t *frames, int32_t num_frames, int32_t start_frame, int32_t width, int32_t length,
                              int32_t hotx, int32_t hoty, char trans, TOCKS speed) {
    int32_t result;

    result = mouse_set_shape(&frames[length * width * start_frame], width, length, width, hotx, hoty, trans);

    if (result != -1) {
        mouse_fptr = frames;
        mouse_num_frames = num_frames;
        mouse_speed = speed;
        mouse_curr_frame = start_frame;
        add_bk_process(mouse_anim);

        result = 0;
    }

    return result;
}

void mouse_anim(void) {
    static TOCKS ticker = 0;

    if (elapsed_time(ticker) >= mouse_speed) {
        ticker = get_time();

        mouse_curr_frame++;

        if (mouse_curr_frame == mouse_num_frames) {
            mouse_curr_frame = 0;
        }

        mouse_shape = &mouse_fptr[mouse_width * mouse_length * mouse_curr_frame];

        if (!mouse_is_hidden) {
            mouse_show();
        }
    }
}

void mouse_show(void) {
    int32_t xlen;
    int32_t ylen;
    int32_t xoff;
    int32_t yoff;

    if (have_mouse) {
        win_get_mouse_buf(mouse_buf);

        yoff = 0;
        xoff = 0;

        for (int32_t i = 0; i < mouse_length; i++) {
            for (int32_t j = 0; j < mouse_width; j++) {
                if (mouse_shape[j + yoff] != mouse_trans) {
                    mouse_buf[xoff] = mouse_shape[j + yoff];
                }

                xoff++;
            }
            yoff += mouse_full;
        }

        if (mouse_x >= scr_size.ulx) {
            if (mouse_width + mouse_x - 1 <= scr_size.lrx) {
                xoff = 0;
                xlen = mouse_width;
            } else {
                xoff = 0;
                xlen = scr_size.lrx - mouse_x + 1;
            }
        } else {
            xoff = scr_size.ulx - mouse_x;
            xlen = mouse_width - (scr_size.ulx - mouse_x);
        }

        if (mouse_y >= scr_size.uly) {
            if (mouse_length + mouse_y - 1 <= scr_size.lry) {
                yoff = 0;
                ylen = mouse_length;
            } else {
                yoff = 0;
                ylen = scr_size.lry - mouse_y + 1;
            }
        } else {
            yoff = scr_size.uly - mouse_y;
            ylen = mouse_length - (scr_size.uly - mouse_y);
        }

        mouse_blit(mouse_buf, mouse_width, mouse_length, xoff, yoff, xlen, ylen, xoff + mouse_x, yoff + mouse_y);

        mouse_is_hidden = 0;
    }
}

void mouse_hide(void) {
    Rect rect;

    if (have_mouse) {
        if (!mouse_is_hidden) {
            mouse_is_hidden = 1;
            mouse_get_rect(&rect);
            win_refresh_all(&rect);
        }
    }
}

void mouse_info(void) {
    uint32_t buttons = 0;
    int delta_x;
    int delta_y;

    if (have_mouse && !mouse_is_hidden && !mouse_disabled) {
        uint32_t button_bitmask;

        button_bitmask = SDL_GetRelativeMouseState(&delta_x, &delta_y);

        if (SDL_BUTTON(SDL_BUTTON_LEFT) & button_bitmask) {
            buttons |= GNW_MOUSE_BUTTON_LEFT;
        }

        if (SDL_BUTTON(SDL_BUTTON_RIGHT) & button_bitmask) {
            buttons |= GNW_MOUSE_BUTTON_RIGHT;
        }

        if (SDL_BUTTON(SDL_BUTTON_MIDDLE) & button_bitmask) {
            buttons |= GNW_MOUSE_BUTTON_MIDDLE;
        }

        delta_x = (int32_t)((double)delta_x * mouse_sensitivity);
        delta_y = (int32_t)((double)delta_y * mouse_sensitivity);

        mouse_simulate_input(delta_x, delta_y, buttons);
    }
}

void mouse_simulate_input(int32_t delta_x, int32_t delta_y, uint32_t buttons) {
    static uint32_t right_time;
    static uint32_t left_time;
    static int32_t old;

    if (!have_mouse || mouse_is_hidden) {
        return;
    }

    old = mouse_buttons;
    mouse_buttons = 0;
    last_buttons = buttons;

    if (old & (MOUSE_PRESS_LEFT | MOUSE_LONG_PRESS_LEFT)) {
        if (buttons & GNW_MOUSE_BUTTON_LEFT) {
            mouse_buttons = MOUSE_LONG_PRESS_LEFT;

            if (elapsed_time(left_time) > 250) {
                mouse_buttons |= MOUSE_PRESS_LEFT;
                left_time = get_time();
            }
        } else {
            mouse_buttons = MOUSE_RELEASE_LEFT;
        }
    } else {
        if (buttons & GNW_MOUSE_BUTTON_LEFT) {
            mouse_buttons = MOUSE_PRESS_LEFT;
            left_time = get_time();
        }
    }

    if (old & (MOUSE_PRESS_RIGHT | MOUSE_LONG_PRESS_RIGHT)) {
        if (buttons & GNW_MOUSE_BUTTON_RIGHT) {
            mouse_buttons |= MOUSE_LONG_PRESS_RIGHT;
            if (elapsed_time(right_time) > 250) {
                mouse_buttons |= MOUSE_PRESS_RIGHT;
                right_time = get_time();
            }
        } else {
            mouse_buttons |= MOUSE_RELEASE_RIGHT;
        }
    } else if (buttons & GNW_MOUSE_BUTTON_RIGHT) {
        mouse_buttons |= MOUSE_PRESS_RIGHT;
        right_time = get_time();
    }

    if (delta_x || delta_y) {
        Rect rect;

        rect.ulx = mouse_x;
        rect.uly = mouse_y;
        rect.lrx = mouse_width + mouse_x - 1;
        rect.lry = mouse_length + mouse_y - 1;

        mouse_x += delta_x;
        mouse_y += delta_y;

        mouse_clip();
        win_refresh_all(&rect);
        mouse_show();
    }
}

int32_t mouse_in(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    int32_t result;

    if (have_mouse) {
        result =
            ((mouse_length + mouse_y) > uly) && (lrx >= mouse_x) && ((mouse_width + mouse_x) > ulx) && (lry >= mouse_y);
    } else {
        result = 0;
    }

    return result;
}

int32_t mouse_click_in(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    int32_t result;

    if (have_mouse) {
        result = ((mouse_hoty + mouse_y) >= uly) && ((mouse_hotx + mouse_x) <= lrx) &&
                 ((mouse_hotx + mouse_x) >= ulx) && ((mouse_hoty + mouse_y) <= lry);
    } else {
        result = 0;
    }

    return result;
}

void mouse_get_rect(Rect *m) {
    m->ulx = mouse_x;
    m->uly = mouse_y;
    m->lrx = mouse_width + mouse_x - 1;
    m->lry = mouse_length + mouse_y - 1;
}

void mouse_get_position(int32_t *x, int32_t *y) {
    *x = mouse_hotx + mouse_x;
    *y = mouse_hoty + mouse_y;
}

void mouse_set_position(int32_t x, int32_t y) {
    mouse_x = x - mouse_hotx;
    mouse_y = y - mouse_hoty;

    mouse_clip();

    Svga_WarpMouse(mouse_x, mouse_y);
}

void mouse_clip(void) {
    if ((mouse_hotx + mouse_x) >= scr_size.ulx) {
        if ((mouse_hotx + mouse_x) > scr_size.lrx) {
            mouse_x = scr_size.lrx - mouse_hotx;
        }
    } else {
        mouse_x = scr_size.ulx - mouse_hotx;
    }
    if ((mouse_hoty + mouse_y) >= scr_size.uly) {
        if ((mouse_hoty + mouse_y) > scr_size.lry) {
            mouse_y = scr_size.lry - mouse_hoty;
        }
    } else {
        mouse_y = scr_size.uly - mouse_hoty;
    }
}

uint32_t mouse_get_buttons(void) { return mouse_buttons; }

int32_t mouse_hidden(void) { return mouse_is_hidden; }

void mouse_get_hotspot(int32_t *hotx, int32_t *hoty) {
    *hotx = mouse_hotx;
    *hoty = mouse_hoty;
}

void mouse_set_hotspot(int32_t hotx, int32_t hoty) {
    if (!mouse_is_hidden) {
        mouse_hide();
    }

    mouse_x += mouse_hotx - hotx;
    mouse_y += mouse_hoty - hoty;
    mouse_hotx = hotx;
    mouse_hoty = hoty;

    Svga_WarpMouse(mouse_x, mouse_y);

    if (!mouse_is_hidden) {
        mouse_show();
    }
}

int32_t mouse_query_exist(void) { return have_mouse; }

void mouse_disable(void) { mouse_disabled = 1; }

void mouse_enable(void) { mouse_disabled = 0; }

int32_t mouse_is_disabled(void) { return mouse_disabled; }

void mouse_set_sensitivity(double new_sensitivity) {
    if (new_sensitivity > 0.0 && new_sensitivity < 2.0) {
        mouse_sensitivity = new_sensitivity;
    }
}

double mouse_get_sensitivity(void) { return mouse_sensitivity; }
