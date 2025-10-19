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
 * Provides pre-computed range fields enabling the AI to quickly determine if targets
 * are within weapon or scan range without expensive pathfinding. For each map cell,
 * stores the minimum range² required for a unit to reach that cell from any position
 * on its traversable terrain.
 *
 * PURPOSE
 * =======
 * Answers: "How much range does a unit need to reach this target?"
 *
 * - Land units: Query m_land_unit_range_field for range² from nearest land position
 * - Water units: Query m_water_unit_range_field for range² from nearest water position
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
 *    - Traversable cells: MOVEMENT_RANGE_ONLY
 *    - Non-traversable cells: RANGE_MASK (compute on first query)
 *
 * 2. LAZY COMPUTATION (GetMinimumRange):
 *    - On first query: expanding ring BFS to find nearest opposite terrain
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
 * - Bits 0-30 (RANGE_MASK): Range² value or lazy-eval marker
 * - Bit 31 (TRAVERSABLE_FLAG): Set if cell is traversable by that unit type
 *
 * Special values:
 * - MOVEMENT_RANGE_ONLY (0xFFFFFFFF): Traversable, no range calculation needed
 * - RANGE_MASK (0x7FFFFFFF): Not yet computed, will calculate on first query
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

TerrainDistanceField::TerrainDistanceField() {
    auto dimensions = ResourceManager_MapSize;

    // Initialize both fields with RANGE_MASK (lazy evaluation - will compute on first query)
    m_land_unit_range_field.resize(dimensions.x * dimensions.y, RANGE_MASK);
    m_water_unit_range_field.resize(dimensions.x * dimensions.y, RANGE_MASK);

    // Step 1: Process base terrain from map data
    // LAND cells → Traversable for land units (no range calculation needed)
    // WATER cells → Traversable for water units (no range calculation needed)
    for (int32_t i = 0; i < dimensions.x; ++i) {
        for (int32_t j = 0; j < dimensions.y; ++j) {
            if (ResourceManager_MapSurfaceMap[i + j * dimensions.x] == SURFACE_TYPE_LAND) {
                // This is land - traversable for land units
                m_land_unit_range_field[i + j * dimensions.x] = MOVEMENT_RANGE_ONLY;
            }

            if (ResourceManager_MapSurfaceMap[i + j * dimensions.x] == SURFACE_TYPE_WATER) {
                // This is water - traversable for water units
                m_water_unit_range_field[i + j * dimensions.x] = MOVEMENT_RANGE_ONLY;
            }
        }
    }

    // Step 2: Process ground cover units (bridges and water platforms)
    // Bridges count as both land and water cells
    for (auto it = UnitsManager_GroundCoverUnits.Begin(); it != UnitsManager_GroundCoverUnits.End(); ++it) {
        if ((*it).GetUnitType() == BRIDGE || (*it).GetUnitType() == WTRPLTFM) {
            if ((*it).GetOrder() != ORDER_IDLE && (*it).hits > 0) {
                // BRIDGES: Built on water - water units can traverse, land units need to calculate range
                // Water field: Already marked as traversable by Step 1
                // Land field: Mark as traversable
                m_land_unit_range_field[(*it).grid_x + (*it).grid_y * dimensions.x] = MOVEMENT_RANGE_ONLY;

                if ((*it).GetUnitType() == WTRPLTFM) {
                    // WATER PLATFORMS: Converts water to land (land units traverse, water units need to calculate
                    // range)
                    m_water_unit_range_field[(*it).grid_x + (*it).grid_y * dimensions.x] = RANGE_MASK;
                }
            }
        }
    }

    // Step 3: Process buildings - require range calculation from both terrain types (RANGE_MASK)
    // Buildings can be targeted from either land or water positions
    // They are neither land nor water - both unit types need to calculate range
    for (auto it = UnitsManager_StationaryUnits.Begin(); it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetOrder() != ORDER_IDLE && (*it).hits > 0 && (*it).GetUnitType() != CNCT_4W) {
            Rect bounds;

            (*it).GetBounds(&bounds);

            for (int32_t i = bounds.ulx; i < bounds.lrx; ++i) {
                for (int32_t j = bounds.uly; j < bounds.lry; ++j) {
                    // Mark for lazy computation in both fields (will compute range on first query)
                    m_land_unit_range_field[i + j * dimensions.x] = RANGE_MASK;
                    m_water_unit_range_field[i + j * dimensions.x] = RANGE_MASK;
                }
            }
        }
    }
}

TerrainDistanceField::~TerrainDistanceField() {}

/*
 * Lazy evaluation of range using expanding ring search
 *
 * ALGORITHM: Distance Transform via multi-source Breadth-First Search (BFS).
 *
 * When a cell has RANGE_MASK (not yet computed), this performs an expanding
 * ring search to find the nearest non-traversable cell. The search radiates
 * outward in concentric rings until non-traversable terrain is found or time limit is hit.
 *
 * Time complexity: O(r²) where r is range to nearest non-traversable terrain
 * Optimization: Stops expanding when ring radius² exceeds current shortest range
 *
 * range_field: The range field to query (land or water unit field)
 * location: Grid position to query
 * Returns: Squared range required, or 0 if out of time budget
 */
uint32_t TerrainDistanceField::GetCalculatedRange(std::vector<uint32_t>& range_field, const Point location) {
    const auto dimensions = ResourceManager_MapSize;
    const auto stored_distance = range_field[location.x + location.y * dimensions.x] & RANGE_MASK;
    uint32_t result;

    if (stored_distance >= RANGE_MASK) {
        // Range not yet computed - do lazy evaluation
        auto position = location;
        uint32_t shortest_distance = RANGE_MASK;

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

                        if (position.x >= 0 && position.x < dimensions.x && position.y >= 0 &&
                            position.y < dimensions.y) {
                            // Check if this cell is non-traversable (requires range calculation)
                            if (range_field[position.x + position.y * dimensions.x] & TRAVERSABLE_FLAG) {
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
            range_field[location.x + location.y * dimensions.x] =
                (range_field[location.x + location.y * dimensions.x] & TRAVERSABLE_FLAG) | shortest_distance;

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
 * Handle terrain becoming non-traversable (e.g., bridge destroyed, building placed)
 *
 * ALGORITHM: Eager propagation via incremental Distance Transform
 *
 * When a cell becomes non-traversable, this immediately performs a flood fill to update
 * all affected cells. Starting from the changed location, it expands outward in concentric
 * rings, updating any cell whose new range to this non-traversable terrain is shorter
 * than its currently stored range.
 *
 * The flood continues expanding until no more cells need updating (all cells in the
 * current ring already have shorter ranges stored).
 *
 * PERFORMANCE OPTIMIZATION:
 * - Only executes if at least one computer player exists in the game
 * - Human-only games skip all processing (no AI needs range data)
 * - Schedules TaskUpdateTerrain for first computer player found to lazily process
 *   any remaining RANGE_MASK cells across multiple game ticks
 *
 * range_field: The range field to update (land or water unit field)
 * location: Grid position that became non-traversable
 */
void TerrainDistanceField::AddNonTraversableTerrain(std::vector<uint32_t>& range_field, const Point location) {
    const auto dimensions = ResourceManager_MapSize;
    auto position = location;
    bool flag = true;

    if (!(range_field[location.x + location.y * dimensions.x] & TRAVERSABLE_FLAG)) {
        // Only proceed if this cell wasn't already marked as non-traversable

        // Schedule lazy updates for first AI team
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                TaskManager.AppendTask(*new (std::nothrow) TaskUpdateTerrain(team));

                // Mark this cell as non-traversable (units can't move here)
                range_field[location.x + location.y * dimensions.x] = MOVEMENT_RANGE_ONLY;

                // Flood fill: Update all cells that now have a closer non-traversable terrain
                for (int32_t i = 1; flag; ++i) {
                    --position.x;
                    ++position.y;

                    flag = false;  // Assume this ring doesn't need propagation

                    for (int32_t direction = 0; direction < 8; direction += 2) {
                        for (int32_t j = 0; j < i * 2; ++j) {
                            position += Paths_8DirPointsArray[direction];

                            if (position.x >= 0 && position.x < dimensions.x && position.y >= 0 &&
                                position.y < dimensions.y) {
                                const uint32_t distance = Access_GetDistance(position, location);

                                // If this cell's stored range is longer than range to newly non-traversable terrain
                                if ((range_field[position.x + position.y * dimensions.x] & RANGE_MASK) > distance) {
                                    // If cell had a valid range (not RANGE_MASK), need to propagate further
                                    if ((range_field[position.x + position.y * dimensions.x] & RANGE_MASK) !=
                                        RANGE_MASK) {
                                        flag = true;  // Continue to next ring
                                    }

                                    // Update to shorter range
                                    range_field[position.x + position.y * dimensions.x] =
                                        (range_field[position.x + position.y * dimensions.x] & TRAVERSABLE_FLAG) |
                                        distance;
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
 * Handle terrain becoming traversable (e.g., bridge placed, building destroyed)
 *
 * ALGORITHM: Lazy invalidation via conservative marking
 *
 * When a cell becomes traversable, we cannot efficiently determine new ranges immediately
 * (affected cells may now have farther ranges to different non-traversable terrain).
 * Instead, this conservatively invalidates any cell whose stored range exactly matches
 * its distance to the removed non-traversable terrain, marking it with RANGE_MASK.
 *
 * Invalidated cells are lazily recomputed later:
 * 1. TaskUpdateTerrain iteratively processes the entire map across multiple game ticks
 * 2. For each cell with RANGE_MASK, calls GetCalculatedRange() to compute actual range
 * 3. Expanding ring BFS finds nearest non-traversable terrain and caches result
 *
 * CONSERVATIVE STRATEGY:
 * This may invalidate cells that coincidentally have the same range to other non-traversable
 * terrain, but this is safe - it just triggers unnecessary recomputation. The alternative
 * (tracking all dependencies) would be more complex and memory-intensive.
 *
 * PERFORMANCE OPTIMIZATION:
 * - Only executes if at least one computer player exists in the game
 * - Human-only games skip all processing (no AI needs range data)
 * - Schedules TaskUpdateTerrain for first computer player found to handle lazy recalculation
 *
 * range_field: The range field to update (land or water unit field)
 * location: Grid position that became traversable
 */
void TerrainDistanceField::RemoveNonTraversableTerrain(std::vector<uint32_t>& range_field, const Point location) {
    const auto dimensions = ResourceManager_MapSize;
    auto position = location;
    bool flag = true;

    if (range_field[location.x + location.y * dimensions.x] & TRAVERSABLE_FLAG) {
        // Only proceed if this cell was actually marked as non-traversable

        // Notify all AI teams
        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                TaskManager.AppendTask(*new (std::nothrow) TaskUpdateTerrain(team));

                // Clear traversable flag and invalidate stored range
                range_field[location.x + location.y * dimensions.x] = RANGE_MASK;

                // Invalidation flood: Mark cells that had this terrain as their nearest
                for (int32_t i = 1; flag; ++i) {
                    --position.x;
                    ++position.y;

                    flag = false;

                    for (int32_t direction = 0; direction < 8; direction += 2) {
                        for (int32_t j = 0; j < i * 2; ++j) {
                            position += Paths_8DirPointsArray[direction];

                            if (position.x >= 0 && position.x < dimensions.x && position.y >= 0 &&
                                position.y < dimensions.y) {
                                const uint32_t distance = Access_GetDistance(position, location);

                                // If cell's range exactly matches range to removed non-traversable terrain
                                if ((range_field[position.x + position.y * dimensions.x] & RANGE_MASK) == distance) {
                                    // Invalidate it (will recompute on next query)
                                    range_field[position.x + position.y * dimensions.x] =
                                        (range_field[position.x + position.y * dimensions.x] & TRAVERSABLE_FLAG) |
                                        RANGE_MASK;

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
 * Main public API for range queries. Returns the minimum range² needed to reach
 * the target from any position on the unit's traversable terrain.
 *
 * Used by AI for attack and scan range planning without expensive pathfinding.
 *
 * location: Target cell position to evaluate
 * surface_type: Unit's movement type (SURFACE_TYPE_LAND, SURFACE_TYPE_WATER, or both)
 * Returns: Squared range required, or 0 for amphibious units
 */
uint32_t TerrainDistanceField::GetMinimumRange(const Point location, const int32_t surface_type) {
    uint32_t result;

    if (!(surface_type & SURFACE_TYPE_WATER)) {
        // LAND-ONLY unit: Query land unit range field
        // Returns attack range² from nearest land position to target
        result = GetCalculatedRange(m_land_unit_range_field, location);

    } else if (!(surface_type & SURFACE_TYPE_LAND)) {
        // SEA-ONLY unit: Query water unit range field
        // Returns attack range² from nearest water position to target
        result = GetCalculatedRange(m_water_unit_range_field, location);

    } else {
        // AMPHIBIOUS unit: Can reach any cell by movement, no attack range needed
        result = 0uL;
    }

    return result;
}

/*
 * Update range fields when terrain changes
 *
 * Called when buildings are placed/destroyed or terrain permanently changes.
 *
 * Surface type flags indicate traversability:
 * - SURFACE_TYPE_LAND: Land units can traverse
 * - SURFACE_TYPE_WATER: Water units can traverse
 * - Both flags: Both unit types can traverse
 * - No flags: Non-traversable (e.g., building)
 *
 * location: Grid position that changed
 * surface_type: New terrain type flags
 */
void TerrainDistanceField::OnTerrainChanged(const Point location, const int32_t surface_type) {
    // Update land unit range field (affects land unit attack planning)
    if (surface_type & SURFACE_TYPE_LAND) {
        // Cell is land → Land units can traverse (no range calculation needed)
        RemoveNonTraversableTerrain(m_land_unit_range_field, location);

    } else {
        // Cell is not land → Land units need range calculation
        AddNonTraversableTerrain(m_land_unit_range_field, location);
    }

    // Update water unit range field (affects water unit attack planning)
    if (surface_type & SURFACE_TYPE_WATER) {
        // Cell is water → Water units can traverse (no range calculation needed)
        RemoveNonTraversableTerrain(m_water_unit_range_field, location);

    } else {
        // Cell is not water → Water units need range calculation
        AddNonTraversableTerrain(m_water_unit_range_field, location);
    }
}
