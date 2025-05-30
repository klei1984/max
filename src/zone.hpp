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

#ifndef ZONE_HPP
#define ZONE_HPP

#include "point.hpp"
#include "smartobjectarray.hpp"

class UnitInfo;
class Task;

extern "C" {
#include "gnw.h"
}

class Zone : public SmartObject {
    SmartPointer<Task> task;
    bool is_unimportant;

public:
    Zone(UnitInfo* unit, Task* task);
    Zone(UnitInfo* unit, Task* task, Rect* bounds);
    ~Zone();

    void Add(Point* point);
    void Add(Rect* bounds);
    void Finished(const bool status);
    bool IsImportant() const;
    void SetImportance(bool value);

    ObjectArray<Point> points;
    SmartPointer<UnitInfo> unit;
};

struct ZoneSquare {
    Point point;
    UnitInfo* unit;

    ZoneSquare();
    ZoneSquare(const int32_t grid_x, const int32_t grid_y, UnitInfo* const unit);
};

#endif /* ZONE_HPP */
