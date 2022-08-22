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

#ifndef TASKOBTAINUNITS_HPP
#define TASKOBTAINUNITS_HPP

#include "task.hpp"

class TaskObtainUnits : public Task {
    SmartObjectArray<ResourceID> units;
    Point point;
    bool field_27;
    bool field_28;

    bool TaskObtainUnits_sub_464C6(UnitInfo* unit, bool mode);

public:
    TaskObtainUnits(Task* task, Point point);
    ~TaskObtainUnits();

    unsigned short CountInstancesOfUnitType(ResourceID unit_type);
    UnitInfo* TaskObtainUnits_sub_465F4(ResourceID unit_type, bool mode);

    int GetMemoryUse() const;
    unsigned short GetFlags() const;
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    bool Task_vfunc9();
    void Task_vfunc11(UnitInfo& unit);
    void AddReminder();
    void Execute();
    void EndTurn();
    void RemoveSelf();

    void AddUnit(ResourceID unit_type);
};

#endif /* TASKOBTAINUNITS_HPP */
