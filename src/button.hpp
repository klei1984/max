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

#include "fonts.hpp"
#include "image.hpp"

class Button {
    ButtonID bid;
    short ulx;
    short uly;
    short lrx;
    short lry;
    Image *up;
    Image *down;
    Image *up_disabled;
    Image *down_disabled;
    int p_value;
    int r_value;
    unsigned int flags;
    bool enable;
    ButtonFunc p_func;
    ButtonFunc r_func;
    WinID wid;
    unsigned short sfx;
    bool rest_state;

    friend void Button_PFunc(ButtonID bid, Button *b);
    friend void Button_RFunc(ButtonID bid, Button *b);

public:
    Button(short ulx, short uly, short lrx, short lry);
    Button(unsigned short up, unsigned short down, short ulx, short uly);
    ~Button();

    void Allocate();
    Rect GetBounds() const;

    char *GetUpData() const;
    char *GetDownData() const;
    char *GetUpDisabledData() const;
    char *GetDownDisabledData() const;

    void Copy(WinID wid);
    void Copy(unsigned short id, int ulx, int uly);
    void CopyDisabled(WindowInfo *w);
    void CopyUp(unsigned short id);
    void CopyDown(unsigned short id);
    void CopyUpDisabled(unsigned short id);
    void CopyDownDisabled(unsigned short id);
    void CopyUp(char *data);
    void CopyDown(char *data);
    void CopyUpDisabled(char *data);
    void CopyDownDisabled(char *data);

    void SetPFunc(ButtonFunc p_func, int p_value);
    void SetRFunc(ButtonFunc r_func, int r_value);

    void RegisterButton(WinID wid);

    void Enable(bool enable = true, bool redraw = true);
    void Disable(bool redraw = true);

    void SetRestState(bool rest_state);
    void PlaySound() const;
    void SetSfx(unsigned short id);
    ButtonID GetId() const;
    void SetPValue(int r_value);
    void SetRValue(int r_value);
    void SetFlags(unsigned int flags);
    void SetCaption(const char *caption, short x = 0, short y = 0, FontColor color_up = Fonts_GoldColor,
                    FontColor color_down = Fonts_DarkOrageColor, FontColor color_up_disabled = Fonts_DarkGrayColor,
                    FontColor color_down_disabled = Fonts_DarkGrayColor);
    void SetCaption(const char *caption, Rect r, FontColor color_up = Fonts_GoldColor,
                    FontColor color_down = Fonts_DarkOrageColor, FontColor color_up_disabled = Fonts_DarkGrayColor,
                    FontColor color_down_disabled = Fonts_DarkGrayColor);
};

#endif /* BUTTON_HPP */
