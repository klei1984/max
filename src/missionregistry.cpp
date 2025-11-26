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

#include "missionregistry.hpp"

#include <SDL3/SDL.h>

#include <algorithm>

MissionRegistry::MissionRegistry(const std::filesystem::path& root) {
    std::array<size_t, MISSION_CATEGORY_COUNT> resource_end_indexes{};

    for (uint32_t id = SC_MIS_S; id < SC_MIS_E; ++id) {
        uint32_t file_size = ResourceManager_GetResourceSize(static_cast<ResourceID>(id));
        uint8_t* file_base = ResourceManager_ReadResource(static_cast<ResourceID>(id));

        if (file_size && file_base) {
            for (size_t i = 0; i < file_size; ++i) {
                file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
            }

            std::string script(reinterpret_cast<const char*>(file_base), file_size);

            auto mission = std::make_shared<Mission>();

            if (mission && mission->LoadBuffer(script)) {
                m_categories[mission->GetCategory()].push_back(std::move(mission));
            }
        }

        delete[] file_base;
    }

    for (uint32_t i = 0; i < MISSION_CATEGORY_COUNT; ++i) {
        resource_end_indexes[i] = m_categories[i].size();
    }

    if (std::filesystem::exists(root) && std::filesystem::is_directory(root)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                if (entry.path().filename().string().ends_with(".mission.json")) {
                    auto mission = std::make_shared<Mission>();

                    if (mission->LoadFile(entry.path().string())) {
                        m_categories[mission->GetCategory()].push_back(std::move(mission));
                    }
                }
            }
        }

    } else {
        SDL_Log("\nInvalid folder path to mission descriptors: %s\n", root.string().c_str());
    }

    for (uint32_t i = 0; i < MISSION_CATEGORY_COUNT; ++i) {
        auto& category = m_categories[i];
        if (category.size() > resource_end_indexes[i]) {
            std::sort(category.begin() + resource_end_indexes[i], category.end(),
                      [](const std::shared_ptr<Mission>& a, const std::shared_ptr<Mission>& b) {
                          return a->GetTitle() < b->GetTitle();
                      });
        }
    }
}

MissionRegistry::~MissionRegistry() {}

const std::vector<std::shared_ptr<Mission>>& MissionRegistry::GetMissions(const MissionCategory category) noexcept {
    return m_categories[category];
}

std::shared_ptr<Mission> MissionRegistry::GetMission(const MissionCategory category, const std::string hash) noexcept {
    for (const auto& mission : GetMissions(category)) {
        for (const auto& mission_hash : mission->GetMissionHashes()) {
            if (mission_hash == hash) {
                return mission;
            }
        }
    }

    return {};
}
