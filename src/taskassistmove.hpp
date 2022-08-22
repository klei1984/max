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

#ifndef TASKASSISTMOVE_HPP
#define TASKASSISTMOVE_HPP

#include "task.hpp"

class TaskAssistMove : public Task {
    SmartList<UnitInfo> units;

    void RequestTransport(UnitInfo* unit1, UnitInfo* unit2);
    void CompleteTransport(UnitInfo* unit1, UnitInfo* unit2, Point site);

public:
    TaskAssistMove(unsigned short team);
    ~TaskAssistMove();

    bool Task_vfunc1(UnitInfo& unit);
    bool Task_vfunc2(UnitInfo& unit);
    int GetMemoryUse() const;
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    void Task_vfunc11(UnitInfo& unit);
    void Execute();
    void EndTurn();
    bool Task_vfunc17(UnitInfo& unit);
    void RemoveSelf();
    void Remove(UnitInfo& unit);
    void Task_vfunc24(UnitInfo& unit1, UnitInfo& unit2);
    void Task_vfunc26(UnitInfo& unit1, UnitInfo& unit2);
};

#endif /* TASKASSISTMOVE_HPP */