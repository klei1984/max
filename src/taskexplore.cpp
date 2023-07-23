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

#include "taskexplore.hpp"

#include "access.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "inifile.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskobtainunits.hpp"
#include "units_manager.hpp"

TaskExplore::TaskExplore(uint16_t team_, Point point_) : TaskAbstractSearch(team_, nullptr, 0x1B00, point_) {
    memset(obtain_requests, 0, sizeof(obtain_requests));

    obtain_requests[SCOUT] = 1;
    obtain_requests[AWAC] = Builder_IsBuildable(AWAC);
    obtain_requests[SUBMARNE] = Builder_IsBuildable(SUBMARNE) && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE;
    obtain_requests[COMMANDO] = Builder_IsBuildable(COMMANDO) && ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_EXPERT;
}

TaskExplore::~TaskExplore() {}

bool TaskExplore::IsUnitUsable(UnitInfo& unit) {
    bool result;

    if (obtain_requests[unit.unit_type]) {
        int32_t unit_count = 0;

        AiLog log("Can explore task use %s?", UnitsManager_BaseUnits[unit.unit_type].singular_name);

        for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
            if ((*it).unit_type == unit.unit_type) {
                ++unit_count;
            }
        }

        log.Log("Task has %i %s", unit_count, UnitsManager_BaseUnits[unit.unit_type].plural_name);

        if (unit.unit_type == SCOUT || unit.unit_type == FASTBOAT) {
            result = unit_count < 3;

        } else {
            result = unit_count == 0;
        }

    } else {
        result = false;
    }

    return result;
}

char* TaskExplore::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Explore map.");

    return buffer;
}

uint8_t TaskExplore::GetType() const { return TaskType_TaskExplore; }

bool TaskExplore::Execute(UnitInfo& unit) {
    bool result;

    if (unit.IsReadyForOrders(this) && unit.speed) {
        AiLog log("Exlore: Move Finished");

        FindDestination(unit, unit.GetBaseValues()->GetAttribute(ATTRIB_SCAN));

        result = true;

    } else {
        result = false;
    }

    return result;
}

void TaskExplore::TaskAbstractSearch_vfunc28(UnitInfo& unit) {
    obtain_requests[unit.unit_type] = false;

    TaskManager.RemoveTask(*this);
}

bool TaskExplore::IsVisited(UnitInfo& unit, Point point) {
    uint8_t** info_map = AiPlayer_Teams[team].GetInfoMap();

    if (unit.unit_type == FASTBOAT || unit.unit_type == SUBMARNE) {
        if (ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * point.y + point.x] != SURFACE_TYPE_WATER) {
            return true;
        }

    } else if (ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * point.y + point.x] != SURFACE_TYPE_LAND) {
        return true;
    }

    return info_map && (info_map[point.x][point.y] & 0x1);
}

void TaskExplore::ObtainUnit() {
    SmartPointer<TaskObtainUnits> obtain_units_task = new (std::nothrow) TaskObtainUnits(this, point);

    if (obtain_requests[SCOUT]) {
        obtain_units_task->AddUnit(SCOUT);
    }

    if (obtain_requests[AWAC]) {
        obtain_units_task->AddUnit(AWAC);
    }

    if (obtain_requests[FASTBOAT]) {
        obtain_units_task->AddUnit(FASTBOAT);
    }

    if (obtain_requests[SUBMARNE]) {
        obtain_units_task->AddUnit(SUBMARNE);
    }

    if (obtain_requests[COMMANDO]) {
        obtain_units_task->AddUnit(COMMANDO);
    }

    TaskManager.AppendTask(*obtain_units_task);
}
