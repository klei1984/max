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

#include "taskwaittoattack.hpp"

#include "aiattack.hpp"
#include "task_manager.hpp"
#include "units_manager.hpp"

TaskWaitToAttack::TaskWaitToAttack(UnitInfo* unit)
    : Task(unit->team, unit->GetTask(), unit->GetTask() ? unit->GetTask()->GetFlags() : 0x2900), attacker(unit) {}

TaskWaitToAttack::~TaskWaitToAttack() {}

char* TaskWaitToAttack::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Waiting to attack.");

    return buffer;
}

uint8_t TaskWaitToAttack::GetType() const { return TaskType_TaskWaitToAttack; }

bool TaskWaitToAttack::IsThinking() { return false; }

void TaskWaitToAttack::Begin() {
    SmartList<Task>::Iterator it;

    for (it = attacker->GetTasks().Begin();
         it != attacker->GetTasks().End() && (*it).GetType() != TaskType_TaskWaitToAttack; ++it) {
    }

    if (it != attacker->GetTasks().End()) {
        parent = nullptr;

        TaskManager.RemoveTask(*this);

    } else {
        attacker->AddTask(this);
    }
}

void TaskWaitToAttack::EndTurn() { CheckReactions(); }

void TaskWaitToAttack::RemoveSelf() {
    if (attacker) {
        attacker->RemoveTask(this);
    }

    attacker = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

bool TaskWaitToAttack::CheckReactions() {
    if (attacker && !attacker->delayed_reaction && UnitsManager_UnitList6.GetCount() == 0 &&
        TaskManager_word_1731C0 != 2) {
        AiAttack_EvaluateAttack(&*attacker);

        attacker->RemoveTask(this);
        parent = nullptr;
        TaskManager.RemoveTask(*this);
    }

    return false;
}

void TaskWaitToAttack::RemoveUnit(UnitInfo& unit) {
    if (attacker == unit) {
        attacker = nullptr;
        parent = nullptr;

        TaskManager.RemoveTask(*this);
    }
}
