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

#ifndef TASKABSTRACTSEARCH_HPP
#define TASKABSTRACTSEARCH_HPP

#include "task.hpp"

class TaskAbstractSearch : public Task {
protected:
    Point point;
    SmartList<UnitInfo> units;
    short requestors;

    void FindDestination(UnitInfo& unit, int radius);

public:
    TaskAbstractSearch(unsigned short team, Task* task, unsigned short flags, Point point);
    ~TaskAbstractSearch();

    Point GetPoint() const;

    void AddUnit(UnitInfo& unit);
    void BeginTurn();
    void ChildComplete(Task* task);
    void EndTurn();
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);

    virtual void TaskAbstractSearch_vfunc28(UnitInfo& unit);
    virtual bool IsVisited(UnitInfo& unit, Point point) = 0;
    virtual void ObtainUnit() = 0;
};

#endif /* TASKABSTRACTSEARCH_HPP */
