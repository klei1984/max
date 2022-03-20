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

#include "game_manager.hpp"

#include "sound_manager.hpp"

Rect GameManager_GridPosition;
SmartPointer<UnitInfo> GameManager_SelectedUnit;
unsigned int GameManager_TurnCounter;
int GameMamager_GameFileNumber;
int GameManager_HumanPlayerCount;
unsigned char GameManager_MarkerColor = 0xFF;

void GameManager_GameLoop(int game_state) {
    /// \todo Implement function
    soundmgr.PlayMusic(MAIN_MSC, false);
}

void GameManager_DrawUnitSelector(unsigned char *buffer, int width, int offsetx, int height, int offsety, int bottom,
                                  int item_height, int top, int scaling_factor, int is_big_sprite, bool double_marker) {
    unsigned char color;
    int loop_count;
    int scaled_size2;
    int size2;
    int scaled_size1;
    int size1;
    int var1;
    int var2;
    int var3;

    if (is_big_sprite) {
        size1 = 32;
    } else {
        size1 = 16;
    }

    scaled_size1 = (size1 << 16) / scaling_factor;

    if (is_big_sprite) {
        size2 = 128;
    } else {
        size2 = 64;
    }

    scaled_size2 = (size2 << 16) / scaling_factor - 2;

    if (double_marker) {
        loop_count = 3;
    } else {
        loop_count = 1;
    }

    if (double_marker) {
        color = GameManager_MarkerColor;
    } else {
        color = 0xFF;
    }

    for (int i = 0; i < loop_count; ++i) {
        if (height >= bottom && height <= top && scaled_size1 + offsetx >= offsety && offsetx <= item_height) {
            draw_line(buffer, width, std::max(offsetx, offsety), height, std::min(scaled_size1 + offsetx, item_height),
                      height, i);
        }

        if (offsetx >= offsety && offsetx <= item_height && scaled_size1 + height >= bottom && height <= top) {
            draw_line(buffer, width, offsetx, std::max(height, bottom), offsetx, std::min(scaled_size1 + height, top),
                      i);
        }

        var1 = scaled_size2 + offsetx;

        if (height >= bottom && height <= top && var1 >= offsety && var1 - scaled_size1 <= item_height) {
            draw_line(buffer, width, std::max(var1 - scaled_size1, offsety), height, std::min(var1, item_height),
                      height, i);
        }

        if (var1 >= offsety && var1 <= item_height && scaled_size1 + height >= bottom && height <= top) {
            draw_line(buffer, width, var1, std::max(height, bottom), var1, std::min(scaled_size1 + height, top), i);
        }

        var3 = scaled_size2 + height;

        if (var3 >= bottom && var3 <= top && var1 >= offsety && var1 - scaled_size1 <= item_height) {
            draw_line(buffer, width, std::max(var1 - scaled_size1, offsety), var3, std::min(var1, item_height), var3,
                      i);
        }

        if (var1 >= offsety && var1 <= item_height && var3 >= bottom && var3 - scaled_size1 <= top) {
            draw_line(buffer, width, var1, std::min(var3, top), var1, std::max(var3 - scaled_size1, bottom), i);
        }

        var2 = var1 - scaled_size2;

        if (var3 >= bottom && var3 <= top && scaled_size1 + var2 >= offsety && var2 <= item_height) {
            draw_line(buffer, width, std::max(var2, offsety), var3, std::min(scaled_size1 + var2, item_height), var3,
                      i);
        }

        if (var2 >= offsety && var2 <= item_height && var3 >= bottom && var3 - scaled_size1 <= top) {
            draw_line(buffer, width, var2, std::min(var3, top), var2, std::max(var3 - scaled_size1, bottom), i);
        }

        scaled_size2 -= 2;
        offsetx = var2 + 1;
        height = var3 - scaled_size2 + 1;
        --scaled_size1;
        color ^= 0xFF;
    }
}
