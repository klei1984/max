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

#include "cargoselector.hpp"

#include "cargomenu.hpp"
#include "game_manager.hpp"
#include "reportstats.hpp"
#include "smartstring.hpp"
#include "text.hpp"
#include "units_manager.hpp"

CargoSelector::CargoSelector(Window* window, WindowInfo* window_info, SmartObjectArray<ResourceID> unit_types,
                             SmartObjectArray<unsigned short> cargos, unsigned short team, int key_code,
                             Button* button_scroll_up, Button* button_scroll_down)
    : UnitTypeSelector(window, window_info, unit_types, team, key_code, button_scroll_up, button_scroll_down),
      cargos(cargos),
      cargomenu(window) {}

CargoSelector::~CargoSelector() {}

void CargoSelector::Add(ResourceID unit_type, int position) {
    unsigned short value;

    if (unit_types.GetCount() < position) {
        position = unit_types.GetCount();
    }

    value = 0;
    cargos.Insert(&value, position);

    UnitTypeSelector::Add(unit_type, position);
}

void CargoSelector::RemoveLast() {
    if (unit_types.GetCount()) {
        cargos.Remove(page_max_index);
        UnitTypeSelector::RemoveLast();
    }
}

void CargoSelector::Draw() {
    int width;
    char text[200];
    char text2[50];
    int index;
    int storage;
    int cargo;

    width = window_info.window.lrx - window_info.window.ulx;

    buf_to_buf(image->data, image->width, image->height, image->width, window_info.buffer, window_info.width);

    for (int i = 0; i < max_item_count && (page_min_index + i) < unit_types.GetCount(); ++i) {
        ResourceID unit_type;

        unit_type = *unit_types[page_min_index + i];

        ReportStats_DrawListItemIcon(window_info.buffer, window_info.width, unit_type, GameManager_PlayerTeam, 16,
                                     32 * i + 16);

        strcpy(text, UnitsManager_BaseUnits[unit_type].singular_name);

        index = 1;

        for (int j = 0; j < (page_min_index + i); ++j) {
            if (unit_type == *unit_types[j]) {
                ++index;
            }
        }

        {
            SmartString string;

            strcat(text, string.Sprintf(10, " %i", index).GetCStr());
        }

        storage = dynamic_cast<CargoMenu*>(window)->GetCurrentUnitValues(unit_type)->GetAttribute(ATTRIB_STORAGE);
        cargo = *cargos[page_min_index + i];

        if (storage > 0) {
            if (cargo) {
                if (storage > cargo) {
                    if (unit_type == FUELTRCK) {
                        sprintf(text2, "\n(%i Fuel)", cargo);
                    } else if (unit_type == GOLDTRCK) {
                        sprintf(text2, "\n(%i Gold)", cargo);
                    } else {
                        sprintf(text2, "\n(%i Mat.)", cargo);
                    }

                    strcat(text, text2);

                } else {
                    strcat(text, "\n(Full)");
                }

            } else {
                strcat(text, "\n(Empty)");
            }
        }

        text_font(5);

        Text_TextBox(window_info.buffer, window_info.width, text, 32, 32 * i, width - 32, 32, 0xA2, false);
    }

    if (page_max_index >= page_min_index && page_max_index < max_item_count + page_min_index) {
        BaseUnit* unit;
        int height;

        unit = &UnitsManager_BaseUnits[*unit_types[page_max_index]];
        height = 32 * (page_max_index - page_min_index);

        GameManager_DrawUnitSelector(window_info.buffer, window_info.width, 0, height, 0, height, 32, height + 32,
                                     unit->flags & BUILDING ? 0x40000 : 0x20000, unit->flags & BUILDING, true);
    }

    if (page_min_index > 0) {
        button_scroll_up->Enable();
    } else {
        button_scroll_up->Disable();
    }

    if ((page_min_index + max_item_count) < unit_types->GetCount()) {
        button_scroll_down->Enable();
    } else {
        button_scroll_down->Disable();
    }

    win_draw_rect(window_info.id, &window_info.window);
}
