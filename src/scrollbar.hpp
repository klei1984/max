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
    Scrollbar* scrollbar;
    int16_t value;

public:
    EventScrollbarChange(Scrollbar* scrollbar, int16_t value);
    uint16_t GetEventId() const;

    int16_t GetValue() const;
    int16_t GetScrollbarValue() const;
};

EVENTS_DECLARE_EVENT_ID(ScrollbarEvent);

void LoadHorizontalBar(uint8_t* buffer, int32_t width, int32_t capacity, int32_t height, ResourceID id);
void LoadHorizontalTape(uint8_t* buffer, int32_t full2, int32_t length, int32_t width, ResourceID id);
void LoadVerticalBar(uint8_t* buffer, int32_t width, int32_t capacity, int32_t height, ResourceID id);

class Scrollbar {
protected:
    int16_t free_capacity;
    int16_t zero_offset;
    int16_t value;
    Image* xfer_slider;
    Image* xfer_amount_background;
    ResourceID material_bar;
    int32_t key_code_increase;
    int32_t key_code_decrease;
    int32_t button_p_value;
    Button* button_slider;
    Window* window;
    uint16_t scaling_factor;
    bool scrollbar_type;

    friend void LoadHorizontalBar(uint8_t* buffer, int32_t width, int32_t capacity, int32_t height, ResourceID id);
    friend void LoadVerticalBar(uint8_t* buffer, int32_t width, int32_t capacity, int32_t height, ResourceID id);
    void ProcessValueChange(int16_t value);

public:
    Scrollbar(Window* window, Rect* xfer_slider_bounds, Rect* xfer_amount_bounds, ResourceID id,
              int32_t key_code_increase, int32_t key_code_decrease, int32_t key_code_click_slider,
              int16_t scaling_factor, bool vertical = false);
    virtual ~Scrollbar();

    void SetValue(uint16_t value);
    void SetZeroOffset(int16_t offset);
    void SetFreeCapacity(int16_t free_capacity);
    void SetMaterialBar(ResourceID id);

    int16_t GetValue() const;

    virtual void Register();
    virtual void RefreshScreen();
    virtual bool ProcessKey(int32_t key_code);
};

class LimitedScrollbar : public Scrollbar {
    int16_t xfer_give_max;
    int16_t xfer_take_max;

public:
    LimitedScrollbar(Window* window, Rect* xfer_slider_bounds, Rect* xfer_amount_bounds, ResourceID id,
                     int32_t key_code_increase, int32_t key_code_decrease, int32_t key_code_click_slider,
                     int16_t scaling_factor, bool vertical = false);
    ~LimitedScrollbar();

    void SetXferGiveMax(int16_t limit);
    void SetXferTakeMax(int16_t limit);

    bool ProcessKey(int32_t key_code);
};

#endif /* SCROLLBAR_HPP */
