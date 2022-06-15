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

#ifndef REPORTSTATS_HPP
#define REPORTSTATS_HPP

#include "gnw.h"
#include "unitinfo.hpp"

extern const ResourceID ReportStats_CargoIcons[];

void ReportStats_DrawListItemIcon(unsigned char* buffer, int width, ResourceID unit_type, unsigned short team, int ulx,
                                  int uly);

void ReportStats_DrawListItem(unsigned char* buffer, int width, ResourceID unit_type, int ulx, int uly, int full,
                              int color);

void ReportStats_DrawNumber(unsigned char* buffer, int number, int width, int full, int color);

void ReportStats_DrawText(unsigned char* buffer, char* text, int width, int full, int color);

void ReportStats_DrawRow(const char* text, WinID id, Rect* bounds, ResourceID icon_normal, ResourceID icon_empty,
                         int current_value, int base_value, int value3, bool drawline);

void ReportStats_Draw(UnitInfo* unit, WinID id, Rect* bounds);

#endif /* REPORTSTATS_HPP */
