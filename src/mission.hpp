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

#ifndef MISSION_HPP
#define MISSION_HPP

#include <variant>

#include "resource_manager.hpp"
struct MissionObject;

enum MissionCategory {
    MISSION_CATEGORY_TRAINING,
    MISSION_CATEGORY_CAMPAIGN,
    MISSION_CATEGORY_DEMO,
    MISSION_CATEGORY_SCENARIO,
    MISSION_CATEGORY_MULTI_PLAYER_SCENARIO,
    MISSION_CATEGORY_COUNT
};

class Mission {
    std::string m_language;
    std::unique_ptr<MissionObject> m_mission;

    [[nodiscard]] std::string LoadSchema();

public:
    using ResourceType = std::variant<ResourceID, std::filesystem::path>;

    struct Story {
        std::string text;

        ResourceType background;
        ResourceType video;
        ResourceType music;
    };

    Mission();
    Mission(const std::string& language);
    ~Mission();

    [[nodiscard]] bool LoadFile(const std::string& path);
    [[nodiscard]] bool LoadBuffer(const std::string& script);
    void Setlanguage(const std::string& language);

    [[nodiscard]] std::string GetTitle() const;
    [[nodiscard]] MissionCategory GetCategory() const;
    [[nodiscard]] std::string GetDescription() const;
    [[nodiscard]] std::filesystem::path GetMission() const;

    [[nodiscard]] bool HasIntroInfo() const;
    [[nodiscard]] bool GetIntroInfo(Story& story) const;

    [[nodiscard]] bool HasVictoryInfo() const;
    [[nodiscard]] bool GetVictoryInfo(Story& story) const;

    [[nodiscard]] bool HasDefeatInfo() const;
    [[nodiscard]] bool GetDefeatInfo(Story& story) const;

    [[nodiscard]] bool HasVictoryConditions() const;
    [[nodiscard]] std::string GetVictoryConditions() const;

    [[nodiscard]] bool HasDefeatConditions() const;
    [[nodiscard]] std::string GetDefeatConditions() const;

    [[nodiscard]] bool HasGameRules() const;
    [[nodiscard]] std::string GetGameRules() const;
};

#endif /* MISSION_HPP */
