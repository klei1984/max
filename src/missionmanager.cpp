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

#include "missionmanager.hpp"

#include "maxregistryhandler.hpp"
#include "resource_manager.hpp"

std::unique_ptr<MissionRegistry> MissionManager::m_missionregistry;

MissionManager::MissionManager() : m_language(ResourceManager_GetSystemLocale()) {
    if (!m_missionregistry) {
        m_missionregistry = std::make_unique<MissionRegistry>(ResourceManager_FilePathGamePref);
    }
}

MissionManager::~MissionManager() {}

void MissionManager::SetupMission() {
    if (m_mission) {
        MaxRegistryHandler::Init();
        m_mission->SetLanguage(m_language);
        m_gameruleshandler = std::make_unique<GameRulesHandler>();
        if (m_gameruleshandler) {
            if (!m_gameruleshandler->LoadScript(*m_mission)) {
                m_gameruleshandler.reset();
                m_mission.reset();
            }
        } else {
            m_mission.reset();
        }
        m_winlosshandler = std::make_unique<WinLossHandler>();
        if (m_winlosshandler) {
            if (!m_winlosshandler->LoadScript(*m_mission)) {
                m_gameruleshandler.reset();
                m_winlosshandler.reset();
                m_mission.reset();
            }
        } else {
            m_gameruleshandler.reset();
            m_mission.reset();
        }
    }
}

bool MissionManager::LoadMission(const MissionCategory category, const std::string hash) {
    m_mission = FindMission(category, hash);

    SetupMission();

    return m_mission ? true : false;
}

bool MissionManager::LoadMission(const MissionCategory category, const uint32_t index) {
    const auto& missions = m_missionregistry->GetMissions(category);
    bool result;

    if (missions.size() > index) {
        result = LoadMission(missions[index]);

    } else {
        result = false;
    }

    return result;
}

bool MissionManager::LoadMission(std::shared_ptr<Mission> mission) {
    m_mission = mission;

    SetupMission();

    return m_mission ? true : false;
}

const std::unique_ptr<GameRulesHandler>& MissionManager::GetGameRulesHandler() const { return m_gameruleshandler; }

const std::unique_ptr<WinLossHandler>& MissionManager::GetWinLossHandler() const { return m_winlosshandler; }

const std::shared_ptr<Mission> MissionManager::GetMission() const { return m_mission; }

const std::shared_ptr<Mission> MissionManager::FindMission(const MissionCategory category,
                                                           const std::string hash) const {
    return m_missionregistry->GetMission(category, hash);
}

void MissionManager::ResetMission() {
    m_mission.reset();
    MaxRegistryHandler::Reset();
}

const std::vector<std::shared_ptr<Mission>>& MissionManager::GetMissions(
    const MissionCategory category) const noexcept {
    return m_missionregistry->GetMissions(category);
}

int32_t MissionManager::GetMissionIndex(const MissionCategory category, const std::vector<std::string>& hashes) const {
    auto result = MissionManager::InvalidID;

    for (const std::string& hash : hashes) {
        const auto mission = FindMission(category, hash);

        if (mission) {
            result = GetMissionIndex(mission);
            break;
        }
    }

    return result;
}

int32_t MissionManager::GetMissionIndex(const MissionCategory category, const std::string hash) const {
    const auto mission = FindMission(category, hash);

    return GetMissionIndex(mission);
}

int32_t MissionManager::GetMissionIndex(const std::shared_ptr<Mission> mission) const {
    auto result = MissionManager::InvalidID;

    if (mission) {
        const auto mission_category = mission->GetCategory();
        const auto& missions = GetMissions(mission_category);

        for (auto it = missions.begin(); it != missions.end(); ++it) {
            if (it->get() == mission.get()) {
                result = std::distance(missions.begin(), it);
                break;
            }
        }
    }

    return result;
}

void MissionManager::HandleMissionStateChanges() { MaxRegistryHandler::Update(); }

void MissionManager::SetLanguage(const std::string& language) {
    m_language = language;

    if (m_mission) {
        m_mission->SetLanguage(m_language);
    }

    for (uint32_t i = MISSION_CATEGORY_CUSTOM; i < MISSION_CATEGORY_COUNT; ++i) {
        const auto& missions = GetMissions(static_cast<MissionCategory>(i));

        for (auto it = missions.begin(); it != missions.end(); ++it) {
            it->get()->SetLanguage(m_language);
        }
    }
}
