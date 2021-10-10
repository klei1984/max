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
#include "gui.hpp"
#include "text.hpp"

extern "C" {
#include "gnw.h"
}

void DialogMenu::DrawText() {
    char* buffer;
    int height;
    int num_rows;
    int row_width;

    canvas->Write(&window);

    buffer = reinterpret_cast<char*>(&window.buffer[window.width * 14 + 20]);
    height = text_height() * row_count;

    if (height < 175) {
        buffer += window.width * ((175 - height) / 2);
    }

    num_rows = max_row_count + row_offset;

    if (num_rows > row_count) {
        num_rows = row_count;
    }

    row_width = 0;

    for (int i = row_offset; i < num_rows; ++i) {
        if (field_64) {
            row_width = text_width(strings[i].GetCStr());

            if (row_width > 265) {
                row_width = 265;
            }

            row_width = (265 - row_width) / 2;
        }

        text_to_buf(
            reinterpret_cast<unsigned char*>(&buffer[row_width + ((i - row_offset) * text_height()) * window.width]),
            strings[i].GetCStr(), 265, window.width, 0x100FF);
    }

    win_draw_rect(window.id, &window.window);
}

bool DialogMenu::ProcessKey(int key) {
    if (key > 0 && key < 0x7000) {
        event_release = false;
    }

    if (key == 0x149 || key == 0x3E8) {
        if (row_offset) {
            int num_rows;

            num_rows = row_offset - max_row_count;

            if (num_rows < 0) {
                num_rows = 0;
            }

            do {
                unsigned int time_Stamp = timer_get_stamp32();

                --row_offset;
                DrawText();

                while (timer_get_stamp32() - time_Stamp < 12428) {
                    ;
                }
            } while (num_rows != row_offset);
        }
    } else if (key == 0x0D || key == 0x1B) {
        event_click_ok = true;
    } else if (key == 0x151 || key == 0x3E9) {
        if ((max_row_count + row_offset) < row_count) {
            int num_rows;

            num_rows = row_offset + max_row_count;

            do {
                unsigned int time_Stamp = timer_get_stamp32();

                ++row_offset;
                DrawText();

                while (timer_get_stamp32() - time_Stamp < 12428) {
                    ;
                }
            } while (num_rows != row_offset);
        }
    }

    if (key >= 0x7000) {
        if (!event_release) {
            if (key == 0x700D) {
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
    : Window(HELPFRAM, GUI_GameMode == 3 ? 0 : 38),
      field_62(GUI_GameMode != 3),
      strings(nullptr),
      row_offset(0),
      button_up(nullptr),
      button_down(nullptr),
      field_64(mode) {
    Cursor_SetCursor(CURSOR_HAND);
    text_font(5);
    SetFlags(0x10);
    Add();
    FillWindowInfo(&window);

    canvas = new (std::nothrow) Image(20, 14, 265, 175);
    canvas->Copy(&window);

    button_ok = new (std::nothrow) Button(HELPOK_U, HELPOK_D, 120, 193);
    button_ok->SetCaption("OK", 2, 2);
    button_ok->SetRValue(0x0D);
    button_ok->SetPValue(0x700D);
    button_ok->SetSfx(NDONE0);
    button_ok->RegisterButton(window.id);

    max_row_count = 175 / text_height();

    strings = Text_SplitText(caption, max_row_count * 3, 265, &row_count);

    if (row_count > max_row_count) {
        button_up = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 20, 197);
        button_up->SetRValue(0x3E8);
        button_up->SetPValue(0x73E8);
        button_up->SetSfx(KCARG0);
        button_up->RegisterButton(window.id);

        button_down = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 40, 197);
        button_down->SetRValue(0x3E9);
        button_down->SetPValue(0x73E9);
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
        int key = get_input();

        ProcessKey(key);

        /// \todo Implement missing stuff
        //        if (dword_175624) {
        //            if (sub_C8835()) {
        //                sub_102CB8();
        //            }
        //        } else {
        //            if (field_62) {
        //                sub_A0E32(1, 1);
        //            }
        //        }
    }
}