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
#include "text.hpp"

DesyncMenu::DesyncMenu()
    : Window(HELPFRAM, 38), event_click_restart(false), event_click_quit(false), event_release(false) {
    Cursor_SetCursor(CURSOR_HAND);
    text_font(5);
    SetFlags(0x10);

    Add();
    FillWindowInfo(&window);

    button_restart = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 155, 193);
    button_restart->SetCaption("Restart", 2, 2);
    button_restart->SetRValue(0x0D);
    button_restart->SetPValue(0x700D);
    button_restart->SetSfx(NDONE0);
    button_restart->RegisterButton(window.id);

    button_quit = new (std::nothrow) Button(XFRCAN_U, XFRCAN_D, 85, 193);
    button_quit->SetCaption("Quit", 2, 2);
    button_quit->SetRValue(0x1B);
    button_quit->SetPValue(0x701B);
    button_quit->SetSfx(NCANC0);
    button_quit->RegisterButton(window.id);

    Text_TextBox(reinterpret_cast<char*>(window.buffer), window.width,
                 "Unable to continue with next turn - remote players are no longer synchronized.\n\nPress Restart "
                 "to load from last auto-saved file.\nPress Quit to exit this game.",
                 20, 14, 265, 175, 0x100FF, false);
    win_draw_rect(window.id, &window.window);
}

DesyncMenu::~DesyncMenu() {
    delete button_restart;
    delete button_quit;
    /// \todo Clear MouseEvent object array
}

bool DesyncMenu::Run() {
    event_click_restart = false;
    event_click_quit = false;
    event_release = false;

    /// \todo Integrate missing stuff
    while (!event_click_restart && !event_click_quit /* && dword_175624 */) {
        int key;

        //        if (sub_C8626()) {
        //            key = 0x0D;
        //        } else {
        key = get_input();
        //        }

        if (key > 0 && key < 0x7000) {
            event_release = false;
        }

        if (key == 0x0D) {
            //            sub_C8897(51, GUI_PlayerTeamIndex, 0);
            event_click_restart = true;
        } else if (key == 0x1B) {
            event_click_quit = true;
        }

        if (!event_release) {
            if (key == 0x700D) {
                button_restart->PlaySound();
            } else {
                button_quit->PlaySound();
            }

            event_release = true;
        }
    }

    return event_click_restart;
}
