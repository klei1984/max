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

#include "unittypeselector.hpp"

#include "events.hpp"
#include "game_manager.hpp"
#include "gui.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"

EVENTS_REGISTER_EVENT(UnitSelectEvent);

class EventUnitSelect : public Event {
    UnitTypeSelector* selector;
    unsigned char value;

public:
    EventUnitSelect(UnitTypeSelector* selector, short value) : selector(selector), value(value) {}

    unsigned short GetEventId() { return EVENTS_GET_EVENT_ID(UnitSelectEvent); }
};

UnitTypeSelector::UnitTypeSelector(Window* window, WindowInfo* window_info, SmartObjectArray<ResourceID> unit_types,
                                   unsigned short team, int key_code, Button* button_scroll_up,
                                   Button* button_scroll_down)
    : window(window),
      window_info(*window_info),
      unit_types(unit_types),
      key_code(key_code),
      button_scroll_up(button_scroll_up),
      button_scroll_down(button_scroll_down),
      team(team),
      page_min_index(0),
      page_max_index(0) {
    int width;
    int height;

    button_scroll_up->SetRValue(this->key_code);
    button_scroll_up->SetPValue(GNW_INPUT_PRESS);
    button_scroll_up->SetSfx(KCARG0);
    button_scroll_up->RegisterButton(this->window_info.id);

    button_scroll_down->SetRValue(this->key_code + 1);
    button_scroll_down->SetPValue(GNW_INPUT_PRESS + 1);
    button_scroll_down->SetSfx(KCARG0);
    button_scroll_down->RegisterButton(this->window_info.id);

    max_item_count = (this->window_info.window.lry - this->window_info.window.uly) / 32;

    width = this->window_info.window.lrx - this->window_info.window.ulx;
    height = max_item_count * 32;

    for (int i = 0; i < max_item_count; ++i) {
        button_group.Add(win_register_button(this->window_info.id, this->window_info.window.ulx,
                                             this->window_info.window.uly + i * 32, width, 32, -1, -1, -1,
                                             this->key_code + 2 + i, nullptr, nullptr, nullptr, 0x00));
    }

    image = reinterpret_cast<struct ImageSimpleHeader*>(malloc(
        width * height + sizeof(image->ulx) + sizeof(image->uly) + sizeof(image->width) + sizeof(image->height)));

    image->ulx = width;
    image->uly = height;
    image->width = 0;
    image->height = 0;

    buf_to_buf(this->window_info.buffer, width, height, this->window_info.width, image->data, image->ulx);
}

UnitTypeSelector::~UnitTypeSelector() { free(image); }

ResourceID UnitTypeSelector::GetLast() {
    ResourceID unit_type;

    if (unit_types.GetCount()) {
        unit_type = *unit_types[page_max_index];
    } else {
        unit_type = INVALID_ID;
    }

    return unit_type;
}

void UnitTypeSelector::ScrollTo(int index) {
    if (index < page_min_index) {
        page_min_index = index;
    }

    if ((page_min_index + max_item_count) <= index) {
        page_min_index = index - max_item_count + 1;
    }

    if ((page_min_index + max_item_count) > unit_types.GetCount()) {
        page_min_index = unit_types.GetCount() - max_item_count;
    }

    if (page_min_index < 0) {
        page_min_index = 0;
    }
}

void UnitTypeSelector::AddItems(SmartObjectArray<ResourceID> array) {
    ResourceID unit_type;

    unit_type = GetLast();
    page_max_index = (*array).Find(&unit_type);

    if (page_max_index < 0) {
        page_max_index = 0;
    }

    unit_types = array;
    ScrollTo(page_max_index);
    Draw();

    if (!unit_types.GetCount() || GetLast() != unit_type) {
        Select(0);
    }
}

void UnitTypeSelector::Add(ResourceID unit_type, int position) {
    if (unit_types.GetCount() < position) {
        position = unit_types.GetCount();
    }

    page_max_index = position;
    unit_types.Insert(&unit_type, position);
    Select(0);
    ScrollTo(page_max_index);
    Draw();
}

void UnitTypeSelector::RemoveLast() {
    if (unit_types.GetCount()) {
        unit_types.Remove(page_max_index);

        if (unit_types.GetCount() <= page_max_index) {
            page_max_index = unit_types.GetCount() - 1;
        }

        if (page_max_index < 0) {
            page_max_index = 0;
        }

        Select(0);
        ScrollTo(page_max_index);
        Draw();
    }
}

bool UnitTypeSelector::ProcessKeys(int key_press) {
    bool result;

    if ((key_code + 2) <= key_press && (key_code + 2 + max_item_count) > key_press) {
        int position;

        position = key_press + page_min_index - 2 - key_code;

        if (unit_types.GetCount() > position) {
            if (page_max_index == position) {
                Select(1);
            } else {
                page_max_index = position;
                Draw();
                Select(0);
            }
        }

        result = true;
    } else if (key_press == GNW_KB_KEY_UP) {
        if (page_max_index > 0) {
            --page_max_index;

            if (page_max_index < page_min_index) {
                page_min_index = page_max_index;
            }

            Draw();
            Select(0);
        }

        result = true;
    } else if (key_press == GNW_KB_KEY_DOWN) {
        if (page_max_index < unit_types.GetCount() - 1) {
            ++page_max_index;

            if (page_max_index > (page_min_index + max_item_count - 1)) {
                page_min_index = page_max_index - max_item_count + 1;
            }

            Draw();
            Select(0);
        }

        result = true;
    } else if (key_press == GNW_KB_KEY_PAGEDOWN || key_code + 1 == key_press) {
        int time_stamp;

        for (int i = 0; i < max_item_count; ++i) {
            time_stamp = timer_get_stamp32();
            ++page_min_index;

            if ((page_min_index + page_max_index) > unit_types.GetCount()) {
                --page_min_index;
                break;
            }

            Draw();

            while ((timer_get_stamp32() - time_stamp) < TIMER_FPS_TO_TICKS(48)) {
            }
        }

        result = true;
    } else if (key_press == GNW_KB_KEY_PAGEUP || key_code == key_press) {
        int time_stamp;

        for (int i = 0; i < max_item_count && page_min_index > 0; ++i) {
            time_stamp = timer_get_stamp32();
            --page_min_index;

            Draw();

            while ((timer_get_stamp32() - time_stamp) < TIMER_FPS_TO_TICKS(48)) {
            }
        }

        result = true;
    } else if (key_press == GNW_INPUT_PRESS || key_press == GNW_INPUT_PRESS + 1) {
        button_scroll_up->PlaySound();
        result = true;

    } else {
        result = false;
    }

    return result;
}

void UnitTypeSelector::Select(unsigned char value) {
    EventUnitSelect event(this, value);
    soundmgr.PlaySfx(KCARG0);
    window->EventHandler(event);
}

void UnitTypeSelector::PushBack(ResourceID unit_type) { Add(unit_type, unit_types.GetCount()); }

void UnitList_DrawIcon(unsigned char* buffer, int width, ResourceID unit_type, unsigned short team, int ulx, int uly) {
    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        /// \todo Implement missing stuff
    }
}

void UnitList_DrawItem(unsigned char* buffer, int width, ResourceID unit_type, int ulx, int uly, int full, int color) {
    UnitList_DrawIcon(buffer, width, unit_type, GUI_PlayerTeamIndex, ulx + 16, uly + 16);
    Text_TextBox(buffer, width, UnitsManager_BaseUnits[unit_type].singular_name, ulx + 35, uly, full - 35, 32, color,
                 false);
}

void UnitTypeSelector::Draw() {
    int width;

    width = window_info.window.lrx - window_info.window.ulx;

    buf_to_buf(image->data, image->ulx, image->uly, image->ulx, window_info.buffer, window_info.width);
    text_font(5);

    for (int i = 0; i < max_item_count && i + page_min_index < unit_types.GetCount(); ++i) {
        UnitList_DrawItem(window_info.buffer, window_info.width, *unit_types[page_min_index + i], 0, i * 32, width,
                          0xA2);
    }

    if ((page_max_index >= page_min_index) && (page_max_index < (page_min_index + max_item_count))) {
        int height;
        unsigned int flags;

        height = (page_max_index - page_min_index) * 32;
        flags = UnitsManager_BaseUnits[*unit_types[page_max_index]].flags;

        GameManager_DrawUnitSelector(window_info.buffer, window_info.width, 0, height, 0, height, 32, height + 32,
                                     flags & BUILDING ? 0x40000 : 0x20000, flags & BUILDING);
    }

    if (page_min_index > 0) {
        button_scroll_up->Enable();
    } else {
        button_scroll_up->Disable();
    }

    if ((page_min_index + max_item_count) < unit_types.GetCount()) {
        button_scroll_down->Enable();
    } else {
        button_scroll_down->Disable();
    }

    win_draw_rect(window_info.id, &window_info.window);
}
