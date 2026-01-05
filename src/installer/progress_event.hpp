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

#ifndef PROGRESS_EVENT_HPP
#define PROGRESS_EVENT_HPP

#include <SDL3/SDL.h>

#include <cstdint>

namespace Installer {

/**
 * \brief SDL event type identifiers for installer progress reporting from worker threads to main thread event loop.
 *
 * These event types must be registered via SDL_RegisterEvents() before use. Call InstallProgressEvents_Register() once
 * at installer initialization to allocate the event type IDs from SDL's user event range.
 */
enum InstallProgressEventType : uint32_t {
    INSTALL_PROGRESS_PERCENT = 0,     ///< Simple percentage progress (0-100) for overall task completion tracking
    INSTALL_PROGRESS_DETAILED = 1,    ///< Detailed progress with resource name, count, and status message
    INSTALL_PROGRESS_EVENT_COUNT = 2  ///< Total number of event types (internal use for registration)
};

/**
 * \brief Simple percentage progress event data structure for overall installer progress bar updates (0-100).
 *
 * Sent from worker threads via SDL_PushEvent() to update main thread progress displays. The main thread receives this
 * event via SDL_PollEvent() and updates the UI progress bar accordingly.
 */
struct InstallProgressPercentEvent {
    int percent_complete;  ///< Completion percentage (0-100), suitable for progress bar rendering
};

/**
 * \brief Detailed progress event data structure for fine-grained status reporting from asset conversion operations.
 *
 * This structure is heap-allocated by worker threads and passed to the main thread via SDL_PushEvent(). The main
 * thread is responsible for freeing the memory after processing the event. Contains success/fail counts for accurate
 * conversion tracking without undersampled resource names (due to rate limiting).
 */
struct InstallProgressDetailedEvent {
    size_t success_count;  ///< Number of resources successfully converted so far in this operation
    size_t fail_count;     ///< Number of resources that failed conversion so far in this operation
    size_t total_count;    ///< Total number of resources scheduled for conversion in this operation
};

/**
 * \brief Registers SDL user event types for installer progress reporting with SDL's event system.
 *
 * Must be called once during installer initialization before any progress events are pushed. This function allocates
 * consecutive event type IDs from SDL's user event range (SDL_EVENT_USER through SDL_EVENT_LAST). The allocated IDs
 * are stored internally and used by PushProgressPercentEvent() and PushProgressDetailedEvent().
 *
 * \return True if event types were successfully registered, false if SDL's event system failed allocation.
 */
bool InstallProgressEvents_Register() noexcept;

/**
 * \brief Pushes a simple percentage progress event from worker thread to main thread SDL event queue.
 *
 * Thread-safe function that can be called from any worker thread. The event is added to SDL's global event queue and
 * will be received by the main thread's SDL_PollEvent() loop. No dynamic memory allocation is required for this event
 * type since the data fits within SDL_Event's user event fields.
 *
 * \param percent_complete Completion percentage (0-100) for overall progress bar updates.
 */
void PushProgressPercentEvent(int percent_complete) noexcept;

/**
 * \brief Pushes a detailed progress event from worker thread to main thread with success/fail/total conversion counts.
 *
 * Thread-safe function that allocates an InstallProgressDetailedEvent structure on the heap, populates it with the
 * provided count data, and pushes it to SDL's event queue. The main thread receives this event via SDL_PollEvent()
 * and must free the event data pointer after processing to avoid memory leaks.
 *
 * \param success_count Number of resources successfully converted so far in this operation.
 * \param fail_count Number of resources that failed conversion so far in this operation.
 * \param total_count Total number of resources scheduled for conversion in this operation.
 */
void PushProgressDetailedEvent(size_t success_count, size_t fail_count, size_t total_count) noexcept;

/**
 * \brief Frees heap-allocated detailed progress event data after main thread processes the event.
 *
 * Must be called by the main thread after handling an INSTALL_PROGRESS_DETAILED event to release the memory allocated
 * by PushProgressDetailedEvent(). The event_data pointer is obtained from SDL_Event.user.data1.
 *
 * \param event_data Pointer to InstallProgressDetailedEvent structure to be freed (from SDL_Event.user.data1).
 */
void FreeProgressDetailedEvent(void* event_data) noexcept;

/**
 * \brief Retrieves the SDL event type ID for simple percentage progress events.
 *
 * This function returns the event type ID allocated by InstallProgressEvents_Register(). Use this ID to filter events
 * in the main thread's SDL_PollEvent() loop (e.g., if (event.type == GetProgressPercentEventType())).
 *
 * \return SDL event type ID for INSTALL_PROGRESS_PERCENT events, or 0 if registration has not been called.
 */
uint32_t GetProgressPercentEventType() noexcept;

/**
 * \brief Retrieves the SDL event type ID for detailed progress events with resource names and status messages.
 *
 * This function returns the event type ID allocated by InstallProgressEvents_Register(). Use this ID to filter events
 * in the main thread's SDL_PollEvent() loop (e.g., if (event.type == GetProgressDetailedEventType())).
 *
 * \return SDL event type ID for INSTALL_PROGRESS_DETAILED events, or 0 if registration has not been called.
 */
uint32_t GetProgressDetailedEventType() noexcept;

}  // namespace Installer

#endif /* PROGRESS_EVENT_HPP */
