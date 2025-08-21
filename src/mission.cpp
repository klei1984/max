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

#include "mission.hpp"

#include <SDL.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "nlohmann/json-schema.hpp"
#include "nlohmann/json.hpp"
#include "scripter.hpp"
#include "sha2.h"

using json = nlohmann::json;
using validator = nlohmann::json_schema::json_validator;

struct Resource {
    std::vector<std::filesystem::path> file;
    std::vector<ResourceID> id;
};

struct Media {
    Resource resource;
    std::string copyright;
    std::string license;
};

struct VideoLang {
    std::unordered_map<std::string, Media> languages;
};

struct TextBlock {
    std::unordered_map<std::string, std::string> text;
};

struct ScriptBlock {
    std::string script;
};

struct StoryBlock {
    TextBlock text;
    std::optional<Media> background;
    std::optional<std::unordered_map<std::string, Media>> video;
    std::optional<Media> music;
};

struct GameEventMessage {
    std::unordered_map<std::string, std::string> text;
    std::unordered_map<std::string, Media> voice;
};

struct GameEvent {
    size_t turn;
    Resource resource;
    struct {
        std::string type;
        std::string id;
    } target;
    GameEventMessage message;
    bool log;
};

struct MusicTrack {
    Media music;
};

struct GameMusic {
    bool shuffle;
    std::vector<MusicTrack> tracks;
};

struct MissionObject {
    std::string script;
    std::string author;
    std::string copyright;
    std::string license;
    MissionCategory category;
    std::filesystem::path mission;
    std::vector<std::string> hash;
    TextBlock title;
    TextBlock description;
    std::optional<StoryBlock> intro;
    std::optional<StoryBlock> victory;
    std::optional<StoryBlock> defeat;
    std::optional<ScriptBlock> winloss_conditions;
    std::optional<ScriptBlock> game_rules;
    std::optional<std::vector<GameEvent>> game_events;
    std::optional<GameMusic> game_music;
};

static inline std::string Mission_GetText(const TextBlock& text, const std::string& language) {
    auto it = text.text.find(language);
    return (it != text.text.end()) ? it->second : text.text.at("en-US");
}

static inline Mission::ResourceType Mission_GetOptionalMedia(const std::optional<Media>& media) {
    Mission::ResourceType resource;

    if (media.has_value()) {
        if (media.value().resource.file.empty()) {
            resource = media.value().resource.id;

        } else {
            resource = media.value().resource.file;
        }

    } else {
        resource = std::vector<ResourceID>{INVALID_ID};
    }

    return resource;
}

static inline Mission::ResourceType Mission_GetOptionalMedia(
    const std::optional<std::unordered_map<std::string, Media>>& media, const std::string& language) {
    Mission::ResourceType resource;

    if (media.has_value()) {
        auto it = media.value().find(language);
        auto& m = (it != media.value().end()) ? it->second : media.value().at("en-US");

        if (m.resource.file.empty()) {
            resource = m.resource.id;

        } else {
            resource = m.resource.file;
        }

    } else {
        resource = std::vector<ResourceID>{INVALID_ID};
    }

    return resource;
}

static inline bool Mission_GetOptionalStory(Mission::Story& story, const std::optional<StoryBlock>& story_block,
                                            const std::string& language) {
    if (!story_block.has_value()) {
        return false;
    }

    story.text = Mission_GetText(story_block.value().text, language);

    story.background = Mission_GetOptionalMedia(story_block.value().background);
    story.video = Mission_GetOptionalMedia(story_block.value().video, language);
    story.music = Mission_GetOptionalMedia(story_block.value().music);

    return true;
}

static inline void Mission_ProcessCategory(const json& j, MissionObject& m) {
    const std::unordered_map<std::string, MissionCategory> map = {
        {"Custom", MISSION_CATEGORY_CUSTOM},
        {"Training", MISSION_CATEGORY_TRAINING},
        {"Campaign", MISSION_CATEGORY_CAMPAIGN},
        {"Hot-seat", MISSION_CATEGORY_HOT_SEAT},
        {"Multiplayer", MISSION_CATEGORY_MULTI},
        {"Demo", MISSION_CATEGORY_DEMO},
        {"Single Player Scenario", MISSION_CATEGORY_SCENARIO},
        {"Multiplayer Scenario", MISSION_CATEGORY_MULTI_PLAYER_SCENARIO}};

    std::string category;

    j.at("category").get_to(category);

    auto it = map.find(category);

    if (it == map.end()) {
        throw std::runtime_error((std::string("Invalid mission category: ") + category).c_str());
    }

    m.category = it->second;
}

[[nodiscard]] std::string Mission::Mission_Sha256(const std::filesystem::path& path) {
    constexpr size_t BLOCK_SIZE = 4096;

    uint8_t buffer[BLOCK_SIZE];
    uint8_t digest[SHA256_DIGEST_SIZE];
    sha256_ctx ctx;

    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open mission file");
    }

    sha256_init(&ctx);

    while (file) {
        file.read(reinterpret_cast<char*>(buffer), BLOCK_SIZE);

        std::streamsize bytes_read = file.gcount();

        if (bytes_read > 0) {
            sha256_update(&ctx, buffer, static_cast<size_t>(bytes_read));
        }
    }

    sha256_final(&ctx, digest);

    std::ostringstream hex;

    hex << std::hex << std::setfill('0');

    for (size_t i = 0; i < SHA256_DIGEST_SIZE; ++i) {
        hex << std::setw(2) << static_cast<int>(digest[i]);
    }

    return hex.str();
}

static inline bool Mission_TestMissionFile(const std::filesystem::path& path, const std::vector<std::string>& hashes) {
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path) &&
           std::find(hashes.begin(), hashes.end(), Mission::Mission_Sha256(path)) != hashes.end();
}

static inline bool Mission_TestMediaFile(const std::filesystem::path& path) {
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

static inline void Mission_ProcessMission(const json& j, MissionObject& m) {
    std::string file;
    std::vector<std::string> hash;
    std::filesystem::path path;

    if (m.category == MISSION_CATEGORY_CUSTOM) {
        m.hash.push_back("MISSION_CATEGORY_CUSTOM");

        return;

    } else if (m.category == MISSION_CATEGORY_HOT_SEAT) {
        m.hash.push_back("MISSION_CATEGORY_HOT_SEAT");

        return;

    } else if (m.category == MISSION_CATEGORY_MULTI) {
        m.hash.push_back("MISSION_CATEGORY_MULTI");

        return;
    }

    j.at("mission").get_to(file);
    j.at("hash").get_to(hash);

    path = ResourceManager_FilePathGameData / file;

    if (Mission_TestMissionFile(path, hash)) {
        m.mission = path;
        m.hash = hash;

    } else {
        path = ResourceManager_FilePathGameBase / file;

        if (Mission_TestMissionFile(path, hash)) {
            m.mission = path;
            m.hash = hash;

        } else {
            path = ResourceManager_FilePathGamePref / file;

            if (Mission_TestMissionFile(path, hash)) {
                m.mission = path;
                m.hash = hash;

            } else {
                throw std::runtime_error((std::string("Invalid mission file: ") + file).c_str());
            }
        }
    }
}

void from_json(const json& j, TextBlock& m) {
    j.at("text").get_to(m.text);

    auto it = m.text.find("en-US");

    if (it == m.text.end()) {
        throw std::runtime_error("No default language in text block");
    }
}

void from_json(const json& j, ScriptBlock& m) {
    j.get_to(m.script);
    std::string error;

    if (!Scripter::TestScript(m.script, &error)) {
        throw std::runtime_error((std::string("LUA script syntax error: ") + error).c_str());
    }
}

void from_json(const json& j, Resource& m) {
    if (j.contains("file")) {
        std::vector<std::string> files;
        std::filesystem::path path;

        m.id = std::vector<ResourceID>{INVALID_ID};

        j.at("file").get_to(files);

        for (const auto& file : files) {
            path = ResourceManager_FilePathGameData / file;

            if (Mission_TestMediaFile(path)) {
                m.file.push_back(path);

            } else {
                path = ResourceManager_FilePathGameBase / file;

                if (Mission_TestMediaFile(path)) {
                    m.file.push_back(path);

                } else {
                    path = ResourceManager_FilePathGamePref / file;

                    if (Mission_TestMediaFile(path)) {
                        m.file.push_back(path);

                    } else {
                        throw std::runtime_error((std::string("Invalid media files: ") + file).c_str());
                    }
                }
            }
        }

        if (m.file.size() == 0) {
            throw std::runtime_error("No valid media files found.");
        }

    } else if (j.contains("id")) {
        std::vector<std::string> ids;

        j.at("id").get_to(ids);

        for (const auto& id : ids) {
            const auto resource_id = ResourceManager_GetResourceID(id);

            if (resource_id != INVALID_ID) {
                m.id.push_back(resource_id);

            } else {
                throw std::runtime_error((std::string("Invalid media resource: ") + id).c_str());
            }
        }

        if (m.id.size() == 0) {
            throw std::runtime_error("No valid media resource found.");
        }
    }
}

void from_json(const json& j, Media& m) {
    m.resource = j.at("resource").get<Resource>();
    j.at("copyright").get_to(m.copyright);
    j.at("license").get_to(m.license);
}

void from_json(const json& j, StoryBlock& m) {
    m.text = j.get<TextBlock>();

    if (j.contains("background")) {
        m.background = j.at("background").get<Media>();
    }

    if (j.contains("video")) {
        m.video = j.at("video").get<std::unordered_map<std::string, Media>>();
    }

    if (j.contains("music")) {
        m.music = j.at("music").get<Media>();
    }
}

void from_json(const json& j, MissionObject& m) {
    j.at("author").get_to(m.author);
    j.at("copyright").get_to(m.copyright);
    j.at("license").get_to(m.license);

    Mission_ProcessCategory(j, m);
    Mission_ProcessMission(j, m);

    m.title = j.at("title").get<TextBlock>();
    m.description = j.at("description").get<TextBlock>();

    if (j.contains("intro")) {
        m.intro = j.at("intro").get<StoryBlock>();
    }

    if (j.contains("victory")) {
        m.victory = j.at("victory").get<StoryBlock>();
    }

    if (j.contains("defeat")) {
        m.defeat = j.at("defeat").get<StoryBlock>();
    }

    if (j.contains("winloss_conditions")) {
        m.winloss_conditions = j.at("winloss_conditions").get<ScriptBlock>();
    }

    if (j.contains("game_rules")) {
        m.game_rules = j.at("game_rules").get<ScriptBlock>();
    }
}

Mission::Mission() : m_language("en-US"), m_mission(std::make_unique<MissionObject>()) {}

Mission::Mission(const std::string& language) : m_language(language), m_mission(std::make_unique<MissionObject>()) {}

Mission::~Mission() {}

void Mission::SetLanguage(const std::string& tag) { m_language = tag; }

std::string Mission::GetTitle() const { return Mission_GetText(m_mission->title, m_language); }

MissionCategory Mission::GetCategory() const { return m_mission->category; }

std::string Mission::GetDescription() const { return Mission_GetText(m_mission->description, m_language); }

std::filesystem::path Mission::GetMission() const { return m_mission->mission; }

bool Mission::SetMission(std::filesystem::path path) {
    bool result;

    if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
        m_mission->mission = path;
        result = true;

    } else {
        result = false;
    }

    return result;
}

bool Mission::HasIntroInfo() const { return m_mission->intro.has_value(); }

bool Mission::GetIntroInfo(Story& story) const { return Mission_GetOptionalStory(story, m_mission->intro, m_language); }

bool Mission::HasVictoryInfo() const { return m_mission->victory.has_value(); }

bool Mission::GetVictoryInfo(Story& story) const {
    return Mission_GetOptionalStory(story, m_mission->victory, m_language);
}

bool Mission::HasDefeatInfo() const { return m_mission->defeat.has_value(); }

bool Mission::GetDefeatInfo(Story& story) const {
    return Mission_GetOptionalStory(story, m_mission->defeat, m_language);
}

bool Mission::HasWinLossConditions() const { return m_mission->winloss_conditions.has_value(); }

std::string Mission::GetWinLossConditions() const {
    return (m_mission->winloss_conditions.has_value()) ? m_mission->winloss_conditions.value().script : "return false";
}

bool Mission::HasGameRules() const { return m_mission->game_rules.has_value(); }

std::string Mission::GetScript() const { return m_mission->script; }

std::unique_ptr<std::vector<uint8_t>> Mission::GetBinaryScript() const {
    auto buffer = std::make_unique<std::vector<uint8_t>>(m_mission->script.size());

    if (buffer) {
        for (size_t i = 0; i < buffer->size(); ++i) {
            (*buffer)[i] =
                m_mission->script[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
        }
    }

    return buffer;
}

std::string Mission::GetGameRules() const {
    return (m_mission->game_rules.has_value()) ? m_mission->game_rules.value().script : "return false";
}

std::string Mission::LoadSchema() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_SCHEM);
    uint8_t* file_base = ResourceManager_ReadResource(SC_SCHEM);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string script(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return script;
}

bool Mission::LoadBuffer(const std::string& script) {
    bool result{false};

    try {
        validator validator;
        json jschema = json::parse(LoadSchema());
        json jscript = json::parse(script);

        validator.set_root_schema(jschema);
        validator.validate(jscript);

        *m_mission = jscript.get<MissionObject>();

        m_mission->script = jscript.dump();

        result = true;

    } catch (const json::parse_error& e) {
        SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());

    } catch (const std::exception& e) {
        SDL_Log("\n%s\n", (std::string("JSON parse error: ") + e.what()).c_str());
    }

    return result;
}

[[nodiscard]] bool Mission::LoadBinaryBuffer(const std::vector<uint8_t>& script) {
    auto decoded_script = script;

    for (size_t i = 0; i < decoded_script.size(); ++i) {
        decoded_script[i] = decoded_script[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string jscript(decoded_script.begin(), decoded_script.end());

    return LoadBuffer(jscript);
}

bool Mission::LoadFile(const std::string& path) {
    std::ifstream fs(path.c_str());
    bool result{false};

    if (fs) {
        std::string contents((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

        result = LoadBuffer(contents);
    }

    return result;
}

const std::vector<std::string> Mission::GetMissionHashes() const { return m_mission->hash; }

bool Mission::IdentifyMission(const std::string hash) const {
    return std::find(m_mission->hash.begin(), m_mission->hash.end(), hash) != m_mission->hash.end();
}
