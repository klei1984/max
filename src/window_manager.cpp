/* Copyright (c) 2022 M.A.X. Port Team
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

#include "window_manager.hpp"

#include "resource_manager.hpp"

#define WINDOW_RECT(ulx, uly, lrx, lry) \
    { ulx, uly, lrx, lry }

#define WINDOW_ITEM(rect, unknown, id, buffer) \
    { rect, unknown, id, buffer }

static void WindowManager_SwapSystemPalette(ImageBigHeader *image);

static char *empty_string = (char *)"\0";

Color *WindowManager_SystemPalette;
Color *WindowManager_ColorPalette;

static WindowInfo windows[WINDOW_COUNT] = {WINDOW_ITEM(WINDOW_RECT(0, 0, 640, 480), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(-1, -1, -1, -1), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(-1, -1, -1, -1), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(390, 2, 461, 22), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(17, 4, 83, 21), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(86, 4, 152, 21), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(10, 29, 137, 156), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(146, 123, 164, 140), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(146, 143, 164, 161), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(10, 173, 162, 220), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(10, 173, 162, 184), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(10, 185, 162, 196), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(10, 197, 162, 208), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(10, 209, 162, 220), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(4, 227, 23, 249), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(32, 227, 51, 248), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(60, 228, 97, 249), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(99, 228, 124, 249), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(125, 228, 162, 249), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(21, 250, 45, 273), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(51, 252, 98, 270), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(102, 252, 149, 270), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(2, 275, 19, 288), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(20, 275, 149, 288), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(150, 275, 169, 288), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(20, 275, 149, 293), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(4, 298, 58, 313), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(4, 314, 58, 329), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(4, 330, 58, 347), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(59, 298, 113, 313), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(59, 314, 113, 329), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(59, 330, 113, 347), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(114, 298, 168, 313), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(114, 314, 168, 329), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(114, 330, 168, 347), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(15, 356, 126, 467), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(136, 387, 162, 412), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(136, 413, 162, 439), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(180, 18, 627, 465), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(50, 0, 590, 8), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(590, 0, 639, 8), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(632, 0, 639, 50), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(632, 50, 639, 430), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(632, 430, 639, 479), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(590, 472, 639, 479), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(50, 472, 590, 479), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(0, 472, 50, 479), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(0, 430, 8, 479), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(0, 50, 8, 430), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(0, 0, 8, 50), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(0, 0, 50, 8), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(242, 456, 331, 476), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(338, 456, 559, 476), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(468, 2, 528, 22), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(534, 2, 594, 22), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(384, 0, 639, 22), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(240, 456, 585, 479), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(0, 0, 178, 238), 640, 0, nullptr),
                                           WINDOW_ITEM(WINDOW_RECT(0, 239, 178, 479), 640, 0, nullptr)};

int WindowManager_Init() {
    unsigned char result;
    WinID wid;

    db_init(nullptr, nullptr, empty_string);

    win_init(init_mode_640_480, reset_mode, 0);

    wid = win_add(0, 0, 640, 480, 0, 2);

    if (wid == -1) {
        result = EXIT_CODE_INSUFFICIENT_MEMORY;
    } else {
        windows[WINDOW_MAIN_WINDOW].id = wid;
        windows[WINDOW_MAIN_WINDOW].buffer = win_get_buf(wid);

        if (windows[0].buffer) {
            for (int i = 0; i < WINDOW_COUNT; i++) {
                windows[i].id = wid;
                windows[i].buffer =
                    &windows[WINDOW_MAIN_WINDOW].buffer[640 * windows[i].window.uly + windows[i].window.ulx];
            }

            WindowManager_SystemPalette = (unsigned char *)malloc(3 * PALETTE_SIZE);
            memset(WindowManager_SystemPalette, 0, 3 * PALETTE_SIZE);

            result = EXIT_CODE_NO_ERROR;
        } else {
            result = EXIT_CODE_INSUFFICIENT_MEMORY;
        }
    }

    return result;
}

WindowInfo *WindowManager_GetWindow(unsigned char id) {
    WindowInfo *window;

    if (id < WINDOW_COUNT) {
        window = &windows[id];
    } else {
        window = nullptr;
    }

    return window;
}

void WindowManager_ClearWindow() {
    WindowInfo *window;

    WindowManager_FadeOut(100);
    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    setSystemPalette(WindowManager_SystemPalette);
    memset(window->buffer, 0, window->window.lry * window->window.lrx);
    win_draw(window->id);
}

void WindowManager_FadeOut(int steps) { fadeSystemPalette(getColorPalette(), WindowManager_SystemPalette, steps); }

void WindowManager_FadeIn(int steps) { fadeSystemPalette(WindowManager_SystemPalette, getColorPalette(), steps); }

void WindowManager_SwapSystemPalette(ImageBigHeader *image) {
    unsigned char *palette;

    WindowManager_ClearWindow();
    palette = getSystemPalette();

    for (int i = 0; i < (3 * PALETTE_SIZE); i++) {
        palette[i] = image->palette[i] / 4;
    }

    setSystemPalette(palette);

    WindowManager_ColorPalette = getColorPalette();
    memcpy(WindowManager_ColorPalette, palette, 3 * PALETTE_SIZE);
    setColorPalette(WindowManager_ColorPalette);
}

void WindowManager_LoadPalette(ResourceID id) {
    ImageBigHeader *image;

    image = (ImageBigHeader *)ResourceManager_ReadResource(id);
    if (image) {
        WindowManager_SwapSystemPalette(image);
    }

    delete[] image;
}

void WindowManager_DecodeImage(struct ImageBigHeader *image, unsigned char *buffer, int width, int height, int ulx) {
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

int WindowManager_LoadImage(ResourceID id, WindowInfo *window, short offx, int palette_from_image, int draw_to_screen,
                            int width_from_image, int height_from_image) {
    ImageBigHeader *image;

    process_bk();

    image = (ImageBigHeader *)ResourceManager_ReadResource(id);

    if (!image) {
        return 0;
    }

    if (palette_from_image) {
        WindowManager_SwapSystemPalette(image);
    }

    if (width_from_image == -1) {
        width_from_image = image->ulx;
    }

    if (height_from_image == -1) {
        height_from_image = image->uly;
    }

    WindowManager_DecodeImage(image, window->buffer, width_from_image, height_from_image, offx);

    if (draw_to_screen) {
        win_draw(window->id);
    }

    delete[] image;

    return 1;
}

void WindowManager_DecodeImage2(struct ImageSimpleHeader *image, int ulx, int uly, int has_transparency,
                                WindowInfo *w) {
    int height;
    int width;
    unsigned char *buffer;
    unsigned char *image_data;
    int length;

    if (image) {
        if (w == nullptr) {
            w = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
        }

        width = w->window.lrx - w->window.ulx + 1;
        height = w->window.lry - w->window.uly + 1;

        if (width > 640) {
            width = 640;
        }

        if (height > 480) {
            height = 480;
        }

        length = image->ulx;

        ulx -= image->width;
        uly -= image->height;

        buffer = &w->buffer[ulx + uly * w->width];
        image_data = image->data;

        if (ulx < 0) {
            image_data -= ulx;
            buffer -= ulx;
            length += ulx;
        }

        if (image->ulx + ulx >= width) {
            length -= image->ulx + ulx - width;
        }

        for (int i = 0; (i < image->uly) && (uly < height); i++) {
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

            image_data += image->ulx;
            buffer += w->width;
            uly++;
        }
    }
}

void WindowManager_LoadImage2(ResourceID id, int ulx, int uly, int has_transparency, WindowInfo *w) {
    WindowManager_DecodeImage2(reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(id)), ulx, uly,
                               has_transparency, w);
}
