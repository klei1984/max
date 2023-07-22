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

#ifndef TASKFRONTALATTACK_HPP
#define TASKFRONTALATTACK_HPP

#include "spottedunit.hpp"
#include "task.hpp"

class TaskFrontalAttack : public Task {
    bool field_19;
    SmartPointer<SpottedUnit> spotted_unit;
    SmartList<UnitInfo> units1;
    SmartList<UnitInfo> units2;
    int caution_level;

    static void MoveFinishedCallback(Task* task, UnitInfo* unit, char result);
    void Finish();
    void IssueOrders();

public:
    TaskFrontalAttack(unsigned short team, SpottedUnit* spotted_unit, int caution_level);
    ~TaskFrontalAttack();

    int GetCautionLevel(UnitInfo& unit);
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    void AddUnit(UnitInfo& unit);
    void Begin();
    void BeginTurn();
    void EndTurn();
    bool Execute(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);
    void Task_vfunc23(UnitInfo& unit);
};

#endif /* TASKFRONTALATTACK_HPP */
