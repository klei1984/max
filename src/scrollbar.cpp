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

#include "scrollbar.hpp"

#include <algorithm>
#include <cstdio>
#include <new>

#include "resource_manager.hpp"
#include "sound_manager.hpp"

EVENTS_REGISTER_EVENT(ScrollbarEvent);

EventScrollbarChange::EventScrollbarChange(Scrollbar* scrollbar, short value) : scrollbar(scrollbar), value(value) {}
unsigned short EventScrollbarChange::GetEventId() { return EVENTS_GET_EVENT_ID(ScrollbarEvent); }

void LoadHorizontalBar(unsigned char* buffer, short width, short capacity, short height, ResourceID id) {
    struct ImageSimpleHeader* sprite;
    char transparent_color;
    unsigned char* data;

    sprite = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(id));

    data = &sprite->data[sprite->width - height];
    transparent_color = sprite->data[0];

    for (int i = 0; i < capacity; ++i) {
        for (int j = 0; j < height; ++j) {
            if (data[j] != transparent_color) {
                buffer[j] = data[j];
            }
        }

        buffer = &buffer[width];
        data = &data[sprite->width];
    }
}

void LoadVerticalBar(unsigned char* buffer, short width, short capacity, short height, ResourceID id) {
    struct ImageSimpleHeader* sprite;
    char transparent_color;
    unsigned char* data;
    int offset;

    if (capacity > 0) {
        sprite = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(id));

        if (capacity > sprite->height) {
            capacity = sprite->height;
        }

        data = sprite->data;
        buffer = &buffer[(sprite->width - height) / 2];
        transparent_color = sprite->data[0];

        for (int i = 0; i < capacity; ++i) {
            for (int j = 0; j < sprite->width; ++j) {
                if (data[j] != transparent_color) {
                    buffer[j] = data[j];
                }
            }

            buffer = &buffer[width];
            data = &data[sprite->width];
        }
    }
}

void Scrollbar::ProcessValueChange(short value) {
    short old_value;

    old_value = this->value;
    this->value = value;

    if (old_value != value) {
        soundmgr.PlaySfx(KCARG0);
        EventScrollbarChange event(this, old_value);
        RefreshScreen();
        window->EventHandler(event);
    }
}

Scrollbar::Scrollbar(Window* window, Rect* xfer_slider_bounds, Rect* xfer_amount_bounds, ResourceID id,
                     int key_code_increase, int key_code_decrease, int key_code_click_slider, short scaling_factor,
                     bool vertical)
    : material_bar(id),
      key_code_increase(key_code_increase),
      key_code_decrease(key_code_decrease),
      button_p_value(key_code_click_slider),
      window(window),
      free_capacity(0),
      zero_offset(0),
      value(0),
      scaling_factor(scaling_factor),
      scrollbar_type(vertical) {
    short ulx;
    short uly;
    short width;
    short height;

    if (scaling_factor) {
        ulx = xfer_amount_bounds->ulx;
        uly = xfer_amount_bounds->uly;
        width = xfer_amount_bounds->lrx - xfer_amount_bounds->ulx;
        height = xfer_amount_bounds->lry - xfer_amount_bounds->uly;
        xfer_amount_background = new (std::nothrow) Image(ulx, uly, width, height);
    } else {
        xfer_amount_background = nullptr;
    }

    ulx = xfer_slider_bounds->ulx;
    uly = xfer_slider_bounds->uly;
    width = xfer_slider_bounds->lrx - xfer_slider_bounds->ulx;
    height = xfer_slider_bounds->lry - xfer_slider_bounds->uly;
    xfer_slider = new (std::nothrow) Image(ulx, uly, width, height);

    if (vertical) {
        ulx = xfer_slider_bounds->ulx;
        uly = xfer_slider_bounds->uly - 3;
        width = xfer_slider_bounds->lrx - xfer_slider_bounds->ulx;
        height = xfer_slider_bounds->lry - xfer_slider_bounds->uly + 6;

        button_slider = new (std::nothrow) Button(ulx, uly, width, height);
    } else {
        ulx = xfer_slider_bounds->ulx - 3;
        uly = xfer_slider_bounds->uly;
        width = xfer_slider_bounds->lrx - xfer_slider_bounds->ulx + 6;
        height = xfer_slider_bounds->lry - xfer_slider_bounds->uly;

        button_slider = new (std::nothrow) Button(ulx, uly, width, height);
    }

    button_slider->SetPValue(key_code_click_slider);
}

Scrollbar::~Scrollbar() {
    delete xfer_slider;
    delete xfer_amount_background;
    delete button_slider;
}

void Scrollbar::SetValue(unsigned short value) { this->value = value; }

void Scrollbar::SetZeroOffset(short offset) { zero_offset = offset; }

void Scrollbar::SetFreeCapacity(short free_capacity) { this->free_capacity = free_capacity; }

void Scrollbar::SetMaterialBar(ResourceID id) { material_bar = id; }

short Scrollbar::GetValue() const { return value; }

void Scrollbar::Register() {
    WindowInfo win;

    window->FillWindowInfo(&win);

    if (xfer_amount_background) {
        xfer_amount_background->Copy(&win);
    }

    xfer_slider->Copy(&win);
    button_slider->RegisterButton(win.id);
}

void Scrollbar::RefreshScreen() {
    WindowInfo win;
    int capacity;
    unsigned char* buffer;

    window->FillWindowInfo(&win);

    if (xfer_amount_background) {
        xfer_amount_background->Write(&win);
    }

    xfer_slider->Write(&win);

    capacity = free_capacity - zero_offset;

    buffer = &win.buffer[xfer_slider->GetULX() + xfer_slider->GetULY() * win.width];

    if (scrollbar_type) {
        if (capacity) {
            capacity = ((value - zero_offset) * xfer_slider->GetHeight()) / capacity;
        }

        buffer = &buffer[win.width * (xfer_slider->GetHeight() - capacity)];
        LoadVerticalBar(buffer, win.width, capacity, xfer_slider->GetWidth(), material_bar);
    } else {
        if (capacity) {
            capacity = ((value - zero_offset) * xfer_slider->GetWidth()) / capacity;
        }

        LoadHorizontalBar(buffer, win.width, capacity, xfer_slider->GetHeight(), material_bar);
    }

    if (scaling_factor) {
        char text[20];
        Rect r;

        sprintf(text, "%i", scaling_factor * value);

        buffer = &win.buffer[xfer_amount_background->GetULX() + xfer_amount_background->GetULY() * win.width];

        buffer = &buffer[((xfer_amount_background->GetHeight() - text_height()) / 2) * win.width];
        buffer = &buffer[(xfer_amount_background->GetWidth() - text_width(text)) / 2];
        text_to_buf(buffer, text, text_width(text), win.width, 255);

        r.ulx = xfer_slider->GetULX();
        r.uly = xfer_slider->GetULY();
        r.lrx = xfer_slider->GetULX() + xfer_slider->GetWidth();
        r.lry = xfer_slider->GetULY() + xfer_slider->GetHeight();

        win_draw_rect(win.id, &r);

        if (xfer_amount_background) {
            r.ulx = xfer_amount_background->GetULX();
            r.uly = xfer_amount_background->GetULY();
            r.lrx = xfer_amount_background->GetULX() + xfer_slider->GetWidth();
            r.lry = xfer_amount_background->GetULY() + xfer_slider->GetHeight();

            win_draw_rect(win.id, &r);
        }
    }
}

bool Scrollbar::ProcessKey(int key_code) {
    bool result;

    if (key_code == key_code_increase) {
        if (value < free_capacity) {
            ProcessValueChange(value + 1);
            result = true;
        }
    } else if (key_code == key_code_decrease) {
        if (value > zero_offset) {
            ProcessValueChange(value - 1);
            result = true;
        }
    } else if (key_code == button_p_value) {
        int x;
        int y;
        int capacity;
        short new_value;

        window->GetCursorPosition(x, y);
        capacity = free_capacity - zero_offset;

        if (scrollbar_type) {
            y = xfer_slider->GetULY() + xfer_slider->GetHeight() - y;
            new_value = (xfer_slider->GetHeight() / 2 + capacity * y) / xfer_slider->GetHeight() + zero_offset;
        } else {
            x = x - xfer_slider->GetULX();
            new_value = (xfer_slider->GetWidth() / 2 + capacity * x) / xfer_slider->GetWidth() + zero_offset;
        }

        new_value = std::max(new_value, zero_offset);
        new_value = std::min(new_value, free_capacity);

        ProcessValueChange(new_value);
        result = true;
    } else {
        result = false;
    }

    return result;
}

LimitedScrollbar::LimitedScrollbar(Window* window, Rect* xfer_slider_bounds, Rect* xfer_amount_bounds, ResourceID id,
                                   int key_code_increase, int key_code_decrease, int key_code_click_slider,
                                   short scaling_factor, bool vertical)
    : Scrollbar(window, xfer_slider_bounds, xfer_amount_bounds, id, key_code_increase, key_code_decrease,
                key_code_click_slider, scaling_factor, vertical),
      xfer_give_max(0),
      xfer_take_max(36864) {}

LimitedScrollbar::~LimitedScrollbar() {}

void LimitedScrollbar::SetXferGiveMax(unsigned short limit) { xfer_give_max = limit; }

void LimitedScrollbar::SetXferTakeMax(unsigned short limit) { xfer_take_max = limit; }

bool LimitedScrollbar::ProcessKey(int key_code) {
    bool result;

    if (key_code == key_code_increase) {
        if (value >= xfer_give_max) {
            result = true;
        }
    }

    if (key_code == key_code_decrease) {
        if (value <= xfer_take_max) {
            result = true;
        }
    }

    if (key_code == button_p_value) {
        int x;
        int y;
        int capacity;
        short new_value;

        window->GetCursorPosition(x, y);
        capacity = free_capacity - zero_offset;

        if (scrollbar_type) {
            y = xfer_slider->GetULY() + xfer_slider->GetHeight() - y;
            new_value = (xfer_slider->GetHeight() / 2 + capacity * y) / xfer_slider->GetHeight() + zero_offset;
        } else {
            x = x - xfer_slider->GetULX();
            new_value = (xfer_slider->GetWidth() / 2 + capacity * x) / xfer_slider->GetWidth() + zero_offset;
        }

        new_value = std::max(new_value, zero_offset);
        new_value = std::min(new_value, free_capacity);

        if (new_value > xfer_give_max) {
            new_value = xfer_give_max;
        }

        if (new_value > xfer_take_max) {
            new_value = xfer_take_max;
        }

        ProcessValueChange(new_value);
        result = true;
    } else {
        result = Scrollbar::ProcessKey(key_code);
    }

    return result;
}
