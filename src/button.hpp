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

#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "enums.hpp"
#include "fonts.hpp"
#include "image.hpp"

class Button {
    ButtonID bid;
    int32_t ulx;
    int32_t uly;
    int32_t width;
    int32_t height;
    Image *up;
    Image *down;
    Image *up_disabled;
    Image *down_disabled;
    int32_t p_value;
    int32_t r_value;
    uint32_t flags;
    ButtonFunc p_func;
    ButtonFunc r_func;
    WinID wid;
    ResourceID sfx;
    bool enable;
    bool rest_state;

    friend void Button_PFunc(ButtonID bid, Button *b);
    friend void Button_RFunc(ButtonID bid, Button *b);

public:
    Button(int32_t ulx, int32_t uly, int32_t width, int32_t height);
    Button(ResourceID up, ResourceID down, int32_t ulx, int32_t uly);
    ~Button();

    void Allocate();
    Rect GetBounds() const;

    uint8_t *GetUpData() const;
    uint8_t *GetDownData() const;
    uint8_t *GetUpDisabledData() const;
    uint8_t *GetDownDisabledData() const;

    void Copy(WinID wid);
    void Copy(ResourceID id, int32_t ulx, int32_t uly);
    void CopyDisabled(WindowInfo *w);
    void CopyUp(ResourceID id);
    void CopyDown(ResourceID id);
    void CopyUpDisabled(ResourceID id);
    void CopyDownDisabled(ResourceID id);
    void CopyUp(uint8_t *data);
    void CopyDown(uint8_t *data);
    void CopyUpDisabled(uint8_t *data);
    void CopyDownDisabled(uint8_t *data);

    void SetPFunc(ButtonFunc p_func, int32_t p_value);
    void SetRFunc(ButtonFunc r_func, int32_t r_value);

    void RegisterButton(WinID wid);

    void Enable(bool enable = true, bool redraw = true);
    void Disable(bool redraw = true);

    void SetRestState(bool rest_state);
    void PlaySound() const;
    void SetSfx(ResourceID id);
    ButtonID GetId() const;
    void SetPValue(int32_t p_value);
    void SetRValue(int32_t r_value);
    void SetFlags(uint32_t flags);
    void SetCaption(const char *caption, int16_t x = 0, int16_t y = 0, FontColor color_up = Fonts_GoldColor,
                    FontColor color_down = Fonts_DarkOrageColor, FontColor color_up_disabled = Fonts_DarkGrayColor,
                    FontColor color_down_disabled = Fonts_DarkGrayColor);
    void SetCaption(const char *caption, Rect *r, FontColor color_up = Fonts_GoldColor,
                    FontColor color_down = Fonts_DarkOrageColor, FontColor color_up_disabled = Fonts_DarkGrayColor,
                    FontColor color_down_disabled = Fonts_DarkGrayColor);
};

#endif /* BUTTON_HPP */
