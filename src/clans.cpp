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

#include "clans.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <fstream>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "resource_manager.hpp"
#include "scripter.hpp"

using json = nlohmann::json;
using validator = nlohmann::json_schema::json_validator;

static inline std::string Clans_GetText(const ClanTextBlock& text, const std::string& language) {
    auto it = text.find(language);
    return (it != text.end()) ? it->second : text.at("en-US");
}

void from_json(const json& j, ClanScriptBlock& m) {
    j.get_to(m.script);
    std::string error;

    if (!Scripter::TestScript(m.script, &error)) {
        throw std::runtime_error((std::string("LUA script syntax error: ") + error).c_str());
    }
}

void from_json(const json& j, ClanResource& m) {
    if (j.contains("file")) {
        m.id = INVALID_ID;

        throw std::runtime_error("Not supported yet.");

    } else if (j.contains("id")) {
        std::string id;

        m.file.clear();

        j.at("id").get_to(id);

        const auto resource_id = ResourceManager_GetResourceID(id);

        if (resource_id != INVALID_ID) {
            m.id = resource_id;

        } else {
            throw std::runtime_error((std::string("Invalid media resource: ") + id).c_str());
        }

    } else {
        throw std::runtime_error("No valid media type found.");
    }
}

void from_json(const json& j, ClanMedia& m) {
    j.at("resource").get_to(m.resource);
    j.at("copyright").get_to(m.copyright);
    j.at("license").get_to(m.license);
}

void from_json(const json& j, UnitTradedoffs& m) {
    SDL_memset(&m, 0, sizeof(UnitTradedoffs));

    if (j.contains("Turns to build")) {
        j.at("Turns to build").get_to(m.turns_to_build);
    }

    if (j.contains("Hit points")) {
        j.at("Hit points").get_to(m.hit_points);
    }

    if (j.contains("Armor rating")) {
        j.at("Armor rating").get_to(m.armor_rating);
    }

    if (j.contains("Attack rating")) {
        j.at("Attack rating").get_to(m.attack_rating);
    }

    if (j.contains("Move and fire")) {
        j.at("Move and fire").get_to(m.move_and_fire);
    }

    if (j.contains("Movement points")) {
        j.at("Movement points").get_to(m.movement_points);
    }

    if (j.contains("Attack range")) {
        j.at("Attack range").get_to(m.attack_range);
    }

    if (j.contains("Shots per turn")) {
        j.at("Shots per turn").get_to(m.shots_per_turn);
    }

    if (j.contains("Scan range")) {
        j.at("Scan range").get_to(m.scan_range);
    }

    if (j.contains("Storage capacity")) {
        j.at("Storage capacity").get_to(m.storage_capacity);
    }

    if (j.contains("Ammunition")) {
        j.at("Ammunition").get_to(m.ammunition);
    }

    if (j.contains("Blast radius")) {
        j.at("Blast radius").get_to(m.blast_radius);
    }

    if (j.contains("Experience")) {
        j.at("Experience").get_to(m.experience);
    }
}

void from_json(const json& j, ClanObject& m) {
    j.at("name").get_to(m.name);

    if (m.name.find("en-US") == m.name.end()) {
        throw std::runtime_error("No default language in text block");
    }

    j.at("description").get_to(m.description);

    if (m.description.find("en-US") == m.description.end()) {
        throw std::runtime_error("No default language in text block");
    }

    j.at("logo").get_to(m.logo);

    if (m.logo.resource.file.empty()) {
        if (m.logo.resource.id < CLN0LOGO || m.logo.resource.id > CLN8LOGO) {
            throw std::runtime_error("Invalid logo resource id");
        }

    } else {
        throw std::runtime_error("Not supported yet.");
    }

    if (m.name.find("en-US") == m.name.end() || m.description.find("en-US") == m.description.end()) {
        throw std::runtime_error("No default language in text block");
    }

    j.at("loadout_rules").get_to(m.loadout_rules);

    const auto& advantages = j.at("advantages");

    advantages.at("credits").get_to(m.credits);

    const auto& attributes = advantages.at("attributes");

    for (const auto& [unit_id, unit_data] : attributes.items()) {
        const auto resource_id = ResourceManager_GetResourceID(unit_id);

        if (resource_id >= UNIT_START && resource_id < UNIT_END) {
            m.tradeoffs[resource_id] = unit_data.get<UnitTradedoffs>();

        } else {
            throw std::runtime_error((std::string("Invalid unit type: ") + unit_id).c_str());
        }
    }
}

Clans::Clans()
    : m_language(ResourceManager_GetSystemLocale()),
      m_clans(std::make_unique<std::unordered_map<std::string, ClanObject>>()) {}

Clans::~Clans() {}

Clans::KeyIterator::KeyIterator(MapIterator iter) : m_iter(iter) {}

Clans::KeyIterator::reference Clans::KeyIterator::operator*() const { return m_iter->first; }

Clans::KeyIterator::pointer Clans::KeyIterator::operator->() const { return &m_iter->first; }

Clans::KeyIterator& Clans::KeyIterator::operator++() {
    ++m_iter;
    return *this;
}

Clans::KeyIterator Clans::KeyIterator::operator++(int) {
    KeyIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool operator==(const Clans::KeyIterator& a, const Clans::KeyIterator& b) { return a.m_iter == b.m_iter; }

bool operator!=(const Clans::KeyIterator& a, const Clans::KeyIterator& b) { return a.m_iter != b.m_iter; }

Clans::KeyRange::KeyRange(const std::unordered_map<std::string, ClanObject>* map) : m_map(map) {}

Clans::KeyIterator Clans::KeyRange::begin() const { return KeyIterator(m_map->begin()); }

Clans::KeyIterator Clans::KeyRange::end() const { return KeyIterator(m_map->end()); }

[[nodiscard]] std::string Clans::LoadSchema() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_SCHEC);
    uint8_t* file_base = ResourceManager_ReadResource(SC_SCHEC);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string schema(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return schema;
}

[[nodiscard]] bool Clans::LoadScript(const std::string& script) {
    bool result{false};

    m_clans->clear();

    try {
        validator validator;
        json jschema = json::parse(LoadSchema());
        json jscript = json::parse(script);

        validator.set_root_schema(jschema);
        validator.validate(jscript);

        const auto& clans_object = jscript["clans"];
        for (const auto& [clan_id, clan_data] : clans_object.items()) {
            (*m_clans)[clan_id] = clan_data.get<ClanObject>();
        }

        result = true;

    } catch (const json::parse_error& e) {
        SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());

    } catch (const std::exception& e) {
        SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());
    }

    return result;
}

[[nodiscard]] bool Clans::LoadFile(const std::string& path) {
    std::ifstream fs(path.c_str());
    bool result{false};

    if (fs) {
        std::string contents((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

        result = LoadScript(contents);
    }

    return result;
}

[[nodiscard]] bool Clans::LoadResource() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_CLANS);
    uint8_t* file_base = ResourceManager_ReadResource(SC_CLANS);
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

void Clans::SetLanguage(const std::string& language) { m_language = language; }

[[nodiscard]] bool Clans::HasLoadoutRules(const std::string& clan_id) const {
    if (m_clans) {
        auto it = m_clans->find(clan_id);
        if (it != m_clans->end()) {
            return !it->second.loadout_rules.script.empty();
        }
    }

    return false;
}

[[nodiscard]] std::string Clans::GetLoadoutRules(const std::string& clan_id) const {
    if (m_clans) {
        auto it = m_clans->find(clan_id);
        if (it != m_clans->end()) {
            return it->second.loadout_rules.script;
        }
    }

    return std::string();
}

[[nodiscard]] int32_t Clans::GetCredits(const std::string& clan_id) const {
    if (m_clans) {
        auto it = m_clans->find(clan_id);
        if (it != m_clans->end()) {
            return it->second.credits;
        }
    }

    return 0;
}

[[nodiscard]] std::string Clans::GetName(const std::string& clan_id) const {
    if (m_clans) {
        auto it = m_clans->find(clan_id);
        if (it != m_clans->end()) {
            return Clans_GetText(it->second.name, m_language);
        }
    }

    return std::string();
}

[[nodiscard]] std::string Clans::GetDescription(const std::string& clan_id) const {
    if (m_clans) {
        auto it = m_clans->find(clan_id);
        if (it != m_clans->end()) {
            return Clans_GetText(it->second.description, m_language);
        }
    }

    return std::string();
}

[[nodiscard]] int32_t Clans::GetUnitTradeoff(const std::string& clan_id, const ResourceID resource_id,
                                             const AttributeID attribute) const {
    int32_t result = 0L;

    if (m_clans) {
        auto clan_it = m_clans->find(clan_id);
        if (clan_it != m_clans->end()) {
            auto tradeoff_it = clan_it->second.tradeoffs.find(resource_id);
            if (tradeoff_it != clan_it->second.tradeoffs.end()) {
                const UnitTradedoffs& tradeoffs = tradeoff_it->second;

                switch (attribute) {
                    case ATTRIB_TURNS_TO_BUILD: {
                        result = tradeoffs.turns_to_build;
                    } break;

                    case ATTRIB_HIT_POINTS: {
                        result = tradeoffs.hit_points;
                    } break;

                    case ATTRIB_ARMOR_RATING: {
                        result = tradeoffs.armor_rating;
                    } break;

                    case ATTRIB_ATTACK_RATING: {
                        result = tradeoffs.attack_rating;
                    } break;

                    case ATTRIB_MOVE_AND_FIRE: {
                        result = tradeoffs.move_and_fire;
                    } break;

                    case ATTRIB_MOVEMENT_POINTS: {
                        result = tradeoffs.movement_points;
                    } break;

                    case ATTRIB_ATTACK_RANGE: {
                        result = tradeoffs.attack_range;
                    } break;

                    case ATTRIB_SHOTS_PER_TURN: {
                        result = tradeoffs.shots_per_turn;
                    } break;

                    case ATTRIB_SCAN_RANGE: {
                        result = tradeoffs.scan_range;
                    } break;

                    case ATTRIB_STORAGE_CAPACITY: {
                        result = tradeoffs.storage_capacity;
                    } break;

                    case ATTRIB_AMMUNITION: {
                        result = tradeoffs.ammunition;
                    } break;

                    case ATTRIB_BLAST_RADIUS: {
                        result = tradeoffs.blast_radius;
                    } break;

                    case ATTRIB_EXPERIENCE: {
                        result = tradeoffs.experience;
                    } break;

                    default: {
                        SDL_assert(0);
                    } break;
                }
            }
        }
    }

    return result;
}

[[nodiscard]] Clans::ResourceType Clans::GetLogo(const std::string& clan_id) const {
    ResourceType resource = INVALID_ID;

    if (m_clans) {
        auto clan_it = m_clans->find(clan_id);
        if (clan_it != m_clans->end()) {
            if (clan_it->second.logo.resource.file.empty()) {
                resource = clan_it->second.logo.resource.id;

            } else {
                resource = clan_it->second.logo.resource.file;
            }
        }
    }

    return resource;
}
