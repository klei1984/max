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

#include "cursor.hpp"
#include "game_manager.hpp"
#include "localization.hpp"
#include "mouseevent.hpp"
#include "remote.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

AssertMenu::AssertMenu(const char* caption)
    : Window(HELPFRAM, GameManager_GetDialogWindowCenterMode()),
      event_click(false),
      event_release(false),
      show_cursor_state(!mouse_hidden()) {
    Cursor_SetCursor(CURSOR_HAND);
    Text_SetFont(GNW_TEXT_FONT_5);
    SetFlags(WINDOW_MODAL);

    Add();
    FillWindowInfo(&window);

    if (!show_cursor_state) {
        mouse_set_position(WindowManager_GetWidth(&window) / 2, WindowManager_GetHeight(&window) / 2);
        mouse_show();
    }

    button_ignore = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 45, 193);
    button_ignore->SetCaption("Ignore", 2, 2);
    button_ignore->SetRValue(GNW_KB_KEY_RETURN);
    button_ignore->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_RETURN);
    button_ignore->SetSfx(NDONE0);
    button_ignore->RegisterButton(window.id);

    button_abort = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 115, 193);
    button_abort->SetCaption("Abort", 2, 2);
    button_abort->SetRValue(GNW_KB_KEY_ESCAPE);
    button_abort->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE);
    button_abort->SetSfx(NCANC0);
    button_abort->RegisterButton(window.id);

    button_break = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 185, 193);
    button_break->SetCaption("Debug", 2, 2);
    button_break->SetRValue(GNW_KB_KEY_D);
    button_break->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_D);
    button_break->SetSfx(NDONE0);
    button_break->RegisterButton(window.id);

    Text_TextBox(window.buffer, window.width, caption, 20, 14, 265, 175, GNW_TEXT_OUTLINE | 0xFF, true);
    win_draw_rect(window.id, &window.window);
}

AssertMenu::~AssertMenu() {
    delete button_ignore;
    delete button_abort;
    delete button_break;

    MouseEvent::Clear();

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
