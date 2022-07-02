/* Copyright (c) 2022 M.A.X. Port Team
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

#include "color.hpp"

int Color_GetColorDistance(Color* color1, Color* color2) {
    int diff_r;
    int diff_g;
    int diff_b;
    diff_r = color1[0] - color2[0];
    diff_g = color1[1] - color2[1];
    diff_b = color1[2] - color2[2];

    return diff_r * diff_r + diff_g * diff_g + diff_b * diff_b;
}

void Color_ChangeColorTemperature(int factor_r, int factor_g, int factor_b, int multiplier_factor1,
                                  int multiplier_factor2, unsigned char* color_map) {
    int multiplier_factor_sum;
    Color* palette;
    int index;
    Color color[3];
    int distance1;
    int distance2;

    palette = getSystemPalette();

    multiplier_factor_sum = multiplier_factor1 + multiplier_factor2;

    for (int i = 0; i < PALETTE_SIZE; ++i) {
        index = i;

        color[0] = (palette[i * 3 + 0] * multiplier_factor2 + factor_r * multiplier_factor1) / multiplier_factor_sum;
        color[1] = (palette[i * 3 + 1] * multiplier_factor2 + factor_g * multiplier_factor1) / multiplier_factor_sum;
        color[2] = (palette[i * 3 + 2] * multiplier_factor2 + factor_b * multiplier_factor1) / multiplier_factor_sum;

        distance1 = Color_GetColorDistance(&palette[index * 3], color);

        for (int j = 0; j < PALETTE_SIZE && distance1 > 0; ++j) {
            distance2 = Color_GetColorDistance(&palette[j * 3], color);

            if (distance2 < distance1) {
                distance1 = distance2;
                index = j;
            }
        }

        color_map[i] = index;
    }
}

void Color_RecolorPixels(unsigned char* buffer, int width, int width_text, int height_text, unsigned char* color_map) {
    for (int i = 0; i < height_text; ++i) {
        for (int j = 0; j < width_text; ++j) {
            buffer[i * width + j] = color_map[buffer[i * width + j]];
        }
    }
}
