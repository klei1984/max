/* Copyright (c) 2024 M.A.X. Port Team
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

#include "ticktimer.hpp"

#include <algorithm>
#include <cstring>

#include "gnw.h"

#define TICKTIMER_BENCHMARK_ARRAY_SIZE 20

static bool TickTimer_RequestUpdate;
static bool TickTimer_IsInitialized;
static uint32_t TickTimer_LastTimeStamp;
static uint32_t TickTimer_TimeLimit{TIMER_FPS_TO_MS(30 / 1.1)};
static uint8_t TickTimer_TimeBenchmarkNextIndex;
static uint8_t TickTimer_TimeBenchmarkIndices[TICKTIMER_BENCHMARK_ARRAY_SIZE];
static uint32_t TickTimer_TimeBenchmarkValues[TICKTIMER_BENCHMARK_ARRAY_SIZE];

bool TickTimer_HaveTimeToThink() noexcept { return (timer_get() - TickTimer_LastTimeStamp) <= TickTimer_TimeLimit; }

bool TickTimer_HaveTimeToThink(uint32_t milliseconds) noexcept {
    return (timer_get() - TickTimer_LastTimeStamp) <= milliseconds;
}

void TickTimer_RequestTimeLimitUpdate() noexcept { TickTimer_RequestUpdate = true; }

void TickTimer_UpdateTimeLimit() noexcept {
    if (!TickTimer_IsInitialized) {
        for (int32_t i = 0; i < TICKTIMER_BENCHMARK_ARRAY_SIZE; ++i) {
            TickTimer_TimeBenchmarkValues[i] = TIMER_FPS_TO_MS(30 / 1.1);
            TickTimer_TimeBenchmarkIndices[i] = i;

            TickTimer_RequestUpdate = true;
            TickTimer_IsInitialized = true;
        }
    }

    if (TickTimer_RequestUpdate) {
        uint8_t index = TickTimer_TimeBenchmarkIndices[TickTimer_TimeBenchmarkNextIndex];

        if (index < TICKTIMER_BENCHMARK_ARRAY_SIZE - 1) {
            std::memmove(&TickTimer_TimeBenchmarkIndices[index], &TickTimer_TimeBenchmarkIndices[index + 1],
                         sizeof(TickTimer_TimeBenchmarkIndices[0]) * (TICKTIMER_BENCHMARK_ARRAY_SIZE - 1 - index));
        }

        uint32_t elapsed_time = timer_get() - TickTimer_LastTimeStamp;

        if (elapsed_time > TIMER_FPS_TO_MS(1)) {
            elapsed_time = TIMER_FPS_TO_MS(1);
        }

        TickTimer_TimeBenchmarkValues[TickTimer_TimeBenchmarkNextIndex] = elapsed_time;

        for (index = 0; index < TICKTIMER_BENCHMARK_ARRAY_SIZE - 1; ++index) {
            if (TickTimer_TimeBenchmarkValues[TickTimer_TimeBenchmarkIndices[index]] >= elapsed_time) {
                break;
            }
        }

        if (index < TICKTIMER_BENCHMARK_ARRAY_SIZE - 1) {
            std::memmove(&TickTimer_TimeBenchmarkIndices[index + 1], &TickTimer_TimeBenchmarkIndices[index],
                         sizeof(TickTimer_TimeBenchmarkIndices[0]) * (TICKTIMER_BENCHMARK_ARRAY_SIZE - 1 - index));
        }

        TickTimer_TimeBenchmarkIndices[index] = TickTimer_TimeBenchmarkNextIndex;

        TickTimer_TimeBenchmarkNextIndex = (TickTimer_TimeBenchmarkNextIndex + 1) % TICKTIMER_BENCHMARK_ARRAY_SIZE;

        uint32_t time_budget = (elapsed_time * 3) / 2;

        time_budget = std::max(time_budget, TIMER_FPS_TO_MS(50));
        time_budget = std::min(time_budget, TIMER_FPS_TO_MS(30));

        TickTimer_TimeLimit =
            time_budget +
            TickTimer_TimeBenchmarkValues[TickTimer_TimeBenchmarkIndices[TICKTIMER_BENCHMARK_ARRAY_SIZE / 2]];

        if (elapsed_time >= TIMER_FPS_TO_MS(30)) {
            elapsed_time *= 2;

        } else {
            elapsed_time += TIMER_FPS_TO_MS(30);
        }

        TickTimer_TimeLimit = std::min(TickTimer_TimeLimit, elapsed_time);
        TickTimer_TimeLimit = std::max(TickTimer_TimeLimit, TIMER_FPS_TO_MS(30 / 1.1));
    }
}

uint32_t TickTimer_GetElapsedTime() noexcept { return timer_elapsed_time(TickTimer_LastTimeStamp); }

uint32_t TickTimer_GetLastTimeStamp() noexcept { return TickTimer_LastTimeStamp; }

void TickTimer_SetLastTimeStamp(const uint32_t time_stamp) noexcept { TickTimer_LastTimeStamp = time_stamp; }
