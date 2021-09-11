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

extern "C" {
#include "game.h"
}

#include "gwindow.h"

#define GWINDOW_COUNT 59

#define WINDOW_RECT(ulx, uly, lrx, lry) \
    { ulx, uly, lrx, lry }
#define WINDOW_ITEM(rect, unknown, id, buffer) \
    { rect, unknown, id, buffer }

static void gwin_swap_system_palette(ImageHeader *image);

static char *empty_string = (char *)"\0";

static WindowInfo windows[GWINDOW_COUNT] = {WINDOW_ITEM(WINDOW_RECT(0, 0, 640, 480), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(-1, -1, -1, -1), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(-1, -1, -1, -1), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(390, 2, 461, 22), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(17, 4, 83, 21), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(86, 4, 152, 21), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(10, 29, 137, 156), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(146, 123, 164, 140), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(146, 143, 164, 161), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(10, 173, 162, 220), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(10, 173, 162, 184), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(10, 185, 162, 196), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(10, 197, 162, 208), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(10, 209, 162, 220), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(4, 227, 23, 249), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(32, 227, 51, 248), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(60, 228, 97, 249), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(99, 228, 124, 249), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(125, 228, 162, 249), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(21, 250, 45, 273), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(51, 252, 98, 270), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(102, 252, 149, 270), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(2, 275, 19, 288), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(20, 275, 149, 288), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(150, 275, 169, 288), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(20, 275, 149, 293), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(4, 298, 58, 313), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(4, 314, 58, 329), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(4, 330, 58, 347), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(59, 298, 113, 313), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(59, 314, 113, 329), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(59, 330, 113, 347), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(114, 298, 168, 313), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(114, 314, 168, 329), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(114, 330, 168, 347), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(15, 356, 126, 467), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(136, 387, 162, 412), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(136, 413, 162, 439), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(180, 18, 627, 465), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(50, 0, 590, 8), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(590, 0, 639, 8), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(632, 0, 639, 50), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(632, 50, 639, 430), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(632, 430, 639, 479), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(590, 472, 639, 479), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(50, 472, 590, 479), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(0, 472, 50, 479), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(0, 430, 8, 479), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(0, 50, 8, 430), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(0, 0, 8, 50), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(0, 0, 50, 8), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(242, 456, 331, 476), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(338, 456, 559, 476), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(468, 2, 528, 22), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(534, 2, 594, 22), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(384, 0, 639, 22), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(240, 456, 585, 479), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(0, 0, 178, 238), 640, 0, NULL),
                                        WINDOW_ITEM(WINDOW_RECT(0, 239, 178, 479), 640, 0, NULL)};

unsigned char gwin_init(void) {
    unsigned char result;
    WinID wid;

    db_init(NULL, NULL, empty_string);

    win_init(init_mode_640_480, reset_mode, 0);

    wid = win_add(0, 0, 640, 480, 0, 2);

    if (wid == -1) {
        result = EXIT_CODE_INSUFFICIENT_MEMORY;
    } else {
        windows[0].id = wid;
        windows[0].buffer = win_get_buf(wid);

        if (windows[0].buffer) {
            for (int i = 0; i < GWINDOW_COUNT; i++) {
                windows[i].id = wid;
                windows[i].buffer = &windows[0].buffer[640 * windows[i].window.uly + windows[i].window.ulx];
            }

            system_palette_ptr = (unsigned char *)malloc(3 * PALETTE_SIZE);
            memset(system_palette_ptr, 0, 3 * PALETTE_SIZE);

            result = EXIT_CODE_NO_ERROR;
        } else {
            result = EXIT_CODE_INSUFFICIENT_MEMORY;
        }
    }

    return result;
}

WindowInfo *gwin_get_window(unsigned char id) {
    WindowInfo *win;

    if (id < GWINDOW_COUNT) {
        win = &windows[id];
    } else {
        win = NULL;
    }

    return win;
}

void gwin_clear_window(void) {
    WindowInfo *win;

    gwin_fade_in(100);
    win = gwin_get_window(0);
    setSystemPalette(system_palette_ptr);
    memset(win->buffer, 0, win->window.lry * win->window.lrx);
    win_draw(win->id);
}

void gwin_fade_in(int steps) { fadeSystemPalette(getColorPalette(), system_palette_ptr, steps); }

void gwin_fade_out(int steps) { fadeSystemPalette(system_palette_ptr, getColorPalette(), steps); }

void gwin_swap_system_palette(ImageHeader *image) {
    unsigned char *palette;

    gwin_clear_window();
    palette = getSystemPalette();

    for (int i = 0; i < (3 * PALETTE_SIZE); i++) {
        palette[i] = image->palette[i] / 4;
    }

    setSystemPalette(palette);

    color_palette = getColorPalette();
    memcpy(color_palette, palette, 3 * PALETTE_SIZE);
    setColorPalette(color_palette);
}

void gwin_load_palette_from_resource(GAME_RESOURCE id) {
    ImageHeader *image;

    image = (ImageHeader *)read_game_resource(id);
    if (image) {
        gwin_swap_system_palette(image);
        free(image);
    }
}

void gwin_decode_image(ImageHeader *image, unsigned char *buffer, int width, int height, int ulx) {
    int image_height;
    int image_width;
    unsigned char *image_data;
    int buffer_position;

    image_width = image->width;
    image_height = image->height;

    buffer_position = width + ulx * height;
    image_data = image->data;

    process_bk();

    for (int line_count = 0; line_count < image_height; line_count++) {
        short opt_word;

        for (int line_position = 0; line_position < image_width; line_position += opt_word) {
            opt_word = *(short *)image_data;
            image_data += sizeof(opt_word);

            if (opt_word > 0) {
                memcpy(&buffer[buffer_position + line_position], image_data, opt_word);

                image_data += opt_word;
            } else {
                opt_word = -opt_word;

                memset(&buffer[buffer_position + line_position], image_data[0], opt_word);

                image_data += sizeof(unsigned char);
            }
        }

        buffer_position += ulx;

        if ((line_count % 32) == 0) {
            process_bk();
        }
    }
}

int gwin_load_image(GAME_RESOURCE id, WindowInfo *wid, short offx, short palette_from_image, int draw_to_screen,
                    int width_from_image, int height_from_image) {
    ImageHeader *image;

    process_bk();

    image = (ImageHeader *)read_game_resource(id);

    if (!image) {
        return 0;
    }

    if (palette_from_image) {
        gwin_swap_system_palette(image);
    }

    if (width_from_image == -1) {
        width_from_image = image->field_0;
    }

    if (height_from_image == -1) {
        height_from_image = image->field_2;
    }

    gwin_decode_image(image, wid->buffer, width_from_image, height_from_image, offx);

    if (draw_to_screen) {
        win_draw(wid->id);
    }

    free(image);

    return 1;
}

void gwin_decode_image2(ImageHeader2 *image, int ulx, int uly, int has_transparency, WindowInfo *w) {
    int height;
    int width;
    unsigned char *buffer;
    unsigned char *image_data;
    int length;

    if (image) {
        if (w == NULL) {
            w = gwin_get_window(0);
        }

        width = w->window.lrx - w->window.ulx + 1;
        height = w->window.lry - w->window.uly + 1;

        if (width > 640) {
            width = 640;
        }

        if (height > 480) {
            height = 480;
        }

        length = image->field_0;

        word_173658 = image->field_2;

        ulx -= image->width;
        uly -= image->height;

        buffer = &w->buffer[ulx + uly * w->unknown];
        image_data = image->data;

        if (ulx < 0) {
            image_data -= ulx;
            buffer -= ulx;
            length += ulx;
        }

        if (image->field_0 + ulx >= width) {
            length -= image->field_0 + ulx - width;
        }

        for (int i = 0; (i < word_173658) && (uly < height); i++) {
            if (uly >= 0) {
                if (has_transparency) {
                    for (int j = 0; j < length; j++) {
                        if (image_data[j] != image->data[0]) {
                            buffer[j] = image_data[j];
                        }
                    }
                } else {
                    memcpy(buffer, image_data, length);
                }
            }

            image_data += image->field_0;
            buffer += w->unknown;
            uly++;
        }
    }
}

void gwin_load_image2(GAME_RESOURCE id, int ulx, int uly, int has_transparency, WindowInfo *w) {
    gwin_decode_image2((ImageHeader2 *)load_game_resource(id), ulx, uly, has_transparency, w);
}
