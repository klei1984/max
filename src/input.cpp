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

#include "input.h"

#include <SDL3/SDL.h>
#include <string.h>

#include <memory>
#include <unordered_map>

#include "cursor.hpp"
#include "gnw.h"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "screendump.h"
#include "svga.h"

#define GNW_INPUT_BUFFER_SIZE 40

#define GNW_KB_KEY_MAP_ITEM(sdl_key, base, shift_key, lalt, ralt, ctrl_key) \
    {                                                                       \
        sdl_key, { base, shift_key, lalt, ralt, ctrl_key }                  \
    }

typedef struct FuncData* FuncPtr;

struct KeyMapping {
    int32_t key;
    int32_t shift;
    int32_t left_alt;
    int32_t right_alt;
    int32_t ctrl;
};

struct FuncData {
    uint32_t flags;
    BackgroundProcess f;
    FuncPtr next;
};

struct InputData {
    int32_t input;
    int32_t mx;
    int32_t my;
};

static int32_t get_input_buffer(void);
static void pause_game(void);
static WinID default_pause_window(void);
static void GNW_process_key(SDL_KeyboardEvent* key_data);
static int32_t input_sdl_to_game_key(SDL_Keycode sdl_key, uint16_t sdl_mod);

static std::unique_ptr<std::unordered_map<SDL_Keycode, KeyMapping>> input_key_map;
static InputData input_buffer[GNW_INPUT_BUFFER_SIZE];
static int32_t input_mx;
static int32_t input_my;
static FuncPtr bk_list;
static int32_t pause_key;
static int32_t input_get;
static PauseWinFunc pause_win_func;
static int32_t input_put;
static UTF8InputFunc utf8_input_callback;
static uint8_t input_key_states[SDL_SCANCODE_COUNT];
static bool game_paused;
static bool bk_disabled;

int32_t GNW_input_init(void) {
    int32_t result;

    bk_list = NULL;

    input_put = 0;
    input_get = -1;
    input_mx = -1;
    input_my = -1;
    utf8_input_callback = NULL;

    memset(input_key_states, 0, sizeof(input_key_states));

    input_key_map = std::make_unique<
        std::unordered_map<SDL_Keycode, KeyMapping>>(std::initializer_list<std::pair<const SDL_Keycode, KeyMapping>>{

        // Special keys
        GNW_KB_KEY_MAP_ITEM(SDLK_ESCAPE, GNW_KB_KEY_ESCAPE, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_RETURN, GNW_KB_KEY_RETURN, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_BACKSPACE, GNW_KB_KEY_BACKSPACE, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_TAB, GNW_KB_KEY_TAB, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_SPACE, GNW_KB_KEY_SPACE, -1, -1, -1, -1),

        // Commands
        GNW_KB_KEY_MAP_ITEM(SDLK_A, -1, -1, GNW_KB_KEY_LALT_A, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_B, -1, -1, GNW_KB_KEY_LALT_B, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_C, -1, -1, GNW_KB_KEY_LALT_C, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_D, -1, -1, GNW_KB_KEY_LALT_D, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_E, -1, -1, GNW_KB_KEY_LALT_E, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_F, -1, -1, GNW_KB_KEY_LALT_F, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_G, -1, -1, GNW_KB_KEY_LALT_G, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_H, -1, -1, GNW_KB_KEY_LALT_H, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_I, -1, -1, GNW_KB_KEY_LALT_I, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_J, -1, -1, GNW_KB_KEY_LALT_J, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_K, -1, -1, GNW_KB_KEY_LALT_K, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_L, -1, -1, GNW_KB_KEY_LALT_L, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_M, -1, -1, GNW_KB_KEY_LALT_M, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_N, -1, -1, GNW_KB_KEY_LALT_N, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_O, -1, -1, GNW_KB_KEY_LALT_O, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_P, -1, -1, GNW_KB_KEY_LALT_P, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_Q, -1, -1, GNW_KB_KEY_LALT_Q, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_R, -1, -1, GNW_KB_KEY_LALT_R, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_S, -1, -1, GNW_KB_KEY_LALT_S, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_T, -1, -1, GNW_KB_KEY_LALT_T, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_U, -1, -1, GNW_KB_KEY_LALT_U, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_V, -1, -1, GNW_KB_KEY_LALT_V, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_W, -1, -1, GNW_KB_KEY_LALT_W, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_X, -1, -1, GNW_KB_KEY_LALT_X, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_Y, -1, -1, GNW_KB_KEY_LALT_Y, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_Z, -1, -1, GNW_KB_KEY_LALT_Z, -1, -1),

        // Function keys
        GNW_KB_KEY_MAP_ITEM(SDLK_F1, GNW_KB_KEY_F1, GNW_KB_KEY_SHIFT_F1, GNW_KB_KEY_LALT_F1, -1, GNW_KB_KEY_CTRL_F1),
        GNW_KB_KEY_MAP_ITEM(SDLK_F2, GNW_KB_KEY_F2, GNW_KB_KEY_SHIFT_F2, GNW_KB_KEY_LALT_F2, -1, GNW_KB_KEY_CTRL_F2),
        GNW_KB_KEY_MAP_ITEM(SDLK_F3, GNW_KB_KEY_F3, GNW_KB_KEY_SHIFT_F3, GNW_KB_KEY_LALT_F3, -1, GNW_KB_KEY_CTRL_F3),
        GNW_KB_KEY_MAP_ITEM(SDLK_F4, GNW_KB_KEY_F4, GNW_KB_KEY_SHIFT_F4, GNW_KB_KEY_LALT_F4, -1, GNW_KB_KEY_CTRL_F4),
        GNW_KB_KEY_MAP_ITEM(SDLK_F5, GNW_KB_KEY_F5, GNW_KB_KEY_SHIFT_F5, GNW_KB_KEY_LALT_F5, -1, GNW_KB_KEY_CTRL_F5),
        GNW_KB_KEY_MAP_ITEM(SDLK_F6, GNW_KB_KEY_F6, GNW_KB_KEY_SHIFT_F6, GNW_KB_KEY_LALT_F6, -1, GNW_KB_KEY_CTRL_F6),
        GNW_KB_KEY_MAP_ITEM(SDLK_F7, GNW_KB_KEY_F7, GNW_KB_KEY_SHIFT_F7, GNW_KB_KEY_LALT_F7, -1, GNW_KB_KEY_CTRL_F7),
        GNW_KB_KEY_MAP_ITEM(SDLK_F8, GNW_KB_KEY_F8, GNW_KB_KEY_SHIFT_F8, GNW_KB_KEY_LALT_F8, -1, GNW_KB_KEY_CTRL_F8),
        GNW_KB_KEY_MAP_ITEM(SDLK_F9, GNW_KB_KEY_F9, GNW_KB_KEY_SHIFT_F9, GNW_KB_KEY_LALT_F9, -1, GNW_KB_KEY_CTRL_F9),
        GNW_KB_KEY_MAP_ITEM(SDLK_F10, GNW_KB_KEY_F10, GNW_KB_KEY_SHIFT_F10, GNW_KB_KEY_LALT_F10, -1,
                            GNW_KB_KEY_CTRL_F10),
        GNW_KB_KEY_MAP_ITEM(SDLK_F11, GNW_KB_KEY_F11, GNW_KB_KEY_SHIFT_F11, GNW_KB_KEY_LALT_F11, -1,
                            GNW_KB_KEY_CTRL_F11),
        GNW_KB_KEY_MAP_ITEM(SDLK_F12, GNW_KB_KEY_F12, GNW_KB_KEY_SHIFT_F12, GNW_KB_KEY_LALT_F12, -1,
                            GNW_KB_KEY_CTRL_F12),

        // Navigation keys
        GNW_KB_KEY_MAP_ITEM(SDLK_HOME, GNW_KB_KEY_HOME, GNW_KB_KEY_SHIFT_HOME, GNW_KB_KEY_LALT_HOME, -1,
                            GNW_KB_KEY_CTRL_HOME),
        GNW_KB_KEY_MAP_ITEM(SDLK_END, GNW_KB_KEY_END, GNW_KB_KEY_SHIFT_END, GNW_KB_KEY_LALT_END, -1,
                            GNW_KB_KEY_CTRL_END),
        GNW_KB_KEY_MAP_ITEM(SDLK_PAGEUP, GNW_KB_KEY_PAGEUP, GNW_KB_KEY_SHIFT_PAGEUP, GNW_KB_KEY_LALT_PAGEUP, -1,
                            GNW_KB_KEY_CTRL_PAGEUP),
        GNW_KB_KEY_MAP_ITEM(SDLK_PAGEDOWN, GNW_KB_KEY_PAGEDOWN, GNW_KB_KEY_SHIFT_PAGEDOWN, GNW_KB_KEY_LALT_PAGEDOWN, -1,
                            GNW_KB_KEY_CTRL_PAGEDOWN),
        GNW_KB_KEY_MAP_ITEM(SDLK_UP, GNW_KB_KEY_UP, GNW_KB_KEY_SHIFT_UP, GNW_KB_KEY_LALT_UP, -1, GNW_KB_KEY_CTRL_UP),
        GNW_KB_KEY_MAP_ITEM(SDLK_DOWN, GNW_KB_KEY_DOWN, GNW_KB_KEY_SHIFT_DOWN, GNW_KB_KEY_LALT_DOWN, -1,
                            GNW_KB_KEY_CTRL_DOWN),
        GNW_KB_KEY_MAP_ITEM(SDLK_LEFT, GNW_KB_KEY_LEFT, GNW_KB_KEY_SHIFT_LEFT, GNW_KB_KEY_LALT_LEFT, -1,
                            GNW_KB_KEY_CTRL_LEFT),
        GNW_KB_KEY_MAP_ITEM(SDLK_RIGHT, GNW_KB_KEY_RIGHT, GNW_KB_KEY_SHIFT_RIGHT, GNW_KB_KEY_LALT_RIGHT, -1,
                            GNW_KB_KEY_CTRL_RIGHT),
        GNW_KB_KEY_MAP_ITEM(SDLK_INSERT, GNW_KB_KEY_INSERT, GNW_KB_KEY_SHIFT_INSERT, GNW_KB_KEY_LALT_INSERT, -1,
                            GNW_KB_KEY_CTRL_INSERT),
        GNW_KB_KEY_MAP_ITEM(SDLK_DELETE, GNW_KB_KEY_DELETE, GNW_KB_KEY_SHIFT_DELETE, GNW_KB_KEY_LALT_DELETE, -1,
                            GNW_KB_KEY_CTRL_DELETE),

        // Punctuation - handled by SDL_EVENT_TEXT_INPUT
        GNW_KB_KEY_MAP_ITEM(SDLK_MINUS, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_EQUALS, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_LEFTBRACKET, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_RIGHTBRACKET, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_SEMICOLON, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_APOSTROPHE, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_COMMA, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_PERIOD, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_SLASH, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_GRAVE, -1, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_BACKSLASH, -1, -1, -1, -1, GNW_KB_KEY_CTRL_BACKSLASH),

        // Keypad
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_ENTER, GNW_KB_KEY_KP_ENTER, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_PLUS, GNW_KB_KEY_KP_PLUS, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_MINUS, GNW_KB_KEY_KP_MINUS, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_MULTIPLY, GNW_KB_KEY_KP_MULTIPLY, -1, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_DIVIDE, GNW_KB_KEY_KP_DIVIDE, -1, -1, -1, GNW_KB_KEY_CTRL_KP_DIVIDE),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_0, GNW_KB_KEY_INSERT, GNW_KB_KEY_SHIFT_INSERT, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_1, GNW_KB_KEY_END, GNW_KB_KEY_SHIFT_END, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_2, GNW_KB_KEY_DOWN, GNW_KB_KEY_SHIFT_DOWN, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_3, GNW_KB_KEY_PAGEDOWN, GNW_KB_KEY_SHIFT_PAGEDOWN, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_4, GNW_KB_KEY_LEFT, GNW_KB_KEY_SHIFT_LEFT, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_5, GNW_KB_KEY_KP_5, GNW_KB_KEY_SHIFT_KP_5, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_6, GNW_KB_KEY_RIGHT, GNW_KB_KEY_SHIFT_RIGHT, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_7, GNW_KB_KEY_HOME, GNW_KB_KEY_SHIFT_HOME, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_8, GNW_KB_KEY_UP, GNW_KB_KEY_SHIFT_UP, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_9, GNW_KB_KEY_PAGEUP, GNW_KB_KEY_SHIFT_PAGEUP, -1, -1, -1),
        GNW_KB_KEY_MAP_ITEM(SDLK_KP_PERIOD, GNW_KB_KEY_DELETE, GNW_KB_KEY_SHIFT_DELETE, -1, -1, -1),
    });

    SDL_StartTextInput(Svga_GetWindow());

    result = GNW_mouse_init();

    bk_disabled = false;
    game_paused = false;

    pause_key = GNW_KB_KEY_LALT_P;
    pause_win_func = default_pause_window;

    screendump_register(GNW_KB_KEY_LALT_C, nullptr);

    return result;
}

void GNW_input_exit(void) {
    FuncPtr fp;
    FuncPtr np;

    for (fp = bk_list; fp; fp = np) {
        np = fp->next;
        SDL_free(fp);
    }

    bk_list = NULL;

    input_key_map.reset();
}

void GNW_register_utf8_input(UTF8InputFunc callback) { utf8_input_callback = callback; }

void GNW_unregister_utf8_input(void) { utf8_input_callback = NULL; }

void GNW_process_message(void) {
    SDL_Event ev;

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_EVENT_KEY_UP:
            case SDL_EVENT_KEY_DOWN: {
                GNW_process_key(&(ev.key));
            } break;
            case SDL_EVENT_TEXT_INPUT: {
                if (utf8_input_callback) {
                    utf8_input_callback(ev.text.text);

                } else {
                    const char* text = ev.text.text;

                    if (text && text[0]) {
                        if ((unsigned char)text[0] < 128 && !text[1]) {
                            GNW_add_input_buffer((unsigned char)text[0]);
                        }
                    }
                }
            } break;
            case SDL_EVENT_MOUSE_WHEEL: {
                mouse_add_wheel_event(ev.wheel.x, ev.wheel.y);
            } break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                if (mouse_get_lock() != MOUSE_LOCK_LOCKED) {
                    mouse_set_lock(MOUSE_LOCK_FOCUSED);
                }
            } break;
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                mouse_set_lock(MOUSE_LOCK_UNLOCKED);
            } break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_RESIZED: {
                Cursor_UpdateScale();
            } break;
            case SDL_EVENT_WINDOW_MOUSE_ENTER: {
            } break;
            case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
            } break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                Remote_Deinit();
                ResourceManager_ExitGame(EXIT_CODE_THANKS);
            } break;
        }
    }
}

void GNW_process_key(SDL_KeyboardEvent* key_data) {
    if (key_data->scancode < SDL_SCANCODE_COUNT) {
        input_key_states[key_data->scancode] = (key_data->type == SDL_EVENT_KEY_DOWN) ? 1 : 0;
    }

    if (key_data->type == SDL_EVENT_KEY_DOWN) {
        if ((key_data->mod & SDL_KMOD_LALT) && (key_data->key == SDLK_RETURN || key_data->key == SDLK_KP_ENTER)) {
            Svga_ToggleFullscreen();
            return;
        }

        const int32_t game_key = input_sdl_to_game_key(key_data->key, key_data->mod);

        if (game_key != 0) {
            GNW_add_input_buffer(game_key);
        }
    }
}

int32_t get_input(void) {
    int32_t result;
    int32_t input;

    GNW_process_message();

    process_bk();
    input = get_input_buffer();
    if ((input == -1) &&
        (mouse_get_buttons() & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT | MOUSE_RELEASE_LEFT | MOUSE_RELEASE_RIGHT))) {
        mouse_get_position(&input_mx, &input_my);
        result = -2;
    } else {
        result = GNW_check_menu_bars(input);
    }

    return result;
}

void get_input_position(int32_t* x, int32_t* y) {
    *x = input_mx;
    *y = input_my;
}

void process_bk(void) {
    GNW_do_bk_process();

    mouse_info();

    const int32_t input = win_check_all_buttons();

    if (input != -1) {
        GNW_add_input_buffer(input);
    }
}

void GNW_add_input_buffer(int32_t input) {
    if (input != -1) {
        if (input == pause_key) {
            pause_game();

        } else if (input == screendump_get_key()) {
            screendump_dump_screen();

        } else if (input_put != input_get) {
            input_buffer[input_put].input = input;

            mouse_get_position(&input_buffer[input_put].mx, &input_buffer[input_put].my);

            input_put++;

            if (input_put == GNW_INPUT_BUFFER_SIZE) {
                input_put = 0;

            } else if (input_get == -1) {
                input_get = 0;
            }
        }
    }
}

int32_t get_input_buffer(void) {
    int32_t input;

    input = -1;

    if (input_get != -1) {
        input = input_buffer[input_get].input;

        input_mx = input_buffer[input_get].mx;
        input_my = input_buffer[input_get].my;

        input_get++;

        if (input_get == GNW_INPUT_BUFFER_SIZE) {
            input_get = 0;
        }

        if (input_get == input_put) {
            input_put = 0;
            input_get = -1;
        }
    }

    return input;
}

void flush_input_buffer(void) {
    input_put = 0;
    input_get = -1;
}

void GNW_do_bk_process(void) {
    FuncPtr fp;
    FuncPtr next;
    FuncPtr* prev;

    if (!game_paused && !bk_disabled) {
        prev = &bk_list;

        for (fp = bk_list; fp; fp = next) {
            next = fp->next;

            if (fp->flags & 1) {
                *prev = fp->next;
                SDL_free(fp);

            } else {
                fp->f();
                prev = &fp->next;
            }
        }
    }
}

void add_bk_process(BackgroundProcess f) {
    FuncPtr fp;

    for (fp = bk_list; fp; fp = fp->next) {
        if ((BackgroundProcess)fp->f == f) {
            if (fp->flags & 1) {
                fp->flags &= ~1;
            }

            return;
        }
    }

    fp = (FuncPtr)SDL_malloc(sizeof(struct FuncData));

    if (fp) {
        fp->flags = 0;
        fp->f = f;
        fp->next = bk_list;
        bk_list = fp;
    }
}

void remove_bk_process(BackgroundProcess f) {
    FuncPtr fp;

    for (fp = bk_list; fp; fp = fp->next) {
        if (fp->f == f) {
            fp->flags |= 1uL;

            return;
        }
    }
}

void enable_bk(void) { bk_disabled = false; }

void disable_bk(void) { bk_disabled = true; }

void pause_game(void) {
    if (!game_paused) {
        game_paused = true;

        WinID id = pause_win_func();

        while (get_input() != GNW_KB_KEY_ESCAPE) {
            ;
        }

        game_paused = false;

        win_delete(id);
    }
}

WinID default_pause_window(void) {
    uint8_t* buf;
    WinID result;
    WinID id;
    int32_t width;
    int32_t length;

    width = Text_GetWidth("Paused") + 32;
    length = 3 * Text_GetHeight() + 16;

    id = win_add((scr_size.lrx - scr_size.ulx + 1 - width) / 2, (scr_size.lry - scr_size.uly + 1 - length) / 2, width,
                 length, 256, WINDOW_MODAL | WINDOW_MOVE_ON_TOP);

    if (id == -1) {
        result = -1;

    } else {
        win_border(id);

        buf = win_get_buf(id);

        Text_Blit(&buf[8 * width + 16], "Paused", width, width, Color_RGB2Color(0x7C00));

        win_register_text_button(id, (width - Text_GetWidth("Done") - 16) / 2, length - 8 - Text_GetHeight() - 6, -1,
                                 -1, -1, 27, "Done", 0);

        win_draw(id);

        result = id;
    }

    return result;
}

void register_pause(int32_t new_pause_key, PauseWinFunc new_pause_win_func) {
    pause_key = new_pause_key;

    if (new_pause_win_func) {
        pause_win_func = new_pause_win_func;

    } else {
        pause_win_func = default_pause_window;
    }
}

int32_t input_sdl_to_game_key(SDL_Keycode sdl_key, uint16_t sdl_mod) {
    int32_t result = 0;

    if (input_key_map) {
        auto it = input_key_map->find(sdl_key);
        if (it != input_key_map->end()) {
            const KeyMapping& mapping = it->second;
            const bool is_shift_pressed = sdl_mod & SDL_KMOD_SHIFT;
            const bool is_ctrl_pressed = sdl_mod & SDL_KMOD_CTRL;
            const bool is_lalt_pressed = sdl_mod & SDL_KMOD_LALT;

            if (is_shift_pressed && mapping.shift != -1) {
                result = mapping.shift;

            } else if (is_lalt_pressed && mapping.left_alt != -1) {
                result = mapping.left_alt;

            } else if (is_ctrl_pressed && mapping.ctrl != -1) {
                result = mapping.ctrl;

            } else if (mapping.key != -1) {
                result = mapping.key;
            }
        }
    }

    return result;
}

bool input_is_shift_pressed(void) {
    return input_key_states[SDL_SCANCODE_RSHIFT] | input_key_states[SDL_SCANCODE_LSHIFT];
}

bool input_is_ctrl_pressed(void) { return input_key_states[SDL_SCANCODE_LCTRL] | input_key_states[SDL_SCANCODE_RCTRL]; }

bool input_is_alt_pressed(void) { return input_key_states[SDL_SCANCODE_LALT] | input_key_states[SDL_SCANCODE_RALT]; }

bool input_is_up_arrow_pressed(void) {
    return input_key_states[SDL_SCANCODE_UP] || input_key_states[SDL_SCANCODE_KP_8];
}

bool input_is_down_arrow_pressed(void) {
    return input_key_states[SDL_SCANCODE_DOWN] || input_key_states[SDL_SCANCODE_KP_2];
}

bool input_is_left_arrow_pressed(void) {
    return input_key_states[SDL_SCANCODE_LEFT] || input_key_states[SDL_SCANCODE_KP_4];
}

bool input_is_right_arrow_pressed(void) {
    return input_key_states[SDL_SCANCODE_RIGHT] || input_key_states[SDL_SCANCODE_KP_6];
}
