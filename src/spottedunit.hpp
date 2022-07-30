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

#ifndef SPOTTEDUNIT_HPP
#define SPOTTEDUNIT_HPP

#include "unitinfo.hpp"

class SpottedUnit : public SmartObject {
    SmartPointer<UnitInfo> unit;
    SmartPointer<Task> task;
    unsigned short team;
    Point last_position;
    bool visible_to_team;

public:
    SpottedUnit(UnitInfo* unit, unsigned short team);
    SpottedUnit(SmartFileReader& file);
    ~SpottedUnit();

    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);

    Task* GetTask() const;
    void SetTask(Task* task);

    Point GetLastPosition();
    int GetLastPositionX();
    int GetLastPositionY();
    void SetPosition(Point position);
    void UpdatePosition();
    void UpdatePositionIfVisible();

    UnitInfo* GetUnit() const;

    unsigned short GetTeam() const;
};

#endif /* SPOTTEDUNIT_HPP */
