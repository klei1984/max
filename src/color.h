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

#ifndef COLOR_H
#define COLOR_H

#include <SDL.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PALETTE_SIZE 256
#define PALETTE_STRIDE 3

#define COLOR_BLACK 0x00
#define COLOR_RED 0x01
#define COLOR_GREEN 0x02
#define COLOR_BLUE 0x03
#define COLOR_YELLOW 0x04
#define COLOR_CHROME_YELLOW 0x05
#define COLOR_ROMAN_SILVER 0x06
#define COLOR_RED_ORANGE 0x07
#define COLOR_PASTEL_YELLOW 0x08

typedef uint8_t ColorIndex;
typedef int32_t ColorRGB;
typedef uint8_t Color;

extern Color intensityColorTable[256][PALETTE_SIZE];

int32_t Color_Init(void);
void Color_Deinit(void);
Color Color_RGB2Color(ColorRGB c);
int32_t Color_GetColorDistance(Color* color1, Color* color2);
void Color_ChangeColorTemperature(int32_t factor_r, int32_t factor_g, int32_t factor_b, int32_t multiplier_factor1,
                                  int32_t multiplier_factor2, uint8_t* color_map);
void Color_RecolorPixels(uint8_t* buffer, int32_t width, int32_t width_text, int32_t height_text, uint8_t* color_map);
void Color_FadeSystemPalette(uint8_t* src, uint8_t* dest, int32_t steps);
uint8_t* Color_GetSystemPalette(void);
void Color_SetSystemPalette(uint8_t* palette);
void Color_SetSystemPaletteEntry(int32_t entry, uint8_t r, uint8_t g, uint8_t b);
uint8_t* Color_GetColorPalette(void);
void Color_SetColorPalette(uint8_t* palette);

#ifdef __cplusplus
}
#endif

#endif /* COLOR_H */
