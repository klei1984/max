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

#include "gnw.h"
#include "resource_manager.hpp"
#include "smartstring.hpp"

#define AILOG_FILE_LIMIT UINT16_MAX

std::ofstream AiLog::AiLog_File;
SDL_mutex* AiLog::AiLog_Mutex;
int32_t AiLog::AiLog_SectionCount;
int32_t AiLog::AiLog_EntryCount;

inline void AiLog::AiLog_InitMutex() {
    if (!AiLog_Mutex) {
        AiLog_Mutex = SDL_CreateMutex();

        if (!AiLog_Mutex) {
            ResourceManager_ExitGame(EXIT_CODE_INSUFFICIENT_MEMORY);
        }
    }
}

AiLog::AiLog(const char* format, ...) {
    AiLog_InitMutex();

    SDL_LockMutex(AiLog_Mutex);

    if (AiLog_File.is_open()) {
        time_stamp = timer_get();

        va_list args;

        va_start(args, format);
        VSprintf(format, args);
        va_end(args);
    }

    ++AiLog_SectionCount;

    SDL_UnlockMutex(AiLog_Mutex);
}

AiLog::~AiLog() {
    AiLog_InitMutex();

    SDL_LockMutex(AiLog_Mutex);

    --AiLog_SectionCount;

    if (AiLog_File.is_open()) {
        auto elapsed_time{timer_elapsed_time(time_stamp)};

        if (elapsed_time >= TIMER_FPS_TO_MS(50)) {
            NoLockLog("log section complete, %li msecs elapsed", elapsed_time);

        } else {
            NoLockLog("log section complete");
        }

        if (AiLog_EntryCount > AILOG_FILE_LIMIT) {
            auto filepath{(ResourceManager_FilePathGamePref / "ai_log.txt").lexically_normal()};

            AiLog::AiLog_File.close();
            AiLog::AiLog_File.open(filepath.string().c_str());
            AiLog::AiLog_EntryCount = 0;
        }
    }

    SDL_UnlockMutex(AiLog_Mutex);
}

void AiLog::VSprintf(const char* format, va_list args) {
    AiLog_File << SmartString().Sprintf(200, "\n%3i: ", AiLog_SectionCount + 1).GetCStr();

    if (AiLog_SectionCount > 0) {
        AiLog_File << SmartString().Sprintf(200, "%*s", AiLog_SectionCount, "").GetCStr();
    }

    AiLog_File << SmartString().VSprintf(200, format, args).GetCStr();
    AiLog_File.flush();

    ++AiLog_EntryCount;
}

void AiLog::Log(const char* format, ...) {
    AiLog_InitMutex();

    SDL_LockMutex(AiLog_Mutex);

    if (AiLog_File.is_open()) {
        va_list args;

        va_start(args, format);
        VSprintf(format, args);
        va_end(args);
    }

    SDL_UnlockMutex(AiLog_Mutex);
}

void AiLog::NoLockLog(const char* format, ...) {
    va_list args;

    va_start(args, format);
    VSprintf(format, args);
    va_end(args);
}

void AiLog::VLog(const char* format, va_list args) {
    AiLog_InitMutex();

    SDL_LockMutex(AiLog_Mutex);

    if (AiLog_File.is_open()) {
        VSprintf(format, args);
    }

    SDL_UnlockMutex(AiLog_Mutex);
}

void AiLog_Open() {
    AiLog::AiLog_InitMutex();

    SDL_LockMutex(AiLog::AiLog_Mutex);

    if (!AiLog::AiLog_File.is_open()) {
        auto filepath{(ResourceManager_FilePathGamePref / "ai_log.txt").lexically_normal()};

        AiLog::AiLog_File.open(filepath.string().c_str());
        AiLog::AiLog_EntryCount = 0;
    }

    SDL_UnlockMutex(AiLog::AiLog_Mutex);
}

void AiLog_Close() {
    AiLog::AiLog_InitMutex();

    SDL_LockMutex(AiLog::AiLog_Mutex);

    AiLog::AiLog_File.close();

    SDL_UnlockMutex(AiLog::AiLog_Mutex);
}

[[nodiscard]] bool AiLog_IsEnabled() noexcept {
    AiLog::AiLog_InitMutex();

    SDL_LockMutex(AiLog::AiLog_Mutex);

    auto result{AiLog::AiLog_File.is_open()};

    SDL_UnlockMutex(AiLog::AiLog_Mutex);

    return result;
}
