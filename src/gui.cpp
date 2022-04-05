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

#include "gui.hpp"

#include "cursor.hpp"
#include "desyncmenu.hpp"
#include "enums.hpp"
#include "game_manager.hpp"
#include "remote.hpp"
#include "sound_manager.hpp"
#include "transfermenu.hpp"
#include "window_manager.hpp"

char GUI_PlayerTeamIndex;
char GUI_GameState;

bool GUI_DesyncMenu() {
    Remote_sub_C9753();

    return DesyncMenu().Run();
}

bool GUI_SelfDestructActiveMenu(WindowInfo* window) {
    Button* button_destruct;
    bool event_click_destruct;
    bool event_click_cancel;
    bool event_release;

    for (unsigned short id = SLFDOPN1; id <= SLFDOPN6; ++id) {
        unsigned int time_Stamp = timer_get_stamp32();

        // gwin_load_image2(static_cast<ResourceID>(id), 13, 11, 0, window);
        win_draw(window->id);
        /// \todo Implement function
        //        sub_A0E32(1, 1);

        while (timer_get_stamp32() - time_Stamp < TIMER_FPS_TO_TICKS(48)) {
        }
    }

    button_destruct = new (std::nothrow) Button(SLFDOK_U, SLFDOK_D, 13, 11);
    button_destruct->SetRValue(GNW_KB_KEY_RETURN);
    button_destruct->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_RETURN);
    button_destruct->SetSfx(NDONE0);
    button_destruct->RegisterButton(window->id);

    win_draw(window->id);

    event_click_destruct = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_destruct && !event_click_cancel) {
        int key = get_input();

        if (GameManager_RequestMenuExit) {
            key = GNW_KB_KEY_ESCAPE;
        }

        if (key == GNW_KB_KEY_RETURN) {
            event_click_destruct = true;
        } else if (key == GNW_KB_KEY_ESCAPE) {
            event_click_cancel = true;
        } else if (key >= GNW_INPUT_PRESS && !event_release) {
            if (key == GNW_INPUT_PRESS + GNW_KB_KEY_RETURN) {
                button_destruct->PlaySound();
            } else {
                /// \todo integrate interface
                soundmgr.PlaySfx(NCANC0);
            }

            event_release = true;
        }

        /// \todo Implement function
        //        sub_A0E32(1, 1);
    }

    delete button_destruct;

    return event_click_destruct;
}

bool GUI_SelfDestructMenu() {
    Window destruct_window(SELFDSTR, WINDOW_MAIN_MAP);
    WindowInfo window;
    Button* button_arm;
    Button* button_cancel;
    bool event_click_arm;
    bool event_click_cancel;
    bool event_release;

    Cursor_SetCursor(CURSOR_HAND);
    text_font(5);
    destruct_window.SetFlags(0x10);

    destruct_window.Add();
    destruct_window.FillWindowInfo(&window);

    button_arm = new (std::nothrow) Button(SLFDAR_U, SLFDAR_D, 89, 14);
    button_arm->SetCaption("Arm");
    button_arm->SetFlags(0x05);
    button_arm->SetPValue(GNW_KB_KEY_RETURN);
    button_arm->SetSfx(MBUTT0);
    button_arm->RegisterButton(window.id);

    button_cancel = new (std::nothrow) Button(SLFDCN_U, SLFDCN_D, 89, 46);
    button_cancel->SetCaption("Cancel");
    button_cancel->SetRValue(GNW_KB_KEY_ESCAPE);
    button_cancel->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    win_draw(window.id);

    event_click_arm = false;
    event_click_cancel = false;
    event_release = false;

    while (!event_click_arm && !event_click_cancel) {
        int key = get_input();

        if (GameManager_RequestMenuExit) {
            key = GNW_KB_KEY_ESCAPE;
        }

        if (key == GNW_KB_KEY_RETURN) {
            button_arm->PlaySound();
            button_arm->Disable();
            if (GUI_SelfDestructActiveMenu(&window)) {
                event_click_arm = true;
            } else {
                event_click_cancel = true;
            }
        } else if (key == GNW_KB_KEY_ESCAPE) {
            event_click_cancel = true;
        } else if (key >= GNW_INPUT_PRESS && !event_release) {
            button_cancel->PlaySound();
            event_release = true;
        }

        /// \todo Implement function
        //        sub_A0E32(1, 1);
    }

    delete button_arm;
    delete button_cancel;

    return event_click_arm;
}

unsigned short GUI_TransferMenu(UnitInfo* unit) {
    TransferMenu transfer_menu(unit);

    transfer_menu.Run();

    return transfer_menu.GetCargoTransferred();
}
