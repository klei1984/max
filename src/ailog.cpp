/* Copyright (c) 2023 M.A.X. Port Team
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

#include "ailog.hpp"

#include <SDL3/SDL.h>

#include <cstdarg>

constexpr int32_t AILOG_FILE_LIMIT_DEFAULT = UINT16_MAX;

std::ofstream AiLog::AiLog_File;
SDL_Mutex* AiLog::AiLog_Mutex = nullptr;
int32_t AiLog::AiLog_SectionCount = 0;
int32_t AiLog::AiLog_EntryCount = 0;
int32_t AiLog::AiLog_EntryLimit = AILOG_FILE_LIMIT_DEFAULT;
std::optional<std::regex> AiLog::AiLog_Filter;

void AiLog::AiLog_InitMutex() {
    if (!AiLog_Mutex) {
        AiLog_Mutex = ResourceManager_CreateMutex();

        if (!AiLog_Mutex) {
            ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
        }
    }
}

bool AiLog::ShouldFilter() const noexcept {
    if (!AiLog_Filter.has_value()) {
        return false;
    }

    try {
        std::string subject = std::format("{}|{}", m_file, m_function);

        bool filter_matches = std::regex_search(subject, AiLog_Filter.value());

        return !filter_matches;

    } catch (...) {
        return false;
    }
}

void AiLog::WriteLog(std::string_view message) {
    AiLog_File << std::format("\n{:3d}: ", AiLog_SectionCount + 1);

    if (AiLog_SectionCount > 0) {
        AiLog_File << std::format("{:{}}", "", AiLog_SectionCount);
    }

    AiLog_File << message;
    AiLog_File.flush();

    ++AiLog_EntryCount;
}

void AiLog::NoLockLog(std::string_view message) { WriteLog(message); }

AiLog::~AiLog() {
    AiLog_InitMutex();

    ResourceManager_MutexLock lock(AiLog_Mutex);

    --AiLog_SectionCount;

    if (AiLog_File.is_open() && !m_is_filtered) {
        auto elapsed_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_time_stamp)
                .count();

        constexpr int64_t THRESHOLD_MS = 1000;

        if (elapsed_time >= THRESHOLD_MS) {
            NoLockLog(std::format("log section complete, {} msecs elapsed", elapsed_time));

        } else {
            NoLockLog("log section complete");
        }

        if (AiLog_EntryCount > AiLog_EntryLimit) {
            auto filepath = (ResourceManager_FilePathGamePref / "ai_log.txt").lexically_normal();

            AiLog_File.close();
            AiLog_File.open(filepath.string().c_str(), std::ios::out);
            AiLog_EntryCount = 0;
        }
    }
}

void AiLog_Open() {
    AiLog::AiLog_InitMutex();

    ResourceManager_MutexLock lock(AiLog::AiLog_Mutex);

    if (!AiLog::AiLog_File.is_open()) {
        auto filepath = (ResourceManager_FilePathGamePref / "ai_log.txt").lexically_normal();

        AiLog::AiLog_File.open(filepath.string().c_str(), std::ios::out);
        AiLog::AiLog_EntryCount = 0;

        const char* filter_env = SDL_getenv("MAX_AILOG_FILTER");

        if (filter_env && filter_env[0] != '\0') {
            try {
                AiLog::AiLog_Filter = std::regex(filter_env, std::regex::icase);

            } catch (const std::regex_error&) {
                AiLog::AiLog_Filter = std::nullopt;
            }

        } else {
            AiLog::AiLog_Filter = std::nullopt;
        }

        const char* limit_env = SDL_getenv("MAX_AILOG_ENTRY_LIMIT");

        if (limit_env && limit_env[0] != '\0') {
            try {
                int32_t limit = std::stoi(limit_env);

                if (limit > 0) {
                    AiLog::AiLog_EntryLimit = limit;

                } else {
                    AiLog::AiLog_EntryLimit = AILOG_FILE_LIMIT_DEFAULT;
                }

            } catch (...) {
                AiLog::AiLog_EntryLimit = AILOG_FILE_LIMIT_DEFAULT;
            }

        } else {
            AiLog::AiLog_EntryLimit = AILOG_FILE_LIMIT_DEFAULT;
        }
    }
}

void AiLog_Close() {
    AiLog::AiLog_InitMutex();

    ResourceManager_MutexLock lock(AiLog::AiLog_Mutex);

    AiLog::AiLog_File.close();
    AiLog::AiLog_Filter = std::nullopt;
}

[[nodiscard]] bool AiLog_IsEnabled() noexcept {
    AiLog::AiLog_InitMutex();

    ResourceManager_MutexLock lock(AiLog::AiLog_Mutex);

    auto result = AiLog::AiLog_File.is_open();

    return result;
}
