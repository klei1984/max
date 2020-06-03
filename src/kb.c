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

#include "kb.h"

#include <SDL.h>

typedef struct key_ansi_s {
    short keys;
    short normal;
    short shift;
    short alt;
    short ctrl;
} key_ansi_t;

typedef struct key_data_s {
    char scan_code;
    unsigned short modifiers;
} key_data_t;

static unsigned char kb_lock_flags;
static unsigned char kb_installed;
static unsigned char keynumpress;
static int kb_put;
static int kb_get;
static char keys[256];
static kb_layout_t kb_layout;

void kb_init_lock_status(void) {
    SDL_Keymod shift_flags;

    shift_flags = SDL_GetModState();

    kb_lock_flags = 0;

    kb_lock_flags |= (shift_flags & KMOD_NUM) ? 1 : 0;
    kb_lock_flags |= (shift_flags & KMOD_CAPS) ? 2 : 0;
    /* handle scroll lock */
    kb_lock_flags |= (shift_flags & 0) ? 4 : 0;
}

void key_init() {
    if (0 == kb_installed) {
        kb_installed = 1;

        kb_clear();
        kb_init_lock_status();
        kb_set_layout(english);
    }
}

void key_close(void){};

void kb_clear(void) {
    if (kb_installed) {
        keynumpress = 0;
        memset(keys, 0, sizeof(keys));
        kb_put = 0;
        kb_get = 0;
    }
};

int process() {
    const size_t max_events = 256;
    size_t event;
    SDL_Event ev;

    for (event = 0; event < max_events; event++) {
        if (SDL_PollEvent(&ev) == 0) break;

        switch (ev.type) {
            case SDL_KEYUP:
            case SDL_KEYDOWN:
                if (!ev.key.repeat) return ev.key.keysym.sym;
                break;

            case SDL_QUIT:
                SDL_Quit();
                exit(0);
        }
    }

    return -1;
}

int kb_getch(void) {
    if (kb_installed) {
        return process();
    }

    return -1;
};

void kb_set_layout(kb_layout_t layout) { kb_layout = layout; };

kb_layout_t kb_get_layout(void) { return kb_layout; }

void kb_simulate_key(unsigned short scan_code) {}
