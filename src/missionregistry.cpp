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

#include <SDL.h>

#include <algorithm>

MissionRegistry::MissionRegistry(const std::filesystem::path& root) {
    for (int32_t id = SCRIPT_S + 2; id < SCRIPT_E; ++id) {
        uint32_t file_size = ResourceManager_GetResourceSize(static_cast<ResourceID>(id));
        uint8_t* file_base = ResourceManager_ReadResource(static_cast<ResourceID>(id));

        if (file_size && file_base) {
            for (size_t i = 0; i < file_size; ++i) {
                file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
            }

            std::string script(reinterpret_cast<const char*>(file_base), file_size);

            auto mission = std::make_unique<Mission>("en-US");

            if (mission->LoadBuffer(script)) {
                m_categories[mission->GetCategory()].push_back(std::move(mission));
            }
        }

        delete[] file_base;
    }

    for (auto& category : m_categories) {
        std::sort(category.begin(), category.end(),
                  [](const std::unique_ptr<Mission>& a, const std::unique_ptr<Mission>& b) {
                      return a->GetTitle() < b->GetTitle();
                  });
    }

    if (std::filesystem::exists(root) && std::filesystem::is_directory(root)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                if (entry.path().filename().string().ends_with(".mission.json")) {
                    auto mission = std::make_unique<Mission>("en-US");

                    if (mission->LoadFile(entry.path().string())) {
                        m_categories[mission->GetCategory()].push_back(std::move(mission));
                    }
                }
            }
        }

    } else {
        SDL_Log("Invalid folder path to mission descriptors: %s", root.string().c_str());
    }
}

MissionRegistry::~MissionRegistry() {}

const std::vector<std::unique_ptr<Mission>>& MissionRegistry::GetMissions(const MissionCategory category) noexcept {
    return m_categories[category];
}
