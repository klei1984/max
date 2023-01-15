/* Copyright (c) 2022 M.A.X. Port Team
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

#include "menulandingsequence.hpp"

#include "game_manager.hpp"
#include "sound_manager.hpp"
#include "window_manager.hpp"

MenuLandingSequence::MenuLandingSequence()
    : panel_top(WindowManager_GetWindow(WINDOW_INTERFACE_PANEL_TOP)),
      panel_bottom(WindowManager_GetWindow(WINDOW_INTERFACE_PANEL_BOTTOM)),
      image_1(nullptr),
      image_2(nullptr),
      button_1(nullptr),
      button_2(nullptr),
      time_stamp(0) {}

MenuLandingSequence::~MenuLandingSequence() { Deinit(); }

void MenuLandingSequence::DeleteButtons() {
    delete button_1;
    button_1 = nullptr;

    delete button_2;
    button_2 = nullptr;
}

void MenuLandingSequence::Deinit() {
    delete image_1;
    image_1 = nullptr;

    delete image_2;
    image_2 = nullptr;

    DeleteButtons();
}

void MenuLandingSequence::Init(bool enable_controls) {
    GameManager_MenuInitButtons(false);
    GameManager_MenuDeinitButtons();

    image_1 = new (std::nothrow) Image(0, 0, panel_top->window.lrx - panel_top->window.ulx + 1,
                                       panel_top->window.lry - panel_top->window.uly + 1);
    image_1->Copy(panel_top);

    image_2 = new (std::nothrow) Image(0, 0, panel_bottom->window.lrx - panel_bottom->window.ulx + 1,
                                       panel_bottom->window.lry - panel_bottom->window.uly + 1);
    image_2->Copy(panel_bottom);

    WindowManager_LoadImage2(PANELTOP, panel_top->window.ulx, panel_top->window.uly, true);
    WindowManager_LoadImage2(PANELBTM, panel_bottom->window.ulx, panel_bottom->window.uly, true);

    if (enable_controls) {
        button_1 = new (std::nothrow) Button(PNLHLP_U, PNLHLP_D, 103, 252);
        button_1->SetRValue(GNW_KB_KEY_SHIFT_DIVIDE);
        button_1->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_SHIFT_DIVIDE);
        button_1->SetSfx(NHELP0);
        button_1->RegisterButton(panel_bottom->id);
        button_1->Enable();

        text_font(GNW_TEXT_FONT_5);

        button_2 = new (std::nothrow) Button(PNLCAN_U, PNLCAN_D, 40, 252);
        button_2->SetRValue(GNW_KB_KEY_ESCAPE);
        button_2->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE);
        button_2->SetSfx(NCANC0);
        button_2->SetCaption("Cancel");
        button_2->RegisterButton(panel_bottom->id);
        button_2->Enable();
    }
}

void MenuLandingSequence::AnimateStep(int offset) {
    WindowManager_LoadImage2(PANELTOP, panel_top->window.ulx, panel_top->window.uly - offset, true);
    WindowManager_LoadImage2(PANELBTM, panel_bottom->window.ulx, panel_bottom->window.uly + offset, true);
    win_draw_rect(panel_top->id, &panel_top->window);
    win_draw_rect(panel_bottom->id, &panel_bottom->window);

    process_bk();

    while ((timer_get_stamp32() - time_stamp) < TIMER_FPS_TO_TICKS(24)) {
    }
}

void MenuLandingSequence::OpenPanel() {
    DeleteButtons();
    SoundManager.PlaySfx(IOPEN0);

    for (int i = 0; i < 11; ++i) {
        time_stamp = timer_get_stamp32();
        image_1->Write(panel_top);
        image_2->Write(panel_bottom);

        AnimateStep(i * 20);
    }

    if (image_1) {
        image_1->Write(panel_top);
        win_draw_rect(panel_top->id, &panel_top->window);

        delete image_1;
        image_1 = nullptr;
    }

    if (image_2) {
        image_2->Write(panel_bottom);
        win_draw_rect(panel_bottom->id, &panel_bottom->window);

        delete image_2;
        image_2 = nullptr;
    }
}

void MenuLandingSequence::ClosePanel() {
    if (GameManager_DisplayControlsInitialized) {
        GameManager_PlayFlic = false;
        Gamemanager_FlicButton->Disable();

        GameManager_MenuDeinitButtons();

        SoundManager.PlaySfx(ICLOS0);

        for (int i = 11; i >= 0; --i) {
            time_stamp = timer_get_stamp32();
            AnimateStep(i * 20);
        }
    }

    Deinit();
}