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

/*
 * Range optimization system for terrain-aware AI attack and scan planning
 *
 * OVERVIEW
 * ========
 * Provides pre-computed range fields enabling the AI to quickly determine if targets are within reachable weapon or
 * scan range without expensive pathfinding. For each map cell, stores the minimum range² required for a unit to reach
 * that cell from any position on its traversable terrain.
 *
 * PURPOSE
 * =======
 * Answers: "How much range does a unit need to reach this target?"
 *
 * - Land units: Query m_land_unit_range_field for range² from nearest land position
 * - Sea units: Query m_water_unit_range_field for range² from nearest water position
 * - Amphibious units: Return 0 (can reach any cell by movement)
 *
 * Enables efficient target filtering in AI planning:
 *
 *   uint32_t range_needed = terrain_field.GetMinimumRange(target_pos, SURFACE_TYPE_LAND);
 *   if (range_needed <= (unit_range * unit_range)) {
 *       consider_target(target);  // Within range from some land position
 *   }
 *
 * ALGORITHM
 * =========
 * Distance Transform with lazy evaluation:
 *
 * 1. INITIALIZATION (Constructor):
 *    - Traversable cells: TRAVERSABLE_UNEVALUATED
 *    - Non-traversable cells: DISTANCE_UNEVALUATED (compute on first query)
 *
 * 2. LAZY COMPUTATION (GetMinimumRange):
 *    - On first query: expanding ring search to find the nearest traversable tile for the queried movement type
 *    - Caches result for subsequent queries
 *    - Time budget prevents frame drops (returns 0 if out of time)
 *
 * 3. INCREMENTAL UPDATES (OnTerrainChanged):
 *    - Terrain changes: Flood fill to update affected ranges
 *    - Invalidates cached values when needed
 *
 * DATA REPRESENTATION
 * ===================
 * Each field is a vector of uint32_t (one per map cell):
 *
 * - Bits 0-30: Range² value or DISTANCE_UNEVALUATED (lazy-eval marker)
 * - Bit 31 (TRAVERSABLE_BIT): Set if cell is traversable by that unit type
 *
 * Special values:
 * - TRAVERSABLE_UNEVALUATED (0xFFFFFFFF): Traversable, distance not yet computed
 * - DISTANCE_UNEVALUATED (0x7FFFFFFF): Not yet computed, will calculate on first query
 * - 0x00000000 - 0x7FFFFFFE: Computed range²
 *
 * IMPLEMENTATION NOTES
 * ====================
 * - All ranges are SQUARED (avoids expensive sqrt)
 * - Time-budgeted computations prevent UI lag
 * - Incremental updates maintain accuracy as map changes
 * - Used for attack and scan planning, NOT pathfinding
 */

#include "terraindistancefield.hpp"

#include "access.hpp"
#include "resource_manager.hpp"
#include "task_manager.hpp"
#include "taskupdateterrain.hpp"
#include "ticktimer.hpp"
#include "units_manager.hpp"

TerrainDistanceField::TerrainDistanceField(const Point dimensions) : m_dimensions(dimensions) {
    // Initialize both fields with DISTANCE_UNEVALUATED (lazy evaluation - will compute on first query)
    m_land_unit_range_field.resize(m_dimensions.x * m_dimensions.y, DISTANCE_UNEVALUATED);
    m_water_unit_range_field.resize(m_dimensions.x * m_dimensions.y, DISTANCE_UNEVALUATED);

    // Step 1: Process base terrain from map data
    // LAND cells → Traversable for land units (no range calculation needed)
    // WATER cells → Traversable for sea units (no range calculation needed)
    for (int32_t i = 0; i < m_dimensions.x; ++i) {
        for (int32_t j = 0; j < m_dimensions.y; ++j) {
            const uint32_t field_offset = i + j * m_dimensions.x;

            if (ResourceManager_MapSurfaceMap[field_offset] == SURFACE_TYPE_LAND) {
                // This is land - traversable for land units
                m_land_unit_range_field[field_offset] = TRAVERSABLE_UNEVALUATED;
            }

            if (ResourceManager_MapSurfaceMap[field_offset] == SURFACE_TYPE_WATER) {
                // This is water - traversable for sea units
                m_water_unit_range_field[field_offset] = TRAVERSABLE_UNEVALUATED;
            }
        }
    }

    // Step 2: Process ground cover units (bridges and water platforms)
    for (auto it = UnitsManager_GroundCoverUnits.Begin(); it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).GetUnitType() == BRIDGE || (*it).GetUnitType() == WTRPLTFM) {
            if ((*it).GetOrder() != ORDER_IDLE && (*it).hits > 0) {
                const int32_t field_offset = (*it).grid_x + (*it).grid_y * m_dimensions.x;

                if ((*it).GetUnitType() == BRIDGE) {
                    // BRIDGES: Traversable by BOTH land and sea units
                    // Must explicitly mark water field since bridges can be on SURFACE_TYPE_COAST (which is not water)
                    m_land_unit_range_field[field_offset] = TRAVERSABLE_UNEVALUATED;
                    m_water_unit_range_field[field_offset] = TRAVERSABLE_UNEVALUATED;

                } else {
                    // WATER PLATFORMS: Converts water to land (land units traverse, sea units cannot)
                    m_land_unit_range_field[field_offset] = TRAVERSABLE_UNEVALUATED;
                    m_water_unit_range_field[field_offset] = DISTANCE_UNEVALUATED;
                }
            }
        }
    }

    // Step 3: Process stationary units - require range calculation from both terrain types (RANGE_MASK)
    // Stationary units can be targeted from either land or water positions
    // They are neither land nor water - both unit types need to calculate range
    // Stationary units can be constructed on top of water platforms
    for (auto it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetOrder() != ORDER_IDLE && (*it).hits > 0 && (*it).GetUnitType() != CNCT_4W) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            for (int32_t i = bounds.ulx; i < bounds.lrx; ++i) {
                for (int32_t j = bounds.uly; j < bounds.lry; ++j) {
                    const uint32_t field_offset = i + j * m_dimensions.x;

                    // Mark for lazy computation in both fields (will compute range on first query)
                    m_land_unit_range_field[field_offset] = DISTANCE_UNEVALUATED;
                    m_water_unit_range_field[field_offset] = DISTANCE_UNEVALUATED;
                }
            }
        }
    }
}

TerrainDistanceField::~TerrainDistanceField() {}

/*
 * Lazy evaluation of range using expanding ring search
 *
 * When a cell has DISTANCE_UNEVALUATED (not yet computed), this performs an expanding ring search to find the nearest
 * traversable cell for the queried movement type. The search radiates outward in concentric rings until a traversable
 * tile is found or time limit is hit.
 *
 * Time complexity: O(r²) where r is range to nearest traversable terrain
 *
 * Optimization:
 * - Stops expanding when ring radius² exceeds current shortest range
 *
 * range_field: The range field to query (land or sea unit field)
 * location: Grid position to query
 * Returns: Squared range required, or 0 if out of time budget
 */
uint32_t TerrainDistanceField::ComputeDistanceToNearestTraversable(std::vector<uint32_t>& range_field,
                                                                   const Point location) {
    const int32_t target_field_offset = location.x + location.y * m_dimensions.x;
    const auto stored_distance = range_field[target_field_offset] & DISTANCE_UNEVALUATED;
    uint32_t result;

    if (stored_distance >= DISTANCE_UNEVALUATED) {
        // Range not yet computed - do lazy evaluation
        auto position = location;
        uint32_t shortest_distance = DISTANCE_UNEVALUATED;

        if (TickTimer_HaveTimeToThink()) {
            // Expanding ring search: Check cells at increasing distances
            for (uint32_t i = 1; i * i < shortest_distance; ++i) {
                // Start position for this ring (south-west corner)
                --position.x;
                ++position.y;

                // Walk around the ring in 4 cardinal directions
                for (int32_t direction = 0; direction < 8; direction += 2) {
                    for (uint32_t j = 0; j < i * 2; ++j) {
                        position += Paths_8DirPointsArray[direction];

                        if (position.x >= 0 && position.x < m_dimensions.x && position.y >= 0 &&
                            position.y < m_dimensions.y) {
                            const int32_t field_offset = position.x + position.y * m_dimensions.x;

                            // Check if position is traversable (then calculate range to location)
                            if (range_field[field_offset] & TRAVERSABLE_BIT) {
                                const uint32_t distance = Access_GetDistance(position, location);

                                if (distance < shortest_distance) {
                                    shortest_distance = distance;
                                }
                            }
                        }
                    }
                }
            }

            // Cache the computed range for future queries
            range_field[target_field_offset] = (range_field[target_field_offset] & TRAVERSABLE_BIT) | shortest_distance;

            result = shortest_distance;

        } else {
            // Out of time - return 0 to avoid blocking game logic
            result = 0;
        }

    } else {
        // Distance already computed - return cached value
        result = stored_distance;
    }

    return result;
}

/*
 * Mark a cell traversable (add anchor) and decrease nearby ranges
 *
 * Marks the cell as traversable (adds an anchor for the corresponding movement type), then performs a local flood
 * update to decrease stored ranges where this new anchor provides a closer approach. Starting from the changed
 * location, it expands outward in concentric rings, updating any cell whose range to this position is shorter than its
 * currently stored range.
 *
 * Optimization:
 * - Only executes if at least one computer player exists in the game
 * - Stops expanding when ring radius² exceeds current shortest range
 * - Schedules TaskUpdateTerrain for first computer player found to lazily process any remaining DISTANCE_UNEVALUATED
 * cells across multiple game ticks
 *
 * range_field: The range field to update (land or sea unit field)
 * location: Grid position that changed
 */
void TerrainDistanceField::AddAnchorAndPropagate(std::vector<uint32_t>& range_field, const Point location) {
    const int32_t target_field_offset = location.x + location.y * m_dimensions.x;
    auto position = location;
    bool flag = true;

    if (!(range_field[target_field_offset] & TRAVERSABLE_BIT)) {
        // Only proceed if this cell wasn't already marked as traversable

        // Schedule lazy updates for first AI team if any
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                TaskManager.AppendTask(*new (std::nothrow) TaskUpdateTerrain(team));

                // Mark this cell as traversable and unevaluated (add anchor here)
                range_field[target_field_offset] = TRAVERSABLE_UNEVALUATED;

                // Flood fill: Update all cells that now have a closer traversable position
                for (int32_t i = 1; flag; ++i) {
                    --position.x;
                    ++position.y;

                    flag = false;  // Assume this ring doesn't need propagation

                    for (int32_t direction = 0; direction < 8; direction += 2) {
                        for (int32_t j = 0; j < i * 2; ++j) {
                            position += Paths_8DirPointsArray[direction];

                            if (position.x >= 0 && position.x < m_dimensions.x && position.y >= 0 &&
                                position.y < m_dimensions.y) {
                                const int32_t field_offset = position.x + position.y * m_dimensions.x;
                                const uint32_t distance = Access_GetDistance(position, location);

                                // If this cell's stored range is longer than range to the newly marked traversable cell
                                if ((range_field[field_offset] & DISTANCE_UNEVALUATED) > distance) {
                                    // If cell had a valid range (not DISTANCE_UNEVALUATED), need to propagate further
                                    if ((range_field[field_offset] & DISTANCE_UNEVALUATED) != DISTANCE_UNEVALUATED) {
                                        flag = true;  // Continue to next ring
                                    }

                                    // Update to shorter range
                                    range_field[field_offset] =
                                        (range_field[field_offset] & TRAVERSABLE_BIT) | distance;
                                }
                            }
                        }
                    }
                }

                return;
            }
        }
    }
}

/*
 * Mark a cell non-traversable (remove anchor) and invalidate dependent ranges
 *
 * ALGORITHM: Lazy invalidation via conservative marking
 *
 * When a cell becomes non-traversable (anchor removed), we cannot efficiently determine new ranges immediately for all
 * affected cells. Instead, conservatively invalidate any cell whose stored range exactly matches its distance to this
 * cell, marking it with DISTANCE_UNEVALUATED so it will recompute lazily.
 *
 * Invalidated cells are lazily recomputed later:
 * 1. TaskUpdateTerrain iteratively processes the entire map across multiple game ticks
 * 2. For each cell with DISTANCE_UNEVALUATED, calls ComputeDistanceToNearestTraversable() to compute actual range
 * 3. Expanding ring search finds nearest traversable terrain and caches result
 *
 * PERFORMANCE OPTIMIZATION:
 * - Only executes if at least one computer player exists in the game
 * - Human-only games skip all processing (no AI needs range data)
 * - Schedules TaskUpdateTerrain for first computer player found to handle lazy recalculation
 *
 * range_field: The range field to update (land or sea unit field)
 * location: Grid position that became traversable
 */
void TerrainDistanceField::RemoveAnchorAndInvalidate(std::vector<uint32_t>& range_field, const Point location) {
    const int32_t target_field_offset = location.x + location.y * m_dimensions.x;
    auto position = location;
    bool flag = true;

    if (range_field[target_field_offset] & TRAVERSABLE_BIT) {
        // Only proceed if this cell is currently traversable (has an anchor)

        // Notify all AI teams
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                TaskManager.AppendTask(*new (std::nothrow) TaskUpdateTerrain(team));

                // Remove traversable flag and invalidate stored range (remove anchor here)
                range_field[target_field_offset] = DISTANCE_UNEVALUATED;

                // Invalidation flood: Mark cells that had this terrain as their nearest
                for (int32_t i = 1; flag; ++i) {
                    --position.x;
                    ++position.y;

                    flag = false;

                    for (int32_t direction = 0; direction < 8; direction += 2) {
                        for (int32_t j = 0; j < i * 2; ++j) {
                            position += Paths_8DirPointsArray[direction];

                            if (position.x >= 0 && position.x < m_dimensions.x && position.y >= 0 &&
                                position.y < m_dimensions.y) {
                                const int32_t field_offset = position.x + position.y * m_dimensions.x;
                                const uint32_t distance = Access_GetDistance(position, location);

                                // If cell's range exactly matches range to the modified terrain at 'location'
                                if ((range_field[field_offset] & DISTANCE_UNEVALUATED) == distance) {
                                    // Invalidate it (will recompute on next query)
                                    range_field[field_offset] =
                                        (range_field[field_offset] & TRAVERSABLE_BIT) | DISTANCE_UNEVALUATED;

                                    flag = true;  // Continue propagating invalidation
                                }
                            }
                        }
                    }
                }

                return;
            }
        }
    }
}

/*
 * Query minimum range² required to reach a target cell
 *
 * Main public API for range queries. Returns the minimum range² needed to reach the target from any position on the
 * unit's traversable terrain.
 *
 * Used by AI for attack and scan range planning without expensive pathfinding.
 *
 * location: Target cell position to evaluate
 * surface_type: Unit's movement type (SURFACE_TYPE_LAND, SURFACE_TYPE_WATER, or both)
 * Returns: Squared range required, or 0 for amphibious units or when out of time budget
 */
uint32_t TerrainDistanceField::GetMinimumRange(const Point location, const int32_t surface_type) {
    uint32_t result;

    if (!(surface_type & SURFACE_TYPE_WATER)) {
        // LAND-ONLY unit: Query land unit range field
        // Returns attack range² from nearest land position to target
        result = ComputeDistanceToNearestTraversable(m_land_unit_range_field, location);

    } else if (!(surface_type & SURFACE_TYPE_LAND)) {
        // SEA-ONLY unit: Query sea unit range field
        // Returns attack range² from nearest water position to target
        result = ComputeDistanceToNearestTraversable(m_water_unit_range_field, location);

    } else {
        // AMPHIBIOUS unit: Can reach any cell by movement, no attack range needed
        result = 0uL;
    }

    return result;
}

/*
 * Update range fields when terrain changes
 *
 * Called when stationary units are placed or destroyed.
 *
 * Surface type flags indicate traversability:
 * - SURFACE_TYPE_LAND: Land units can traverse
 * - SURFACE_TYPE_WATER: Sea units can traverse
 * - Both flags: Both unit types can traverse
 * - No flags: Non-traversable (e.g., a building is present)
 *
 * location: Grid position that changed
 * surface_type: New terrain type flags
 */
void TerrainDistanceField::OnTerrainChanged(const Point location, const int32_t surface_type) {
    // Update land unit range field (affects land unit attack planning)
    if (surface_type & SURFACE_TYPE_LAND) {
        // Cell is land → Land units can traverse (add anchor)
        AddAnchorAndPropagate(m_land_unit_range_field, location);

    } else {
        // Cell is not land → Land units cannot traverse (remove anchor)
        RemoveAnchorAndInvalidate(m_land_unit_range_field, location);
    }

    // Update sea unit range field (affects sea unit attack planning)
    if (surface_type & SURFACE_TYPE_WATER) {
        // Cell is water → Sea units can traverse (add anchor)
        AddAnchorAndPropagate(m_water_unit_range_field, location);

    } else {
        // Cell is not water → Sea units cannot traverse (remove anchor)
        RemoveAnchorAndInvalidate(m_water_unit_range_field, location);
    }
}
