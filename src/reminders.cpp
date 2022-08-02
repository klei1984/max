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

#include "unitinfo.hpp"
#include "units_manager.hpp"

Reminder::Reminder() {}

Reminder::~Reminder() {}

RemindTurnStart::RemindTurnStart(Task& task) : task(task) { this->task->SetField7(true); }

RemindTurnStart::~RemindTurnStart() {}

void RemindTurnStart::Reminder_vfunc1() {
    task->SetField7(false);
    task->Execute();
}

int RemindTurnStart::Reminder_vfunc2() { return 0; }

int RemindTurnStart::Reminder_vfunc3() { return 4; }

RemindTurnEnd::RemindTurnEnd(Task& task) : task(task) { this->task->SetField8(true); }

RemindTurnEnd::~RemindTurnEnd() {}

void RemindTurnEnd::Reminder_vfunc1() {
    task->SetField8(false);
    task->EndTurn();
}

int RemindTurnEnd::Reminder_vfunc2() { return 1; }

int RemindTurnEnd::Reminder_vfunc3() { return 4; }

RemindAvailable::RemindAvailable(UnitInfo& unit) : unit(unit) {}

RemindAvailable::~RemindAvailable() {}

void RemindAvailable::Reminder_vfunc1() {
    /// \todo Implement missing stuff
    //    Task* task = unit->GetList1FrontTask();
    //    if (!task) {
    //        TaskManager.TaskManager_sub_45466(&*unit);
    //    }
}

int RemindAvailable::Reminder_vfunc2() { return 2; }

int RemindAvailable::Reminder_vfunc3() { return 4; }

RemindMoveFinished::RemindMoveFinished(UnitInfo& new_unit) : new_unit(new_unit) {
    /// \todo Implement missing stuff
    //    new_unit->unitinfo_ChangeField221(0x100, 1);
}

RemindMoveFinished::~RemindMoveFinished() {}

void RemindMoveFinished::Reminder_vfunc1() {
    /// \todo Implement missing stuff

    //    if (new_unit != nullptr) {
    //        new_unit->unitinfo_ChangeField221(0x100, 0);
    //    }
    //
    //    Task* task = new_unit->GetList1FrontTask();
    //    if (task && new_unit->hits) {
    //        task->Task_sub_42E21(*new_unit);
    //    }
}

int RemindMoveFinished::Reminder_vfunc2() { return 3; }

int RemindMoveFinished::Reminder_vfunc3() { return 4; }

RemindAttack::RemindAttack(UnitInfo& unit) : unit(unit) {}

RemindAttack::~RemindAttack() {}

void RemindAttack::Reminder_vfunc1() {
    /// \todo Implement missing stuff
    if (unit->hits && unit->shots && UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
        //        sub_18840(&*unit);
    }
}

int RemindAttack::Reminder_vfunc2() { return 4; }

int RemindAttack::Reminder_vfunc3() { return 4; }
