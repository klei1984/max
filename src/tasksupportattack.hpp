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

#ifndef TASKSUPPORTATTACK_HPP
#define TASKSUPPORTATTACK_HPP

#include "task.hpp"

class TaskSupportAttack : public Task {
    SmartList<UnitInfo> units;
    ResourceID unit_type1;
    ResourceID unit_type2;
    unsigned int unit_flags;

    void ObtainUnits(unsigned int unit_flags);
    bool IssueOrders(UnitInfo* unit);

    static void MoveFinishedCallback(Task* task, UnitInfo* unit, char result);

public:
    TaskSupportAttack(Task* task);
    ~TaskSupportAttack();

    bool IsUnitUsable(UnitInfo& unit);
    char* WriteStatusLog(char* buffer) const;
    Rect* GetBounds(Rect* bounds);
    unsigned char GetType() const;
    bool IsNeeded();
    void AddUnit(UnitInfo& unit);
    void BeginTurn();
    void EndTurn();
    bool Execute(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);

    bool AddReminders();
    SmartList<UnitInfo>& GetUnits();
};

#endif /* TASKSUPPORTATTACK_HPP */
