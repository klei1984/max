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

#include "buttonmanager.hpp"

#include <new>

ButtonManager::ButtonManager() : slots(0), used(0), buttons(nullptr) {}

ButtonManager::~ButtonManager() { Deinit(); }

void ButtonManager::Add(ButtonID button_id) {
    if (slots == used) {
        ButtonID* buffer;

        slots += 5;

        buffer = new (std::nothrow) ButtonID[slots];

        if (used) {
            memcpy(buffer, buttons, used * sizeof(ButtonID));
            delete[] buttons;
        }

        buttons = buffer;
    }

    buttons[used++] = button_id;
}

void ButtonManager::Deinit() {
    for (int i = 0; i < used; ++i) {
        win_delete_button(buttons[i]);
    }

    delete[] buttons;
    buttons = nullptr;
    used = 0;
    slots = 0;
}

void ButtonManager::EnableAll() {
    for (int i = 0; i < used; ++i) {
        win_enable_button(buttons[i]);
    }
}

void ButtonManager::DisableAll() {
    for (int i = 0; i < used; ++i) {
        win_disable_button(buttons[i]);
    }
}

void ButtonManager::Group() { win_group_radio_buttons(used, buttons); }
