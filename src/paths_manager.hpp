/* Copyright (c) 2022 M.A.X. Port Team
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

#ifndef PATHS_MANAGER_HPP
#define PATHS_MANAGER_HPP

#include <unordered_map>
#include <unordered_set>

#include "accessmap.hpp"
#include "path_worker.hpp"
#include "pathrequest.hpp"
#include "smartlist.hpp"
#include "unitinfo.hpp"
#include "worker_thread.hpp"

/**
 * \class PathsManager
 * \brief Manages pathfinding requests with worker thread support.
 *
 * Path requests go through the following states:
 * 1. PENDING: In pending_requests queue, waiting for AccessMap to be built
 * 2. DISPATCHED: AccessMap built, job sent to worker thread
 * 3. COMPLETED: Worker finished, result ready for CompleteRequest()
 *
 * Cancellation can happen at any stage - cancelled jobs are tracked and
 * their results discarded when they arrive from the worker.
 */
class PathsManager {
    /// Temporary AccessMap used during job preparation (reused to avoid allocations).
    AccessMap m_access_map;

    /// Requests waiting to be processed (AccessMap not yet built).
    SmartList<PathRequest> m_pending_requests;

    /// Requests dispatched to worker thread, indexed by job_id.
    std::unordered_map<uint32_t, SmartPointer<PathRequest>> m_dispatched_requests;

    /// Job IDs that were cancelled while in the worker - results will be discarded.
    std::unordered_set<uint32_t> m_cancelled_job_ids;

    /// Worker thread for background A* searches.
    WorkerThread<PathWorkerJob, PathWorkerResult> m_worker;

    /// Next job ID to assign.
    uint32_t m_next_job_id;

    /// Complete a request with the given path result.
    void CompleteRequest(SmartPointer<PathRequest>& request, GroundPath* path);

    /// Prepare and dispatch a single request to the worker.
    bool PrepareAndDispatchJob(SmartPointer<PathRequest> request);

    /// Build the AccessMap for a unit/request combination.
    bool BuildAccessMap(UnitInfo* unit, PathRequest* request);

public:
    PathsManager();
    ~PathsManager();

    void PushBack(PathRequest& object);
    void PushFront(PathRequest& object);
    void Clear();
    void RemoveRequest(PathRequest* path_request);
    void RemoveRequest(UnitInfo* unit);
    void PollResults();
    void DispatchJobs();
    [[nodiscard]] bool HasRequest(UnitInfo* unit) const;

    [[nodiscard]] AccessMap& GetAccessMap() { return m_access_map; }
};

#endif /* PATHS_MANAGER_HPP */
