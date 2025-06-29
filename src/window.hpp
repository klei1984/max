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

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "events.hpp"
#include "resource_manager.hpp"

class Window {
protected:
    WinID window_id;
    int16_t ulx;
    int16_t uly;
    int16_t width;
    int16_t height;
    uint32_t flags;
    ResourceID resource_id;
    bool palette_from_image;

public:
    Window(int16_t ulx, int16_t uly, int16_t width, int16_t height);
    Window(ResourceID id);
    Window(ResourceID id, uint8_t win_id);
    virtual ~Window();

    void FillWindowInfo(WindowInfo* window);
    void Add(bool draw_to_screen = false);
    void GetCursorPosition(int32_t& x, int32_t& y) const;
    void SetFlags(uint32_t flags);
    void SetPaletteMode(bool palette_from_image);
    virtual bool EventHandler(Event* event);
    WinID GetId() const;
    void ResetId();
};

#endif /* WINDOW_HPP */
