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

#include "language.hpp"

#include <SDL.h>

#include <fstream>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "resource_manager.hpp"

using json = nlohmann::json;
using validator = nlohmann::json_schema::json_validator;

struct LanguageObject {
    std::unordered_map<uint32_t, std::unordered_map<std::string, std::string>> entries;
};

void from_json(const json& j, LanguageObject& l) {
    for (auto& [uuid_string, value] : j["language"].items()) {
        std::unordered_map<std::string, std::string> translations;

        for (auto& [lang_tag, text] : value.items()) {
            translations[lang_tag] = text.get<std::string>();
        }

        if (translations.find("en-US") == translations.end()) {
            throw std::runtime_error("No default language in language entry: " + uuid_string);
        }

        uint32_t uuid = static_cast<uint32_t>(std::stoul(uuid_string, nullptr, 16));

        if (l.entries.find(uuid) != l.entries.end()) {
            throw std::runtime_error("Duplicate key in language entries: " + uuid_string);
        }

        l.entries[uuid] = std::move(translations);
    }
}

Language::Language() : m_languageobject(std::make_unique<LanguageObject>()) {}

Language::Language(const std::string& language)
    : m_language(language), m_languageobject(std::make_unique<LanguageObject>()) {}

Language::~Language() {}

void Language::SetLanguage(const std::string& language) { m_language = language; }

std::string Language::LoadSchema() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_SCHEL);
    uint8_t* file_base = ResourceManager_ReadResource(SC_SCHEL);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string schema(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return schema;
}

bool Language::LoadFile(const std::string& path) {
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

            *m_languageobject = jscript.get<LanguageObject>();

            result = true;

        } catch (const json::parse_error& e) {
            SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());

        } catch (const std::exception& e) {
            SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());
        }
    }

    return result;
}

const std::string& Language::GetEntry(const uint32_t key) {
    if (key == 0xffff) {
        return m_empty;
    }

    if (m_languageobject) {
        auto it = m_languageobject->entries.find(key);

        if (it != m_languageobject->entries.end()) {
            const auto& translations = it->second;
            auto lang_it = translations.find(m_language);

            if (lang_it != translations.end() && !lang_it->second.empty()) {
                return lang_it->second;

            } else {
                auto default_it = translations.find("en-US");

                if (default_it != translations.end()) {
                    return default_it->second;
                }
            }
        }
    }

    return m_error;
}
