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

#include "drawloadbar.hpp"

#include "enums.hpp"
#include "game_manager.hpp"
#include "resource_manager.hpp"
#include "text.hpp"
#include "window_manager.hpp"

DrawLoadBar::DrawLoadBar(const char* text) : Window(DIALGPIC, GameManager_GetDialogWindowCenterMode()) {
    Text_SetFont(GNW_TEXT_FONT_5);
    Add();
    FillWindowInfo(&window);
    Text_TextBox(&window, text, 0, 140, window.width, 30, true);
    Rect slider_bounds = {14, 175, 171, 192};
    Rect value_bounds = {0, 0, 0, 0};
    loadbar = new (std::nothrow) LimitedScrollbar(this, &slider_bounds, &value_bounds, SMBRRAW, 0, 0, 0, 0, false);
    loadbar->Register();
    loadbar->SetFreeCapacity(100);
    loadbar->SetValue(0);
    loadbar->SetMaterialBar(LOADBAR);
    loadbar->RefreshScreen();
    win_draw_rect(window.id, &window.window);
}

DrawLoadBar::~DrawLoadBar() { delete loadbar; }

void DrawLoadBar::SetValue(int16_t value) {
    loadbar->SetValue(value);
    loadbar->RefreshScreen();

    if (value <= 80) {
        int32_t length = ResourceManager_MapSize.y * value / 75;

        if (length > ResourceManager_MapSize.y) {
            length = ResourceManager_MapSize.y;
        }

        const int32_t window_width{window.window.lrx - window.window.ulx + 1 - 76};
        const int32_t window_height{window.window.lry - window.window.uly + 1 - 99};

        if (ResourceManager_MapSize.x == window_width && ResourceManager_MapSize.y == window_height) {
            buf_to_buf(ResourceManager_MinimapFov, ResourceManager_MapSize.x, length, ResourceManager_MapSize.x,
                       &window.buffer[window.width * 21 + 37], window.width);

        } else {
            int32_t output_length = window_height * value / 75;

            if (output_length > window_height) {
                output_length = window_height;
            }

            cscale(ResourceManager_MinimapFov, ResourceManager_MapSize.x, length, ResourceManager_MapSize.x,
                   &window.buffer[window.width * 21 + 37], window_width, output_length, window.width);
        }
    }

    win_draw_rect(window.id, &window.window);
    process_bk();
}
