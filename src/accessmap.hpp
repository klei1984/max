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

#ifndef ACCESSMAP_HPP
#define ACCESSMAP_HPP

#include <cstdint>
#include <vector>

#include "point.hpp"
#include "smartlist.hpp"

class UnitInfo;

/**
 * \class AccessMap
 * \brief Terrain accessibility map using contiguous row-major storage.
 *
 * Stores traversal costs for each map cell. Uses row-major std::vector for cache efficiency.
 * The map is automatically sized to match ResourceManager_MapSize on construction.
 *
 * Cell format:
 * - Bits 0-4: Traversal cost (0 = impassable, 1-31 = passable with cost)
 * - Bit 5 (0x20): Visited/filled marker (used by PathFill)
 * - Bit 6 (0x40): Special exclusion flag
 * - Bit 7 (0x80): Air transport passable flag
 */
class AccessMap {
    std::vector<uint8_t> m_data;
    Point m_size;

    // Process stationary units to mark their positions as impassable.
    void ProcessStationaryUnits(UnitInfo* unit);

    // Process mobile units to mark their positions as impassable.
    void ProcessMobileUnits(SmartList<UnitInfo>* units, UnitInfo* unit, uint8_t flags);

    // Process map surface to set traversal costs based on surface type.
    void ProcessMapSurface(int32_t surface_type, uint8_t value);

    // Process ground cover units (bridges, platforms, roads, etc.).
    void ProcessGroundCover(UnitInfo* unit, int32_t surface_type);

    // Process dangerous enemy units to mark their attack ranges.
    void ProcessDangers(UnitInfo* unit);

    // Process the surface area around a dangerous unit.
    void ProcessSurface(UnitInfo* unit);

public:
    /**
     * \brief Constructs an AccessMap sized to the current map dimensions.
     *
     * Allocates storage for ResourceManager_MapSize and initializes all cells to zero.
     */
    AccessMap();

    /**
     * \brief Constructs an AccessMap with the specified dimensions.
     *
     * \param size The dimensions of the map.
     */
    explicit AccessMap(Point size);

    ~AccessMap() = default;

    AccessMap(const AccessMap&) = default;
    AccessMap& operator=(const AccessMap&) = default;
    AccessMap(AccessMap&&) = default;
    AccessMap& operator=(AccessMap&&) = default;

    /**
     * \brief Access a cell value by coordinates.
     *
     * \param x The X coordinate (column).
     * \param y The Y coordinate (row).
     * \return Reference to the cell value.
     */
    uint8_t& operator()(int32_t x, int32_t y) { return m_data[x * m_size.y + y]; }
    uint8_t operator()(int32_t x, int32_t y) const { return m_data[x * m_size.y + y]; }

    /**
     * \brief Gets the map dimensions.
     *
     * \return The map size as a Point.
     */
    Point GetSize() const { return m_size; }

    /**
     * \brief Gets a reference to this AccessMap.
     *
     * Provided for compatibility with code that calls map.GetMap().
     *
     * \return Reference to this AccessMap.
     */
    AccessMap& GetMap() { return *this; }
    const AccessMap& GetMap() const { return *this; }

    /**
     * \brief Gets a pointer to raw data for a column.
     *
     * \param x The column index.
     * \return Pointer to the start of the column data.
     * \note For row-major storage, this returns a pointer that can be indexed by [y].
     */
    uint8_t* GetColumn(int32_t x) { return &m_data[x * m_size.y]; }
    const uint8_t* GetColumn(int32_t x) const { return &m_data[x * m_size.y]; }

    /**
     * \brief Fills the entire map with a value.
     *
     * \param value The value to fill with.
     */
    void Fill(uint8_t value);

    /**
     * \brief Fills a column with a value.
     *
     * \param x The column index.
     * \param value The value to fill with.
     */
    void FillColumn(int32_t x, uint8_t value);

    /**
     * \brief Fills a range within a column with a value.
     *
     * \param x The column index.
     * \param y_start The starting row index.
     * \param count The number of cells to fill.
     * \param value The value to fill with.
     */
    void FillColumn(int32_t x, int32_t y_start, int32_t count, uint8_t value);

    /**
     * \brief Check if the map is valid (has non-zero dimensions).
     *
     * \return True if the map has valid dimensions.
     */
    bool IsValid() const { return m_size.x > 0 && m_size.y > 0; }

    /**
     * \brief Check if a grid cell is a valid destination.
     *
     * \param grid_x The X coordinate.
     * \param grid_y The Y coordinate.
     * \return True if the cell is passable and not air-transport-only.
     */
    [[nodiscard]] bool IsProcessed(int32_t grid_x, int32_t grid_y) const;

    /**
     * \brief Initialize the access map for pathfinding based on unit capabilities.
     *
     * \param unit The unit to build the access map for.
     * \param flags Access modifier flags.
     * \param caution_level Caution level for avoiding danger.
     */
    void Init(UnitInfo* unit, uint8_t flags, int32_t caution_level);

    /**
     * \brief Apply caution level to the access map, marking dangerous areas as impassable.
     *
     * \param unit The unit to consider for danger assessment.
     * \param caution_level The caution level to apply.
     */
    void ApplyCautionLevel(UnitInfo* unit, int32_t caution_level);
};

#endif /* ACCESSMAP_HPP */
