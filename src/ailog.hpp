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

#ifndef AILOG_HPP
#define AILOG_HPP

#include <SDL3/SDL_mutex.h>

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <format>
#include <fstream>
#include <optional>
#include <regex>
#include <string>
#include <string_view>

#include "resource_manager.hpp"

class AiLog {
    static std::ofstream AiLog_File;
    static SDL_Mutex* AiLog_Mutex;
    static int32_t AiLog_SectionCount;
    static int32_t AiLog_EntryCount;
    static int32_t AiLog_EntryLimit;
    static std::optional<std::regex> AiLog_Filter;

    std::chrono::steady_clock::time_point m_time_stamp;
    const char* m_file;
    const char* m_function;
    const bool m_is_filtered;

    static void AiLog_InitMutex();
    void WriteLog(std::string_view message);
    void NoLockLog(std::string_view message);
    [[nodiscard]] bool ShouldFilter() const noexcept;

    friend void AiLog_Open();
    friend void AiLog_Close();
    friend bool AiLog_IsEnabled() noexcept;

public:
    template <typename... Args>
    AiLog(const char* file, const char* function, std::format_string<Args...> fmt, Args&&... args)
        : m_time_stamp(std::chrono::steady_clock::now()),
          m_file(file),
          m_function(function),
          m_is_filtered(ShouldFilter()) {
        AiLog_InitMutex();

        ResourceManager_MutexLock lock(AiLog_Mutex);

        if (AiLog_File.is_open() && !m_is_filtered) {
            auto message = std::format(fmt, std::forward<Args>(args)...);
            WriteLog(message);
        }

        ++AiLog_SectionCount;
    }

    ~AiLog();

    template <typename... Args>
    void Log(std::format_string<Args...> fmt, Args&&... args) {
        AiLog_InitMutex();

        ResourceManager_MutexLock lock(AiLog_Mutex);

        if (AiLog_File.is_open() && !m_is_filtered) {
            auto message = std::format(fmt, std::forward<Args>(args)...);
            WriteLog(message);
        }
    }
};

#if !defined(NDEBUG)
#define AILOG(log, ...) AiLog log(__FILE__, __func__, __VA_ARGS__)
#define AILOG_LOG(log, ...) (log).Log(__VA_ARGS__)
#else
#define AILOG(log, ...)
#define AILOG_LOG(log, ...)
#endif /* !defined(NDEBUG) */

void AiLog_Open();
void AiLog_Close();
[[nodiscard]] bool AiLog_IsEnabled() noexcept;

#endif /* AILOG_HPP */
