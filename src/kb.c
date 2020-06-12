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

#include "gnw.h"

#define GNW_KB_KEY_MAP_ITEM(key, keys, shift, left_alt, right_alt, ctrl) \
    ascii_table[key] = (key_ansi_t) { keys, shift, left_alt, right_alt, ctrl }

typedef struct key_ansi_s {
    short key;
    short shift;
    short left_alt;
    short right_alt;
    short ctrl;
} key_ansi_t;

typedef struct key_data_s {
    char scan_code;
    unsigned short modifiers;
} key_data_t;

typedef int (*AsciiConvert)(void);

static int kb_next_ascii_English_US(void);
static int kb_next_ascii_French(void);
static int kb_next_ascii_German(void);
static int kb_next_ascii_Italian(void);
static int kb_next_ascii_Spanish(void);
static int kb_next_ascii(void);
static void kb_map_ascii_English_US(void);
static void kb_map_ascii_French(void);
static void kb_map_ascii_German(void);
static void kb_map_ascii_Italian(void);
static void kb_map_ascii_Spanish(void);
static void kb_init_lock_status(void);
static void kb_toggle_caps(void);
static void kb_toggle_num(void);
static void kb_toggle_scroll(void);
static void kb_set_led_status(void);
static int kb_buffer_put(key_data_t* key_data);
static int kb_buffer_get(key_data_t* key_data);
static int kb_buffer_peek(int count, key_data_t** key_data);

static unsigned char kb_installed;
static int kb_disabled;
static int kb_put;
static int kb_get;
static unsigned short extended_code;
static unsigned char kb_lock_flags;
static AsciiConvert kb_scan_to_ascii = kb_next_ascii_English_US;

static key_data_t kb_buffer[64];
static key_ansi_t ascii_table[256];
static kb_layout_t kb_layout = english;
static unsigned char keynumpress;

char keys[256];

void GNW_kb_set(void) {
    if (!kb_installed) {
        kb_installed = 1;

        kb_clear();
        kb_init_lock_status();
        kb_set_layout(english);
    }
}

void GNW_kb_restore(void) { kb_installed = 0; };

void kb_wait(void) {
    if (kb_installed) {
        kb_clear();
        while (!keynumpress) {
            GNW_process_message();
        }
        kb_clear();
    }
}

void kb_clear(void) {
    if (kb_installed) {
        keynumpress = 0;
        memset(keys, 0, sizeof(keys));
        kb_put = 0;
        kb_get = 0;
    }
};

int kb_getch(void) {
    int result;

    if (kb_installed) {
        result = kb_scan_to_ascii();
    } else {
        result = -1;
    }

    return result;
};

void kb_disable(void) { kb_disabled = 1; }

void kb_enable(void) { kb_disabled = 0; }

int kb_is_disabled(void) { return kb_disabled; }

void kb_set_layout(kb_layout_t layout) {
    kb_layout_t old_layout;

    old_layout = kb_layout;
    kb_layout = layout;

    if (layout >= unsupported_language) {
        kb_layout = old_layout;
    } else {
        switch (layout) {
            case english:
                kb_scan_to_ascii = kb_next_ascii_English_US;
                kb_map_ascii_English_US();
                break;
            case french:
                kb_scan_to_ascii = kb_next_ascii_French;
                kb_map_ascii_French();
                break;
            case german:
                kb_scan_to_ascii = kb_next_ascii_German;
                kb_map_ascii_German();
                break;
            case italian:
                kb_scan_to_ascii = kb_next_ascii_Italian;
                kb_map_ascii_Italian();
                break;
            case spanish:
                kb_scan_to_ascii = kb_next_ascii_Spanish;
                kb_map_ascii_Spanish();
                break;
        }
    }
};

kb_layout_t kb_get_layout(void) { return kb_layout; }

int kb_ascii_to_scan(int ascii) {
    for (int k = 0; k < 256; ++k) {
        if (ascii_table[k].key == ascii || ascii_table[k].shift == ascii || ascii_table[k].left_alt == ascii ||
            ascii_table[k].right_alt == ascii || ascii_table[k].ctrl == ascii) {
            return k;
        }
    }

    return -1;
}

void kb_simulate_key(unsigned short scan_code) {
    static key_data_t temp;
    unsigned short press_code;

    if ((vcr_state == 0) && (vcr_buffer_index != 4095)) {
        vcr_buffer[vcr_buffer_index].type = key;
        vcr_buffer[vcr_buffer_index].time = vcr_time;
        vcr_buffer[vcr_buffer_index].counter = vcr_counter;
        vcr_buffer[vcr_buffer_index].data.key_data.scan_code = scan_code;

        vcr_buffer_index++;
    }

    if (scan_code == 0xE0) {
        extended_code = 0x80;
    } else {
        if (scan_code & 0x80) {
            press_code = 0;
            scan_code &= 0x7Fu;
        } else {
            press_code = 1;
        }

        scan_code += extended_code;

        if (press_code && keys[scan_code]) {
            press_code = 2;
        }

        if (keys[scan_code] != press_code) {
            keys[scan_code] = press_code;

            if (press_code == 1) {
                ++keynumpress;
            } else if (!press_code) {
                --keynumpress;
            }
        }

        if (press_code) {
            temp.scan_code = scan_code;
            temp.modifiers = 0;

            switch (scan_code) {
                case GNW_KB_SCAN_CAPSLOCK:
                    if (!keys[GNW_KB_SCAN_LCTRL] && !keys[GNW_KB_SCAN_RCTRL]) {
                        if (kb_lock_flags & GNW_KB_LOCK_CAPS) {
                            if (kb_layout == english) {
                                kb_toggle_caps();
                            }
                        } else {
                            kb_toggle_caps();
                        }
                    }
                    break;
                case GNW_KB_SCAN_NUMLOCK:
                    if (!keys[GNW_KB_SCAN_LCTRL] && !keys[GNW_KB_SCAN_RCTRL]) {
                        kb_toggle_num();
                    }
                    break;
                case GNW_KB_SCAN_SCROLLLOCK:
                    if (!keys[GNW_KB_SCAN_LCTRL] && !keys[GNW_KB_SCAN_RCTRL]) {
                        kb_toggle_scroll();
                    }
                    break;
                default:
                    if ((scan_code == GNW_KB_SCAN_LSHIFT || scan_code == GNW_KB_SCAN_RSHIFT) &&
                        (kb_lock_flags & GNW_KB_LOCK_CAPS) && kb_layout != english) {
                        kb_toggle_caps();
                    }
                    break;
            }

            if (kb_lock_flags) {
                if (kb_lock_flags & GNW_KB_LOCK_NUM) {
                    temp.modifiers |= GNW_KB_MOD_NUM;
                }

                if (kb_lock_flags & GNW_KB_LOCK_CAPS) {
                    temp.modifiers |= GNW_KB_MOD_CAPS;
                }

                if (kb_lock_flags & GNW_KB_LOCK_SCROLL) {
                    temp.modifiers |= GNW_KB_MOD_SCROLL;
                }
            }

            if (keys[GNW_KB_SCAN_LSHIFT]) {
                temp.modifiers |= GNW_KB_MOD_LSHIFT;
            }

            if (keys[GNW_KB_SCAN_RSHIFT]) {
                temp.modifiers |= GNW_KB_MOD_RSHIFT;
            }

            if (keys[GNW_KB_SCAN_LALT]) {
                temp.modifiers |= GNW_KB_MOD_LALT;
            }

            if (keys[GNW_KB_SCAN_RALT]) {
                temp.modifiers |= GNW_KB_MOD_RALT;
            }

            if (keys[GNW_KB_SCAN_LCTRL]) {
                temp.modifiers |= GNW_KB_MOD_LCTRL;
            }

            if (keys[GNW_KB_SCAN_RCTRL]) {
                temp.modifiers |= GNW_KB_MOD_RCTRL;
            }

            kb_buffer_put(&temp);
        }

        extended_code = 0;
    }

    if (keys[0xC6]) {
        kb_clear();
    }
}

int kb_next_ascii_English_US(void) {
    signed int result;
    key_data_t* this_key;

    if (kb_buffer_peek(0, &this_key)) {
        result = -1;
    } else {
        if (this_key->modifiers & GNW_KB_MOD_CAPS) {
            switch (this_key->scan_code) {
                case GNW_KB_SCAN_Q:
                case GNW_KB_SCAN_W:
                case GNW_KB_SCAN_E:
                case GNW_KB_SCAN_R:
                case GNW_KB_SCAN_T:
                case GNW_KB_SCAN_Y:
                case GNW_KB_SCAN_U:
                case GNW_KB_SCAN_I:
                case GNW_KB_SCAN_O:
                case GNW_KB_SCAN_P:
                case GNW_KB_SCAN_A:
                case GNW_KB_SCAN_S:
                case GNW_KB_SCAN_D:
                case GNW_KB_SCAN_F:
                case GNW_KB_SCAN_G:
                case GNW_KB_SCAN_H:
                case GNW_KB_SCAN_J:
                case GNW_KB_SCAN_K:
                case GNW_KB_SCAN_L:
                case GNW_KB_SCAN_Z:
                case GNW_KB_SCAN_X:
                case GNW_KB_SCAN_C:
                case GNW_KB_SCAN_V:
                case GNW_KB_SCAN_B:
                case GNW_KB_SCAN_N:
                case GNW_KB_SCAN_M:
                    if (this_key->modifiers & GNW_KB_MOD_SHIFT) {
                        this_key->modifiers &= ~GNW_KB_MOD_SHIFT;
                    } else {
                        this_key->modifiers |= GNW_KB_MOD_LSHIFT;
                    }
                    break;
                default:
                    break;
            }
        }

        result = kb_next_ascii();
    }

    return result;
}

int kb_next_ascii_French(void) { return -1; }

int kb_next_ascii_German(void) { return -1; }

int kb_next_ascii_Italian(void) { return -1; }

int kb_next_ascii_Spanish(void) { return -1; }

int kb_next_ascii(void) {
    key_data_t* this_key;
    int ascii;

    ascii = -1;
    if (kb_buffer_peek(0, &this_key)) {
        return -1;
    }

    switch (this_key->scan_code) {
        case GNW_KB_SCAN_INSERT:
        case GNW_KB_SCAN_END:
        case GNW_KB_SCAN_DOWN:
        case GNW_KB_SCAN_PAGEDOWN:
        case GNW_KB_SCAN_LEFT:
        case GNW_KB_SCAN_KP_5:
        case GNW_KB_SCAN_RIGHT:
        case GNW_KB_SCAN_HOME:
        case GNW_KB_SCAN_UP:
        case GNW_KB_SCAN_PAGEUP:
        case GNW_KB_SCAN_DELETE:
            if (!(this_key->modifiers & GNW_KB_MOD_ALT) && (this_key->modifiers & GNW_KB_MOD_NUM)) {
                if (this_key->modifiers & GNW_KB_MOD_SHIFT) {
                    this_key->modifiers &= ~GNW_KB_MOD_SHIFT;
                } else {
                    this_key->modifiers |= GNW_KB_MOD_LSHIFT;
                }
            }
            break;
        default:
            break;
    }

    if (this_key->modifiers & GNW_KB_MOD_CTRL) {
        ascii = ascii_table[this_key->scan_code].ctrl;
    } else if (this_key->modifiers & GNW_KB_MOD_LALT) {
        ascii = ascii_table[this_key->scan_code].left_alt;
    } else if (this_key->modifiers & GNW_KB_MOD_RALT) {
        ascii = ascii_table[this_key->scan_code].right_alt;
    } else if (this_key->modifiers & GNW_KB_MOD_SHIFT) {
        ascii = ascii_table[this_key->scan_code].shift;
    } else {
        ascii = ascii_table[this_key->scan_code].key;
    }

    kb_buffer_get(NULL);

    return ascii;
}

void kb_map_ascii_English_US(void) {
    memset(ascii_table, -1, 256 * sizeof(key_ansi_t));

    GNW_KB_KEY_MAP_ITEM(1, 27, 27, 27, 27, 27);
    GNW_KB_KEY_MAP_ITEM(2, 49, 33, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(3, 50, 64, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(4, 51, 35, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(5, 52, 36, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(6, 53, 37, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(7, 54, 94, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(8, 55, 38, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(9, 56, 42, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(10, 57, 40, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(11, 48, 41, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(12, 45, 95, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(13, 61, 43, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(14, 8, 8, 8, 8, 127);
    GNW_KB_KEY_MAP_ITEM(15, 9, 9, 9, 9, 9);
    GNW_KB_KEY_MAP_ITEM(16, 113, 81, 272, 272, 17);
    GNW_KB_KEY_MAP_ITEM(17, 119, 87, 273, 273, 23);
    GNW_KB_KEY_MAP_ITEM(18, 101, 69, 274, 274, 5);
    GNW_KB_KEY_MAP_ITEM(19, 114, 82, 275, 275, 18);
    GNW_KB_KEY_MAP_ITEM(20, 116, 84, 276, 276, 20);
    GNW_KB_KEY_MAP_ITEM(21, 121, 89, 277, 277, 25);
    GNW_KB_KEY_MAP_ITEM(22, 117, 85, 278, 278, 21);
    GNW_KB_KEY_MAP_ITEM(23, 105, 73, 279, 279, 9);
    GNW_KB_KEY_MAP_ITEM(24, 111, 79, 280, 280, 15);
    GNW_KB_KEY_MAP_ITEM(25, 112, 80, 281, 281, 16);
    GNW_KB_KEY_MAP_ITEM(26, 91, 123, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(27, 93, 125, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(28, 13, 13, 13, 13, 10);
    GNW_KB_KEY_MAP_ITEM(29, -1, -1, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(30, 97, 65, 286, 286, 1);
    GNW_KB_KEY_MAP_ITEM(31, 115, 83, 287, 287, 19);
    GNW_KB_KEY_MAP_ITEM(32, 100, 68, 288, 288, 4);
    GNW_KB_KEY_MAP_ITEM(33, 102, 70, 289, 289, 6);
    GNW_KB_KEY_MAP_ITEM(34, 103, 71, 290, 290, 7);
    GNW_KB_KEY_MAP_ITEM(35, 104, 72, 291, 291, 8);
    GNW_KB_KEY_MAP_ITEM(36, 106, 74, 292, 292, 10);
    GNW_KB_KEY_MAP_ITEM(37, 107, 75, 293, 293, 11);
    GNW_KB_KEY_MAP_ITEM(38, 108, 76, 294, 294, 12);
    GNW_KB_KEY_MAP_ITEM(39, 59, 58, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(40, 39, 34, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(41, 96, 126, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(42, -1, -1, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(43, 92, 124, -1, -1, 192);
    GNW_KB_KEY_MAP_ITEM(44, 122, 90, 300, 300, 26);
    GNW_KB_KEY_MAP_ITEM(45, 120, 88, 301, 301, 24);
    GNW_KB_KEY_MAP_ITEM(46, 99, 67, 302, 302, 3);
    GNW_KB_KEY_MAP_ITEM(47, 118, 86, 303, 303, 22);
    GNW_KB_KEY_MAP_ITEM(48, 98, 66, 304, 304, 2);
    GNW_KB_KEY_MAP_ITEM(49, 110, 78, 305, 305, 14);
    GNW_KB_KEY_MAP_ITEM(50, 109, 77, 306, 306, 13);
    GNW_KB_KEY_MAP_ITEM(51, 44, 60, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(52, 46, 62, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(53, 47, 63, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(54, -1, -1, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(55, 42, 42, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(56, -1, -1, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(57, 32, 32, 32, 32, 32);
    GNW_KB_KEY_MAP_ITEM(58, -1, -1, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(59, 315, 340, 360, 360, 350);
    GNW_KB_KEY_MAP_ITEM(60, 316, 341, 361, 361, 351);
    GNW_KB_KEY_MAP_ITEM(61, 317, 342, 362, 362, 352);
    GNW_KB_KEY_MAP_ITEM(62, 318, 343, 362, 363, 353);
    GNW_KB_KEY_MAP_ITEM(63, 319, 344, 364, 364, 354);
    GNW_KB_KEY_MAP_ITEM(64, 320, 345, 365, 365, 355);
    GNW_KB_KEY_MAP_ITEM(65, 321, 346, 366, 366, 356);
    GNW_KB_KEY_MAP_ITEM(66, 322, 347, 367, 367, 357);
    GNW_KB_KEY_MAP_ITEM(67, 323, 348, 368, 368, 358);
    GNW_KB_KEY_MAP_ITEM(68, 324, 349, 369, 369, 359);
    GNW_KB_KEY_MAP_ITEM(69, -1, -1, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(71, 327, 55, 407, 407, 375);
    GNW_KB_KEY_MAP_ITEM(72, 328, 56, 408, 408, 397);
    GNW_KB_KEY_MAP_ITEM(73, 329, 57, 409, 409, 388);
    GNW_KB_KEY_MAP_ITEM(74, 45, 45, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(75, 331, 52, 411, 411, 371);
    GNW_KB_KEY_MAP_ITEM(76, 332, 53, 9999, 9999, 399);
    GNW_KB_KEY_MAP_ITEM(77, 333, 54, 413, 413, 372);
    GNW_KB_KEY_MAP_ITEM(78, 43, 43, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(79, 335, 49, 415, 415, 373);
    GNW_KB_KEY_MAP_ITEM(80, 336, 50, 416, 416, 401);
    GNW_KB_KEY_MAP_ITEM(81, 337, 51, 417, 417, 374);
    GNW_KB_KEY_MAP_ITEM(82, 338, 48, 418, 418, 402);
    GNW_KB_KEY_MAP_ITEM(83, 339, 46, -1, 419, 403);
    GNW_KB_KEY_MAP_ITEM(87, 389, 391, 395, 395, 393);
    GNW_KB_KEY_MAP_ITEM(88, 390, 392, 396, 396, 394);
    GNW_KB_KEY_MAP_ITEM(156, 13, 13, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(157, -1, -1, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(181, 47, 47, -1, -1, 3);
    GNW_KB_KEY_MAP_ITEM(184, -1, -1, -1, -1, -1);
    GNW_KB_KEY_MAP_ITEM(199, 327, 327, 407, 407, 375);
    GNW_KB_KEY_MAP_ITEM(200, 328, 328, 408, 408, 397);
    GNW_KB_KEY_MAP_ITEM(201, 329, 329, 409, 409, 388);
    GNW_KB_KEY_MAP_ITEM(203, 331, 331, 411, 411, 371);
    GNW_KB_KEY_MAP_ITEM(205, 333, 333, 413, 413, 372);
    GNW_KB_KEY_MAP_ITEM(207, 335, 335, 415, 415, 373);
    GNW_KB_KEY_MAP_ITEM(208, 336, 336, 416, 416, 401);
    GNW_KB_KEY_MAP_ITEM(209, 337, 337, 417, 417, 374);
    GNW_KB_KEY_MAP_ITEM(210, 338, 338, 418, 418, 402);
    GNW_KB_KEY_MAP_ITEM(211, 339, 339, 419, 419, 403);
}

void kb_map_ascii_French(void) {}

void kb_map_ascii_German(void) {}

void kb_map_ascii_Italian(void) {}

void kb_map_ascii_Spanish(void) {}

void kb_init_lock_status(void) {
    SDL_Keymod modifiers;

    modifiers = SDL_GetModState();

    kb_lock_flags = GNW_KB_LOCK_NONE;

    kb_lock_flags |= (modifiers & KMOD_NUM) ? GNW_KB_LOCK_NUM : GNW_KB_LOCK_NONE;
    kb_lock_flags |= (modifiers & KMOD_CAPS) ? GNW_KB_LOCK_CAPS : GNW_KB_LOCK_NONE;
    kb_lock_flags |= (modifiers & KMOD_NONE) ? GNW_KB_LOCK_SCROLL : GNW_KB_LOCK_NONE;
}

void kb_toggle_caps(void) {
    if (kb_lock_flags & GNW_KB_LOCK_CAPS) {
        kb_lock_flags &= ~GNW_KB_LOCK_CAPS;
    } else {
        kb_lock_flags |= GNW_KB_LOCK_CAPS;
    }

    kb_set_led_status();
}

void kb_toggle_num(void) {
    if (kb_lock_flags & GNW_KB_LOCK_NUM) {
        kb_lock_flags &= ~GNW_KB_LOCK_NUM;
    } else {
        kb_lock_flags |= GNW_KB_LOCK_NUM;
    }

    kb_set_led_status();
}

void kb_toggle_scroll(void) {
    if (kb_lock_flags & GNW_KB_LOCK_SCROLL) {
        kb_lock_flags &= ~GNW_KB_LOCK_SCROLL;
    } else {
        kb_lock_flags |= GNW_KB_LOCK_SCROLL;
    }

    kb_set_led_status();
}

void kb_set_led_status(void) {}

int kb_buffer_put(key_data_t* key_data) {
    int result;

    if (((kb_put + 1) & 0x3F) != kb_get) {
        kb_buffer[kb_put] = *key_data;
        kb_put++;
        kb_put &= 0x3F;

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int kb_buffer_get(key_data_t* key_data) {
    int result;

    if (kb_get != kb_put) {
        if (key_data) {
            *key_data = kb_buffer[kb_get];
        }
        kb_get++;
        kb_get &= 0x3Fu;

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

int kb_buffer_peek(int count, key_data_t** key_data) {
    int dist;
    int result;

    result = -1;

    if (kb_get != kb_put) {
        if (kb_put <= kb_get) {
            dist = 64 - kb_get - 1 + kb_put;
        } else {
            dist = kb_put - kb_get - 1;
        }

        if (count <= dist) {
            *key_data = &kb_buffer[(count + kb_get) & 0x3F];
            result = 0;
        }
    }

    return result;
}
