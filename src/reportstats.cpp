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
#include "unitstats.hpp"

struct InterfaceMeta {
    unsigned short divisor;
    ColorIndex* color_index_table;
    unsigned short brightness;
    ResourceID icon;
    unsigned char* buffer;
    unsigned short width;
    Rect bounds;
    unsigned short ulx;
    unsigned short uly;
    unsigned short frame_index;
};

static void UnitList_RenderSprite(struct InterfaceMeta* data);
static void ReportStats_DrawIcons(WindowInfo* window, ResourceID icon_normal, ResourceID icon_empty, int value1,
                                  int value2);
static void ReportStats_DrawRowEx(char* text, WinID id, Rect* bounds, int row_id, ResourceID icon_normal,
                                  ResourceID icon_empty, int current_value, int base_value, int value3, bool drawline);

void UnitList_RenderSprite(struct InterfaceMeta* data) {
    /// \todo
}

void ReportStats_DrawIcons(WindowInfo* window, ResourceID icon_normal, ResourceID icon_empty, int value1, int value2) {
    struct ImageSimpleHeader* image;
    int width;
    int height;
    int scaled_height;
    int rounded_value2;
    int reminder_value2;
    int offset_value2;
    int var_48;
    int var_30;
    int var_4C;
    int var_18;
    int var_24;
    unsigned char* buffer;
    int offset;

    width = window->window.lrx - window->window.ulx;
    height = window->window.lry - window->window.uly;

    if (value1) {
        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(icon_normal));
    } else {
        image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(icon_empty));
    }

    if (image) {
        if (value2 >= 6) {
            scaled_height = (height - 1 - image->height) / 4;
        } else {
            scaled_height = 0;
        }

        buffer = &window->buffer[(((height - 1 - scaled_height * 4) - image->height) / 2) * window->width];
        rounded_value2 = (value2 + 4) / 5;
        reminder_value2 = value2 - (rounded_value2 - 1) * 5;
        width -= rounded_value2 * image->width;
        offset_value2 = rounded_value2 * 4 - (5 - reminder_value2);

        if (offset_value2) {
            if (width < offset_value2) {
                var_48 = 1;
                var_30 = (rounded_value2 + (offset_value2 - width) - 2) / (rounded_value2 - 1);
            } else {
                var_48 = width / offset_value2;

                if (var_48 > image->width) {
                    var_48 = image->width;
                    var_30 = 0;
                }
            }

        } else {
            var_48 = image->width;
            var_30 = 0;
        }

        var_4C = (var_48 * 4 + image->width) - var_30;
        offset = 0;

        for (int i = 0; i < value2; ++i) {
            var_18 = i % 5;
            offset = (i / 5) * var_4C + var_18 * var_48;
            var_24 = var_18 * scaled_height;

            UnitStats_DrawImage(&buffer[offset + var_24 * window->width], window->width, image);
            --value1;

            if (!value1) {
                image = reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(icon_empty));
            }
        }
    }
}

void ReportStats_DrawRowEx(char* text, WinID id, Rect* bounds, int row_id, ResourceID icon_normal,
                           ResourceID icon_empty, int current_value, int base_value, int value3, bool drawline) {
    Rect new_bounds;
    int height;

    height = bounds->lry - bounds->uly;

    new_bounds.ulx = bounds->ulx;
    new_bounds.lrx = bounds->lrx;

    new_bounds.uly = ((height * row_id) / 4) + bounds->uly + 1;
    new_bounds.lry = ((height * (row_id + 1)) / 4) + bounds->uly - 1;

    ReportStats_DrawRow(text, id, &new_bounds, icon_normal, icon_empty, current_value, base_value, value3, drawline);
}

void ReportStats_DrawRow(char* text, WinID id, Rect* bounds, ResourceID icon_normal, ResourceID icon_empty,
                         int current_value, int base_value, int value3, bool drawline) {
    WindowInfo window;
    int width;
    int height;
    int color;
    int scaling_factor;
    char string[10];

    width = bounds->lrx - bounds->ulx;
    height = bounds->lry - bounds->uly;

    window.id = id;
    window.width = win_width(id);
    window.buffer = &win_get_buf(id)[window.width * bounds->uly + bounds->ulx];
    window.window = *bounds;

    if (drawline) {
        draw_line(window.buffer, window.width, 0, height, width - 1, height, 0x3C);
    }

    if (base_value) {
        current_value = std::min(current_value, base_value);

        if (current_value > base_value / 4) {
            if (current_value > base_value / 2) {
                color = 0x2;

            } else {
                if (icon_normal != SI_HITSB) {
                    icon_normal = SI_HITSY;
                    icon_empty = EI_HITSY;
                }

                color = 0x4;
            }

        } else {
            if (icon_normal != SI_HITSB) {
                icon_normal = SI_HITSR;
                icon_empty = EI_HITSR;
            }

            color = 0x1;
        }

        text_font(2);
        sprintf(string, "%i/%i", current_value, base_value);

        if (icon_normal == SI_FUEL || icon_normal == SI_GOLD) {
            scaling_factor = 20;
        } else {
            scaling_factor = 25;
        }

        if (((base_value + value3 - 1) / value3) > scaling_factor) {
            value3 = (base_value + scaling_factor - 1) / value3;
        }

        Text_TextBox(window.buffer, window.width, string, 0, 0, 45, height, color, true);

        base_value = (base_value + value3 - 1) / value3;

        Text_TextBox(window.buffer, window.width, text, 45, 0, 30, height, 0xA2);

        window.buffer = &window.buffer[75];
        window.window.ulx += 75;

        ReportStats_DrawIcons(&window, icon_normal, icon_empty, current_value, base_value);
    }
}

void ReportStats_DrawListItemIcon(unsigned char* buffer, int width, ResourceID unit_type, unsigned short team, int ulx,
                                  int uly) {
    InterfaceMeta data;

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        data.divisor = 2;
    } else {
        data.divisor = 1;
    }

    data.buffer = buffer;
    data.width = width;
    data.frame_index = 0;
    data.brightness = 0xFF;
    data.color_index_table = UnitsManager_TeamInfo[team].team_units->color_index_table;
    data.icon = UnitsManager_BaseUnits[unit_type].icon;
    data.ulx = ulx;
    data.uly = uly;

    data.bounds.ulx = ulx - 16;
    data.bounds.uly = uly - 16;
    data.bounds.lrx = data.bounds.ulx + 32;
    data.bounds.lry = data.bounds.uly + 32;

    UnitList_RenderSprite(&data);

    if (UnitsManager_BaseUnits[unit_type].flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        struct BaseUnitDataFile* data_file =
            reinterpret_cast<struct BaseUnitDataFile*>(UnitsManager_BaseUnits[unit_type].data_buffer);

        data.ulx += data_file->angle_offsets[2].x >> data.divisor;
        data.uly += data_file->angle_offsets[2].y >> data.divisor;
        ++data.frame_index;

        UnitList_RenderSprite(&data);
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
