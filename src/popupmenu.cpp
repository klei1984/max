/* Copyright (c) 2026 M.A.X. Port Team
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

#include "popupmenu.hpp"

#include <new>

#include "cursor.hpp"
#include "mouseevent.hpp"
#include "text.hpp"
#include "window_manager.hpp"

/* Geometry of the HELPFRAM popup frame: the caption area and the button row. */
static constexpr int32_t PopupMenu_CaptionUlx = 20;
static constexpr int32_t PopupMenu_CaptionUly = 14;
static constexpr int32_t PopupMenu_CaptionWidth = 265;
static constexpr int32_t PopupMenu_CaptionHeight = 175;
static constexpr int32_t PopupMenu_ButtonRowUly = 193;

PopupMenu::PopupMenu(const char* caption, uint8_t parent_window_id, bool center_align_caption)
    : Window(HELPFRAM, parent_window_id) {
    Cursor_SetCursor(CURSOR_HAND);
    Text_SetFont(GNW_TEXT_FONT_5);
    SetFlags(WINDOW_MODAL);

    Add();
    FillWindowInfo(&window);

    Text_TextBox(window.buffer, window.width, caption, PopupMenu_CaptionUlx, PopupMenu_CaptionUly,
                 PopupMenu_CaptionWidth, PopupMenu_CaptionHeight, GNW_TEXT_OUTLINE | 0xFF, center_align_caption);
}

PopupMenu::~PopupMenu() { MouseEvent::Clear(); }

Button* PopupMenu::CreateButton(ResourceID up, ResourceID down, int32_t ulx, const char* caption, int32_t r_value,
                                ResourceID sfx) {
    Button* button = new (std::nothrow) Button(up, down, ulx, PopupMenu_ButtonRowUly);

    button->SetCaption(caption, 2, 2);
    button->SetRValue(r_value);
    button->SetPValue(GNW_INPUT_PRESS + r_value);
    button->SetSfx(sfx);
    button->RegisterButton(window.id);

    return button;
}

void PopupMenu::Draw() { win_draw_rect(window.id, &window.window); }
