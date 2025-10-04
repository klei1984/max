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

#include <SDL_mutex.h>

#include <cstdint>
#include <fstream>

class AiLog {
    static std::ofstream AiLog_File;
    static SDL_mutex* AiLog_Mutex;
    static int32_t AiLog_SectionCount;
    static int32_t AiLog_EntryCount;

    uint32_t time_stamp;

    static void AiLog_InitMutex();
    void VSprintf(const char* format, va_list args);
    void NoLockLog(const char* format, ...);

    friend void AiLog_Open();
    friend void AiLog_Close();
    friend bool AiLog_IsEnabled() noexcept;

public:
    AiLog(const char* format, ...);
    ~AiLog();

    void Log(const char* format, ...);
    void VLog(const char* format, va_list args);
};

void AiLog_Open();
void AiLog_Close();
[[nodiscard]] bool AiLog_IsEnabled() noexcept;

#endif /* AILOG_HPP */
