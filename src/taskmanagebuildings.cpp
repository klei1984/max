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
#include "task_manager.hpp"
#include "units_manager.hpp"

enum AreaMapMarkers {
    AREA_BLOCKED = 0x0,
    AREA_RESERVED = 0x1,
    AREA_WATER_BLOCKED = 0x2,
    AREA_WATER_RESERVED = 0x3,
    AREA_FREE = 0x4,
    AREA_PATHWAY = 0x5,
    AREA_OBSTRUCTED = 0x6,
    AREA_DANGERZONE = 0x7,
    AREA_AIRZONE = 0x8,
    AREA_BUILT_IN = 0x9,
};

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

bool TaskManageBuildings_IsUnitAvailable(unsigned short team, SmartList<UnitInfo>* units, ResourceID unit_type) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).unit_type == unit_type) {
            return true;
        }
    }

    return false;
}

TaskManageBuildings::TaskManageBuildings(unsigned short team, Point site_) : Task(team, nullptr, 0x1D00) {
    site = site_;
}

TaskManageBuildings::~TaskManageBuildings() {}

void TaskManageBuildings::BuildBridge(Point site, Task* task) {}

void TaskManageBuildings::FillMap(unsigned short** construction_map, int ulx, int uly, int lrx, int lry,
                                  int fill_value) {}

unsigned short** TaskManageBuildings::CreateMap() {}

void TaskManageBuildings::DeleteMap(unsigned short** construction_map) {}

void TaskManageBuildings::MarkMiningAreas(unsigned short** construction_map) {}

void TaskManageBuildings::MarkBuildingAreas(unsigned short** construction_map, int area_expanse, int area_offset) {}

void TaskManageBuildings::ClearBuildingAreas(unsigned short** construction_map, TaskCreateBuilding* task) {}

void TaskManageBuildings::ClearPathways(unsigned short** construction_map) {}

void TaskManageBuildings::ClearPlannedBuildings(unsigned short** construction_map, TaskCreateBuilding* task,
                                                ResourceID unit_type, unsigned short task_flags) {}

bool TaskManageBuildings::FillBlockMap(int ul1, int lr1, int ul2, int lr2, signed char* address_a,
                                       signed char* address_b) {}

void TaskManageBuildings::ClearBlocks(unsigned short** construction_map, struct Block** block_map, Rect bounds,
                                      int area_marker, int unit_size) {}

void TaskManageBuildings::LimitBlockSize(unsigned short** construction_map, int unit_size) {}

int TaskManageBuildings::EvaluateSiteValue(unsigned short** construction_map, Point site, ResourceID unit_type) {}

bool TaskManageBuildings::IsFavorableMiningSite(Point site) {}

bool TaskManageBuildings::IsViableMiningSite(unsigned short** construction_map, int ulx, int uly, int lrx, int lry) {}

bool TaskManageBuildings::IsSafeSite(unsigned short** construction_map, Point site, ResourceID unit_type) {}

bool TaskManageBuildings::EvaluateSite(unsigned short** construction_map, ResourceID unit_type, Point site) {}

bool TaskManageBuildings::FindSite(ResourceID unit_type, TaskCreateBuilding* task, Point site,
                                   unsigned short task_flags) {}

int TaskManageBuildings::GetUnitCount(ResourceID unit_type, unsigned short task_flags) {}

bool TaskManageBuildings::IsSupremeTeam(unsigned short team) {}

int TaskManageBuildings::GetHighestGreenHouseCount(unsigned short team) {}

bool TaskManageBuildings::CreateBuildings(int building_demand, ResourceID unit_type, unsigned short task_flags) {}

bool TaskManageBuildings::PlanNextBuildJob() {}

void TaskManageBuildings::UpdateCargoDemand(int* limit, short* material, int max_mining) {}

void TaskManageBuildings::MakeConnectors(int ulx, int uly, int lrx, int lry, Task* task) {}

bool TaskManageBuildings::CheckNeeds() {}

void TaskManageBuildings::ClearAreasNearBuildings(unsigned char** access_map, int area_expanse,
                                                  TaskCreateBuilding* task) {}

void TaskManageBuildings::EvaluateDangers(unsigned char** access_map) {}

void TaskManageBuildings::MarkDefenseSites(unsigned short** construction_map, unsigned char** access_map,
                                           TaskCreateBuilding* task, int value) {}

void TaskManageBuildings::ClearDefenseSites(unsigned char** access_map, ResourceID unit_type, TaskCreateBuilding* task,
                                            unsigned short task_flags) {}

bool TaskManageBuildings::IsSiteWithinRadarRange(Point site, int unit_range, TaskCreateBuilding* task) {}

void TaskManageBuildings::UpdateAccessMap(unsigned char** access_map, TaskCreateBuilding* task) {}

bool TaskManageBuildings::EvaluateNeedForRadar(unsigned char** access_map, TaskCreateBuilding* task) {}

bool TaskManageBuildings::MarkBuildings(unsigned char** access_map, Point site) {}

void TaskManageBuildings::MarkConnections(unsigned char** access_map, Point site, int value) {}

void TaskManageBuildings::UpdateConnectors(unsigned char** access_map, int ulx, int uly, int lrx, int lry) {}

int TaskManageBuildings::GetConnectionDistance(unsigned char** access_map, Point site1, Point site2,
                                               unsigned short team, int value) {}

bool TaskManageBuildings::ConnectBuilding(unsigned char** access_map, Point site, int value) {}

bool TaskManageBuildings::FindMarkedSite(unsigned char** access_map, Rect* bounds) {}

int TaskManageBuildings::GetMemoryUse() const { return units.GetMemorySize() - 6 + tasks.GetMemorySize() - 10; }

char* TaskManageBuildings::WriteStatusLog(char* buffer) const {
    strcpy(buffer, "Manage buildings.");

    return buffer;
}

unsigned char TaskManageBuildings::GetType() const { return TaskType_TaskManageBuildings; }

bool TaskManageBuildings::Task_vfunc9() { return true; }

void TaskManageBuildings::AddUnit(UnitInfo& unit) {}

void TaskManageBuildings::Begin() {}

void TaskManageBuildings::BeginTurn() {}

void TaskManageBuildings::ChildComplete(Task* task) {}

void TaskManageBuildings::EndTurn() {}

void TaskManageBuildings::RemoveSelf() {
    units.Clear();
    tasks.Clear();

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskManageBuildings::RemoveUnit(UnitInfo& unit) {}

void TaskManageBuildings::Task_vfunc23(UnitInfo& unit) {}

bool TaskManageBuildings::CreateBuilding(ResourceID unit_type, Task* task, unsigned short task_flags) {}

bool TaskManageBuildings::ReconnectBuildings() {}

void TaskManageBuildings::CheckWorkers() {}

bool TaskManageBuildings::CheckPower() {}

bool TaskManageBuildings::FindSiteForRadar(TaskCreateBuilding* task, Point& site) {}

void TaskManageBuildings::AddCreateOrder(TaskCreateBuilding* task) {}

bool TaskManageBuildings::FindDefenseSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site) {}

bool TaskManageBuildings::ChangeSite(TaskCreateBuilding* task, Point& site) {}
