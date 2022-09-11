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

#include "taskmanagebuildings.hpp"

#include "resource_manager.hpp"
#include "units_manager.hpp"

static bool TaskManageBuildings_IsSiteValuable(Point site, unsigned short team);
static bool TaskManageBuildings_IsUnitAvailable(unsigned short team, SmartList<UnitInfo>* units, ResourceID unit_type);

bool TaskManageBuildings_IsSiteValuable(Point site, unsigned short team) {
    unsigned short cargo_site;
    bool result;

    cargo_site = ResourceManager_CargoMap[ResourceManager_MapSize.x * site.y + site.x];

    if ((cargo_site & UnitsManager_TeamInfo[team].team_units->hash_team_id) && (cargo_site & 0x1F) >= 8) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool TaskManageBuildings_IsUnitAvailable(unsigned short team, SmartList<UnitInfo>* units, ResourceID unit_type) {}

TaskManageBuildings::TaskManageBuildings(unsigned short team, Point site) : Task(team, nullptr, 0x1D00) {}

TaskManageBuildings::~TaskManageBuildings() {}

void TaskManageBuildings::BuildBridge(Point site, Task* task) {}

bool TaskManageBuildings::ReconnectBuildings() {}

void TaskManageBuildings::CheckWorkers() {}

bool TaskManageBuildings::CheckPower() {}

bool TaskManageBuildings::FindSiteForRadar(TaskCreateBuilding* task, Point& site) {}

void TaskManageBuildings::AddCreateOrder(TaskCreateBuilding* task) {}

bool TaskManageBuildings::FindDefenseSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site) {}

bool TaskManageBuildings::ChangeSite(TaskCreateBuilding* task, Point& site) {}
