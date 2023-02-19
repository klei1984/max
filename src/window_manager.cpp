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

#include "gfx.hpp"
#include "resource_manager.hpp"

#define WINDOW_RECT(ulx, uly, lrx, lry) \
    { ulx, uly, lrx, lry }

#define WINDOW_ITEM(rect, unknown, id, buffer, name) \
    { rect, unknown, id, buffer }

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static void WindowManager_SwapSystemPalette(ImageBigHeader *image);
static void WindowManager_ScaleWindows(int id);

static char *empty_string = (char *)"\0";

Color *WindowManager_SystemPalette;
Color *WindowManager_ColorPalette;

int WindowManager_WindowWidth;
int WindowManager_WindowHeight;
int WindowManager_MapWidth;
int WindowManager_MapHeight;

static WindowInfo windows[WINDOW_COUNT] = {
    WINDOW_ITEM(WINDOW_RECT(0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1), WINDOW_WIDTH, 0, nullptr, WINDOW_MAIN_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(-1, -1, -1, -1), WINDOW_WIDTH, 0, nullptr, WINDOW_POPUP_BUTTONS),
    WINDOW_ITEM(WINDOW_RECT(-1, -1, -1, -1), WINDOW_WIDTH, 0, nullptr, WINDOW_MESSAGE_BOX),
    WINDOW_ITEM(WINDOW_RECT(390, 2, 461, 22), WINDOW_WIDTH, 0, nullptr, WINDOW_ENDTURN_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(17, 4, 83, 21), WINDOW_WIDTH, 0, nullptr, WINDOW_FILES_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(86, 4, 152, 21), WINDOW_WIDTH, 0, nullptr, WINDOW_PREFS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(10, 29, 137, 156), WINDOW_WIDTH, 0, nullptr, WINDOW_CORNER_FLIC),
    WINDOW_ITEM(WINDOW_RECT(146, 123, 164, 140), WINDOW_WIDTH, 0, nullptr, WINDOW_PLAY_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(146, 143, 164, 161), WINDOW_WIDTH, 0, nullptr, WINDOW_PAUSE_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(10, 173, 162, 220), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(10, 173, 162, 184), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_ROW_1),
    WINDOW_ITEM(WINDOW_RECT(10, 185, 162, 196), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_ROW_2),
    WINDOW_ITEM(WINDOW_RECT(10, 197, 162, 208), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_ROW_3),
    WINDOW_ITEM(WINDOW_RECT(10, 209, 162, 220), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_ROW_4),
    WINDOW_ITEM(WINDOW_RECT(4, 227, 23, 249), WINDOW_WIDTH, 0, nullptr, WINDOW_CENTER_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(32, 227, 51, 248), WINDOW_WIDTH, 0, nullptr, WINDOW_LOCK_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(60, 228, 97, 249), WINDOW_WIDTH, 0, nullptr, WINDOW_PRE_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(99, 228, 124, 249), WINDOW_WIDTH, 0, nullptr, WINDOW_DONE_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(125, 228, 162, 249), WINDOW_WIDTH, 0, nullptr, WINDOW_NXT_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(21, 250, 45, 273), WINDOW_WIDTH, 0, nullptr, WINDOW_HELP_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(51, 252, 98, 270), WINDOW_WIDTH, 0, nullptr, WINDOW_REPORTS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(102, 252, 149, 270), WINDOW_WIDTH, 0, nullptr, WINDOW_CHAT_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(2, 275, 19, 288), WINDOW_WIDTH, 0, nullptr, WINDOW_ZOOM_PLUS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(20, 275, 149, 288), WINDOW_WIDTH, 0, nullptr, WINDOW_ZOOM_SLIDER_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(150, 275, 169, 288), WINDOW_WIDTH, 0, nullptr, WINDOW_ZOOM_MINUS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(20, 275, 149, 293), WINDOW_WIDTH, 0, nullptr, WINDOW_ZOOM_SLIDER_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(4, 298, 58, 313), WINDOW_WIDTH, 0, nullptr, WINDOW_SURVEY_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(4, 314, 58, 329), WINDOW_WIDTH, 0, nullptr, WINDOW_STATUS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(4, 330, 58, 347), WINDOW_WIDTH, 0, nullptr, WINDOW_COLORS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(59, 298, 113, 313), WINDOW_WIDTH, 0, nullptr, WINDOW_HITS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(59, 314, 113, 329), WINDOW_WIDTH, 0, nullptr, WINDOW_AMMO_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(59, 330, 113, 347), WINDOW_WIDTH, 0, nullptr, WINDOW_RANGE_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(114, 298, 168, 313), WINDOW_WIDTH, 0, nullptr, WINDOW_SCAN_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(114, 314, 168, 329), WINDOW_WIDTH, 0, nullptr, WINDOW_GRID_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(114, 330, 168, 347), WINDOW_WIDTH, 0, nullptr, WINDOW_NAME_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(15, 356, 126, 467), WINDOW_WIDTH, 0, nullptr, WINDOW_MINIMAP),
    WINDOW_ITEM(WINDOW_RECT(136, 387, 162, 412), WINDOW_WIDTH, 0, nullptr, WINDOW_2X_MINIMAP),
    WINDOW_ITEM(WINDOW_RECT(136, 413, 162, 439), WINDOW_WIDTH, 0, nullptr, WINDOW_TNT_MINIMAP),
    WINDOW_ITEM(WINDOW_RECT(180, 18, 7 * 64 + 180 - 1, 7 * 64 + 18 - 1), WINDOW_WIDTH, 0, nullptr, WINDOW_MAIN_MAP),
    WINDOW_ITEM(WINDOW_RECT(50, 0, 590, 8), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_UP_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(590, 0, 639, 8), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_UP_RIGHT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(632, 0, 639, 50), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_RIGHT_UP_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(632, 50, 639, 430), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_RIGHT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(632, 430, 639, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_RIGHT_DOWN_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(590, 472, 639, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_DOWN_RIGHT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(50, 472, 590, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_DOWN_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 472, 50, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_DOWN_LEFT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 430, 8, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_LEFT_DOWN_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 50, 8, 430), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_LEFT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 0, 8, 50), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_LEFT_UP_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 0, 50, 8), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_UP_LEFT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(242, 456, 331, 476), WINDOW_WIDTH, 0, nullptr, WINDOW_COORDINATES_DISPLAY),
    WINDOW_ITEM(WINDOW_RECT(338, 456, 559, 476), WINDOW_WIDTH, 0, nullptr, WINDOW_UNIT_DESCRIPTION_DISPLAY),
    WINDOW_ITEM(WINDOW_RECT(468, 2, 528, 22), WINDOW_WIDTH, 0, nullptr, WINDOW_TURN_COUNTER_DISPLAY),
    WINDOW_ITEM(WINDOW_RECT(534, 2, 594, 22), WINDOW_WIDTH, 0, nullptr, WINDOW_TURN_TIMER_DISPLAY),
    WINDOW_ITEM(WINDOW_RECT(384, 0, 639, 22), WINDOW_WIDTH, 0, nullptr, WINDOW_TOP_INSTRUMENTS_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(240, 456, 585, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_BOTTOM_INSTRUMENTS_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 0, 178, 238), WINDOW_WIDTH, 0, nullptr, WINDOW_INTERFACE_PANEL_TOP),
    WINDOW_ITEM(WINDOW_RECT(0, 239, 178, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_INTERFACE_PANEL_BOTTOM)};

void WindowManager_ScaleWindows(int id) {
    WindowInfo *const screen = &windows[WINDOW_MAIN_WINDOW];
    WindowInfo *const window = &windows[id];

    switch (id) {
        case WINDOW_MAIN_MAP: {
            int screen_width = WindowManager_GetWidth(screen);
            int screen_height = WindowManager_GetHeight(screen);

            int window_left = window->window.ulx;
            int window_tile_count_x = (WINDOW_WIDTH - window_left) / GFX_MAP_TILE_SIZE;
            int window_right = WINDOW_WIDTH - window_left - window_tile_count_x * GFX_MAP_TILE_SIZE;
            int window_top = window->window.uly;
            int window_tile_count_y = (WINDOW_HEIGHT - window_top) / GFX_MAP_TILE_SIZE;
            int window_bottom = WINDOW_HEIGHT - window_top - window_tile_count_y * GFX_MAP_TILE_SIZE;

            double scale;

            if (screen_width >= screen_height) {
                scale = static_cast<double>(screen_height) / WINDOW_HEIGHT;

            } else {
                scale = static_cast<double>(screen_width) / WINDOW_WIDTH;
            }

            int frame_left = window_left * scale;
            int frame_tile_count_x = (screen_width - frame_left) / GFX_MAP_TILE_SIZE;
            int frame_right = screen_width - frame_left - frame_tile_count_x * GFX_MAP_TILE_SIZE;
            int frame_tile_count_y =
                (screen_height - static_cast<int>((window_top + window_bottom) * scale)) / GFX_MAP_TILE_SIZE;
            int frame_top = (screen_height - frame_tile_count_y * GFX_MAP_TILE_SIZE) / 2;
            int frame_bottom = screen_height - frame_top - frame_tile_count_y * GFX_MAP_TILE_SIZE;

            window->window.ulx = frame_left;
            window->window.uly = frame_top;
            window->window.lrx = frame_left + frame_tile_count_x * GFX_MAP_TILE_SIZE - 1;
            window->window.lry = frame_top + frame_tile_count_y * GFX_MAP_TILE_SIZE - 1;
            window->buffer = &screen->buffer[screen->width * frame_top + frame_left];

            WindowManager_MapWidth = frame_tile_count_x * GFX_MAP_TILE_SIZE;
            WindowManager_MapHeight = frame_tile_count_y * GFX_MAP_TILE_SIZE;

            SDL_assert(WindowManager_MapWidth + frame_left + frame_right == screen_width);
            SDL_assert(WindowManager_MapHeight + frame_top + frame_bottom == screen_height);
        } break;
    }

    windows[id].width = screen->width;
}

int WindowManager_Init() {
    unsigned char result;
    WinID wid;

    db_init(nullptr, nullptr, empty_string);

    win_init(Svga_Init, Svga_Deinit, 0);

    WindowManager_WindowWidth = Svga_GetScreenWidth();
    WindowManager_WindowHeight = Svga_GetScreenHeight();

    wid = win_add(0, 0, WindowManager_WindowWidth, WindowManager_WindowHeight, COLOR_BLACK, 2);

    if (wid == -1) {
        result = EXIT_CODE_INSUFFICIENT_MEMORY;

    } else {
        windows[WINDOW_MAIN_WINDOW].width = WindowManager_WindowWidth;
        windows[WINDOW_MAIN_WINDOW].window.lrx = WindowManager_WindowWidth - 1;
        windows[WINDOW_MAIN_WINDOW].window.lry = WindowManager_WindowHeight - 1;

        windows[WINDOW_MAIN_WINDOW].id = wid;
        windows[WINDOW_MAIN_WINDOW].buffer = win_get_buf(wid);

        if (windows[WINDOW_MAIN_WINDOW].buffer) {
            for (int i = 0; i < WINDOW_COUNT; ++i) {
                windows[i].id = wid;
                windows[i].buffer =
                    &windows[WINDOW_MAIN_WINDOW]
                         .buffer[windows[WINDOW_MAIN_WINDOW].width * windows[i].window.uly + windows[i].window.ulx];

                WindowManager_ScaleWindows(i);
            }

            WindowManager_SystemPalette = new (std::nothrow) unsigned char[3 * PALETTE_SIZE];
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
    memset(window->buffer, 0, (window->window.lry + 1) * (window->window.lrx + 1));
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
    unsigned char *resource;
    ImageBigHeader *image;

    resource = ResourceManager_ReadResource(id);
    image = reinterpret_cast<ImageBigHeader *>(resource);

    if (image) {
        WindowManager_SwapSystemPalette(image);
    }

    delete[] resource;
}

void WindowManager_DecodeImage(struct ImageBigHeader *image, unsigned char *buffer, int ulx, int uly, int pitch) {
    int image_height;
    int image_width;
    unsigned char *image_data;
    int buffer_position;

    image_width = image->width;
    image_height = image->height;

    buffer_position = ulx + pitch * uly;
    image_data = &image->transparent_color;

    process_bk();

    for (int line_count = 0; line_count < image_height; ++line_count) {
        short opt_word;

        for (int line_position = 0; line_position < image_width; line_position += opt_word) {
            opt_word = *reinterpret_cast<short *>(image_data);
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

        buffer_position += pitch;

        if ((line_count % 32) == 0) {
            process_bk();
        }
    }
}

int WindowManager_LoadImage(ResourceID id, WindowInfo *window, short pitch, int palette_from_image, int draw_to_screen,
                            int ulx, int uly) {
    unsigned char *resource;
    ImageBigHeader *image;

    process_bk();

    resource = ResourceManager_ReadResource(id);
    image = reinterpret_cast<ImageBigHeader *>(resource);

    if (!image) {
        return 0;
    }

    if (palette_from_image) {
        WindowManager_SwapSystemPalette(image);
    }

    if (ulx == -1) {
        ulx = (window->window.lrx - window->window.ulx - (image->ulx + image->width)) / 2;

        if (ulx < 0) {
            ulx = 0;
        }
    }

    if (uly == -1) {
        uly = (window->window.lry - window->window.uly - (image->uly + image->height)) / 2;

        if (uly < 0) {
            uly = 0;
        }
    }

    WindowManager_DecodeImage(image, window->buffer, ulx, uly, pitch);

    if (draw_to_screen) {
        win_draw(window->id);
    }

    delete[] resource;

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
        const int screen_width = WindowManager_GetWidth(&windows[WINDOW_MAIN_WINDOW]);
        const int screen_height = WindowManager_GetHeight(&windows[WINDOW_MAIN_WINDOW]);

        if (w == nullptr) {
            w = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
        }

        width = w->window.lrx - w->window.ulx + 1;
        height = w->window.lry - w->window.uly + 1;

        if (width > screen_width) {
            width = screen_width;
        }

        if (height > screen_height) {
            height = screen_height;
        }

        length = image->width;

        ulx -= image->ulx;
        uly -= image->uly;

        buffer = &w->buffer[ulx + uly * w->width];
        image_data = &image->transparent_color;

        if (ulx < 0) {
            image_data -= ulx;
            buffer -= ulx;
            length += ulx;
        }

        if (image->width + ulx >= width) {
            length -= image->width + ulx - width;
        }

        for (int i = 0; (i < image->height) && (uly < height); i++) {
            if (uly >= 0) {
                if (has_transparency) {
                    for (int j = 0; j < length; j++) {
                        if (image_data[j] != image->transparent_color) {
                            buffer[j] = image_data[j];
                        }
                    }
                } else {
                    memcpy(buffer, image_data, length);
                }
            }

            image_data += image->width;
            buffer += w->width;
            uly++;
        }
    }
}

void WindowManager_LoadImage2(ResourceID id, int ulx, int uly, int has_transparency, WindowInfo *w) {
    WindowManager_DecodeImage2(reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(id)), ulx, uly,
                               has_transparency, w);
}

struct ImageSimpleHeader *WindowManager_RescaleImage(struct ImageSimpleHeader *image, int scaling_factor) {
    int width;
    int height;
    int scaled_width;
    int scaled_height;
    int scaling_factor_width;
    int scaling_factor_height;
    struct ImageSimpleHeader *scaled_image;
    unsigned char *buffer;
    unsigned char *image_data;
    unsigned char *scaled_image_data;

    width = image->width;
    height = image->height;

    scaled_width = (width << 16) / scaling_factor;
    scaled_height = (height << 16) / scaling_factor;

    scaled_image = reinterpret_cast<struct ImageSimpleHeader *>(
        new (std::nothrow) unsigned char[scaled_width * scaled_height + sizeof(image->width) * 4]);

    scaled_image->width = scaled_width;
    scaled_image->height = scaled_height;
    scaled_image->ulx = (image->ulx << 16) / scaling_factor;
    scaled_image->uly = (image->uly << 16) / scaling_factor;

    scaling_factor_width = ((width - 1) << 16) / (scaled_width - 1) + 8;
    scaling_factor_height = ((height - 1) << 16) / (scaled_height - 1) + 8;

    image_data = &image->transparent_color;
    scaled_image_data = &scaled_image->transparent_color;

    for (int i = 0; i < scaled_height; ++i) {
        buffer = &image_data[((i * scaling_factor_height) >> 16) * width];

        for (int j = 0; j < scaled_width; ++j) {
            scaled_image_data[j + i * scaled_width] = buffer[(j * scaling_factor_width) >> 16];
        }
    }

    return scaled_image;
}

int WindowManager_GetWidth(WindowInfo *w) { return win_width(w->id); }

int WindowManager_GetHeight(WindowInfo *w) { return win_height(w->id); }

int WindowManager_ScaleUlx(WindowInfo *w, int ulx) {
    return (WindowManager_GetWidth(&windows[WINDOW_MAIN_WINDOW]) - WINDOW_WIDTH) / 2 + ulx;
}

int WindowManager_ScaleUly(WindowInfo *w, int uly) {
    return (WindowManager_GetHeight(&windows[WINDOW_MAIN_WINDOW]) - WINDOW_HEIGHT) / 2 + uly;
}

int WindowManager_ScaleLrx(WindowInfo *w, int ulx, int lrx) {
    int width = lrx - ulx;

    return WindowManager_ScaleUlx(w, ulx) + width;
}

int WindowManager_ScaleLry(WindowInfo *w, int uly, int lry) {
    int height = lry - uly;

    return WindowManager_ScaleUly(w, uly) + height;
}

int WindowManager_ScaleOffset(WindowInfo *w, int ulx, int uly) {
    return w->width * WindowManager_ScaleUly(w, uly) + WindowManager_ScaleUlx(w, ulx);
}
