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

#include "attributes.hpp"

#include <SDL.h>

#include <fstream>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "resource_manager.hpp"

using json = nlohmann::json;
using validator = nlohmann::json_schema::json_validator;

void from_json(const json& j, UnitAttributes& a) {
    j.at("Turns to build").get_to(a.turns_to_build);
    j.at("Hit points").get_to(a.hit_points);
    j.at("Armor rating").get_to(a.armor_rating);
    j.at("Attack rating").get_to(a.attack_rating);
    j.at("Move and fire").get_to(a.move_and_fire);
    j.at("Movement points").get_to(a.movement_points);
    j.at("Attack range").get_to(a.attack_range);
    j.at("Shots per turn").get_to(a.shots_per_turn);
    j.at("Scan range").get_to(a.scan_range);
    j.at("Storage capacity").get_to(a.storage_capacity);
    j.at("Ammunition").get_to(a.ammunition);
    j.at("Blast radius").get_to(a.blast_radius);
}

Attributes::Attributes() : m_attributes(std::make_unique<std::unordered_map<std::string, UnitAttributes>>()) {}

Attributes::~Attributes() {}

[[nodiscard]] std::string Attributes::LoadSchema() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_SCHEA);
    uint8_t* file_base = ResourceManager_ReadResource(SC_SCHEA);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string schema(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return schema;
}

[[nodiscard]] bool Attributes::LoadScript(const std::string& script) {
    bool result{false};

    m_attributes->clear();

    try {
        validator validator;
        json jschema = json::parse(LoadSchema());
        json jscript = json::parse(script);

        validator.set_root_schema(jschema);
        validator.validate(jscript);

        const auto& attributes_object = jscript["attributes"];
        for (const auto& [unit_name, unit_data] : attributes_object.items()) {
            (*m_attributes)[unit_name] = unit_data.get<UnitAttributes>();
        }

        result = true;

    } catch (const json::parse_error& e) {
        SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());

    } catch (const std::exception& e) {
        SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());
    }

    return result;
}

[[nodiscard]] bool Attributes::LoadFile(const std::string& path) {
    std::ifstream fs(path.c_str());
    bool result{false};

    if (fs) {
        std::string contents((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

        result = LoadScript(contents);
    }

    return result;
}

[[nodiscard]] bool Attributes::LoadResource() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_ATTRI);
    uint8_t* file_base = ResourceManager_ReadResource(SC_ATTRI);
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

[[nodiscard]] bool Attributes::GetUnitAttributes(const std::string unit_id, UnitAttributes* const attributes) {
    if (m_attributes) {
        auto it = m_attributes->find(unit_id);

        if (it != m_attributes->end()) {
            *attributes = it->second;

            return true;
        }
    }

    return false;
}
