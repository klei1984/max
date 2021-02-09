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

#include "gbutton.h"

#include <new>

#include "SDL_assert.h"
#include "gwindow.h"

static_assert(sizeof(GButton) == 56, "The structure needs to be packed.");

static void gbutton_get_button_rect(GButton *b, Rect *r);
static unsigned char *gbutton_get_up_image(GButton *b);
static unsigned char *gbutton_get_down_image(GButton *b);
static unsigned char *gbutton_get_up_disabled_image(GButton *b);
static unsigned char *gbutton_get_down_disabled_image(GButton *b);
static void gbutton_p_func(ButtonID bid, GButton *b);
static void gbutton_r_func(ButtonID bid, GButton *b);

void gbutton_init(GButton *b) {
    b->bid = 0;

    b->up = NULL;
    b->down = NULL;
    b->up_disabled = NULL;
    b->down_disabled = NULL;

    b->p_value = -1;
    b->r_value = -1;

    b->flags = 0;

    b->p_func = NULL;
    b->r_func = NULL;

    b->wid = 0;
    b->sfx = -1;
    b->rest_state = 0;
}

GButton *gbutton_init_rect(GButton *b, int ulx, int uly, int lrx, int lry) {
    gbutton_init(b);

    b->ulx = ulx;
    b->uly = uly;
    b->lrx = lrx;
    b->lry = lry;

    b->enable = 1;
    b->sfx = -1;

    return b;
}

GButton *gbutton_init_texture(GButton *b, GAME_RESOURCE up, GAME_RESOURCE down, int ulx, int uly) {
    gbutton_init(b);

    GImage *image_up = new (std::nothrow) GImage();
    if (image_up) {
        image_up = gimage_init(image_up, up, ulx, uly);
    } else {
        image_up = NULL;
    }

    b->up = image_up;

    b->ulx = gimage_get_ulx(b->up);
    b->uly = gimage_get_uly(b->up);
    b->lrx = gimage_get_width(b->up);
    b->lry = gimage_get_height(b->up);

    GImage *image_down = new (std::nothrow) GImage();
    if (image_down) {
        image_down = gimage_init(image_down, down, b->ulx, b->uly);
    } else {
        image_down = NULL;
    }

    b->down = image_down;
    b->enable = 1;

    return b;
}

GButton *gbutton_delete(GButton *b) {
    if (b->bid) {
        if (b->wid) {
            GImage *image = new (std::nothrow) GImage();

            SDL_assert(image);

            gimage_alloc_ex(image, b->ulx, b->uly, b->lrx, b->lry);

            Window window;

            window.id = b->wid;
            window.buffer = win_get_buf(b->wid);
            window.unknown = win_width(b->wid);

            gimage_copy_from_window(image, &window);
            win_delete_button(b->bid);
            gimage_copy_to_window(image, &window);

            delete gimage_delete(image);
        } else {
            win_delete_button(b->bid);
        }
    }

    if (b->up) {
        delete gimage_delete(b->up);
        b->up = NULL;
    }

    if (b->down) {
        delete gimage_delete(b->down);
        b->down = NULL;
    }

    if (b->up_disabled) {
        delete gimage_delete(b->up_disabled);
        b->up_disabled = NULL;
    }

    if (b->down_disabled) {
        delete gimage_delete(b->down_disabled);
        b->down_disabled = NULL;
    }

    return b;
}

void gbutton_alloc(GButton *b) {
    if (b->up) {
        gimage_alloc(b->up);
    }

    if (b->down) {
        gimage_alloc(b->down);
    }

    if (b->up_disabled) {
        gimage_alloc(b->up_disabled);
    }

    if (b->down_disabled) {
        gimage_alloc(b->down_disabled);
    }
}

void gbutton_get_button_rect(GButton *b, Rect *r) {
    r->ulx = b->ulx;
    r->uly = b->uly;
    r->lrx = b->lrx + b->ulx;
    r->lry = b->lry + b->uly;
}

unsigned char *gbutton_get_up_image(GButton *b) {
    unsigned char *buffer;

    if (b->up) {
        buffer = gimage_get_buffer(b->up);
    } else {
        buffer = NULL;
    }

    return buffer;
}

unsigned char *gbutton_get_down_image(GButton *b) {
    unsigned char *buffer;

    if (b->down) {
        buffer = gimage_get_buffer(b->down);
    } else {
        buffer = NULL;
    }

    return buffer;
}

unsigned char *gbutton_get_up_disabled_image(GButton *b) {
    unsigned char *buffer;

    if (b->up_disabled) {
        buffer = gimage_get_buffer(b->up_disabled);
    } else {
        buffer = gbutton_get_up_image(b);
    }

    return buffer;
}

unsigned char *gbutton_get_down_disabled_image(GButton *b) {
    unsigned char *buffer;

    if (b->down_disabled) {
        buffer = gimage_get_buffer(b->down_disabled);
    } else {
        buffer = gbutton_get_down_image(b);
    }

    return buffer;
}

void gbutton_copy_from_window(GButton *b, WinID wid) {
    Window window;
    GImage *source_image;

    window.id = wid;
    window.buffer = win_get_buf(wid);
    window.unknown = win_width(wid);

    source_image = new (std::nothrow) GImage();
    SDL_assert(source_image);

    gimage_alloc_ex(source_image, b->ulx, b->uly, b->lrx, b->lry);
    gimage_copy_from_window(source_image, &window);

    if (b->up) {
        GImage *up_image;

        up_image = new (std::nothrow) GImage();
        SDL_assert(up_image);

        gimage_alloc_ex(up_image, b->ulx, b->uly, b->lrx, b->lry);
        gimage_copy_from_image(up_image, source_image);
        gimage_copy_content(up_image, b->up);

        if (b->up) {
            delete gimage_delete(b->up);
        }

        b->up = up_image;
    }

    if (b->down) {
        GImage *down_image;

        down_image = new (std::nothrow) GImage();
        SDL_assert(down_image);

        gimage_alloc_ex(down_image, b->ulx, b->uly, b->lrx, b->lry);
        gimage_copy_from_image(down_image, source_image);
        gimage_copy_content(down_image, b->down);

        if (b->down) {
            delete gimage_delete(b->down);
        }

        b->down = down_image;
    }

    if (b->up_disabled) {
        GImage *up_disabled_image;

        up_disabled_image = new (std::nothrow) GImage();
        SDL_assert(up_disabled_image);

        gimage_alloc_ex(up_disabled_image, b->ulx, b->uly, b->lrx, b->lry);
        gimage_copy_from_image(up_disabled_image, source_image);
        gimage_copy_content(up_disabled_image, b->up_disabled);

        if (b->up_disabled) {
            delete gimage_delete(b->up_disabled);
        }

        b->up_disabled = up_disabled_image;
    }

    if (b->down_disabled) {
        GImage *down_disabled_image;

        down_disabled_image = new (std::nothrow) GImage();
        SDL_assert(down_disabled_image);

        gimage_alloc_ex(down_disabled_image, b->ulx, b->uly, b->lrx, b->lry);
        gimage_copy_from_image(down_disabled_image, source_image);
        gimage_copy_content(down_disabled_image, b->down_disabled);

        if (b->down_disabled) {
            delete gimage_delete(b->down_disabled);
        }

        b->down_disabled = down_disabled_image;
    }

    delete gimage_delete(source_image);
}

void gbutton_copy_from_resource(GButton *b, GAME_RESOURCE id, int ulx, int uly) {
    Window window;

    gbutton_alloc(b);

    window.unknown = b->lrx;
    window.window.ulx = 0;
    window.window.uly = 0;
    window.window.lrx = b->lrx;
    window.window.lry = b->lry;

    window.buffer = gimage_get_buffer(b->up);
    gwin_load_image2(id, ulx, uly, 1, &window);

    window.buffer = gimage_get_buffer(b->down);
    gwin_load_image2(id, ulx, uly + 1, 1, &window);
}

void gbutton_copy_disabled_from_window(GButton *b, Window *w) {
    GImage *up_disabled_image;
    GImage *down_disabled_image;

    if (b->up_disabled) {
        delete gimage_delete(b->up_disabled);
    }

    if (b->down_disabled) {
        delete gimage_delete(b->down_disabled);
    }

    up_disabled_image = new (std::nothrow) GImage();
    if (up_disabled_image) {
        gimage_alloc_ex(up_disabled_image, b->ulx, b->uly, b->lrx, b->lry);
    }

    b->up_disabled = up_disabled_image;

    down_disabled_image = new (std::nothrow) GImage();
    if (down_disabled_image) {
        down_disabled_image = gimage_alloc_ex(down_disabled_image, b->ulx, b->uly, b->lrx, b->lry);
    }

    b->down_disabled = down_disabled_image;

    gimage_copy_from_window(b->up_disabled, w);
    gimage_copy_from_image(b->down_disabled, b->up_disabled);
}

void gbutton_copy_up_from_resource(GButton *b, GAME_RESOURCE id) {
    GImage *image;

    if (b->up) {
        delete gimage_delete(b->up);
    }

    image = new (std::nothrow) GImage();
    if (image) {
        gimage_init(image, id, b->ulx, b->uly);
    }

    b->up = image;
}

void gbutton_copy_down_from_resource(GButton *b, GAME_RESOURCE id) {
    GImage *image;

    if (b->down) {
        delete gimage_delete(b->down);
    }

    image = new (std::nothrow) GImage();
    if (image) {
        gimage_init(image, id, b->ulx, b->uly);
    }

    b->down = image;
}

void gbutton_copy_up_disabled_from_resource(GButton *b, GAME_RESOURCE id) {
    GImage *image;

    if (b->up_disabled) {
        delete gimage_delete(b->up_disabled);
    }

    image = new (std::nothrow) GImage();
    if (image) {
        gimage_init(image, id, b->ulx, b->uly);
    }

    b->up_disabled = image;
}

void gbutton_copy_down_disabled_from_resource(GButton *b, GAME_RESOURCE id) {
    GImage *image;

    if (b->down_disabled) {
        delete gimage_delete(b->down_disabled);
    }

    image = new (std::nothrow) GImage();
    if (image) {
        gimage_init(image, id, b->ulx, b->uly);
    }

    b->down_disabled = image;
}

void gbutton_copy_up_from_buffer(GButton *b, unsigned char *buffer) {
    GImage *image;

    if (b->up) {
        delete gimage_delete(b->up);
    }

    image = new (std::nothrow) GImage();
    if (image) {
        gimage_alloc_ex(image, b->ulx, b->uly, b->lrx, b->lry);
    }

    b->up = image;

    buf_to_buf(buffer, b->lrx, b->lry, b->lrx, gimage_get_buffer(b->up), b->lrx);
}

void gbutton_copy_down_from_buffer(GButton *b, unsigned char *buffer) {
    GImage *image;

    if (b->down) {
        delete gimage_delete(b->down);
    }

    image = new (std::nothrow) GImage();
    if (image) {
        gimage_alloc_ex(image, b->ulx, b->uly, b->lrx, b->lry);
    }

    b->down = image;

    buf_to_buf(buffer, b->lrx, b->lry, b->lrx, gimage_get_buffer(b->down), b->lrx);
}

void gbutton_copy_up_disabled_from_buffer(GButton *b, unsigned char *buffer) {
    GImage *image;

    if (b->up_disabled) {
        delete gimage_delete(b->up_disabled);
    }

    image = new (std::nothrow) GImage();
    if (image) {
        gimage_alloc_ex(image, b->ulx, b->uly, b->lrx, b->lry);
    }

    b->up_disabled = image;

    buf_to_buf(buffer, b->lrx, b->lry, b->lrx, gimage_get_buffer(b->up_disabled), b->lrx);
}

void gbutton_copy_down_disabled_from_buffer(GButton *b, unsigned char *buffer) {
    GImage *image;

    if (b->down_disabled) {
        delete gimage_delete(b->down_disabled);
    }

    image = new (std::nothrow) GImage();
    if (image) {
        gimage_alloc_ex(image, b->ulx, b->uly, b->lrx, b->lry);
    }

    b->down_disabled = image;

    buf_to_buf(buffer, b->lrx, b->lry, b->lrx, gimage_get_buffer(b->down_disabled), b->lrx);
}

void gbutton_set_p_func(GButton *b, ButtonFunc p_func, int p_value) {
    b->p_func = p_func;
    b->p_value = p_value;
}

void gbutton_set_r_func(GButton *b, ButtonFunc r_func, int r_value) {
    b->r_func = r_func;
    b->r_value = r_value;
}

void gbutton_register_button(GButton *b, WinID wid) {
    Window w;
    GImage *image;
    unsigned char *up;
    unsigned char *down;
    int flags;
    int r_value;
    int p_value;

    if (b->down && ((gimage_get_width(b->down) != b->lrx) || (gimage_get_height(b->down) != b->lry))) {
        w.id = wid;
        w.buffer = win_get_buf(wid);
        w.unknown = win_width(wid);

        image = new (std::nothrow) GImage();
        if (image) {
            gimage_alloc_ex(image, b->ulx, b->uly, b->lrx, b->lry);
        }

        gimage_copy_from_window(image, &w);
        gimage_copy_from_image(image, b->down);

        if (b->down) {
            delete gimage_delete(b->down);
        }

        b->down = image;
    }

    flags = b->flags;

    if (b->enable) {
        up = gbutton_get_up_image(b);
        down = gbutton_get_down_image(b);
    } else {
        up = gbutton_get_up_disabled_image(b);
        down = gbutton_get_down_disabled_image(b);
        b->flags |= 8u;
    }

    p_value = b->p_value;
    r_value = b->r_value;

    if (b->p_func || b->r_func) {
        p_value = (int)b;
        r_value = (int)b;
    }

    b->bid = win_register_button(wid, b->ulx, b->uly, b->lrx, b->lry, -1, -1, p_value, r_value, (char *)up,
                                 (char *)down, NULL, flags);

    if (b->p_func || b->r_func) {
        win_register_button_func(b->bid, NULL, NULL, (ButtonFunc)gbutton_p_func, (ButtonFunc)gbutton_r_func);
    }

    b->wid = wid;
}

void gbutton_enable(GButton *b, char enable, char redraw) {
    if (enable) {
        if (!b->enable) {
            if (b->bid) {
                win_enable_button(b->bid);
                win_register_button_image(b->bid, (char *)gbutton_get_up_image(b), (char *)gbutton_get_down_image(b),
                                          NULL, redraw);
            }
            b->enable = 1;
        }
    } else {
        gbutton_disable(b, 1);
    }
}

void gbutton_disable(GButton *b, char redraw) {
    if (b->enable) {
        if (b->bid) {
            win_register_button_image(b->bid, (char *)gbutton_get_up_disabled_image(b),
                                      (char *)gbutton_get_down_disabled_image(b), NULL, redraw);
            win_disable_button(b->bid);
        }

        b->enable = 0;
    }
}

void gbutton_set_rest_state(GButton *b, char rest_state) {
    if (b->bid) {
        win_set_button_rest_state(b->bid, rest_state, 0);

        b->rest_state = rest_state;
    }
}

void gbutton_play_sound(GButton *b) {
    /// \todo Soundmgr_play_sfx(b->sfx);
}

static inline void gbutton_watcall(ButtonFunc func, ButtonID id, int value) {
    __asm__ __volatile__("	call	*%2\n"
                         : /* out  */
                         : /* in   */ "a"(id), "d"(value), "b"(func)
                         : /* clob */);
}

void gbutton_p_func(ButtonID bid, GButton *b) {
    if (!b->rest_state) {
        b->rest_state = 1;

        if (b->sfx != -1) {
            gbutton_play_sound(b);
        }
    }

    if (b->p_func) {
        gbutton_watcall(b->p_func, bid, b->p_value);
        /// \todo b->p_func(bid, b->p_value);
    }
}

void gbutton_r_func(ButtonID bid, GButton *b) {
    b->rest_state = 0;

    if (b->r_func) {
        gbutton_watcall(b->r_func, bid, b->r_value);
        /// \todo b->r_func(bid, b->r_value);
    }
}

void gbutton_set_sfx(GButton *b, GAME_RESOURCE id) { b->sfx = id; }

ButtonID gbutton_get_button_id(GButton *b) { return b->bid; }

void gbutton_set_r_value(GButton *b, int r_value) { b->r_value = r_value; }

void gbutton_set_p_value(GButton *b, int p_value) { b->p_value = p_value; }

void gbutton_set_flags(GButton *b, int flags) { b->flags = flags; }
