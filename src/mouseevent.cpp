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

#include "mouseevent.hpp"

extern "C" {
#include "gnw.h"
}

#include "smartobjectarray.hpp"

static ObjectArray<MouseEvent> MouseEvent_MouseEvents;
static unsigned char MouseEvent_PreviousButtons;

void MouseEvent::Clear() {
    MouseEvent_MouseEvents.Clear();
    MouseEvent_PreviousButtons = 0;
}

void MouseEvent::ProcessInput() {
    int mouse_buttons;

    process_bk();
    mouse_buttons = mouse_get_buttons();

    if (mouse_buttons & MOUSE_LONG_PRESS_LEFT) {
        mouse_buttons |= MOUSE_PRESS_LEFT;
    }

    if (mouse_buttons & MOUSE_LONG_PRESS_RIGHT) {
        mouse_buttons |= MOUSE_PRESS_RIGHT;
    }

    if (mouse_buttons != MouseEvent_PreviousButtons) {
        MouseEvent mouse_event;
        int mouse_x;
        int mouse_y;

        mouse_get_position(&mouse_x, &mouse_y);

        if ((mouse_buttons & MOUSE_PRESS_LEFT)) {
            if ((MouseEvent_PreviousButtons & MOUSE_PRESS_LEFT) == 0) {
                mouse_event.buttons = MOUSE_PRESS_LEFT;
                mouse_event.point.x = mouse_x;
                mouse_event.point.y = mouse_y;

                MouseEvent_MouseEvents.Append(&mouse_event);
            }
        } else if (MouseEvent_PreviousButtons & MOUSE_PRESS_LEFT) {
            mouse_event.buttons = MOUSE_RELEASE_LEFT;
            mouse_event.point.x = mouse_x;
            mouse_event.point.y = mouse_y;

            MouseEvent_MouseEvents.Append(&mouse_event);
        }

        if ((mouse_buttons & MOUSE_PRESS_RIGHT) != 0) {
            if ((MouseEvent_PreviousButtons & MOUSE_PRESS_RIGHT) == 0) {
                mouse_event.buttons = MOUSE_PRESS_RIGHT;
                mouse_event.point.x = mouse_x;
                mouse_event.point.y = mouse_y;

                MouseEvent_MouseEvents.Append(&mouse_event);
            }
        } else if ((MouseEvent_PreviousButtons & MOUSE_PRESS_RIGHT) != 0) {
            mouse_event.buttons = MOUSE_RELEASE_RIGHT;
            mouse_event.point.x = mouse_x;
            mouse_event.point.y = mouse_y;

            MouseEvent_MouseEvents.Append(&mouse_event);
        }

        MouseEvent_PreviousButtons = mouse_buttons;
    }
}

bool MouseEvent::PopFront(MouseEvent& mouse_event) {
    bool result;

    ProcessInput();

    if (MouseEvent_MouseEvents.GetCount()) {
        mouse_event = *MouseEvent_MouseEvents[0];
        MouseEvent_MouseEvents.Remove(0);

        result = true;

    } else {
        result = false;
    }

    return result;
}
