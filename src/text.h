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

#ifdef __cplusplus
extern "C" {
#endif

enum {
    GNW_TEXT_FONT_0,
    GNW_TEXT_FONT_1,
    GNW_TEXT_FONT_2,
    GNW_TEXT_FONT_3,
    GNW_TEXT_FONT_4,
    GNW_TEXT_FONT_5,
    GNW_TEXT_FONT_6,
    GNW_TEXT_FONT_7,
    GNW_TEXT_FONT_8,
    GNW_TEXT_FONT_9,
    GNW_TEXT_FONT_COUNT
};

#define GNW_TEXT_UNKNOWN_0 0x100
#define GNW_TEXT_UNKNOWN_1 0x10000
#define GNW_TEXT_UNDERLINE 0x20000
#define GNW_TEXT_MONOSPACE 0x40000
#define GNW_TEXT_REFRESH_WINDOW 0x1000000
#define GNW_TEXT_UNKNOWN_2 0x2000000
#define GNW_TEXT_UNKNOWN_3 0x4000000
#define GNW_TEXT_COLOR_MASK 0xFF

typedef struct FontMgr* FontMgrPtr;

typedef void (*text_to_buf_func)(unsigned char*, const char*, int, int, int);
typedef int (*text_height_func)(void);
typedef int (*text_width_func)(const char*);
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

#ifdef __cplusplus
}
#endif

#endif /* TEXT_H */
