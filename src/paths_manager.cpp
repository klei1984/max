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

#include "paths_manager.hpp"

#include "access.hpp"
#include "accessmap.hpp"
#include "ai.hpp"
#include "aiplayer.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "pathfill.hpp"
#include "resource_manager.hpp"
#include "searcher.hpp"
#include "units_manager.hpp"
#include "zonewalker.hpp"

class PathsManager {
    static unsigned char **PathsManager_AccessMap;

    SmartPointer<PathRequest> request;
    SmartList<PathRequest> requests;
    unsigned int time_stamp;
    unsigned int elapsed_time;
    Searcher *forward_searcher;
    Searcher *backward_searcher;

    void CompleteRequest(GroundPath *path);

    friend unsigned char **PathsManager_GetAccessMap();

public:
    PathsManager();
    ~PathsManager();

    void PushBack(PathRequest &object);
    void PushFront(PathRequest &object);
    void Clear();
    int GetRequestCount(unsigned short team) const;
    void RemoveRequest(PathRequest *path_request);
    void RemoveRequest(UnitInfo *unit);
    void EvaluateTiles();
    bool HasRequest(UnitInfo *unit) const;
    bool Init(UnitInfo *unit);
    void ProcessRequest();
};

unsigned char **PathsManager::PathsManager_AccessMap;

static PathsManager PathsManager_Instance;

static void PathsManager_ProcessStationaryUnits(unsigned char **map, UnitInfo *unit);
static void PathsManager_ProcessMobileUnits(unsigned char **map, SmartList<UnitInfo> *units, UnitInfo *unit,
                                            unsigned char flags);
static void PathsManager_ProcessMapSurface(unsigned char **map, int surface_type, unsigned char value);
static void PathsManager_ProcessGroundCover(unsigned char **map, UnitInfo *unit, int surface_type);
static bool PathsManager_IsProcessed(int grid_x, int grid_y);
static void PathsManager_ProcessDangers(unsigned char **map, UnitInfo *unit);
static void PathsManager_ProcessSurface(unsigned char **map, UnitInfo *unit);

PathsManager::PathsManager() : forward_searcher(nullptr), backward_searcher(nullptr), time_stamp(0), elapsed_time(0) {}

PathsManager::~PathsManager() {
    delete forward_searcher;
    delete backward_searcher;

    if (PathsManager_AccessMap) {
        for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
            delete[] PathsManager_AccessMap[i];
        }

        delete[] PathsManager_AccessMap;
        PathsManager_AccessMap = nullptr;
    }
}

void PathsManager::PushBack(PathRequest &object) { requests.PushBack(object); }

void PathsManager::Clear() {
    delete forward_searcher;
    forward_searcher = nullptr;

    delete backward_searcher;
    backward_searcher = nullptr;

    request = nullptr;
    requests.Clear();
}

void PathsManager::PushFront(PathRequest &object) {
    if (request != nullptr) {
        requests.PushFront(*request);

        delete forward_searcher;
        forward_searcher = nullptr;

        delete backward_searcher;
        backward_searcher = nullptr;

        request = nullptr;
    }

    requests.PushFront(object);
}

int PathsManager::GetRequestCount(unsigned short team) const {
    int count;

    count = 0;

    for (SmartList<PathRequest>::Iterator it = requests.Begin(); it != requests.End(); ++it) {
        UnitInfo *unit = (*it).GetUnit1();

        if (unit && unit->team == team) {
            ++count;
        }
    }

    if (request != nullptr && request->GetUnit1() && request->GetUnit1()->team == team) {
        ++count;
    }

    return count;
}

void PathsManager::RemoveRequest(PathRequest *path_request) {
    /// the smart pointer is required to avoid premature destruction of the held object
    SmartPointer<PathRequest> protect_request(path_request);

    if (request == path_request) {
        delete forward_searcher;
        forward_searcher = nullptr;

        delete backward_searcher;
        backward_searcher = nullptr;

        request = nullptr;
    }

    requests.Remove(*path_request);

    path_request->Cancel();
}

void PathsManager::RemoveRequest(UnitInfo *unit) {
    for (SmartList<PathRequest>::Iterator it = requests.Begin(); it != requests.End(); ++it) {
        if ((*it).GetUnit1() == unit) {
            (*it).Cancel();
            requests.Remove(it);
        }
    }

    if (request != nullptr && request->GetUnit1() == unit) {
        RemoveRequest(&*request);
    }
}

void PathsManager::EvaluateTiles() {
    SmartPointer<UnitInfo> unit;
    SmartPointer<GroundPath> ground_path;
    SmartPointer<PathRequest> path_request;

    if (request != nullptr) {
        elapsed_time = timer_get_stamp32() - elapsed_time;

        if (backward_searcher == nullptr) {
            requests.PushFront(*request);
            request = nullptr;
        }
    }

    if ((timer_get_stamp32() - Paths_LastTimeStamp <= Paths_TimeLimit) ||
        (request == nullptr && requests.GetCount() == 0)) {
        while (request == nullptr) {
            if (requests.GetCount()) {
                ProcessRequest();

                if ((timer_get_stamp32() - Paths_LastTimeStamp > Paths_TimeLimit)) {
                    elapsed_time = timer_get_stamp32() - elapsed_time;
                    return;
                }

            } else {
                return;
            }
        }

        unit = request->GetUnit1();
        path_request = request;

        for (int index = 5;;) {
            MouseEvent::ProcessInput();
            --index;

            if (index >= 0) {
                if (path_request != request) {
                    return;
                }

                SDL_assert(backward_searcher != nullptr);

                backward_searcher->BackwardSearch(forward_searcher);

                if (!forward_searcher->ForwardSearch(backward_searcher)) {
                    if (Paths_DebugMode >= 1) {
                        char message[100];

                        sprintf(message, "Debug: path generator evaluated %i tiles in %i msecs, max depth = %i",
                                Paths_EvaluatedTileCount, timer_elapsed_time_ms(time_stamp), Paths_MaxDepth);

                        MessageManager_DrawMessage(message, 0, 0);
                    }

                    ground_path =
                        forward_searcher->DeterminePath(Point(unit->grid_x, unit->grid_y), path_request->GetMaxCost());

                    delete forward_searcher;
                    forward_searcher = nullptr;

                    delete backward_searcher;
                    backward_searcher = nullptr;

                    CompleteRequest(&*ground_path);

                    return;
                }

            } else {
                index = 5;

                if (timer_get_stamp32() - Paths_LastTimeStamp > Paths_TimeLimit) {
                    break;
                }
            }
        }

        elapsed_time = timer_get_stamp32() - elapsed_time;

    } else {
        elapsed_time = timer_get_stamp32() - elapsed_time;
    }
}

bool PathsManager::HasRequest(UnitInfo *unit) const {
    for (SmartList<PathRequest>::Iterator it = requests.Begin(); it != requests.End(); ++it) {
        if ((*it).GetUnit1() == unit) {
            return true;
        }
    }

    if (request != nullptr && request->GetUnit1() == unit) {
        return true;
    }

    return false;
}

int PathsManager_GetRequestCount(unsigned short team) { return PathsManager_Instance.GetRequestCount(team); }

void PathsManager_RemoveRequest(PathRequest *request) { PathsManager_Instance.RemoveRequest(request); }

void PathsManager_RemoveRequest(UnitInfo *unit) { PathsManager_Instance.RemoveRequest(unit); }

void PathsManager_PushBack(PathRequest &object) { PathsManager_Instance.PushBack(object); }

void PathsManager_PushFront(PathRequest &object) { PathsManager_Instance.PushFront(object); }

void PathsManager_EvaluateTiles() { PathsManager_Instance.EvaluateTiles(); }

void PathsManager_Clear() { PathsManager_Instance.Clear(); }

bool PathsManager_HasRequest(UnitInfo *unit) { return PathsManager_Instance.HasRequest(unit); }

bool PathsManager::Init(UnitInfo *unit) {
    bool result;

    if (PathsManager_AccessMap == nullptr) {
        PathsManager_AccessMap = new (std::nothrow) unsigned char *[ResourceManager_MapSize.x];

        for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
            PathsManager_AccessMap[i] = new (std::nothrow) unsigned char[ResourceManager_MapSize.y];
        }
    }

    PathsManager_InitAccessMap(unit, PathsManager_AccessMap, request->GetFlags(), request->GetCautionLevel());

    if (request->GetUnit2()) {
        AccessMap access_map;

        PathsManager_InitAccessMap(request->GetUnit2(), access_map.GetMap(), request->GetFlags(),
                                   CAUTION_LEVEL_AVOID_ALL_DAMAGE);

        for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
            for (int j = 0; j < ResourceManager_MapSize.y; ++j) {
                if (access_map.GetMapColumn(i)[j]) {
                    if (PathsManager_AccessMap[i][j] == 0x00) {
                        PathsManager_AccessMap[i][j] = (access_map.GetMapColumn(i)[j] * 3) | 0x80;
                    }

                } else {
                    PathsManager_AccessMap[i][j] |= 0x40;
                }
            }
        }
    }

    {
        Point point1(unit->grid_x, unit->grid_y);
        Point point2 = request->GetPoint();
        int minimum_distance;
        int minimum_distance_sqrt;
        int distance_squared;
        int distance_x;
        int distance_y;
        int limit;
        int limit2;

        PathsManager_AccessMap[point1.x][point1.y] = 2;

        if (request->GetBoardTransport()) {
            SmartPointer<UnitInfo> receiver_unit;

            receiver_unit = Access_GetReceiverUnit(unit, point2.x, point2.y);

            if (receiver_unit != nullptr) {
                int grid_x = receiver_unit->grid_x;
                int grid_y = receiver_unit->grid_y;

                PathsManager_AccessMap[grid_x][grid_y] = 2;

                if (receiver_unit->flags & BUILDING) {
                    PathsManager_AccessMap[grid_x + 1][grid_y] = 2;
                    PathsManager_AccessMap[grid_x][grid_y + 1] = 2;
                    PathsManager_AccessMap[grid_x + 1][grid_y + 1] = 2;
                }
            }
        }

        minimum_distance = request->GetMinimumDistance();
        minimum_distance_sqrt = sqrt(minimum_distance);

        result = false;

        if (minimum_distance_sqrt == 0) {
            unsigned char value = PathsManager_AccessMap[point2.x][point2.y];

            if (value && !(value & 0x80)) {
                result = true;
            }
        }

        for (int i = point2.x - minimum_distance_sqrt; i < point2.x; ++i) {
            int j;

            distance_squared = (i - point2.x) * (i - point2.x);

            for (j = point2.y - minimum_distance_sqrt; j <= point2.y; ++j) {
                if ((j - point2.y) * (j - point2.y) + distance_squared <= minimum_distance) {
                    break;
                }
            }

            if (!result && j <= point2.y) {
                distance_x = point2.x * 2 - i;
                distance_y = point2.y * 2 - j;

                if (PathsManager_IsProcessed(i, j)) {
                    result = true;
                }

                if (PathsManager_IsProcessed(i, distance_y)) {
                    result = true;
                }

                if (PathsManager_IsProcessed(distance_x, j)) {
                    result = true;
                }

                if (PathsManager_IsProcessed(distance_x, distance_y)) {
                    result = true;
                }
            }

            ++j;
            limit = point2.y * 2 - j;

            if (j < 0) {
                j = 0;
            }

            if (limit > ResourceManager_MapSize.y - 1) {
                limit = ResourceManager_MapSize.y - 1;
            }

            limit2 = i + 1;
            distance_x = point2.x * 2 - limit2;

            for (; j <= limit; ++j) {
                if (limit2 >= 0) {
                    PathsManager_AccessMap[limit2][j] = 2 | 0x40;
                }

                if (distance_x < ResourceManager_MapSize.x) {
                    PathsManager_AccessMap[distance_x][j] = 2 | 0x40;
                }
            }
        }
    }

    return result;
}

void PathsManager::CompleteRequest(GroundPath *path) {
    SmartPointer<PathRequest> path_request(request);

    delete forward_searcher;
    forward_searcher = nullptr;

    delete backward_searcher;
    backward_searcher = nullptr;

    request = nullptr;

    path_request->Finish(path);
}

void PathsManager::ProcessRequest() {
    if (requests.GetCount()) {
        SmartList<PathRequest>::Iterator it = requests.Begin();
        request = &*it;
        requests.Remove(it);

        SmartPointer<UnitInfo> unit(request->GetUnit1());

        Point destination(request->GetPoint());
        Point position(unit->grid_x, unit->grid_y);

        if (request->PathRequest_Vfunc1()) {
            request = nullptr;

        } else {
            time_stamp = timer_get_stamp32();
            elapsed_time = time_stamp;

            Paths_EvaluatedTileCount = 0;
            Paths_EvaluatorCallCount = 0;
            Paths_SquareAdditionsCount = 0;
            Paths_EvaluatedSquareCount = 0;
            Paths_SquareInsertionsCount = 0;
            Paths_MaxDepth = 0;

            if (position == destination) {
                CompleteRequest(nullptr);

            } else {
                if (Access_GetDistance(position, destination) > 2) {
                    if (Init(&*unit)) {
                        bool mode;

                        if (request->GetUnit2() && request->GetUnit2()->unit_type == AIRTRANS) {
                            mode = true;

                        } else {
                            mode = false;
                        }

                        SmartPointer<PathRequest> path_request(request);

                        PathFill path_fill(PathsManager_AccessMap);

                        path_fill.Fill(position);

                        if (PathsManager_AccessMap[destination.x][destination.y] & 0x20) {
                            forward_searcher = new (std::nothrow) Searcher(position, destination, mode);
                            backward_searcher = new (std::nothrow) Searcher(destination, position, mode);

                            forward_searcher->Process(position, true);
                            backward_searcher->Process(destination, false);

                        } else {
                            CompleteRequest(nullptr);
                        }

                    } else {
                        CompleteRequest(nullptr);
                    }

                } else {
                    bool is_path_viable = false;

                    if (request->GetBoardTransport() && Access_GetReceiverUnit(&*unit, destination.x, destination.y)) {
                        is_path_viable = true;

                    } else {
                        if (Access_IsAccessible(unit->unit_type, unit->team, destination.x, destination.y,
                                                request->GetFlags()) &&
                            !Ai_IsDangerousLocation(&*unit, destination, request->GetCautionLevel(), 0x01)) {
                            is_path_viable = true;
                        }
                    }

                    if (is_path_viable) {
                        SmartPointer<GroundPath> ground_path(new (std::nothrow)
                                                                 GroundPath(destination.x, destination.y));
                        ground_path->AddStep(destination.x - position.x, destination.y - position.y);

                        CompleteRequest(&*ground_path);

                    } else {
                        CompleteRequest(nullptr);
                    }
                }
            }
        }
    }
}

void PathsManager_ProcessStationaryUnits(unsigned char **map, UnitInfo *unit) {
    unsigned short team = unit->team;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).unit_type != CNCT_4W && ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team))) {
            map[(*it).grid_x][(*it).grid_y] = 0;

            if ((*it).flags & BUILDING) {
                map[(*it).grid_x + 1][(*it).grid_y] = 0;
                map[(*it).grid_x][(*it).grid_y + 1] = 0;
                map[(*it).grid_x + 1][(*it).grid_y + 1] = 0;
            }
        }
    }
}

void PathsManager_ProcessMobileUnits(unsigned char **map, SmartList<UnitInfo> *units, UnitInfo *unit,
                                     unsigned char flags) {
    unsigned short team = unit->team;

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if ((*it).orders != ORDER_IDLE && (*it).IsVisibleToTeam(team)) {
            if ((flags & 2) || ((flags & 1) && (*it).team != team)) {
                map[(*it).grid_x][(*it).grid_y] = 0;

                if ((*it).path != nullptr && (*it).state != ORDER_STATE_1 && (&*it) != unit) {
                    Point position = (*it).path->GetPosition(&*it);
                    map[position.x][position.y] = 0;
                }
            }
        }
    }
}

void PathsManager_ProcessMapSurface(unsigned char **map, int surface_type, unsigned char value) {
    for (int index_x = 0; index_x < ResourceManager_MapSize.x; ++index_x) {
        for (int index_y = 0; index_y < ResourceManager_MapSize.y; ++index_y) {
            if (ResourceManager_MapSurfaceMap[index_y * ResourceManager_MapSize.x + index_x] == surface_type) {
                map[index_x][index_y] = value;
            }
        }
    }
}

void PathsManager_ProcessGroundCover(unsigned char **map, UnitInfo *unit, int surface_type) {
    unsigned short team = unit->team;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
         it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team)) {
            switch ((*it).unit_type) {
                case BRIDGE: {
                    if (surface_type & SURFACE_TYPE_LAND) {
                        map[(*it).grid_x][(*it).grid_y] = 4;
                    }
                } break;

                case WTRPLTFM: {
                    if (surface_type & SURFACE_TYPE_LAND) {
                        map[(*it).grid_x][(*it).grid_y] = 4;

                    } else {
                        map[(*it).grid_x][(*it).grid_y] = 0;
                    }
                } break;

                case LANDMINE:
                case SEAMINE: {
                    if ((*it).team != team && (*it).IsDetectedByTeam(team)) {
                        map[(*it).grid_x][(*it).grid_y] = 0;
                    }
                } break;
            }
        }
    }

    if ((surface_type & SURFACE_TYPE_LAND) && unit->GetLayingState() != 2 && unit->GetLayingState() != 1) {
        for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
             it != UnitsManager_GroundCoverUnits.End(); ++it) {
            if ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team)) {
                if ((*it).unit_type == ROAD || (*it).unit_type == SMLSLAB || (*it).unit_type == LRGSLAB ||
                    (*it).unit_type == BRIDGE) {
                    map[(*it).grid_x][(*it).grid_y] = 2;

                    if ((*it).flags & BUILDING) {
                        map[(*it).grid_x + 1][(*it).grid_y] = 2;
                        map[(*it).grid_x][(*it).grid_y + 1] = 2;
                        map[(*it).grid_x + 1][(*it).grid_y + 1] = 2;
                    }
                }
            }
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();
         it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).IsVisibleToTeam(team) || (*it).IsDetectedByTeam(team)) {
            if ((*it).unit_type == LRGTAPE || (*it).unit_type == LRGTAPE) {
                map[(*it).grid_x][(*it).grid_y] = 0;

                if ((*it).flags & BUILDING) {
                    map[(*it).grid_x + 1][(*it).grid_y] = 0;
                    map[(*it).grid_x][(*it).grid_y + 1] = 0;
                    map[(*it).grid_x + 1][(*it).grid_y + 1] = 0;
                }
            }
        }
    }
}

void PathsManager_InitAccessMap(UnitInfo *unit, unsigned char **map, unsigned char flags, int caution_level) {
    if (unit->flags & MOBILE_AIR_UNIT) {
        for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
            memset(map[i], 4, ResourceManager_MapSize.y);
        }

        PathsManager_ProcessMobileUnits(map, &UnitsManager_MobileAirUnits, unit, flags);

    } else {
        int surface_types = UnitsManager_BaseUnits[unit->unit_type].land_type;

        for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
            memset(map[i], 0, ResourceManager_MapSize.y);
        }

        if (surface_types & SURFACE_TYPE_LAND) {
            PathsManager_ProcessMapSurface(map, SURFACE_TYPE_LAND, 4);
        }

        if (surface_types & SURFACE_TYPE_COAST) {
            PathsManager_ProcessMapSurface(map, SURFACE_TYPE_COAST, 4);
        }

        if (surface_types & SURFACE_TYPE_WATER) {
            if ((surface_types & SURFACE_TYPE_LAND) && unit->unit_type != SURVEYOR) {
                PathsManager_ProcessMapSurface(map, SURFACE_TYPE_WATER, 8);

            } else {
                PathsManager_ProcessMapSurface(map, SURFACE_TYPE_WATER, 4);
            }
        }

        PathsManager_ProcessGroundCover(map, unit, surface_types);
        PathsManager_ProcessMobileUnits(map, &UnitsManager_MobileLandSeaUnits, unit, flags);
        PathsManager_ProcessStationaryUnits(map, unit);
    }

    if (caution_level > 0) {
        PathsManager_ApplyCautionLevel(map, unit, caution_level);
    }
}

unsigned char **PathsManager_GetAccessMap() { return PathsManager::PathsManager_AccessMap; }

void PathsManager_ApplyCautionLevel(unsigned char **map, UnitInfo *unit, int caution_level) {
    if (caution_level > 0) {
        if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_PLAYER) {
            PathsManager_ProcessDangers(map, unit);
        }

        if (UnitsManager_TeamInfo[unit->team].team_type == TEAM_TYPE_COMPUTER) {
            int unit_hits = unit->hits;
            short **damage_potential_map;

            if (unit->GetId() == 0xFFFF) {
                damage_potential_map =
                    AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit->unit_type, caution_level, 0x01);

            } else {
                damage_potential_map = AiPlayer_Teams[unit->team].GetDamagePotentialMap(unit, caution_level, 0x01);
            }

            if (damage_potential_map) {
                if (caution_level == CAUTION_LEVEL_AVOID_ALL_DAMAGE) {
                    unit_hits = 1;
                }

                for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
                    for (int j = 0; j < ResourceManager_MapSize.y; ++j) {
                        if (damage_potential_map[i][j] >= unit_hits) {
                            map[i][j] = 0;
                        }
                    }
                }
            }
        }
    }
}

bool PathsManager_IsProcessed(int grid_x, int grid_y) {
    bool result;

    if (grid_x >= 0 && grid_x < ResourceManager_MapSize.x && grid_y >= 0 && grid_y < ResourceManager_MapSize.y) {
        unsigned char value = PathsManager_GetAccessMap()[grid_x][grid_y];

        if (value && !(value & 0x80)) {
            result = true;

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

void PathsManager_PrintPathDebugMode() {
    Paths_DebugMode = (Paths_DebugMode + 1) % 3;

    switch (Paths_DebugMode) {
        case 0: {
            MessageManager_DrawMessage("No path debugging.", 0, 0);
        } break;

        case 1: {
            MessageManager_DrawMessage("Show path generator statistics.", 0, 0);
        } break;

        case 2: {
            MessageManager_DrawMessage("Draw path searches.", 0, 0);
        } break;
    }
}

void PathsManager_ProcessDangers(unsigned char **map, UnitInfo *unit) {
    unsigned short team = unit->team;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team != team && (*it).IsVisibleToTeam(team) &&
            (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0 && (*it).orders != ORDER_DISABLE &&
            (*it).orders != ORDER_IDLE && (*it).hits > 0 &&
            Access_IsValidAttackTargetType((*it).unit_type, unit->unit_type)) {
            PathsManager_ProcessSurface(map, &*it);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team != team && (*it).IsVisibleToTeam(team) &&
            (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0 && (*it).orders != ORDER_DISABLE &&
            (*it).orders != ORDER_IDLE && (*it).hits > 0 &&
            Access_IsValidAttackTargetType((*it).unit_type, unit->unit_type)) {
            PathsManager_ProcessSurface(map, &*it);
        }
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();
         it != UnitsManager_MobileAirUnits.End(); ++it) {
        if ((*it).team != team && (*it).IsVisibleToTeam(team) &&
            (*it).GetBaseValues()->GetAttribute(ATTRIB_ATTACK) > 0 && (*it).orders != ORDER_DISABLE &&
            (*it).orders != ORDER_IDLE && (*it).hits > 0 &&
            Access_IsValidAttackTargetType((*it).unit_type, unit->unit_type)) {
            PathsManager_ProcessSurface(map, &*it);
        }
    }
}

void PathsManager_ProcessSurface(unsigned char **map, UnitInfo *unit) {
    int range = unit->GetBaseValues()->GetAttribute(ATTRIB_RANGE);
    Point position(unit->grid_x, unit->grid_y);

    if (unit->unit_type == SUBMARNE || unit->unit_type == CORVETTE) {
        ZoneWalker walker(position, range);

        do {
            if (ResourceManager_MapSurfaceMap[walker.GetGridY() * ResourceManager_MapSize.x + walker.GetGridX()] &
                (SURFACE_TYPE_WATER | SURFACE_TYPE_COAST)) {
                map[walker.GetGridX()][walker.GetGridY()] = 0;
            }
        } while (walker.FindNext());

    } else {
        Point point3;
        Point point4;
        int range_square;
        int distance_square;

        range_square = range * range;

        point3.x = std::max(position.x - range, 0);
        point4.x = std::min(position.x + range, ResourceManager_MapSize.x - 1);

        for (; point3.x <= point4.x; ++point3.x) {
            distance_square = (point3.x - position.x) * (point3.x - position.x);

            for (point3.y = range; point3.y >= 0 && (point3.y * point3.y + distance_square) > range_square;
                 --point3.y) {
            }

            point4.y = std::min(position.y + point3.y, ResourceManager_MapSize.y - 1);
            point3.y = std::max(position.y - point3.y, 0);

            if (point4.y >= point3.y) {
                memset(&map[point3.x][point3.y], 0, point4.y - point3.y + 1);
            }
        }
    }
}
