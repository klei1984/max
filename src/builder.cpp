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

#include "builder.hpp"

#include "access.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "inifile.hpp"
#include "resource_manager.hpp"
#include "units_manager.hpp"

unsigned short Builder_CapabilityListNormal[] = {

    /* builder */ CONSTRCT,
    /* unit list size */ 15,
    /* unit list */
    MININGST,
    POWERSTN,
    LIGHTPLT,
    LANDPLT,
    AIRPLT,
    SHIPYARD,
    COMMTWR,
    DEPOT,
    HANGAR,
    DOCK,
    HABITAT,
    RESEARCH,
    GREENHSE,
    TRAINHAL,
    BARRACKS,

    /* builder */ LIGHTPLT,
    /* unit list size */ 11,
    /* unit list */
    SCOUT,
    SURVEYOR,
    ENGINEER,
    REPAIR,
    SPLYTRCK,
    FUELTRCK,
    GOLDTRCK,
    SP_FLAK,
    MINELAYR,
    BULLDOZR,
    CLNTRANS,

    /* builder */ LANDPLT,
    /* unit list size */ 6,
    /* unit list */
    CONSTRCT,
    SCANNER,
    TANK,
    ARTILLRY,
    ROCKTLCH,
    MISSLLCH,

    /* builder */ SHIPYARD,
    /* unit list size */ 8,
    /* unit list */
    FASTBOAT,
    CORVETTE,
    BATTLSHP,
    SUBMARNE,
    SEATRANS,
    MSSLBOAT,
    SEAMNLYR,
    CARGOSHP,

    /* builder */ AIRPLT,
    /* unit list size */ 4,
    /* unit list */
    FIGHTER,
    BOMBER,
    AIRTRANS,
    AWAC,

    /* builder */ ENGINEER,
    /* unit list size */ 15,
    /* unit list */
    ADUMP,
    FDUMP,
    GOLDSM,
    POWGEN,
    CNCT_4W,
    RADAR,
    GUNTURRT,
    ANTIAIR,
    ARTYTRRT,
    ANTIMSSL,
    LANDPAD,
    BRIDGE,
    WTRPLTFM,
    BLOCK,
    ROAD,

    /* builder */ TRAINHAL,
    /* unit list size */ 2,
    /* unit list */
    COMMANDO,
    INFANTRY,
};

ResourceID Builder_CapabilityListTraining1[] = {
    ADUMP,
    FDUMP,
    CNCT_4W,
    INVALID_ID,
};

ResourceID Builder_CapabilityListTraining2[] = {
    ADUMP, FDUMP, CNCT_4W, POWGEN, LIGHTPLT, GUNTURRT, SCOUT, ENGINEER, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining3[] = {
    ADUMP, FDUMP, CNCT_4W, POWGEN, LIGHTPLT, GUNTURRT, RADAR, LANDPLT, CONSTRCT, ENGINEER, SCOUT, TANK, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining4[] = {
    ADUMP,    FDUMP,    CNCT_4W, POWGEN,   LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,
    CONSTRCT, ENGINEER, SCOUT,   SURVEYOR, TANK,     DEPOT,    MININGST, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining5[] = {
    ADUMP,    FDUMP,    CNCT_4W, POWGEN,   LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,
    CONSTRCT, ENGINEER, SCOUT,   SURVEYOR, TANK,     DEPOT,    MININGST, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining6[] = {
    ADUMP,    FDUMP,    CNCT_4W, POWGEN,   LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,
    CONSTRCT, ENGINEER, SCOUT,   SURVEYOR, TANK,     DEPOT,    MININGST, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining7[] = {
    ADUMP,    FDUMP,    CNCT_4W, POWGEN,   LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,
    CONSTRCT, ENGINEER, SCOUT,   SURVEYOR, TANK,     DEPOT,    MININGST, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining8[] = {
    ADUMP,    FDUMP, CNCT_4W,  POWGEN, LIGHTPLT, GUNTURRT, RADAR,  LANDPLT,    CONSTRCT,
    ENGINEER, SCOUT, SURVEYOR, TANK,   DEPOT,    MININGST, GOLDSM, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining9[] = {
    ADUMP,    FDUMP, CNCT_4W,  POWGEN, LIGHTPLT, GUNTURRT, RADAR,  LANDPLT, CONSTRCT,
    ENGINEER, SCOUT, SURVEYOR, TANK,   DEPOT,    MININGST, GOLDSM, COMMTWR, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining10[] = {
    ADUMP, FDUMP,    CNCT_4W, POWGEN, LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,  CONSTRCT, ENGINEER,
    SCOUT, SURVEYOR, TANK,    DEPOT,  MININGST, HABITAT,  GREENHSE, ARTILLRY, ROCKTLCH, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining11[] = {
    ADUMP, FDUMP, CNCT_4W,  POWGEN,  LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,  CONSTRCT, ENGINEER, SCOUT,      SURVEYOR,
    TANK,  DEPOT, MININGST, HABITAT, TRAINHAL, COMMANDO, INFANTRY, GREENHSE, ARTILLRY, ROCKTLCH, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining12[] = {
    ADUMP, FDUMP, CNCT_4W,  POWGEN,  LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,  CONSTRCT, ENGINEER, SCOUT,    SURVEYOR,
    TANK,  DEPOT, MININGST, HABITAT, TRAINHAL, COMMANDO, INFANTRY, GREENHSE, MINELAYR, ARTILLRY, ROCKTLCH, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining13[] = {
    ADUMP, FDUMP,    CNCT_4W, POWGEN, LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,  CONSTRCT,   ENGINEER,
    SCOUT, SURVEYOR, TANK,    DEPOT,  MININGST, REPAIR,   ARTILLRY, ROCKTLCH, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining14[] = {
    ADUMP, FDUMP,    CNCT_4W, POWGEN, LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,  CONSTRCT, ENGINEER,
    SCOUT, SURVEYOR, TANK,    DEPOT,  MININGST, REPAIR,   ARTILLRY, ROCKTLCH, SPLYTRCK, INVALID_ID,
};

ResourceID Builder_CapabilityListTraining15[] = {
    ADUMP, FDUMP, CNCT_4W,  POWGEN,  LIGHTPLT, GUNTURRT, RADAR,    LANDPLT,  CONSTRCT, ENGINEER, SCOUT,    SURVEYOR,
    TANK,  DEPOT, MININGST, HABITAT, ARTILLRY, ROCKTLCH, GREENHSE, RESEARCH, TRAINHAL, INFANTRY, COMMANDO, INVALID_ID,
};

ResourceID *Builder_CapabilityListTrainingList[] = {
    Builder_CapabilityListTraining1,  Builder_CapabilityListTraining2,  Builder_CapabilityListTraining3,
    Builder_CapabilityListTraining4,  Builder_CapabilityListTraining5,  Builder_CapabilityListTraining6,
    Builder_CapabilityListTraining7,  Builder_CapabilityListTraining8,  Builder_CapabilityListTraining9,
    Builder_CapabilityListTraining10, Builder_CapabilityListTraining11, Builder_CapabilityListTraining12,
    Builder_CapabilityListTraining13, Builder_CapabilityListTraining14, Builder_CapabilityListTraining15,
};

ResourceID Builder_GetBuilderType(ResourceID unit_type) {
    ResourceID result;
    ResourceID builder_unit;
    ResourceID list_item;
    unsigned short list_size;

    for (int i = 0; i < sizeof(Builder_CapabilityListNormal) / sizeof(unsigned short);) {
        builder_unit = static_cast<ResourceID>(Builder_CapabilityListNormal[i++]);
        list_size = Builder_CapabilityListNormal[i++];

        for (int j = 0; j < list_size; ++j) {
            if (Builder_CapabilityListNormal[i++] == unit_type) {
                return builder_unit;
            }
        }
    }

    return INVALID_ID;
}

bool Builder_IsBuildable(ResourceID unit_type) {
    bool result;

    if (ini_get_setting(INI_GAME_FILE_TYPE) == GAME_TYPE_TRAINING) {
        if (GameManager_GameFileNumber <= 15) {
            ResourceID *unit_list;

            unit_list = Builder_CapabilityListTrainingList[GameManager_GameFileNumber];

            for (int i = 0; unit_list[i] != INVALID_ID; ++i) {
                if (unit_list[i] == unit_type) {
                    result = true;
                    break;
                }
            }

            result = false;

        } else {
            result = true;
        }

    } else {
        result = true;
    }

    return result;
}

bool Builder_IssueBuildOrder(UnitInfo *unit, short *grid_x, short *grid_y, ResourceID unit_type) {
    BaseUnit *base_unit;
    bool result;
    unsigned short team;

    base_unit = &UnitsManager_BaseUnits[unit_type];
    team = unit->team;

    if (unit->flags & STATIONARY) {
        result = true;

    } else {
        Hash_MapHash.Remove(unit);

        if (base_unit->flags & BUILDING) {
            result = Builder_IsAccessible(team, unit_type, *grid_x, *grid_y) ||
                     Builder_IsAccessible(team, unit_type, *grid_x, *grid_y) ||
                     Builder_IsAccessible(team, unit_type, *grid_x, *grid_y) ||
                     Builder_IsAccessible(team, unit_type, *grid_x, *grid_y);

        } else {
            result = Access_IsAccessible(unit_type, team, *grid_x, *grid_y, 2);
        }

        Hash_MapHash.Add(unit);
    }

    return result;
}

bool Builder_IsAccessible(unsigned short team, ResourceID unit_type, int grid_x, int grid_y) {
    bool result;

    if (grid_x >= 0 && grid_y >= 0 && grid_x <= ResourceManager_MapSize.x - 2 &&
        grid_y <= ResourceManager_MapSize.x - 2) {
        result = Access_IsAccessible(unit_type, team, grid_x, grid_y, 2) &&
                 Access_IsAccessible(unit_type, team, grid_x + 1, grid_y, 2) &&
                 Access_IsAccessible(unit_type, team, grid_x, grid_y + 1, 2) &&
                 Access_IsAccessible(unit_type, team, grid_x + 1, grid_y + 1, 2);
    } else {
        result = 0;
    }

    return result;
}

SmartObjectArray<ResourceID> Builder_GetBuildableUnits(ResourceID unit_type) {
    SmartObjectArray<ResourceID> units;
    ResourceID builder_unit;
    ResourceID buildable_unit;
    unsigned short list_size;

    for (int i = 0; i < sizeof(Builder_CapabilityListNormal) / sizeof(unsigned short);) {
        builder_unit = static_cast<ResourceID>(Builder_CapabilityListNormal[i++]);
        list_size = Builder_CapabilityListNormal[i++];

        if (builder_unit != unit_type) {
            i += list_size;

        } else {
            for (int j = 0; j < list_size; ++j) {
                buildable_unit = static_cast<ResourceID>(Builder_CapabilityListNormal[i++]);

                units.PushBack(&buildable_unit);
            }

            break;
        }
    }

    return units;
}
