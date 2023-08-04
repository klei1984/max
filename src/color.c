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

#include "color.h"

#include "gnw.h"

#define RGB555_COLOR_COUNT (1 << 15)

Color intensityColorTable[256][PALETTE_SIZE];

static int8_t Color_Inited;
static ColorIndex Color_RgbIndexTable[RGB555_COLOR_COUNT];
static uint8_t Color_ColorPalette[PALETTE_STRIDE * PALETTE_SIZE];
static uint8_t Color_SystemPalette[PALETTE_STRIDE * PALETTE_SIZE];

Color Color_RGB2Color(ColorRGB c) { return Color_RgbIndexTable[c]; }

void Color_FadeSystemPalette(uint8_t* src, uint8_t* dest, int32_t steps) {
    uint8_t temp[PALETTE_STRIDE * PALETTE_SIZE];

    for (int32_t i = 0; i < steps; ++i) {
        for (int32_t j = 0; j < sizeof(temp); ++j) {
            int32_t d = src[j] - dest[j];
            temp[j] = src[j] - (uint32_t)(i * d / steps);
        }

        Color_SetSystemPalette(temp);
    }

    Color_SetSystemPalette(dest);
}

void Color_SetSystemPalette(uint8_t* palette) {
    memmove(Color_SystemPalette, palette, PALETTE_STRIDE * PALETTE_SIZE);

    for (int32_t i = 0; i < PALETTE_SIZE; ++i) {
        SDL_Color color;

        color.r = palette[PALETTE_STRIDE * i + 0] * 4;
        color.g = palette[PALETTE_STRIDE * i + 1] * 4;
        color.b = palette[PALETTE_STRIDE * i + 2] * 4;
        color.a = 0;

        Svga_SetPaletteColor(i, &color);
    }
}

uint8_t* Color_GetSystemPalette(void) { return Color_SystemPalette; }

void Color_SetSystemPaletteEntry(int32_t entry, uint8_t r, uint8_t g, uint8_t b) {
    Color_SystemPalette[entry * PALETTE_STRIDE] = r;
    Color_SystemPalette[entry * PALETTE_STRIDE + 1] = g;
    Color_SystemPalette[entry * PALETTE_STRIDE + 2] = b;

    SDL_Color color;

    color.r = r * 4;
    color.g = g * 4;
    color.b = b * 4;
    color.a = 0;

    Svga_SetPaletteColor(entry, &color);
}

uint8_t* Color_GetColorPalette(void) { return Color_ColorPalette; }

void Color_SetColorPalette(uint8_t* palette) { memmove(Color_ColorPalette, palette, sizeof(Color_ColorPalette)); }

int32_t Color_Init(void) {
    int32_t result;

    if (Color_Inited) {
        result = 1;

    } else {
        FILE* in = fopen("COLOR.PAL", "rb");

        if (in == NULL) {
            result = 0;

        } else {
            fread(Color_ColorPalette, sizeof(Color_ColorPalette), 1, in);
            fread(Color_RgbIndexTable, sizeof(Color_RgbIndexTable), 1, in);

            fclose(in);

            Color_SetSystemPalette(Color_ColorPalette);

            Color_Inited = 1;
            result = 1;
        }
    }

    return result;
}

void Color_Deinit(void) { Color_Inited = 0; }

int32_t Color_GetColorDistance(Color* color1, Color* color2) {
    const int32_t diff_r = color1[0] - color2[0];
    const int32_t diff_g = color1[1] - color2[1];
    const int32_t diff_b = color1[2] - color2[2];

    return diff_r * diff_r + diff_g * diff_g + diff_b * diff_b;
}

void Color_ChangeColorTemperature(int32_t factor_r, int32_t factor_g, int32_t factor_b, int32_t multiplier_factor1,
                                  int32_t multiplier_factor2, uint8_t* color_map) {
    int32_t multiplier_factor_sum;
    Color* palette;
    int32_t index;
    Color color[3];
    int32_t distance1;
    int32_t distance2;

    palette = Color_GetSystemPalette();

    multiplier_factor_sum = multiplier_factor1 + multiplier_factor2;

    for (int32_t i = 0; i < PALETTE_SIZE; ++i) {
        index = i;

        color[0] = (palette[i * PALETTE_STRIDE + 0] * multiplier_factor2 + factor_r * multiplier_factor1) /
                   multiplier_factor_sum;
        color[1] = (palette[i * PALETTE_STRIDE + 1] * multiplier_factor2 + factor_g * multiplier_factor1) /
                   multiplier_factor_sum;
        color[2] = (palette[i * PALETTE_STRIDE + 2] * multiplier_factor2 + factor_b * multiplier_factor1) /
                   multiplier_factor_sum;

        distance1 = Color_GetColorDistance(&palette[index * PALETTE_STRIDE], color);

        for (int32_t j = 0; j < PALETTE_SIZE && distance1 > 0; ++j) {
            distance2 = Color_GetColorDistance(&palette[j * PALETTE_STRIDE], color);

            if (distance2 < distance1) {
                distance1 = distance2;
                index = j;
            }
        }

        color_map[i] = index;
    }
}

void Color_RecolorPixels(uint8_t* buffer, int32_t width, int32_t width_text, int32_t height_text, uint8_t* color_map) {
    for (int32_t i = 0; i < height_text; ++i) {
        for (int32_t j = 0; j < width_text; ++j) {
            buffer[i * width + j] = color_map[buffer[i * width + j]];
        }
    }
}
