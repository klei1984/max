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

#ifndef QUICKBUILD_HPP
#define QUICKBUILD_HPP

#include "point.hpp"
#include "unitinfo.hpp"

/**
 * \brief Opens the QuickBuild menu interface allowing the player to select and place units on the map directly.
 *
 * Displays a modal window showing available unit types filtered by team capabilities and allows the player to select
 * one for placement. The menu supports paging through multiple units and provides immediate visual feedback by
 * spawning a preview unit that follows the cursor until placed or cancelled.
 */
void QuickBuild_Menu();

/**
 * \brief Checks if the specified unit type is eligible for QuickBuild placement based on unit properties and flags.
 *
 * \param unit_type The resource ID of the unit type to validate for QuickBuild compatibility and availability.
 * \return True if the unit type can be placed via QuickBuild menu, false if excluded (missiles, slabs, cones, tape).
 */
bool QuickBuild_IsUnitValid(ResourceID unit_type);

/**
 * \brief Validates whether a unit can be placed at the specified grid coordinates based on terrain and occupancy.
 *
 * \param unit The preview unit instance to check placement validity for (handles buildings and mobile units
 * differently).
 * \param grid_x The target X coordinate in the game grid where the unit is being placed for validation checks.
 * \param grid_y The target Y coordinate in the game grid where the unit is being placed for validation checks.
 * \return True if placement is valid and accessible with no blocking units, false if terrain or obstructions prevent
 * placement.
 */
bool QuickBuild_IsPlacementValid(UnitInfo* unit, int32_t grid_x, int32_t grid_y);

/**
 * \brief Updates the image base for amphibious units based on current position terrain type (land vs water).
 *
 * \param unit The amphibious unit instance whose sprite frame needs terrain-based adjustment.
 * \param position The current map position to evaluate surface type and determine appropriate land or water image base.
 */
void QuickBuild_UpdateAmphibiousUnitImageBase(UnitInfo* unit, Point position);

/**
 * \brief Gets the currently selected unit type ID from the QuickBuild menu for deployment on the game map.
 *
 * \return The ResourceID of the unit type that was selected from the QuickBuild menu and is ready for placement.
 */
ResourceID QuickBuild_GetUnitId();

/**
 * \brief Sets the unit type ID to be initially selected or displayed when opening the QuickBuild menu interface.
 *
 * \param unit_id The ResourceID of the unit type to pre-select in the QuickBuild menu for the next menu opening.
 */
void QuickBuild_SetMenuUnitId(ResourceID unit_id);

/**
 * \brief Gets the unit type ID that was last displayed or selected in the QuickBuild menu for maintaining state.
 *
 * \return The ResourceID of the unit type displayed in the QuickBuild menu during the most recent menu session.
 */
ResourceID QuickBuild_GetMenuUnitId();

/**
 * \brief Retrieves a unit at the specified grid location that can be targeted for build orders in QuickBuild mode.
 *
 * \param grid_x The X coordinate in the game grid to search for valid build target units (alien, ground cover, etc.).
 * \param grid_y The Y coordinate in the game grid to search for valid build target units at this map position.
 * \return Pointer to the unit that can be targeted for build orders, or nullptr if no valid target exists at location.
 */
UnitInfo* QuickBuild_GetTargetUnit(int32_t grid_x, int32_t grid_y);

/**
 * \brief Handles mouse click events within the QuickBuild menu's active state to process unit placement or targeting.
 *
 * \param unit The player team unit at the mouse position, or nullptr if no friendly unit exists at that location.
 * \param mouse_position The grid coordinates where the mouse click occurred for placement or build order targeting.
 * \param mouse_buttons The mouse button flags indicating which buttons were pressed (left/right release) for action.
 * \param player_team The team ID of the current player who is interacting with the QuickBuild system for validation.
 */
void QuickBuild_ProcessClick(UnitInfo* unit, Point mouse_position, uint32_t mouse_buttons, uint16_t player_team);

extern bool QuickBuild_MenuActive;
extern SmartPointer<UnitInfo> QuickBuild_PreviewUnit;

#endif /* QUICKBUILD_HPP */
