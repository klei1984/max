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

#include "reminders.hpp"

#include "aiattack.hpp"
#include "ailog.hpp"
#include "task_manager.hpp"
#include "taskdebugger.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"

Reminder::Reminder() {}

Reminder::~Reminder() {}

RemindTurnStart::RemindTurnStart(Task& task) : task(task) { this->task->SetField7(true); }

RemindTurnStart::~RemindTurnStart() {}

void RemindTurnStart::Execute() {
    char buffer[500];

    AiLog("Begin turn for %s", task->WriteStatusLog(buffer));

    task->SetField7(false);
    TaskDebugger_DebugBreak(task->GetId());
    task->BeginTurn();
}

int RemindTurnStart::GetType() { return REMINDER_TYPE_TURN_START; }

int RemindTurnStart::GetMemoryUse() const { return 4; }

RemindTurnEnd::RemindTurnEnd(Task& task) : task(task) { this->task->SetField8(true); }

RemindTurnEnd::~RemindTurnEnd() {}

void RemindTurnEnd::Execute() {
    char buffer[500];

    AiLog("End turn for %s", task->WriteStatusLog(buffer));

    task->SetField8(false);
    TaskDebugger_DebugBreak(task->GetId());
    task->EndTurn();
}

int RemindTurnEnd::GetType() { return REMINDER_TYPE_TURN_END; }

int RemindTurnEnd::GetMemoryUse() const { return 4; }

RemindAvailable::RemindAvailable(UnitInfo& unit) : unit(unit) {}

RemindAvailable::~RemindAvailable() {}

void RemindAvailable::Execute() {
    if (!unit->GetTask()) {
        TaskManager.FindTaskForUnit(&*unit);
    }
}

int RemindAvailable::GetType() { return REMINDER_TYPE_AVAILABLE; }

int RemindAvailable::GetMemoryUse() const { return 4; }

RemindMoveFinished::RemindMoveFinished(UnitInfo& new_unit_) : new_unit(new_unit_) {
    new_unit->ChangeField221(0x100, true);
}

RemindMoveFinished::~RemindMoveFinished() {}

void RemindMoveFinished::Execute() {
    if (new_unit) {
        new_unit->ChangeField221(0x100, false);
    }

    Task* task = new_unit->GetTask();

    if (task && new_unit->hits > 0) {
        char buffer[500];

        AiLog("Move finished reminder for %s", task->WriteStatusLog(buffer));

        task->Execute(*new_unit);
    }
}

int RemindMoveFinished::GetType() { return REMINDER_TYPE_MOVE; }

int RemindMoveFinished::GetMemoryUse() const { return 4; }

RemindAttack::RemindAttack(UnitInfo& unit) : unit(unit) {}

RemindAttack::~RemindAttack() {}

void RemindAttack::Execute() {
    if (unit->hits && unit->shots && UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
        if (unit->GetTask()) {
            char buffer[500];

            AiLog("Attack reminder for %s", unit->GetTask()->WriteStatusLog(buffer));
        }

        AiAttack_EvaluateAttack(&*unit);
    }
}

int RemindAttack::GetType() { return REMINDER_TYPE_ATTACK; }

int RemindAttack::GetMemoryUse() const { return 4; }
