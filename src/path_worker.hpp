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

#ifndef PATH_WORKER_HPP
#define PATH_WORKER_HPP

#include <cstdint>
#include <memory>
#include <optional>

#include "pathrequest.hpp"
#include "searcher.hpp"
#include "smartpointer.hpp"

/**
 * \struct PathWorkerJob
 * \brief A self-contained path search job for worker thread execution.
 *
 * Contains all data needed to perform a path search independently of the main thread:
 * - The search context (owns AccessMap copy and Searchers)
 * - Reference to the original PathRequest (kept alive via SmartPointer)
 *
 * The Execute() method runs the complete A* search and returns the result.
 */
struct PathWorkerJob {
    uint32_t job_id;                             ///< Unique identifier for tracking/cancellation.
    SmartPointer<PathRequest> request;           ///< The original request (kept alive).
    std::unique_ptr<PathSearchContext> context;  ///< Owns AccessMap copy and Searchers.
    Point start_position;                        ///< Cached unit position at dispatch time.

    PathWorkerJob(uint32_t id, SmartPointer<PathRequest> req, std::unique_ptr<PathSearchContext> ctx, Point start)
        : job_id(id), request(std::move(req)), context(std::move(ctx)), start_position(start) {}

    /**
     * \brief Execute the path search.
     *
     * Runs the complete bidirectional A* search on the worker thread.
     * This method is thread-safe as it only operates on owned data.
     *
     * \return The path result if found, or std::nullopt if no path exists.
     */
    std::optional<PathResult> Execute() {
        if (context) {
            return context->RunSearch();
        }
        return std::nullopt;
    }
};

/**
 * \typedef PathWorkerResult
 * \brief The result type for path worker jobs.
 */
using PathWorkerResult = std::optional<PathResult>;

#endif /* PATH_WORKER_HPP */
