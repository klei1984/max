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

#include "paths_manager.hpp"

#include <cmath>
#include <vector>

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "aiplayer.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "pathfill.hpp"
#include "resource_manager.hpp"
#include "searcher.hpp"
#include "task_manager.hpp"
#include "ticktimer.hpp"
#include "unit.hpp"
#include "units_manager.hpp"

PathsManager::PathsManager() : m_next_job_id(0) { m_worker.Start("PathWorker"); }

PathsManager::~PathsManager() {
    m_worker.Stop();

    m_access_map = AccessMap();
    m_pending_requests.Clear();
    m_dispatched_requests.clear();
    m_cancelled_job_ids.clear();
}

void PathsManager::PushBack(PathRequest& object) { m_pending_requests.PushBack(object); }

void PathsManager::Clear() {
    m_worker.Stop();

    m_access_map = AccessMap();
    m_pending_requests.Clear();
    m_dispatched_requests.clear();
    m_cancelled_job_ids.clear();

    m_worker.Start("PathWorker");
}

void PathsManager::PushFront(PathRequest& object) { m_pending_requests.PushFront(object); }

void PathsManager::RemoveRequest(PathRequest* path_request) {
    // The smart pointer is required to avoid premature destruction of the held object
    SmartPointer<PathRequest> protect_request(path_request);

    AILOG(log, "Remove path request for {}.",
          ResourceManager_GetUnit(protect_request->GetClient()->GetUnitType()).GetSingularName().data());

    // Check pending queue first
    for (SmartList<PathRequest>::Iterator it = m_pending_requests.Begin(); it != m_pending_requests.End(); ++it) {
        if (&*it == path_request) {
            protect_request->Cancel();
            m_pending_requests.Remove(it);

            return;
        }
    }

    // Check dispatched requests - mark for cancellation
    for (auto& [job_id, req] : m_dispatched_requests) {
        if (req.Get() == path_request) {
            m_cancelled_job_ids.insert(job_id);
            protect_request->Cancel();

            m_dispatched_requests.erase(job_id);

            return;
        }
    }
}

void PathsManager::RemoveRequest(UnitInfo* unit) {
    SmartList<PathRequest> to_remove_pending;
    std::vector<uint32_t> to_remove;

    // Remove from pending queue - collect items to remove first to avoid iterator invalidation
    for (SmartList<PathRequest>::Iterator it = m_pending_requests.Begin(); it != m_pending_requests.End(); ++it) {
        if ((*it).GetClient() == unit) {
            AILOG(log, "Remove path request for {}.",
                  ResourceManager_GetUnit((*it).GetClient()->GetUnitType()).GetSingularName().data());

            (*it).Cancel();
            to_remove_pending.PushBack(*it);
        }
    }

    for (SmartList<PathRequest>::Iterator it = to_remove_pending.Begin(); it != to_remove_pending.End(); ++it) {
        m_pending_requests.Remove(it);
    }

    // Remove from dispatched - mark job IDs for cancellation
    for (auto& [job_id, req] : m_dispatched_requests) {
        if (req && req->GetClient() == unit) {
            AILOG(log, "Remove dispatched path request for {}.",
                  ResourceManager_GetUnit(req->GetClient()->GetUnitType()).GetSingularName().data());

            m_cancelled_job_ids.insert(job_id);

            req->Cancel();
            to_remove.push_back(job_id);
        }
    }

    for (uint32_t job_id : to_remove) {
        m_dispatched_requests.erase(job_id);
    }
}

void PathsManager::PollResults() {
    // Poll completed results from worker thread
    WorkerThread<PathWorkerJob, PathWorkerResult>::CompletedJob completed_job(nullptr, std::nullopt);

    while (m_worker.PollResult(completed_job)) {
        uint32_t job_id = completed_job.job->job_id;

        // Check if this job was cancelled
        if (m_cancelled_job_ids.count(job_id)) {
            AILOG(log, "Discarding cancelled path result for job {}.", job_id);

            m_cancelled_job_ids.erase(job_id);

            continue;
        }

        // Find the request for this job
        auto it = m_dispatched_requests.find(job_id);
        if (it != m_dispatched_requests.end()) {
            SmartPointer<PathRequest> request = it->second;

            m_dispatched_requests.erase(it);

            // Build GroundPath from result
            SmartPointer<GroundPath> ground_path;

            if (completed_job.result) {
                ground_path = new (std::nothrow)
                    GroundPath(completed_job.result->destination.x, completed_job.result->destination.y);

                for (const auto& step : completed_job.result->steps) {
                    ground_path->AddStep(step.x, step.y);
                }

                AILOG(log, "Found path, {} steps.", ground_path->GetSteps()->GetCount());

            } else {
                AILOG(log, "No path found.");
            }

            CompleteRequest(request, &*ground_path);
        }
    }
}

void PathsManager::DispatchJobs() {
    // Dispatch new jobs from pending queue (as many as time allows)
    while (m_pending_requests.GetCount() > 0) {
        // Process mouse events to keep UI responsive
        MouseEvent::ProcessInput();

        if (!TickTimer_HaveTimeToThink()) {
            break;
        }

        //
        PollResults();

        SmartList<PathRequest>::Iterator it = m_pending_requests.Begin();
        SmartPointer<PathRequest> request = it->Get();
        m_pending_requests.Remove(it);

        PrepareAndDispatchJob(request);
    }
}

bool PathsManager::HasRequest(UnitInfo* unit) const {
    // Check pending queue
    for (SmartList<PathRequest>::Iterator it = m_pending_requests.Begin(); it != m_pending_requests.End(); ++it) {
        if ((*it).GetClient() == unit) {
            return true;
        }
    }

    // Check dispatched requests
    for (const auto& [job_id, req] : m_dispatched_requests) {
        if (req && req->GetClient() == unit) {
            return true;
        }
    }

    return false;
}

void PathsManager::CompleteRequest(SmartPointer<PathRequest>& request, GroundPath* path) { request->Finish(path); }

bool PathsManager::BuildAccessMap(UnitInfo* unit, PathRequest* request) {
    bool result;

    // Ensure m_access_map is sized for current map
    if (m_access_map.GetSize() != ResourceManager_MapSize) {
        m_access_map = AccessMap();  // Reconstructs with current ResourceManager_MapSize
    }

    m_access_map.Init(unit, request->GetFlags(), request->GetCautionLevel());

    if (request->GetTransporter()) {
        AccessMap local_access_map;

        local_access_map.Init(request->GetTransporter(), request->GetFlags(), CAUTION_LEVEL_AVOID_ALL_DAMAGE);

        for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
            for (int32_t j = 0; j < ResourceManager_MapSize.y; ++j) {
                if (local_access_map(i, j)) {
                    if (m_access_map(i, j) == 0x00) {
                        m_access_map(i, j) = (local_access_map(i, j) * 3) | 0x80;
                    }

                } else {
                    m_access_map(i, j) |= 0x40;
                }
            }
        }
    }

    {
        Point point1(unit->grid_x, unit->grid_y);
        Point point2 = request->GetDestination();
        int32_t minimum_distance;
        int32_t minimum_distance_sqrt;
        int32_t distance_squared;
        int32_t distance_x;
        int32_t distance_y;
        int32_t limit;
        int32_t limit2;

        m_access_map(point1.x, point1.y) = 2;

        if (request->GetBoardTransport()) {
            SmartPointer<UnitInfo> receiver_unit;

            receiver_unit = Access_GetReceiverUnit(unit, point2.x, point2.y);

            if (receiver_unit != nullptr) {
                int32_t grid_x = receiver_unit->grid_x;
                int32_t grid_y = receiver_unit->grid_y;

                m_access_map(grid_x, grid_y) = 2;

                if (receiver_unit->flags & BUILDING) {
                    m_access_map(grid_x + 1, grid_y) = 2;
                    m_access_map(grid_x, grid_y + 1) = 2;
                    m_access_map(grid_x + 1, grid_y + 1) = 2;
                }
            }
        }

        minimum_distance = request->GetMinimumDistance();
        minimum_distance_sqrt = std::sqrt(minimum_distance);

        result = false;

        if (minimum_distance_sqrt == 0) {
            uint8_t value = m_access_map(point2.x, point2.y);

            if (value && !(value & 0x80)) {
                result = true;
            }
        }

        for (int32_t i = point2.x - minimum_distance_sqrt; i < point2.x; ++i) {
            int32_t j;

            distance_squared = (i - point2.x) * (i - point2.x);

            for (j = point2.y - minimum_distance_sqrt; j <= point2.y; ++j) {
                if ((j - point2.y) * (j - point2.y) + distance_squared <= minimum_distance) {
                    break;
                }
            }

            if (!result && j <= point2.y) {
                distance_x = point2.x * 2 - i;
                distance_y = point2.y * 2 - j;

                if (m_access_map.IsProcessed(i, j)) {
                    result = true;
                }

                if (m_access_map.IsProcessed(i, distance_y)) {
                    result = true;
                }

                if (m_access_map.IsProcessed(distance_x, j)) {
                    result = true;
                }

                if (m_access_map.IsProcessed(distance_x, distance_y)) {
                    result = true;
                }
            }

            ++j;
            limit = point2.y * 2 - j;

            if (j < 0) {
                j = 0;
            }

            if (limit > ResourceManager_MapSize.y - 1) {
                limit = ResourceManager_MapSize.y - 1;
            }

            limit2 = i + 1;
            distance_x = point2.x * 2 - limit2;

            for (; j <= limit; ++j) {
                if (limit2 >= 0) {
                    m_access_map(limit2, j) = 2 | 0x40;
                }

                if (distance_x < ResourceManager_MapSize.x) {
                    m_access_map(distance_x, j) = 2 | 0x40;
                }
            }
        }
    }

    return result;
}

bool PathsManager::PrepareAndDispatchJob(SmartPointer<PathRequest> request) {
    SmartPointer<UnitInfo> unit(request->GetClient());

    Point destination(request->GetDestination());
    Point position(unit->grid_x, unit->grid_y);

    // Check for early-out
    if (request->TryUseCachedPath()) {
        return false;
    }

    AILOG(log, "Start Search for path for {} at [{},{}] to [{},{}].",
          ResourceManager_GetUnit(unit->GetUnitType()).GetSingularName().data(), position.x + 1, position.y + 1,
          destination.x + 1, destination.y + 1);

    // Trivial case: already at destination
    if (position == destination) {
        AILOG_LOG(log, "Start and destination are the same.");

        CompleteRequest(request, nullptr);

        return false;
    }

    // Short distance: check if single step is viable
    if (Access_GetSquaredDistance(position, destination) <= 2) {
        bool is_path_viable = false;

        if (request->GetBoardTransport() && Access_GetReceiverUnit(&*unit, destination.x, destination.y)) {
            is_path_viable = true;

        } else {
            if (Access_IsAccessible(unit->GetUnitType(), unit->team, destination.x, destination.y,
                                    request->GetFlags()) &&
                !Ai_IsDangerousLocation(&*unit, destination, request->GetCautionLevel(), true)) {
                is_path_viable = true;
            }
        }

        if (is_path_viable) {
            SmartPointer<GroundPath> ground_path(new (std::nothrow) GroundPath(destination.x, destination.y));
            ground_path->AddStep(destination.x - position.x, destination.y - position.y);
            CompleteRequest(request, &*ground_path);

        } else {
            CompleteRequest(request, nullptr);
        }

        return false;
    }

    // Build AccessMap (main thread only - reads global game state)
    if (!BuildAccessMap(&*unit, &*request)) {
        AILOG_LOG(log, "No valid destination found.");

        CompleteRequest(request, nullptr);

        return false;
    }

    bool use_air_transport = false;

    if (request->GetTransporter() && request->GetTransporter()->GetUnitType() == AIRTRANS) {
        use_air_transport = true;
    }

    AILOG_LOG(log, "Checking if destination is reachable.");

    // Check reachability with PathFill
    PathFill path_fill(m_access_map);

    path_fill.Fill(position);

    if (!(m_access_map(destination.x, destination.y) & 0x20)) {
        AILOG_LOG(log, "Path not found.");

        CompleteRequest(request, nullptr);

        return false;
    }

    // Create search context with COPY of access map (worker will own this)
    auto context = std::make_unique<PathSearchContext>(m_access_map, position, destination, use_air_transport,
                                                       request->GetMaxCost());

    // Assign job ID and dispatch to worker
    uint32_t job_id = m_next_job_id++;

    auto job = std::make_unique<PathWorkerJob>(job_id, request, std::move(context), position);

    // Track the dispatched request
    m_dispatched_requests[job_id] = request;

    AILOG_LOG(log, "Dispatching path job {} to worker thread.", job_id);

    m_worker.Submit(std::move(job));

    return true;
}
