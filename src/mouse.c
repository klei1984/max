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

#include <SDL3/SDL.h>

#include "cursor.h"
#include "gnw.h"
#include "svga.h"

#define GNW_MOUSE_BUTTON_LEFT 1
#define GNW_MOUSE_BUTTON_RIGHT 2
#define GNW_MOUSE_BUTTON_MIDDLE 4

static void mouse_clip(void);

static float mouse_sensitivity = 1.f;
static int32_t mouse_wheel_sensitivity = 3;
static uint32_t last_buttons;
static int32_t mouse_is_hidden;
static int32_t mouse_disabled;
static int32_t mouse_buttons;
static int32_t mouse_hotx;
static int32_t mouse_hoty;
static int32_t mouse_x;
static int32_t mouse_y;
static int32_t have_mouse;
static int32_t mouse_width;
static int32_t mouse_length;
static int32_t mouse_lock;
static int32_t mouse_wheel_x;
static int32_t mouse_wheel_y;

ScreenBlitFunc mouse_blit;

int32_t GNW_mouse_init(void) {
    int32_t result;
    uint32_t window_flags;

    if (!Svga_GetWindowFlags(&window_flags)) {
        result = -1;

    } else {
        have_mouse = 0;
        mouse_disabled = 0;
        mouse_is_hidden = 1;
        mouse_lock = (window_flags & SDL_WINDOW_FULLSCREEN) ? MOUSE_LOCK_LOCKED : MOUSE_LOCK_UNLOCKED;
        mouse_wheel_x = 0;
        mouse_wheel_y = 0;
        mouse_width = 1;
        mouse_length = 1;
        mouse_hotx = 0;
        mouse_hoty = 0;
        mouse_blit = scr_blit;
        mouse_x = scr_size.ulx / 2;
        mouse_y = scr_size.uly / 2;
        have_mouse = 1;

        result = 0;
    }

    return result;
}

void mouse_show(void) {
    if (have_mouse && mouse_is_hidden) {
        mouse_is_hidden = 0;
        Cursor_Show();
    }
}

void mouse_hide(void) {
    if (have_mouse && !mouse_is_hidden) {
        mouse_is_hidden = 1;
        Cursor_Hide();
    }
}

void mouse_info(void) {
    if (have_mouse && !mouse_disabled) {
        uint32_t buttons = 0;
        uint32_t button_bitmask;
        float window_x;
        float window_y;
        float logical_x;
        float logical_y;

        SDL_PumpEvents();

        // Get mouse position in window coordinates
        button_bitmask = SDL_GetMouseState(&window_x, &window_y);

        // Convert window coordinates to logical coordinates (handles fullscreen scaling)
        Svga_WindowToLogicalCoordinates(window_x, window_y, &logical_x, &logical_y);

        // Calculate delta from current mouse position
        float delta_x = logical_x - (mouse_hotx + mouse_x);
        float delta_y = logical_y - (mouse_hoty + mouse_y);

        if (SDL_BUTTON_MASK(SDL_BUTTON_LEFT) & button_bitmask) {
            buttons |= GNW_MOUSE_BUTTON_LEFT;
        }

        if (SDL_BUTTON_MASK(SDL_BUTTON_RIGHT) & button_bitmask) {
            buttons |= GNW_MOUSE_BUTTON_RIGHT;
        }

        if (SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE) & button_bitmask) {
            buttons |= GNW_MOUSE_BUTTON_MIDDLE;
        }

        if (mouse_lock == MOUSE_LOCK_FOCUSED && buttons) {
            mouse_lock = MOUSE_LOCK_LOCKED;
            buttons = 0;
        }

        if (mouse_lock == MOUSE_LOCK_LOCKED) {
            mouse_simulate_input(delta_x, delta_y, buttons);
        }
    }
}

void mouse_simulate_input(int32_t delta_x, int32_t delta_y, uint32_t buttons) {
    static uint64_t right_time;
    static uint64_t left_time;
    static int32_t old;

    if (!have_mouse) {
        return;
    }

    old = mouse_buttons;
    mouse_buttons = 0;
    last_buttons = buttons;

    if (old & (MOUSE_PRESS_LEFT | MOUSE_LONG_PRESS_LEFT)) {
        if (buttons & GNW_MOUSE_BUTTON_LEFT) {
            mouse_buttons = MOUSE_LONG_PRESS_LEFT;

            if (timer_elapsed_time(left_time) > 250) {
                mouse_buttons |= MOUSE_PRESS_LEFT;
                left_time = timer_get();
            }
        } else {
            mouse_buttons = MOUSE_RELEASE_LEFT;
        }
    } else {
        if (buttons & GNW_MOUSE_BUTTON_LEFT) {
            mouse_buttons = MOUSE_PRESS_LEFT;
            left_time = timer_get();
        }
    }

    if (old & (MOUSE_PRESS_RIGHT | MOUSE_LONG_PRESS_RIGHT)) {
        if (buttons & GNW_MOUSE_BUTTON_RIGHT) {
            mouse_buttons |= MOUSE_LONG_PRESS_RIGHT;
            if (timer_elapsed_time(right_time) > 250) {
                mouse_buttons |= MOUSE_PRESS_RIGHT;
                right_time = timer_get();
            }
        } else {
            mouse_buttons |= MOUSE_RELEASE_RIGHT;
        }
    } else if (buttons & GNW_MOUSE_BUTTON_RIGHT) {
        mouse_buttons |= MOUSE_PRESS_RIGHT;
        right_time = timer_get();
    }

    if (delta_x || delta_y) {
        mouse_x += delta_x;
        mouse_y += delta_y;

        mouse_clip();
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

void mouse_get_rect(Rect* m) {
    m->ulx = mouse_x;
    m->uly = mouse_y;
    m->lrx = mouse_width + mouse_x - 1;
    m->lry = mouse_length + mouse_y - 1;
}

void mouse_get_position(int32_t* x, int32_t* y) {
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

void mouse_get_hotspot(int32_t* hotx, int32_t* hoty) {
    *hotx = mouse_hotx;
    *hoty = mouse_hoty;
}

void mouse_set_hotspot(int32_t hotx, int32_t hoty) {
    mouse_x += mouse_hotx - hotx;
    mouse_y += mouse_hoty - hoty;
    mouse_hotx = hotx;
    mouse_hoty = hoty;

    Svga_WarpMouse(mouse_x, mouse_y);
}

int32_t mouse_query_exist(void) { return have_mouse; }

void mouse_disable(void) { mouse_disabled = 1; }

void mouse_enable(void) { mouse_disabled = 0; }

int32_t mouse_is_disabled(void) { return mouse_disabled; }

void mouse_set_sensitivity(float new_sensitivity) {
    if (new_sensitivity > 0.f && new_sensitivity < 2.f) {
        mouse_sensitivity = new_sensitivity;
    }
}

float mouse_get_sensitivity(void) { return mouse_sensitivity; }

int32_t mouse_get_lock(void) { return mouse_lock; }

void mouse_set_lock(int32_t state) { mouse_lock = state; }

void mouse_add_wheel_event(int32_t delta_x, int32_t delta_y) {
    mouse_wheel_x += delta_x;
    mouse_wheel_y += delta_y;
}

void mouse_get_wheel_delta(int32_t* delta_x, int32_t* delta_y) {
    *delta_x = mouse_wheel_x * mouse_wheel_sensitivity;
    *delta_y = mouse_wheel_y * mouse_wheel_sensitivity;
    mouse_wheel_x = 0;
    mouse_wheel_y = 0;
}

void mouse_set_wheel_sensitivity(int32_t new_sensitivity) {
    if (new_sensitivity >= 1 && new_sensitivity <= 10) {
        mouse_wheel_sensitivity = new_sensitivity;
    }
}

int32_t mouse_get_wheel_sensitivity(void) { return mouse_wheel_sensitivity; }

void mouse_set_fullscreen_mode(int32_t is_fullscreen) {
    if (have_mouse) {
        float window_x, window_y;

        // Clip mouse position to screen bounds in case it was outside
        mouse_clip();

        // Convert logical coordinates to window coordinates for SDL
        Svga_LogicalToWindowCoordinates((float)(mouse_hotx + mouse_x), (float)(mouse_hoty + mouse_y), &window_x,
                                        &window_y);

        // Warp SDL mouse to current logical position to prevent cursor jumping on next mouse_info() call
        SDL_WarpMouseInWindow(Svga_GetWindow(), window_x, window_y);
    }
}
