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
    SmartPointer<UnitInfo> unit1;
    SmartPointer<UnitInfo> unit2;
    SmartPointer<UnitInfo> unit3;

    void TaskGetResource_sub_46DF3();
    void TaskGetResource_sub_471A4(int unknown, char mode);

protected:
    UnitInfo* TaskGetResource_sub_46D29(Complex* complex);
    void TaskGetResource_sub_471F8();

public:
    TaskGetResource(Task* task, UnitInfo& unit);
    ~TaskGetResource();

    virtual void AddReminder();
    virtual void EndTurn();
    virtual void RemoveSelf();
    virtual void Remove(UnitInfo& unit);

    virtual void Task_vfunc28() = 0;
    virtual UnitInfo* Task_vfunc29() = 0;
    virtual void Task_vfunc30() = 0;
};

#endif /* TASKGETRESOURCE_HPP */
