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
#include "aiplayer.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "inifile.hpp"
#include "mouseevent.hpp"
#include "resource_manager.hpp"
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

TaskManageBuildings::TaskManageBuildings(unsigned short team, Point site) : Task(team, nullptr, 0x1D00) {
    building_site = site;
}

TaskManageBuildings::~TaskManageBuildings() {}

void TaskManageBuildings::BuildBridge(Point site, Task* task) {}

void TaskManageBuildings::FillMap(unsigned short** construction_map, int ulx, int uly, int lrx, int lry,
                                  int fill_value) {
    ulx = std::max(0, ulx);
    uly = std::max(0, uly);
    lrx = std::min(static_cast<int>(ResourceManager_MapSize.x), lrx);
    lry = std::min(static_cast<int>(ResourceManager_MapSize.y), lry);

    for (int x = ulx; x < lrx; ++x) {
        for (int y = uly; y < lry; ++y) {
            construction_map[x][y] = fill_value;
        }
    }
}

unsigned short** TaskManageBuildings::CreateMap() {
    unsigned short** construction_map = new (std::nothrow) unsigned short*[ResourceManager_MapSize.x];

    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        construction_map[i] = new (std::nothrow) unsigned short[ResourceManager_MapSize.y];
    }

    FillMap(construction_map, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y, AREA_FREE);

    return construction_map;
}

void TaskManageBuildings::DeleteMap(unsigned short** construction_map) {
    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        delete[] construction_map[i];
    }

    delete[] construction_map;
}

void TaskManageBuildings::MarkMiningAreas(unsigned short** construction_map) {
    unsigned short hash_team_id = UnitsManager_TeamInfo[team].team_units->hash_team_id;
    bool mining_station_found = false;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).unit_type != WTRPLTFM && (*it).unit_type != BRIDGE) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx - 10, bounds.uly - 10, bounds.lrx + 10, bounds.lry + 10, AREA_BLOCKED);

            if ((*it).unit_type == MININGST) {
                mining_station_found = true;
            }
        }
    }

    for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
            if (construction_map[x][y] == AREA_BLOCKED ||
                (!mining_station_found && construction_map[x][y] != AREA_RESERVED)) {
                unsigned short cargo = ResourceManager_CargoMap[ResourceManager_MapSize.x * y + x];

                if ((cargo & hash_team_id) && (cargo & 0x1F) >= 8) {
                    FillMap(construction_map, x - 1, y - 1, x + 1, y + 1, AREA_RESERVED);

                } else {
                    construction_map[x][y] == AREA_FREE;
                }
            }
        }
    }
}

void TaskManageBuildings::MarkBuildingAreas(unsigned short** construction_map, int area_expanse, int area_offset) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).unit_type != WTRPLTFM && (*it).unit_type != BRIDGE) {
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
        if ((*it).unit_type != CNCT_4W && (*it).unit_type != WTRPLTFM && (*it).unit_type != BRIDGE) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx - area_offset - 1, bounds.uly - area_offset - 1, bounds.lrx + 2,
                    bounds.lry + 2, AREA_FREE);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).unit_type != CNCT_4W && (*it).unit_type != WTRPLTFM && (*it).unit_type != BRIDGE) {
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

void TaskManageBuildings::ClearBuildingAreas(unsigned short** construction_map, TaskCreateBuilding* task) {
    unsigned short** damage_potential_map =
        AiPlayer_Teams[team].GetDamagePotentialMap(ENGINEER, CAUTION_LEVEL_AVOID_ALL_DAMAGE, 0x01);

    if (damage_potential_map) {
        for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
            for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
                if (damage_potential_map[x][y] > 0) {
                    construction_map[x][y] = AREA_DANGERZONE;
                }
            }
        }
    }

    if (!TaskManageBuildings_IsUnitAvailable(team, &UnitsManager_MobileLandSeaUnits, MINELAYR)) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).team == team && (*it).unit_type == LANDMINE) {
                construction_map[(*it).grid_x][(*it).grid_y] = AREA_OBSTRUCTED;
            }
        }
    }

    if (!TaskManageBuildings_IsUnitAvailable(team, &UnitsManager_MobileLandSeaUnits, SEAMNLYR)) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).team == team && (*it).unit_type == SEAMINE) {
                construction_map[(*it).grid_x][(*it).grid_y] = AREA_OBSTRUCTED;
            }
        }
    }

    if (!TaskManageBuildings_IsUnitAvailable(team, &UnitsManager_MobileLandSeaUnits, BULLDOZR)) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).unit_type == LRGRUBLE || (*it).unit_type == SMLRUBLE) {
                Rect bounds;

                (*it).GetBounds(&bounds);

                FillMap(construction_map, bounds.ulx, bounds.uly, bounds.lrx, bounds.lry, AREA_OBSTRUCTED);
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && !(*it).unit_type == CNCT_4W) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            FillMap(construction_map, bounds.ulx, bounds.uly, bounds.lrx, bounds.lry, AREA_BUILT_IN);
        }
    }
}

void TaskManageBuildings::ClearPathways(unsigned short** construction_map, Rect bounds, int unit_size) {
    Point location(bounds.ulx - 1, bounds.lry);
    Point directions[8];
    int directions_index = 0;
    int range = bounds.lrx - bounds.ulx;

    for (int direction = 0; direction < 8; direction += 2) {
        for (int i = 0; i < range; ++i) {
            location += Paths_8DirPointsArray[direction];

            if (location.x >= 0 && location.x < ResourceManager_MapSize.x && location.y >= 0 &&
                location.y < ResourceManager_MapSize.y) {
                if (construction_map[location.x][location.y] < AREA_DANGERZONE) {
                    directions[directions_index] = location;
                    ++directions_index;
                }
            }
        }

        location += Paths_8DirPointsArray[direction];
    }

    if (directions_index == 1) {
        construction_map[directions[0].x][directions[0].y] = AREA_PATHWAY;

    } else if (directions_index == 2 && unit_size == 2 &&
               (directions[0].x == directions[1].x || directions[0].y == directions[1].y)) {
        construction_map[directions[0].x][directions[0].y] = AREA_PATHWAY;
        construction_map[directions[1].x][directions[1].y] = AREA_PATHWAY;
    }
}

void TaskManageBuildings::ClearPlannedBuildings(unsigned short** construction_map, TaskCreateBuilding* task,
                                                ResourceID unit_type, unsigned short task_flags) {
    int unit_size = UnitsManager_BaseUnits[unit_type].flags & BUILDING ? 2 : 1;

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

    bool is_water = UnitsManager_BaseUnits[unit_type].land_type == SURFACE_TYPE_WATER;
    bool needs_land_access =
        unit_type == LIGHTPLT || unit_type == LANDPLT || unit_type == DEPOT || unit_type == TRAINHAL;

    if (task_flags <= 0x1300) {
        needs_land_access = false;
    }

    for (int x = 0; x < ResourceManager_MapSize.x; ++x) {
        for (int y = 0; y < ResourceManager_MapSize.y; ++y) {
            int surface_type = ResourceManager_MapSurfaceMap[ResourceManager_MapSize.x * y + x];
            int marker_type;

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
        if ((*it).unit_type == WTRPLTFM) {
            if (is_water) {
                construction_map[(*it).grid_x][(*it).grid_y] = AREA_BUILT_IN;

            } else {
                int marker_type = construction_map[(*it).grid_x][(*it).grid_y];

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

        if ((*it).unit_type == BRIDGE) {
            construction_map[(*it).grid_x][(*it).grid_y] = AREA_OBSTRUCTED;
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && ((*it).unit_type == BARRACKS || (*it).unit_type == DEPOT || (*it).unit_type == DOCK ||
                                   (*it).unit_type == TRAINHAL || (*it).unit_type == LIGHTPLT ||
                                   (*it).unit_type == LANDPLT || (*it).unit_type == SHIPYARD)) {
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

bool TaskManageBuildings::FillBlockMap(int ul1, int lr1, int ul2, int lr2, signed char* address_a,
                                       signed char* address_b) {
    int distance = ul1 - ul2;

    if (ul1 > ul2 && distance > *address_a) {
        *address_a = distance;
    }

    distance = lr2 - lr1;

    if (lr1 < lr2 && distance > *address_b) {
        *address_b = distance;
    }

    return lr1 + *address_b + *address_a - ul1 <= 6;
}

void TaskManageBuildings::ClearBlocks(unsigned short** construction_map, struct Block** block_map, Rect bounds,
                                      int area_marker, int unit_size) {
    Point position;
    bool flag;
    Rect limits;
    Rect limits2;

    limits.ulx = std::max(0, bounds.ulx - unit_size);
    limits.uly = std::max(0, bounds.uly - unit_size);
    limits.lrx = std::min(bounds.lrx + 1, static_cast<int>(ResourceManager_MapSize.x));
    limits.lry = std::min(bounds.lry + 1, static_cast<int>(ResourceManager_MapSize.y));

    for (int x = limits.ulx; x < limits.lrx; ++x) {
        for (int y = limits.uly; y < limits.lry; ++y) {
            if (construction_map[x][y] == AREA_BLOCKED) {
                limits2.ulx = std::max(0, x - 1);
                limits2.uly = std::max(0, y - 1);
                limits2.lrx = std::min(static_cast<int>(ResourceManager_MapSize.x), x + 1 + unit_size);
                limits2.lry = std::min(static_cast<int>(ResourceManager_MapSize.y), y + 1 + unit_size);

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

void TaskManageBuildings::LimitBlockSize(unsigned short** construction_map, int unit_size) {
    Point site;
    Block block;
    Block** block_map;
    int marker;

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

int TaskManageBuildings::EvaluateSiteValue(unsigned short** construction_map, Point site, ResourceID unit_type) {
    int cargo_total;

    if (unit_type == MININGST) {
        short raw;
        short gold;
        short fuel;

        Survey_GetTotalResourcesInArea(site.x, site.y, 1, &raw, &gold, &fuel, false, team);

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
        int marker;

        cargo_total = -TaskManager_GetDistance(site, building_site);
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
    int cargo_total;
    short raw;
    short gold;
    short fuel;
    bool result;

    Survey_GetTotalResourcesInArea(site.x, site.y, 1, &raw, &gold, &fuel, false, team);

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

bool TaskManageBuildings::IsViableMiningSite(unsigned short** construction_map, int ulx, int uly, int lrx, int lry) {
    Point site1;
    Point site2;
    unsigned short hash_team_id;

    ulx = std::max(1, ulx);
    uly = std::max(1, uly);
    lrx = std::min(lrx, ResourceManager_MapSize.x - 1);
    lry = std::min(lry, ResourceManager_MapSize.y - 1);

    hash_team_id = UnitsManager_TeamInfo[team].team_units->hash_team_id;

    for (site1.x = ulx; site1.x < lrx; ++site1.x) {
        for (site1.y = uly; site1.y < lry; ++site1.y) {
            if (construction_map[site1.x][site1.y] <= AREA_FREE) {
                unsigned short cargo_level = ResourceManager_CargoMap[site1.y * ResourceManager_MapSize.x + site1.x];

                if ((cargo_level & 0x1F) >= 8) {
                    for (site2.x = site1.x - 1; site2.x <= site1.x; ++site2.x) {
                        for (site2.y = site1.y - 1; site2.y <= site1.y; ++site2.y) {
                            if (site2.x > 0 && site2.x < ResourceManager_MapSize.x - 1 && site2.y > 0 &&
                                site2.y < ResourceManager_MapSize.y) {
                                bool is_found = false;

                                for (int x = site2.x; x <= site2.x + 1; ++x) {
                                    for (int y = site2.y; y <= site2.y + 1; ++y) {
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

bool TaskManageBuildings::IsSafeSite(unsigned short** construction_map, Point site, ResourceID unit_type) {
    Rect site_bounds;
    Rect map_bounds;
    bool result;

    rect_init(&site_bounds, site.x, site.y, site.x + 1, site.y + 1);
    rect_init(&map_bounds, 0, 0, ResourceManager_MapSize.x, ResourceManager_MapSize.y);

    if (unit_type == MININGST && !IsFavorableMiningSite(site)) {
        result = false;

    } else {
        if (UnitsManager_BaseUnits[unit_type].flags & BUILDING) {
            ++site_bounds.lrx;
            ++site_bounds.lry;
        }

        if (site_bounds.lrx <= ResourceManager_MapSize.x && site_bounds.lry <= ResourceManager_MapSize.y) {
            int marker_index = 0;
            int markers[12];
            int marker;

            for (site.x = site_bounds.ulx; site.x < site_bounds.lrx; ++site.x) {
                for (site.y = site_bounds.uly; site.y < site_bounds.lry; ++site.y) {
                    if (construction_map[site.x][site.y] > AREA_FREE) {
                        return false;
                    }
                }
            }

            site.x = site_bounds.ulx - 1;
            site.y = site_bounds.lry;

            for (int direction = 0; direction < 8; direction += 2) {
                for (int range = 0; range < site_bounds.lrx - site_bounds.ulx + 1; ++range) {
                    site += Paths_8DirPointsArray[direction];

                    if (!Access_IsInsideBounds(&map_bounds, &site)) {
                        return false;
                    }

                    marker = construction_map[site.x][site.y];

                    if (marker == AREA_AIRZONE && unit_type != MININGST) {
                        return false;
                    }

                    if (!marker_index || markers[marker_index] != marker) {
                        markers[marker_index] = marker;
                        ++marker_index;
                    }
                }
            }

            if (marker_index > 1 && markers[0] == markers[marker_index - 1]) {
                --marker_index;
            }

            for (int i = 0; i < marker_index - 1; ++i) {
                if (markers[i + 1] >= 10) {
                    for (int j = i + 1; j < marker_index; ++j) {
                        if (markers[i] == markers[j]) {
                            return false;
                        }
                    }
                }
            }

            int site_count = 0;

            site.x = site_bounds.ulx - 1;
            site.y = site_bounds.uly;

            for (int direction = 0; direction < 8; direction += 2) {
                for (int range = 0; range < site_bounds.lrx - site_bounds.ulx; ++range) {
                    site += Paths_8DirPointsArray[direction];

                    if (construction_map[site.x][site.y] < AREA_DANGERZONE) {
                        ++site_count;
                    }
                }

                site += Paths_8DirPointsArray[direction];
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

bool TaskManageBuildings::EvaluateSite(unsigned short** construction_map, ResourceID unit_type, Point site) {
    Point position;
    int site_value;
    int best_site_value;
    bool is_site_found = false;
    bool result;

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

        if (UnitsManager_BaseUnits[unit_type].flags & BUILDING) {
            ++bounds.lrx;
            ++bounds.lry;

            for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
                if ((*it).GetUnitType() == BRIDGE && !(*it).Task_vfunc28()) {
                    Point point = (*it).DeterminePosition();

                    if (Access_IsInsideBounds(&bounds, &point)) {
                        (*it).RemoveSelf();
                    }
                }
            }

            result = true;
        }

    } else {
        result = false;
    }

    return result;
}

bool TaskManageBuildings::FindSite(ResourceID unit_type, TaskCreateBuilding* task, Point site,
                                   unsigned short task_flags) {
    unsigned short** construction_map = CreateMap();
    int unit_size = UnitsManager_BaseUnits[unit_type].flags & BUILDING ? 2 : 1;
    bool result;

    if (unit_type == MININGST) {
        MarkMiningAreas(construction_map);

    } else {
        int area_offset = unit_type == SHIPYARD ? 10 : 4;

        MarkBuildingAreas(construction_map, unit_size, area_offset);
    }

    ClearBuildingAreas(construction_map, task);
    ClearPlannedBuildings(construction_map, task, unit_type, task_flags);
    LimitBlockSize(construction_map, unit_size);
    result = EvaluateSiteValue(construction_map, site, unit_type);
    DeleteMap(construction_map);

    return result;
}

int TaskManageBuildings::GetUnitCount(ResourceID unit_type, unsigned short task_flags) {
    int unit_count = 0;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).unit_type == unit_type) {
            ++unit_count;
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == unit_type && ((*it).Task_vfunc28() || (*it).DeterminePriority(task_flags) <= 0)) {
            ++unit_count;
        }
    }

    return unit_count;
}

bool TaskManageBuildings::IsSupremeTeam(unsigned short team_) {
    int team_points = UnitsManager_TeamInfo[team_].team_points;
    bool result;

    if (team_points > 0) {
        for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
            if (UnitsManager_TeamInfo[i].team_type != TEAM_TYPE_NONE) {
                if (team_points <= UnitsManager_TeamInfo[i].team_points) {
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

int TaskManageBuildings::GetHighestGreenHouseCount(unsigned short team_) {
    int highest_greenhouse_count = 0;
    int greenhouse_counts[PLAYER_TEAM_MAX];

    memset(greenhouse_counts, 0, sizeof(greenhouse_counts));

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).unit_type == GREENHSE) {
            ++greenhouse_counts[(*it).team];
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).unit_type == CONSTRCT && (*it).orders == ORDER_BUILD && (*it).state == ORDER_STATE_11 &&
            (*it).GetConstructedUnitType() == GREENHSE) {
            ++greenhouse_counts[(*it).team];
        }
    }

    for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (i != team_) {
            highest_greenhouse_count = std::max(highest_greenhouse_count, greenhouse_counts[i]);
        }
    }

    return highest_greenhouse_count;
}

bool TaskManageBuildings::CreateBuildings(int building_demand, ResourceID unit_type, unsigned short task_flags) {
    if (Builder_IsBuildable(unit_type)) {
        building_demand -= GetUnitCount(unit_type, task_flags);

        for (int i = 0; i < building_demand; ++i) {
            if (CreateBuilding(unit_type, this, task_flags)) {
                return true;
            }
        }
    }

    return false;
}

bool TaskManageBuildings::PlanNextBuildJob() {
    bool result;

    if (units.GetCount()) {
        if (!GetUnitCount(LIGHTPLT, 0x700) && CreateBuilding(LIGHTPLT, this, 0x700)) {
            result = true;

        } else if (!GetUnitCount(LANDPLT, 0x1300) && CreateBuilding(LANDPLT, this, 0x1300)) {
            result = true;

        } else {
            int unit_count_lightplant = GetUnitCount(LIGHTPLT, 0x00);
            int unit_count_landplant = GetUnitCount(LANDPLT, 0x00);
            int unit_count_airplant = GetUnitCount(AIRPLT, 0x00);
            int unit_count_shipyard = GetUnitCount(SHIPYARD, 0x00);
            int unit_count_trainhall = GetUnitCount(TRAINHAL, 0x00);
            int total_unit_count = unit_count_lightplant + unit_count_landplant + unit_count_airplant +
                                   unit_count_shipyard + unit_count_trainhall;

            if (unit_count_lightplant > 0 && unit_count_landplant > 0) {
                if (!GetUnitCount(AIRPLT, 0x1300) && CreateBuilding(AIRPLT, this, 0x1300)) {
                    return true;
                }

                if (!GetUnitCount(HABITAT, 0xC00) && CreateBuilding(HABITAT, this, 0xC00)) {
                    return true;
                }

                if (!GetUnitCount(GREENHSE, 0x1600) && CreateBuilding(GREENHSE, this, 0x1600)) {
                    return true;
                }

                if (!GetUnitCount(RESEARCH, 0x1500) && CreateBuilding(RESEARCH, this, 0x1500)) {
                    return true;
                }

                if (unit_count_airplant > 0) {
                    if (CreateBuildings((AiPlayer_Teams[team].GetField5() * total_unit_count) / 10, GREENHSE, 0x1600)) {
                        return true;
                    }

                    if (CreateBuildings(GetUnitCount(GREENHSE, 0x1500), RESEARCH, 0x1500)) {
                        return true;
                    }

                    if (IsSupremeTeam(team) && CreateBuildings(GetHighestGreenHouseCount(team) + 1, GREENHSE, 0x1600)) {
                        return true;
                    }
                }
            }

            total_unit_count /= 3;

            switch (AiPlayer_Teams[team].GetStrategy()) {
                case AI_STRATEGY_DEFENSIVE: {
                    if (CreateBuildings(total_unit_count, LIGHTPLT, 0x1800)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_MISSILES: {
                } break;

                case AI_STRATEGY_AIR: {
                    if (!GetUnitCount(AIRPLT, 0x1300) && CreateBuilding(AIRPLT, this, 0x1300)) {
                        return true;
                    }

                    if (CreateBuildings(total_unit_count, AIRPLT, 0x1800)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_SEA: {
                    if (!GetUnitCount(SHIPYARD, 0x1300) && CreateBuilding(SHIPYARD, this, 0x1300)) {
                        return true;
                    }

                    if (CreateBuildings(total_unit_count, SHIPYARD, 0x1800)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_SCOUT_HORDE: {
                    if (CreateBuildings(total_unit_count, LIGHTPLT, 0x1800)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_TANK_HORDE: {
                    if (CreateBuildings(total_unit_count, LANDPLT, 0x1800)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_FAST_ATTACK: {
                    if (CreateBuildings(total_unit_count / 2, LIGHTPLT, 0x1800)) {
                        return true;
                    }

                    if (CreateBuildings(total_unit_count / 2, LANDPLT, 0x1800)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_COMBINED_ARMS: {
                    if (CreateBuildings(total_unit_count, LANDPLT, 0x1800)) {
                        return true;
                    }
                } break;

                case AI_STRATEGY_ESPIONAGE: {
                    if (!GetUnitCount(TRAINHAL, 0x1300) && CreateBuilding(TRAINHAL, this, 0x1300)) {
                        return true;
                    }

                    if (CreateBuildings(total_unit_count, TRAINHAL, 0x1800)) {
                        return true;
                    }
                } break;
            }

            if (GetUnitCount(LANDPLT, 0x00) && !GetUnitCount(DEPOT, 0x1400) && CreateBuilding(DEPOT, this, 0x1400)) {
                result = true;

            } else if (GetUnitCount(AIRPLT, 0x00) && !GetUnitCount(HANGAR, 0x1400) &&
                       CreateBuilding(HANGAR, this, 0x1400)) {
                result = true;

            } else if (GetUnitCount(SHIPYARD, 0x00) && !GetUnitCount(DOCK, 0x1400) &&
                       CreateBuilding(DOCK, this, 0x1400)) {
                result = true;

            } else if (GetUnitCount(TRAINHAL, 0x00) && !GetUnitCount(BARRACKS, 0x1D00) &&
                       CreateBuilding(BARRACKS, this, 0x1D00)) {
                result = true;

            } else if (unit_count_airplant > 0 && !GetUnitCount(SHIPYARD, 0x1300) &&
                       CreateBuilding(SHIPYARD, this, 0x1300)) {
                result = true;

            } else {
                result = false;
            }
        }

    } else {
        result = false;
    }

    return result;
}

void TaskManageBuildings::UpdateCargoDemand(short* limit, short* material, int max_mining) {
    short demand = -(*material);

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
    cargo_demand.Init();

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && ((*it).unit_type == ENGINEER || (*it).unit_type == CONSTRCT)) {
            cargo_demand.raw += (*it).storage;

            if ((*it).unit_type == ENGINEER) {
                cargo_demand.raw -= 20;

            } else {
                cargo_demand.raw -= 40;
            }
        }
    }

    cargo_demand.raw = (cargo_demand.raw - 50) / 10;
    cargo_demand.fuel -= 5;

    if (Builder_IsBuildable(COMMTWR)) {
        cargo_demand.gold -= Cargo_GetGoldConsumptionRate(COMMTWR);
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team) {
            cargo_demand.raw -= Cargo_GetRawConsumptionRate((*it).unit_type, 1);
            cargo_demand.fuel -= Cargo_GetFuelConsumptionRate((*it).unit_type);
            cargo_demand.gold -= Cargo_GetGoldConsumptionRate((*it).unit_type);

            switch (UnitsManager_BaseUnits[(*it).unit_type].cargo_type) {
                case CARGO_TYPE_RAW: {
                    cargo_demand.raw += (*it).storage / 10;
                } break;

                case CARGO_TYPE_FUEL: {
                    cargo_demand.fuel += (*it).storage / 10;
                } break;

                case CARGO_TYPE_GOLD: {
                    cargo_demand.gold += (*it).storage / 20;
                } break;
            }
        }
    }

    int cargo_gold = cargo_demand.gold;

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
        if ((*it).unit_type == MININGST && (*it).team == team) {
            short capacity_limit = 16;

            UpdateCargoDemand(&capacity_limit, &cargo_demand.fuel, (*it).fuel_mining_max);
            UpdateCargoDemand(&capacity_limit, &cargo_demand.raw, (*it).raw_mining_max);

            cargo_gold += std::min(capacity_limit, static_cast<short>((*it).gold_mining_max));

            UpdateCargoDemand(&capacity_limit, &cargo_demand.gold, (*it).gold_mining_max);
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == MININGST) {
            short capacity_limit = 16;
            Rect bounds;
            short raw;
            short fuel;
            short gold;

            (*it).GetBounds(&bounds);

            Survey_GetTotalResourcesInArea(bounds.ulx, bounds.uly, 1, &raw, &gold, &fuel, false, team);

            UpdateCargoDemand(&capacity_limit, &cargo_demand.fuel, fuel);
            UpdateCargoDemand(&capacity_limit, &cargo_demand.raw, raw);

            if ((*it).Task_vfunc28()) {
                cargo_gold += std::min(capacity_limit, gold);
            }

            UpdateCargoDemand(&capacity_limit, &cargo_demand.gold, gold);
        }
    }

    if (Cargo_GetGoldConsumptionRate(COMMTWR) <= cargo_gold) {
        CreateBuilding(COMMTWR, this, 0x1500);
    }
}

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

void TaskManageBuildings::AddUnit(UnitInfo& unit) {
    if (unit.flags & STATIONARY) {
        units.PushBack(unit);

        if (unit.orders == ORDER_BUILD || unit.orders == ORDER_HALT_BUILDING) {
            SmartPointer<TaskCreateUnit> create_unit_task(new (std::nothrow) TaskCreateUnit(&unit, this));

            TaskManager.AppendTask(*create_unit_task);
        }

    } else {
        SmartPointer<TaskCreateBuilding> create_building_task(new (std::nothrow) TaskCreateBuilding(&unit, this));

        tasks.PushBack(*create_building_task);

        TaskManager.AppendTask(*create_building_task);
    }
}

void TaskManageBuildings::Begin() {
    if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        if (Builder_IsBuildable(ANTIAIR)) {
            SmartPointer<TaskDefenseAssistant> task(new (std::nothrow) TaskDefenseAssistant(this, ANTIAIR));

            TaskManager.AppendTask(*task);
        }
    }

    if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_APPRENTICE) {
        if (Builder_IsBuildable(GUNTURRT)) {
            SmartPointer<TaskDefenseAssistant> task(new (std::nothrow) TaskDefenseAssistant(this, GUNTURRT));

            TaskManager.AppendTask(*task);
        }
    }

    if (ini_get_setting(INI_OPPONENT) >= OPPONENT_TYPE_AVERAGE) {
        if (Builder_IsBuildable(ANTIMSSL)) {
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

    if (Builder_IsBuildable(HABITAT)) {
        SmartPointer<TaskHabitatAssistant> task(new (std::nothrow) TaskHabitatAssistant(this));

        TaskManager.AppendTask(*task);
    }

    {
        SmartPointer<TaskConnectionAssistant> task(new (std::nothrow) TaskConnectionAssistant(this));

        TaskManager.AppendTask(*task);
    }
}

void TaskManageBuildings::BeginTurn() {
    MouseEvent::ProcessInput();
    CheckNeeds();

    if (!GetField8()) {
        TaskManager.AppendReminder(new (std::nothrow) class RemindTurnEnd(*this));
    }
}

void TaskManageBuildings::ChildComplete(Task* task) {
    if (task->GetType() == TaskType_TaskCreateBuilding) {
        tasks.Remove(*dynamic_cast<TaskCreateBuilding*>(task));
    }
}

void TaskManageBuildings::EndTurn() {
    Cargo cargo;
    Cargo capacity;

    int raw_mining_max = 0;
    int fuel_mining_max = 0;
    int gold_mining_max = 0;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).unit_type == MININGST) {
            raw_mining_max += (*it).raw_mining_max;
            fuel_mining_max += (*it).fuel_mining_max;
            gold_mining_max += (*it).gold_mining_max;
        }

        switch (UnitsManager_BaseUnits[(*it).unit_type].cargo_type) {
            case CARGO_TYPE_RAW: {
                cargo.raw += (*it).storage;
                capacity.raw += (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            } break;

            case CARGO_TYPE_FUEL: {
                cargo.fuel += (*it).storage;
                capacity.fuel += (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            } break;

            case CARGO_TYPE_GOLD: {
                cargo.gold += (*it).storage;
                capacity.gold += (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
            } break;
        }
    }

    TeamUnits* team_units = UnitsManager_TeamInfo[team].team_units;

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == MININGST && (*it).Task_vfunc28()) {
            Rect bounds;
            short raw;
            short fuel;
            short gold;

            (*it).GetBounds(&bounds);

            Survey_GetTotalResourcesInArea(bounds.ulx, bounds.uly, 1, &raw, &gold, &fuel, false, team);

            raw_mining_max += raw;
            fuel_mining_max += fuel;
            gold_mining_max += gold;
        }

        switch (UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type) {
            case CARGO_TYPE_RAW: {
                capacity.raw += team_units->GetCurrentUnitValues((*it).GetUnitType())->GetAttribute(ATTRIB_STORAGE);
            } break;

            case CARGO_TYPE_FUEL: {
                capacity.fuel += team_units->GetCurrentUnitValues((*it).GetUnitType())->GetAttribute(ATTRIB_STORAGE);
            } break;

            case CARGO_TYPE_GOLD: {
                capacity.gold += team_units->GetCurrentUnitValues((*it).GetUnitType())->GetAttribute(ATTRIB_STORAGE);
            } break;
        }
    }

    if (raw_mining_max > 0 && (cargo.raw >= (capacity.raw * 3) / 4) && capacity.raw < 500) {
        CreateBuilding(ADUMP, this, 0xA00);
    }

    if (fuel_mining_max > 0 && (cargo.fuel >= (capacity.fuel * 3) / 4) && capacity.fuel < 500) {
        CreateBuilding(FDUMP, this, 0xA00);
    }

    if (gold_mining_max > 0 && (cargo.gold >= (capacity.gold * 3) / 4) && capacity.gold < 500 &&
        Builder_IsBuildable(GOLDSM)) {
        CreateBuilding(GOLDSM, this, 0xA00);
    }
}

void TaskManageBuildings::RemoveSelf() {
    units.Clear();
    tasks.Clear();

    parent = nullptr;

    TaskManager.RemoveTask(*this);
}

void TaskManageBuildings::RemoveUnit(UnitInfo& unit) { units.Remove(unit); }

void TaskManageBuildings::Task_vfunc23(UnitInfo& unit) { units.Remove(unit); }

bool TaskManageBuildings::CreateBuilding(ResourceID unit_type, Task* task, unsigned short task_flags) {}

bool TaskManageBuildings::ReconnectBuildings() {}

void TaskManageBuildings::CheckWorkers() {
    int total_consumption = 0;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        int consumption_rate = Cargo_GetLifeConsumptionRate((*it).unit_type);

        if (consumption_rate > 0) {
            total_consumption += consumption_rate;
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).Task_vfunc28()) {
            int consumption_rate = Cargo_GetLifeConsumptionRate((*it).GetUnitType());

            if (consumption_rate > 0) {
                total_consumption += consumption_rate;
            }
        }
    }

    {
        int consumption_rate = -Cargo_GetLifeConsumptionRate(HABITAT);
        int habitat_demand = (total_consumption + consumption_rate - 1) / consumption_rate;

        CreateBuildings(habitat_demand, HABITAT, 0xC00);
    }
}

bool TaskManageBuildings::CheckPower() {
    int power_consumption = 0;
    int power_generation = 0;
    int total_power_generation = 0;
    int total_storage = 0;

    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        int power_consumption_rate = Cargo_GetPowerConsumptionRate((*it).unit_type);

        if (power_consumption_rate >= 0) {
            power_consumption += power_consumption_rate;

        } else {
            power_generation -= power_consumption_rate;
        }

        if (UnitsManager_BaseUnits[(*it).unit_type].cargo_type == CARGO_TYPE_AIR) {
            total_storage += (*it).storage;
        }
    }

    int turns_to_build = BuildMenu_GetTurnsToBuild(POWERSTN, team);

    total_power_generation = power_generation;

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).GetUnitType() == POWGEN) {
            ++total_power_generation;
        }

        if ((*it).Task_vfunc28() && (*it).GetUnitType() == POWERSTN) {
            int power_consumption_rate = (*it).EstimateBuildTime();

            turns_to_build = std::min(turns_to_build, power_consumption_rate);
        }
    }

    for (SmartList<TaskCreateBuilding>::Iterator it = tasks.Begin(); it != tasks.End(); ++it) {
        if ((*it).Task_vfunc28() && (*it).EstimateBuildTime() < turns_to_build) {
            int power_consumption_rate = Cargo_GetPowerConsumptionRate((*it).GetUnitType());

            if (power_consumption_rate > 0) {
                power_consumption += power_consumption_rate;
            }
        }
    }

    {
        int power_generation_rate1 = -Cargo_GetPowerConsumptionRate(POWERSTN);
        int power_generation_rate2 = -Cargo_GetPowerConsumptionRate(POWGEN);
        int fuel_consumption_rate1 = Cargo_GetFuelConsumptionRate(POWERSTN);
        int fuel_consumption_rate2 = Cargo_GetFuelConsumptionRate(POWGEN);

        if (total_storage < 50) {
            int power_station_demand =
                power_generation_rate1 - ((fuel_consumption_rate1 * power_generation_rate2) / fuel_consumption_rate2);

            power_station_demand = (power_consumption + power_station_demand) / power_generation_rate1;

            if (CreateBuildings(power_station_demand, POWERSTN, 0xB00)) {
                return true;
            }
        }

        if (power_consumption > total_power_generation && CreateBuilding(POWGEN, this, 0xB00)) {
            return true;

        } else {
            return false;
        }
    }
}

bool TaskManageBuildings::FindSiteForRadar(TaskCreateBuilding* task, Point& site) {}

void TaskManageBuildings::AddCreateOrder(TaskCreateBuilding* task) {
    tasks.PushBack(*task);
    TaskManager.AppendTask(*task);
}

bool TaskManageBuildings::FindDefenseSite(ResourceID unit_type, TaskCreateBuilding* task, Point& site) {}

bool TaskManageBuildings::ChangeSite(TaskCreateBuilding* task, Point& site) {}
