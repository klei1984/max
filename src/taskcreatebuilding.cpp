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

#include "taskcreatebuilding.hpp"

#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskmanagebuildings.hpp"

enum {
    CREATE_BUILDING_STATE_INITIALIZING,
    CREATE_BUILDING_STATE_WAITING_FOR_PLATFORM,
    CREATE_BUILDING_STATE_REMOVING_MINE,
    CREATE_BUILDING_STATE_GETTING_BUILDER,
    CREATE_BUILDING_STATE_GETTING_MATERIALS,
    CREATE_BUILDING_STATE_EVALUTING_SITE,
    CREATE_BUILDING_STATE_SITE_BLOCKED,
    CREATE_BUILDING_STATE_MOVING_TO_SITE,
    CREATE_BUILDING_STATE_CLEARING_SITE,
    CREATE_BUILDING_STATE_BUILDING,
    CREATE_BUILDING_STATE_MOVING_OFF_SITE,
    CREATE_BUILDING_STATE_FINISHED,
};

TaskCreateBuilding::TaskCreateBuilding(Task* task, unsigned short flags_, ResourceID unit_type_, Point site_,
                                       TaskManageBuildings* manager_)
    : TaskCreate(task, flags_, unit_type_) {
    site = site_;
    manager = manager_;
    field_42 = false;
    op_state = CREATE_BUILDING_STATE_INITIALIZING;

    SDL_assert(site.x >= 0 && site.x < ResourceManager_MapSize.x && site.y >= 0 && site.y < ResourceManager_MapSize.y);
}

TaskCreateBuilding::TaskCreateBuilding(UnitInfo* unit_, TaskManageBuildings* manager_)
    : TaskCreate(manager_, unit_), site(unit_->grid_x, unit_->grid_y) {
    if (unit_->state == ORDER_STATE_UNIT_READY) {
        op_state = CREATE_BUILDING_STATE_MOVING_OFF_SITE;

    } else {
        op_state = CREATE_BUILDING_STATE_BUILDING;
    }

    field_42 = false;
    manager = manager_;
}

TaskCreateBuilding::~TaskCreateBuilding() {}

int TaskCreateBuilding::GetMemoryUse() const { return 4; }

char* TaskCreateBuilding::WriteStatusLog(char* buffer) const {}

Rect* TaskCreateBuilding::GetBounds(Rect* bounds) {}

unsigned char TaskCreateBuilding::GetType() const { return TaskType_TaskCreateBuilding; }

bool TaskCreateBuilding::Task_vfunc9() {}

void TaskCreateBuilding::Task_vfunc11(UnitInfo& unit) {}

void TaskCreateBuilding::Begin() {}

void TaskCreateBuilding::BeginTurn() {}

void TaskCreateBuilding::ChildComplete(Task* task) {}

void TaskCreateBuilding::EndTurn() {}

bool TaskCreateBuilding::Task_vfunc16(UnitInfo& unit) {}

bool TaskCreateBuilding::Task_vfunc17(UnitInfo& unit) {}

void TaskCreateBuilding::RemoveSelf() {}

void TaskCreateBuilding::RemoveUnit(UnitInfo& unit) {}

void TaskCreateBuilding::Task_vfunc27(Zone* zone, char mode) {}

bool TaskCreateBuilding::Task_vfunc28() {}

bool TaskCreateBuilding::Task_vfunc29() {}
