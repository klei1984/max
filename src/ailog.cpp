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

#if !defined(NDEBUG)

#include <SDL3/SDL.h>

#include <cstdarg>
#include <filesystem>
#include <mutex>
#include <system_error>

#include "order_log.hpp"
#include "settings.hpp"

std::ofstream AiLog::AiLog_File;
SDL_Mutex* AiLog::AiLog_Mutex = nullptr;
int32_t AiLog::AiLog_SectionCount = 0;
int32_t AiLog::AiLog_EntryCount = 0;
int32_t AiLog::AiLog_EntryLimit = 0;
std::optional<std::regex> AiLog::AiLog_Filter;

static uint32_t AiLog_Tick;

void AiLog::AiLog_InitMutex() {
    /* Worker threads log too, and an unsynchronized lazy create lets two of them build one
     * mutex each and then lock different objects, leaving the first concurrent use unguarded.
     */
    static std::once_flag mutex_once;

    std::call_once(mutex_once, []() {
        AiLog_Mutex = ResourceManager_CreateMutex(&AiLog_Mutex);

        if (!AiLog_Mutex) {
            ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
        }
    });
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
    /* the order log tick in progress matches the entries with the records of order_log.txt */
    const uint32_t tick = OrderLog_GetTick();

    if (tick != AiLog_Tick) {
        AiLog_File << std::format("\n# tick {}", tick);
        AiLog_Tick = tick;
    }

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

        /* the depth of the next entry already shows where a section ended, only a slow one gets a line */
        if (elapsed_time >= THRESHOLD_MS) {
            NoLockLog(std::format("log section complete, {} msecs elapsed", elapsed_time));
        }

        /* a limit of 0 keeps the whole log in one file */
        if (AiLog_EntryLimit > 0 && AiLog_EntryCount > AiLog_EntryLimit) {
            AiLog_File.close();

            AiLog_OpenNewFile();
        }
    }
}

void AiLog::AiLog_OpenNewFile() {
    auto filepath = (ResourceManager_FilePathGamePref / "ai_log.txt").lexically_normal();
    auto rolled = (ResourceManager_FilePathGamePref / "ai_log.prev.txt").lexically_normal();
    std::error_code error;

    /* Roll the last file aside rather than reopening it truncated, whether it filled up or an earlier run or
     * session wrote it: the entries leading up to a crash or a restart are the ones worth keeping.
     */
    std::filesystem::rename(filepath, rolled, error);

    AiLog_File.open(filepath.string().c_str(), std::ios::out);
    AiLog_EntryCount = 0;
    AiLog_Tick = 0;
}

void AiLog_Open() {
    AiLog::AiLog_InitMutex();

    ResourceManager_MutexLock lock(AiLog::AiLog_Mutex);

    if (!AiLog::AiLog_File.is_open()) {
        AiLog::AiLog_OpenNewFile();
        AiLog::AiLog_EntryLimit = ResourceManager_GetSettings()->GetNumericValue("ai_log_entry_limit");

        /* the filter is matched against the source file and function of each log section */
        const std::string filter = ResourceManager_GetSettings()->GetStringValue("ai_log_filter");

        if (!filter.empty()) {
            try {
                AiLog::AiLog_Filter = std::regex(filter, std::regex::icase);

            } catch (const std::regex_error&) {
                SDL_Log("Warning: ai_log_filter is not a valid regular expression, the AI log is not filtered.");

                AiLog::AiLog_Filter = std::nullopt;
            }

        } else {
            AiLog::AiLog_Filter = std::nullopt;
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

#endif /* !defined(NDEBUG) */
