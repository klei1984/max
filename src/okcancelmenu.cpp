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

#include "game_manager.hpp"

OKCancelMenu::OKCancelMenu(const char* caption)
    : PopupMenu(caption, GameManager_GetDialogWindowCenterMode(), true),
      event_click_ok(false),
      event_click_cancel(false),
      button_ok(CreateButton(HELPOK_U, HELPOK_D, 155, _(755f), GNW_KB_KEY_RETURN, NDONE0)),
      button_cancel(CreateButton(XFRCAN_U, XFRCAN_D, 85, _(2879), GNW_KB_KEY_ESCAPE, NCANC0)),
      event_release(false) {
    Draw();
}

OKCancelMenu::~OKCancelMenu() {
    delete button_ok;
    delete button_cancel;
}

bool OKCancelMenu::Run() {
    event_click_ok = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_ok && !event_click_cancel) {
        int32_t key = get_input();

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
