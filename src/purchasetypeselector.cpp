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

#include "purchasetypeselector.hpp"

#include "cargo.hpp"
#include "reportstats.hpp"
#include "units_manager.hpp"

PurchaseTypeSelector::PurchaseTypeSelector(Window *window, WindowInfo *window_info,
                                           SmartObjectArray<ResourceID> unit_types, uint16_t team, int32_t key_code,
                                           Button *button_scroll_up, Button *button_scroll_down, int32_t ulx,
                                           int32_t width)
    : UnitTypeSelector(window, window_info, unit_types, team, key_code, button_scroll_up, button_scroll_down) {
    image = new (std::nothrow) Image(ulx, 0, width, max_item_count * 32);
    image->Copy(window_info);
}

PurchaseTypeSelector::~PurchaseTypeSelector() { delete image; }

void PurchaseTypeSelector::Draw() {
    int32_t turns;
    int32_t cost;
    Rect bounds;

    UnitTypeSelector::Draw();

    image->Write(&window_info);

    Text_SetFont(GNW_TEXT_FONT_5);

    for (uint32_t i = 0; i < max_item_count && i < unit_types->GetCount(); ++i) {
        turns = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], *unit_types[page_min_index + i])
                    ->GetAttribute(ATTRIB_TURNS);

        if (UnitsManager_BaseUnits[*unit_types[page_min_index + i]].flags & STATIONARY) {
            cost = Cargo_GetRawConsumptionRate(CONSTRCT, 1) * turns;
        } else {
            cost = Cargo_GetRawConsumptionRate(LANDPLT, 1) * turns;
        }

        ReportStats_DrawNumber(&window_info.buffer[(i * 32 + 16 - Text_GetHeight() / 2) * window_info.width +
                                                   image->GetULX() + image->GetWidth()],
                               cost, image->GetWidth(), window_info.width, COLOR_YELLOW);
    }

    bounds.ulx = window_info.window.ulx + image->GetULX();
    bounds.uly = window_info.window.uly;
    bounds.lrx = bounds.ulx + image->GetWidth();
    bounds.lry = bounds.uly + image->GetHeight();

    win_draw_rect(window_info.id, &bounds);
}
