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

#include <SDL.h>

#include "gnw.h"

#define GNW_INPUT_BUFFER_SIZE 40

#define GNW_INPUT_SCANCODE_MAP_ITEM(array_index, sdl_scancode, dos_scancode) dos_scancode

typedef struct funcdata *FuncPtr;

struct funcdata {
    unsigned int flags;
    BackgroundProcess f;
    FuncPtr next;
};

typedef struct inputdata_s {
    int input;
    int mx;
    int my;
} inputdata;

static int get_input_buffer(void);
static void pause_game(void);
static WinID default_pause_window(void);
static void buf_blit(unsigned char *buf, unsigned int bufw, unsigned int bufh, unsigned int sx, unsigned int sy,
                     unsigned int w, unsigned int h, unsigned int dstx, unsigned int dsty);
static int default_screendump(int width, int length, unsigned char *buf, unsigned char *pal);
static void GNW_process_key(SDL_KeyboardEvent *key_data);

static inputdata input_buffer[GNW_INPUT_BUFFER_SIZE];
static int input_mx;
static int input_my;
static FuncPtr bk_list;
static int game_paused;
static int screendump_key;
static int using_msec_timer;
static int pause_key;
static ScreenDumpFunc screendump_func;
static int input_get;
static char *screendump_buf;
static PauseWinFunc pause_win_func;
static int input_put;
static int bk_disabled;

int GNW_input_init(int use_msec_timer) {
    int result;

    bk_list = NULL;

    input_put = 0;
    input_get = -1;
    input_mx = -1;
    input_my = -1;

    GNW_kb_set();
    result = GNW_mouse_init();

    bk_disabled = 0;
    game_paused = 0;

    pause_key = 281;
    pause_win_func = default_pause_window;

    screendump_key = 302;
    screendump_func = default_screendump;

    using_msec_timer = use_msec_timer;
    if (use_msec_timer) {
        timer_set_rate();
    }

    return result;
}

void GNW_input_exit(void) {
    FuncPtr fp;
    FuncPtr np;

    for (fp = bk_list; fp; fp = np) {
        np = fp->next;
        free(fp);
    }

    bk_list = NULL;

    GNW_kb_restore();
    GNW_mouse_exit();
}

void GNW_process_message(void) {
    const size_t max_events = 1;
    size_t event;
    SDL_Event ev;

    for (event = 0; (event < max_events) && SDL_PollEvent(&ev); event++) {
        switch (ev.type) {
            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                if (!kb_is_disabled()) {
                    GNW_process_key(&(ev.key));
                }
            } break;
            case SDL_WINDOWEVENT: {
                if (!SDL_GetRelativeMouseMode()) {
                    switch (ev.window.event) {
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            SDL_ShowCursor(SDL_DISABLE);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            SDL_ShowCursor(SDL_ENABLE);
                            break;
                        case SDL_WINDOWEVENT_ENTER: {
                            int mouse_x;
                            int mouse_y;
                            int mouse_state;

                            SDL_GetMouseState(&mouse_x, &mouse_y);

                            mouse_state = mouse_hidden();
                            if (!mouse_state) {
                                mouse_hide();
                            }
                            mouse_set_position(mouse_x, mouse_y);
                            if (!mouse_state) {
                                mouse_show();
                            }
                        } break;
                        case SDL_WINDOWEVENT_LEAVE:
                            break;
                    }
                }
            } break;
        }
    }
}

void GNW_process_key(SDL_KeyboardEvent *key_data) {
    /* map to convert SDL (USB) scan codes to IBM PC scan codes set 1 */
    static const unsigned short input_scancode_map[256] = {
        GNW_INPUT_SCANCODE_MAP_ITEM(0x00, SDL_SCANCODE_UNKNOWN, GNW_KB_SCAN_BUFFER_FULL),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x01, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x02, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x03, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x04, SDL_SCANCODE_A, GNW_KB_SCAN_A),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x05, SDL_SCANCODE_B, GNW_KB_SCAN_B),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x06, SDL_SCANCODE_C, GNW_KB_SCAN_C),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x07, SDL_SCANCODE_D, GNW_KB_SCAN_D),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x08, SDL_SCANCODE_E, GNW_KB_SCAN_E),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x09, SDL_SCANCODE_F, GNW_KB_SCAN_F),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x0A, SDL_SCANCODE_G, GNW_KB_SCAN_G),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x0B, SDL_SCANCODE_H, GNW_KB_SCAN_H),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x0C, SDL_SCANCODE_I, GNW_KB_SCAN_I),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x0D, SDL_SCANCODE_J, GNW_KB_SCAN_J),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x0E, SDL_SCANCODE_K, GNW_KB_SCAN_K),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x0F, SDL_SCANCODE_L, GNW_KB_SCAN_L),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x10, SDL_SCANCODE_M, GNW_KB_SCAN_M),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x11, SDL_SCANCODE_N, GNW_KB_SCAN_N),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x12, SDL_SCANCODE_O, GNW_KB_SCAN_O),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x13, SDL_SCANCODE_P, GNW_KB_SCAN_P),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x14, SDL_SCANCODE_Q, GNW_KB_SCAN_Q),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x15, SDL_SCANCODE_R, GNW_KB_SCAN_R),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x16, SDL_SCANCODE_S, GNW_KB_SCAN_S),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x17, SDL_SCANCODE_T, GNW_KB_SCAN_T),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x18, SDL_SCANCODE_U, GNW_KB_SCAN_U),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x19, SDL_SCANCODE_V, GNW_KB_SCAN_V),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x1A, SDL_SCANCODE_W, GNW_KB_SCAN_W),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x1B, SDL_SCANCODE_X, GNW_KB_SCAN_X),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x1C, SDL_SCANCODE_Y, GNW_KB_SCAN_Y),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x1D, SDL_SCANCODE_Z, GNW_KB_SCAN_Z),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x1E, SDL_SCANCODE_1, GNW_KB_SCAN_1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x1F, SDL_SCANCODE_2, GNW_KB_SCAN_2),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x20, SDL_SCANCODE_3, GNW_KB_SCAN_3),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x21, SDL_SCANCODE_4, GNW_KB_SCAN_4),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x22, SDL_SCANCODE_5, GNW_KB_SCAN_5),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x23, SDL_SCANCODE_6, GNW_KB_SCAN_6),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x24, SDL_SCANCODE_7, GNW_KB_SCAN_7),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x25, SDL_SCANCODE_8, GNW_KB_SCAN_8),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x26, SDL_SCANCODE_9, GNW_KB_SCAN_9),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x27, SDL_SCANCODE_0, GNW_KB_SCAN_0),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x28, SDL_SCANCODE_RETURN, GNW_KB_SCAN_RETURN),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x29, SDL_SCANCODE_ESCAPE, GNW_KB_SCAN_ESCAPE),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x2A, SDL_SCANCODE_BACKSPACE, GNW_KB_SCAN_BACKSPACE),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x2B, SDL_SCANCODE_TAB, GNW_KB_SCAN_TAB),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x2C, SDL_SCANCODE_SPACE, GNW_KB_SCAN_SPACE),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x2D, SDL_SCANCODE_MINUS, GNW_KB_SCAN_MINUS),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x2E, SDL_SCANCODE_EQUALS, GNW_KB_SCAN_EQUALS),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x2F, SDL_SCANCODE_LEFTBRACKET, GNW_KB_SCAN_LEFTBRACKET),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x30, SDL_SCANCODE_RIGHTBRACKET, GNW_KB_SCAN_RIGHTBRACKET),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x31, SDL_SCANCODE_BACKSLASH, GNW_KB_SCAN_BACKSLASH),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x32, SDL_SCANCODE_NONUSHASH, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x33, SDL_SCANCODE_SEMICOLON, GNW_KB_SCAN_SEMICOLON),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x34, SDL_SCANCODE_APOSTROPHE, GNW_KB_SCAN_APOSTROPHE),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x35, SDL_SCANCODE_GRAVE, GNW_KB_SCAN_GRAVE),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x36, SDL_SCANCODE_COMMA, GNW_KB_SCAN_COMMA),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x37, SDL_SCANCODE_PERIOD, GNW_KB_SCAN_PERIOD),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x38, SDL_SCANCODE_SLASH, GNW_KB_SCAN_DIVIDE),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x39, SDL_SCANCODE_CAPSLOCK, GNW_KB_SCAN_CAPSLOCK),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x3A, SDL_SCANCODE_F1, GNW_KB_SCAN_F1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x3B, SDL_SCANCODE_F2, GNW_KB_SCAN_F2),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x3C, SDL_SCANCODE_F3, GNW_KB_SCAN_F3),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x3D, SDL_SCANCODE_F4, GNW_KB_SCAN_F4),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x3E, SDL_SCANCODE_F5, GNW_KB_SCAN_F5),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x3F, SDL_SCANCODE_F6, GNW_KB_SCAN_F6),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x40, SDL_SCANCODE_F7, GNW_KB_SCAN_F7),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x41, SDL_SCANCODE_F8, GNW_KB_SCAN_F8),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x42, SDL_SCANCODE_F9, GNW_KB_SCAN_F9),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x43, SDL_SCANCODE_F10, GNW_KB_SCAN_F10),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x44, SDL_SCANCODE_F11, GNW_KB_SCAN_F11),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x45, SDL_SCANCODE_F12, GNW_KB_SCAN_F12),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x46, SDL_SCANCODE_PRINTSCREEN, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x47, SDL_SCANCODE_SCROLLLOCK, GNW_KB_SCAN_SCROLLLOCK),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x48, SDL_SCANCODE_PAUSE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x49, SDL_SCANCODE_INSERT, GNW_KB_SCAN_INSERT),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x4A, SDL_SCANCODE_HOME, GNW_KB_SCAN_HOME),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x4B, SDL_SCANCODE_PAGEUP, GNW_KB_SCAN_PAGEUP),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x4C, SDL_SCANCODE_DELETE, GNW_KB_SCAN_DELETE),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x4D, SDL_SCANCODE_END, GNW_KB_SCAN_END),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x4E, SDL_SCANCODE_PAGEDOWN, GNW_KB_SCAN_PAGEDOWN),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x4F, SDL_SCANCODE_RIGHT, GNW_KB_SCAN_RIGHT),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x50, SDL_SCANCODE_LEFT, GNW_KB_SCAN_LEFT),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x51, SDL_SCANCODE_DOWN, GNW_KB_SCAN_DOWN),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x52, SDL_SCANCODE_UP, GNW_KB_SCAN_UP),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x53, SDL_SCANCODE_NUMLOCKCLEAR, GNW_KB_SCAN_NUMLOCK),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x54, SDL_SCANCODE_KP_DIVIDE, GNW_KB_SCAN_DIVIDE),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x55, SDL_SCANCODE_KP_MULTIPLY, GNW_KB_SCAN_KP_MULTIPLY),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x56, SDL_SCANCODE_KP_MINUS, GNW_KB_SCAN_KP_MINUS),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x57, SDL_SCANCODE_KP_PLUS, GNW_KB_SCAN_KP_PLUS),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x58, SDL_SCANCODE_KP_ENTER, GNW_KB_SCAN_KP_ENTER),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x59, SDL_SCANCODE_KP_1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x5A, SDL_SCANCODE_KP_2, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x5B, SDL_SCANCODE_KP_3, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x5C, SDL_SCANCODE_KP_4, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x5D, SDL_SCANCODE_KP_5, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x5E, SDL_SCANCODE_KP_6, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x5F, SDL_SCANCODE_KP_7, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x60, SDL_SCANCODE_KP_8, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x61, SDL_SCANCODE_KP_9, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x62, SDL_SCANCODE_KP_0, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x63, SDL_SCANCODE_KP_PERIOD, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x64, SDL_SCANCODE_NONUSBACKSLASH, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x65, SDL_SCANCODE_APPLICATION, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x66, SDL_SCANCODE_POWER, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x67, SDL_SCANCODE_KP_EQUALS, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x68, SDL_SCANCODE_F13, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x69, SDL_SCANCODE_F14, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x6A, SDL_SCANCODE_F15, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x6B, SDL_SCANCODE_F16, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x6C, SDL_SCANCODE_F17, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x6D, SDL_SCANCODE_F18, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x6E, SDL_SCANCODE_F19, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x6F, SDL_SCANCODE_F20, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x70, SDL_SCANCODE_F21, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x71, SDL_SCANCODE_F22, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x72, SDL_SCANCODE_F23, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x73, SDL_SCANCODE_F24, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x74, SDL_SCANCODE_EXECUTE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x75, SDL_SCANCODE_HELP, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x76, SDL_SCANCODE_MENU, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x77, SDL_SCANCODE_SELECT, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x78, SDL_SCANCODE_STOP, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x79, SDL_SCANCODE_AGAIN, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x7A, SDL_SCANCODE_UNDO, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x7B, SDL_SCANCODE_CUT, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x7C, SDL_SCANCODE_COPY, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x7D, SDL_SCANCODE_PASTE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x7E, SDL_SCANCODE_FIND, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x7F, SDL_SCANCODE_MUTE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x80, SDL_SCANCODE_VOLUMEUP, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x81, SDL_SCANCODE_VOLUMEDOWN, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x82, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x83, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x84, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x85, SDL_SCANCODE_KP_COMMA, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x86, SDL_SCANCODE_KP_EQUALSAS400, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x87, SDL_SCANCODE_INTERNATIONAL1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x88, SDL_SCANCODE_INTERNATIONAL2, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x89, SDL_SCANCODE_INTERNATIONAL3, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x8A, SDL_SCANCODE_INTERNATIONAL4, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x8B, SDL_SCANCODE_INTERNATIONAL5, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x8C, SDL_SCANCODE_INTERNATIONAL6, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x8D, SDL_SCANCODE_INTERNATIONAL7, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x8E, SDL_SCANCODE_INTERNATIONAL8, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x8F, SDL_SCANCODE_INTERNATIONAL9, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x90, SDL_SCANCODE_LANG1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x91, SDL_SCANCODE_LANG2, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x92, SDL_SCANCODE_LANG3, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x93, SDL_SCANCODE_LANG4, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x94, SDL_SCANCODE_LANG5, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x95, SDL_SCANCODE_LANG6, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x96, SDL_SCANCODE_LANG7, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x97, SDL_SCANCODE_LANG8, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x98, SDL_SCANCODE_LANG9, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x99, SDL_SCANCODE_ALTERASE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x9A, SDL_SCANCODE_SYSREQ, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x9B, SDL_SCANCODE_CANCEL, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x9C, SDL_SCANCODE_CLEAR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x9D, SDL_SCANCODE_PRIOR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x9E, SDL_SCANCODE_RETURN2, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0x9F, SDL_SCANCODE_SEPARATOR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA0, SDL_SCANCODE_OUT, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA1, SDL_SCANCODE_OPER, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA2, SDL_SCANCODE_CLEARAGAIN, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA3, SDL_SCANCODE_CRSEL, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA4, SDL_SCANCODE_EXSEL, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA5, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA6, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA7, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA8, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xA9, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xAA, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xAB, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xAC, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xAD, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xAE, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xAF, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB0, SDL_SCANCODE_KP_00, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB1, SDL_SCANCODE_KP_000, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB2, SDL_SCANCODE_THOUSANDSSEPARATOR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB3, SDL_SCANCODE_DECIMALSEPARATOR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB4, SDL_SCANCODE_CURRENCYUNIT, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB5, SDL_SCANCODE_CURRENCYSUBUNIT, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB6, SDL_SCANCODE_KP_LEFTPAREN, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB7, SDL_SCANCODE_KP_RIGHTPAREN, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB8, SDL_SCANCODE_KP_LEFTBRACE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xB9, SDL_SCANCODE_KP_RIGHTBRACE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xBA, SDL_SCANCODE_KP_TAB, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xBB, SDL_SCANCODE_KP_BACKSPACE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xBC, SDL_SCANCODE_KP_A, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xBD, SDL_SCANCODE_KP_B, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xBE, SDL_SCANCODE_KP_C, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xBF, SDL_SCANCODE_KP_D, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC0, SDL_SCANCODE_KP_E, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC1, SDL_SCANCODE_KP_F, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC2, SDL_SCANCODE_KP_XOR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC3, SDL_SCANCODE_KP_POWER, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC4, SDL_SCANCODE_KP_PERCENT, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC5, SDL_SCANCODE_KP_LESS, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC6, SDL_SCANCODE_KP_GREATER, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC7, SDL_SCANCODE_KP_AMPERSAND, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC8, SDL_SCANCODE_KP_DBLAMPERSAND, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xC9, SDL_SCANCODE_KP_VERTICALBAR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xCA, SDL_SCANCODE_KP_DBLVERTICALBAR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xCB, SDL_SCANCODE_KP_COLON, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xCC, SDL_SCANCODE_KP_HASH, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xCD, SDL_SCANCODE_KP_SPACE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xCE, SDL_SCANCODE_KP_AT, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xCF, SDL_SCANCODE_KP_EXCLAM, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD0, SDL_SCANCODE_KP_MEMSTORE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD1, SDL_SCANCODE_KP_MEMRECALL, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD2, SDL_SCANCODE_KP_MEMCLEAR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD3, SDL_SCANCODE_KP_MEMADD, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD4, SDL_SCANCODE_KP_MEMSUBTRACT, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD5, SDL_SCANCODE_KP_MEMMULTIPLY, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD6, SDL_SCANCODE_KP_MEMDIVIDE, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD7, SDL_SCANCODE_KP_PLUSMINUS, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD8, SDL_SCANCODE_KP_CLEAR, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xD9, SDL_SCANCODE_KP_CLEARENTRY, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xDA, SDL_SCANCODE_KP_BINARY, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xDB, SDL_SCANCODE_KP_OCTAL, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xDC, SDL_SCANCODE_KP_DECIMAL, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xDD, SDL_SCANCODE_KP_HEXADECIMAL, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xDE, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xDF, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE0, SDL_SCANCODE_LCTRL, GNW_KB_SCAN_LCTRL),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE1, SDL_SCANCODE_LSHIFT, GNW_KB_SCAN_LSHIFT),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE2, SDL_SCANCODE_LALT, GNW_KB_SCAN_LALT),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE3, SDL_SCANCODE_LGUI, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE4, SDL_SCANCODE_RCTRL, GNW_KB_SCAN_RCTRL),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE5, SDL_SCANCODE_RSHIFT, GNW_KB_SCAN_RSHIFT),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE6, SDL_SCANCODE_RALT, GNW_KB_SCAN_RALT),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE7, SDL_SCANCODE_RGUI, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE8, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xE9, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xEA, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xEB, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xEC, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xED, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xEE, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xEF, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF0, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF1, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF2, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF3, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF4, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF5, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF6, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF7, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF8, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xF9, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xFA, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xFB, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xFC, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xFD, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xFE, -1, -1),
        GNW_INPUT_SCANCODE_MAP_ITEM(0xFF, -1, -1),
    };

    if (vcr_state == 1) {
        if (vcr_terminate_flags & 1) {
            vcr_terminated_condition = 2;
            vcr_stop();
        }
    } else {
        switch (key_data->type) {
            case SDL_KEYDOWN:
                if (key_data->keysym.scancode < 256) {
                    kb_simulate_key(input_scancode_map[key_data->keysym.scancode]);
                }
                break;

            case SDL_KEYUP:
                if (key_data->keysym.scancode < 256) {
                    kb_simulate_key(0x80 | input_scancode_map[key_data->keysym.scancode]);
                }
                break;
        }
    }
}

int get_input(void) {
    int result;
    int i;

    GNW_process_message();

    process_bk();
    i = get_input_buffer();
    if ((i == -1) &&
        (mouse_get_buttons() & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT | MOUSE_RELEASE_LEFT | MOUSE_RELEASE_RIGHT))) {
        mouse_get_position(&input_mx, &input_my);
        result = -2;
    } else {
        result = GNW_check_menu_bars(i);
    }

    return result;
}

void get_input_position(int *x, int *y) {
    *x = input_mx;
    *y = input_my;
}

void process_bk(void) {
    int input;

    GNW_do_bk_process();

    if (vcr_update() != mouse) {
        mouse_info();
    }

    input = win_check_all_buttons();
    if (input != -1) {
        GNW_add_input_buffer(input);
    } else {
        input = kb_getch();
        if (input != -1) {
            GNW_add_input_buffer(input);
        }
    }
}

void GNW_add_input_buffer(int input) {
    if (input != -1) {
        if (input == pause_key) {
            pause_game();
        } else if (input == screendump_key) {
            dump_screen();
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

int get_input_buffer(void) {
    int input;

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
    FuncPtr *prev;

    if (!game_paused && !bk_disabled) {
        prev = &bk_list;
        for (fp = bk_list; fp; fp = next) {
            next = fp->next;
            if (fp->flags & 1) {
                *prev = fp->next;
                free(fp);
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
                fp->flags &= 0xFFFFFFFEuL;
            }

            return;
        }
    }

    fp = (FuncPtr)malloc(sizeof(struct funcdata));
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

void enable_bk(void) { bk_disabled = 0; }

void disable_bk(void) { bk_disabled = 1; }

void pause_game(void) {
    WinID id;

    if (!game_paused) {
        game_paused = 1;

        id = pause_win_func();
        while (get_input() != GNW_KB_KEY_CTRL_ESCAPE) {
            ;
        }

        game_paused = 0;

        win_delete(id);
    }
}

WinID default_pause_window(void) {
    unsigned char *buf;
    WinID result;
    WinID id;
    int width;
    int length;

    width = text_width("Paused") + 32;
    length = 3 * text_height() + 16;

    id = win_add((scr_size.lrx - scr_size.ulx + 1 - width) / 2, (scr_size.lry - scr_size.uly + 1 - length) / 2, width,
                 length, 256, 20);

    if (id == -1) {
        result = -1;
    } else {
        win_border(id);

        buf = win_get_buf(id);

        text_to_buf(&buf[8 * width + 16], "Paused", width, width, colorTable[31744]);

        win_register_text_button(id, (width - text_width("Done") - 16) / 2, length - 8 - text_height() - 6, -1, -1, -1,
                                 27, "Done", 0);

        win_draw(id);

        result = id;
    }

    return result;
}

void register_pause(int new_pause_key, PauseWinFunc new_pause_win_func) {
    pause_key = new_pause_key;

    if (new_pause_win_func) {
        pause_win_func = new_pause_win_func;
    } else {
        pause_win_func = default_pause_window;
    }
}

void dump_screen(void) {
    ScreenBlitFunc old_scr_blit;
    ScreenBlitFunc old_mouse_blit;

    unsigned char *pal;
    int width;
    int length;

    width = scr_size.lrx - scr_size.ulx + 1;
    length = scr_size.lry - scr_size.uly + 1;

    screendump_buf = (char *)malloc(length * width);

    if (screendump_buf) {
        old_scr_blit = scr_blit;
        old_mouse_blit = mouse_blit;
        scr_blit = buf_blit;
        mouse_blit = buf_blit;

        win_refresh_all(&scr_size);

        mouse_blit = old_mouse_blit;
        scr_blit = old_scr_blit;

        pal = getSystemPalette();

        screendump_func(width, length, screendump_buf, pal);

        free(screendump_buf);
        screendump_buf = NULL;
    }
}

void buf_blit(unsigned char *buf, unsigned int bufw, unsigned int bufh, unsigned int sx, unsigned int sy,
              unsigned int w, unsigned int h, unsigned int dstx, unsigned int dsty) {
    buf_to_buf(&buf[sx] + bufw * sy, w, h, bufw, &screendump_buf[dstx] + (scr_size.lrx - scr_size.ulx + 1) * dsty,
               scr_size.lrx - scr_size.ulx + 1);
}

int default_screendump(int width, int length, unsigned char *buf, unsigned char *pal) {
    unsigned char blue;
    unsigned char reserved;
    unsigned char red;
    unsigned char green;

    int result;

    int i;
    FILE *fp;
    char fname[13];

    int temp_signed_32;
    unsigned int temp_unsigned_32;
    unsigned short temp_unsigned_16;

    for (i = 0; i < 100000; ++i) {
        sprintf(fname, "scr%.5d.bmp", i);
        fp = fopen(fname, "rb");
        if (!fp) break;
        fclose(fp);
    }

    if (i != 100000 && (fp = fopen(fname, "wb")) != 0) {
        temp_unsigned_16 = 'B' + ('M' << 8);
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_32 = length * width + 1078;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_unsigned_16 = 0;
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_16 = 0;
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_32 = 1078;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_unsigned_32 = 40;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_signed_32 = width;
        fwrite(&temp_signed_32, sizeof(temp_signed_32), 1, fp);

        temp_signed_32 = length;
        fwrite(&temp_signed_32, sizeof(temp_signed_32), 1, fp);

        temp_unsigned_16 = 1;
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_16 = 8;
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_32 = 0;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_unsigned_32 = 0;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_signed_32 = 0;
        fwrite(&temp_signed_32, sizeof(temp_signed_32), 1, fp);

        temp_signed_32 = 0;
        fwrite(&temp_signed_32, sizeof(temp_signed_32), 1, fp);

        temp_unsigned_32 = 0;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_unsigned_32 = 0;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        for (i = 0; i < 256; ++i) {
            reserved = 0;
            red = 4 * pal[0];
            green = 4 * pal[1];
            blue = 4 * pal[2];
            pal += 3;

            fwrite(&blue, 1, 1, fp);
            fwrite(&green, 1, 1, fp);
            fwrite(&red, 1, 1, fp);
            fwrite(&reserved, 1, 1, fp);
        }

        for (i = length - 1; i >= 0; i--) {
            fwrite(&buf[width * i], sizeof(unsigned char), width, fp);
        }

        fflush(fp);
        fclose(fp);

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

void register_screendump(int new_screendump_key, ScreenDumpFunc new_screendump_func) {
    screendump_key = new_screendump_key;

    if (new_screendump_func) {
        screendump_func = new_screendump_func;
    } else {
        screendump_func = default_screendump;
    }
}

TOCKS get_time(void) { return TIMER_MS_TO_TICKS(SDL_GetTicks()); }

void pause_for_tocks(unsigned int tocks) {
    TOCKS past_time;

    past_time = get_time();
    while (elapsed_time(past_time) < tocks) {
        process_bk();
    }
}

void block_for_tocks(unsigned int tocks) {
    TOCKS past_time;

    past_time = get_time();
    while (elapsed_time(past_time) < tocks) {
        ;
    }
}

TOCKS elapsed_time(TOCKS past_time) {
    TOCKS t;

    t = get_time();
    if (past_time > t) {
        t = 0x7FFFFFFF;
    } else {
        t = t - past_time;
    }

    return t;
}

TOCKS elapsed_tocks(TOCKS end_time, TOCKS start_time) {
    TOCKS t;

    if (start_time > end_time) {
        t = 0x7FFFFFFF;
    } else {
        t = end_time - start_time;
    }

    return t;
}
