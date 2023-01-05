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
#include "game_manager.hpp"
#include "mouseevent.hpp"
#include "text.hpp"
#include "window_manager.hpp"

OKCancelMenu::OKCancelMenu(const char* caption, bool mode)
    : Window(HELPFRAM, mode ? WINDOW_MAIN_WINDOW : WINDOW_MAIN_MAP),
      event_click_ok(false),
      event_click_cancel(false),
      event_release(false) {
    Cursor_SetCursor(CURSOR_HAND);
    text_font(GNW_TEXT_FONT_5);
    SetFlags(0x10);

    Add();
    FillWindowInfo(&window);

    button_ok = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 155, 193);
    button_ok->SetCaption("OK", 2, 2);
    button_ok->SetRValue(GNW_KB_KEY_RETURN);
    button_ok->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_RETURN);
    button_ok->SetSfx(NDONE0);
    button_ok->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(XFRCAN_U, XFRCAN_D, 85, 193);
    button_cancel->SetCaption("Cancel", 2, 2);
    button_cancel->SetRValue(GNW_KB_KEY_ESCAPE);
    button_cancel->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    Text_TextBox(window.buffer, window.width, caption, 20, 14, 265, 175, GNW_TEXT_OUTLINE | 0xFF, true);
    win_draw_rect(window.id, &window.window);
}

OKCancelMenu::~OKCancelMenu() {
    delete button_ok;
    delete button_cancel;

    MouseEvent::Clear();
}

bool OKCancelMenu::Run() {
    event_click_ok = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_ok && !event_click_cancel) {
        int key = get_input();

        if (key > 0 && key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        if (key == GNW_KB_KEY_RETURN) {
            event_click_ok = true;
        } else if (key == GNW_KB_KEY_ESCAPE) {
            event_click_cancel = true;
        }

        if (!event_release) {
            if (key == GNW_INPUT_PRESS + GNW_KB_KEY_RETURN) {
                button_ok->PlaySound();
            } else {
                button_cancel->PlaySound();
            }

            event_release = true;
        }

        GameManager_ProcessState(true);
    }

    return event_click_ok;
}
