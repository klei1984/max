/* Copyright (c) 2023 M.A.X. Port Team
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

#include "assertmenu.hpp"

#include "game_manager.hpp"
#include "remote.hpp"
#include "window_manager.hpp"

AssertMenu::AssertMenu(const char* caption)
    : PopupMenu(caption, GameManager_GetDialogWindowCenterMode(), true),
      button_break(CreateButton(HELPOK_U, HELPOK_D, 185, "Debug", GNW_KB_KEY_D, NDONE0)),
      button_abort(CreateButton(HELPOK_U, HELPOK_D, 115, "Abort", GNW_KB_KEY_ESCAPE, NCANC0)),
      button_ignore(CreateButton(HELPOK_U, HELPOK_D, 45, "Ignore", GNW_KB_KEY_RETURN, NDONE0)),
      show_cursor_state(!mouse_hidden()),
      event_click(false),
      event_release(false) {
    if (!show_cursor_state) {
        mouse_set_position(WindowManager_GetWidth(&window) / 2, WindowManager_GetHeight(&window) / 2);
        mouse_show();
    }

    Draw();
}

AssertMenu::~AssertMenu() {
    delete button_ignore;
    delete button_abort;
    delete button_break;

    if (!show_cursor_state) {
        mouse_hide();
    }
}

int32_t AssertMenu::Run() {
    int32_t result{SDL_ASSERTION_ALWAYS_IGNORE};

    if (Remote_IsNetworkGame) {
        result = SDL_ASSERTION_ALWAYS_IGNORE;

    } else {
        event_click = false;
        event_release = false;

        while (!event_click) {
            int32_t key = get_input();

            if (key > 0 && key < GNW_INPUT_PRESS) {
                event_release = false;
            }

            switch (key) {
                case GNW_KB_KEY_RETURN: {
                    result = SDL_ASSERTION_ALWAYS_IGNORE;
                    event_click = true;
                } break;

                case GNW_KB_KEY_ESCAPE: {
                    result = SDL_ASSERTION_ABORT;
                    event_click = true;
                } break;

                case GNW_KB_KEY_D: {
                    result = SDL_ASSERTION_BREAK;
                    event_click = true;
                } break;
            }

            if (!event_release) {
                if (key == GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE) {
                    button_abort->PlaySound();
                } else {
                    button_ignore->PlaySound();
                }

                event_release = true;
            }
        }
    }

    return result;
}
