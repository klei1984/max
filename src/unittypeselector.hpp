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

#ifndef UNITTYPESELECTOR_HPP
#define UNITTYPESELECTOR_HPP

#include "button.hpp"
#include "buttonmanager.hpp"
#include "events.hpp"
#include "smartobjectarray.hpp"
#include "window.hpp"

class UnitTypeSelector;

class EventUnitSelect : public Event {
    UnitTypeSelector *selector;
    unsigned char value;

public:
    EventUnitSelect(UnitTypeSelector *selector, short value);
    unsigned short GetEventId() const;
    UnitTypeSelector *GetSelector() const;
    bool GetValue() const;
};

EVENTS_DECLARE_EVENT_ID(UnitSelectEvent);

class UnitTypeSelector {
protected:
    Window *window;
    WindowInfo window_info;
    SmartObjectArray<ResourceID> unit_types;
    unsigned short team;
    short page_min_index;
    short page_max_index;
    short max_item_count;
    struct ImageSimpleHeader *image;
    int key_code;
    ButtonManager button_group;
    Button *button_scroll_up;
    Button *button_scroll_down;

public:
    UnitTypeSelector(Window *window, WindowInfo *window_info, SmartObjectArray<ResourceID> unit_types,
                     unsigned short team, int key_code, Button *button_scroll_up, Button *button_scroll_down);
    virtual ~UnitTypeSelector();

    ResourceID GetLast();
    void ScrollTo(int index);
    void AddItems(SmartObjectArray<ResourceID> unit_types);
    void Select(unsigned char value);
    void PushBack(ResourceID unit_type);
    int GetPageMaxIndex() const;

    virtual void Add(ResourceID unit_type, int position);
    virtual void RemoveLast();
    virtual bool ProcessKeys(int key_press);
    virtual void Draw();
};

#endif /* UNITTYPESELECTOR_HPP */
