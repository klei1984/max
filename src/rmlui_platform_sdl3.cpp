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

#include "rmlui_platform_sdl3.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>

SystemInterface_SDL3::SystemInterface_SDL3()
    : m_window(nullptr),
      m_cursor_default(nullptr),
      m_cursor_move(nullptr),
      m_cursor_pointer(nullptr),
      m_cursor_resize(nullptr),
      m_cursor_cross(nullptr),
      m_cursor_text(nullptr),
      m_cursor_unavailable(nullptr) {
    m_cursor_default = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    m_cursor_move = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    m_cursor_pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    m_cursor_resize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
    m_cursor_cross = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    m_cursor_text = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    m_cursor_unavailable = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);
}

SystemInterface_SDL3::~SystemInterface_SDL3() {
    SDL_DestroyCursor(m_cursor_default);
    SDL_DestroyCursor(m_cursor_move);
    SDL_DestroyCursor(m_cursor_pointer);
    SDL_DestroyCursor(m_cursor_resize);
    SDL_DestroyCursor(m_cursor_cross);
    SDL_DestroyCursor(m_cursor_text);
    SDL_DestroyCursor(m_cursor_unavailable);
}

void SystemInterface_SDL3::SetWindow(SDL_Window* window) { m_window = window; }

double SystemInterface_SDL3::GetElapsedTime() {
    static const Uint64 start = SDL_GetPerformanceCounter();
    static const double frequency = double(SDL_GetPerformanceFrequency());

    return double(SDL_GetPerformanceCounter() - start) / frequency;
}

void SystemInterface_SDL3::SetMouseCursor(const Rml::String& cursor_name) {
    SDL_Cursor* cursor = nullptr;

    if (cursor_name.empty() || cursor_name == "arrow") {
        cursor = m_cursor_default;

    } else if (cursor_name == "move") {
        cursor = m_cursor_move;

    } else if (cursor_name == "pointer") {
        cursor = m_cursor_pointer;

    } else if (cursor_name == "resize") {
        cursor = m_cursor_resize;

    } else if (cursor_name == "cross") {
        cursor = m_cursor_cross;

    } else if (cursor_name == "text") {
        cursor = m_cursor_text;

    } else if (cursor_name == "unavailable") {
        cursor = m_cursor_unavailable;

    } else if (Rml::StringUtilities::StartsWith(cursor_name, "rmlui-scroll")) {
        cursor = m_cursor_move;
    }

    if (cursor) {
        SDL_SetCursor(cursor);
    }
}

void SystemInterface_SDL3::SetClipboardText(const Rml::String& text) { SDL_SetClipboardText(text.c_str()); }

void SystemInterface_SDL3::GetClipboardText(Rml::String& text) {
    char* raw_text = SDL_GetClipboardText();

    text = Rml::String(raw_text);

    SDL_free(raw_text);
}

void SystemInterface_SDL3::ActivateKeyboard(Rml::Vector2f caret_position, float line_height) {
    if (m_window) {
        const SDL_Rect rect = {int(caret_position.x), int(caret_position.y), 1, int(line_height)};

        SDL_SetTextInputArea(m_window, &rect, 0);
    }
}

void SystemInterface_SDL3::DeactivateKeyboard() {
    if (m_window) {
        SDL_SetTextInputArea(m_window, nullptr, 0);
    }
}

namespace RmlSDL3 {

static Rml::Input::KeyIdentifier ConvertKey(SDL_Keycode sdlkey);
static int ConvertMouseButton(int button);
static int GetKeyModifierState();

bool InputEventHandler(Rml::Context* context, SDL_Window* window, SDL_Event& ev) {
    bool result = true;

    switch (ev.type) {
        case SDL_EVENT_MOUSE_MOTION: {
            result = context->ProcessMouseMove(int(ev.motion.x), int(ev.motion.y), GetKeyModifierState());
        } break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            result = context->ProcessMouseButtonDown(ConvertMouseButton(ev.button.button), GetKeyModifierState());
            SDL_CaptureMouse(true);
        } break;

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            SDL_CaptureMouse(false);
            result = context->ProcessMouseButtonUp(ConvertMouseButton(ev.button.button), GetKeyModifierState());
        } break;

        case SDL_EVENT_MOUSE_WHEEL: {
            result = context->ProcessMouseWheel(float(-ev.wheel.y), GetKeyModifierState());
        } break;

        case SDL_EVENT_KEY_DOWN: {
            result = context->ProcessKeyDown(ConvertKey(ev.key.key), GetKeyModifierState());
            if (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER) {
                result &= context->ProcessTextInput('\n');
            }
        } break;

        case SDL_EVENT_KEY_UP: {
            result = context->ProcessKeyUp(ConvertKey(ev.key.key), GetKeyModifierState());
        } break;

        case SDL_EVENT_TEXT_INPUT: {
            result = context->ProcessTextInput(Rml::String(&ev.text.text[0]));
        } break;

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_RESIZED: {
            Rml::Vector2i dimensions(ev.window.data1, ev.window.data2);
            context->SetDimensions(dimensions);
        } break;

        case SDL_EVENT_WINDOW_MOUSE_ENTER: {
            // Ensure RmlUI refreshes its internal state when mouse re-enters window
        } break;

        case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
            context->ProcessMouseLeave();
        } break;

        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
            const float display_scale = SDL_GetWindowDisplayScale(window);
            context->SetDensityIndependentPixelRatio(display_scale);
        } break;

        default: {
        } break;
    }

    return result;
}

static Rml::Input::KeyIdentifier ConvertKey(SDL_Keycode sdlkey) {
    switch (sdlkey) {
        case SDLK_UNKNOWN:
            return Rml::Input::KI_UNKNOWN;
        case SDLK_ESCAPE:
            return Rml::Input::KI_ESCAPE;
        case SDLK_SPACE:
            return Rml::Input::KI_SPACE;
        case SDLK_0:
            return Rml::Input::KI_0;
        case SDLK_1:
            return Rml::Input::KI_1;
        case SDLK_2:
            return Rml::Input::KI_2;
        case SDLK_3:
            return Rml::Input::KI_3;
        case SDLK_4:
            return Rml::Input::KI_4;
        case SDLK_5:
            return Rml::Input::KI_5;
        case SDLK_6:
            return Rml::Input::KI_6;
        case SDLK_7:
            return Rml::Input::KI_7;
        case SDLK_8:
            return Rml::Input::KI_8;
        case SDLK_9:
            return Rml::Input::KI_9;
        case SDLK_A:
            return Rml::Input::KI_A;
        case SDLK_B:
            return Rml::Input::KI_B;
        case SDLK_C:
            return Rml::Input::KI_C;
        case SDLK_D:
            return Rml::Input::KI_D;
        case SDLK_E:
            return Rml::Input::KI_E;
        case SDLK_F:
            return Rml::Input::KI_F;
        case SDLK_G:
            return Rml::Input::KI_G;
        case SDLK_H:
            return Rml::Input::KI_H;
        case SDLK_I:
            return Rml::Input::KI_I;
        case SDLK_J:
            return Rml::Input::KI_J;
        case SDLK_K:
            return Rml::Input::KI_K;
        case SDLK_L:
            return Rml::Input::KI_L;
        case SDLK_M:
            return Rml::Input::KI_M;
        case SDLK_N:
            return Rml::Input::KI_N;
        case SDLK_O:
            return Rml::Input::KI_O;
        case SDLK_P:
            return Rml::Input::KI_P;
        case SDLK_Q:
            return Rml::Input::KI_Q;
        case SDLK_R:
            return Rml::Input::KI_R;
        case SDLK_S:
            return Rml::Input::KI_S;
        case SDLK_T:
            return Rml::Input::KI_T;
        case SDLK_U:
            return Rml::Input::KI_U;
        case SDLK_V:
            return Rml::Input::KI_V;
        case SDLK_W:
            return Rml::Input::KI_W;
        case SDLK_X:
            return Rml::Input::KI_X;
        case SDLK_Y:
            return Rml::Input::KI_Y;
        case SDLK_Z:
            return Rml::Input::KI_Z;
        case SDLK_SEMICOLON:
            return Rml::Input::KI_OEM_1;
        case SDLK_PLUS:
            return Rml::Input::KI_OEM_PLUS;
        case SDLK_COMMA:
            return Rml::Input::KI_OEM_COMMA;
        case SDLK_MINUS:
            return Rml::Input::KI_OEM_MINUS;
        case SDLK_PERIOD:
            return Rml::Input::KI_OEM_PERIOD;
        case SDLK_SLASH:
            return Rml::Input::KI_OEM_2;
        case SDLK_GRAVE:
            return Rml::Input::KI_OEM_3;
        case SDLK_LEFTBRACKET:
            return Rml::Input::KI_OEM_4;
        case SDLK_BACKSLASH:
            return Rml::Input::KI_OEM_5;
        case SDLK_RIGHTBRACKET:
            return Rml::Input::KI_OEM_6;
        case SDLK_DBLAPOSTROPHE:
            return Rml::Input::KI_OEM_7;
        case SDLK_KP_0:
            return Rml::Input::KI_NUMPAD0;
        case SDLK_KP_1:
            return Rml::Input::KI_NUMPAD1;
        case SDLK_KP_2:
            return Rml::Input::KI_NUMPAD2;
        case SDLK_KP_3:
            return Rml::Input::KI_NUMPAD3;
        case SDLK_KP_4:
            return Rml::Input::KI_NUMPAD4;
        case SDLK_KP_5:
            return Rml::Input::KI_NUMPAD5;
        case SDLK_KP_6:
            return Rml::Input::KI_NUMPAD6;
        case SDLK_KP_7:
            return Rml::Input::KI_NUMPAD7;
        case SDLK_KP_8:
            return Rml::Input::KI_NUMPAD8;
        case SDLK_KP_9:
            return Rml::Input::KI_NUMPAD9;
        case SDLK_KP_ENTER:
            return Rml::Input::KI_NUMPADENTER;
        case SDLK_KP_MULTIPLY:
            return Rml::Input::KI_MULTIPLY;
        case SDLK_KP_PLUS:
            return Rml::Input::KI_ADD;
        case SDLK_KP_MINUS:
            return Rml::Input::KI_SUBTRACT;
        case SDLK_KP_PERIOD:
            return Rml::Input::KI_DECIMAL;
        case SDLK_KP_DIVIDE:
            return Rml::Input::KI_DIVIDE;
        case SDLK_KP_EQUALS:
            return Rml::Input::KI_OEM_NEC_EQUAL;
        case SDLK_BACKSPACE:
            return Rml::Input::KI_BACK;
        case SDLK_TAB:
            return Rml::Input::KI_TAB;
        case SDLK_CLEAR:
            return Rml::Input::KI_CLEAR;
        case SDLK_RETURN:
            return Rml::Input::KI_RETURN;
        case SDLK_PAUSE:
            return Rml::Input::KI_PAUSE;
        case SDLK_CAPSLOCK:
            return Rml::Input::KI_CAPITAL;
        case SDLK_PAGEUP:
            return Rml::Input::KI_PRIOR;
        case SDLK_PAGEDOWN:
            return Rml::Input::KI_NEXT;
        case SDLK_END:
            return Rml::Input::KI_END;
        case SDLK_HOME:
            return Rml::Input::KI_HOME;
        case SDLK_LEFT:
            return Rml::Input::KI_LEFT;
        case SDLK_UP:
            return Rml::Input::KI_UP;
        case SDLK_RIGHT:
            return Rml::Input::KI_RIGHT;
        case SDLK_DOWN:
            return Rml::Input::KI_DOWN;
        case SDLK_INSERT:
            return Rml::Input::KI_INSERT;
        case SDLK_DELETE:
            return Rml::Input::KI_DELETE;
        case SDLK_HELP:
            return Rml::Input::KI_HELP;
        case SDLK_F1:
            return Rml::Input::KI_F1;
        case SDLK_F2:
            return Rml::Input::KI_F2;
        case SDLK_F3:
            return Rml::Input::KI_F3;
        case SDLK_F4:
            return Rml::Input::KI_F4;
        case SDLK_F5:
            return Rml::Input::KI_F5;
        case SDLK_F6:
            return Rml::Input::KI_F6;
        case SDLK_F7:
            return Rml::Input::KI_F7;
        case SDLK_F8:
            return Rml::Input::KI_F8;
        case SDLK_F9:
            return Rml::Input::KI_F9;
        case SDLK_F10:
            return Rml::Input::KI_F10;
        case SDLK_F11:
            return Rml::Input::KI_F11;
        case SDLK_F12:
            return Rml::Input::KI_F12;
        case SDLK_F13:
            return Rml::Input::KI_F13;
        case SDLK_F14:
            return Rml::Input::KI_F14;
        case SDLK_F15:
            return Rml::Input::KI_F15;
        case SDLK_NUMLOCKCLEAR:
            return Rml::Input::KI_NUMLOCK;
        case SDLK_SCROLLLOCK:
            return Rml::Input::KI_SCROLL;
        case SDLK_LSHIFT:
            return Rml::Input::KI_LSHIFT;
        case SDLK_RSHIFT:
            return Rml::Input::KI_RSHIFT;
        case SDLK_LCTRL:
            return Rml::Input::KI_LCONTROL;
        case SDLK_RCTRL:
            return Rml::Input::KI_RCONTROL;
        case SDLK_LALT:
            return Rml::Input::KI_LMENU;
        case SDLK_RALT:
            return Rml::Input::KI_RMENU;
        case SDLK_LGUI:
            return Rml::Input::KI_LMETA;
        case SDLK_RGUI:
            return Rml::Input::KI_RMETA;
        default:
            break;
    }

    return Rml::Input::KI_UNKNOWN;
}

static int ConvertMouseButton(int button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return 0;
        case SDL_BUTTON_RIGHT:
            return 1;
        case SDL_BUTTON_MIDDLE:
            return 2;
        default:
            return 3;
    }
}

static int GetKeyModifierState() {
    SDL_Keymod sdl_mods = SDL_GetModState();
    int retval = 0;

    if (sdl_mods & SDL_KMOD_CTRL) {
        retval |= Rml::Input::KM_CTRL;
    }

    if (sdl_mods & SDL_KMOD_SHIFT) {
        retval |= Rml::Input::KM_SHIFT;
    }

    if (sdl_mods & SDL_KMOD_ALT) {
        retval |= Rml::Input::KM_ALT;
    }

    if (sdl_mods & SDL_KMOD_NUM) {
        retval |= Rml::Input::KM_NUMLOCK;
    }

    if (sdl_mods & SDL_KMOD_CAPS) {
        retval |= Rml::Input::KM_CAPSLOCK;
    }

    return retval;
}

}  // namespace RmlSDL3
