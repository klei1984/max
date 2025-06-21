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
static lua_Integer CountReadyUnits(lua_Integer team, lua_Integer unit_type);
static int LuaCountReadyUnits(lua_State* lua);
static void RegisterCountReadyUnits(lua_State* lua);
static void TimoutHook(lua_State* lua, lua_Debug* ar);
static int LuaPrintRedirect(lua_State* lua);
static inline void AddEnum(lua_State* lua, const char* global_name, const MaxEnumType& fields);
static inline void ScriptLuaValuesToResults(lua_State* lua, ScriptParameters& results);
static inline MaxRegistryValueType MaxRegistryFromLuaValue(lua_State* lua, int index);
static inline void ScriptArgsToLuaValues(lua_State* lua, const ScriptParameters& args);
static inline void MaxRegistryToLuaValue(lua_State* lua, const MaxRegistryValueType& value);
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
        return luaL_error(lua, "[max_has_materials] Error: Invalid MAX_TEAM (%i)", team);
    }

    if (cargo_type < CARGO_TYPE_RAW or cargo_type >= CARGO_TYPE_GOLD) {
        return luaL_error(lua, "[max_has_materials] Error: Invalid MAX_CARGO_TYPE (%i)", cargo_type);
    }

    auto result = HasMaterials(team, cargo_type);

    lua_pushinteger(lua, result);

    return 1;
}

static void RegisterHasMaterials(lua_State* lua) { lua_register(lua, "max_has_materials", LuaHasMaterials); }

static lua_Integer CountReadyUnits(lua_Integer team, lua_Integer unit_type) {
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
        return luaL_error(lua, "[max_count_ready_units] Error: Invalid MAX_TEAM (%i)", team);
    }

    if (unit_type > UNIT_END or unit_type < COMMTWR) {
        return luaL_error(lua, "[max_count_ready_units] Error: Invalid MAX_UNIT (%i)", unit_type);
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
        return luaL_error(lua, "[max_has_attack_power] Error: Invalid MAX_TEAM (%i)", team);
    }

    if (unit_class != (MOBILE_LAND_UNIT | MOBILE_SEA_UNIT) or unit_class != MOBILE_AIR_UNIT or
        unit_class != STATIONARY) {
        return luaL_error(lua, "[max_has_attack_power] Error: Invalid MAX_UNIT_CLASS (%i)", unit_class);
    }

    auto result = HasAttackPower(team, unit_class);

    lua_pushboolean(lua, result);

    return 1;
}

static void RegisterHasAttackPower(lua_State* lua) { lua_register(lua, "max_has_attack_power", LuaHasAttackPower); }

bool TestScript(const std::string script, std::string* error) {
    auto lua = luaL_newstate();
    ;
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

            SDL_Log("%s", (std::string("Script error: ") + error).c_str());

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

    SDL_Log("Script message: %s", string.c_str());

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
    lua_newtable(lua);
    lua_pushvalue(lua, -3);
    lua_setfield(lua, -2, "__index");
    lua_pushcfunction(lua,
                      [](lua_State* ctx) -> int { return luaL_error(ctx, "Script enumerated table is read-only"); });
    lua_setfield(lua, -2, "__newindex");
    lua_pushboolean(lua, 0);
    lua_pushstring(lua, "protected");
    lua_setfield(lua, -2, "__metatable");
    lua_setmetatable(lua, -2);
    lua_setglobal(lua, global_name);
    lua_pop(lua, 1);
}

static inline void ScriptLuaValuesToResults(lua_State* lua, ScriptParameters& results) {
    int index = -results.size();

    for (auto& result : results) {
        switch (result.index()) {
            case 0: {
                result = static_cast<bool>(lua_toboolean(lua, index++));
            } break;

            case 1: {
                result = static_cast<size_t>(lua_tointeger(lua, index++));
            } break;

            case 2: {
                result = static_cast<double>(lua_tonumber(lua, index++));
            } break;

            case 3: {
                result = std::string(lua_tostring(lua, index++));
            } break;

            default: {
                index++;

                SDL_assert(0);
            } break;
        }
    }

    lua_pop(lua, 0);
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

void Init() {
    if (MaxRegistryMutex == nullptr) {
        MaxRegistryMutex = SDL_CreateMutex();

        if (MaxRegistryMutex == nullptr) {
            ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
        }
    }

    if (MaxRegistryFunctionsMutex == nullptr) {
        MaxRegistryFunctionsMutex = SDL_CreateMutex();

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
        SDL_LockMutex(MaxRegistryMutex);

        auto it = MaxRegistry.find(key);

        if (it != MaxRegistry.end()) {
            MaxRegistryToLuaValue(lua, it->second);

        } else {
            lua_pushnil(lua);
        }

        SDL_UnlockMutex(MaxRegistryMutex);
    }

    return 1;
}

static int MaxRegistryTableAddRemove(lua_State* lua) {
    auto key = lua_tostring(lua, 2);

    if (!key) {
        return 0;
    }

    {
        SDL_LockMutex(MaxRegistryFunctionsMutex);

        auto it = MaxRegistryFunctions.find(key);

        if (it != MaxRegistryFunctions.end()) {
            SDL_UnlockMutex(MaxRegistryFunctionsMutex);
            return 0;
        }

        SDL_UnlockMutex(MaxRegistryFunctionsMutex);
    }

    auto value = MaxRegistryFromLuaValue(lua, 3);

    {
        SDL_LockMutex(MaxRegistryMutex);

        if (std::holds_alternative<std::monostate>(value)) {
            MaxRegistry.erase(key);

        } else {
            MaxRegistry[key] = value;
        }

        SDL_UnlockMutex(MaxRegistryMutex);
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
    SDL_LockMutex(MaxRegistryFunctionsMutex);

    MaxRegistryFunctions[key] = fn;

    SDL_UnlockMutex(MaxRegistryFunctionsMutex);
}

void MaxRegistryRemove(const std::string key) {
    SDL_LockMutex(MaxRegistryFunctionsMutex);
    SDL_LockMutex(MaxRegistryMutex);

    MaxRegistryFunctions.erase(key);
    MaxRegistry.erase(key);

    SDL_UnlockMutex(MaxRegistryMutex);
    SDL_UnlockMutex(MaxRegistryFunctionsMutex);
}

void MaxRegistryReset() {
    SDL_LockMutex(MaxRegistryFunctionsMutex);
    SDL_LockMutex(MaxRegistryMutex);

    MaxRegistryFunctions.clear();
    MaxRegistry.clear();

    SDL_UnlockMutex(MaxRegistryMutex);
    SDL_UnlockMutex(MaxRegistryFunctionsMutex);
}

void MaxRegistryUpdate() {
    SDL_LockMutex(MaxRegistryFunctionsMutex);
    SDL_LockMutex(MaxRegistryMutex);

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

    SDL_UnlockMutex(MaxRegistryMutex);
    SDL_UnlockMutex(MaxRegistryFunctionsMutex);
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
                MaxEnumType fields(UNIT_END);

                fields.clear();

                for (int32_t i = 0; i < UNIT_END; ++i) {
                    fields[i] = {ResourceManager_GetResourceID(static_cast<ResourceID>(i)), i};
                }

                fields[UNIT_END] = {ResourceManager_GetResourceID(static_cast<ResourceID>(INVALID_ID)), INVALID_ID};

                AddEnum(lua, "MAX_UNIT", fields);
            }

            {
                MaxEnumType fields(V_END - V_START);

                fields.clear();

                for (int32_t i = 0; i < V_END - V_START; ++i) {
                    fields[i] = {ResourceManager_GetResourceID(static_cast<ResourceID>(V_START + 1 + i)),
                                 V_START + 1 + i};
                }

                AddEnum(lua, "MAX_VOICE", fields);
            }

            {
                MaxEnumType fields(LOGOFLIC - FXS_END);

                fields.clear();

                for (int32_t i = 0; i < LOGOFLIC - FXS_END; ++i) {
                    fields[i] = {ResourceManager_GetResourceID(static_cast<ResourceID>(LOGOFLIC + 1 + i)),
                                 LOGOFLIC + 1 + i};
                }

                AddEnum(lua, "MAX_MUSIC", fields);
            }

            {
                MaxEnumType fields(ENDARM - RSRCHPIC);

                fields.clear();

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
                RegisterMaxRegistry(lua);

                if (type == WINLOSS_CONDITIONS) {
                    RegisterHasMaterials(lua);
                    RegisterCountReadyUnits(lua);
                    RegisterHasAttackPower(lua);
                }

                if (type == GAME_RULES) {
                    ScriptParameters args{};
                    ScriptParameters results{};
                    std::string error;

                    if (!RunScript(
                            static_cast<void*>(context),
                            "max_builder_capability_list={[MAX_UNIT.CONSTRCT]={MAX_UNIT.MININGST,MAX_UNIT.POWERSTN,MAX_"
                            "UNIT.LIGHTPLT,MAX_UNIT.LANDPLT,MAX_UNIT.AIRPLT,MAX_UNIT.SHIPYARD,MAX_UNIT.COMMTWR,MAX_"
                            "UNIT.DEPOT,MAX_UNIT.HANGAR,MAX_UNIT.DOCK,MAX_UNIT.HABITAT,MAX_UNIT.RESEARCH,MAX_UNIT."
                            "GREENHSE,MAX_UNIT.TRAINHAL,MAX_UNIT.BARRACKS},[MAX_UNIT.ENGINEER]={MAX_UNIT.ADUMP,MAX_"
                            "UNIT.FDUMP,MAX_UNIT.GOLDSM,MAX_UNIT.POWGEN,MAX_UNIT.CNCT_4W,MAX_UNIT.RADAR,MAX_UNIT."
                            "GUNTURRT,MAX_UNIT.ANTIAIR,MAX_UNIT.ARTYTRRT,MAX_UNIT.ANTIMSSL,MAX_UNIT.LANDPAD,MAX_UNIT."
                            "BRIDGE,MAX_UNIT.WTRPLTFM,MAX_UNIT.BLOCK,MAX_UNIT.ROAD},[MAX_UNIT.LIGHTPLT]={MAX_UNIT."
                            "SCOUT,MAX_UNIT.SURVEYOR,MAX_UNIT.ENGINEER,MAX_UNIT.REPAIR,MAX_UNIT.SPLYTRCK,MAX_UNIT."
                            "FUELTRCK,MAX_UNIT.GOLDTRCK,MAX_UNIT.SP_FLAK,MAX_UNIT.MINELAYR,MAX_UNIT.BULLDOZR,MAX_UNIT."
                            "CLNTRANS},[MAX_UNIT.LANDPLT]={MAX_UNIT.CONSTRCT,MAX_UNIT.SCANNER,MAX_UNIT.TANK,MAX_UNIT."
                            "ARTILLRY,MAX_UNIT.ROCKTLCH,MAX_UNIT.MISSLLCH},[MAX_UNIT.AIRPLT]={MAX_UNIT.FIGHTER,MAX_"
                            "UNIT.BOMBER,MAX_UNIT.AIRTRANS,MAX_UNIT.AWAC},[MAX_UNIT.SHIPYARD]={MAX_UNIT.FASTBOAT,MAX_"
                            "UNIT.CORVETTE,MAX_UNIT.BATTLSHP,MAX_UNIT.SUBMARNE,MAX_UNIT.SEATRANS,MAX_UNIT.MSSLBOAT,MAX_"
                            "UNIT.SEAMNLYR,MAX_UNIT.CARGOSHP},[MAX_UNIT.TRAINHAL]={MAX_UNIT.COMMANDO,MAX_UNIT.INFANTRY}"
                            "}function max_get_builder_type(a)for b,c in pairs(max_builder_capability_list)do for d,e "
                            "in ipairs(c)do if e==a then return b end end end;return MAX_UNIT.INVALID_ID end",
                            args, results, &error)) {
                        SDL_Log("%s", error.c_str());

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
