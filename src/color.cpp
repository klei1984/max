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

#include "cursor.hpp"
#include "resource_manager.hpp"

static constexpr uint32_t RGB555_COLOR_COUNT{1 << 15};
static constexpr uint32_t PALETTE_FILE_TAG{0x4E455743} /* 'NEWC' */;

static int8_t Color_Inited;
static uint8_t Color_ColorPalette[PALETTE_STRIDE * PALETTE_SIZE];
static uint8_t Color_SystemPalette[PALETTE_STRIDE * PALETTE_SIZE];
static ColorIndex Color_RgbIndexTable[RGB555_COLOR_COUNT];
static Color Color_IntensityColorTable[256][PALETTE_SIZE];

Color Color_RGB2Color(ColorRGB c) { return Color_RgbIndexTable[c]; }
Color Color_ColorIntensity(int32_t intensity, Color color) { return Color_IntensityColorTable[color][intensity / 512]; }

void Color_FadeSystemPalette(uint8_t* src, uint8_t* dest, int32_t time_limit) {
    uint8_t palette[PALETTE_STRIDE * PALETTE_SIZE];

    for (int32_t time_spent{0}, measured_time{1}; time_spent < time_limit; time_spent += measured_time) {
        const uint64_t time_stamp = timer_get();

        for (uint32_t j = 0; j < sizeof(palette); ++j) {
            const int32_t d = src[j] - dest[j];

            palette[j] = src[j] - ((time_spent * d) / time_limit);
        }

        if (get_input() > 0 || (mouse_get_buttons() & (MOUSE_PRESS_LEFT | MOUSE_PRESS_RIGHT))) {
            break;
        }

        Color_SetSystemPalette(palette);

        measured_time = timer_elapsed_time(time_stamp);
        measured_time = std::min<int32_t>(measured_time, time_limit);
        measured_time = std::max<int32_t>(measured_time, 1);
    }

    Color_SetSystemPalette(dest);
}

void Color_SetSystemPalette(uint8_t* palette) {
    for (int32_t i = 0; i < PALETTE_SIZE; ++i) {
        Color_SetSystemPaletteEntry(i, palette[PALETTE_STRIDE * i + 0], palette[PALETTE_STRIDE * i + 1],
                                    palette[PALETTE_STRIDE * i + 2]);
    }

    Cursor_MarkDirty();
}

uint8_t* Color_GetSystemPalette(void) { return Color_SystemPalette; }

void Color_SetSystemPaletteEntry(int32_t entry, uint8_t r, uint8_t g, uint8_t b) {
    Color_SystemPalette[entry * PALETTE_STRIDE + 0] = r;
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
        auto fp{ResourceManager_OpenFileResource("COLOR.PAL", ResourceType_GameData)};

        if (!fp) {
            result = 0;

        } else {
            uint32_t tag;

            fread(Color_ColorPalette, sizeof(Color_ColorPalette), 1, fp);
            fread(Color_RgbIndexTable, sizeof(Color_RgbIndexTable), 1, fp);

            fread(&tag, sizeof(tag), 1, fp);

            if (tag == PALETTE_FILE_TAG) {
                fread(Color_IntensityColorTable, sizeof(Color_IntensityColorTable), 1, fp);
            }

            fclose(fp);

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

void Color_GenerateIntensityTable1(int32_t red_level, int32_t green_level, int32_t blue_level, int32_t factor1,
                                   int32_t factor2, uint8_t* table) {
    int32_t factor_sum;
    Color* palette;
    int32_t index;
    Color color[3];
    int32_t distance1;
    int32_t distance2;

    palette = Color_GetSystemPalette();

    factor_sum = factor1 + factor2;

    for (int32_t i = 0; i < PALETTE_SIZE; ++i) {
        index = i;

        color[0] = (palette[i * PALETTE_STRIDE + 0] * factor2 + red_level * factor1) / factor_sum;
        color[1] = (palette[i * PALETTE_STRIDE + 1] * factor2 + green_level * factor1) / factor_sum;
        color[2] = (palette[i * PALETTE_STRIDE + 2] * factor2 + blue_level * factor1) / factor_sum;

        distance1 = Color_GetColorDistance(&palette[index * PALETTE_STRIDE], color);

        for (int32_t j = 0; j < PALETTE_SIZE && distance1 > 0; ++j) {
            distance2 = Color_GetColorDistance(&palette[j * PALETTE_STRIDE], color);

            if (distance2 < distance1) {
                distance1 = distance2;
                index = j;
            }
        }

        table[i] = index;
    }
}

void Color_GenerateIntensityTable2(uint8_t* palette, int32_t red_level, int32_t green_level, int32_t blue_level,
                                   ColorIndex* table) {
    int32_t max_level;
    int32_t max_color;
    int32_t red;
    int32_t green;
    int32_t blue;

    if (!red_level && !green_level && !blue_level) {
        red_level = 1;
        green_level = 1;
        blue_level = 1;
    }

    max_level = std::max(red_level, green_level);
    max_level = std::max(max_level, blue_level);

    for (int32_t i = 0, j = 0; i < PALETTE_STRIDE * PALETTE_SIZE; i += PALETTE_STRIDE, ++j) {
        red = palette[i + 0];
        green = palette[i + 1];
        blue = palette[i + 2];

        max_color = std::max(red, green);
        max_color = std::max(max_color, blue);

        max_color = (max_color + max_level) / 2;

        red = (max_color * red_level) / max_level;
        green = (max_color * green_level) / max_level;
        blue = (max_color * blue_level) / max_level;

        table[j] = Color_MapColor(palette, red, green, blue, false);
    }
}

void Color_GenerateIntensityTable3(uint8_t* palette, int32_t red_level, int32_t green_level, int32_t blue_level,
                                   int32_t factor, ColorIndex* table) {
    int32_t red;
    int32_t green;
    int32_t blue;

    for (int32_t i = 0, j = 0; i < PALETTE_STRIDE * PALETTE_SIZE; i += PALETTE_STRIDE, ++j) {
        red = palette[i + 0];
        green = palette[i + 1];
        blue = palette[i + 2];

        red = (std::max(red, red_level / 4) * red_level) / factor;
        green = (std::max(green, green_level / 4) * green_level) / factor;
        blue = (std::max(blue, blue_level / 4) * blue_level) / factor;

        table[j] = Color_MapColor(palette, red, green, blue, false);
    }
}

void Color_RecolorPixels(uint8_t* buffer, int32_t width, int32_t width_text, int32_t height_text, uint8_t* color_map) {
    for (int32_t i = 0; i < height_text; ++i) {
        for (int32_t j = 0; j < width_text; ++j) {
            buffer[i * width + j] = color_map[buffer[i * width + j]];
        }
    }
}

ColorIndex Color_MapColor(uint8_t* palette, Color r, Color g, Color b, bool full_scan) {
    ColorIndex color_index = 0;
    int32_t color_distance;
    int32_t color_distance_minimum;
    int32_t red;
    int32_t green;
    int32_t blue;

    color_distance_minimum = INT_MAX;

    for (int32_t i = 0; i < PALETTE_STRIDE * PALETTE_SIZE; i += PALETTE_STRIDE) {
        if (full_scan || ((i < 9 * PALETTE_STRIDE || i > 31 * PALETTE_STRIDE) &&
                          (i < 96 * PALETTE_STRIDE || i > 127 * PALETTE_STRIDE))) {
            red = palette[i] - r;
            green = palette[i + 1] - g;
            blue = palette[i + 2] - b;

            color_distance = blue * blue + green * green + red * red;

            if (color_distance < color_distance_minimum) {
                color_distance_minimum = color_distance;
                color_index = i / PALETTE_STRIDE;

                if (!color_distance_minimum) {
                    /* found perfect match in palette */
                    break;
                }
            }
        }
    }

    return color_index;
}
