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

#include "dialogmenu.hpp"

#include "cursor.hpp"
#include "game_manager.hpp"
#include "localization.hpp"
#include "remote.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

void DialogMenu::DrawText() {
    uint8_t* buffer;
    int32_t height;
    int32_t num_rows;
    int32_t row_width;

    canvas->Write(&window);

    buffer = &window.buffer[window.width * 14 + 20];
    height = Text_GetHeight() * row_count;

    if (height < 175) {
        buffer += window.width * ((175 - height) / 2);
    }

    num_rows = max_row_count + row_offset;

    if (num_rows > row_count) {
        num_rows = row_count;
    }

    row_width = 0;

    for (int32_t i = row_offset; i < num_rows; ++i) {
        if (center_align_text) {
            row_width = Text_GetWidth(strings[i].GetCStr());

            if (row_width > 265) {
                row_width = 265;
            }

            row_width = (265 - row_width) / 2;
        }

        Text_Blit(&buffer[row_width + ((i - row_offset) * Text_GetHeight()) * window.width], strings[i].GetCStr(), 265,
                  window.width, GNW_TEXT_OUTLINE | 0xFF);
    }

    win_draw_rect(window.id, &window.window);
}

bool DialogMenu::ProcessKey(int32_t key) {
    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_release = false;
    }

    switch (key) {
        case GNW_KB_KEY_PAGEUP:
        case 1000: {
            if (row_offset) {
                int32_t num_rows;

                num_rows = row_offset - max_row_count;

                if (num_rows < 0) {
                    num_rows = 0;
                }

                do {
                    uint32_t time_Stamp = timer_get();

                    --row_offset;
                    DrawText();

                    while (timer_get() - time_Stamp < TIMER_FPS_TO_MS(96)) {
                    }
                } while (num_rows != row_offset);
            }
        } break;

        case GNW_KB_KEY_RETURN:
        case GNW_KB_KEY_ESCAPE: {
            event_click_ok = true;
        } break;

        case GNW_KB_KEY_PAGEDOWN:
        case 1001: {
            if ((max_row_count + row_offset) < row_count) {
                int32_t num_rows;

                num_rows = row_offset + max_row_count;

                do {
                    uint32_t time_Stamp = timer_get();

                    ++row_offset;
                    DrawText();

                    while (timer_get() - time_Stamp < TIMER_FPS_TO_MS(96)) {
                    }
                } while (num_rows != row_offset);
            }
        } break;
    }

    if (key >= GNW_INPUT_PRESS) {
        if (!event_release) {
            if (key == GNW_INPUT_PRESS + GNW_KB_KEY_RETURN) {
                button_ok->PlaySound();
            } else {
                button_up->PlaySound();
            }

            event_release = true;
        }
    }

    return true;
}

DialogMenu::DialogMenu(const char* caption, bool mode)
    : Window(HELPFRAM, GameManager_GetDialogWindowCenterMode()),
      is_ingame(GameManager_GameState > GAME_STATE_3_MAIN_MENU),
      strings(nullptr),
      row_offset(0),
      button_up(nullptr),
      button_down(nullptr),
      center_align_text(mode) {
    Cursor_SetCursor(CURSOR_HAND);
    Text_SetFont(GNW_TEXT_FONT_5);
    SetFlags(WINDOW_MODAL);
    Add();
    FillWindowInfo(&window);

    canvas = new (std::nothrow) Image(20, 14, 265, 175);
    canvas->Copy(&window);

    button_ok = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 120, 193);
    button_ok->SetCaption(_(ae0b), 2, 2);
    button_ok->SetRValue(GNW_KB_KEY_RETURN);
    button_ok->SetPValue(GNW_INPUT_PRESS + GNW_KB_KEY_RETURN);
    button_ok->SetSfx(NDONE0);
    button_ok->RegisterButton(window.id);

    max_row_count = 175 / Text_GetHeight();

    strings = Text_SplitText(caption, max_row_count * 3, 265, &row_count);

    if (row_count > max_row_count) {
        button_up = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 20, 197);
        button_up->SetRValue(1000);
        button_up->SetPValue(GNW_INPUT_PRESS + 1000);
        button_up->SetSfx(KCARG0);
        button_up->RegisterButton(window.id);

        button_down = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 40, 197);
        button_down->SetRValue(1001);
        button_down->SetPValue(GNW_INPUT_PRESS + 1001);
        button_down->SetSfx(KCARG0);
        button_down->RegisterButton(window.id);
    }

    DrawText();
}

DialogMenu::~DialogMenu() {
    delete button_ok;
    delete button_up;
    delete button_down;
    delete[] strings;
    delete canvas;
}

void DialogMenu::Run() {
    event_click_ok = false;
    event_release = false;

    while (!event_click_ok) {
        int32_t key = get_input();

        ProcessKey(key);

        if (Remote_IsNetworkGame) {
            if (Remote_UiProcessTick()) {
                UnitsManager_ProcessOrders();
            }
        } else {
            if (is_ingame) {
                GameManager_ProcessState(true);
            }
        }
    }
}

void DialogMenu::RunMenu() {
    event_click_ok = false;

    while (!event_click_ok) {
        int32_t key = get_input();

        if (Remote_IsNetworkGame) {
            if (Remote_CheckUnpauseEvent()) {
                key = GNW_KB_KEY_ESCAPE;
            }
        }

        ProcessKey(key);
    }
}
