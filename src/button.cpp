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

#include "button.hpp"

#include "enums.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "window_manager.hpp"

void Button_PFunc(ButtonID bid, Button *b) {
    if (!b->rest_state) {
        b->rest_state = 1;

        if (b->sfx != INVALID_ID) {
            b->PlaySound();
        }
    }

    if (b->p_func) {
        b->p_func(bid, b->p_value);
    }
}

void Button_RFunc(ButtonID bid, Button *b) {
    b->rest_state = 0;

    if (b->r_func) {
        b->r_func(bid, b->r_value);
    }
}

Button::Button(int32_t ulx, int32_t uly, int32_t width, int32_t height)
    : bid(0),
      ulx(ulx),
      uly(uly),
      width(width),
      height(height),
      up(nullptr),
      down(nullptr),
      up_disabled(nullptr),
      down_disabled(nullptr),
      p_value(-1),
      r_value(-1),
      flags(0),
      enable(true),
      p_func(nullptr),
      r_func(nullptr),
      wid(0),
      sfx(INVALID_ID),
      rest_state(false) {}

Button::Button(ResourceID up, ResourceID down, int32_t ulx, int32_t uly)
    : bid(0),
      up_disabled(nullptr),
      down_disabled(nullptr),
      p_value(-1),
      r_value(-1),
      flags(0),
      enable(true),
      p_func(nullptr),
      r_func(nullptr),
      wid(0),
      sfx(INVALID_ID),
      rest_state(false) {
    this->up = new (std::nothrow) Image(up, ulx, uly);
    this->ulx = this->up->GetULX();
    this->uly = this->up->GetULY();
    this->width = this->up->GetWidth();
    this->height = this->up->GetHeight();
    this->down = new (std::nothrow) Image(down, this->ulx, this->uly);
}

Button::~Button() {
    if (bid) {
        if (wid) {
            if (GNW_win_init_flag) {
                Image image = Image(ulx, uly, width, height);

                WindowInfo window;

                window.id = wid;
                window.buffer = win_get_buf(wid);
                window.width = win_width(wid);

                image.Copy(&window);
                win_delete_button(bid);
                image.Write(&window);
            }

        } else {
            win_delete_button(bid);
        }
    }

    delete up;
    delete down;
    delete up_disabled;
    delete down_disabled;
}

void Button::Allocate() {
    if (up) {
        up->Allocate();
    }

    if (down) {
        down->Allocate();
    }

    if (up_disabled) {
        up_disabled->Allocate();
    }

    if (down_disabled) {
        down_disabled->Allocate();
    }
}

Rect Button::GetBounds() const { return {ulx, uly, width + ulx, height + uly}; }

uint8_t *Button::GetUpData() const {
    uint8_t *buffer;

    if (up) {
        buffer = up->GetData();
    } else {
        buffer = nullptr;
    }

    return buffer;
}

uint8_t *Button::GetDownData() const {
    uint8_t *buffer;

    if (down) {
        buffer = down->GetData();
    } else {
        buffer = nullptr;
    }

    return buffer;
}

uint8_t *Button::GetUpDisabledData() const {
    uint8_t *buffer;

    if (up_disabled) {
        buffer = up_disabled->GetData();
    } else {
        buffer = GetUpData();
    }

    return buffer;
}

uint8_t *Button::GetDownDisabledData() const {
    uint8_t *buffer;

    if (down_disabled) {
        buffer = down_disabled->GetData();
    } else {
        buffer = GetDownData();
    }

    return buffer;
}

void Button::Copy(WinID wid) {
    Image image = Image(ulx, uly, width, height);

    WindowInfo window;

    window.id = wid;
    window.buffer = win_get_buf(wid);
    window.width = win_width(wid);

    image.Copy(&window);

    if (up) {
        Image *up_image = new (std::nothrow) Image(ulx, uly, width, height);
        up_image->Copy(image);
        up_image->Blend(*up);
        delete up;
        up = up_image;
    }

    if (down) {
        Image *down_image = new (std::nothrow) Image(ulx, uly, width, height);
        down_image->Copy(image);
        down_image->Blend(*down);
        delete down;
        down = down_image;
    }

    if (up_disabled) {
        Image *up_disabled_image = new (std::nothrow) Image(ulx, uly, width, height);
        up_disabled_image->Copy(image);
        up_disabled_image->Blend(*up_disabled);
        delete up_disabled;
        up_disabled = up_disabled_image;
    }

    if (down_disabled) {
        Image *down_disabled_image = new (std::nothrow) Image(ulx, uly, width, height);
        down_disabled_image->Copy(image);
        down_disabled_image->Blend(*down_disabled);
        delete down_disabled;
        down_disabled = down_disabled_image;
    }
}

void Button::Copy(ResourceID id, int32_t ulx, int32_t uly) {
    WindowInfo window;

    Allocate();

    window.width = width;
    window.window.ulx = 0;
    window.window.uly = 0;
    window.window.lrx = width;
    window.window.lry = height;

    window.buffer = up->GetData();
    WindowManager_LoadSimpleImage(id, ulx, uly, 1, &window);

    window.buffer = down->GetData();
    WindowManager_LoadSimpleImage(id, ulx, uly + 1, 1, &window);
}

void Button::CopyDisabled(WindowInfo *w) {
    delete up_disabled;
    delete down_disabled;

    up_disabled = new (std::nothrow) Image(ulx, uly, width, height);
    down_disabled = new (std::nothrow) Image(ulx, uly, width, height);

    up_disabled->Copy(w);
    down_disabled->Copy(*up_disabled);
}

void Button::CopyUp(ResourceID id) {
    delete up;
    up = new (std::nothrow) Image(id, ulx, uly);
}

void Button::CopyDown(ResourceID id) {
    delete down;
    down = new (std::nothrow) Image(id, ulx, uly);
}

void Button::CopyUpDisabled(ResourceID id) {
    delete up_disabled;
    up_disabled = new (std::nothrow) Image(id, ulx, uly);
}

void Button::CopyDownDisabled(ResourceID id) {
    delete down_disabled;
    down_disabled = new (std::nothrow) Image(id, ulx, uly);
}

void Button::CopyUp(uint8_t *buffer) {
    delete up;
    up = new (std::nothrow) Image(ulx, uly, width, height);
    buf_to_buf(buffer, width, height, width, up->GetData(), width);
}

void Button::CopyDown(uint8_t *buffer) {
    delete down;
    down = new (std::nothrow) Image(ulx, uly, width, height);
    buf_to_buf(buffer, width, height, width, down->GetData(), width);
}

void Button::CopyUpDisabled(uint8_t *buffer) {
    delete up_disabled;
    up_disabled = new (std::nothrow) Image(ulx, uly, width, height);
    buf_to_buf(buffer, width, height, width, up_disabled->GetData(), width);
}

void Button::CopyDownDisabled(uint8_t *buffer) {
    delete down_disabled;
    down_disabled = new (std::nothrow) Image(ulx, uly, width, height);
    buf_to_buf(buffer, width, height, width, down_disabled->GetData(), width);
}

void Button::SetPFunc(ButtonFunc p_func, intptr_t p_value) {
    this->p_func = p_func;
    this->p_value = p_value;
}

void Button::SetRFunc(ButtonFunc r_func, intptr_t r_value) {
    this->r_func = r_func;
    this->r_value = r_value;
}

void Button::RegisterButton(WinID wid) {
    uint8_t *up_data;
    uint8_t *down_data;
    uint32_t flags;
    intptr_t r_value;
    intptr_t p_value;

    if (down && ((down->GetWidth() != width) || (down->GetHeight() != height))) {
        Image *image;
        WindowInfo w;

        w.id = wid;
        w.buffer = win_get_buf(wid);
        w.width = win_width(wid);

        image = new (std::nothrow) Image(ulx, uly, width, height);
        image->Copy(&w);
        image->Copy(*down);
        delete down;
        down = image;
    }

    flags = this->flags;

    if (enable) {
        up_data = GetUpData();
        down_data = GetDownData();
    } else {
        up_data = GetUpDisabledData();
        down_data = GetDownDisabledData();
        this->flags |= 8;
    }

    p_value = this->p_value;
    r_value = this->r_value;

    if (this->p_func || this->r_func) {
        p_value = reinterpret_cast<intptr_t>(this);
        r_value = reinterpret_cast<intptr_t>(this);
    }

    bid =
        win_register_button(wid, ulx, uly, width, height, -1, -1, p_value, r_value, up_data, down_data, nullptr, flags);

    if (this->p_func || this->r_func) {
        win_register_button_func(bid, nullptr, nullptr, reinterpret_cast<ButtonFunc>(Button_PFunc),
                                 reinterpret_cast<ButtonFunc>(Button_RFunc));
    }

    this->wid = wid;
}

void Button::Enable(bool enable, bool redraw) {
    if (enable) {
        if (!this->enable) {
            if (bid) {
                win_enable_button(bid);
                win_register_button_image(bid, GetUpData(), GetDownData(), nullptr, redraw);
            }
            this->enable = true;
        }
    } else {
        Disable();
    }
}

void Button::Disable(bool redraw) {
    if (enable) {
        if (bid) {
            win_register_button_image(bid, GetUpDisabledData(), GetDownDisabledData(), nullptr, redraw);
            win_disable_button(bid);
        }

        enable = false;
    }
}

void Button::SetRestState(bool rest_state) {
    if (bid) {
        win_set_button_rest_state(bid, rest_state, 0);

        this->rest_state = rest_state;
    }
}

void Button::PlaySound() const { SoundManager_PlaySfx(sfx); }

void Button::SetSfx(ResourceID id) { sfx = id; }

ButtonID Button::GetId() const { return bid; }

void Button::SetPValue(intptr_t p_value) { this->p_value = p_value; }

void Button::SetRValue(intptr_t r_value) { this->r_value = r_value; }

void Button::SetFlags(uint32_t flags) { this->flags = flags; }

void Button::SetCaption(const char *caption, int16_t x, int16_t y, FontColor color_up, FontColor color_down,
                        FontColor color_up_disabled, FontColor color_down_disabled) {
    Rect bounds = {0, 0, width - x, height - y};

    SetCaption(caption, &bounds, color_up, color_down, color_up_disabled, color_down_disabled);
}

void Button::SetCaption(const char *caption, Rect *r, FontColor color_up, FontColor color_down,
                        FontColor color_up_disabled, FontColor color_down_disabled) {
    WindowInfo window;
    int32_t width;
    int32_t height;
    int32_t uly_caption;

    Allocate();

    width = r->lrx - r->ulx;
    height = r->lry - r->uly;

    window.buffer = up->GetData();
    window.width = up->GetWidth();
    uly_caption = ((height - Text_GetHeight()) / 2) + r->uly;
    Text_TextLine(&window, caption, r->ulx, uly_caption, width, true, color_up);

    window.buffer = down->GetData();
    Text_TextLine(&window, caption, r->ulx, uly_caption, width, true, color_down);

    if (up_disabled) {
        window.buffer = up_disabled->GetData();
        window.width = up_disabled->GetWidth();
        Text_TextLine(&window, caption, r->ulx, uly_caption, width, true, color_up_disabled);
    }

    if (down_disabled) {
        window.buffer = down_disabled->GetData();
        window.width = down_disabled->GetWidth();
        Text_TextLine(&window, caption, r->ulx, uly_caption, width, true, color_down_disabled);
    }
}
