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

#include "heatmap.hpp"

#include <SDL3/SDL.h>

#include "smartfile.hpp"
#include "unitinfo.hpp"

HeatMap::HeatMap(int32_t width, int32_t height, uint16_t team, HeatMapQualifier sea_qualifier,
                 HeatMapQualifier land_qualifier, const HeatMapCallouts& callouts)
    : m_width(width),
      m_height(height),
      m_team(team),
      m_cells(static_cast<size_t>(width) * height),
      m_sea_qualifier(std::move(sea_qualifier)),
      m_land_qualifier(std::move(land_qualifier)),
      m_callouts(callouts) {
    SDL_assert(width > 0);
    SDL_assert(height > 0);
}

HeatMap::HeatMap(SmartFileReader& file, int32_t width, int32_t height, uint16_t team, HeatMapQualifier sea_qualifier,
                 HeatMapQualifier land_qualifier, const HeatMapCallouts& callouts)
    : m_width(width),
      m_height(height),
      m_team(team),
      m_cells(static_cast<size_t>(width) * height),
      m_sea_qualifier(std::move(sea_qualifier)),
      m_land_qualifier(std::move(land_qualifier)),
      m_callouts(callouts) {
    SDL_assert(width > 0);
    SDL_assert(height > 0);
    Load(file);
}

bool HeatMap::Add(const UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    SDL_assert(IsValidCoordinate(grid_x, grid_y));

    const size_t index = GetIndex(grid_x, grid_y);
    HeatMapCell& cell = m_cells[index];

    // Check stealth qualifiers and update respective maps with transition callouts
    if (m_sea_qualifier && m_sea_qualifier(unit)) {
        const bool sea_was_zero = (cell.stealth_sea == 0);

        ++cell.stealth_sea;

        if (sea_was_zero && m_callouts.on_sea_revealed) {
            m_callouts.on_sea_revealed(unit, grid_x, grid_y);
        }
    }

    if (m_land_qualifier && m_land_qualifier(unit)) {
        const bool land_was_zero = (cell.stealth_land == 0);

        ++cell.stealth_land;

        if (land_was_zero && m_callouts.on_land_revealed) {
            m_callouts.on_land_revealed(unit, grid_x, grid_y);
        }
    }

    // Always update complete map
    const bool was_hidden = (cell.complete == 0);

    ++cell.complete;

    // Invoke callout if cell just became visible
    if (was_hidden && cell.complete == 1) {
        if (m_callouts.on_cell_revealed) {
            m_callouts.on_cell_revealed(unit, grid_x, grid_y);
        }

        return true;
    }

    return false;
}

bool HeatMap::Remove(const UnitInfo* unit, int32_t grid_x, int32_t grid_y) {
    SDL_assert(IsValidCoordinate(grid_x, grid_y));

    const size_t index = GetIndex(grid_x, grid_y);
    HeatMapCell& cell = m_cells[index];

    // Check stealth qualifiers and update respective maps with underflow protection
    if (m_sea_qualifier && m_sea_qualifier(unit)) {
        SDL_assert(cell.stealth_sea > 0);  // Assert on underflow attempt

        cell.stealth_sea = (cell.stealth_sea > 0) ? cell.stealth_sea - 1 : 0;
    }

    if (m_land_qualifier && m_land_qualifier(unit)) {
        SDL_assert(cell.stealth_land > 0);  // Assert on underflow attempt

        cell.stealth_land = (cell.stealth_land > 0) ? cell.stealth_land - 1 : 0;
    }

    SDL_assert(cell.complete > 0);  // Assert on underflow attempt

    // Always update complete map with underflow protection
    cell.complete = (cell.complete > 0) ? cell.complete - 1 : 0;

    // Invoke callout if cell just became hidden
    if (cell.complete == 0) {
        if (m_callouts.on_cell_hidden) {
            m_callouts.on_cell_hidden(unit, grid_x, grid_y);
        }

        return true;
    }

    return false;
}

uint32_t HeatMap::GetComplete(int32_t grid_x, int32_t grid_y) const noexcept {
    if (!IsValidCoordinate(grid_x, grid_y)) {
        return 0;
    }

    return m_cells[GetIndex(grid_x, grid_y)].complete;
}

uint32_t HeatMap::GetStealthSea(int32_t grid_x, int32_t grid_y) const noexcept {
    if (!IsValidCoordinate(grid_x, grid_y)) {
        return 0;
    }

    return m_cells[GetIndex(grid_x, grid_y)].stealth_sea;
}

uint32_t HeatMap::GetStealthLand(int32_t grid_x, int32_t grid_y) const noexcept {
    if (!IsValidCoordinate(grid_x, grid_y)) {
        return 0;
    }

    return m_cells[GetIndex(grid_x, grid_y)].stealth_land;
}

bool HeatMap::IsVisible(int32_t grid_x, int32_t grid_y) const noexcept { return GetComplete(grid_x, grid_y) > 0; }

bool HeatMap::IsVisible(const UnitInfo* unit) const noexcept {
    if (!unit) {
        return false;
    }

    if (unit->flags & BUILDING) {
        return GetComplete(unit->grid_x, unit->grid_y) || GetComplete(unit->grid_x + 1, unit->grid_y) ||
               GetComplete(unit->grid_x, unit->grid_y + 1) || GetComplete(unit->grid_x + 1, unit->grid_y + 1);
    }

    return GetComplete(unit->grid_x, unit->grid_y) > 0;
}

void HeatMap::Clear() noexcept {
    for (auto& cell : m_cells) {
        cell.complete = 0;
        cell.stealth_sea = 0;
        cell.stealth_land = 0;
    }
}

void HeatMap::Save(SmartFileWriter& file) const noexcept {
    file.Write(m_cells.data(), m_cells.size() * sizeof(HeatMapCell));
}

void HeatMap::Load(SmartFileReader& file) noexcept { file.Read(m_cells.data(), m_cells.size() * sizeof(HeatMapCell)); }

void HeatMap::LoadV70(SmartFileReader& file, uint32_t map_cell_count) noexcept {
    SDL_assert(map_cell_count == m_cells.size());

    // Allocate temporary buffers for the three V70 int8_t arrays
    std::vector<int8_t> legacy_complete(map_cell_count);
    std::vector<int8_t> legacy_stealth_sea(map_cell_count);
    std::vector<int8_t> legacy_stealth_land(map_cell_count);

    // Read the three arrays from the file
    file.Read(legacy_complete.data(), map_cell_count);
    file.Read(legacy_stealth_sea.data(), map_cell_count);
    file.Read(legacy_stealth_land.data(), map_cell_count);

    // Convert to new format, correcting negative values to 0
    for (uint32_t i = 0; i < map_cell_count; ++i) {
        m_cells[i].complete = (legacy_complete[i] < 0) ? 0 : static_cast<uint32_t>(legacy_complete[i]);
        m_cells[i].stealth_sea = (legacy_stealth_sea[i] < 0) ? 0 : static_cast<uint32_t>(legacy_stealth_sea[i]);
        m_cells[i].stealth_land = (legacy_stealth_land[i] < 0) ? 0 : static_cast<uint32_t>(legacy_stealth_land[i]);
    }
}

size_t HeatMap::GetIndex(int32_t grid_x, int32_t grid_y) const noexcept {
    return static_cast<size_t>(grid_y) * m_width + grid_x;
}

bool HeatMap::IsValidCoordinate(int32_t grid_x, int32_t grid_y) const noexcept {
    return grid_x >= 0 && grid_x < m_width && grid_y >= 0 && grid_y < m_height;
}
