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

#ifndef RMLUI_PLATFORM_SDL3_HPP
#define RMLUI_PLATFORM_SDL3_HPP

#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL3/SDL.h>

/**
 * \class SystemInterface_SDL3
 * \brief SDL3 platform implementation for RmlUi providing system services, input handling, and clipboard operations.
 *
 * This class implements the RmlUi SystemInterface for SDL3, providing time tracking, mouse cursor management, clipboard
 * access, and virtual keyboard activation. It handles platform-specific SDL3 operations to integrate RmlUi with SDL3
 * windows.
 */
class SystemInterface_SDL3 : public Rml::SystemInterface {
public:
    SystemInterface_SDL3();
    ~SystemInterface_SDL3();

    /**
     * \brief Assigns the SDL3 window context for mouse cursor operations and text input management.
     *
     * \param window The SDL_Window pointer to use for cursor and keyboard operations. Must remain valid during usage.
     */
    void SetWindow(SDL_Window* window);

    /**
     * \brief Returns elapsed time in seconds since SystemInterface_SDL3 construction for animation and timing
     * operations.
     *
     * \return Elapsed time in seconds as a double-precision floating point value with microsecond precision.
     */
    double GetElapsedTime() override;

    /**
     * \brief Changes the active mouse cursor to match the specified RmlUi cursor name for visual feedback states.
     *
     * \param cursor_name Standard cursor name (arrow, move, pointer, resize, cross, text, unavailable, rmlui-scroll*).
     */
    void SetMouseCursor(const Rml::String& cursor_name) override;

    /**
     * \brief Copies the provided text string to the system clipboard for inter-application data exchange.
     *
     * \param text The UTF-8 encoded text string to place on the clipboard. Empty strings clear the clipboard content.
     */
    void SetClipboardText(const Rml::String& text) override;

    /**
     * \brief Retrieves current clipboard text content from the system clipboard into the provided string reference.
     *
     * \param text Output reference to receive the UTF-8 encoded clipboard text. Cleared if clipboard is empty/invalid.
     */
    void GetClipboardText(Rml::String& text) override;

    /**
     * \brief Activates virtual keyboard for text input at the specified caret position with given line height metrics.
     *
     * \param caret_position Screen coordinates in pixels where the text input caret is positioned for keyboard popup.
     * \param line_height Height of the current text line in pixels to size the keyboard appropriately for the field.
     */
    void ActivateKeyboard(Rml::Vector2f caret_position, float line_height) override;

    /**
     * \brief Deactivates the virtual keyboard and stops text input mode when text field loses focus or is dismissed.
     */
    void DeactivateKeyboard() override;

private:
    SDL_Window* m_window;

    SDL_Cursor* m_cursor_default;
    SDL_Cursor* m_cursor_move;
    SDL_Cursor* m_cursor_pointer;
    SDL_Cursor* m_cursor_resize;
    SDL_Cursor* m_cursor_cross;
    SDL_Cursor* m_cursor_text;
    SDL_Cursor* m_cursor_unavailable;
};

namespace RmlSDL3 {

/**
 * \brief Processes an SDL3 event and dispatches it to the RmlUi context for input handling and UI interaction.
 *
 * \param context The RmlUi context that should receive and process the input event for its UI elements and documents.
 * \param window The SDL window that generated the event, used for coordinate transformation and window state queries.
 * \param ev The SDL event structure containing mouse, keyboard, or window event data to process for UI interactions.
 * \return True if the event propagated through RmlUi without being consumed, false if RmlUi handled and consumed it.
 */
bool InputEventHandler(Rml::Context* context, SDL_Window* window, SDL_Event& ev);

}  // namespace RmlSDL3

#endif
