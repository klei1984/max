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

#include "taskradarassistant.hpp"

#include "task_manager.hpp"
#include "units_manager.hpp"

TaskRadarAssistant::TaskRadarAssistant(TaskManageBuildings* manager_)
    : Task(manager_->GetTeam(), manager_, manager_->GetFlags()) {
    manager = manager_;
}

TaskRadarAssistant::~TaskRadarAssistant() {}

char* TaskRadarAssistant::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Radar assistant");

    return buffer;
}

uint8_t TaskRadarAssistant::GetType() const { return TaskType_TaskRadarAssistant; }

void TaskRadarAssistant::BeginTurn() {
    Point site;

    if (Task_EstimateTurnsTillMissionEnd() >=
        UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], RADAR)->GetAttribute(ATTRIB_TURNS)) {
        if (manager->FindSiteForRadar(nullptr, site)) {
            SmartPointer<TaskCreateBuilding> create_building_task(
                new (std::nothrow) TaskCreateBuilding(this, 0x0D00, RADAR, site, &*manager));

            manager->AddCreateOrder(&*create_building_task);
        }
    }
}

void TaskRadarAssistant::RemoveSelf() {
    manager = nullptr;
    parent = nullptr;

    TaskManager.RemoveTask(*this);
}
