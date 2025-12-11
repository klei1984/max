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

#include "access.hpp"
#include "accessmap.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "missionmanager.hpp"
#include "mouseevent.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "sitemarker.hpp"
#include "survey.hpp"
#include "task_manager.hpp"
#include "taskconnectionassistant.hpp"
#include "taskcreatebuilding.hpp"
#include "taskcreateunit.hpp"
#include "taskdefenseassistant.hpp"
#include "taskhabitatassistant.hpp"
#include "taskpowerassistant.hpp"
#include "taskradarassistant.hpp"
#include "unit.hpp"
#include "units_manager.hpp"
#include "world.hpp"
#include "zonewalker.hpp"

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

enum ConnectionMapMarkers {
    MARKER_EMPTY_SQUARE,
    MARKER_PLANNED_SQUARE,
    MARKER_BUILT_SQUARE,
    MARKER_CONNECTED_SQUARE,
    MARKER_CONSTRUCTION_SQUARE,
    MARKER_CONNECTED_CONSTRUCTION,
};

static bool TaskManageBuildings_IsSiteValuable(Point site, uint16_t team);
static bool TaskManageBuildings_IsUnitAvailable(uint16_t team, SmartList<UnitInfo>* units, ResourceID unit_type);

static const char* TaskManageBuildings_ConnectionStrings[] = {"empty square",        "planned square",
                                                              "built square",        "connected square",
                                                              "construction square", "connected construction"};

bool TaskManageBuildings_IsSiteValuable(Point site, uint16_t team) {
    uint16_t cargo_site;
    bool result;

    cargo_site = ResourceManager_CargoMap[ResourceManager_MapSize.x * site.y + site.x];

    if ((cargo_site & UnitsManager_TeamInfo[team].team_units->hash_team_id) && (cargo_site & 0x1F) >= 8) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool TaskManageBuildings_IsUnitAvailable(uint16_t team, SmartList<UnitInfo>* units, ResourceID unit_type) {
    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type) {
            return true;
        }
    }

    return false;
}

TaskManageBuildings::TaskManageBuildings(uint16_t team, Point site)
    : Task(team, nullptr, TASK_PRIORITY_MANAGE_BUILDINGS) {
    building_site = site;
}

TaskManageBuildings::~TaskManageBuildings() {}

void TaskManageBuildings::BuildBridge(Point site, Task* task) {
    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        Rect bounds;

        (*it).GetBounds(&bounds);

        if (Access_IsInsideBounds(&bounds, &site)) {
            return;
        }
    }

    SmartPointer<TaskCreateBuilding> create_building_task(
        new (std::nothrow) TaskCreateBuilding(task, task->GetPriority(), BRIDGE, site, this));

    AddCreateOrder(&*create_building_task);
}

void TaskManageBuildings::FillMap(uint16_t** construction_map, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry,
                                  int32_t fill_value) {
    ulx = std::max(0, ulx);
    uly = std::max(0, uly);
    lrx = std::min(static_cast<int32_t>(ResourceManager_MapSize.x), lrx);
    lry = std::min(static_cast<int32_t>(ResourceManager_MapSize.y), lry);

    for (int32_t x = ulx; x < lrx; ++x) {
        for (int32_t y = uly; y < lry; ++y) {
            construction_map[x][y] = fill_value;
        }
    }
}

uint16_t** TaskManageBuildings::CreateMap() {
    uint16_t** construction_map = new (std::nothrow) uint16_t*[ResourceManager_MapSize.x];

    for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
        construction_map[i] = new (std::nothrow) uint16_t[ResourceManager_MapSize.y];
    }

    FillMap(construction_map, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y, AREA_FREE);

    return construction_map;
}

void TaskManageBuildings::DeleteMap(uint16_t** construction_map) {
    for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
        delete[] construction_map[i];
    }

    delete[] construction_map;
}

void TaskManageBuildings::MarkMiningAreas(uint16_t** construction_map) {
    uint16_t hash_team_id = UnitsManager_TeamInfo[m_team].team_units->hash_team_id;
    bool mining_station_found = false;

    AILOG(log, "Mark mining areas.");

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() != WTRPLTFM && (*it).GetUnitType() != BRIDGE) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx - 10, bounds.uly - 10, bounds.lrx + 10, bounds.lry + 10, AREA_BLOCKED);

            if ((*it).GetUnitType() == MININGST) {
                mining_station_found = true;
            }
        }
    }

    for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
            if (construction_map[x][y] == AREA_BLOCKED ||
                (!mining_station_found && construction_map[x][y] != AREA_RESERVED)) {
                uint16_t cargo = ResourceManager_CargoMap[ResourceManager_MapSize.x * y + x];

                if ((cargo & hash_team_id) && (cargo & 0x1F) >= 8) {
                    FillMap(construction_map, x - 1, y - 1, x + 1, y + 1, AREA_RESERVED);

                } else {
                    construction_map[x][y] = AREA_FREE;
                }
            }
        }
    }
}

void TaskManageBuildings::MarkBuildingAreas(uint16_t** construction_map, int32_t area_expanse, int32_t area_offset) {
    AILOG(log, "Mark building areas.");

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() != WTRPLTFM && (*it).GetUnitType() != BRIDGE) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            if (Access_GetSurfaceType(bounds.ulx, bounds.uly) == SURFACE_TYPE_WATER) {
                FillMap(construction_map, bounds.ulx - 10 - area_offset + 1, bounds.uly, bounds.lrx + 10, bounds.lry,
                        AREA_RESERVED);
                FillMap(construction_map, bounds.ulx, bounds.uly - 10 - area_offset + 1, bounds.lrx, bounds.lry + 10,
                        AREA_RESERVED);

            } else {
                FillMap(construction_map, bounds.ulx - area_expanse - area_offset + 1, bounds.uly,
                        bounds.lrx + area_expanse, bounds.lry, AREA_RESERVED);
                FillMap(construction_map, bounds.ulx, bounds.uly - area_expanse - area_offset + 1, bounds.lrx,
                        bounds.lry + area_expanse, AREA_RESERVED);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() != CNCT_4W && (*it).GetUnitType() != WTRPLTFM && (*it).GetUnitType() != BRIDGE) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx - area_offset - 1, bounds.uly - area_offset - 1, bounds.lrx + 2,
                    bounds.lry + 2, AREA_FREE);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() != CNCT_4W && (*it).GetUnitType() != WTRPLTFM && (*it).GetUnitType() != BRIDGE) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx - area_offset, bounds.uly - area_offset + 1, bounds.lrx + 1,
                    bounds.lry, AREA_BLOCKED);
            FillMap(construction_map, bounds.ulx - area_offset + 1, bounds.uly - area_offset, bounds.lrx,
                    bounds.lry + 1, AREA_BLOCKED);
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).Task_vfunc28() && (*it).GetUnitType() != WTRPLTFM && (*it).GetUnitType() != BRIDGE) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx - area_offset, bounds.uly - area_offset + 1, bounds.lrx + 1,
                    bounds.lry, AREA_BLOCKED);
            FillMap(construction_map, bounds.ulx - area_offset + 1, bounds.uly - area_offset, bounds.lrx,
                    bounds.lry + 1, AREA_BLOCKED);
        }
    }
}

void TaskManageBuildings::ClearBuildingAreas(uint16_t** construction_map, TaskCreateBuilding* task) {
    AILOG(log, "Clear building areas.");

    int16_t** damage_potential_map =
        AiPlayer_Teams[m_team].GetDamagePotentialMap(ENGINEER, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true);

    if (damage_potential_map) {
        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                if (damage_potential_map[x][y] > 0) {
                    construction_map[x][y] = AREA_DANGERZONE;
                }
            }
        }
    }

    if (!TaskManageBuildings_IsUnitAvailable(m_team, &UnitsManager_MobileLandSeaUnits, MINELAYR)) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).team == m_team && (*it).GetUnitType() == LANDMINE) {
                construction_map[(*it).grid_x][(*it).grid_y] = AREA_OBSTRUCTED;
            }
        }
    }

    if (!TaskManageBuildings_IsUnitAvailable(m_team, &UnitsManager_MobileLandSeaUnits, SEAMNLYR)) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).team == m_team && (*it).GetUnitType() == SEAMINE) {
                construction_map[(*it).grid_x][(*it).grid_y] = AREA_OBSTRUCTED;
            }
        }
    }

    if (!TaskManageBuildings_IsUnitAvailable(m_team, &UnitsManager_MobileLandSeaUnits, BULLDOZR)) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).GetUnitType() == LRGRUBLE || (*it).GetUnitType() == SMLRUBLE) {
                Rect bounds;

                (*it).GetBounds(&bounds);

                FillMap(construction_map, bounds.ulx, bounds.uly, bounds.lrx, bounds.lry, AREA_OBSTRUCTED);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == m_team && (*it).GetUnitType() != CNCT_4W) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx, bounds.uly, bounds.lrx, bounds.lry, AREA_BUILT_IN);
        }
    }
}

void TaskManageBuildings::ClearPathways(uint16_t** construction_map, Rect bounds, int32_t unit_size) {
    Point location(bounds.ulx - 1, bounds.lry);
    Point directions[8];
    int32_t directions_index = 0;
    int32_t range = bounds.lrx - bounds.ulx;

    for (int32_t direction = 0; direction < 8; direction += 2) {
        for (int32_t i = 0; i < range; ++i) {
            location += DIRECTION_OFFSETS[direction];

            if (location.x >= 0 && location.x < ResourceManager_MapSize.x && location.y >= 0 &&
                location.y < ResourceManager_MapSize.y) {
                if (construction_map[location.x][location.y] < AREA_DANGERZONE) {
                    directions[directions_index] = location;
                    ++directions_index;
                }
            }
        }

        location += DIRECTION_OFFSETS[direction];
    }

    if (directions_index == 1) {
        construction_map[directions[0].x][directions[0].y] = AREA_PATHWAY;

    } else if (directions_index == 2 && unit_size == 2 &&
               (directions[0].x == directions[1].x || directions[0].y == directions[1].y)) {
        construction_map[directions[0].x][directions[0].y] = AREA_PATHWAY;
        construction_map[directions[1].x][directions[1].y] = AREA_PATHWAY;
    }
}

void TaskManageBuildings::ClearPlannedBuildings(uint16_t** construction_map, TaskCreateBuilding* task,
                                                ResourceID unit_type, uint16_t task_priority) {
    AILOG(log, "Clear planned buildings.");

    int32_t unit_size = ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING ? 2 : 1;

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if (&*it != task && (*it).GetUnitType() != CNCT_4W && (*it).GetUnitType() != WTRPLTFM && (*it).Task_vfunc29()) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            if ((*it).GetUnitType() == BRIDGE) {
                if ((*it).Task_vfunc28()) {
                    construction_map[bounds.ulx][bounds.uly] = AREA_OBSTRUCTED;
                }

            } else {
                FillMap(construction_map, bounds.ulx, bounds.uly, bounds.lrx, bounds.lry, AREA_BUILT_IN);
            }
        }
    }

    bool is_water = ResourceManager_GetUnit(unit_type).GetLandType() == SURFACE_TYPE_WATER;
    bool needs_land_access =
        unit_type == LIGHTPLT || unit_type == LANDPLT || unit_type == DEPOT || unit_type == TRAINHAL;

    if (task_priority <= TASK_PRIORITY_LAND_ACCESS_THRESHOLD) {
        needs_land_access = false;
    }

    auto world = ResourceManager_GetActiveWorld();

    for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
            int32_t surface_type = world->GetSurfaceType(x, y);
            int32_t marker_type;

            switch (surface_type) {
                case SURFACE_TYPE_WATER: {
                    marker_type = construction_map[x][y];

                    switch (marker_type) {
                        case AREA_BLOCKED: {
                            construction_map[x][y] = AREA_WATER_BLOCKED;
                        } break;

                        case AREA_RESERVED: {
                            construction_map[x][y] = AREA_WATER_RESERVED;
                        } break;

                        default: {
                            if (needs_land_access) {
                                construction_map[x][y] = AREA_BUILT_IN;
                            }
                        } break;
                    }

                } break;

                case SURFACE_TYPE_COAST: {
                    if (is_water) {
                        construction_map[x][y] = AREA_AIRZONE;

                    } else if (construction_map[x][y] < AREA_FREE) {
                        construction_map[x][y] = AREA_WATER_RESERVED;
                    }
                } break;

                case SURFACE_TYPE_AIR: {
                    construction_map[x][y] = AREA_AIRZONE;
                } break;

                case SURFACE_TYPE_LAND: {
                    if (is_water) {
                        construction_map[x][y] = AREA_AIRZONE;
                    }
                } break;
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() == WTRPLTFM) {
            if (is_water) {
                construction_map[(*it).grid_x][(*it).grid_y] = AREA_BUILT_IN;

            } else {
                int32_t marker_type = construction_map[(*it).grid_x][(*it).grid_y];

                switch (marker_type) {
                    case AREA_WATER_BLOCKED: {
                        construction_map[(*it).grid_x][(*it).grid_y] = AREA_BLOCKED;
                    } break;

                    case AREA_WATER_RESERVED: {
                        construction_map[(*it).grid_x][(*it).grid_y] = AREA_RESERVED;
                    } break;
                }
            }
        }

        if ((*it).GetUnitType() == BRIDGE) {
            construction_map[(*it).grid_x][(*it).grid_y] = AREA_OBSTRUCTED;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == m_team &&
            ((*it).GetUnitType() == BARRACKS || (*it).GetUnitType() == DEPOT || (*it).GetUnitType() == DOCK ||
             (*it).GetUnitType() == TRAINHAL || (*it).GetUnitType() == LIGHTPLT || (*it).GetUnitType() == LANDPLT ||
             (*it).GetUnitType() == SHIPYARD)) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            ClearPathways(construction_map, bounds, unit_size);
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if (&*it != task && (*it).GetUnitType() != CNCT_4W && (*it).GetUnitType() != BRIDGE &&
            (*it).GetUnitType() != WTRPLTFM && (*it).Task_vfunc29()) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            ClearPathways(construction_map, bounds, unit_size);
        }
    }
}

bool TaskManageBuildings::FillBlockMap(int32_t ul1, int32_t lr1, int32_t ul2, int32_t lr2, int8_t* address_a,
                                       int8_t* address_b) {
    int32_t distance = ul1 - ul2;

    if (ul1 > ul2 && distance > *address_a) {
        *address_a = distance;
    }

    distance = lr2 - lr1;

    if (lr1 < lr2 && distance > *address_b) {
        *address_b = distance;
    }

    return lr1 + *address_b + *address_a - ul1 <= 6;
}

void TaskManageBuildings::ClearBlocks(uint16_t** construction_map, struct Block** block_map, Rect bounds,
                                      int32_t area_marker, int32_t unit_size) {
    Point position;
    bool flag;
    Rect limits;
    Rect limits2;

    limits.ulx = std::max(0, bounds.ulx - unit_size);
    limits.uly = std::max(0, bounds.uly - unit_size);
    limits.lrx = std::min(bounds.lrx + 1, static_cast<int32_t>(ResourceManager_MapSize.x));
    limits.lry = std::min(bounds.lry + 1, static_cast<int32_t>(ResourceManager_MapSize.y));

    for (int32_t x = limits.ulx; x < limits.lrx; ++x) {
        for (int32_t y = limits.uly; y < limits.lry; ++y) {
            if (construction_map[x][y] == AREA_BLOCKED) {
                limits2.ulx = std::max(0, x - 1);
                limits2.uly = std::max(0, y - 1);
                limits2.lrx = std::min(static_cast<int32_t>(ResourceManager_MapSize.x), x + 1 + unit_size);
                limits2.lry = std::min(static_cast<int32_t>(ResourceManager_MapSize.y), y + 1 + unit_size);

                flag = false;

                for (position.x = limits2.ulx; position.x < limits2.lrx; ++position.x) {
                    for (position.y = limits2.uly; position.y < limits2.lry; ++position.y) {
                        if (construction_map[position.x][position.y] == area_marker) {
                            flag = true;
                        }
                    }
                }

                if (flag) {
                    Block* block = &block_map[x][y];

                    if (!FillBlockMap(x, x + unit_size, bounds.ulx, bounds.lrx, &block->d1, &block->d2) ||
                        !FillBlockMap(y, y + unit_size, bounds.uly, bounds.lry, &block->d3, &block->d4)) {
                        construction_map[x][y] = AREA_PATHWAY;
                    }
                }
            }
        }
    }
}

void TaskManageBuildings::LimitBlockSize(uint16_t** construction_map, int32_t unit_size) {
    Point site;
    Block block;
    Block** block_map;
    int32_t marker;

    AILOG(log, "Limit block size.");

    block_map = new (std::nothrow) Block*[ResourceManager_MapSize.x];

    memset(&block, 0, sizeof(block));

    for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
        block_map[site.x] = new (std::nothrow) Block[ResourceManager_MapSize.y];

        for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
            if (construction_map[site.x][site.y] == AREA_BLOCKED) {
                block_map[site.x][site.y] = block;
            }
        }
    }

    marker = AREA_BLOCKED;

    SiteMarker site_marker(construction_map);

    for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
        for (site.y = 0; site.y < ResourceManager_MapSize.y; ++site.y) {
            if (construction_map[site.x][site.y] == AREA_BUILT_IN) {
                ++marker;

                site_marker.Fill(site, marker + 10);
                ClearBlocks(construction_map, block_map, *site_marker.GetBounds(), marker + 10, unit_size);
            }
        }
    }

    for (site.x = 0; site.x < ResourceManager_MapSize.x; ++site.x) {
        delete[] block_map[site.x];
    }

    delete[] block_map;
}

int32_t TaskManageBuildings::EvaluateSiteValue(uint16_t** construction_map, Point site, ResourceID unit_type) {
    int32_t cargo_total;

    if (unit_type == MININGST) {
        int16_t raw;
        int16_t gold;
        int16_t fuel;

        Survey_GetTotalResourcesInArea(site.x, site.y, 1, &raw, &gold, &fuel, false, m_team);

        cargo_total = raw + gold + fuel;

        if (cargo_demand.raw + raw < 0) {
            cargo_total += (cargo_demand.raw + raw) * 100;
        }

        if (cargo_demand.fuel + fuel < 0) {
            cargo_total += (cargo_demand.fuel + fuel) * 100;
        }

        if (cargo_demand.gold + gold < 0) {
            cargo_total += (cargo_demand.gold + gold) * 100;
        }

    } else {
        int32_t marker;

        cargo_total = -Access_GetApproximateDistance(site, building_site);
        marker = construction_map[site.x][site.y];

        switch (marker) {
            case AREA_WATER_BLOCKED: {
                cargo_total += -2000;
            } break;

            case AREA_WATER_RESERVED: {
                cargo_total += -3000;
            } break;

            case AREA_RESERVED: {
                cargo_total += -1000;
            } break;
        }
    }

    return cargo_total;
}

bool TaskManageBuildings::IsFavorableMiningSite(Point site) {
    int32_t cargo_total;
    int16_t raw;
    int16_t gold;
    int16_t fuel;
    bool result;

    Survey_GetTotalResourcesInArea(site.x, site.y, 1, &raw, &gold, &fuel, false, m_team);

    if (raw + gold + fuel >= 10) {
        if (cargo_demand.raw < 0 || cargo_demand.gold < 0 || cargo_demand.fuel < 0) {
            cargo_total = -cargo_demand.raw;

            if (cargo_total < -cargo_demand.fuel) {
                cargo_total = -cargo_demand.fuel;
            }

            if (cargo_total < -cargo_demand.gold) {
                cargo_total = -cargo_demand.gold;
            }

            if (cargo_total > 8) {
                cargo_total = 8;
            }

            if (-cargo_demand.raw >= cargo_total && raw >= 8) {
                result = true;

            } else if (-cargo_demand.fuel >= cargo_total && fuel >= 8) {
                result = true;

            } else if (-cargo_demand.gold >= cargo_total && gold > 0) {
                result = true;

            } else {
                result = false;
            }

        } else {
            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskManageBuildings::IsViableMiningSite(uint16_t** construction_map, int32_t ulx, int32_t uly, int32_t lrx,
                                             int32_t lry) {
    Point site1;
    Point site2;

    ulx = std::max(1, ulx);
    uly = std::max(1, uly);
    lrx = std::min(lrx, ResourceManager_MapSize.x - 1);
    lry = std::min(lry, ResourceManager_MapSize.y - 1);

    for (site1.x = ulx; site1.x < lrx; ++site1.x) {
        for (site1.y = uly; site1.y < lry; ++site1.y) {
            if (construction_map[site1.x][site1.y] <= AREA_FREE) {
                uint16_t cargo_level = ResourceManager_CargoMap[site1.y * ResourceManager_MapSize.x + site1.x];

                if ((cargo_level & 0x1F) >= 8) {
                    for (site2.x = site1.x - 1; site2.x <= site1.x; ++site2.x) {
                        for (site2.y = site1.y - 1; site2.y <= site1.y; ++site2.y) {
                            if (site2.x > 0 && site2.x < ResourceManager_MapSize.x - 1 && site2.y > 0 &&
                                site2.y < ResourceManager_MapSize.y) {
                                bool is_found = false;

                                for (int32_t x = site2.x; x <= site2.x + 1; ++x) {
                                    for (int32_t y = site2.y; y <= site2.y + 1; ++y) {
                                        if (construction_map[x][y] > AREA_FREE) {
                                            is_found = true;
                                        }
                                    }
                                }

                                if (!is_found) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool TaskManageBuildings::IsSafeSite(uint16_t** construction_map, Point site, ResourceID unit_type) {
    Rect site_bounds;
    Rect map_bounds;
    bool result;

    rect_init(&site_bounds, site.x, site.y, site.x + 1, site.y + 1);
    rect_init(&map_bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (unit_type == MININGST && !IsFavorableMiningSite(site)) {
        result = false;

    } else {
        if (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING) {
            ++site_bounds.lrx;
            ++site_bounds.lry;
        }

        if (site_bounds.lrx <= ResourceManager_MapSize.x && site_bounds.lry <= ResourceManager_MapSize.y) {
            int32_t marker_index = 0;
            int32_t markers[12];
            int32_t marker;

            for (site.x = site_bounds.ulx; site.x < site_bounds.lrx; ++site.x) {
                for (site.y = site_bounds.uly; site.y < site_bounds.lry; ++site.y) {
                    if (construction_map[site.x][site.y] > AREA_FREE) {
                        return false;
                    }
                }
            }

            site.x = site_bounds.ulx - 1;
            site.y = site_bounds.lry;

            for (int32_t direction = 0; direction < 8; direction += 2) {
                for (int32_t range = 0; range < site_bounds.lrx - site_bounds.ulx + 1; ++range) {
                    site += DIRECTION_OFFSETS[direction];

                    if (!Access_IsInsideBounds(&map_bounds, &site)) {
                        return false;
                    }

                    marker = construction_map[site.x][site.y];

                    if (marker == AREA_AIRZONE && unit_type != MININGST) {
                        return false;
                    }

                    if ((!marker_index || markers[marker_index - 1] != marker) &&
                        marker_index < static_cast<int32_t>(std::size(markers))) {
                        markers[marker_index] = marker;
                        ++marker_index;
                    }
                }
            }

            if (marker_index > 1 && markers[0] == markers[marker_index - 1]) {
                --marker_index;
            }

            for (int32_t i = 0; i < marker_index - 1; ++i) {
                if (markers[i] >= 10) {
                    for (int32_t j = i + 1; j < marker_index; ++j) {
                        if (markers[i] == markers[j]) {
                            return false;
                        }
                    }
                }
            }

            int32_t site_count = 0;

            site.x = site_bounds.ulx - 1;
            site.y = site_bounds.uly;

            for (int32_t direction = 0; direction < 8; direction += 2) {
                for (int32_t range = 0; range < site_bounds.lrx - site_bounds.ulx; ++range) {
                    site += DIRECTION_OFFSETS[direction];

                    if (construction_map[site.x][site.y] < AREA_DANGERZONE) {
                        ++site_count;
                    }
                }

                site += DIRECTION_OFFSETS[direction];
            }

            if (site_count > 0) {
                if (unit_type == MININGST ||
                    !IsViableMiningSite(construction_map, site_bounds.ulx - 1, site_bounds.uly - 1, site_bounds.lrx + 1,
                                        site_bounds.lry + 1)) {
                    result = true;

                } else {
                    result = false;
                }

            } else {
                result = false;
            }

        } else {
            result = false;
        }
    }

    return result;
}

bool TaskManageBuildings::EvaluateSite(uint16_t** construction_map, ResourceID unit_type, Point& site) {
    Point position;
    int32_t site_value;
    int32_t best_site_value{INT32_MIN};
    bool is_site_found = false;
    bool result;

    AILOG(log, "Find site / site map.");

    for (position.x = 0; position.x < ResourceManager_MapSize.x; ++position.x) {
        for (position.y = 0; position.y < ResourceManager_MapSize.y; ++position.y) {
            if (construction_map[position.x][position.y] < AREA_FREE) {
                site_value = EvaluateSiteValue(construction_map, position, unit_type);

                if ((!is_site_found || site_value > best_site_value) &&
                    IsSafeSite(construction_map, position, unit_type)) {
                    site = position;
                    best_site_value = site_value;
                    is_site_found = true;
                }
            }
        }
    }

    if (is_site_found) {
        Rect bounds;

        rect_init(&bounds, site.x, site.y, site.x + 1, site.y + 1);

        if (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING) {
            ++bounds.lrx;
            ++bounds.lry;
        }

        for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
            if ((*it).GetUnitType() == BRIDGE && !(*it).Task_vfunc28()) {
                Point point = (*it).DeterminePosition();

                if (Access_IsInsideBounds(&bounds, &point)) {
                    (*it).RemoveSelf();
                }
            }
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool TaskManageBuildings::FindSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site,
                                   uint16_t task_priority) {
    uint16_t** construction_map = CreateMap();
    int32_t unit_size = (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING) ? 2 : 1;
    bool result;

    AILOG(log, "Find Site.");

    if (unit_type == MININGST) {
        MarkMiningAreas(construction_map);

    } else {
        int32_t area_expanse;

        if (unit_type == SHIPYARD || unit_type == DEPOT || unit_type == HANGAR || unit_type == DOCK ||
            unit_type == BARRACKS) {
            area_expanse = 10;

        } else {
            area_expanse = 4;
        }

        MarkBuildingAreas(construction_map, area_expanse, unit_size);
    }

    ClearBuildingAreas(construction_map, task);
    ClearPlannedBuildings(construction_map, task, unit_type, task_priority);
    LimitBlockSize(construction_map, unit_size);
    result = EvaluateSite(construction_map, unit_type, site);
    DeleteMap(construction_map);

    return result;
}

int32_t TaskManageBuildings::GetUnitCount(ResourceID unit_type, uint16_t task_priority) {
    int32_t unit_count = 0;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() == unit_type) {
            ++unit_count;
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == unit_type && ((*it).Task_vfunc28() || (*it).ComparePriority(task_priority) <= 0)) {
            ++unit_count;
        }
    }

    return unit_count;
}

bool TaskManageBuildings::IsSupremeTeam(uint16_t team_) {
    int32_t team_points = UnitsManager_TeamInfo[team_].team_points;
    bool result;

    if (team_points > 0) {
        for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
            if (UnitsManager_TeamInfo[i].team_type != TEAM_TYPE_NONE) {
                if (static_cast<uint32_t>(team_points) <= UnitsManager_TeamInfo[i].team_points) {
                    return false;
                }
            }
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

int32_t TaskManageBuildings::GetHighestGreenHouseCount(uint16_t team_) {
    int32_t highest_greenhouse_count = 0;
    int32_t greenhouse_counts[PLAYER_TEAM_MAX];

    memset(greenhouse_counts, 0, sizeof(greenhouse_counts));

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetUnitType() == GREENHSE) {
            ++greenhouse_counts[(*it).team];
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).GetUnitType() == CONSTRCT && (*it).GetOrder() == ORDER_BUILD &&
            (*it).GetOrderState() == ORDER_STATE_BUILD_IN_PROGRESS && (*it).GetConstructedUnitType() == GREENHSE) {
            ++greenhouse_counts[(*it).team];
        }
    }

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (i != team_) {
            highest_greenhouse_count = std::max(highest_greenhouse_count, greenhouse_counts[i]);
        }
    }

    return highest_greenhouse_count;
}

bool TaskManageBuildings::CreateBuildings(int32_t building_demand, ResourceID unit_type, uint16_t task_priority) {
    if (Builder_IsBuildable(m_team, unit_type)) {
        building_demand -= GetUnitCount(unit_type, task_priority);

        for (int32_t i = 0; i < building_demand; ++i) {
            if (CreateBuilding(unit_type, this, task_priority)) {
                return true;
            }
        }
    }

    return false;
}

bool TaskManageBuildings::PlanNextBuildJob() {
    bool result;

    if (units.GetCount()) {
        if (!GetUnitCount(LIGHTPLT, TASK_PRIORITY_BUILDING_LIGHT_PLANT) &&
            CreateBuilding(LIGHTPLT, this, TASK_PRIORITY_BUILDING_LIGHT_PLANT)) {
            result = true;

        } else if (!GetUnitCount(LANDPLT, TASK_PRIORITY_LAND_ACCESS_THRESHOLD) &&
                   CreateBuilding(LANDPLT, this, TASK_PRIORITY_LAND_ACCESS_THRESHOLD)) {
            result = true;

        } else {
            int32_t unit_count_lightplant = GetUnitCount(LIGHTPLT, 0x00);
            int32_t unit_count_landplant = GetUnitCount(LANDPLT, 0x00);
            int32_t unit_count_airplant = GetUnitCount(AIRPLT, 0x00);
            int32_t unit_count_shipyard = GetUnitCount(SHIPYARD, 0x00);
            int32_t unit_count_trainhall = GetUnitCount(TRAINHAL, 0x00);
            int32_t total_unit_count = unit_count_lightplant + unit_count_landplant + unit_count_airplant +
                                       unit_count_shipyard + unit_count_trainhall;

            if (unit_count_lightplant > 0 && unit_count_landplant > 0) {
                if (!GetUnitCount(AIRPLT, TASK_PRIORITY_LAND_ACCESS_THRESHOLD) &&
                    CreateBuilding(AIRPLT, this, TASK_PRIORITY_LAND_ACCESS_THRESHOLD)) {
                    return true;
                }

                if (!GetUnitCount(HABITAT, TASK_PRIORITY_BUILDING_HABITAT) &&
                    CreateBuilding(HABITAT, this, TASK_PRIORITY_BUILDING_HABITAT)) {
                    return true;
                }

                if (!GetUnitCount(GREENHSE, TASK_PRIORITY_BUILDING_GREENHOUSE) &&
                    CreateBuilding(GREENHSE, this, TASK_PRIORITY_BUILDING_GREENHOUSE)) {
                    return true;
                }

                if (!GetUnitCount(RESEARCH, TASK_PRIORITY_BUILDING_UPGRADES) &&
                    CreateBuilding(RESEARCH, this, TASK_PRIORITY_BUILDING_UPGRADES)) {
                    return true;
                }

                if (unit_count_airplant > 0) {
                    if (CreateBuildings((AiPlayer_Teams[m_team].GetField5() * total_unit_count) / 10, GREENHSE,
                                        TASK_PRIORITY_BUILDING_GREENHOUSE)) {
                        return true;
                    }

                    if (CreateBuildings(GetUnitCount(GREENHSE, TASK_PRIORITY_BUILDING_UPGRADES), RESEARCH,
                                        TASK_PRIORITY_BUILDING_UPGRADES)) {
                        return true;
                    }

                    if (IsSupremeTeam(m_team) && CreateBuildings(GetHighestGreenHouseCount(m_team) + 1, GREENHSE,
                                                                 TASK_PRIORITY_BUILDING_GREENHOUSE)) {
                        return true;
                    }
                }
            }

            total_unit_count /= 3;

            switch (AiPlayer_Teams[m_team].GetStrategy()) {
                case AI_STRATEGY_DEFENSIVE: {
                    if (CreateBuildings(total_unit_count, LIGHTPLT, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_MISSILES: {
                } break;

                case AI_STRATEGY_AIR: {
                    if (!GetUnitCount(AIRPLT, TASK_PRIORITY_LAND_ACCESS_THRESHOLD) &&
                        CreateBuilding(AIRPLT, this, TASK_PRIORITY_LAND_ACCESS_THRESHOLD)) {
                        return true;
                    }

                    if (CreateBuildings(total_unit_count, AIRPLT, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_SEA: {
                    if (!GetUnitCount(SHIPYARD, TASK_PRIORITY_LAND_ACCESS_THRESHOLD) &&
                        CreateBuilding(SHIPYARD, this, TASK_PRIORITY_LAND_ACCESS_THRESHOLD)) {
                        return true;
                    }

                    if (CreateBuildings(total_unit_count, SHIPYARD, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_SCOUT_HORDE: {
                    if (CreateBuildings(total_unit_count, LIGHTPLT, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_TANK_HORDE: {
                    if (CreateBuildings(total_unit_count, LANDPLT, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_FAST_ATTACK: {
                    if (CreateBuildings(total_unit_count / 2, LIGHTPLT, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }

                    if (CreateBuildings(total_unit_count / 2, LANDPLT, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_COMBINED_ARMS: {
                    if (CreateBuildings(total_unit_count, LANDPLT, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_ESPIONAGE: {
                    if (!GetUnitCount(TRAINHAL, TASK_PRIORITY_LAND_ACCESS_THRESHOLD) &&
                        CreateBuilding(TRAINHAL, this, TASK_PRIORITY_LAND_ACCESS_THRESHOLD)) {
                        return true;
                    }

                    if (CreateBuildings(total_unit_count, TRAINHAL, TASK_PRIORITY_FOLLOW_DEFENSE)) {
                        return true;
                    }
                } break;
            }

            if (GetUnitCount(LANDPLT, 0x00) && !GetUnitCount(DEPOT, TASK_PRIORITY_BUILDING_REPAIR_SHOP) &&
                CreateBuilding(DEPOT, this, TASK_PRIORITY_BUILDING_REPAIR_SHOP)) {
                result = true;

            } else if (GetUnitCount(AIRPLT, 0x00) && !GetUnitCount(HANGAR, TASK_PRIORITY_BUILDING_REPAIR_SHOP) &&
                       CreateBuilding(HANGAR, this, TASK_PRIORITY_BUILDING_REPAIR_SHOP)) {
                result = true;

            } else if (GetUnitCount(SHIPYARD, 0x00) && !GetUnitCount(DOCK, TASK_PRIORITY_BUILDING_REPAIR_SHOP) &&
                       CreateBuilding(DOCK, this, TASK_PRIORITY_BUILDING_REPAIR_SHOP)) {
                result = true;

            } else if (GetUnitCount(TRAINHAL, 0x00) && !GetUnitCount(BARRACKS, TASK_PRIORITY_MANAGE_BUILDINGS) &&
                       CreateBuilding(BARRACKS, this, TASK_PRIORITY_MANAGE_BUILDINGS)) {
                result = true;

            } else if (unit_count_airplant > 0 && !GetUnitCount(SHIPYARD, TASK_PRIORITY_LAND_ACCESS_THRESHOLD) &&
                       CreateBuilding(SHIPYARD, this, TASK_PRIORITY_LAND_ACCESS_THRESHOLD)) {
                result = true;

            } else {
                int32_t unit_count = Access_GetRepairShopClientCount(m_team, DEPOT);
                int32_t shop_capacity =
                    UnitsManager_TeamInfo[m_team].team_units->GetBaseUnitValues(DEPOT)->GetAttribute(ATTRIB_STORAGE);

                if (GetUnitCount(DEPOT, TASK_PRIORITY_BUILDING_REPAIR_SHOP) < (unit_count / (shop_capacity * 2)) &&
                    CreateBuilding(DEPOT, this, TASK_PRIORITY_BUILDING_REPAIR_SHOP)) {
                    result = true;

                } else {
                    unit_count = Access_GetRepairShopClientCount(m_team, HANGAR);
                    shop_capacity = UnitsManager_TeamInfo[m_team].team_units->GetBaseUnitValues(HANGAR)->GetAttribute(
                        ATTRIB_STORAGE);

                    if (GetUnitCount(HANGAR, TASK_PRIORITY_BUILDING_REPAIR_SHOP) < (unit_count / (shop_capacity * 2)) &&
                        CreateBuilding(HANGAR, this, TASK_PRIORITY_BUILDING_REPAIR_SHOP)) {
                        result = true;

                    } else {
                        unit_count = Access_GetRepairShopClientCount(m_team, DOCK);
                        shop_capacity = UnitsManager_TeamInfo[m_team].team_units->GetBaseUnitValues(DOCK)->GetAttribute(
                            ATTRIB_STORAGE);

                        if (GetUnitCount(DOCK, TASK_PRIORITY_BUILDING_REPAIR_SHOP) <
                                (unit_count / (shop_capacity * 2)) &&
                            CreateBuilding(DOCK, this, TASK_PRIORITY_BUILDING_REPAIR_SHOP)) {
                            result = true;

                        } else {
                            unit_count = Access_GetRepairShopClientCount(m_team, BARRACKS);
                            shop_capacity =
                                UnitsManager_TeamInfo[m_team].team_units->GetBaseUnitValues(BARRACKS)->GetAttribute(
                                    ATTRIB_STORAGE);

                            if (GetUnitCount(BARRACKS, TASK_PRIORITY_MANAGE_BUILDINGS) <
                                    (unit_count / (shop_capacity * 2)) &&
                                CreateBuilding(BARRACKS, this, TASK_PRIORITY_MANAGE_BUILDINGS)) {
                                result = true;

                            } else {
                                result = false;
                            }
                        }
                    }
                }
            }
        }

    } else {
        result = false;
    }

    return result;
}

void TaskManageBuildings::UpdateCargoDemand(int16_t* limit, int16_t* material, int32_t max_mining) {
    int16_t demand = -(*material);

    if (demand > 0) {
        if (demand > *limit) {
            demand = *limit;
        }

        if (demand > max_mining) {
            demand = max_mining;
        }

        *limit -= demand;
        *material += demand;
    }
}

void TaskManageBuildings::UpdateMiningNeeds() {
    AILOG(log, "Task Manage Buildings: Update Mining Needs");

    cargo_demand.Init();

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == m_team && ((*it).GetUnitType() == ENGINEER || (*it).GetUnitType() == CONSTRCT)) {
            cargo_demand.raw += (*it).storage;

            if ((*it).GetUnitType() == ENGINEER) {
                cargo_demand.raw -= 20;

            } else {
                cargo_demand.raw -= 40;
            }
        }
    }

    cargo_demand.raw = (cargo_demand.raw - 50) / 10;
    cargo_demand.fuel -= 5;

    if (Builder_IsBuildable(m_team, COMMTWR)) {
        cargo_demand.gold -= Cargo_GetGoldConsumptionRate(COMMTWR);
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == m_team) {
            cargo_demand.raw -= Cargo_GetRawConsumptionRate((*it).GetUnitType(), 1);
            cargo_demand.fuel -= Cargo_GetFuelConsumptionRate((*it).GetUnitType());
            cargo_demand.gold -= Cargo_GetGoldConsumptionRate((*it).GetUnitType());

            switch (ResourceManager_GetUnit((*it).GetUnitType()).GetCargoType()) {
                case Unit::CargoType::CARGO_TYPE_RAW: {
                    cargo_demand.raw += (*it).storage / 10;
                } break;

                case Unit::CargoType::CARGO_TYPE_FUEL: {
                    cargo_demand.fuel += (*it).storage / 10;
                } break;

                case Unit::CargoType::CARGO_TYPE_GOLD: {
                    cargo_demand.gold += (*it).storage / 20;
                } break;
            }
        }
    }

    int32_t cargo_gold = cargo_demand.gold;

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        cargo_gold -= Cargo_GetGoldConsumptionRate((*it).GetUnitType());

        if ((*it).Task_vfunc28()) {
            cargo_demand.raw -= Cargo_GetRawConsumptionRate((*it).GetUnitType(), 1);
            cargo_demand.fuel -= Cargo_GetFuelConsumptionRate((*it).GetUnitType());
            cargo_demand.gold -= Cargo_GetGoldConsumptionRate((*it).GetUnitType());
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetUnitType() == MININGST && (*it).team == m_team) {
            int16_t capacity_limit = 16;

            UpdateCargoDemand(&capacity_limit, &cargo_demand.fuel, (*it).fuel_mining_max);
            UpdateCargoDemand(&capacity_limit, &cargo_demand.raw, (*it).raw_mining_max);

            cargo_gold += std::min(capacity_limit, static_cast<int16_t>((*it).gold_mining_max));

            UpdateCargoDemand(&capacity_limit, &cargo_demand.gold, (*it).gold_mining_max);
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == MININGST) {
            int16_t capacity_limit = 16;
            Rect bounds;
            int16_t raw;
            int16_t fuel;
            int16_t gold;

            (*it).GetBounds(&bounds);

            Survey_GetTotalResourcesInArea(bounds.ulx, bounds.uly, 1, &raw, &gold, &fuel, false, m_team);

            UpdateCargoDemand(&capacity_limit, &cargo_demand.fuel, fuel);
            UpdateCargoDemand(&capacity_limit, &cargo_demand.raw, raw);

            if ((*it).Task_vfunc28()) {
                cargo_gold += std::min(capacity_limit, gold);
            }

            UpdateCargoDemand(&capacity_limit, &cargo_demand.gold, gold);
        }
    }

    if (cargo_demand.raw < 0) {
        AILOG_LOG(log, "{} desired material", -cargo_demand.raw);
    }

    if (cargo_demand.fuel < 0) {
        AILOG_LOG(log, "{} desired fuel", -cargo_demand.fuel);
    }

    if (cargo_demand.gold < 0) {
        AILOG_LOG(log, "{} desired gold", -cargo_demand.gold);
    }

    if (Cargo_GetGoldConsumptionRate(COMMTWR) <= cargo_gold) {
        CreateBuilding(COMMTWR, this, TASK_PRIORITY_BUILDING_UPGRADES);
    }
}

void TaskManageBuildings::MakeConnectors(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, Task* task) {
    Point site;

    AILOG(log, "Task Manage Buildings: Make Connectors.");

    if (ulx < 1) {
        ulx = 1;
    }

    if (lrx >= ResourceManager_MapSize.x - 1) {
        lrx = ResourceManager_MapSize.x - 1;
    }

    if (uly < 1) {
        uly = 1;
    }

    if (lry >= ResourceManager_MapSize.y - 1) {
        lry = ResourceManager_MapSize.y - 1;
    }

    for (site.x = ulx; site.x < lrx; ++site.x) {
        for (site.y = uly; site.y < lry; ++site.y) {
            SmartPointer<TaskCreateBuilding> create_building_task(
                new (std::nothrow) TaskCreateBuilding(task, TASK_PRIORITY_BUILDING_CONNECTOR, CNCT_4W, site, this));

            AddCreateOrder(&*create_building_task);
        }
    }
}

bool TaskManageBuildings::CheckNeeds() {
    bool result;
    bool build_order = false;

    AILOG(log, "Task Manage Buildings: Check Needs.");

    if (PlanNextBuildJob()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnStart(*this));

        result = true;

    } else {
        UpdateMiningNeeds();

        if (cargo_demand.raw < 0 || cargo_demand.fuel < 0 || cargo_demand.gold < 0) {
            SmartList<UnitInfo>::Iterator it;
            uint16_t task_priority;

            for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == m_team && (*it).GetUnitType() == MININGST) {
                    break;
                }
            }

            if (it != UnitsManager_StationaryUnits.End()) {
                task_priority = TASK_PRIORITY_BUILDING_MEDIUM;

            } else {
                task_priority = TASK_PRIORITY_BUILDING_LOW;
            }

            if (CreateBuilding(MININGST, this, task_priority)) {
                build_order = true;
            }
        }

        result = build_order;
    }

    return result;
}

void TaskManageBuildings::ClearAreasNearBuildings(AccessMap& access_map, int32_t area_expanse,
                                                  TaskCreateBuilding* task) {
    Rect bounds;
    Point site;
    int32_t unit_size;

    AILOG(log, "Clear areas near buildings.");

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() != CNCT_4W && (*it).GetUnitType() != WTRPLTFM && (*it).GetUnitType() != BRIDGE) {
            Rect limits;

            unit_size = ((*it).flags & BUILDING) ? area_expanse : 1;

            (*it).GetBounds(&limits);

            limits.ulx -= unit_size;
            limits.lrx += unit_size;
            limits.uly -= unit_size;
            limits.lry += unit_size;

            for (site.x = limits.ulx; site.x < limits.lrx; ++site.x) {
                for (site.y = limits.uly; site.y < limits.lry; ++site.y) {
                    if (Access_IsInsideBounds(&bounds, &site)) {
                        access_map(site.x, site.y) = 0;
                    }
                }
            }
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((&*it) != task && (*it).Task_vfunc29()) {
            ResourceID unit_type = (*it).GetUnitType();

            if (unit_type != CNCT_4W && unit_type != WTRPLTFM && unit_type != BRIDGE) {
                Rect limits;

                unit_size = (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING) ? area_expanse : 1;

                (*it).GetBounds(&limits);

                limits.ulx -= unit_size;
                limits.lrx += unit_size;
                limits.uly -= unit_size;
                limits.lry += unit_size;

                for (site.x = limits.ulx; site.x < limits.lrx; ++site.x) {
                    for (site.y = limits.uly; site.y < limits.lry; ++site.y) {
                        if (Access_IsInsideBounds(&bounds, &site)) {
                            access_map(site.x, site.y) = 0;
                        }
                    }
                }
            }
        }
    }
}

void TaskManageBuildings::EvaluateDangers(AccessMap& access_map) {
    int16_t** damage_potential_map =
        AiPlayer_Teams[m_team].GetDamagePotentialMap(ENGINEER, CAUTION_LEVEL_AVOID_ALL_DAMAGE, true);

    if (damage_potential_map) {
        for (int32_t x = 0; x < ResourceManager_MapSize.x; ++x) {
            for (int32_t y = 0; y < ResourceManager_MapSize.y; ++y) {
                if (damage_potential_map[x][y] > 0) {
                    access_map(x, y) = 0;
                }
            }
        }
    }
}

void TaskManageBuildings::MarkDefenseSites(uint16_t** construction_map, AccessMap& access_map, TaskCreateBuilding* task,
                                           int32_t value) {
    AILOG(log, "Mark defense sites.");

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if (((*it).flags & BUILDING) || (*it).GetUnitType() == RADAR) {
            Rect bounds;
            Point site;
            int32_t accessmap_value;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx - 2, bounds.uly - 2, bounds.lrx + 2, bounds.lry + 2, 0x01);
            FillMap(construction_map, bounds.ulx - 1, bounds.uly - 1, bounds.lrx + 1, bounds.lry + 1, 0x00);

            bounds.ulx = std::max(bounds.ulx - 2, 0);
            bounds.uly = std::max(bounds.uly - 2, 0);
            bounds.lrx = std::min(bounds.lrx + 2, static_cast<int32_t>(ResourceManager_MapSize.x));
            bounds.lry = std::min(bounds.lry + 2, static_cast<int32_t>(ResourceManager_MapSize.y));

            accessmap_value = ((*it).GetUnitType() == RADAR) ? value + 1 : value;

            for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                    access_map(site.x, site.y) = accessmap_value;
                }
            }
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((&*it) != task && (*it).Task_vfunc28()) {
            if ((ResourceManager_GetUnit((*it).GetUnitType()).GetFlags() & BUILDING) || (*it).GetUnitType() == RADAR) {
                Rect bounds;
                Point site;
                int32_t accessmap_value;

                (*it).GetBounds(&bounds);

                FillMap(construction_map, bounds.ulx - 1, bounds.uly - 1, bounds.lrx + 1, bounds.lry + 1, 0x00);

                bounds.ulx = std::max(bounds.ulx - 1, 0);
                bounds.uly = std::max(bounds.uly - 1, 0);
                bounds.lrx = std::min(bounds.lrx + 1, static_cast<int32_t>(ResourceManager_MapSize.x));
                bounds.lry = std::min(bounds.lry + 1, static_cast<int32_t>(ResourceManager_MapSize.y));

                accessmap_value = ((*it).GetUnitType() == RADAR) ? value + 1 : value;

                for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                    for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                        access_map(site.x, site.y) = accessmap_value;
                    }
                }
            }
        }
    }
}

void TaskManageBuildings::ClearDefenseSites(AccessMap& access_map, ResourceID unit_type, TaskCreateBuilding* task,
                                            uint16_t task_priority) {
    Point position;

    AILOG(log, "Clear defended sites for {}.", ResourceManager_GetUnit(unit_type).GetSingularName().data());

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() == unit_type) {
            int32_t unit_range = (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE);

            position.x = (*it).grid_x;
            position.y = (*it).grid_y;

            ZoneWalker walker(position, unit_range);

            do {
                uint8_t& cell = access_map(walker.GetGridX(), walker.GetGridY());
                if (cell > 0) {
                    --cell;
                }
            } while (walker.FindNext());
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == unit_type && (&*it) != task && (*it).ComparePriority(task_priority) <= 0) {
            int32_t unit_range =
                UnitsManager_TeamInfo[m_team].team_units->GetCurrentUnitValues(unit_type)->GetAttribute(ATTRIB_RANGE);

            position = (*it).DeterminePosition();

            ZoneWalker walker(position, unit_range);

            do {
                uint8_t& cell = access_map(walker.GetGridX(), walker.GetGridY());
                if (cell > 0) {
                    --cell;
                }
            } while (walker.FindNext());
        }
    }
}

bool TaskManageBuildings::IsSiteWithinRadarRange(Point site, int32_t unit_range, TaskCreateBuilding* task) {
    Point position;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() == RADAR) {
            int32_t distance = (*it).GetBaseValues()->GetAttribute(ATTRIB_SCAN) - unit_range;

            position.x = site.x - (*it).grid_x;
            position.y = site.y - (*it).grid_y;

            if (position.x * position.x + position.y * position.y <= distance * distance) {
                return true;
            }
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == RADAR && (&*it) != task) {
            int32_t distance =
                UnitsManager_TeamInfo[m_team].team_units->GetCurrentUnitValues(RADAR)->GetAttribute(ATTRIB_SCAN) -
                unit_range;

            position = (*it).DeterminePosition();

            position -= site;

            if (position.x * position.x + position.y * position.y <= distance * distance) {
                return true;
            }
        }
    }

    return false;
}

void TaskManageBuildings::UpdateAccessMap(AccessMap& access_map, TaskCreateBuilding* task) {
    int32_t unit_scan =
        UnitsManager_TeamInfo[m_team].team_units->GetCurrentUnitValues(RADAR)->GetAttribute(ATTRIB_SCAN);

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() == RADAR) {
            ZoneWalker walker(Point((*it).grid_x, (*it).grid_y), (*it).GetBaseValues()->GetAttribute(ATTRIB_SCAN) / 2);

            do {
                access_map(walker.GetGridX(), walker.GetGridY()) = 0;

            } while (walker.FindNext());
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == RADAR && (&*it) != task) {
            ZoneWalker walker((*it).DeterminePosition(), unit_scan / 2);

            do {
                access_map(walker.GetGridX(), walker.GetGridY()) = 0;

            } while (walker.FindNext());
        }
    }
}

bool TaskManageBuildings::EvaluateNeedForRadar(AccessMap& access_map, TaskCreateBuilding* task) {
    int32_t unit_scan =
        UnitsManager_TeamInfo[m_team].team_units->GetCurrentUnitValues(RADAR)->GetAttribute(ATTRIB_SCAN);
    bool is_radar_needed = false;
    bool result;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0) {
            Point position((*it).grid_x, (*it).grid_y);

            if (!IsSiteWithinRadarRange(position, (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE), task)) {
                ZoneWalker walker(position, unit_scan - (*it).GetBaseValues()->GetAttribute(ATTRIB_RANGE));

                is_radar_needed = true;

                do {
                    if (!TaskManageBuildings_IsSiteValuable(*walker.GetCurrentLocation(), m_team) &&
                        Access_GetModifiedSurfaceType(walker.GetGridX(), walker.GetGridY()) != SURFACE_TYPE_AIR) {
                        ++access_map(walker.GetGridX(), walker.GetGridY());
                    }

                } while (walker.FindNext());
            }
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).Task_vfunc28()) {
            Point position((*it).DeterminePosition());
            SmartPointer<UnitValues> unit_values(
                UnitsManager_TeamInfo[m_team].team_units->GetCurrentUnitValues((*it).GetUnitType()));

            if (unit_values->GetAttribute(ATTRIB_ATTACK) > 0 &&
                !IsSiteWithinRadarRange(position, unit_values->GetAttribute(ATTRIB_RANGE), task)) {
                ZoneWalker walker(position, unit_scan - unit_values->GetAttribute(ATTRIB_RANGE));

                is_radar_needed = true;

                do {
                    if (!TaskManageBuildings_IsSiteValuable(*walker.GetCurrentLocation(), m_team) &&
                        Access_GetModifiedSurfaceType(walker.GetGridX(), walker.GetGridY()) != SURFACE_TYPE_AIR) {
                        ++access_map(walker.GetGridX(), walker.GetGridY());
                    }

                } while (walker.FindNext());
            }
        }
    }

    if (is_radar_needed) {
        ClearAreasNearBuildings(access_map, 2, task);
        EvaluateDangers(access_map);
        UpdateAccessMap(access_map, task);

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool TaskManageBuildings::MarkBuildings(AccessMap& access_map, Point& site) {
    ResourceID best_unit_type = INVALID_ID;
    int32_t best_complex_size = 0;

    AILOG(log, "Mark Buildings.");

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() != WTRPLTFM && (*it).GetUnitType() != BRIDGE && (*it).GetUnitType() != CNCT_4W) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            if ((*it).GetUnitType() == MININGST) {
                if (best_unit_type != MININGST) {
                    best_unit_type = INVALID_ID;

                } else {
                    const int32_t complex_size = (*it).GetComplex()->GetBuildings();

                    if (best_complex_size < complex_size) {
                        best_complex_size = complex_size;
                        best_unit_type = INVALID_ID;
                    }
                }
            }

            if (best_unit_type == INVALID_ID) {
                best_unit_type = (*it).GetUnitType();
                site.x = bounds.ulx;
                site.y = bounds.uly;
            }

            for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
                for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                    access_map(x, y) = MARKER_BUILT_SQUARE;
                }
            }
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        ResourceID unit_type = (*it).GetUnitType();

        if (unit_type != WTRPLTFM && unit_type != BRIDGE && unit_type != CNCT_4W && (*it).Task_vfunc28()) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            if (unit_type == MININGST && best_unit_type != MININGST) {
                best_unit_type = INVALID_ID;
            }

            if (best_unit_type == INVALID_ID) {
                best_unit_type = unit_type;
                site.x = bounds.ulx;
                site.y = bounds.uly;
            }

            for (int32_t x = bounds.ulx; x < bounds.lrx; ++x) {
                for (int32_t y = bounds.uly; y < bounds.lry; ++y) {
                    access_map(x, y) = MARKER_CONSTRUCTION_SQUARE;
                }
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() == CNCT_4W) {
            access_map((*it).grid_x, (*it).grid_y) = MARKER_BUILT_SQUARE;
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == CNCT_4W) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            access_map(bounds.ulx, bounds.uly) = MARKER_BUILT_SQUARE;
        }
    }

    return best_unit_type != INVALID_ID;
}

void TaskManageBuildings::MarkConnections(AccessMap& access_map, Point site, int32_t value) {
    Point site2;
    Point site3;
    bool flag1;
    bool flag2;
    bool flag3;
    bool flag4;

    do {
        AILOG(log, "Mark Connections from {} at [{},{}].", TaskManageBuildings_ConnectionStrings[value], site.x + 1,
              site.y + 1);

        while (site.x > 0 &&
               (access_map(site.x, site.y) == value || access_map(site.x, site.y) == MARKER_BUILT_SQUARE)) {
            --site.x;
        }

        ++site.x;

        if (site.x >= ResourceManager_MapSize.x ||
            (access_map(site.x, site.y) != value && access_map(site.x, site.y) != MARKER_BUILT_SQUARE)) {
            return;
        }

        flag1 = false;
        flag2 = false;
        flag3 = false;
        flag4 = false;

        while (site.x < ResourceManager_MapSize.x &&
               (access_map(site.x, site.y) == value || access_map(site.x, site.y) == MARKER_BUILT_SQUARE)) {
            if (value == MARKER_BUILT_SQUARE) {
                access_map(site.x, site.y) = MARKER_CONNECTED_SQUARE;

            } else {
                access_map(site.x, site.y) = MARKER_CONNECTED_CONSTRUCTION;
            }

            if (site.y > 0 &&
                (access_map(site.x, site.y - 1) == value || access_map(site.x, site.y - 1) == MARKER_BUILT_SQUARE)) {
                if (flag2) {
                    MarkConnections(access_map, site2, value);

                    flag2 = false;
                    flag1 = false;
                }

                if (!flag1) {
                    site2.x = site.x;
                    site2.y = site.y - 1;

                    flag1 = true;
                }

            } else {
                if (flag1) {
                    flag2 = true;
                }
            }

            if (site.y < ResourceManager_MapSize.y - 1 &&
                (access_map(site.x, site.y + 1) == value || access_map(site.x, site.y + 1) == MARKER_BUILT_SQUARE)) {
                if (flag4) {
                    MarkConnections(access_map, site3, value);

                    flag4 = false;
                    flag3 = false;
                }

                if (!flag3) {
                    site3.x = site.x;
                    site3.y = site.y + 1;

                    flag3 = true;
                }

            } else {
                if (flag3) {
                    flag4 = true;
                }
            }

            ++site.x;
        }

        if (flag1) {
            if (flag3) {
                MarkConnections(access_map, site2, value);

            } else {
                flag3 = true;
                site3 = site2;
            }
        }

        site = site3;

    } while (flag3);
}

void TaskManageBuildings::UpdateConnectors(AccessMap& access_map, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    for (int32_t x = ulx; x < lrx; ++x) {
        for (int32_t y = uly; y < lry; ++y) {
            access_map(x, y) = MARKER_BUILT_SQUARE;
        }
    }

    MakeConnectors(ulx, uly, lrx, lry, this);
}

int32_t TaskManageBuildings::GetConnectionDistance(AccessMap& access_map, Point& site1, Point site2, uint16_t team_,
                                                   int32_t value) {
    Point site3;
    Point site4;
    Rect bounds;

    rect_init(&bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    do {
        site1 += site2;

        if (!Access_IsInsideBounds(&bounds, &site1)) {
            return 1000;
        }

    } while (access_map(site1.x, site1.y) == value || access_map(site1.x, site1.y) == MARKER_BUILT_SQUARE);

    site3 = site1;

    for (;;) {
        if (access_map(site3.x, site3.y) > MARKER_EMPTY_SQUARE || TaskManageBuildings_IsSiteValuable(site3, team_) ||
            Access_GetModifiedSurfaceType(site3.x, site3.y) == SURFACE_TYPE_AIR) {
            if (access_map(site3.x, site3.y) == MARKER_CONNECTED_SQUARE) {
                return Access_GetApproximateDistance(site3, site1);

            } else if (access_map(site3.x, site3.y) == MARKER_CONNECTED_CONSTRUCTION &&
                       value == MARKER_CONSTRUCTION_SQUARE) {
                return Access_GetApproximateDistance(site3, site1);

            } else {
                return 1000;
            }
        }

        site3 += site2;

        if (!Access_IsInsideBounds(&bounds, &site3)) {
            return 1000;
        }

        site4.x = site3.x + labs(site2.y);
        site4.y = site3.y + labs(site2.x);

        if (Access_IsInsideBounds(&bounds, &site4) &&
            (access_map(site4.x, site4.y) == MARKER_CONNECTED_SQUARE ||
             (access_map(site4.x, site4.y) == MARKER_CONNECTED_CONSTRUCTION && value == MARKER_CONSTRUCTION_SQUARE))) {
            return Access_GetApproximateDistance(site3, site1) + 2;
        }

        site4.x = site3.x - labs(site2.y);
        site4.y = site3.y - labs(site2.x);

        if (Access_IsInsideBounds(&bounds, &site4) &&
            (access_map(site4.x, site4.y) == MARKER_CONNECTED_SQUARE ||
             (access_map(site4.x, site4.y) == MARKER_CONNECTED_CONSTRUCTION && value == MARKER_CONSTRUCTION_SQUARE))) {
            return Access_GetApproximateDistance(site3, site1) + 2;
        }
    }
}

bool TaskManageBuildings::ConnectBuilding(AccessMap& access_map, Point site, int32_t value) {
    Point site1;
    Point site2;
    Point site3;
    Point site4;
    int32_t distance1;
    int32_t distance2;
    int32_t distance3;
    int32_t distance4;
    int32_t minimum_distance;
    bool is_site_found;
    bool result = false;

    AILOG(log, "Connect {} at [{},{}]", TaskManageBuildings_ConnectionStrings[value], site.x + 1, site.y + 1);

    do {
        site1 = site;
        distance1 = GetConnectionDistance(access_map, site1, Point(-1, 0), m_team, value);

        site2 = site;
        distance2 = GetConnectionDistance(access_map, site2, Point(1, 0), m_team, value);

        site3 = site;
        distance3 = GetConnectionDistance(access_map, site3, Point(0, -1), m_team, value);

        site4 = site;
        distance4 = GetConnectionDistance(access_map, site4, Point(0, 1), m_team, value);

        minimum_distance = ResourceManager_MapSize.x;

        if (minimum_distance > distance1) {
            minimum_distance = distance1;
        }

        if (minimum_distance > distance2) {
            minimum_distance = distance2;
        }

        if (minimum_distance > distance3) {
            minimum_distance = distance3;
        }

        if (minimum_distance > distance4) {
            minimum_distance = distance4;
        }

        is_site_found = false;

        if (minimum_distance == 0) {
            MarkConnections(access_map, site, 2);

        } else {
            if (minimum_distance == distance1) {
                site = site1;
                is_site_found = true;

            } else if (minimum_distance == distance2) {
                site = site2;
                is_site_found = true;

            } else if (minimum_distance == distance3) {
                site = site3;
                is_site_found = true;

            } else if (minimum_distance == distance4) {
                site = site4;
                is_site_found = true;
            }

            if (is_site_found) {
                UpdateConnectors(access_map, site.x, site.y, site.x + 1, site.y + 1);

                result = true;
            }
        }

    } while (is_site_found);

    return result;
}

bool TaskManageBuildings::ReconnectBuilding(AccessMap& access_map, Rect* bounds, int32_t value) {
    Point site;

    if (value == MARKER_CONSTRUCTION_SQUARE) {
        if (access_map(bounds->ulx, bounds->uly) != MARKER_CONSTRUCTION_SQUARE) {
            return false;
        }

    } else if (access_map(bounds->ulx, bounds->uly) == MARKER_CONNECTED_SQUARE) {
        return false;
    }

    for (site.x = bounds->ulx; site.x < bounds->lrx; ++site.x) {
        for (site.y = bounds->uly; site.y < bounds->lry; ++site.y) {
            if (ConnectBuilding(access_map, site, value)) {
                return true;
            }
        }
    }

    for (site.x = bounds->ulx; site.x < bounds->lrx; ++site.x) {
        site.y = bounds->uly - 1;

        if (access_map(site.x, site.y) == MARKER_EMPTY_SQUARE) {
            if (ConnectBuilding(access_map, site, value)) {
                UpdateConnectors(access_map, site.x, site.y, site.x + 1, site.y + 1);

                return true;
            }
        }

        site.y = bounds->lry;

        if (access_map(site.x, site.y) == MARKER_EMPTY_SQUARE) {
            if (ConnectBuilding(access_map, site, value)) {
                UpdateConnectors(access_map, site.x, site.y, site.x + 1, site.y + 1);

                return true;
            }
        }
    }

    for (site.y = bounds->uly; site.y < bounds->lry; ++site.y) {
        site.x = bounds->ulx - 1;

        if (access_map(site.x, site.y) == MARKER_EMPTY_SQUARE) {
            if (ConnectBuilding(access_map, site, value)) {
                UpdateConnectors(access_map, site.x, site.y, site.x + 1, site.y + 1);

                return true;
            }
        }

        site.x = bounds->lrx;

        if (access_map(site.x, site.y) == MARKER_EMPTY_SQUARE) {
            if (ConnectBuilding(access_map, site, value)) {
                UpdateConnectors(access_map, site.x, site.y, site.x + 1, site.y + 1);

                return true;
            }
        }
    }

    return false;
}

bool TaskManageBuildings::FindMarkedSite(AccessMap& access_map, Rect* bounds) {
    Rect limits;
    Point site;

    limits.ulx = std::max(0, bounds->ulx - 1);
    limits.uly = std::max(0, bounds->uly - 1);
    limits.lrx = std::min(ResourceManager_MapSize.x - 1, bounds->lrx + 1);
    limits.lry = std::min(ResourceManager_MapSize.y - 1, bounds->lry + 1);

    for (site.x = limits.ulx; site.x < limits.lrx; ++site.x) {
        for (site.y = bounds->uly; site.y < bounds->lry; ++site.y) {
            if (access_map(site.x, site.y) == MARKER_CONNECTED_SQUARE) {
                return true;
            }
        }
    }

    for (site.x = bounds->ulx; site.x < bounds->lrx; ++site.x) {
        for (site.y = limits.uly; site.y < limits.lry; ++site.y) {
            if (access_map(site.x, site.y) == MARKER_CONNECTED_SQUARE) {
                return true;
            }
        }
    }

    return false;
}

std::string TaskManageBuildings::WriteStatusLog() const { return "Manage buildings."; }

uint8_t TaskManageBuildings::GetType() const { return TaskType_TaskManageBuildings; }

bool TaskManageBuildings::IsNeeded() { return true; }

void TaskManageBuildings::AddUnit(UnitInfo& unit) {
    AILOG(log, "Task Manage Buildings: Add {}.", ResourceManager_GetUnit(unit.GetUnitType()).GetSingularName().data());

    if (unit.flags & STATIONARY) {
        units.PushBack(unit);

        if (unit.GetOrder() == ORDER_BUILD || unit.GetOrder() == ORDER_HALT_BUILDING) {
            SmartPointer<TaskCreateUnit> create_unit_task(new (std::nothrow) TaskCreateUnit(&unit, this));

            TaskManager.AppendTask(*create_unit_task);
        }

    } else {
        SmartPointer<TaskCreateBuilding> create_building_task(new (std::nothrow) TaskCreateBuilding(&unit, this));

        tasks.PushBack(*create_building_task);

        TaskManager.AppendTask(*create_building_task);
    }
}

void TaskManageBuildings::Init() {
    if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_AVERAGE) {
        if (Builder_IsBuildable(m_team, ANTIAIR)) {
            SmartPointer<TaskDefenseAssistant> task(new (std::nothrow) TaskDefenseAssistant(this, ANTIAIR));

            TaskManager.AppendTask(*task);
        }
    }

    if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_APPRENTICE) {
        if (Builder_IsBuildable(m_team, GUNTURRT)) {
            SmartPointer<TaskDefenseAssistant> task(new (std::nothrow) TaskDefenseAssistant(this, GUNTURRT));

            TaskManager.AppendTask(*task);
        }
    }

    if (ResourceManager_GetSettings()->GetNumericValue("opponent") >= OPPONENT_TYPE_AVERAGE) {
        if (Builder_IsBuildable(m_team, ANTIMSSL)) {
            SmartPointer<TaskDefenseAssistant> task(new (std::nothrow) TaskDefenseAssistant(this, ANTIMSSL));

            TaskManager.AppendTask(*task);
        }
    }

    {
        SmartPointer<TaskRadarAssistant> task(new (std::nothrow) TaskRadarAssistant(this));

        TaskManager.AppendTask(*task);
    }

    {
        SmartPointer<TaskPowerAssistant> task(new (std::nothrow) TaskPowerAssistant(this));

        TaskManager.AppendTask(*task);
    }

    if (Builder_IsBuildable(m_team, HABITAT)) {
        SmartPointer<TaskHabitatAssistant> task(new (std::nothrow) TaskHabitatAssistant(this));

        TaskManager.AppendTask(*task);
    }

    {
        SmartPointer<TaskConnectionAssistant> task(new (std::nothrow) TaskConnectionAssistant(this));

        TaskManager.AppendTask(*task);
    }
}

void TaskManageBuildings::BeginTurn() {
    AILOG(log, "Manage buildings: begin turn.");

    MouseEvent::ProcessInput();
    CheckNeeds();

    if (!IsScheduledForTurnEnd()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
    }
}

void TaskManageBuildings::ChildComplete(Task* task) {
    if (task->GetType() == TaskType_TaskCreateBuilding) {
        AILOG(log, "Manage Buildings: Remove project {}", task->WriteStatusLog());

        tasks.Remove(*dynamic_cast<TaskCreateBuilding*>(task));
    }
}

void TaskManageBuildings::EndTurn() {
    Cargo cargo;
    Cargo capacity;

    int32_t raw_mining_max = 0;
    int32_t fuel_mining_max = 0;
    int32_t gold_mining_max = 0;

    AILOG(log, "Manage buildings: end turn.");

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).GetUnitType() == MININGST) {
            raw_mining_max += (*it).raw_mining_max;
            fuel_mining_max += (*it).fuel_mining_max;
            gold_mining_max += (*it).gold_mining_max;
        }

        switch (ResourceManager_GetUnit((*it).GetUnitType()).GetCargoType()) {
            case Unit::CargoType::CARGO_TYPE_RAW: {
                cargo.raw += (*it).storage;
                capacity.raw += (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            } break;

            case Unit::CargoType::CARGO_TYPE_FUEL: {
                cargo.fuel += (*it).storage;
                capacity.fuel += (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            } break;

            case Unit::CargoType::CARGO_TYPE_GOLD: {
                cargo.gold += (*it).storage;
                capacity.gold += (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            } break;
        }
    }

    const auto team_units = UnitsManager_TeamInfo[m_team].team_units;
    const auto adump_capacity = team_units->GetBaseUnitValues(ADUMP)->GetAttribute(ATTRIB_STORAGE);

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == MININGST && (*it).Task_vfunc28()) {
            Rect bounds;
            int16_t raw;
            int16_t fuel;
            int16_t gold;

            (*it).GetBounds(&bounds);

            Survey_GetTotalResourcesInArea(bounds.ulx, bounds.uly, 1, &raw, &gold, &fuel, false, m_team);

            raw_mining_max += raw;
            fuel_mining_max += fuel;
            gold_mining_max += gold;

            // do not count the mediocre storage capacity of planned mining stations under a given capacity threshold
            if (capacity.raw < 2 * adump_capacity) {
                continue;
            }
        }

        switch (ResourceManager_GetUnit((*it).GetUnitType()).GetCargoType()) {
            case Unit::CargoType::CARGO_TYPE_RAW: {
                capacity.raw += team_units->GetCurrentUnitValues((*it).GetUnitType())->GetAttribute(ATTRIB_STORAGE);
            } break;

            case Unit::CargoType::CARGO_TYPE_FUEL: {
                capacity.fuel += team_units->GetCurrentUnitValues((*it).GetUnitType())->GetAttribute(ATTRIB_STORAGE);
            } break;

            case Unit::CargoType::CARGO_TYPE_GOLD: {
                capacity.gold += team_units->GetCurrentUnitValues((*it).GetUnitType())->GetAttribute(ATTRIB_STORAGE);
            } break;
        }
    }

    if (raw_mining_max > 0 && ((cargo.raw >= (capacity.raw * 3) / 4) || (capacity.raw < 2 * adump_capacity)) &&
        capacity.raw < 500) {
        CreateBuilding(ADUMP, this, TASK_PRIORITY_BUILDING_DUMP);
    }

    if (fuel_mining_max > 0 && (cargo.fuel >= (capacity.fuel * 3) / 4) && capacity.fuel < 500) {
        CreateBuilding(FDUMP, this, TASK_PRIORITY_BUILDING_DUMP);
    }

    if (gold_mining_max > 0 && (cargo.gold >= (capacity.gold * 3) / 4) && capacity.gold < 500 &&
        Builder_IsBuildable(m_team, GOLDSM)) {
        CreateBuilding(GOLDSM, this, TASK_PRIORITY_BUILDING_DUMP);
    }
}

void TaskManageBuildings::RemoveSelf() {
    units.Clear();
    tasks.Clear();

    m_parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskManageBuildings::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }

void TaskManageBuildings::EventUnitDestroyed(UnitInfo& unit) { units.Remove(unit); }

bool TaskManageBuildings::CreateBuilding(ResourceID unit_type, Task* task, uint16_t task_priority) {
    Point site;
    bool result;

    AILOG(log, "Manager: Create {}", ResourceManager_GetUnit(unit_type).GetSingularName().data());

    if (Builder_IsBuildable(m_team, unit_type)) {
        if (Task_EstimateTurnsTillMissionEnd() >=
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[m_team], unit_type)->GetAttribute(ATTRIB_TURNS)) {
            uint32_t unit_counters[UNIT_END];

            memset(&unit_counters, 0, sizeof(unit_counters));

            for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                if ((*it).ComparePriority(task_priority + TASK_PRIORITY_ADJUST_MEDIUM) <= 0) {
                    ++unit_counters[(*it).GetUnitType()];
                }
            }

            const auto builder_type = Builder_GetBuilderType(m_team, unit_type);
            int64_t builder_count = 0;
            int64_t unit_count = 0;

            for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
                 it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
                if ((*it).team == m_team && (*it).GetUnitType() == builder_type) {
                    ++builder_count;
                }
            }

            for (const auto unit : Builder_GetBuildableUnits(m_team, builder_type)) {
                unit_count += unit_counters[unit];
            }

            if (builder_count * 2 + 1 > unit_count) {
                bool is_site_found;

                if (unit_type == RADAR) {
                    is_site_found = FindSiteForRadar(nullptr, site);

                } else if (unit_type == GUNTURRT || unit_type == ANTIMSSL || unit_type == ARTYTRRT ||
                           unit_type == ANTIAIR) {
                    is_site_found = FindDefenseSite(unit_type, nullptr, site, 5, task_priority);

                } else {
                    is_site_found = FindSite(unit_type, nullptr, site, task_priority);
                }

                if (is_site_found) {
                    SmartPointer<TaskCreateBuilding> create_building_task(
                        new (std::nothrow) TaskCreateBuilding(task, task_priority, unit_type, site, this));

                    AddCreateOrder(&*create_building_task);

                    result = true;

                } else {
                    AILOG_LOG(log, "No site found.");

                    result = false;
                }

            } else {
                result = false;
            }

        } else {
            AILOG_LOG(log, "Not enough time.");

            result = false;
        }

    } else {
        AILOG_LOG(log, "Illegal type.");

        result = false;
    }

    return result;
}

bool TaskManageBuildings::ReconnectBuildings() {
    bool result;

    if (units.GetCount() > 0 || tasks.GetCount() > 0) {
        const World* world = ResourceManager_GetActiveWorld();
        AccessMap access_map(world);
        Point site;

        if (MarkBuildings(access_map, site)) {
            AILOG(log, "Reconnect buildings.");

            MarkConnections(access_map, site, MARKER_BUILT_SQUARE);

            for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                ResourceID unit_type = (*it).GetUnitType();

                if ((*it).Task_vfunc28() && unit_type != CNCT_4W && unit_type != WTRPLTFM && unit_type != BRIDGE) {
                    Rect bounds;

                    (*it).GetBounds(&bounds);

                    if (FindMarkedSite(access_map, &bounds)) {
                        for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                            for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                                if (access_map(site.x, site.y) == MARKER_CONSTRUCTION_SQUARE) {
                                    MarkConnections(access_map, site, MARKER_CONSTRUCTION_SQUARE);
                                }
                            }
                        }
                    }
                }
            }

            result = false;

            for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
                if ((*it).GetUnitType() != CNCT_4W && (*it).GetUnitType() != WTRPLTFM &&
                    (*it).GetUnitType() != BRIDGE && (*it).GetUnitType() != GUNTURRT &&
                    (*it).GetUnitType() != ARTYTRRT && (*it).GetUnitType() != ANTIMSSL &&
                    (*it).GetUnitType() != ANTIAIR && (*it).GetUnitType() != RADAR) {
                    Rect bounds;

                    (*it).GetBounds(&bounds);

                    if (ReconnectBuilding(access_map, &bounds, MARKER_BUILT_SQUARE)) {
                        result = true;
                    }
                }
            }

            for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                ResourceID unit_type = (*it).GetUnitType();

                if ((*it).Task_vfunc28() && unit_type != CNCT_4W && unit_type != WTRPLTFM && unit_type != BRIDGE &&
                    unit_type != GUNTURRT && unit_type != ARTYTRRT && unit_type != ANTIMSSL && unit_type != ANTIAIR &&
                    unit_type != RADAR) {
                    Rect bounds;

                    (*it).GetBounds(&bounds);

                    if (ReconnectBuilding(access_map, &bounds, MARKER_CONSTRUCTION_SQUARE)) {
                        result = true;
                    }
                }
            }

            AILOG_LOG(log, "Finished reconnecting.");

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void TaskManageBuildings::CheckWorkers() {
    int32_t total_consumption = 0;

    AILOG(log, "Task Manage Buildings: Check Workers.");

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        int32_t consumption_rate = Cargo_GetLifeConsumptionRate((*it).GetUnitType());

        if (consumption_rate > 0) {
            total_consumption += consumption_rate;
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).Task_vfunc28()) {
            int32_t consumption_rate = Cargo_GetLifeConsumptionRate((*it).GetUnitType());

            if (consumption_rate > 0) {
                total_consumption += consumption_rate;
            }
        }
    }

    {
        int32_t consumption_rate = -Cargo_GetLifeConsumptionRate(HABITAT);
        int32_t habitat_demand = (total_consumption + consumption_rate - 1) / consumption_rate;

        CreateBuildings(habitat_demand, HABITAT, TASK_PRIORITY_BUILDING_HABITAT);
    }
}

bool TaskManageBuildings::CheckPower() {
    int32_t power_consumption = 0;
    int32_t power_generation = 0;
    int32_t total_power_generation = 0;
    int32_t total_storage = 0;

    AILOG(log, "Task Manage Buildings: Check Power.");

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        int32_t power_consumption_rate = Cargo_GetPowerConsumptionRate((*it).GetUnitType());

        if (power_consumption_rate >= 0) {
            power_consumption += power_consumption_rate;

        } else {
            power_generation -= power_consumption_rate;
        }

        if (ResourceManager_GetUnit((*it).GetUnitType()).GetCargoType() == Unit::CargoType::CARGO_TYPE_AIR) {
            total_storage += (*it).storage;
        }
    }

    int32_t turns_to_build = BuildMenu_GetTurnsToBuild(POWERSTN, m_team);

    total_power_generation = power_generation;

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == POWGEN) {
            ++total_power_generation;
        }

        if ((*it).Task_vfunc28() && (*it).GetUnitType() == POWERSTN) {
            int32_t power_consumption_rate = (*it).EstimateBuildTime();

            turns_to_build = std::min(turns_to_build, power_consumption_rate);
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).Task_vfunc28() && (*it).EstimateBuildTime() < turns_to_build) {
            int32_t power_consumption_rate = Cargo_GetPowerConsumptionRate((*it).GetUnitType());

            if (power_consumption_rate > 0) {
                power_consumption += power_consumption_rate;
            }
        }
    }

    {
        int32_t power_generation_rate1 = -Cargo_GetPowerConsumptionRate(POWERSTN);
        int32_t power_generation_rate2 = -Cargo_GetPowerConsumptionRate(POWGEN);
        int32_t fuel_consumption_rate1 = Cargo_GetFuelConsumptionRate(POWERSTN);
        int32_t fuel_consumption_rate2 = Cargo_GetFuelConsumptionRate(POWGEN);

        if (total_storage < 50) {
            int32_t power_station_demand =
                power_generation_rate1 - ((fuel_consumption_rate1 * power_generation_rate2) / fuel_consumption_rate2);

            power_station_demand = (power_consumption + power_station_demand) / power_generation_rate1;

            if (CreateBuildings(power_station_demand, POWERSTN, TASK_PRIORITY_BUILDING_POWER)) {
                return true;
            }
        }

        if (power_consumption > total_power_generation && CreateBuilding(POWGEN, this, TASK_PRIORITY_BUILDING_POWER)) {
            return true;

        } else {
            return false;
        }
    }
}

bool TaskManageBuildings::FindSiteForRadar(TaskCreateBuilding* task, Point& site) {
    const World* world = ResourceManager_GetActiveWorld();
    AccessMap access_map(world);
    bool is_found;
    bool result;

    if (EvaluateNeedForRadar(access_map, task)) {
        Point location;
        int32_t access_map_value;
        int32_t best_access_map_value{INT32_MAX};

        AILOG(log, "Find site for radar.");

        MouseEvent::ProcessInput();

        is_found = false;

        for (location.x = 1; location.x < ResourceManager_MapSize.x - 1; ++location.x) {
            for (location.y = 1; location.y < ResourceManager_MapSize.y - 1; ++location.y) {
                if (access_map(location.x, location.y)) {
                    access_map_value = 255 - access_map(location.x, location.y);

                    access_map_value =
                        (access_map_value << 16) + Access_GetApproximateDistance(location, building_site);

                    if (!is_found || access_map_value < best_access_map_value) {
                        is_found = true;
                        site = location;
                        best_access_map_value = access_map_value;
                    }
                }
            }
        }

        result = is_found;

    } else {
        result = false;
    }

    return result;
}

void TaskManageBuildings::AddCreateOrder(TaskCreateBuilding* task) {
    tasks.PushBack(*task);
    TaskManager.AppendTask(*task);
}

bool TaskManageBuildings::FindDefenseSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site, int32_t value,
                                          uint16_t task_priority) {
    bool result;

    AILOG(log, "Find defense site for {}.", ResourceManager_GetUnit(unit_type).GetSingularName().data());

    if (ResourceManager_MapSize.x > 0 && ResourceManager_MapSize.y > 0) {
        uint16_t** construction_map = CreateMap();
        bool is_site_found = false;
        Point location;
        const World* world = ResourceManager_GetActiveWorld();
        AccessMap access_map(world);
        Point target_location = AiPlayer_Teams[m_team].GetTargetLocation();
        int32_t access_map_value;
        int32_t best_access_map_value = 1;
        int32_t distance;
        int32_t minimum_distance{INT32_MAX};

        MouseEvent::ProcessInput();

        MarkDefenseSites(construction_map, access_map, task, value);
        ClearBuildingAreas(construction_map, task);
        ClearPlannedBuildings(construction_map, task, unit_type, task_priority);
        LimitBlockSize(construction_map, 1);

        MouseEvent::ProcessInput();

        ClearDefenseSites(access_map, unit_type, task, task_priority);

        if (unit_type == GUNTURRT || unit_type == ARTYTRRT) {
            ClearDefenseSites(access_map, ANTIMSSL, task, task_priority);
        }

        for (location.x = 1; location.x < ResourceManager_MapSize.x - 1; ++location.x) {
            for (location.y = 1; location.y < ResourceManager_MapSize.y - 1; ++location.y) {
                access_map_value = access_map(location.x, location.y);

                if (access_map_value >= best_access_map_value && construction_map[location.x][location.y] < AREA_FREE) {
                    distance = Access_GetApproximateDistance(location, target_location);

                    if (!is_site_found || access_map_value > best_access_map_value || distance < minimum_distance) {
                        if (IsSafeSite(construction_map, location, unit_type)) {
                            is_site_found = true;
                            site = location;
                            minimum_distance = distance;
                            best_access_map_value = access_map_value;
                        }
                    }
                }
            }
        }

        if (is_site_found) {
            Rect bounds;

            rect_init(&bounds, site.x, site.y, site.x + 1, site.y + 1);

            for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                if ((*it).GetUnitType() == BRIDGE && !(*it).Task_vfunc28()) {
                    Point position = (*it).DeterminePosition();

                    if (Access_IsInsideBounds(&bounds, &position)) {
                        (*it).RemoveSelf();
                    }
                }
            }
        }

        DeleteMap(construction_map);

        result = is_site_found;

    } else {
        result = false;
    }

    return result;
}

bool TaskManageBuildings::ChangeSite(TaskCreateBuilding* task, Point& site) {
    ResourceID unit_type = task->GetUnitType();
    bool is_site_found;

    AILOG(log, "Change site for {}", ResourceManager_GetUnit(unit_type).GetSingularName().data());

    if (unit_type == RADAR) {
        is_site_found = FindSiteForRadar(task, site);

    } else if (unit_type == GUNTURRT || unit_type == ANTIMSSL || unit_type == ARTYTRRT || unit_type == ANTIAIR) {
        is_site_found = FindDefenseSite(unit_type, task, site, 5, task->GetPriority());

    } else {
        is_site_found = FindSite(unit_type, task, site, task->GetPriority());
    }

    if (is_site_found) {
        AILOG_LOG(log, "new site [{},{}].", site.x + 1, site.y + 1);

    } else {
        AILOG_LOG(log, "No site found.");
    }

    return is_site_found;
}
