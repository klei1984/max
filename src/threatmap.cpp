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

#include "threatmap.hpp"

#include "resource_manager.hpp"
#include "units_manager.hpp"

ThreatMap::ThreatMap() : dimension(0, 0) {
    damage_potential_map = nullptr;
    shots_map = nullptr;
    id = 0;
    risk_level = 0;
}

ThreatMap::~ThreatMap() { Deinit(); }

void ThreatMap::Init() {
    if (damage_potential_map) {
        if (dimension.x != ResourceManager_MapSize.x || dimension.y != ResourceManager_MapSize.y) {
            Deinit();
        }
    }

    if (!damage_potential_map) {
        dimension = ResourceManager_MapSize;

        damage_potential_map = new (std::nothrow) int16_t*[dimension.x];
        shots_map = new (std::nothrow) int16_t*[dimension.x];

        for (int32_t i = 0; i < dimension.x; ++i) {
            damage_potential_map[i] = new (std::nothrow) int16_t[dimension.y];
            shots_map[i] = new (std::nothrow) int16_t[dimension.y];
        }
    }

    for (int32_t i = 0; i < dimension.x; ++i) {
        memset(damage_potential_map[i], 0, dimension.y * sizeof(int16_t));
        memset(shots_map[i], 0, dimension.y * sizeof(int16_t));
    }
}

void ThreatMap::Deinit() {
    risk_level = 0;

    if (damage_potential_map) {
        for (int32_t i = 0; i < dimension.x; ++i) {
            delete[] damage_potential_map[i];
        }

        delete[] damage_potential_map;
        damage_potential_map = nullptr;
    }

    if (shots_map) {
        for (int32_t i = 0; i < dimension.x; ++i) {
            delete[] shots_map[i];
        }

        delete[] shots_map;
        shots_map = nullptr;
    }
}

uint16_t ThreatMap::GetRiskLevel(ResourceID unit_type) {
    uint16_t result;

    switch (unit_type) {
        case CLNTRANS: {
            result = 7;
        } break;

        case SUBMARNE: {
            result = 6;
        } break;

        case COMMANDO: {
            result = 5;
        } break;

        case SURVEYOR: {
            result = 2;
        } break;

        default: {
            if (UnitsManager_BaseUnits[unit_type].flags & MOBILE_AIR_UNIT) {
                result = 3;

            } else {
                result = 1;
            }
        } break;
    }

    return result;
}

uint16_t ThreatMap::GetRiskLevel(UnitInfo* unit) {
    uint16_t result;
    int32_t team_index;

    result = GetRiskLevel(unit->unit_type);

    if (result >= 4) {
        for (team_index = PLAYER_TEAM_RED; team_index < PLAYER_TEAM_MAX - 1; ++team_index) {
            if (unit->team != team_index && unit->IsDetectedByTeam(team_index)) {
                break;
            }
        }

        if (team_index < PLAYER_TEAM_MAX - 1) {
            result = 1;

        } else if (result == 5 && unit->shots == 0) {
            result = 4;
        }
    }

    return result;
}

void ThreatMap::SetRiskLevel(uint8_t risk_level_) { risk_level = risk_level_; }

void ThreatMap::Update(int32_t armor_) {
    if (armor != armor_) {
        int32_t difference = armor - armor_;

        armor = armor_;

        for (int32_t grid_x = 0; grid_x < dimension.x; ++grid_x) {
            for (int32_t grid_y = 0; grid_y < dimension.y; ++grid_y) {
                damage_potential_map[grid_x][grid_y] += shots_map[grid_x][grid_y] * difference;
            }
        }
    }
}
