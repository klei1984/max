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

#ifndef MISSIONMANAGER_HPP
#define MISSIONMANAGER_HPP

#include <memory>
#include <string>

#include "gameruleshandler.hpp"
#include "missionregistry.hpp"
#include "winlosshandler.hpp"

class MissionManager {
    static std::unique_ptr<MissionRegistry> m_missionregistry;

    std::string m_language{"en-US"};
    std::shared_ptr<Mission> m_mission;
    std::unique_ptr<GameRulesHandler> m_gameruleshandler;
    std::unique_ptr<WinLossHandler> m_winlosshandler;

public:
    MissionManager();
    virtual ~MissionManager();

    bool LoadMission(const MissionCategory category, const std::string hash);
    bool LoadMission(const MissionCategory category, const uint32_t index);

    const std::unique_ptr<GameRulesHandler>& GetGameRulesHandler() const;
    const std::unique_ptr<WinLossHandler>& GetWinLossHandler() const;
    const std::shared_ptr<Mission> GetMission() const;
    void HandleMissionStateChanges();
    void ResetMission();
    const std::vector<std::shared_ptr<Mission>>& GetMissions(const MissionCategory category) noexcept;
};

#endif /* MISSIONMANAGER_HPP */
