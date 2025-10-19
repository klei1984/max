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
 * Provides efficient range queries for the AI system. For any target cell on the map,
 * quickly determines the minimum weapon/scan range² needed for a unit to reach that
 * target from any position on its traversable terrain.
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
    static constexpr uint32_t RANGE_MASK = static_cast<uint32_t>(INT_MAX);
    static constexpr uint32_t TRAVERSABLE_FLAG = RANGE_MASK + 1uL;
    static constexpr uint32_t MOVEMENT_RANGE_ONLY = (TRAVERSABLE_FLAG | RANGE_MASK);

    std::vector<uint32_t> m_land_unit_range_field;
    std::vector<uint32_t> m_water_unit_range_field;

    uint32_t GetCalculatedRange(std::vector<uint32_t>& range_field, const Point location);
    void AddNonTraversableTerrain(std::vector<uint32_t>& range_field, const Point location);
    void RemoveNonTraversableTerrain(std::vector<uint32_t>& range_field, const Point location);

public:
    TerrainDistanceField();

    ~TerrainDistanceField();

    /**
     * \brief Get minimum range² required to reach a target location
     *
     * Returns the range² needed for a unit to reach the specified target cell from
     * any position on its traversable terrain. Used by AI for attack and scan range
     * planning without expensive pathfinding.
     *
     * \param location Target cell position to evaluate
     * \param surface_type Unit's movement type (SURFACE_TYPE_LAND, SURFACE_TYPE_WATER, or both)
     * \return Range² required from nearest traversable position
     *         - Land units: Range² from nearest land position
     *         - Water units: Range² from nearest water position
     *         - Amphibious units: 0 (can reach any cell by movement)
     *         - Returns 0 if computation budget exceeded
     */
    uint32_t GetMinimumRange(const Point location, const int32_t surface_type);

    /**
     * \brief Update range fields when terrain changes
     *
     * Call when permanent terrain features change (buildings placed/destroyed, etc.).
     *
     * \note Range updates are NOT immediate:
     *       - Adding non-traversable terrain: Immediately propagates closer ranges via flood fill,
     *         but remaining invalidated cells are recalculated lazily
     *       - Removing non-traversable terrain: Conservatively invalidates affected cells,
     *         actual recalculation happens lazily via TaskUpdateTerrain across multiple game ticks
     *       - Range queries may return stale or 0 values until lazy processing completes
     *       - Only processes updates if computer players exist (optimization for human-only games)
     *
     * \param location Grid position that changed
     * \param surface_type New terrain type:
     *        - SURFACE_TYPE_LAND: Land terrain (land units can traverse)
     *        - SURFACE_TYPE_WATER: Water terrain (water units can traverse)
     *        - Both flags: Traversable by both unit types
     *        - No flags: Non-traversable (e.g., building)
     */
    void OnTerrainChanged(const Point location, const int32_t surface_type);
};

#endif /* TERRAINDISTANCEFIELD_HPP */
