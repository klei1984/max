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

#ifndef FONTS_HPP
#define FONTS_HPP

#include <cstdint>

struct FontColor {
    uint8_t base;
    uint8_t outline;
    uint8_t shadow;

    FontColor(uint8_t base, uint8_t outline, uint8_t shadow) : base(base), outline(outline), shadow(shadow) {}
    FontColor(const FontColor& other) : base(other.base), outline(other.outline), shadow(other.shadow) {}
    FontColor& operator=(const FontColor& other) {
        base = other.base;
        outline = other.outline;
        shadow = other.shadow;

        return *this;
    }
};

extern FontColor Fonts_GoldColor;
extern FontColor Fonts_DarkOrageColor;
extern FontColor Fonts_DarkGrayColor;
extern FontColor Fonts_BrightBrownColor;
extern FontColor Fonts_BrightYellowColor;
extern FontColor Fonts_BrightSilverColor;

#endif /* FONTS_HPP */
