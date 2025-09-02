
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

#include "help.hpp"

#include <fstream>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "resource_manager.hpp"

using json = nlohmann::json;
using validator = nlohmann::json_schema::json_validator;

struct Coordinates {
    uint32_t ux;
    uint32_t uy;
    uint32_t lx;
    uint32_t ly;
};

struct HelpEntry {
    Coordinates coordinates;
    std::unordered_map<std::string, std::string> text;
};

struct HelpObject {
    std::unordered_map<std::string, std::vector<HelpEntry>> sections;
};

static inline bool Help_IsInside(const Coordinates& coordinates, const uint32_t x, const uint32_t y) {
    return (x >= coordinates.ux) && (x <= coordinates.lx) && (y >= coordinates.uy) && (y <= coordinates.ly);
}

void from_json(const json& j, Coordinates& c) {
    j.at("ux").get_to(c.ux);
    j.at("uy").get_to(c.uy);
    j.at("lx").get_to(c.lx);
    j.at("ly").get_to(c.ly);
}

void from_json(const json& j, HelpEntry& e) {
    j.at("coordinates").get_to(e.coordinates);
    j.at("text").get_to(e.text);

    if (e.text.find("en-US") == e.text.end()) {
        throw std::runtime_error("No default language in help entry");
    }
}

void from_json(const json& j, HelpObject& h) {
    for (auto& [section, value] : j["help"].items()) {
        std::vector<HelpEntry> entries;

        for (auto& [entry_name, entry_val] : value.items()) {
            entries.push_back(entry_val.get<HelpEntry>());
        }

        h.sections[section] = std::move(entries);
    }
}

Help::Help() : m_language("en-US"), m_help(std::make_unique<HelpObject>()) {}

Help::Help(const std::string& language) : m_language(language), m_help(std::make_unique<HelpObject>()) {}

Help::~Help() {}

void Help::SetLanguage(const std::string& language) { m_language = language; }

std::string Help::LoadSchema() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_SCHEH);
    uint8_t* file_base = ResourceManager_ReadResource(SC_SCHEH);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string schema(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return schema;
}

bool Help::LoadFile(const std::string& path) {
    std::ifstream fs(path.c_str());
    bool result{false};

    if (fs) {
        std::string contents((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

        try {
            validator validator;
            json jschema = json::parse(LoadSchema());
            json jscript = json::parse(contents);

            validator.set_root_schema(jschema);
            validator.validate(jscript);

            *m_help = jscript.get<HelpObject>();

            result = true;

        } catch (const json::parse_error& e) {
            SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());

        } catch (const std::exception& e) {
            SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());
        }
    }

    return result;
}

bool Help::GetEntry(const std::string& section, const int32_t position_x, const int32_t position_y, std::string& text) {
    bool result{false};

    if (m_help) {
        auto it = m_help->sections.find(section);

        if (it != m_help->sections.end()) {
            for (auto& entry : it->second) {
                if (Help_IsInside(entry.coordinates, position_x, position_y)) {
                    auto lang_it = entry.text.find(m_language);

                    if (lang_it != entry.text.end()) {
                        text = lang_it->second;

                    } else {
                        text = entry.text["en-US"];
                    }

                    result = true;

                    break;
                }
            }
        }
    }

    return result;
}
