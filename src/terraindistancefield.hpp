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

/**
 * \file terraindistancefield.hpp
 * \brief Terrain-aware range optimization for strategic level AI attack planning
 *
 * Provides efficient range queries for the AI system. For any target cell on the map, quickly determines the minimum
 * weapon/scan range² needed for a unit to reach that target from any position on its traversable terrain.
 *
 * This enables efficient target filtering without expensive pathfinding.
 * All ranges are squared to avoid expensive sqrt operations.
 */

#ifndef TERRAINDISTANCEFIELD_HPP
#define TERRAINDISTANCEFIELD_HPP

#include <climits>
#include <vector>

#include "point.hpp"

class TerrainDistanceField {
    static constexpr uint32_t DISTANCE_UNEVALUATED = static_cast<uint32_t>(INT_MAX);
    static constexpr uint32_t TRAVERSABLE_BIT = DISTANCE_UNEVALUATED + 1uL;
    static constexpr uint32_t TRAVERSABLE_UNEVALUATED = (TRAVERSABLE_BIT | DISTANCE_UNEVALUATED);

    const Point m_dimensions;

    std::vector<uint32_t> m_land_unit_range_field;
    std::vector<uint32_t> m_water_unit_range_field;

    uint32_t ComputeDistanceToNearestTraversable(std::vector<uint32_t>& range_field, const Point location);
    void AddAnchorAndPropagate(std::vector<uint32_t>& range_field, const Point location);
    void RemoveAnchorAndInvalidate(std::vector<uint32_t>& range_field, const Point location);

public:
    TerrainDistanceField(const Point dimensions);

    ~TerrainDistanceField();

    /**
     * \brief Get minimum range² required to reach a target location
     *
     * Returns the range² needed for a unit to reach the specified target cell from any position on its traversable
     * terrain. Used by AI for attack and scan range planning without expensive pathfinding.
     *
     * \param location Target cell position to evaluate
     * \param surface_type Unit's movement type (SURFACE_TYPE_LAND, SURFACE_TYPE_WATER, or both)
     * \return Range² required from nearest traversable position
     *         - Land units: Range² from nearest land position
     *         - Sea units: Range² from nearest water position
     *         - Amphibious units: 0 (can reach any cell by movement)
     *         - Returns 0 if computation budget exceeded
     */
    uint32_t GetMinimumRange(const Point location, const int32_t surface_type);

    /**
     * \brief Update range fields when terrain changes
     *
     * Call when permanent terrain features change (buildings placed/destroyed, bridges/platforms placed, etc.).
     *
     * \note Range updates are NOT immediate:
     *       - Adding traversable terrain (flag present): Immediately propagates decreased ranges around the new anchor
     *         via flood update; remaining cells recompute lazily
     *       - Removing traversable terrain / adding blocking (flag absent): Conservatively invalidates affected cells
     *         (marks as DISTANCE_UNEVALUATED), actual recalculation happens lazily via TaskUpdateTerrain across ticks
     *       - Range queries may return stale or 0 values until lazy processing completes
     *       - Only processes updates if computer players exist (optimization for human-only games)
     *
     * \param location Grid position that changed
     * \param surface_type New terrain type:
     *        - SURFACE_TYPE_LAND: Land units can traverse (adds anchor to land field)
     *        - SURFACE_TYPE_WATER: Sea units can traverse (adds anchor to water field)
     *        - Both flags: Both unit types can traverse (adds anchors to both fields)
     *        - No flags: Non-traversable (removes anchors from both fields)
     */
    void OnTerrainChanged(const Point location, const int32_t surface_type);
};

#endif /* TERRAINDISTANCEFIELD_HPP */
