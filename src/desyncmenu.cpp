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

#include "desyncmenu.hpp"

#include "cursor.hpp"
#include "game_manager.hpp"
#include "mouseevent.hpp"
#include "remote.hpp"
#include "text.hpp"
#include "window_manager.hpp"

DesyncMenu::DesyncMenu()
    : Window(HELPFRAM, WINDOW_MAIN_MAP), event_click_restart(false), event_click_quit(false), event_release(false) {
    Cursor_SetCursor(CURSOR_HAND);
    text_font(5);
    SetFlags(0x10);

    Add();
    FillWindowInfo(&window);

    button_restart = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 155, 193);
    button_restart->SetCaption("Restart", 2, 2);
    button_restart->SetRValue(GNW_KB_KEY_RETURN);
    button_restart->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_RETURN);
    button_restart->SetSfx(NDONE0);
    button_restart->RegisterButton(window.id);

    button_quit = new (std::nothrow) Button(XFRCAN_U, XFRCAN_D, 85, 193);
    button_quit->SetCaption("Quit", 2, 2);
    button_quit->SetRValue(GNW_KB_KEY_ESCAPE);
    button_quit->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE);
    button_quit->SetSfx(NCANC0);
    button_quit->RegisterButton(window.id);

    Text_TextBox(window.buffer, window.width,
                 "Unable to continue with next turn - remote players are no longer synchronized.\n\nPress Restart "
                 "to load from last auto-saved file.\nPress Quit to exit this game.",
                 20, 14, 265, 175, 0x100FF, false);
    win_draw_rect(window.id, &window.window);
}

DesyncMenu::~DesyncMenu() {
    delete button_restart;
    delete button_quit;

    MouseEvent::Clear();
}

bool DesyncMenu::Run() {
    event_click_restart = false;
    event_click_quit = false;
    event_release = false;

    while (!event_click_restart && !event_click_quit && Remote_IsNetworkGame) {
        int key;

        if (Remote_CheckRestartAfterDesyncEvent()) {
            key = GNW_KB_KEY_RETURN;
        } else {
            key = get_input();
        }

        if (key > 0 && key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        if (key == GNW_KB_KEY_RETURN) {
            Remote_SendNetPacket_Signal(51, GameManager_PlayerTeam, 0);
            event_click_restart = true;
        } else if (key == GNW_KB_KEY_ESCAPE) {
            event_click_quit = true;
        }

        if (!event_release) {
            if (key == GNW_INPUT_PRESS + GNW_KB_KEY_RETURN) {
                button_restart->PlaySound();
            } else {
                button_quit->PlaySound();
            }

            event_release = true;
        }
    }

    return event_click_restart;
}
