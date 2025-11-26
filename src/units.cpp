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

#include "units.hpp"

#include <SDL3/SDL.h>

#include <fstream>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "resource_manager.hpp"
#include "resourcetable.hpp"

using json = nlohmann::json;
using validator = nlohmann::json_schema::json_validator;

static const std::unordered_map<std::string, Unit::Flags> FlagMap = {
    {"GROUND_COVER", Unit::Flags::GROUND_COVER},
    {"EXPLODING", Unit::Flags::EXPLODING},
    {"ANIMATED", Unit::Flags::ANIMATED},
    {"CONNECTOR_UNIT", Unit::Flags::CONNECTOR_UNIT},
    {"BUILDING", Unit::Flags::BUILDING},
    {"MISSILE_UNIT", Unit::Flags::MISSILE_UNIT},
    {"MOBILE_AIR_UNIT", Unit::Flags::MOBILE_AIR_UNIT},
    {"MOBILE_SEA_UNIT", Unit::Flags::MOBILE_SEA_UNIT},
    {"MOBILE_LAND_UNIT", Unit::Flags::MOBILE_LAND_UNIT},
    {"STATIONARY", Unit::Flags::STATIONARY},
    {"UPGRADABLE", Unit::Flags::UPGRADABLE},
    {"HOVERING", Unit::Flags::HOVERING},
    {"HAS_FIRING_SPRITE", Unit::Flags::HAS_FIRING_SPRITE},
    {"FIRES_MISSILES", Unit::Flags::FIRES_MISSILES},
    {"CONSTRUCTOR_UNIT", Unit::Flags::CONSTRUCTOR_UNIT},
    {"ELECTRONIC_UNIT", Unit::Flags::ELECTRONIC_UNIT},
    {"SELECTABLE", Unit::Flags::SELECTABLE},
    {"STANDALONE", Unit::Flags::STANDALONE},
    {"REQUIRES_SLAB", Unit::Flags::REQUIRES_SLAB},
    {"TURRET_SPRITE", Unit::Flags::TURRET_SPRITE},
    {"SENTRY_UNIT", Unit::Flags::SENTRY_UNIT},
    {"SPINNING_TURRET", Unit::Flags::SPINNING_TURRET},
    {"REGENERATING_UNIT", Unit::Flags::REGENERATING_UNIT}};

static const std::unordered_map<std::string, Unit::SurfaceType> SurfaceTypeMap = {
    {"SURFACE_TYPE_NONE", Unit::SurfaceType::SURFACE_TYPE_NONE},
    {"SURFACE_TYPE_LAND", Unit::SurfaceType::SURFACE_TYPE_LAND},
    {"SURFACE_TYPE_WATER", Unit::SurfaceType::SURFACE_TYPE_WATER},
    {"SURFACE_TYPE_COAST", Unit::SurfaceType::SURFACE_TYPE_COAST},
    {"SURFACE_TYPE_AIR", Unit::SurfaceType::SURFACE_TYPE_AIR},
    {"SURFACE_TYPE_ALL", Unit::SurfaceType::SURFACE_TYPE_ALL}};

static const std::unordered_map<std::string, Unit::CargoType> CargoTypeMap = {
    {"CARGO_TYPE_NONE", Unit::CargoType::CARGO_TYPE_NONE}, {"CARGO_TYPE_RAW", Unit::CargoType::CARGO_TYPE_RAW},
    {"CARGO_TYPE_FUEL", Unit::CargoType::CARGO_TYPE_FUEL}, {"CARGO_TYPE_GOLD", Unit::CargoType::CARGO_TYPE_GOLD},
    {"CARGO_TYPE_LAND", Unit::CargoType::CARGO_TYPE_LAND}, {"CARGO_TYPE_SEA", Unit::CargoType::CARGO_TYPE_SEA},
    {"CARGO_TYPE_AIR", Unit::CargoType::CARGO_TYPE_AIR}};

static const std::unordered_map<std::string, Unit::SfxType> SfxTypeMap = {
    {"idle", Unit::SfxType::SFX_TYPE_IDLE},
    {"water_idle", Unit::SfxType::SFX_TYPE_WATER_IDLE},
    {"drive", Unit::SfxType::SFX_TYPE_DRIVE},
    {"water_drive", Unit::SfxType::SFX_TYPE_WATER_DRIVE},
    {"stop", Unit::SfxType::SFX_TYPE_STOP},
    {"water_stop", Unit::SfxType::SFX_TYPE_WATER_STOP},
    {"transform", Unit::SfxType::SFX_TYPE_TRANSFORM},
    {"building", Unit::SfxType::SFX_TYPE_BUILDING},
    {"shrink", Unit::SfxType::SFX_TYPE_SHRINK},
    {"expand", Unit::SfxType::SFX_TYPE_EXPAND},
    {"turret", Unit::SfxType::SFX_TYPE_TURRET},
    {"fire", Unit::SfxType::SFX_TYPE_FIRE},
    {"hit", Unit::SfxType::SFX_TYPE_HIT},
    {"expload", Unit::SfxType::SFX_TYPE_EXPLOAD},
    {"power_consumption_start", Unit::SfxType::SFX_TYPE_POWER_CONSUMPTION_START},
    {"power_consumption_end", Unit::SfxType::SFX_TYPE_POWER_CONSUMPTION_END},
    {"land", Unit::SfxType::SFX_TYPE_LAND},
    {"take", Unit::SfxType::SFX_TYPE_TAKE}};

static uint32_t ParseFlags(const json& flags_array);
static uint8_t ParseLandType(const json& land_type_array);
static Unit::CargoType ParseCargoType(const std::string& cargo_type_str);
static Unit::Gender ParseGender(const std::string& gender_str);
static uint32_t ParseHexString(const std::string& hex_str);
static ResourceID ParseResourceID(const std::string& resource_str);
static std::unordered_map<Unit::SfxType, Unit::SoundEffectInfo> ParseSoundEffects(
    const json& sfx_types_data, const json& default_sound_effects_data, const json& unit_sound_effects_data);
static Unit* CreateUnitFromJson(const json& sfx_types_data, const json& default_sound_effects_data,
                                const json& unit_data);

uint32_t ParseFlags(const json& flags_array) {
    uint32_t flags = 0;

    for (const auto& flag_str : flags_array) {
        std::string flag = flag_str.get<std::string>();
        auto it = FlagMap.find(flag);

        if (it != FlagMap.end()) {
            flags |= it->second;

        } else {
            SDL_Log("Units: Unknown flag '%s'\n", flag.c_str());
        }
    }

    return flags;
}

uint8_t ParseLandType(const json& land_type_array) {
    uint8_t land_type = 0;
    for (const auto& type_str : land_type_array) {
        std::string type = type_str.get<std::string>();
        auto it = SurfaceTypeMap.find(type);

        if (it != SurfaceTypeMap.end()) {
            land_type |= it->second;

        } else {
            SDL_Log("Units: Unknown surface type '%s'\n", type.c_str());
        }
    }

    return land_type;
}

Unit::CargoType ParseCargoType(const std::string& cargo_type_str) {
    auto it = CargoTypeMap.find(cargo_type_str);

    if (it != CargoTypeMap.end()) {
        return it->second;
    }

    SDL_Log("Units: Unknown cargo type '%s'\n", cargo_type_str.c_str());

    return Unit::CargoType::CARGO_TYPE_NONE;
}

Unit::Gender ParseGender(const std::string& gender_str) {
    Unit::Gender result;

    if (gender_str == "M") {
        result = Unit::Gender::GENDER_MASCULINE;

    } else if (gender_str == "F") {
        result = Unit::Gender::GENDER_FEMININE;

    } else if (gender_str == "N") {
        result = Unit::Gender::GENDER_NEUTER;

    } else {
        SDL_Log("Units: Unknown gender '%s'\n", gender_str.c_str());

        result = Unit::Gender::GENDER_NEUTER;
    }

    return result;
}

uint32_t ParseHexString(const std::string& hex_str) {
    uint32_t result;

    if (hex_str.empty() || hex_str == "0x0") {
        result = 0;

    } else {
        try {
            result = static_cast<uint32_t>(std::stoul(hex_str, nullptr, 16));

        } catch (const std::exception& e) {
            SDL_Log("Units: Failed to parse hex string '%s': %s\n", hex_str.c_str(), e.what());

            result = 0;
        }
    }

    return result;
}

ResourceID ParseResourceID(const std::string& resource_str) {
    ResourceID result;

    if (resource_str == "INVALID_ID") {
        result = INVALID_ID;

    } else {
        result = ResourceManager_GetResourceID(resource_str);
    }

    return result;
}

std::unordered_map<Unit::SfxType, Unit::SoundEffectInfo> ParseSoundEffects(const json& sfx_types_data,
                                                                           const json& default_sound_effects_data,
                                                                           const json& unit_sound_effects_data) {
    std::unordered_map<Unit::SfxType, Unit::SoundEffectInfo> sound_effects;

    for (const auto& [sfx_key, sfx_type_flags] : sfx_types_data.items()) {
        auto it = SfxTypeMap.find(sfx_key);

        if (it == SfxTypeMap.end()) {
            continue;
        }

        Unit::SfxType sfx_type = it->second;
        Unit::SoundEffectInfo info;

        info.persistent = sfx_type_flags.at("persistent").get<bool>();
        info.looping = sfx_type_flags.at("infinite_looping").get<bool>();

        bool has_unit_specific = false;
        if (unit_sound_effects_data.contains(sfx_key)) {
            const auto& unit_sfx = unit_sound_effects_data[sfx_key];

            info.resource_id = ParseResourceID(unit_sfx.at("resource_id").get<std::string>());
            info.volume = unit_sfx.at("volume").get<uint32_t>();
            has_unit_specific = true;

        } else if (default_sound_effects_data.contains(sfx_key)) {
            const auto& default_sfx = default_sound_effects_data[sfx_key];

            info.resource_id = ParseResourceID(default_sfx.at("resource_id").get<std::string>());
            info.volume = default_sfx.at("volume").get<uint32_t>();

        } else {
            info.resource_id = INVALID_ID;
            info.volume = 0;
        }

        bool file_exists = false;

        if (info.resource_id != INVALID_ID) {
            auto fp = ResourceManager_OpenFileResource(info.resource_id, ResourceType_GameData);
            if (fp) {
                file_exists = true;

                fclose(fp);
            }
        }

        if (has_unit_specific && !file_exists && default_sound_effects_data.contains(sfx_key)) {
            const auto& default_sfx = default_sound_effects_data[sfx_key];

            info.resource_id = ParseResourceID(default_sfx.at("resource_id").get<std::string>());
            info.volume = default_sfx.at("volume").get<uint32_t>();
            file_exists = false;
            // must not mark as default sound!
            if (info.resource_id != INVALID_ID) {
                auto fp = ResourceManager_OpenFileResource(info.resource_id, ResourceType_GameData);
                if (fp) {
                    file_exists = true;

                    fclose(fp);
                }
            }
        }

        info.is_default = !has_unit_specific;
        sound_effects[sfx_type] = info;
    }

    SDL_assert(sound_effects.size() == Unit::SFX_TYPE_LIMIT - 1);

    return sound_effects;
}

Unit* CreateUnitFromJson(const json& sfx_types_data, const json& default_sound_effects_data, const json& unit_data) {
    uint32_t flags = ParseFlags(unit_data.at("flags"));
    ResourceID sprite = ParseResourceID(unit_data.at("sprite").get<std::string>());
    ResourceID shadow = ParseResourceID(unit_data.at("shadow").get<std::string>());
    ResourceID data = ParseResourceID(unit_data.at("data").get<std::string>());
    ResourceID flics_animation = ParseResourceID(unit_data.at("flics_animation").get<std::string>());
    ResourceID portrait = ParseResourceID(unit_data.at("portrait").get<std::string>());
    ResourceID icon = ParseResourceID(unit_data.at("icon").get<std::string>());
    ResourceID armory_portrait = ParseResourceID(unit_data.at("armory_portrait").get<std::string>());
    uint8_t land_type = ParseLandType(unit_data.at("land_type"));
    Unit::CargoType cargo_type = ParseCargoType(unit_data.at("cargo_type").get<std::string>());
    Unit::Gender gender = ParseGender(unit_data.at("gender").get<std::string>());
    uint32_t singular_name = ParseHexString(unit_data.value("singular_name", "0xffff"));
    uint32_t plural_name = ParseHexString(unit_data.value("plural_name", "0xffff"));
    uint32_t description = ParseHexString(unit_data.value("description", "0xffff"));
    uint32_t tutorial_description = ParseHexString(unit_data.value("tutorial_description", "0xffff"));
    json unit_sound_effects = unit_data.value("sound_effects", json::object());
    std::unordered_map<Unit::SfxType, Unit::SoundEffectInfo> sound_effects =
        ParseSoundEffects(sfx_types_data, default_sound_effects_data, unit_sound_effects);

    return new (std::nothrow)
        Unit(flags, sprite, shadow, data, flics_animation, portrait, icon, armory_portrait, land_type, cargo_type,
             gender, singular_name, plural_name, description, tutorial_description, std::move(sound_effects));
}

Units::Units() : m_units(std::make_unique<std::unordered_map<std::string, Unit*>>()) {}

Units::~Units() {
    if (m_units) {
        for (auto& [key, unit] : *m_units) {
            delete unit;
        }
    }
}

[[nodiscard]] std::string Units::LoadSchema() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_SCHEU);
    uint8_t* file_base = ResourceManager_ReadResource(SC_SCHEU);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string schema(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return schema;
}

[[nodiscard]] bool Units::LoadScript(const std::string& script) {
    bool result{false};

    for (auto& [key, unit] : *m_units) {
        delete unit;
    }

    m_units->clear();

    try {
        validator validator;
        json jschema = json::parse(LoadSchema());
        json jscript = json::parse(script);

        validator.set_root_schema(jschema);
        validator.validate(jscript);

        const auto& sfx_types = jscript["sfx_types"];
        const auto& default_sound_effects = jscript["default_sound_effects"];
        const auto& units_object = jscript["units"];

        for (const auto& [unit_name, unit_data] : units_object.items()) {
            Unit* unit = CreateUnitFromJson(sfx_types, default_sound_effects, unit_data);

            if (unit) {
                (*m_units)[unit_name] = unit;

            } else {
                SDL_Log("Units: Failed to create unit '%s'\n", unit_name.c_str());
            }
        }

        result = true;

    } catch (const json::parse_error& e) {
        SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());

    } catch (const std::exception& e) {
        SDL_Log("\n%s\n", (std::string("JSON error: ") + e.what()).c_str());
    }

    return result;
}

[[nodiscard]] bool Units::LoadFile(const std::string& path) {
    std::ifstream fs(path.c_str());
    bool result{false};

    if (fs) {
        std::string contents((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

        result = LoadScript(contents);
    }

    return result;
}

[[nodiscard]] bool Units::LoadResource() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_UNITS);
    uint8_t* file_base = ResourceManager_ReadResource(SC_UNITS);
    bool result{false};

    if (file_size && file_base) {
        for (size_t i = 0; i < file_size; ++i) {
            file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
        }

        std::string contents(reinterpret_cast<const char*>(file_base), file_size);

        result = LoadScript(contents);

        delete[] file_base;
    }

    return result;
}

[[nodiscard]] Unit* Units::GetUnit(const std::string& unit_id) const {
    if (m_units) {
        auto it = m_units->find(unit_id);

        if (it != m_units->end()) {
            return it->second;
        }
    }

    return nullptr;
}

Units::iterator Units::begin() { return iterator(m_units->begin()); }

Units::iterator Units::end() { return iterator(m_units->end()); }

Units::const_iterator Units::begin() const { return const_iterator(m_units->begin()); }

Units::const_iterator Units::end() const { return const_iterator(m_units->end()); }

Units::const_iterator Units::cbegin() const { return const_iterator(m_units->cbegin()); }

Units::const_iterator Units::cend() const { return const_iterator(m_units->cend()); }
