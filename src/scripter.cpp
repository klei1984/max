/* Copyright (c) 2025 M.A.X. Port Team
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

#include "scripter.hpp"

#include <SDL.h>

#include <unordered_map>

#include "lua.hpp"
#include "resource_manager.hpp"
#include "units_manager.hpp"

namespace Scripter {

struct Context {
    lua_State* m_lua;
    uint8_t m_type;
};

using MaxEnumType = std::vector<std::pair<const char*, int>>;
using MaxRegistryKeyType = std::string;
using MaxRegistryValueType = std::variant<std::monostate, bool, lua_Integer, lua_Number, std::string>;
using MaxRegistryType = std::unordered_map<MaxRegistryKeyType, MaxRegistryValueType>;

using MaxRegistryFunctionMapType = std::unordered_map<MaxRegistryKeyType, MaxRegistryFunctionType>;

constexpr size_t MinimumTimeBudget = 1000;

static MaxRegistryType MaxRegistry;
static SDL_mutex* MaxRegistryMutex;

static MaxRegistryFunctionMapType MaxRegistryFunctions;
static SDL_mutex* MaxRegistryFunctionsMutex;

static inline SmartList<UnitInfo>& GetRelevantUnits(ResourceID unit_type);
static lua_Integer HasMaterials(const lua_Integer team, const lua_Integer cargo_type);
static int LuaHasMaterials(lua_State* lua);
static void RegisterHasMaterials(lua_State* lua);
static lua_Integer CountReadyUnits(const lua_Integer team, const lua_Integer unit_type);
static int LuaCountReadyUnits(lua_State* lua);
static void RegisterCountReadyUnits(lua_State* lua);
static int HasAttackPower(const lua_Integer team, const lua_Integer unit_class);
static int LuaHasAttackPower(lua_State* lua);
static void RegisterHasAttackPower(lua_State* lua);
static int CanRebuildComplex(const lua_Integer team);
static int LuaCanRebuildComplex(lua_State* lua);
static void RegisterCanRebuildComplex(lua_State* lua);
static int CanRebuildBuilders(const lua_Integer team);
static int LuaCanRebuildBuilders(lua_State* lua);
static void RegisterCanRebuildBuilders(lua_State* lua);
static void CountTotalMining(const lua_Integer team, lua_Integer* const raw_mining_max,
                             lua_Integer* const fuel_mining_max, lua_Integer* const gold_mining_max);
static int LuaCountTotalMining(lua_State* lua);
static void RegisterCountTotalMining(lua_State* lua);
static lua_Integer GetTotalUnitsBeingConstructed(const lua_Integer team, const lua_Integer unit_type);
static int LuaGetTotalUnitsBeingConstructed(lua_State* lua);
static void RegisterGetTotalUnitsBeingConstructed(lua_State* lua);
static lua_Integer GetTotalPowerConsumption(const lua_Integer team, const lua_Integer unit_type);
static int LuaGetTotalPowerConsumption(lua_State* lua);
static void RegisterGetTotalPowerConsumption(lua_State* lua);
static lua_Integer GetTotalMining(const lua_Integer team, const lua_Integer cargo_type);
static int LuaGetTotalMining(lua_State* lua);
static void RegisterGetTotalMining(lua_State* lua);
static lua_Integer GetTotalConsumption(const lua_Integer team, const lua_Integer cargo_type);
static int LuaGetTotalConsumption(lua_State* lua);
static void RegisterGetTotalConsumption(lua_State* lua);
static int HasUnitAboveMarkLevel(const lua_Integer team, const lua_Integer unit_type,
                                 const lua_Integer unit_mark_level);
static int LuaHasUnitAboveMarkLevel(lua_State* lua);
static void RegisterHasUnitAboveMarkLevel(lua_State* lua);
static lua_Integer HasUnitExperience(const lua_Integer team, const lua_Integer unit_type);
static int LuaHasUnitExperience(lua_State* lua);
static void RegisterHasUnitExperience(lua_State* lua);
static lua_Integer CountCasualties(const lua_Integer team, const lua_Integer unit_type);
static int LuaCountCasualties(lua_State* lua);
static void RegisterCountCasualties(lua_State* lua);
static lua_Integer CountDamangedUnits(const lua_Integer team, const lua_Integer unit_type);
static int LuaCountDamangedUnits(lua_State* lua);
static void RegisterCountDamangedUnits(lua_State* lua);
static lua_Integer CountUnitsAboveAttribLevel(const lua_Integer team, const lua_Integer unit_type,
                                              const lua_Integer attribute, const lua_Integer level);
static int LuaCountUnitsAboveAttribLevel(lua_State* lua);
static void RegisterCountUnitsAboveAttribLevel(lua_State* lua);
static lua_Integer CountUnitsWithReducedAttrib(const lua_Integer team, const lua_Integer unit_type,
                                               const lua_Integer attribute);
static int LuaCountUnitsWithReducedAttrib(lua_State* lua);
static void RegisterCountUnitsWithReducedAttrib(lua_State* lua);
static lua_Integer GetResearchLevel(const lua_Integer team, const lua_Integer research_topic);
static int LuaGetResearchLevel(lua_State* lua);
static void RegisterGetResearchLevel(lua_State* lua);
static void TimoutHook(lua_State* lua, lua_Debug* ar);
static int LuaPrintRedirect(lua_State* lua);
static inline void AddEnum(lua_State* lua, const char* global_name, const MaxEnumType& fields);
static inline void ScriptLuaValuesToResults(lua_State* lua, ScriptParameters& results);
static inline MaxRegistryValueType MaxRegistryFromLuaValue(lua_State* lua, int index);
static inline void ScriptArgsToLuaValues(lua_State* lua, const ScriptParameters& args);
static inline void MaxRegistryToLuaValue(lua_State* lua, const MaxRegistryValueType& value);
static std::string LoadDefaultBuildRules();
static int MaxRegistryTableIndex(lua_State* lua);
static int MaxRegistryTableAddRemove(lua_State* lua);
static inline void ScriptArgsToLuaValues(lua_State* lua, const ScriptParameters& args);

static inline SmartList<UnitInfo>& GetRelevantUnits(ResourceID unit_type) {
    uint32_t flags;

    flags = UnitsManager_BaseUnits[unit_type].flags;

    if (flags & STATIONARY) {
        if (flags & GROUND_COVER) {
            return UnitsManager_GroundCoverUnits;

        } else {
            return UnitsManager_StationaryUnits;
        }

    } else if (flags & MOBILE_AIR_UNIT) {
        return UnitsManager_MobileAirUnits;

    } else {
        return UnitsManager_MobileLandSeaUnits;
    }
}

static lua_Integer HasMaterials(const lua_Integer team, const lua_Integer cargo_type) {
    lua_Integer cargo_sum{0};

    for (auto it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type == cargo_type) {
            cargo_sum += (*it).storage;
        }
    }

    for (auto it = UnitsManager_MobileLandSeaUnits.Begin(); it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && UnitsManager_BaseUnits[(*it).GetUnitType()].cargo_type == cargo_type) {
            cargo_sum += (*it).storage;
        }
    }

    return cargo_sum;
}

static int LuaHasMaterials(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_has_materials] Error: 2 arguments required: MAX_TEAM, MAX_CARGO_TYPE");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto cargo_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_has_materials] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (cargo_type < CARGO_TYPE_RAW or cargo_type > CARGO_TYPE_GOLD) {
        return luaL_error(lua, "[max_has_materials] Error: Invalid MAX_CARGO_TYPE (%d)", cargo_type);
    }

    auto result = HasMaterials(team, cargo_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterHasMaterials(lua_State* lua) { lua_register(lua, "max_has_materials", LuaHasMaterials); }

static lua_Integer CountReadyUnits(const lua_Integer team, const lua_Integer unit_type) {
    lua_Integer ready_units{0};

    const auto& units = GetRelevantUnits(static_cast<ResourceID>(unit_type));

    for (auto it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type &&
            (*it).GetOrderState() != ORDER_STATE_BUILDING_READY) {
            ++ready_units;
        }
    }

    return ready_units;
}

static int LuaCountReadyUnits(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_count_ready_units] Error: 2 arguments required: MAX_TEAM, MAX_UNIT");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_count_ready_units] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_count_ready_units] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    auto result = CountReadyUnits(team, unit_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterCountReadyUnits(lua_State* lua) { lua_register(lua, "max_count_ready_units", LuaCountReadyUnits); }

static int HasAttackPower(const lua_Integer team, const lua_Integer unit_class) {
    SmartList<UnitInfo>* units{nullptr};

    if (unit_class == (MOBILE_LAND_UNIT | MOBILE_SEA_UNIT)) {
        units = &UnitsManager_MobileLandSeaUnits;

    } else if (unit_class == MOBILE_AIR_UNIT) {
        units = &UnitsManager_MobileAirUnits;

    } else if (unit_class == STATIONARY) {
        units = &UnitsManager_StationaryUnits;

    } else {
        return false;
    }

    for (auto it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).ammo > 0 && (*it).GetUnitType() != SUBMARNE &&
            (*it).GetUnitType() != COMMANDO) {
            return true;
        }
    }

    return false;
}

static int LuaHasAttackPower(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_has_attack_power] Error: 2 arguments required: MAX_TEAM, MAX_UNIT_CLASS");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_class = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_has_attack_power] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_class != (MOBILE_LAND_UNIT | MOBILE_SEA_UNIT) and unit_class != MOBILE_AIR_UNIT and
        unit_class != STATIONARY) {
        return luaL_error(lua, "[max_has_attack_power] Error: Invalid MAX_UNIT_CLASS (%d)", unit_class);
    }

    auto result = HasAttackPower(team, unit_class);

    lua_pushboolean(lua, result);

    return 1;
}

static void RegisterHasAttackPower(lua_State* lua) { lua_register(lua, "max_has_attack_power", LuaHasAttackPower); }

static int CanRebuildComplex(const lua_Integer team) {
    SmartList<UnitInfo>::Iterator it;
    bool result;

    for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team &&
            ((*it).GetUnitType() == SHIPYARD || (*it).GetUnitType() == LIGHTPLT || (*it).GetUnitType() == LANDPLT ||
             (*it).GetUnitType() == AIRPLT || (*it).GetUnitType() == TRAINHAL)) {
            break;
        }
    }

    if (it != UnitsManager_StationaryUnits.End()) {
        int32_t sum_cargo_fuel;
        int32_t sum_cargo_materials;
        int32_t power_demand;
        int32_t sum_cargo_generated_power;

        sum_cargo_fuel = HasMaterials(team, CARGO_TYPE_FUEL);
        sum_cargo_materials = HasMaterials(team, CARGO_TYPE_RAW);
        power_demand = 1;
        sum_cargo_generated_power = 0;

        if (sum_cargo_fuel < 8 || sum_cargo_materials < 24) {
            for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == team && (*it).GetUnitType() == MININGST) {
                    break;
                }
            }

            if (it != UnitsManager_StationaryUnits.End()) {
                power_demand = 2;

            } else {
                result = false;
                return result;
            }
        }

        for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
            if ((*it).team == team && Cargo_GetPowerConsumptionRate((*it).GetUnitType()) < 0) {
                sum_cargo_generated_power -= Cargo_GetPowerConsumptionRate((*it).GetUnitType());
            }
        }

        result = sum_cargo_generated_power >= power_demand;

    } else {
        result = false;
    }

    return result;
}

static int LuaCanRebuildComplex(lua_State* lua) {
    if (lua_gettop(lua) < 1) {
        return luaL_error(lua, "[max_can_rebuild_complex] Error: 1 argument required: MAX_TEAM");
    }

    auto team = luaL_checkinteger(lua, 1);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_can_rebuild_complex] Error: Invalid MAX_TEAM (%d)", team);
    }

    auto result = CanRebuildComplex(team);

    lua_pushboolean(lua, result);

    return 1;
}

static void RegisterCanRebuildComplex(lua_State* lua) {
    lua_register(lua, "max_can_rebuild_complex", LuaCanRebuildComplex);
}

static int CanRebuildBuilders(const lua_Integer team) {
    SmartList<UnitInfo>::Iterator it;
    bool result;

    for (it = UnitsManager_MobileLandSeaUnits.Begin(); it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == CONSTRCT) {
            break;
        }
    }

    if (it != UnitsManager_MobileLandSeaUnits.End()) {
        for (it = UnitsManager_MobileLandSeaUnits.Begin(); it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
            if ((*it).team == team && (*it).GetUnitType() == ENGINEER) {
                break;
            }
        }

        if (it != UnitsManager_MobileLandSeaUnits.End()) {
            int32_t resources_needed;

            resources_needed = 38;

            for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == team && (*it).GetUnitType() == MININGST) {
                    break;
                }
            }

            if (it != UnitsManager_StationaryUnits.End()) {
                resources_needed = 8;
            }

            for (it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).team == team && Cargo_GetPowerConsumptionRate((*it).GetUnitType()) < 0) {
                    break;
                }
            }

            if (it != UnitsManager_StationaryUnits.End()) {
                resources_needed -= 8;
            }

            if (resources_needed == 0 || resources_needed <= HasMaterials(team, CARGO_TYPE_RAW)) {
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

    return result;
}

static int LuaCanRebuildBuilders(lua_State* lua) {
    if (lua_gettop(lua) < 1) {
        return luaL_error(lua, "[max_can_rebuild_builders] Error: 1 argument required: MAX_TEAM");
    }

    auto team = luaL_checkinteger(lua, 1);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_can_rebuild_builders] Error: Invalid MAX_TEAM (%d)", team);
    }

    auto result = CanRebuildBuilders(team);

    lua_pushboolean(lua, result);

    return 1;
}

static void RegisterCanRebuildBuilders(lua_State* lua) {
    lua_register(lua, "max_can_rebuild_builders", LuaCanRebuildBuilders);
}

static void CountTotalMining(const lua_Integer team, lua_Integer* const raw_mining_max,
                             lua_Integer* const fuel_mining_max, lua_Integer* const gold_mining_max) {
    raw_mining_max[0] = 0;
    fuel_mining_max[0] = 0;
    gold_mining_max[0] = 0;

    for (auto it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == MININGST && (*it).GetOrder() == ORDER_POWER_ON) {
            raw_mining_max[0] += (*it).raw_mining_max;
            fuel_mining_max[0] += (*it).fuel_mining_max;
            gold_mining_max[0] += (*it).gold_mining_max;
        }
    }
}

static int LuaCountTotalMining(lua_State* lua) {
    if (lua_gettop(lua) < 1) {
        return luaL_error(lua, "[max_count_total_mining] Error: 1 argument required: MAX_TEAM");
    }

    auto team = luaL_checkinteger(lua, 1);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_count_total_mining] Error: Invalid MAX_TEAM (%d)", team);
    }

    lua_Integer raw_mining_max = 0;
    lua_Integer fuel_mining_max = 0;
    lua_Integer gold_mining_max = 0;

    CountTotalMining(team, &raw_mining_max, &fuel_mining_max, &gold_mining_max);

    lua_pushinteger(lua, raw_mining_max);
    lua_pushinteger(lua, fuel_mining_max);
    lua_pushinteger(lua, gold_mining_max);

    return 3;
}

static void RegisterCountTotalMining(lua_State* lua) {
    lua_register(lua, "max_count_total_mining", LuaCountTotalMining);
}

static lua_Integer GetTotalUnitsBeingConstructed(const lua_Integer team, const lua_Integer unit_type) {
    SmartList<UnitInfo>* units;
    lua_Integer result = 0;

    if (UnitsManager_BaseUnits[unit_type].flags & STATIONARY) {
        units = &UnitsManager_MobileLandSeaUnits;

    } else {
        units = &UnitsManager_StationaryUnits;
    }

    for (auto it = units->Begin(); it != units->End(); ++it) {
        if ((*it).team == team && (*it).GetOrder() == ORDER_BUILD && (*it).build_time != 0 &&
            (*it).GetConstructedUnitType() == unit_type) {
            ++result;
        }
    }

    return result;
}

static int LuaGetTotalUnitsBeingConstructed(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua,
                          "[max_get_total_units_being_constructed] Error: 2 arguments required: MAX_TEAM, MAX_UNIT");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_get_total_units_being_constructed] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_get_total_units_being_constructed] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    auto result = GetTotalUnitsBeingConstructed(team, unit_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterGetTotalUnitsBeingConstructed(lua_State* lua) {
    lua_register(lua, "max_get_total_units_being_constructed", LuaGetTotalUnitsBeingConstructed);
}

static lua_Integer GetTotalPowerConsumption(const lua_Integer team, const lua_Integer unit_type) {
    lua_Integer result = 0;

    for (auto it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type &&
            ((*it).GetOrder() == ORDER_POWER_ON || (*it).GetOrder() == ORDER_BUILD)) {
            ++result;
        }
    }

    return result;
}

static int LuaGetTotalPowerConsumption(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_get_total_power_consumption] Error: 2 arguments required: MAX_TEAM, MAX_UNIT");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_get_total_power_consumption] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_get_total_power_consumption] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    auto result = GetTotalPowerConsumption(team, unit_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterGetTotalPowerConsumption(lua_State* lua) {
    lua_register(lua, "max_get_total_power_consumption", LuaGetTotalPowerConsumption);
}

static lua_Integer GetTotalMining(const lua_Integer team, const lua_Integer cargo_type) {
    lua_Integer result = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == MININGST && (*it).GetOrder() == ORDER_POWER_ON) {
            switch (cargo_type) {
                case CARGO_MATERIALS: {
                    result += (*it).raw_mining;
                } break;

                case CARGO_FUEL: {
                    result += (*it).fuel_mining;
                } break;

                case CARGO_GOLD: {
                    result += (*it).gold_mining;
                } break;
            }
        }
    }

    return result;
}

static int LuaGetTotalMining(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_get_total_mining] Error: 2 arguments required: MAX_TEAM, MAX_CARGO_TYPE");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto cargo_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_get_total_mining] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (cargo_type < CARGO_TYPE_RAW or cargo_type >= CARGO_TYPE_GOLD) {
        return luaL_error(lua, "[max_get_total_mining] Error: Invalid MAX_CARGO_TYPE (%d)", cargo_type);
    }

    auto result = GetTotalMining(team, cargo_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterGetTotalMining(lua_State* lua) { lua_register(lua, "max_get_total_mining", LuaGetTotalMining); }

static lua_Integer GetTotalConsumption(const lua_Integer team, const lua_Integer cargo_type) {
    Cargo cargo;
    lua_Integer result = 0;

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team) {
            cargo = Cargo_GetNetProduction(it->Get());

            switch (cargo_type) {
                case CARGO_MATERIALS: {
                    if (cargo.raw < 0) {
                        result -= cargo.raw;
                    }
                } break;

                case CARGO_FUEL: {
                    if (cargo.fuel < 0) {
                        result -= cargo.fuel;
                    }
                } break;

                case CARGO_GOLD: {
                    if (cargo.gold < 0) {
                        result -= cargo.gold;
                    }
                } break;
            }
        }
    }

    return result;
}

static int LuaGetTotalConsumption(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_get_total_consumption] Error: 2 arguments required: MAX_TEAM, MAX_CARGO_TYPE");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto cargo_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_get_total_consumption] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (cargo_type < CARGO_TYPE_RAW or cargo_type >= CARGO_TYPE_GOLD) {
        return luaL_error(lua, "[max_get_total_consumption] Error: Invalid MAX_CARGO_TYPE (%d)", cargo_type);
    }

    auto result = GetTotalConsumption(team, cargo_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterGetTotalConsumption(lua_State* lua) {
    lua_register(lua, "max_get_total_consumption", LuaGetTotalConsumption);
}

static int HasUnitAboveMarkLevel(const lua_Integer team, const lua_Integer unit_type,
                                 const lua_Integer unit_mark_level) {
    for (SmartList<UnitInfo>::Iterator it = UnitsManager_MobileLandSeaUnits.Begin();
         it != UnitsManager_MobileLandSeaUnits.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type &&
            (*it).GetBaseValues()->GetVersion() > unit_mark_level) {
            return true;
        }
    }

    return false;
}

static int LuaHasUnitAboveMarkLevel(lua_State* lua) {
    if (lua_gettop(lua) < 3) {
        return luaL_error(
            lua, "[max_has_unit_above_mark_level] Error: 3 arguments required: MAX_TEAM, MAX_UNIT, MARK_LEVEL");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);
    auto mark_level = luaL_checkinteger(lua, 3);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_has_unit_above_mark_level] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_has_unit_above_mark_level] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    auto result = HasUnitAboveMarkLevel(team, unit_type, mark_level);

    lua_pushboolean(lua, result);

    return 1;
}

static void RegisterHasUnitAboveMarkLevel(lua_State* lua) {
    lua_register(lua, "max_has_unit_above_mark_level", LuaHasUnitAboveMarkLevel);
}

static lua_Integer HasUnitExperience(const lua_Integer team, const lua_Integer unit_type) {
    const auto& units = GetRelevantUnits(static_cast<ResourceID>(unit_type));

    for (auto it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type && (*it).storage > 0) {
            return true;
        }
    }

    return false;
}

static int LuaHasUnitExperience(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_has_unit_experience] Error: 2 arguments required: MAX_TEAM, MAX_UNIT");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_has_unit_experience] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_has_unit_experience] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    auto result = HasUnitExperience(team, unit_type);

    lua_pushboolean(lua, result > 0);

    return 1;
}

static void RegisterHasUnitExperience(lua_State* lua) {
    lua_register(lua, "max_has_unit_experience", LuaHasUnitExperience);
}

static lua_Integer CountCasualties(const lua_Integer team, const lua_Integer unit_type) {
    return UnitsManager_TeamInfo[team].casualties[unit_type];
}

static int LuaCountCasualties(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_count_casualties] Error: 2 arguments required: MAX_TEAM, MAX_UNIT");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_count_casualties] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_count_casualties] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    auto result = CountCasualties(team, unit_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterCountCasualties(lua_State* lua) { lua_register(lua, "max_count_casualties", LuaCountCasualties); }

static lua_Integer CountDamangedUnits(const lua_Integer team, const lua_Integer unit_type) {
    const auto& units = GetRelevantUnits(static_cast<ResourceID>(unit_type));
    lua_Integer result{0};

    for (auto it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type &&
            (*it).hits != (*it).GetBaseValues()->GetAttribute(ATTRIB_HITS)) {
            ++result;
        }
    }

    return result;
}

static int LuaCountDamangedUnits(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_count_damaged_units] Error: 2 arguments required: MAX_TEAM, MAX_UNIT");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_count_damaged_units] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_count_damaged_units] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    auto result = CountDamangedUnits(team, unit_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterCountDamangedUnits(lua_State* lua) {
    lua_register(lua, "max_count_damaged_units", LuaCountDamangedUnits);
}

static lua_Integer CountUnitsAboveAttribLevel(const lua_Integer team, const lua_Integer unit_type,
                                              const lua_Integer attribute, const lua_Integer level) {
    lua_Integer result{0};
    const auto& units = GetRelevantUnits(static_cast<ResourceID>(unit_type));

    for (auto it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type &&
            (*it).GetBaseValues()->GetAttribute(attribute) > level) {
            ++result;
        }
    }

    return result;
}

static int LuaCountUnitsAboveAttribLevel(lua_State* lua) {
    if (lua_gettop(lua) < 4) {
        return luaL_error(lua,
                          "[max_count_units_above_attrib_level] Error: 4 arguments required: MAX_TEAM, MAX_UNIT, "
                          "MAX_UNIT_ATTRIB, ATTRIB_LEVEL");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);
    auto unit_attrib = luaL_checkinteger(lua, 3);
    auto attrib_level = luaL_checkinteger(lua, 4);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_count_units_above_attrib_level] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_count_units_above_attrib_level] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    if (unit_attrib >= ATTRIB_COUNT) {
        return luaL_error(lua, "[max_count_units_above_attrib_level] Error: Invalid MAX_UNIT_ATTRIB (%d)", unit_attrib);
    }

    auto result = CountUnitsAboveAttribLevel(team, unit_type, unit_attrib, attrib_level);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterCountUnitsAboveAttribLevel(lua_State* lua) {
    lua_register(lua, "max_count_units_above_attrib_level", LuaCountUnitsAboveAttribLevel);
}

static lua_Integer CountUnitsWithReducedAttrib(const lua_Integer team, const lua_Integer unit_type,
                                               const lua_Integer attribute) {
    lua_Integer result{0};
    const auto& units = GetRelevantUnits(static_cast<ResourceID>(unit_type));

    for (auto it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team && (*it).GetUnitType() == unit_type) {
            auto is_qualified{false};

            switch (attribute) {
                case ATTRIB_AMMO: {
                    is_qualified = (*it).ammo != (*it).GetBaseValues()->GetAttribute(ATTRIB_AMMO);
                } break;

                case ATTRIB_SPEED: {
                    is_qualified = (*it).speed != (*it).GetBaseValues()->GetAttribute(ATTRIB_SPEED);
                } break;

                case ATTRIB_ROUNDS: {
                    is_qualified = (*it).shots != (*it).GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);
                } break;

                case ATTRIB_HITS: {
                    is_qualified = (*it).hits != (*it).GetBaseValues()->GetAttribute(ATTRIB_HITS);
                } break;

                case ATTRIB_STORAGE: {
                    is_qualified = (*it).storage != (*it).GetBaseValues()->GetAttribute(ATTRIB_STORAGE);
                } break;
            }

            if (is_qualified) {
                ++result;
            }
        }
    }

    return result;
}

static int LuaCountUnitsWithReducedAttrib(lua_State* lua) {
    if (lua_gettop(lua) < 3) {
        return luaL_error(
            lua,
            "[max_count_units_with_reduced_attrib] Error: 3 arguments required: MAX_TEAM, MAX_UNIT, MAX_UNIT_ATTRIB");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto unit_type = luaL_checkinteger(lua, 2);
    auto unit_attrib = luaL_checkinteger(lua, 3);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_count_units_with_reduced_attrib] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (unit_type > UNIT_END or unit_type < UNIT_START) {
        return luaL_error(lua, "[max_count_units_with_reduced_attrib] Error: Invalid MAX_UNIT (%d)", unit_type);
    }

    if (unit_attrib != ATTRIB_AMMO && unit_attrib != ATTRIB_SPEED && unit_attrib != ATTRIB_ROUNDS &&
        unit_attrib != ATTRIB_HITS && unit_attrib != ATTRIB_STORAGE) {
        return luaL_error(lua, "[max_count_units_with_reduced_attrib] Error: Invalid MAX_UNIT_ATTRIB (%d)",
                          unit_attrib);
    }

    auto result = CountUnitsWithReducedAttrib(team, unit_type, unit_attrib);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterCountUnitsWithReducedAttrib(lua_State* lua) {
    lua_register(lua, "max_count_units_with_reduced_attrib", LuaCountUnitsWithReducedAttrib);
}

static lua_Integer GetResearchLevel(const lua_Integer team, const lua_Integer research_topic) {
    return UnitsManager_TeamInfo[team].research_topics[research_topic].research_level;
}

static int LuaGetResearchLevel(lua_State* lua) {
    if (lua_gettop(lua) < 2) {
        return luaL_error(lua, "[max_get_research_level] Error: 2 arguments required: MAX_TEAM, MAX_RESEARCH_TOPIC");
    }

    auto team = luaL_checkinteger(lua, 1);
    auto research_topic = luaL_checkinteger(lua, 2);

    if (team >= PLAYER_TEAM_MAX or team < PLAYER_TEAM_RED) {
        return luaL_error(lua, "[max_get_research_level] Error: Invalid MAX_TEAM (%d)", team);
    }

    if (research_topic != RESEARCH_TOPIC_ATTACK && research_topic != RESEARCH_TOPIC_SHOTS &&
        research_topic != RESEARCH_TOPIC_RANGE && research_topic != RESEARCH_TOPIC_ARMOR &&
        research_topic != RESEARCH_TOPIC_HITS && research_topic != RESEARCH_TOPIC_SPEED &&
        research_topic != RESEARCH_TOPIC_SCAN && research_topic != RESEARCH_TOPIC_COST) {
        return luaL_error(lua, "[max_get_research_level] Error: Invalid MAX_RESEARCH_TOPIC (%d)", research_topic);
    }

    auto result = GetResearchLevel(team, research_topic);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterGetResearchLevel(lua_State* lua) {
    lua_register(lua, "max_get_research_level", LuaGetResearchLevel);
}

bool TestScript(const std::string script, std::string* error) {
    auto lua = luaL_newstate();
    bool result;

    if (lua) {
        if ((luaL_loadbuffer(lua, script.data(), script.size(), "script")) != LUA_OK) {
            std::string local_error = lua_tostring(lua, -1);
            lua_pop(lua, 1);
            lua_close(lua);

            if (error) {
                *error = std::string("Script syntax error: ") + local_error;
            }

            result = false;

        } else {
            lua_close(lua);

            result = true;
        }

    } else {
        if (error) {
            *error = "Script memory allocation failed";
        }

        result = false;
    }

    return result;
}

bool RunScript(void* const handle, const std::string script, const ScriptParameters& args, ScriptParameters& results,
               std::string* error) {
    auto context = static_cast<Context*>(handle);
    bool local_result;

    if (context and context->m_lua) {
        if ((luaL_loadbuffer(context->m_lua, script.data(), script.size(), "script")) != LUA_OK) {
            std::string local_error = lua_tostring(context->m_lua, -1);
            lua_pop(context->m_lua, 1);

            if (error) {
                *error = std::string("Script syntax error: ") + local_error;
            }

            local_result = false;

        } else {
            MaxRegistryUpdate();

            ScriptArgsToLuaValues(context->m_lua, args);

            if (lua_pcall(context->m_lua, args.size(), results.size(), 0) != LUA_OK) {
                std::string local_error = lua_tostring(context->m_lua, -1);
                lua_pop(context->m_lua, 1);

                if (error) {
                    *error = std::string("Script runtime error: ") + local_error;
                }

                local_result = false;

            } else {
                if (lua_gettop(context->m_lua) == static_cast<int>(results.size())) {
                    ScriptLuaValuesToResults(context->m_lua, results);

                    local_result = true;

                } else {
                    lua_pop(context->m_lua, lua_gettop(context->m_lua));

                    local_result = false;
                }
            }
        }

    } else {
        if (error) {
            *error = "Invalid context";
        }

        local_result = false;
    }

    return local_result;
}

static void TimoutHook(lua_State* lua, lua_Debug* ar) { (void)luaL_error(lua, "Script ran out of time budget"); }

bool SetTimeBudget(void* const handle, const size_t time_budget) {
    auto context = static_cast<Context*>(handle);
    bool result;

    if (context and context->m_lua) {
        lua_sethook(context->m_lua, TimoutHook, LUA_MASKCOUNT,
                    (time_budget > MinimumTimeBudget) ? time_budget : MinimumTimeBudget);

        result = true;

    } else {
        result = false;
    }

    return result;
}

static int LuaPrintRedirect(lua_State* lua) {
    std::string string;

    int n = lua_gettop(lua);
    lua_getglobal(lua, "tostring");

    for (int i = 1; i <= n; i++) {
        lua_pushvalue(lua, -1);
        lua_pushvalue(lua, i);

        if (lua_pcall(lua, 1, 1, 0) != LUA_OK) {
            const char* error = lua_tostring(lua, -1);

            SDL_Log("\n%s\n", (std::string("Script error: ") + error).c_str());

            lua_pop(lua, 1);
        }

        const char* substring = lua_tostring(lua, -1);

        if (substring == nullptr) {
            substring = "<nil>";
        }

        if (i > 1) {
            string += "\t";
        }

        string += substring;

        lua_pop(lua, 1);
    }

    SDL_Log("\nScript message: %s\n", string.c_str());

    return 0;
}

static inline void AddEnum(lua_State* lua, const char* global_name, const MaxEnumType& fields) {
    SDL_assert(lua);

    lua_newtable(lua);

    for (const auto& field : fields) {
        lua_pushinteger(lua, field.second);
        lua_setfield(lua, -2, field.first);
    }

    lua_newtable(lua);
    lua_pushvalue(lua, -2);
    lua_setfield(lua, -2, "__index");
    lua_pushcfunction(lua,
                      [](lua_State* ctx) -> int { return luaL_error(ctx, "Script enumerated table is read-only"); });
    lua_setfield(lua, -2, "__newindex");
    lua_pushstring(lua, "protected");
    lua_setfield(lua, -2, "__metatable");
    lua_setmetatable(lua, -2);
    lua_setglobal(lua, global_name);
}

static inline void ScriptLuaValuesToResults(lua_State* lua, ScriptParameters& results) {
    int index = -results.size();

    for (auto& result : results) {
        switch (result.index()) {
            case 0: {
                result = static_cast<bool>(lua_toboolean(lua, index));
            } break;

            case 1: {
                result = static_cast<size_t>(lua_tointeger(lua, index));
            } break;

            case 2: {
                result = static_cast<double>(lua_tonumber(lua, index));
            } break;

            case 3: {
                result = std::string(lua_tostring(lua, index));
            } break;

            case 4: {
                if (lua_istable(lua, index)) {
                    ScriptTable table;
                    size_t length;

                    lua_len(lua, index);

                    length = static_cast<size_t>(lua_tointeger(lua, -1));

                    lua_pop(lua, 1);

                    table.reserve(length);

                    for (size_t i = 1; i <= length; ++i) {
                        lua_rawgeti(lua, index, i);

                        switch (lua_type(lua, -1)) {
                            case LUA_TBOOLEAN: {
                                table.push_back(static_cast<bool>(lua_toboolean(lua, -1)));
                            } break;

                            case LUA_TNUMBER: {
                                if (lua_isinteger(lua, -1)) {
                                    table.push_back(static_cast<size_t>(lua_tointeger(lua, -1)));
                                } else {
                                    table.push_back(static_cast<double>(lua_tonumber(lua, -1)));
                                }
                            } break;

                            case LUA_TSTRING: {
                                size_t len;
                                const char* str = lua_tolstring(lua, -1, &len);
                                table.push_back(std::string(str, len));
                                break;
                            }

                            default: {
                                SDL_assert(0);
                            } break;
                        }

                        lua_pop(lua, 1);
                    }

                    result = std::move(table);

                } else {
                    result = ScriptTable{};
                }
            } break;

            default: {
                SDL_assert(0);
            } break;
        }

        ++index;
    }

    lua_pop(lua, results.size());
}

static inline MaxRegistryValueType MaxRegistryFromLuaValue(lua_State* lua, int index) {
    int type = lua_type(lua, index);
    MaxRegistryValueType value;

    switch (type) {
        case LUA_TNIL: {
            value = MaxRegistryValueType{std::monostate{}};
        } break;

        case LUA_TBOOLEAN: {
            value = MaxRegistryValueType{static_cast<bool>(lua_toboolean(lua, index))};
        } break;

        case LUA_TNUMBER: {
            if (lua_isinteger(lua, index)) {
                value = MaxRegistryValueType{static_cast<lua_Integer>(lua_tointeger(lua, index))};

            } else {
                value = MaxRegistryValueType{static_cast<lua_Number>(lua_tonumber(lua, index))};
            }
        } break;

        case LUA_TSTRING: {
            value = MaxRegistryValueType{std::string(lua_tostring(lua, index))};
        } break;

        default: {
            value = MaxRegistryValueType{std::monostate{}};
        } break;
    }

    return value;
}

static inline void ScriptArgsToLuaValues(lua_State* lua, const ScriptParameters& args) {
    for (auto& arg : args) {
        switch (arg.index()) {
            case 0: {
                lua_pushboolean(lua, std::get<bool>(arg));
            } break;

            case 1: {
                lua_pushinteger(lua, std::get<size_t>(arg));
            } break;

            case 2: {
                lua_pushnumber(lua, std::get<double>(arg));
            } break;

            case 3: {
                const std::string& s = std::get<std::string>(arg);

                lua_pushlstring(lua, s.c_str(), s.size());
            } break;

            case 4: {
                const auto& values = std::get<ScriptTable>(arg);

                lua_createtable(lua, values.size(), 0);

                for (size_t i = 0; i < values.size(); ++i) {
                    const auto& value = values[i];

                    switch (value.index()) {
                        case 0: {
                            lua_pushboolean(lua, std::get<bool>(value));
                        } break;

                        case 1: {
                            lua_pushinteger(lua, std::get<size_t>(value));
                        } break;

                        case 2: {
                            lua_pushnumber(lua, std::get<double>(value));
                        } break;

                        case 3: {
                            const std::string& s = std::get<std::string>(value);
                            lua_pushlstring(lua, s.c_str(), s.size());
                        } break;

                        default: {
                            lua_pushnil(lua);
                        } break;
                    }

                    lua_rawseti(lua, -2, i + 1);
                }
            } break;

            default: {
                lua_pushnil(lua);

                SDL_assert(0);
            } break;
        }
    }
}

static inline void MaxRegistryToLuaValue(lua_State* lua, const MaxRegistryValueType& value) {
    switch (value.index()) {
        case 0: {
            lua_pushnil(lua);
        } break;

        case 1: {
            lua_pushboolean(lua, std::get<bool>(value));
        } break;

        case 2: {
            lua_pushinteger(lua, std::get<lua_Integer>(value));
        } break;

        case 3: {
            lua_pushnumber(lua, std::get<lua_Number>(value));
        } break;

        case 4: {
            const std::string& s = std::get<std::string>(value);

            lua_pushlstring(lua, s.c_str(), s.size());
        } break;

        default: {
            lua_pushnil(lua);
        } break;
    }
}

static std::string LoadDefaultBuildRules() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_L0001);
    uint8_t* file_base = ResourceManager_ReadResource(SC_L0001);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string script(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return script;
}

void Init() {
    if (MaxRegistryMutex == nullptr) {
        MaxRegistryMutex = ResourceManager_CreateMutex();

        if (MaxRegistryMutex == nullptr) {
            ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
        }
    }

    if (MaxRegistryFunctionsMutex == nullptr) {
        MaxRegistryFunctionsMutex = ResourceManager_CreateMutex();

        if (MaxRegistryFunctionsMutex == nullptr) {
            ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
        }
    }
}

static int MaxRegistryTableIndex(lua_State* lua) {
    auto key = lua_tostring(lua, 2);

    if (!key) {
        lua_pushnil(lua);

        return 1;
    }

    {
        ResourceManager_MutexLock lock(MaxRegistryMutex);

        auto it = MaxRegistry.find(key);

        if (it != MaxRegistry.end()) {
            MaxRegistryToLuaValue(lua, it->second);

        } else {
            lua_pushnil(lua);
        }
    }

    return 1;
}

static int MaxRegistryTableAddRemove(lua_State* lua) {
    auto key = lua_tostring(lua, 2);

    if (!key) {
        return 0;
    }

    {
        ResourceManager_MutexLock lock(MaxRegistryFunctionsMutex);

        auto it = MaxRegistryFunctions.find(key);

        if (it != MaxRegistryFunctions.end()) {
            return 0;
        }
    }

    auto value = MaxRegistryFromLuaValue(lua, 3);

    {
        ResourceManager_MutexLock lock(MaxRegistryMutex);

        if (std::holds_alternative<std::monostate>(value)) {
            MaxRegistry.erase(key);

        } else {
            MaxRegistry[key] = value;
        }
    }

    return 0;
}

void RegisterMaxRegistry(lua_State* lua) {
    lua_newtable(lua);
    lua_newtable(lua);
    lua_pushcfunction(lua, MaxRegistryTableIndex);
    lua_setfield(lua, -2, "__index");
    lua_pushcfunction(lua, MaxRegistryTableAddRemove);
    lua_setfield(lua, -2, "__newindex");
    lua_setmetatable(lua, -2);
    lua_setglobal(lua, "MAX_REGISTRY");
}

void MaxRegistryRegister(const std::string key, MaxRegistryFunctionType fn) {
    ResourceManager_MutexLock lock(MaxRegistryFunctionsMutex);

    MaxRegistryFunctions[key] = fn;
}

void MaxRegistryRemove(const std::string key) {
    ResourceManager_MutexLock lock1(MaxRegistryFunctionsMutex);
    ResourceManager_MutexLock lock2(MaxRegistryMutex);

    MaxRegistryFunctions.erase(key);
    MaxRegistry.erase(key);
}

void MaxRegistryReset() {
    ResourceManager_MutexLock lock1(MaxRegistryFunctionsMutex);
    ResourceManager_MutexLock lock2(MaxRegistryMutex);

    MaxRegistryFunctions.clear();
    MaxRegistry.clear();
}

void MaxRegistryUpdate() {
    ResourceManager_MutexLock lock1(MaxRegistryFunctionsMutex);
    ResourceManager_MutexLock lock2(MaxRegistryMutex);

    for (const auto& [key, fn] : MaxRegistryFunctions) {
        auto value = fn();

        switch (value.index()) {
            case 0: {
                MaxRegistry[key] = std::get<bool>(value);
            } break;

            case 1: {
                MaxRegistry[key] = static_cast<lua_Integer>(std::get<size_t>(value));
            } break;

            case 2: {
                MaxRegistry[key] = static_cast<lua_Number>(std::get<double>(value));
            } break;

            case 3: {
                MaxRegistry[key] = std::get<std::string>(value);
            } break;
        }
    }
}

void* CreateContext(const ScriptType type) {
    auto context = new (std::nothrow) Context();
    void* result;

    if (context) {
        lua_State* lua = luaL_newstate();

        if (lua) {
            luaL_openlibs(lua);

            lua_pushnil(lua);
            lua_setglobal(lua, "os");

            lua_pushnil(lua);
            lua_setglobal(lua, "io");

            lua_pushnil(lua);
            lua_setglobal(lua, "package");

            lua_pushnil(lua);
            lua_setglobal(lua, "debug");

            lua_pushcfunction(lua, LuaPrintRedirect);
            lua_setglobal(lua, "print");

            context->m_lua = lua;
            context->m_type = type;

            {
                MaxEnumType fields(UNIT_END + 1);

                for (int32_t i = 0; i < UNIT_END; ++i) {
                    fields[i] = {ResourceManager_GetResourceID(static_cast<ResourceID>(i)), i};
                }

                fields[UNIT_END] = {"INVALID_ID", INVALID_ID};

                AddEnum(lua, "MAX_UNIT", fields);
            }

            {
                MaxEnumType fields(V_END - V_START);

                for (int32_t i = 0; i < V_END - V_START; ++i) {
                    fields[i] = {ResourceManager_GetResourceID(static_cast<ResourceID>(V_START + 1 + i)),
                                 V_START + 1 + i};
                }

                AddEnum(lua, "MAX_VOICE", fields);
            }

            {
                MaxEnumType fields(LOGOFLIC - FXS_END);

                for (int32_t i = 0; i < LOGOFLIC - FXS_END; ++i) {
                    fields[i] = {ResourceManager_GetResourceID(static_cast<ResourceID>(LOGOFLIC + 1 + i)),
                                 LOGOFLIC + 1 + i};
                }

                AddEnum(lua, "MAX_MUSIC", fields);
            }

            {
                MaxEnumType fields(ENDARM - RSRCHPIC);

                for (int32_t i = 0; i < ENDARM - RSRCHPIC; ++i) {
                    fields[i] = {ResourceManager_GetResourceID(static_cast<ResourceID>(RSRCHPIC + 1 + i)),
                                 RSRCHPIC + 1 + i};
                }

                AddEnum(lua, "MAX_IMAGE", fields);
            }

            {
                AddEnum(lua, "MAX_ICON",
                        {
                            {"LIPS", LIPS},
                            {"COMPLEX", I_CMPLX},
                        });
            }

            {
                AddEnum(lua, "MAX_TEAM",
                        {
                            {"RED", PLAYER_TEAM_RED},
                            {"GREEN", PLAYER_TEAM_GREEN},
                            {"BLUE", PLAYER_TEAM_BLUE},
                            {"GRAY", PLAYER_TEAM_GRAY},
                            {"DERELICT", PLAYER_TEAM_ALIEN},
                        });
            }

            {
                AddEnum(lua, "MAX_CARGO_TYPE",
                        {
                            {"RAW", CARGO_TYPE_RAW},
                            {"FUEL", CARGO_TYPE_FUEL},
                            {"GOLD", CARGO_TYPE_GOLD},
                        });
            }

            {
                AddEnum(lua, "MAX_UNIT_CLASS",
                        {
                            {"MISSILE", MISSILE_UNIT},
                            {"MOBILE_LAND_SEA", MOBILE_LAND_UNIT | MOBILE_SEA_UNIT},
                            {"MOBILE_AIR", MOBILE_AIR_UNIT},
                            {"STATIONARY", STATIONARY},
                        });
            }

            {
                AddEnum(lua, "MAX_UNIT_ATTRIB",
                        {
                            {"ATTACK", ATTRIB_ATTACK},
                            {"SHOTS", ATTRIB_ROUNDS},
                            {"RANGE", ATTRIB_RANGE},
                            {"ARMOR", ATTRIB_ARMOR},
                            {"HITS", ATTRIB_HITS},
                            {"SPEED", ATTRIB_SPEED},
                            {"SCAN", ATTRIB_SCAN},
                            {"COST", ATTRIB_TURNS},
                            {"AMMO", ATTRIB_AMMO},
                            {"MOVE_AND_FIRE", ATTRIB_MOVE_AND_FIRE},
                            {"STORAGE", ATTRIB_STORAGE},
                            {"ATTACK_RADIUS", ATTRIB_ATTACK_RADIUS},
                            {"AGENT_ADJUST", ATTRIB_AGENT_ADJUST},
                        });
            }

            {
                AddEnum(lua, "MAX_RESEARCH_TOPIC",
                        {
                            {"ATTACK", RESEARCH_TOPIC_ATTACK},
                            {"SHOTS", RESEARCH_TOPIC_SHOTS},
                            {"RANGE", RESEARCH_TOPIC_RANGE},
                            {"ARMOR", RESEARCH_TOPIC_ARMOR},
                            {"HITS", RESEARCH_TOPIC_HITS},
                            {"SPEED", RESEARCH_TOPIC_SPEED},
                            {"SCAN", RESEARCH_TOPIC_SCAN},
                            {"COST", RESEARCH_TOPIC_COST},
                        });
            }

            {
                RegisterMaxRegistry(lua);

                if (type == WINLOSS_CONDITIONS) {
                    AddEnum(lua, "MAX_VICTORY_STATE",
                            {
                                {"GENERIC", VICTORY_STATE_GENERIC},
                                {"PENDING", VICTORY_STATE_PENDING},
                                {"WON", VICTORY_STATE_WON},
                                {"LOST", VICTORY_STATE_LOST},
                            });

                    RegisterHasMaterials(lua);
                    RegisterCountReadyUnits(lua);
                    RegisterHasAttackPower(lua);
                    RegisterCanRebuildComplex(lua);
                    RegisterCanRebuildBuilders(lua);
                    RegisterCountTotalMining(lua);
                    RegisterGetTotalUnitsBeingConstructed(lua);
                    RegisterGetTotalPowerConsumption(lua);
                    RegisterGetTotalMining(lua);
                    RegisterGetTotalConsumption(lua);
                    RegisterHasUnitAboveMarkLevel(lua);
                    RegisterHasUnitExperience(lua);
                    RegisterCountCasualties(lua);
                    RegisterCountDamangedUnits(lua);
                    RegisterCountUnitsAboveAttribLevel(lua);
                    RegisterCountUnitsWithReducedAttrib(lua);
                    RegisterGetResearchLevel(lua);
                }

                if (type == GAME_RULES) {
                    ScriptParameters args{};
                    ScriptParameters results{};
                    std::string script = LoadDefaultBuildRules();
                    std::string error;

                    if (!RunScript(static_cast<void*>(context), script, args, results, &error)) {
                        SDL_Log("\n%s\n", error.c_str());

                        SDL_assert(0);
                    }
                }
            }

            result = context;

        } else {
            delete context;
            result = nullptr;
        }

    } else {
        result = nullptr;
    }

    return result;
}

void DestroyContext(void* handle) {
    auto context = static_cast<Context*>(handle);

    if (context) {
        if (context->m_lua) {
            lua_close(context->m_lua);
        }

        delete context;
    }
}

}  // namespace Scripter
