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

#include "window.hpp"

#include "resource_manager.hpp"
#include "window_manager.hpp"

Window::Window(int16_t ulx, int16_t uly, int16_t width, int16_t height)
    : window_id(0),
      ulx(ulx),
      uly(uly),
      width(width),
      height(height),
      flags(WINDOW_NO_FLAGS),
      resource_id(INVALID_ID),
      palette_from_image(false) {}

Window::Window(ResourceID id) : window_id(0), flags(WINDOW_NO_FLAGS), resource_id(id), palette_from_image(false) {
    struct ImageBigHeader image_header;
    int32_t result;

    result = ResourceManager_ReadImageHeader(id, &image_header);
    SDL_assert(result == 1);

    ulx = (WindowManager_WindowWidth - image_header.width) / 2;
    uly = (WindowManager_WindowHeight - image_header.height) / 2;
    width = image_header.width;
    height = image_header.height;
}

Window::Window(ResourceID id, uint8_t win_id)
    : window_id(0), flags(WINDOW_NO_FLAGS), resource_id(id), palette_from_image(false) {
    struct ImageBigHeader image_header;
    WindowInfo* window;
    int32_t result;

    window = WindowManager_GetWindow(win_id);
    result = ResourceManager_ReadImageHeader(id, &image_header);
    SDL_assert(result == 1);

    width = image_header.width;
    height = image_header.height;

    ulx = ((window->window.lrx - window->window.ulx - width) / 2) + window->window.ulx;
    uly = ((window->window.lry - window->window.uly - height) / 2) + window->window.uly;
}

Window::~Window() {
    if (window_id) {
        win_delete(window_id);
    }
}

void Window::FillWindowInfo(WindowInfo* window) {
    window->id = window_id;
    window->buffer = win_get_buf(window_id);
    window->window.ulx = 0;
    window->window.uly = 0;
    window->window.lrx = width;
    window->window.lry = height;
    window->width = width;
}

void Window::Add(bool draw_to_screen) {
    SDL_assert(window_id == 0);

    window_id = win_add(ulx, uly, width, height, 0, flags);

    if (resource_id != INVALID_ID) {
        WindowInfo window;

        FillWindowInfo(&window);
        WindowManager_LoadBigImage(resource_id, &window, width, palette_from_image, draw_to_screen);
    }
}

void Window::GetCursorPosition(int32_t& x, int32_t& y) const {
    int32_t position_x;
    int32_t position_y;

    mouse_get_position(&position_x, &position_y);
    x = position_x - ulx;
    y = position_y - uly;
}

void Window::SetFlags(uint32_t flags) { this->flags = flags; }

void Window::SetPaletteMode(bool palette_from_image) { this->palette_from_image = palette_from_image; }

bool Window::EventHandler(Event* event) { return false; }

WinID Window::GetId() const { return window_id; }

void Window::ResetId() { window_id = 0; }
