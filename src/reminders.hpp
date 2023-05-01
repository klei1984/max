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

#ifndef REMINDERS_HPP
#define REMINDERS_HPP

#include "smartpointer.hpp"
#include "task.hpp"

enum {
    REMINDER_TYPE_TURN_START,
    REMINDER_TYPE_TURN_END,
    REMINDER_TYPE_AVAILABLE,
    REMINDER_TYPE_MOVE,
    REMINDER_TYPE_ATTACK,
    REMINDER_TYPE_COUNT
};

class Reminder : public SmartObject {
public:
    Reminder();
    virtual ~Reminder();

    virtual void Execute() = 0;
    virtual int GetType() = 0;
    virtual int GetMemoryUse() const = 0;
};

class RemindTurnStart : public Reminder {
    SmartPointer<Task> task;

public:
    RemindTurnStart(Task& task);
    ~RemindTurnStart();

    void Execute();
    int GetType();
    int GetMemoryUse() const;
};

class RemindTurnEnd : public Reminder {
    SmartPointer<Task> task;

public:
    RemindTurnEnd(Task& task);
    ~RemindTurnEnd();

    void Execute();
    int GetType();
    int GetMemoryUse() const;
};

class RemindAvailable : public Reminder {
    SmartPointer<UnitInfo> unit;

public:
    RemindAvailable(UnitInfo& unit);
    ~RemindAvailable();

    void Execute();
    int GetType();
    int GetMemoryUse() const;
};

class RemindMoveFinished : public Reminder {
    SmartPointer<UnitInfo> new_unit;

public:
    RemindMoveFinished(UnitInfo& new_unit);
    ~RemindMoveFinished();

    void Execute();
    int GetType();
    int GetMemoryUse() const;
};

class RemindAttack : public Reminder {
    SmartPointer<UnitInfo> unit;

public:
    RemindAttack(UnitInfo& unit);
    ~RemindAttack();

    void Execute();
    int GetType();
    int GetMemoryUse() const;
};

#endif /* REMINDERS_HPP */
