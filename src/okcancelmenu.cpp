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

#include "okcancelmenu.hpp"

#include "cursor.hpp"
#include "text.hpp"

OKCancelMenu::OKCancelMenu(const char* caption, bool mode)
    : Window(HELPFRAM, mode ? 0 : 38), event_click_ok(false), event_click_cancel(false), event_release(false) {
    Cursor_SetCursor(CURSOR_HAND);
    text_font(5);
    SetFlags(0x10);

    Add();
    FillWindowInfo(&window);

    button_ok = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 155, 193);
    button_ok->SetCaption("OK", 2, 2);
    button_ok->SetRValue(0x0D);
    button_ok->SetPValue(0x700D);
    button_ok->SetSfx(NDONE0);
    button_ok->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(XFRCAN_U, XFRCAN_D, 85, 193);
    button_cancel->SetCaption("Cancel", 2, 2);
    button_cancel->SetRValue(0x1B);
    button_cancel->SetPValue(0x701B);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    Text_TextBox(reinterpret_cast<char*>(window.buffer), window.width, caption, 20, 14, 265, 175, 0x100FF, true);
    win_draw_rect(window.id, &window.window);
}

OKCancelMenu::~OKCancelMenu() {
    delete button_ok;
    delete button_cancel;
    /// \todo Clear MouseEvent object array
}

bool OKCancelMenu::Run() {
    event_click_ok = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_ok && !event_click_cancel) {
        int key = get_input();

        if (key > 0 && key < 0x7000) {
            event_release = false;
        }

        if (key == 0x0D) {
            event_click_ok = true;
        } else if (key == 0x1B) {
            event_click_cancel = true;
        }

        if (!event_release) {
            if (key == 0x700D) {
                button_ok->PlaySound();
            } else {
                button_cancel->PlaySound();
            }

            event_release = true;
        }

        /// \todo Implement function
        //        sub_A0E32(1, 1);
    }

    return event_click_ok;
}
