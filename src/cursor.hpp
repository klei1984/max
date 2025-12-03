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

#ifndef CURSOR_HPP
#define CURSOR_HPP

#include "unitinfo.hpp"

/**
 * \brief Cursor type indices for the game's mouse cursor system.
 *
 * These indices map to specific cursor graphics loaded from game resources. The cursor system supports
 * both static cursors (single frame) and animated cursors (multiple frames). Each cursor type has an
 * associated hotspot offset that determines the click point relative to the cursor image.
 */
enum : uint8_t {
    CURSOR_HIDDEN,
    CURSOR_HAND,
    CURSOR_ARROW_N,
    CURSOR_ARROW_NE,
    CURSOR_ARROW_E,
    CURSOR_ARROW_SE,
    CURSOR_ARROW_S,
    CURSOR_ARROW_SW,
    CURSOR_ARROW_W,
    CURSOR_ARROW_NW,
    CURSOR_MAP,
    CURSOR_MINI,
    CURSOR_REPAIR,
    CURSOR_TRANSFER,
    CURSOR_FUEL,
    CURSOR_RELOAD,
    CURSOR_LOAD,
    CURSOR_FRIEND,
    CURSOR_ENEMY,
    CURSOR_FAR_TARGET,
    CURSOR_UNIT_GO,
    CURSOR_UNIT_NO_GO,
    CURSOR_WAY,
    CURSOR_GROUP,
    CURSOR_ACTIVATE,
    CURSOR_MAP2,
    CURSOR_STEAL,
    CURSOR_DISABLE,
    CURSOR_PATH,
    CURSOR_HELP
};

/**
 * \brief Initializes the cursor system.
 *
 * Loads cursor resource data into descriptors and initializes hardware cursor state. Actual SDL cursor
 * creation is deferred until the palette is ready. Registers the animation tick callback for animated
 * cursors.
 */
void Cursor_Init();

/**
 * \brief Deinitializes the cursor system.
 *
 * Destroys all hardware cursors and removes the animation tick callback. Resets all cursor state.
 */
void Cursor_Deinit();

/**
 * \brief Gets the currently active cursor index.
 *
 * \return The index of the currently active cursor type.
 */
uint8_t Cursor_GetCursor();

/**
 * \brief Gets the default cursor type for a specific window.
 *
 * \param window_index Index of the window to query.
 * \return The default cursor type index for the specified window.
 */
uint8_t Cursor_GetDefaultWindowCursor(uint8_t window_index);

/**
 * \brief Sets the active cursor to the specified type.
 *
 * Changes the displayed cursor to the specified type. If the cursor system cache has not been initialized
 * or needs regeneration, it will be updated before applying the new cursor. For animated cursors, the
 * animation is reset to the first frame.
 *
 * \param cursor_index Index of the cursor type to activate.
 */
void Cursor_SetCursor(uint8_t cursor_index);

extern "C" {
/**
 * \brief Shows the mouse cursor.
 *
 * Makes the cursor visible. If cursor regeneration is needed (e.g., after palette change), the cursor
 * cache is updated first. Sets the custom cursor before calling SDL_ShowCursor to prevent default cursor
 * flash.
 */
void Cursor_Show();

/**
 * \brief Hides the mouse cursor.
 *
 * Makes the cursor invisible by calling SDL_HideCursor.
 */
void Cursor_Hide();
}

/**
 * \brief Checks if the cursor is currently hidden.
 *
 * \return True if the cursor is hidden, false if visible.
 */
bool Cursor_IsHidden();

/**
 * \brief Marks the cursor cache as needing regeneration.
 *
 * Should be called when conditions affecting cursor appearance change, such as palette updates. The
 * cursor cache will be regenerated on the next cursor show or set operation.
 */
void Cursor_MarkDirty();

/**
 * \brief Updates the cursor scale based on current window dimensions.
 *
 * Computes the current scale factor and regenerates the cursor cache if the scale has changed
 * significantly, if regeneration is needed, or if the cache was never initialized.
 */
void Cursor_UpdateScale();

/**
 * \brief Regenerates all hardware cursors at the specified scale.
 *
 * Destroys existing hardware cursors and creates new ones at the new scale factor. Re-applies the
 * active cursor at the new scale if it was visible.
 *
 * \param new_scale The scale factor to apply (1.0 = original size).
 */
void Cursor_RegenerateCache(float new_scale);

/**
 * \brief Computes the appropriate cursor scale based on window dimensions.
 *
 * Calculates the scale factor needed to match cursor size with the current display scaling. Uses
 * letterbox scaling (smaller of X and Y scale factors).
 *
 * \return The computed scale factor.
 */
float Cursor_ComputeScale();

/**
 * \brief Updates a cursor to show attack power information.
 *
 * Draws a health bar overlay on the specified cursor showing the target unit's current health and
 * projected damage from the selected unit's attack. Regenerates the hardware cursor after modification.
 *
 * \param selected_unit Pointer to the attacking unit.
 * \param target_unit Pointer to the target unit (may be nullptr to clear the display).
 * \param cursor_index Index of the cursor to modify.
 */
void Cursor_DrawAttackPowerCursor(UnitInfo* selected_unit, UnitInfo* target_unit, uint8_t cursor_index);

/**
 * \brief Updates a cursor to show stealth action success chance.
 *
 * Draws a progress bar overlay on the specified cursor showing the chance of success for a stealth
 * action based on experience level. Regenerates the hardware cursor after modification.
 *
 * \param experience_level The experience level determining success chance (0-100).
 * \param cursor_index Index of the cursor to modify.
 */
void Cursor_DrawStealthActionChanceCursor(int32_t experience_level, uint8_t cursor_index);

/**
 * \brief Renders the current cursor to an indexed-color buffer.
 *
 * Software renders the active cursor into the specified buffer at the current mouse position. Used for
 * screenshot capture and similar operations where the hardware cursor needs to be composited into the
 * frame buffer.
 *
 * \param buffer Pointer to the destination indexed-color pixel buffer.
 * \param buffer_width Width of the destination buffer in pixels.
 * \param buffer_height Height of the destination buffer in pixels.
 */
void Cursor_RenderToBuffer(uint8_t* buffer, int32_t buffer_width, int32_t buffer_height);

#endif /* CURSOR_HPP */
