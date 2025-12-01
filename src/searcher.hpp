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

#ifndef SEARCHER_HPP
#define SEARCHER_HPP

#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include "accessmap.hpp"
#include "path.hpp"

/**
 * \struct PathResult
 * \brief Result of path search containing destination and movement steps.
 */
struct PathResult {
    Point destination;
    std::vector<PathStep> steps;

    PathResult(const Point dest) : destination(dest) {}
};

/**
 * \struct PathSquare
 * \brief A position on the grid with associated movement cost.
 *
 * Used in the A* priority queue to track explored positions. Lower cost squares are processed first.
 */
struct PathSquare {
    Point point;
    uint32_t cost;

    constexpr PathSquare() : point(), cost(0) {}
    constexpr PathSquare(const Point& p, uint32_t c) : point(p), cost(c) {}

    constexpr bool operator>(const PathSquare& other) const { return cost > other.cost; }
};

/**
 * \class Searcher
 * \brief Bidirectional A* pathfinding searcher.
 *
 * Performs A* search on a given AccessMapView. The searcher maintains internal cost and direction maps to track
 * exploration progress. Two Searcher instances (forward and backward) work together to find optimal paths by meeting
 * in the middle.
 *
 * The searcher is stateless with respect to global game data - it only reads from the provided AccessMapView
 * reference.
 */
class Searcher {
private:
    static constexpr uint32_t COST_UNVISITED = 0x3FFFFFFF;
    static constexpr uint32_t DISTANCE_INFINITY = 0x7FFFFFFF;
    static constexpr uint8_t DIRECTION_INVALID = 0xFF;

    const AccessMap& m_access_map;
    const Point m_map_size;
    const int32_t m_distance_array_size;
    std::vector<uint32_t> m_cost_map;
    std::vector<uint8_t> m_direction_map;
    std::vector<uint32_t> m_min_cost_at_distance;
    int32_t m_max_explored_distance;
    std::priority_queue<PathSquare, std::vector<PathSquare>, std::greater<PathSquare>> m_open_set;
    Point m_destination;
    bool m_use_air_transport;

    int32_t EvaluateCost(const Point from_position, const Point to_position) const;
    void EvaluateSquare(const Point neighbor, const uint32_t cost, const int32_t direction,
                        Searcher* const other_searcher);
    void UpdateCost(const Point position, const Point target, const uint32_t position_cost);

public:
    /**
     * \brief Constructs a new Searcher instance for pathfinding.
     *
     * Initializes the cost and direction maps for the given map size. The searcher will explore
     * from start_point toward end_point using the terrain costs from access_map.
     *
     * \param access_map Reference to the terrain cost map (must remain valid during search).
     * \param start_point The starting position for this searcher's exploration.
     * \param end_point The target position this searcher is trying to reach.
     * \param air_support Whether air transport paths should be considered.
     */
    Searcher(const AccessMap& access_map, const Point start_point, const Point end_point, const bool air_support);

    /**
     * \brief Destructs the Searcher instance.
     */
    ~Searcher();

    /**
     * \brief Process initial line-of-sight path from start point.
     *
     * Attempts to find a direct path by following the straight-line direction toward the destination.
     * This provides a fast initial solution before full A* exploration begins.
     *
     * \param start_point The position to begin processing from.
     * \param is_forward True for forward searcher (start to goal), false for backward (goal to start).
     */
    void Process(Point start_point, const bool is_forward);

    /**
     * \brief Perform one iteration of forward A* search.
     *
     * Expands the lowest-cost node in the open set and evaluates its neighbors. Coordinates with
     * the backward searcher to detect when the searches have met.
     *
     * \param backward_searcher The backward searcher to coordinate with for bidirectional search.
     * \return True if search should continue, false if complete or no path exists.
     */
    bool ForwardSearch(Searcher* const backward_searcher);

    /**
     * \brief Perform one iteration of backward A* search.
     *
     * Expands from the goal toward the start, working in conjunction with the forward searcher
     * to find an optimal meeting point.
     *
     * \param forward_searcher The forward searcher to coordinate with for bidirectional search.
     * \return True if search should continue, false if complete or no path exists.
     */
    bool BackwardSearch(Searcher* const forward_searcher);

    /**
     * \brief Extract the final path after bidirectional search completes.
     *
     * Reconstructs the path from start to destination by following direction pointers from the
     * meeting point. Returns the path as a sequence of movement steps.
     *
     * \param meeting_point The point where forward and backward searches met.
     * \param max_cost Maximum allowed path cost; paths exceeding this are rejected.
     * \return The path result if valid path found within cost limit, or std::nullopt otherwise.
     */
    std::optional<PathResult> DeterminePath(const Point meeting_point, const int32_t max_cost) const;
};

/**
 * \struct PathSearchContext
 * \brief Self-contained pathfinding context for thread-safe operation.
 *
 * This struct bundles all state needed to perform a bidirectional A* search:
 * - The access map (terrain costs) - owned copy
 * - Forward and backward searchers - created on demand
 */
struct PathSearchContext {
    AccessMap access_map;
    std::unique_ptr<Searcher> forward_searcher;
    std::unique_ptr<Searcher> backward_searcher;
    Point start_point;
    Point destination;
    int32_t max_cost;
    bool use_air_transport;

    PathSearchContext(const AccessMap& map, const Point start, const Point dest, const bool air_transport,
                      const int32_t cost_limit)
        : access_map(map),
          start_point(start),
          destination(dest),
          max_cost(cost_limit),
          use_air_transport(air_transport) {}

    /**
     * \brief Initialize the bidirectional searchers.
     *
     * Call after access_map is finalized.
     */
    void InitSearchers() {
        forward_searcher = std::make_unique<Searcher>(access_map, start_point, destination, use_air_transport);
        backward_searcher = std::make_unique<Searcher>(access_map, destination, start_point, use_air_transport);
    }

    /**
     * \brief Run the initial line-of-sight path processing.
     */
    void ProcessInitialPath() {
        if (forward_searcher && backward_searcher) {
            forward_searcher->Process(start_point, true);
            backward_searcher->Process(destination, false);
        }
    }

    /**
     * \brief Perform one iteration of bidirectional search.
     *
     * \return True if search should continue, false if complete or no path found.
     */
    bool SearchStep() {
        if (!forward_searcher || !backward_searcher) {
            return false;
        }

        backward_searcher->BackwardSearch(forward_searcher.get());

        return forward_searcher->ForwardSearch(backward_searcher.get());
    }

    /**
     * \brief Extract the path result after search completes.
     *
     * \return The path result if a valid path was found, or std::nullopt otherwise.
     */
    std::optional<PathResult> ExtractPath() const {
        if (forward_searcher) {
            return forward_searcher->DeterminePath(start_point, max_cost);
        }

        return std::nullopt;
    }

    /**
     * \brief Run complete search (for worker thread dispatch).
     *
     * Combines InitSearchers, ProcessInitialPath, and iterative SearchStep.
     *
     * \return The path result if a valid path was found, or std::nullopt otherwise.
     */
    std::optional<PathResult> RunSearch() {
        InitSearchers();
        ProcessInitialPath();

        while (SearchStep()) {
            ;  // Continue until search completes
        }

        return ExtractPath();
    }
};

#endif /* SEARCHER_HPP */
