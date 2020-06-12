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

#ifndef KB_H
#define KB_H

#include "kb_code.h"
#include "timer.h"

typedef enum kb_layout_e { english, french, german, italian, spanish, unsupported_language } kb_layout_t;

extern char keys[256];

void GNW_kb_set(void);
void GNW_kb_restore(void);
void kb_wait(void);
void kb_clear(void);
int kb_getch(void);
void kb_disable(void);
void kb_enable(void);
int kb_is_disabled(void);
void kb_set_layout(kb_layout_t layout);
kb_layout_t kb_get_layout(void);
int kb_ascii_to_scan(int ascii);
void kb_simulate_key(unsigned short scan_code);

#endif /* KB_H */
