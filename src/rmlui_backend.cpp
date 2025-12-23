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

#include "rmlui_backend.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Debugger.h>
#include <SDL3/SDL.h>

#include "rmlui_platform_sdl3.hpp"
#include "rmlui_renderer_gl3.hpp"

std::unique_ptr<Backend> Backend::Create(const char* window_name, int width, int height, bool allow_resize) {
    auto backend = std::unique_ptr<Backend>(new Backend());

    if (!backend->Initialize(window_name, width, height, allow_resize)) {
        return nullptr;
    }

    return backend;
}

Backend::~Backend() {
    if (m_glcontext) {
        SDL_GL_DestroyContext(static_cast<SDL_GLContext>(m_glcontext));
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
    }

    SDL_Quit();
}

bool Backend::Initialize(const char* window_name, int width, int height, bool allow_resize) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    // OpenGL 3.3 Core Profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    const float window_size_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_PropertiesID props = SDL_CreateProperties();

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, window_name);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, int(width * window_size_scale));
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, int(height * window_size_scale));
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, allow_resize);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);

    SDL_Window* window = SDL_CreateWindowWithProperties(props);

    SDL_DestroyProperties(props);

    if (!window) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    if (!glcontext) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        return false;
    }

    SDL_GL_MakeCurrent(window, glcontext);
    SDL_GL_SetSwapInterval(1);

    if (!RmlGL3::Initialize()) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to initialize OpenGL");
        SDL_GL_DestroyContext(glcontext);
        SDL_DestroyWindow(window);

        return false;
    }

    m_window = window;
    m_glcontext = static_cast<void*>(glcontext);
    m_system_interface = std::make_unique<SystemInterface_SDL3>();
    m_render_interface = std::make_unique<RenderInterface_GL3>();
    m_exit_requested = false;

    if (!m_render_interface) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create render interface");
        SDL_GL_DestroyContext(glcontext);
        SDL_DestroyWindow(window);

        return false;
    }

    m_system_interface->SetWindow(window);
    m_render_interface->SetViewport(width, height);

    return true;
}

Rml::SystemInterface* Backend::GetSystemInterface() { return m_system_interface.get(); }

Rml::RenderInterface* Backend::GetRenderInterface() { return m_render_interface.get(); }

bool Backend::ProcessEvents(Rml::Context* context, void* key_down_callback, bool power_save) {
    if (m_exit_requested) {
        return false;
    }

    SDL_Event event;
    bool has_event = false;

    if (power_save) {
        has_event =
            SDL_WaitEventTimeout(&event, static_cast<int>(Rml::Math::Min(context->GetNextUpdateDelay(), 10.0) * 1000));

    } else {
        has_event = SDL_PollEvent(&event);
    }

    while (has_event) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                return false;
            }

            case SDL_EVENT_KEY_DOWN: {
                if (event.key.key == SDLK_ESCAPE) {
                    return false;
                }

#ifdef ENABLE_RMLUI_DEBUGGER
                if (event.key.key == SDLK_F8) {
                    Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
                }
#endif
            } break;

            default: {
            } break;
        }

        RmlSDL3::InputEventHandler(context, m_window, event);

        has_event = SDL_PollEvent(&event);
    }

    return true;
}

void Backend::RequestExit() { m_exit_requested = true; }

void Backend::BeginFrame() {
    m_render_interface->Clear();
    m_render_interface->BeginFrame();
}

void Backend::PresentFrame() {
    m_render_interface->EndFrame();
    SDL_GL_SwapWindow(m_window);
}
