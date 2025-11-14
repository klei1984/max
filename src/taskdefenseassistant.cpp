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

#include "taskdefenseassistant.hpp"

#include "task_manager.hpp"
#include "units_manager.hpp"

TaskDefenseAssistant::TaskDefenseAssistant(TaskManageBuildings* manager_, ResourceID unit_type_)
    : Task(manager_->GetTeam(), manager_, manager_->GetPriority()) {
    manager = manager_;
    unit_type = unit_type_;
}

TaskDefenseAssistant::~TaskDefenseAssistant() {}

std::string TaskDefenseAssistant::WriteStatusLog() const {
    return std::string(UnitsManager_BaseUnits[unit_type].GetSingularName()) + " defense assistant";
}

uint8_t TaskDefenseAssistant::GetType() const { return TaskType_TaskDefenseAssistant; }

void TaskDefenseAssistant::BeginTurn() {
    if (Task_EstimateTurnsTillMissionEnd() >=
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[m_team], unit_type)->GetAttribute(ATTRIB_TURNS)) {
        Point site;

        if (manager->FindDefenseSite(unit_type, nullptr, site, 1, TASK_PRIORITY_BUILDING_DEFENSE)) {
            SmartPointer<TaskCreateBuilding> create_building_task(new (std::nothrow) TaskCreateBuilding(
                this, TASK_PRIORITY_BUILDING_DEFENSE, unit_type, site, &*manager));

            manager->AddCreateOrder(&*create_building_task);
        }
    }
}

void TaskDefenseAssistant::RemoveSelf() {
    m_parent = nullptr;
    manager = nullptr;

    TaskManager.RemoveTask(*this);
}
