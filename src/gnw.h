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

#ifndef GNW_H
#define GNW_H

#include <assert.h>

#include "memory.h"
#include "rect.h"
#include "mouse.h"
#include "debug.h"
#include "text.h"
#include "input.h"
#include "db.h"
#include "interface.h"
#include "button.h"
#include "kb.h"
#include "vcr.h"
#include "color.h"

struct GNW_Window_s {
    WinID id;
    unsigned int flags;
    Rect w;
    int width;
    int length;
    int color;
    int tx;
    int ty;
    unsigned char *buf;
    GNW_ButtonPtr button_list;
    GNW_ButtonPtr last_over;
    GNW_ButtonPtr last_click;
    GNW_Menu *menu;
    trans_b2b trans_b2b_ptr;
};

// void win_refresh_all(Rect *bound);
// void win_get_mouse_buf(char *buf);

#endif /* GNW_H */
