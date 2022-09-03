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

#ifndef TASKGETRESOURCE_HPP
#define TASKGETRESOURCE_HPP

#include "task.hpp"

class TaskGetResource : public Task {
    void ChooseSource();
    static void RendezvousResultCallback(Task* task, UnitInfo* unit, char mode);

protected:
    SmartPointer<UnitInfo> requestor;
    SmartPointer<UnitInfo> supplier;
    SmartPointer<UnitInfo> source;

    UnitInfo* FindClosestBuilding(Complex* complex);
    void ReleaseSource();

public:
    TaskGetResource(Task* task, UnitInfo& unit);
    ~TaskGetResource();

    virtual void Begin();
    virtual void EndTurn();
    virtual void RemoveSelf();
    virtual void RemoveUnit(UnitInfo& unit);

    virtual void DoTransfer() = 0;
    virtual UnitInfo* FindBuilding() = 0;
    virtual void FindTruck() = 0;
};

#endif /* TASKGETRESOURCE_HPP */
