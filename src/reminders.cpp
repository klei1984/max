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

RemindTurnStart::RemindTurnStart(Task& task) : task(task) { this->task->ChangeIsScheduledForTurnStart(true); }

RemindTurnStart::~RemindTurnStart() {}

void RemindTurnStart::Execute() {
    AILOG(log, "Begin turn for {}", task->WriteStatusLog());

    task->ChangeIsScheduledForTurnStart(false);
    TaskDebugger_DebugBreak(task->GetId());
    task->BeginTurn();
}

int32_t RemindTurnStart::GetType() { return REMINDER_TYPE_TURN_START; }

RemindTurnEnd::RemindTurnEnd(Task& task) : task(task) { this->task->ChangeIsScheduledForTurnEnd(true); }

RemindTurnEnd::~RemindTurnEnd() {}

void RemindTurnEnd::Execute() {
    AILOG(log, "End turn for {}", task->WriteStatusLog());

    task->ChangeIsScheduledForTurnEnd(false);
    TaskDebugger_DebugBreak(task->GetId());
    task->EndTurn();
}

int32_t RemindTurnEnd::GetType() { return REMINDER_TYPE_TURN_END; }

RemindAvailable::RemindAvailable(UnitInfo& unit) : unit(unit) {}

RemindAvailable::~RemindAvailable() {}

void RemindAvailable::Execute() {
    if (!unit->GetTask()) {
        TaskManager.FindTaskForUnit(&*unit);
    }
}

int32_t RemindAvailable::GetType() { return REMINDER_TYPE_AVAILABLE; }

RemindMoveFinished::RemindMoveFinished(UnitInfo& new_unit_) : unit(new_unit_) {
    unit->ChangeAiStateBits(UnitInfo::AI_STATE_MOVE_FINISHED_REMINDER, true);
}

RemindMoveFinished::~RemindMoveFinished() {}

void RemindMoveFinished::Execute() {
    if (unit) {
        unit->ChangeAiStateBits(UnitInfo::AI_STATE_MOVE_FINISHED_REMINDER, false);
    }

    Task* task = unit->GetTask();

    if (task && unit->hits > 0) {
        AILOG(log, "Move finished reminder for {}", task->WriteStatusLog());

        task->Execute(*unit);
    }
}

int32_t RemindMoveFinished::GetType() { return REMINDER_TYPE_MOVE; }

RemindAttack::RemindAttack(UnitInfo& unit) : unit(unit) {}

RemindAttack::~RemindAttack() {}

void RemindAttack::Execute() {
    if (unit->hits && unit->shots > 0 && UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
        if (unit->GetTask()) {
            AILOG(log, "Attack reminder for {}", unit->GetTask()->WriteStatusLog());
        }

        AiAttack_EvaluateAttack(&*unit);
    }
}

int32_t RemindAttack::GetType() { return REMINDER_TYPE_ATTACK; }
