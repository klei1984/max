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

#ifndef MAPHASHLIST_HPP
#define MAPHASHLIST_HPP

#include "point.hpp"
#include "tasks.hpp"
#include "unitinfo.hpp"

class MapHashList : public SmartObject {
    SmartPointer<UnitInfo> unit;
    SmartPointer<Task> task;
    unsigned short team;
    Point point;
    bool visible;

public:
    MapHashList(UnitInfo& unit, unsigned short team);
    MapHashList(SmartFileReader& file);
    virtual ~MapHashList();

    void FileSave(SmartFileWriter& file);
    Task* GetTask();
    void SetTask(Task* task);
    Point GetPosition();
    void SetPosition(Point point);
    short GetPositionX();
    short GetPositionY();
    void UpdatePosition();
    void UpdateVisibility();
};

#endif /* MAPHASHLIST_HPP */
