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

#include "zone.hpp"

#include "task.hpp"
#include "unitinfo.hpp"

Zone::Zone(UnitInfo* unit, Task* task) : unit(unit), task(task), field_30(false) {}

Zone::Zone(UnitInfo* unit, Task* task, Rect* bounds) : unit(unit), task(task), field_30(false) { Add(bounds); }

Zone::~Zone() {}

void Zone::Add(Point* point) { points.Append(point); }

void Zone::Add(Rect* bounds) {
    Point point;

    for (point.x = bounds->ulx; point.x < bounds->lrx; ++point.x) {
        for (point.y = bounds->uly; point.y < bounds->lry; ++point.y) {
            points.Append(&point);
        }
    }
}

void Zone::CallTaskVfunc27(bool mode) { task->EventZoneCleared(this, mode); }

bool Zone::GetField30() const { return field_30; }

void Zone::SetField30(bool value) { field_30 = value; }

ZoneSquare::ZoneSquare() : point(0, 0), unit(nullptr) {}

ZoneSquare::ZoneSquare(int32_t grid_x, int32_t grid_y, UnitInfo* unit) : point(grid_x, grid_y), unit(unit) {}

ZoneSquare::ZoneSquare(const ZoneSquare& other) : point(other.point), unit(other.unit) {}
