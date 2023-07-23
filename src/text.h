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

#include <stdint.h>

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

#define GNW_TEXT_OUTLINE 0x10000
#define GNW_TEXT_UNDERLINE 0x20000
#define GNW_TEXT_MONOSPACE 0x40000
#define GNW_TEXT_REFRESH_WINDOW 0x1000000
#define GNW_TEXT_FILL_WINDOW 0x2000000
#define GNW_TEXT_ALLOW_TRUNCATED 0x4000000

#define GNW_TEXT_COLOR_MASK 0xFF
#define GNW_TEXT_RGB_MASK 0xFFFF
#define GNW_TEXT_CONTROL_MASK (~0xFFFF)

typedef struct FontMgr* FontMgrPtr;

typedef void (*text_to_buf_func)(uint8_t* buf, const char* str, int32_t swidth, int32_t fullw, int32_t color);
typedef int32_t (*text_height_func)(void);
typedef int32_t (*text_width_func)(const char* str);
typedef int32_t (*text_char_width_func)(const uint32_t glyph);
typedef int32_t (*text_mono_width_func)(const char* str);
typedef int32_t (*text_spacing_func)(void);
typedef int32_t (*text_size_func)(const char* str);

extern text_to_buf_func Text_Blit;
extern text_height_func Text_GetHeight;
extern text_width_func Text_GetWidth;
extern text_char_width_func Text_GetGlyphWidth;
extern text_mono_width_func Text_GetMonospaceWidth;
extern text_spacing_func Text_GetSpacing;
extern text_size_func Text_GetSize;

int32_t Text_Init(void);
void Text_Exit(void);
int32_t Text_AddManager(FontMgrPtr mgr);
int32_t Text_RemoveManager(int32_t font_num);
int32_t Text_GetFont(void);
void Text_SetFont(int32_t font_num);

#endif /* TEXT_H */
