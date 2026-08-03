/* Copyright (c) 2021 M.A.X. Port Team
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

#ifndef TEXT_HPP
#define TEXT_HPP

#include "fonts.hpp"
#include "gnw.h"
#include "smartstring.hpp"

extern uint32_t Text_TypeWriter_CharacterTimeMs;
extern uint32_t Text_TypeWriter_BeepTimeMs;

SmartString* Text_SplitText(const char* text, int32_t max_row_count, int32_t width, int32_t* row_count);

void Text_TextBox(uint8_t* buffer, uint16_t length, const char* text, int32_t ulx, int32_t uly, int32_t width,
                  int32_t height, int32_t color, bool horizontal_align = false, bool vertical_align = true);

void Text_TextBox(WindowInfo* window, const char* text, int32_t ulx, int32_t uly, int32_t width, int32_t height,
                  bool horizontal_align = false, bool vertical_align = true, FontColor color = Fonts_GoldColor);

void Text_TextLine(WindowInfo* window, const char* str, int32_t ulx, int32_t uly, int32_t swidth,
                   bool horizontal_align = false, FontColor color = Fonts_GoldColor);

void Text_TypeWriter_TextBox(WindowInfo* window, const char* text, int32_t ulx, int32_t uly, int32_t width,
                             int32_t alignment);

void Text_TypeWriter_TextBoxMultiLineWrapText(WindowInfo* window, const char* text, int32_t ulx, int32_t uly,
                                              int32_t width, int32_t height, int32_t alignment);

void Text_AutofitTextBox(uint8_t* buffer, uint16_t pitch, const char* text, Rect* text_area, Rect* draw_area,
                         int32_t color, bool horizontal_align);

/**
 * \brief Appends UTF-8 text to a bounded buffer without splitting a code point.
 *
 * SDL offers SDL_utf8strlcpy but no matching append, and plain strcat has no idea how large the
 * destination is. This finds the end of the destination and hands the space that is left to
 * SDL_utf8strlcpy, so an append that does not fit truncates on a character boundary instead of
 * overrunning or leaving a partial sequence behind.
 *
 * \param dst Destination buffer, already holding a null terminated string.
 * \param src Null terminated UTF-8 text to append.
 * \param dst_bytes Total size of the destination buffer in bytes.
 * \return Length of the resulting string in bytes, not counting the terminator.
 */
size_t Text_AppendUtf8(char* dst, const char* src, size_t dst_bytes) noexcept;

#endif /* TEXT_HPP */
