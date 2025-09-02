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

#ifndef HELPMENU_HPP
#define HELPMENU_HPP

#include "button.hpp"
#include "smartstring.hpp"
#include "window.hpp"

class HelpMenu : public Window {
    WindowInfo window;
    std::string text;
    Button *button_done;
    Button *button_keys;
    Button *button_up;
    Button *button_down;
    bool event_click_cancel;
    bool event_click_help;
    bool event_click_done;
    bool keys_mode;
    uint8_t window_id;
    Image *canvas;
    uint16_t rows_per_page;
    int32_t string_row_count;
    uint16_t string_row_index;
    SmartString *strings;

    void ProcessText(const std::string &string);
    bool ProcessKey(int32_t key);
    void DrawText();

public:
    HelpMenu(const std::string &section, int32_t cursor_x, int32_t cursor_y, const int32_t window_id);
    ~HelpMenu();

    bool Run(int32_t mode);
};

void HelpMenu_Menu(const std::string &section, const int32_t window_index, const bool mode = false);
bool HelpMenu_UnitReport(int32_t mouse_x, int32_t mouse_y);

#endif /* HELPMENU_HPP */
