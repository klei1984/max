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

#ifndef DIALOGMENU_HPP
#define DIALOGMENU_HPP

#include "button.hpp"
#include "smartstring.hpp"
#include "window.hpp"

class DialogMenu : Window {
    WindowInfo window;
    Button* button_ok;
    Button* button_up;
    Button* button_down;
    Image* canvas;
    SmartString* strings;
    int32_t row_count;
    int32_t max_row_count;
    int32_t row_offset;
    bool event_click_ok;
    bool is_ingame;
    bool event_release;
    bool center_align_text;

    void DrawText();
    bool ProcessKey(int32_t key);

public:
    DialogMenu(const char* text, bool mode);
    ~DialogMenu();

    void Run();
    void RunMenu();
};

#endif /* DIALOGMENU_HPP */
