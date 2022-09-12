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

#include "ai.hpp"

#include "access.hpp"
#include "aiplayer.hpp"
#include "inifile.hpp"
#include "units_manager.hpp"

#define AI_SAFE_ENEMY_DISTANCE 400

bool Ai_IsValidStartingPosition(Rect* bounds);
bool Ai_IsSafeStartingPosition(int grid_x, int grid_y, unsigned short team);

bool Ai_IsValidStartingPosition(Rect* bounds) {
    for (int x = bounds->ulx; x <= bounds->lrx; ++x) {
        for (int y = bounds->uly; y <= bounds->lry; ++y) {
            if (Access_GetModifiedSurfaceType(x, y) != SURFACE_TYPE_LAND) {
                return false;
            }
        }
    }

    return true;
}

bool Ai_IsSafeStartingPosition(int grid_x, int grid_y, unsigned short team) {
    Rect bounds;
    Point point;

    rect_init(&bounds, grid_x - 2, grid_y - 2, grid_x + 2, grid_y + 2);

    if (Ai_IsValidStartingPosition(&bounds)) {
        for (int i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
            if (team != i && UnitsManager_TeamMissionSupplies[i].units.GetCount()) {
                point.x = grid_x - UnitsManager_TeamMissionSupplies[i].starting_position.x;
                point.y = grid_y - UnitsManager_TeamMissionSupplies[i].starting_position.y;

                if (point.x * point.x + point.y * point.y < AI_SAFE_ENEMY_DISTANCE) {
                    return false;
                }
            }
        }

        return true;

    } else {
        return false;
    }
}

void Ai_SelectStartingPosition(unsigned short team) {
    int grid_x;
    int grid_y;

    do {
        grid_x = (((ResourceManager_MapSize.x - 6) * dos_rand()) >> 15) + 3;
        grid_y = (((ResourceManager_MapSize.y - 6) * dos_rand()) >> 15) + 3;
    } while (!Ai_IsSafeStartingPosition(grid_x, grid_y, team));

    UnitsManager_TeamMissionSupplies[team].starting_position.x = grid_x;
    UnitsManager_TeamMissionSupplies[team].starting_position.y = grid_y;
}

bool Ai_SetupStrategy(unsigned short team) {
    ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), "Computer");

    return AiPlayer_Teams[team].SelectStrategy();
}

void Ai_SetInfoMapPoint(Point point, unsigned short team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].SetInfoMapPoint(point);
    }
}

void Ai_MarkMineMapPoint(Point point, unsigned short team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].MarkMineMapPoint(point);
    }
}

void Ai_UpdateMineMap(Point point, unsigned short team) {
    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        AiPlayer_Teams[team].UpdateMineMap(point);
    }
}

void Ai_SetTasksPendingFlag(const char* event) {
    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            AiPlayer_Teams[team].ChangeTasksPendingFlag(true);
        }
    }
}

void Ai_Clear() {
    /// \todo
}

void Ai_FileLoad(SmartFileReader& file) {
    /// \todo
}

void Ai_FileSave(SmartFileWriter& file) {
    /// \todo
}

void Ai_AddUnitToTrackerList(UnitInfo* unit) {
    /// \todo
}

void Ai_EnableAutoSurvey(UnitInfo* unit) {
    /// \todo
}

bool Ai_IsDangerousLocation(UnitInfo* unit, Point destination, int caution_level, unsigned char flags) {
    /// \todo
}

void Ai_UpdateTerrain(UnitInfo* unit) {}

int Ai_DetermineCautionLevel(UnitInfo* unit) {}
