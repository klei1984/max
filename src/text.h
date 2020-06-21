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

#ifndef TEXT_H
#define TEXT_H

typedef struct FontMgr_s* FontMgrPtr;

typedef void (*text_to_buf_func)(unsigned char*, char*, int, int, int);
typedef int (*text_height_func)(void);
typedef int (*text_width_func)(char*);
typedef int (*text_char_width_func)(char);
typedef int (*text_mono_width_func)(char*);
typedef int (*text_spacing_func)(void);
typedef int (*text_size_func)(char*);
typedef int (*text_max_func)(void);

extern text_to_buf_func text_to_buf;
extern text_height_func text_height;
extern text_width_func text_width;
extern text_char_width_func text_char_width;
extern text_mono_width_func text_mono_width;
extern text_spacing_func text_spacing;
extern text_size_func text_size;
extern text_max_func text_max;

int GNW_text_init(void);
void GNW_text_exit(void);
int text_add_manager(FontMgrPtr mgr);
int text_remove_manager(int font_num);
int text_curr(void);
void text_font(int font_num);

#endif /* TEXT_H */