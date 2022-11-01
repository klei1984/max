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

#ifndef TASKREMOVERUBBLE_HPP
#define TASKREMOVERUBBLE_HPP

#include "task.hpp"

class TaskRemoveRubble : public Task {
protected:
    SmartPointer<UnitInfo> unit;
    SmartPointer<UnitInfo> target;

    void RemoveTask();
    bool DumpMaterials(UnitInfo* unit);

    static void MoveFinishedCallback(Task* task, UnitInfo* unit, char result);

public:
    TaskRemoveRubble(Task* task, UnitInfo* unit, unsigned short flags);
    ~TaskRemoveRubble();

    int GetMemoryUse() const;
    char* WriteStatusLog(char* buffer) const;
    Rect* GetBounds(Rect* bounds);
    unsigned char GetType() const;
    bool Task_vfunc9();
    void AddUnit(UnitInfo& unit);
    void Begin();
    void EndTurn();
    bool Task_vfunc17(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);

    virtual void ObtainUnit();
};

#endif /* TASKREMOVERUBBLE_HPP */
