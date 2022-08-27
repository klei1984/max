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

#include "terrainmap.hpp"

#include "access.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskupdateterrain.hpp"
#include "units_manager.hpp"

#define TERRAINMAP_PATH_PROCESSED (SHRT_MAX + 1)
#define TERRAINMAP_PATH_MAX_DISTANCE SHRT_MAX
#define TERRAINMAP_PATH_BLOCKED (TERRAINMAP_PATH_PROCESSED | TERRAINMAP_PATH_MAX_DISTANCE)

TerrainMap::TerrainMap() : water_map(nullptr), land_map(nullptr), dimension_x(0) {}

TerrainMap::~TerrainMap() { Deinit(); }

void TerrainMap::Init() {
    if (!water_map) {
        dimension_x = ResourceManager_MapSize.x;

        water_map = new (std::nothrow) unsigned short*[ResourceManager_MapSize.x];
        land_map = new (std::nothrow) unsigned short*[ResourceManager_MapSize.x];

        for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
            water_map[i] = new (std::nothrow) unsigned short[ResourceManager_MapSize.y];
            land_map[i] = new (std::nothrow) unsigned short[ResourceManager_MapSize.y];
        }

        for (int j = 0; j < ResourceManager_MapSize.y; ++j) {
            for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
                if (ResourceManager_MapSurfaceMap[i + j * ResourceManager_MapSize.x] == SURFACE_TYPE_LAND) {
                    water_map[i][j] = TERRAINMAP_PATH_BLOCKED;

                } else {
                    water_map[i][j] = TERRAINMAP_PATH_MAX_DISTANCE;
                }

                if (ResourceManager_MapSurfaceMap[i + j * ResourceManager_MapSize.x] == SURFACE_TYPE_WATER) {
                    land_map[i][j] = TERRAINMAP_PATH_BLOCKED;

                } else {
                    land_map[i][j] = TERRAINMAP_PATH_MAX_DISTANCE;
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).unit_type == BRIDGE || (*it).unit_type == WTRPLTFM) {
                if ((*it).orders != ORDER_IDLE && (*it).hits > 0) {
                    water_map[(*it).grid_x][(*it).grid_y] = TERRAINMAP_PATH_BLOCKED;

                    if ((*it).unit_type == WTRPLTFM) {
                        land_map[(*it).grid_x][(*it).grid_y] = TERRAINMAP_PATH_MAX_DISTANCE;
                    }
                }
            }
        }

        for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
             it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).orders != ORDER_IDLE && (*it).hits > 0 && (*it).unit_type != CNCT_4W) {
                Rect bounds;

                (*it).GetBounds(&bounds);

                for (int i = bounds.ulx; i < bounds.lrx; ++i) {
                    for (int j = bounds.uly; j < bounds.lry; ++j) {
                        water_map[i][j] = TERRAINMAP_PATH_MAX_DISTANCE;
                        land_map[i][j] = TERRAINMAP_PATH_MAX_DISTANCE;
                    }
                }
            }
        }
    }
}

void TerrainMap::Deinit() {
    if (water_map) {
        for (int i = 0; i < dimension_x; ++i) {
            delete[] water_map[i];
            delete[] land_map[i];
        }

        delete[] water_map;
        delete[] land_map;

        water_map = nullptr;
        land_map = nullptr;
    }
}

int TerrainMap::TerrainMap_sub_68EEF(unsigned short** map, Point location) {
    unsigned short stored_distance;
    int result;

    stored_distance = map[location.x][location.y] & TERRAINMAP_PATH_MAX_DISTANCE;

    if (stored_distance >= TERRAINMAP_PATH_MAX_DISTANCE) {
        Point position;
        int shortest_distance;

        position = location;
        shortest_distance = TERRAINMAP_PATH_MAX_DISTANCE;

        if (((timer_get_stamp32() - Paths_LastTimeStamp) <= Paths_TimeLimit)) {
            for (int i = 1; i * i < shortest_distance; ++i) {
                --position.x;
                ++position.y;

                for (int direction = 0; direction < 8; direction += 2) {
                    for (int j = 0; j < i * 2; ++j) {
                        position += Paths_8DirPointsArray[direction];

                        if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                            position.y < ResourceManager_MapSize.y) {
                            if (map[position.x][position.y] & TERRAINMAP_PATH_PROCESSED) {
                                int distance;

                                distance = Access_GetDistance(position, location);

                                if (distance < shortest_distance) {
                                    shortest_distance = distance;
                                }
                            }
                        }
                    }
                }
            }

            map[location.x][location.y] = (map[location.x][location.y] & TERRAINMAP_PATH_PROCESSED) | shortest_distance;

            result = shortest_distance;

        } else {
            result = 0;
        }

    } else {
        result = stored_distance;
    }

    return result;
}

void TerrainMap::SetTerrain(unsigned short** map, Point location) {
    Point position;
    bool flag;
    int distance;

    position = location;
    flag = true;

    if (!(map[location.x][location.y] & TERRAINMAP_PATH_PROCESSED)) {
        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                TaskManager.AddTask(*new (std::nothrow) TaskUpdateTerrain(team));

                map[location.x][location.y] = TERRAINMAP_PATH_BLOCKED;

                for (int i = 1; flag; ++i) {
                    --position.x;
                    ++position.y;

                    flag = false;

                    for (int direction = 0; direction < 8; direction += 2) {
                        for (int j = 0; j < i * 2; ++j) {
                            position += Paths_8DirPointsArray[direction];

                            if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                                position.y < ResourceManager_MapSize.y) {
                                distance = Access_GetDistance(position, location);

                                if ((map[position.x][position.y] & TERRAINMAP_PATH_MAX_DISTANCE) > distance) {
                                    if ((map[position.x][position.y] & TERRAINMAP_PATH_MAX_DISTANCE) !=
                                        TERRAINMAP_PATH_MAX_DISTANCE) {
                                        flag = true;
                                    }

                                    map[position.x][position.y] =
                                        (map[position.x][position.y] & TERRAINMAP_PATH_PROCESSED) | distance;
                                }
                            }
                        }
                    }
                }

                return;
            }
        }
    }
}

void TerrainMap::ClearTerrain(unsigned short** map, Point location) {
    Point position;
    bool flag;
    int distance;

    position = location;
    flag = true;

    if (map[location.x][location.y] & TERRAINMAP_PATH_PROCESSED) {
        for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                TaskManager.AddTask(*new (std::nothrow) TaskUpdateTerrain(team));

                map[location.x][location.y] = TERRAINMAP_PATH_MAX_DISTANCE;

                for (int i = 1; flag; ++i) {
                    --position.x;
                    ++position.y;

                    flag = false;

                    for (int direction = 0; direction < 8; direction += 2) {
                        for (int j = 0; j < i * 2; ++j) {
                            position += Paths_8DirPointsArray[direction];

                            if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                                position.y < ResourceManager_MapSize.y) {
                                distance = Access_GetDistance(position, location);

                                if ((map[position.x][position.y] & TERRAINMAP_PATH_MAX_DISTANCE) == distance) {
                                    map[position.x][position.y] =
                                        (map[position.x][position.y] & TERRAINMAP_PATH_PROCESSED) |
                                        TERRAINMAP_PATH_MAX_DISTANCE;

                                    flag = true;
                                }
                            }
                        }
                    }
                }

                return;
            }
        }
    }
}

int TerrainMap::TerrainMap_sub_690D6(Point location, int surface_type) {
    int result;

    Init();

    if (!(surface_type & SURFACE_TYPE_WATER)) {
        result = TerrainMap_sub_68EEF(water_map, location);

    } else if (!(surface_type & SURFACE_TYPE_LAND)) {
        result = TerrainMap_sub_68EEF(land_map, location);

    } else {
        result = 0;
    }

    return result;
}

void TerrainMap::UpdateTerrain(Point location, int surface_type) {
    if (water_map) {
        if (surface_type & SURFACE_TYPE_LAND) {
            SetTerrain(water_map, location);

        } else {
            ClearTerrain(water_map, location);
        }

        if (surface_type & SURFACE_TYPE_WATER) {
            SetTerrain(land_map, location);

        } else {
            ClearTerrain(land_map, location);
        }
    }
}
