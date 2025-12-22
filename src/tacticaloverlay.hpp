/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef TACTICALOVERLAY_HPP
#define TACTICALOVERLAY_HPP

#include <cstdint>

/**
 * \brief Initialize the tactical overlay system to default state (disabled, mode 1).
 */
void TacticalOverlay_Init();

/**
 * \brief Toggle the tactical overlay on/off state.
 */
void TacticalOverlay_Toggle();

/**
 * \brief Set the active tactical overlay display mode.
 *
 * \param mode The mode to activate (1-7):
 *        - 1: Overall heatmap value for active team
 *        - 2: Land stealth heatmap value for active team
 *        - 3: Sea stealth heatmap value for active team
 *        - 4: Number of team units at grid cell (from hash map)
 *        - 5: WRL surface type enum value for grid cell
 *        - 6: Sea to land TerrainDistanceField value for grid cell
 *        - 7: Land to sea TerrainDistanceField value for grid cell
 */
void TacticalOverlay_SetMode(int32_t mode);

/**
 * \brief Render the tactical overlay on the main map window displaying numeric values for each visible grid cell.
 *
 * This function should be called during the game rendering loop after map tiles are drawn. It iterates over all
 * visible grid cells and renders the appropriate numeric value based on the current overlay mode. Text rendering
 * respects zoom level and automatically scales to fit within grid cell boundaries.
 */
void TacticalOverlay_Render();

#endif /* TACTICALOVERLAY_HPP */
