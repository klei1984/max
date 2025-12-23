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

#ifndef RMLUI_BACKEND_HPP
#define RMLUI_BACKEND_HPP

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>

#include <memory>

// Forward declarations
struct SDL_Window;
class SystemInterface_SDL3;
class RenderInterface_GL3;

/**
 * \class Backend
 * \brief Custom RmlUi backend integrating SDL3 platform and OpenGL3 renderer for UI rendering with RAII lifetime.
 */
class Backend {
public:
    /**
     * \brief Creates and initializes a new RmlUi backend instance with SDL3 window and OpenGL context setup.
     *
     * \param window_name Title string displayed in the window title bar for the RmlUi application window.
     * \param width Window width in pixels for the initial window size (typically 800-1920 for desktop applications).
     * \param height Window height in pixels for the initial window size (typically 600-1080 for desktop applications).
     * \param allow_resize True to enable window resizing by the user, false to create a fixed-size non-resizable
     * window.
     * \return unique_ptr to Backend instance if successful, nullptr if initialization failed (logs error details via
     * RmlUi).
     */
    static std::unique_ptr<Backend> Create(const char* window_name, int width, int height, bool allow_resize);

    /**
     * \brief Destructor - automatically destroys OpenGL context, SDL3 window, and releases all backend resources via
     * RAII.
     */
    ~Backend();

    // Non-copyable, movable for unique_ptr semantics
    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;
    Backend(Backend&&) noexcept = default;
    Backend& operator=(Backend&&) noexcept = default;

    /**
     * \brief Returns pointer to the SDL3 system interface for registering with RmlUi via Rml::SetSystemInterface().
     *
     * \return Pointer to SystemInterface_SDL3 implementation providing time, cursor, clipboard, and keyboard services.
     */
    Rml::SystemInterface* GetSystemInterface();

    /**
     * \brief Returns pointer to the OpenGL3 render interface for registering with RmlUi via Rml::SetRenderInterface().
     *
     * \return Pointer to RenderInterface_GL3 implementation providing geometry compilation and GPU-accelerated
     * rendering.
     */
    Rml::RenderInterface* GetRenderInterface();

    /**
     * \brief Processes SDL3 events and dispatches them to RmlUi context, handling window close requests and input
     * events.
     *
     * \param context The RmlUi context that should receive input events and window state changes for UI interaction
     * processing.
     * \param key_down_callback Optional callback for key events (pass nullptr if not needed for custom key handling
     * logic).
     * \param power_save Enable power saving mode with reduced polling rate (recommended false for responsive UI
     * interaction).
     * \return False if window close was requested or quit event occurred, true to continue the event loop and render
     * next frame.
     */
    bool ProcessEvents(Rml::Context* context, void* key_down_callback = nullptr, bool power_save = false);

    /**
     * \brief Requests application closure by setting internal exit flag, causing next ProcessEvents() call to return
     * false.
     */
    void RequestExit();

    /**
     * \brief Prepares the render state for RmlUi rendering by clearing buffers and setting up OpenGL blending and
     * viewport.
     */
    void BeginFrame();

    /**
     * \brief Presents the rendered frame to the window by swapping OpenGL back buffer and completing the frame
     * rendering cycle.
     */
    void PresentFrame();

    /**
     * \brief Returns the SDL window handle for direct window operations (display scale, properties, etc.).
     *
     * \return SDL_Window pointer managed by this backend instance. Lifetime matches the Backend instance.
     */
    SDL_Window* GetWindow() const { return m_window; }

private:
    Backend() = default;
    bool Initialize(const char* window_name, int width, int height, bool allow_resize);

    SDL_Window* m_window = nullptr;
    void* m_glcontext = nullptr;
    std::unique_ptr<SystemInterface_SDL3> m_system_interface;
    std::unique_ptr<RenderInterface_GL3> m_render_interface;
    bool m_exit_requested = false;
};

#endif
