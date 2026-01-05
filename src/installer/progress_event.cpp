/* Copyright (c) 2026 M.A.X. Port Team
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

#include "progress_event.hpp"

#include <new>

namespace Installer {

static uint32_t ProgressEvent_Types[INSTALL_PROGRESS_EVENT_COUNT] = {0};
static bool ProgressEvent_Registered = false;

bool InstallProgressEvents_Register() noexcept {
    if (ProgressEvent_Registered) {
        return true;
    }

    // Request consecutive event type IDs from SDL's user event range
    uint32_t base_event_type = SDL_RegisterEvents(INSTALL_PROGRESS_EVENT_COUNT);

    if (base_event_type == static_cast<uint32_t>(-1)) {
        return false;
    }

    // Store allocated event type IDs
    for (uint32_t i = 0; i < INSTALL_PROGRESS_EVENT_COUNT; ++i) {
        ProgressEvent_Types[i] = base_event_type + i;
    }

    ProgressEvent_Registered = true;

    return true;
}

void PushProgressPercentEvent(int percent_complete) noexcept {
    if (!ProgressEvent_Registered) {
        return;
    }

    SDL_Event event;

    SDL_zero(event);

    event.type = ProgressEvent_Types[INSTALL_PROGRESS_PERCENT];
    event.user.code = percent_complete;
    event.user.data1 = nullptr;
    event.user.data2 = nullptr;

    (void)SDL_PushEvent(&event);
}

void PushProgressDetailedEvent(size_t success_count, size_t fail_count, size_t total_count) noexcept {
    if (!ProgressEvent_Registered) {
        return;
    }

    // Allocate event data on heap (main thread will free it)
    auto* event_data = new (std::nothrow) InstallProgressDetailedEvent();

    if (!event_data) {
        return;
    }

    event_data->success_count = success_count;
    event_data->fail_count = fail_count;
    event_data->total_count = total_count;

    SDL_Event event;

    SDL_zero(event);

    event.type = ProgressEvent_Types[INSTALL_PROGRESS_DETAILED];
    event.user.data1 = event_data;
    event.user.data2 = nullptr;

    if (!SDL_PushEvent(&event)) {
        delete event_data;
    }
}

void FreeProgressDetailedEvent(void* event_data) noexcept {
    if (event_data) {
        delete static_cast<InstallProgressDetailedEvent*>(event_data);
    }
}

uint32_t GetProgressPercentEventType() noexcept {
    return (ProgressEvent_Registered) ? ProgressEvent_Types[INSTALL_PROGRESS_PERCENT] : 0;
}

uint32_t GetProgressDetailedEventType() noexcept {
    return (ProgressEvent_Registered) ? ProgressEvent_Types[INSTALL_PROGRESS_DETAILED] : 0;
}

}  // namespace Installer
