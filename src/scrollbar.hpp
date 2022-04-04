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

#ifndef SCROLLBAR_HPP
#define SCROLLBAR_HPP

#include "button.hpp"
#include "events.hpp"
#include "window.hpp"

class Scrollbar;

class EventScrollbarChange : public Event {
    Scrollbar *scrollbar;
    short value;

public:
    EventScrollbarChange(Scrollbar *scrollbar, short value);
    unsigned short GetEventId() const;
};

EVENTS_DECLARE_EVENT_ID(ScrollbarEvent);

void LoadHorizontalBar(unsigned char *buffer, short width, short capacity, short height, ResourceID id);
void LoadVerticalBar(unsigned char *buffer, short width, short capacity, short height, ResourceID id);

class Scrollbar {
protected:
    short free_capacity;
    short zero_offset;
    short value;
    Image *xfer_slider;
    Image *xfer_amount_background;
    ResourceID material_bar;
    unsigned int key_code_increase;
    unsigned int key_code_decrease;
    unsigned int button_p_value;
    Button *button_slider;
    Window *window;
    unsigned short scaling_factor;
    bool scrollbar_type;

    friend void LoadHorizontalBar(unsigned char *buffer, short width, short capacity, short height, ResourceID id);
    friend void LoadVerticalBar(unsigned char *buffer, short width, short capacity, short height, ResourceID id);
    void ProcessValueChange(short value);

public:
    Scrollbar(Window *window, Rect *xfer_slider_bounds, Rect *xfer_amount_bounds, ResourceID id, int key_code_increase,
              int key_code_decrease, int key_code_click_slider, short scaling_factor, bool vertical = false);
    virtual ~Scrollbar();

    void SetValue(unsigned short value);
    void SetZeroOffset(short offset);
    void SetFreeCapacity(short free_capacity);
    void SetMaterialBar(ResourceID id);

    short GetValue() const;

    virtual void Register();
    virtual void RefreshScreen();
    virtual bool ProcessKey(int key_code);
};

class LimitedScrollbar : public Scrollbar {
    unsigned short xfer_give_max;
    unsigned short xfer_take_max;

public:
    LimitedScrollbar(Window *window, Rect *xfer_slider_bounds, Rect *xfer_amount_bounds, ResourceID id,
                     int key_code_increase, int key_code_decrease, int key_code_click_slider, short scaling_factor,
                     bool vertical = false);
    ~LimitedScrollbar();

    void SetXferGiveMax(unsigned short limit);
    void SetXferTakeMax(unsigned short limit);

    bool ProcessKey(int key_code);
};

#endif /* SCROLLBAR_HPP */
