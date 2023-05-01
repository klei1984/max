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

#include "taskcheckassaults.hpp"

#include "aiattack.hpp"
#include "ailog.hpp"
#include "reminders.hpp"
#include "task_manager.hpp"
#include "taskmove.hpp"
#include "units_manager.hpp"

TaskCheckAssaults::TaskCheckAssaults(unsigned short team) : Task(team, nullptr, 0x1E) {}

TaskCheckAssaults::~TaskCheckAssaults() {}

void TaskCheckAssaults::CheckAssaults() {
    AiLog log("Task check assaults");
    unsigned short unit_count = 0;

    if (unit_iterator == nullptr && field_6) {
        field_6 = false;

        unit_iterator = UnitsManager_MobileLandSeaUnits.Begin();

        if (unit_iterator == nullptr) {
            unit_iterator = UnitsManager_MobileAirUnits.Begin();
        }

        if (unit_iterator != nullptr && (*unit_iterator).team != team) {
            SelectNext();
        }
    }

    for (; unit_iterator != nullptr; SelectNext()) {
        if (Paths_HaveTimeToThink()) {
            if (EvaluateAssaults()) {
                log.Log("Assault check paused.");

                return;
            }

            ++unit_count;

        } else {
            log.Log("Assault check paused, %i msecs since frame update, %i units checked.",
                    timer_elapsed_time(Paths_LastTimeStamp), unit_count);

            if (!GetField8()) {
                TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
            }

            return;
        }
    }
}

void TaskCheckAssaults::SelectNext() {
    if (unit_iterator != nullptr) {
        do {
            bool is_air_unit = (*unit_iterator).flags & MOBILE_AIR_UNIT;

            ++unit_iterator;

            if (unit_iterator == nullptr && !is_air_unit) {
                unit_iterator = UnitsManager_MobileAirUnits.Begin();
            }

        } while (unit_iterator != nullptr && (*unit_iterator).team != team);
    }
}

bool TaskCheckAssaults::EvaluateAssaults() {
    return AiAttack_EvaluateAssault(&*unit_iterator, this, &MoveFinishedCallback);
}

void TaskCheckAssaults::MoveFinishedCallback(Task* task, UnitInfo* unit, char result) {
    unit->RemoveTask(task);

    if (result != TASKMOVE_RESULT_SUCCESS) {
        unit->RemoveTask(task);
    }

    if (!task->GetField8()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*task));
    }
}

int TaskCheckAssaults::GetMemoryUse() const { return 4; }

char* TaskCheckAssaults::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Advance to attack nearby enemies.");

    return buffer;
}

unsigned char TaskCheckAssaults::GetType() const { return TaskType_TaskCheckAssaults; }

bool TaskCheckAssaults::IsThinking() { return false; }

void TaskCheckAssaults::BeginTurn() { CheckAssaults(); }

void TaskCheckAssaults::EndTurn() { CheckAssaults(); }

void TaskCheckAssaults::RemoveSelf() {
    unit_iterator = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskCheckAssaults::RemoveUnit(UnitInfo& unit) {
    if ((unit_iterator != nullptr) && (&*unit_iterator == &unit)) {
        SelectNext();
    }
}
