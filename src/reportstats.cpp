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

#include "reportstats.hpp"

#include "gui.hpp"
#include "text.hpp"
#include "units_manager.hpp"

void ReportStats_DrawListItemIcon(unsigned char* buffer, int width, ResourceID unit_type, unsigned short team, int ulx,
                                  int uly) {
    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        /// \todo Implement missing stuff
    }
}

void ReportStats_DrawListItem(unsigned char* buffer, int width, ResourceID unit_type, int ulx, int uly, int full,
                              int color) {
    ReportStats_DrawListItemIcon(buffer, width, unit_type, GUI_PlayerTeamIndex, ulx + 16, uly + 16);
    Text_TextBox(buffer, width, UnitsManager_BaseUnits[unit_type].singular_name, ulx + 35, uly, full - 35, 32, color,
                 false);
}

void ReportStats_DrawNumber(unsigned char* buffer, int number, int width, int full, int color) {
    char text_buffer[10];

    snprintf(text_buffer, sizeof(text_buffer), "%i", number);
    ReportStats_DrawText(buffer, text_buffer, width, full, color);
}

void ReportStats_DrawText(unsigned char* buffer, char* text, int width, int full, int color) {
    int length;

    length = text_width(text);

    if (length > width) {
        length = width;
    }

    text_to_buf(&buffer[-length], text, width, full, color);
}
