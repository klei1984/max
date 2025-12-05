/* Copyright (c) 2021 M.A.X. Port Team
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

#ifndef HEATMAP_HPP
#define HEATMAP_HPP

#include <cstdint>
#include <functional>
#include <vector>

class SmartFileReader;
class SmartFileWriter;
class UnitInfo;

/**
 * \struct HeatMapCell
 * \brief Cell data structure containing heat values for all three map layers.
 *
 * Using a single struct with three values allows for one contiguous allocation instead of three separate allocations,
 * improving cache locality. Values are unsigned 32-bit integers to support large maps with many overlapping scan
 * ranges without overflow concerns.
 */
struct HeatMapCell {
    uint32_t complete{0};
    uint32_t stealth_sea{0};
    uint32_t stealth_land{0};
};

/**
 * \brief Callback function type to qualify if a unit contributes to a stealth heat map.
 *
 * \param unit The unit being evaluated.
 * \return True if the unit qualifies to contribute to the respective stealth heat map (sea or land).
 */
using HeatMapQualifier = std::function<bool(const UnitInfo* unit)>;

/**
 * \brief Callout function type invoked when heat map coverage changes.
 *
 * Called when a cell's heat value transitions between zero and non-zero. The unit pointer provides access to team
 * information and other unit properties.
 *
 * \param unit The unit causing the transition (being added or removed).
 * \param grid_x The x coordinate of the cell.
 * \param grid_y The y coordinate of the cell.
 */
using HeatMapTransitionCallout = std::function<void(const UnitInfo* unit, int32_t grid_x, int32_t grid_y)>;

/**
 * \struct HeatMapCallouts
 * \brief Collection of all transition callout functions for heat map state changes.
 *
 * Groups all callout function pointers together for cleaner constructor signatures. Each callout is invoked when the
 * respective heat map layer transitions between zero and non-zero coverage at a cell position.
 */
struct HeatMapCallouts {
    HeatMapTransitionCallout on_cell_revealed{nullptr};  ///< Complete map: 0 -> 1 transition
    HeatMapTransitionCallout on_cell_hidden{nullptr};    ///< Complete map: 1 -> 0 transition
    HeatMapTransitionCallout on_sea_revealed{nullptr};   ///< Stealth sea map: 0 -> 1 transition
    HeatMapTransitionCallout on_land_revealed{nullptr};  ///< Stealth land map: 0 -> 1 transition
};

/**
 * \class HeatMap
 * \brief Encapsulates heat map storage and operations for team scan coverage tracking.
 *
 * Heat maps track which grid cells are within scan range of a team's units. The class manages three logical heat map
 * layers:
 * - Complete: Overall scan coverage by all units
 * - Stealth Sea: Scan coverage by units that can detect submersible units
 * - Stealth Land: Scan coverage by units that can detect land stealth units
 *
 * Each cell value represents a reference count of how many units currently cover that position with scan range. Values
 * use unsigned 32-bit integers to support large maps with many overlapping scan ranges without overflow or underflow
 * concerns.
 */
class HeatMap {
public:
    /**
     * \brief Constructs a heat map with specified dimensions and team ownership.
     *
     * \param width Map width in grid cells.
     * \param height Map height in grid cells.
     * \param team Team index that owns this heat map.
     * \param sea_qualifier Callback to determine if a unit contributes to stealth sea map.
     * \param land_qualifier Callback to determine if a unit contributes to stealth land map.
     * \param callouts Optional struct containing transition callout functions.
     */
    HeatMap(int32_t width, int32_t height, uint16_t team, HeatMapQualifier sea_qualifier,
            HeatMapQualifier land_qualifier, const HeatMapCallouts& callouts = {});

    /**
     * \brief Constructs a heat map by loading from a file.
     *
     * \param file SmartFileReader to load from.
     * \param width Map width in grid cells.
     * \param height Map height in grid cells.
     * \param team Team index that owns this heat map.
     * \param sea_qualifier Callback to determine if a unit contributes to stealth sea map.
     * \param land_qualifier Callback to determine if a unit contributes to stealth land map.
     * \param callouts Optional struct containing transition callout functions.
     */
    HeatMap(SmartFileReader& file, int32_t width, int32_t height, uint16_t team, HeatMapQualifier sea_qualifier,
            HeatMapQualifier land_qualifier, const HeatMapCallouts& callouts = {});

    ~HeatMap() = default;

    // Prevent copying, allow moving
    HeatMap(const HeatMap&) = delete;
    HeatMap& operator=(const HeatMap&) = delete;
    HeatMap(HeatMap&&) = default;
    HeatMap& operator=(HeatMap&&) = default;

    /**
     * \brief Adds a unit's contribution to heat map values at the specified position.
     *
     * The complete heat map is always updated. Stealth maps are updated based on the qualifier callbacks provided at
     * construction.
     *
     * \param unit The unit being added to contribute scan coverage.
     * \param grid_x X coordinate of the cell.
     * \param grid_y Y coordinate of the cell.
     * \return True if the complete heat map transitioned from 0 to 1 at this cell.
     */
    bool Add(const UnitInfo* unit, int32_t grid_x, int32_t grid_y);

    /**
     * \brief Removes a unit's contribution from heat map values at the specified position.
     *
     * The complete heat map is always updated. Stealth maps are updated based on the qualifier callbacks provided at
     * construction. Asserts if removal would cause underflow (value going below 0).
     *
     * \param unit The unit being removed from scan coverage.
     * \param grid_x X coordinate of the cell.
     * \param grid_y Y coordinate of the cell.
     * \return True if the complete heat map transitioned from 1 to 0 at this cell.
     */
    bool Remove(const UnitInfo* unit, int32_t grid_x, int32_t grid_y);

    /**
     * \brief Gets the complete heat map value at a position.
     *
     * \param grid_x X coordinate.
     * \param grid_y Y coordinate.
     * \return Heat value (scan coverage count) at the position.
     */
    [[nodiscard]] uint32_t GetComplete(int32_t grid_x, int32_t grid_y) const noexcept;

    /**
     * \brief Gets the stealth sea heat map value at a position.
     *
     * \param grid_x X coordinate.
     * \param grid_y Y coordinate.
     * \return Heat value for submersible detection at the position.
     */
    [[nodiscard]] uint32_t GetStealthSea(int32_t grid_x, int32_t grid_y) const noexcept;

    /**
     * \brief Gets the stealth land heat map value at a position.
     *
     * \param grid_x X coordinate.
     * \param grid_y Y coordinate.
     * \return Heat value for land stealth unit detection at the position.
     */
    [[nodiscard]] uint32_t GetStealthLand(int32_t grid_x, int32_t grid_y) const noexcept;

    /**
     * \brief Checks if a position is visible (has any scan coverage).
     *
     * \param grid_x X coordinate.
     * \param grid_y Y coordinate.
     * \return True if the complete heat value is greater than 0.
     */
    [[nodiscard]] bool IsVisible(int32_t grid_x, int32_t grid_y) const noexcept;

    /**
     * \brief Checks if a unit is visible on the heat map, accounting for unit size.
     *
     * Buildings occupy a 2x2 grid area, so visibility is checked at all four corners. For non-building units, only the
     * single grid cell at the unit's position is checked.
     *
     * \param unit The unit to check visibility for. If nullptr, returns false.
     * \return True if any occupied cell has scan coverage, false otherwise.
     */
    [[nodiscard]] bool IsVisible(const UnitInfo* unit) const noexcept;

    /**
     * \brief Clears all heat map values to zero.
     */
    void Clear() noexcept;

    /**
     * \brief Saves the heat map to a file.
     *
     * \param file SmartFileWriter to save to.
     */
    void Save(SmartFileWriter& file) const noexcept;

    /**
     * \brief Loads the heat map from a file.
     *
     * \param file SmartFileReader to load from.
     */
    void Load(SmartFileReader& file) noexcept;

    /**
     * \brief Loads V70 legacy format heat map data from a file.
     *
     * V70 save files store heat maps as three separate int8_t arrays. This method reads those arrays from the file and
     * converts them to the new HeatMapCell format. Negative values (which are invalid) are corrected to 0.
     *
     * \param file SmartFileReader to load from (positioned at the heat map data).
     * \param map_cell_count The total number of cells in the map (width * height).
     */
    void LoadV70(SmartFileReader& file, uint32_t map_cell_count) noexcept;

    /**
     * \brief Gets the map width.
     *
     * \return Width in grid cells.
     */
    [[nodiscard]] int32_t GetWidth() const noexcept { return m_width; }

    /**
     * \brief Gets the map height.
     *
     * \return Height in grid cells.
     */
    [[nodiscard]] int32_t GetHeight() const noexcept { return m_height; }

    /**
     * \brief Gets the owning team index.
     *
     * \return Team index.
     */
    [[nodiscard]] uint16_t GetTeam() const noexcept { return m_team; }

    /**
     * \brief Provides direct read-only access to the complete heat map for iteration.
     *
     * For performance-critical code that needs to iterate over the entire map. Index calculation: offset = grid_y *
     * width + grid_x
     *
     * \return Const reference to the internal cell vector.
     */
    [[nodiscard]] const std::vector<HeatMapCell>& GetCells() const noexcept { return m_cells; }

private:
    [[nodiscard]] size_t GetIndex(int32_t grid_x, int32_t grid_y) const noexcept;
    [[nodiscard]] bool IsValidCoordinate(int32_t grid_x, int32_t grid_y) const noexcept;

    int32_t m_width;
    int32_t m_height;
    uint16_t m_team;

    std::vector<HeatMapCell> m_cells;

    HeatMapQualifier m_sea_qualifier;
    HeatMapQualifier m_land_qualifier;

    HeatMapCallouts m_callouts;
};

#endif /* HEATMAP_HPP */
