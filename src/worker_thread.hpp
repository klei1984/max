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

#ifndef WORKER_THREAD_HPP
#define WORKER_THREAD_HPP

#include <SDL3/SDL.h>
#include <SDL3/SDL_thread.h>

#include <deque>
#include <functional>
#include <memory>
#include <utility>

/**
 * \class WorkerThread
 * \brief Generic worker thread for background processing with thread-safe job queues.
 *
 * This template class provides a reusable worker thread pattern using SDL threads
 * for Windows XP compatibility. Jobs are submitted from the main thread, processed
 * asynchronously, and results are collected back on the main thread.
 *
 * \tparam TJob The job type. Must be movable. The worker calls job->Execute() to process.
 * \tparam TResult The result type returned by job execution.
 *
 * Usage:
 * 1. Create a job class with an Execute() method returning TResult
 * 2. Instantiate WorkerThread<JobType, ResultType>
 * 3. Call Start() to spawn the worker thread
 * 4. Submit jobs with Submit()
 * 5. Poll for results with PollResult()
 * 6. Call Stop() or let destructor handle cleanup
 *
 * Thread safety:
 * - Submit() and PollResult() are safe to call from the main thread
 * - The worker thread processes jobs sequentially
 * - Spinlocks protect queue access with minimal lock duration
 */
template <typename TJob, typename TResult>
class WorkerThread {
public:
    /**
     * \brief Result structure pairing a job with its execution result.
     */
    struct CompletedJob {
        std::unique_ptr<TJob> job;
        TResult result;

        CompletedJob(std::unique_ptr<TJob> j, TResult r) : job(std::move(j)), result(std::move(r)) {}
    };

    WorkerThread() : m_thread(nullptr), m_queue_lock(0), m_exit_requested(false), m_running(false) {}

    ~WorkerThread() { Stop(); }

    // Non-copyable, non-movable
    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;
    WorkerThread(WorkerThread&&) = delete;
    WorkerThread& operator=(WorkerThread&&) = delete;

    /**
     * \brief Start the worker thread.
     *
     * \param thread_name Name for the thread (for debugging).
     * \return True if thread started successfully.
     */
    bool Start(const char* thread_name = "WorkerThread") {
        if (m_running) {
            return true;
        }

        m_exit_requested = false;
        m_thread = SDL_CreateThread(ThreadFunction, thread_name, this);

        if (m_thread) {
            m_running = true;
            return true;
        }

        return false;
    }

    /**
     * \brief Stop the worker thread and wait for it to finish.
     *
     * Any pending jobs in the input queue will be discarded.
     * Completed results remain available for polling.
     */
    void Stop() {
        if (!m_running) {
            return;
        }

        m_exit_requested = true;

        if (m_thread) {
            SDL_WaitThread(m_thread, nullptr);
            m_thread = nullptr;
        }

        m_running = false;

        // Clear pending jobs
        SDL_LockSpinlock(&m_queue_lock);
        m_pending_jobs.clear();
        SDL_UnlockSpinlock(&m_queue_lock);
    }

    /**
     * \brief Submit a job for background processing.
     *
     * The job will be queued and processed by the worker thread.
     * Ownership of the job is transferred to the worker.
     *
     * \param job The job to process.
     */
    void Submit(std::unique_ptr<TJob> job) {
        SDL_LockSpinlock(&m_queue_lock);
        m_pending_jobs.push_back(std::move(job));
        SDL_UnlockSpinlock(&m_queue_lock);
    }

    /**
     * \brief Poll for a completed job result.
     *
     * \param completed Output parameter for the completed job and result.
     * \return True if a result was available, false if no results pending.
     */
    bool PollResult(CompletedJob& completed) {
        bool has_result = false;

        SDL_LockSpinlock(&m_queue_lock);
        if (!m_completed_jobs.empty()) {
            completed = std::move(m_completed_jobs.front());
            m_completed_jobs.pop_front();
            has_result = true;
        }
        SDL_UnlockSpinlock(&m_queue_lock);

        return has_result;
    }

    /**
     * \brief Check if the worker thread is running.
     *
     * \return True if the worker is active.
     */
    bool IsRunning() const { return m_running; }

    /**
     * \brief Get the number of pending jobs in the input queue.
     *
     * \return Number of jobs waiting to be processed.
     */
    size_t GetPendingCount() const {
        size_t count = 0;
        SDL_LockSpinlock(&m_queue_lock);
        count = m_pending_jobs.size();
        SDL_UnlockSpinlock(&m_queue_lock);
        return count;
    }

    /**
     * \brief Get the number of completed jobs waiting to be collected.
     *
     * \return Number of results available.
     */
    size_t GetCompletedCount() const {
        size_t count = 0;
        SDL_LockSpinlock(&m_queue_lock);
        count = m_completed_jobs.size();
        SDL_UnlockSpinlock(&m_queue_lock);
        return count;
    }

private:
    static int SDLCALL ThreadFunction(void* data) {
        auto* self = static_cast<WorkerThread*>(data);
        self->Run();
        return 0;
    }

    void Run() {
        while (!m_exit_requested) {
            std::unique_ptr<TJob> job;

            // Try to get a job - minimize lock duration
            SDL_LockSpinlock(&m_queue_lock);
            if (!m_pending_jobs.empty()) {
                job = std::move(m_pending_jobs.front());
                m_pending_jobs.pop_front();
            }
            SDL_UnlockSpinlock(&m_queue_lock);

            if (job) {
                // Execute outside the lock
                TResult result = job->Execute();

                // Store result - minimize lock duration
                SDL_LockSpinlock(&m_queue_lock);
                m_completed_jobs.emplace_back(std::move(job), std::move(result));
                SDL_UnlockSpinlock(&m_queue_lock);

            } else {
                // No work available, sleep briefly to avoid busy-waiting
                SDL_Delay(1);
            }
        }
    }

    SDL_Thread* m_thread;
    mutable SDL_SpinLock m_queue_lock;
    std::deque<std::unique_ptr<TJob>> m_pending_jobs;
    std::deque<CompletedJob> m_completed_jobs;
    volatile bool m_exit_requested;
    bool m_running;
};

#endif /* WORKER_THREAD_HPP */
