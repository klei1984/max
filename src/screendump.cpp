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

#include "screendump.h"

#include "cursor.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "smartstring.hpp"
#include "svga.h"

int32_t screendump_key;
static ScreenDumpFunc screendump_function;
static uint8_t* screendump_buffer;

typedef struct {
    uint8_t Identifier;
    uint8_t Version;
    uint8_t Encoding;
    uint8_t BitsPerPixel;
    uint16_t XStart;
    uint16_t YStart;
    uint16_t XEnd;
    uint16_t YEnd;
    uint16_t HorzRes;
    uint16_t VertRes;
    uint8_t Palette[48];
    uint8_t Reserved1;
    uint8_t NumBitPlanes;
    uint16_t BytesPerLine;
    uint16_t PaletteType;
    uint16_t HorzScreenSize;
    uint16_t VertScreenSize;
    uint8_t Reserved2[54];
} PcxHeader;

static int32_t screendump_default(int32_t width, int32_t length, uint8_t* buffer, uint8_t* palette);

static std::filesystem::path screendump_get_file_path(const char* extension) {
    int32_t file_index;
    SmartString filename;
    std::filesystem::path filepath;
    std::error_code ec;

    file_index = 0;

    do {
        filename.Sprintf(20, "MAX%4.4i.%s", file_index, extension);
        ++file_index;
        filepath = (ResourceManager_FilePathGamePref / filename.GetCStr()).lexically_normal();
    } while (std::filesystem::exists(filepath, ec));

    return filepath;
}

int32_t screendump_save(int32_t width, int32_t length, uint8_t* buffer, uint8_t* palette) {
    int32_t result;

    const std::string format = ResourceManager_GetSettings()->GetStringValue("screenshot_format");

    if (format == "pcx") {
        result = screendump_pcx(width, length, buffer, palette);

    } else if (format == "bmp") {
        result = screendump_default(width, length, buffer, palette);

    } else {
        result = screendump_png(width, length, buffer, palette);
    }

    return result;
}

int32_t screendump_png(int32_t width, int32_t length, uint8_t* buffer, uint8_t* palette) {
    const std::filesystem::path filepath = screendump_get_file_path("png");
    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, length, SDL_PIXELFORMAT_INDEX8, buffer, width);
    int32_t result = -1;

    if (surface) {
        SDL_Palette* surface_palette = SDL_CreateSurfacePalette(surface);

        if (surface_palette) {
            SDL_Color colors[PALETTE_SIZE];

            // the screen shows the 6 bit palette entries multiplied by 4
            for (int32_t i = 0; i < PALETTE_SIZE; ++i) {
                colors[i].r = palette[PALETTE_STRIDE * i + 0] * 4;
                colors[i].g = palette[PALETTE_STRIDE * i + 1] * 4;
                colors[i].b = palette[PALETTE_STRIDE * i + 2] * 4;
                colors[i].a = SDL_ALPHA_OPAQUE;
            }

            if (SDL_SetPaletteColors(surface_palette, colors, 0, PALETTE_SIZE) &&
                SDL_SavePNG(surface, filepath.string().c_str())) {
                result = 0;
            }
        }

        SDL_DestroySurface(surface);
    }

    if (result != 0) {
        SDL_Log("Screendump: failed to save %s: %s", filepath.string().c_str(), SDL_GetError());
    }

    return result;
}

int32_t screendump_pcx(int32_t width, int32_t length, uint8_t* buffer, uint8_t* palette) {
    PcxHeader pcx_header;
    uint8_t pcx_palette[PALETTE_STRIDE * PALETTE_SIZE];
    FILE* fp;
    const std::filesystem::path filepath = screendump_get_file_path("PCX");

    fp = fopen(filepath.string().c_str(), "wb");

    memset(&pcx_header, 0, sizeof(PcxHeader));

    pcx_header.Identifier = 10;
    pcx_header.Version = 5;
    pcx_header.Encoding = 1;
    pcx_header.BitsPerPixel = 8;
    pcx_header.XEnd = width - 1;
    pcx_header.YEnd = length - 1;
    pcx_header.HorzRes = width;
    pcx_header.VertRes = length;
    pcx_header.NumBitPlanes = 1;
    pcx_header.BytesPerLine = width;
    pcx_header.PaletteType = 1;

    for (int32_t i = 0; i < (PALETTE_STRIDE * PALETTE_SIZE); i++) {
        pcx_palette[i] = palette[i] * 4;
    }

    memcpy(pcx_header.Palette, pcx_palette, sizeof(pcx_header.Palette));

    fwrite(&pcx_header, 1, sizeof(PcxHeader), fp);

    while (length) {
        int32_t scan_line_pos;
        int32_t repeat_count;

        scan_line_pos = 0;
        do {
            for (repeat_count = 1; ((scan_line_pos + repeat_count) < width) && (repeat_count < 63) &&
                                   (buffer[scan_line_pos + repeat_count - 1] == buffer[repeat_count + scan_line_pos]);
                 ++repeat_count) {
                ;
            }

            if ((repeat_count > 1) || ((buffer[scan_line_pos] & 0xC0) == 0xC0)) {
                fputc(repeat_count | 0xC0, fp);
            }

            fputc(buffer[scan_line_pos], fp);
            scan_line_pos += repeat_count;
        } while (scan_line_pos < width);

        buffer += width;
        --length;
    }

    fputc(0x0C, fp);

    fwrite(pcx_palette, 1, sizeof(pcx_palette), fp);

    fclose(fp);

    return 0;
}

int32_t screendump_default(int32_t width, int32_t length, uint8_t* buffer, uint8_t* palette) {
    uint8_t blue;
    uint8_t reserved;
    uint8_t red;
    uint8_t green;

    int32_t result;

    int32_t i;
    FILE* fp;
    const std::filesystem::path filepath = screendump_get_file_path("bmp");

    // BMP rows are padded to a multiple of 4 bytes
    const int32_t row_padding = (4 - (width % 4)) % 4;
    const uint8_t padding_bytes[3] = {0, 0, 0};

    int32_t temp_signed_32;
    uint32_t temp_unsigned_32;
    uint16_t temp_unsigned_16;

    if ((fp = fopen(filepath.string().c_str(), "wb")) != 0) {
        temp_unsigned_16 = 'B' + ('M' << 8);
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_32 = length * (width + row_padding) + 1078;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_unsigned_16 = 0;
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_16 = 0;
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_32 = 1078;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_unsigned_32 = 40;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_signed_32 = width;
        fwrite(&temp_signed_32, sizeof(temp_signed_32), 1, fp);

        temp_signed_32 = length;
        fwrite(&temp_signed_32, sizeof(temp_signed_32), 1, fp);

        temp_unsigned_16 = 1;
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_16 = 8;
        fwrite(&temp_unsigned_16, sizeof(temp_unsigned_16), 1, fp);

        temp_unsigned_32 = 0;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_unsigned_32 = 0;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_signed_32 = 0;
        fwrite(&temp_signed_32, sizeof(temp_signed_32), 1, fp);

        temp_signed_32 = 0;
        fwrite(&temp_signed_32, sizeof(temp_signed_32), 1, fp);

        temp_unsigned_32 = 0;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        temp_unsigned_32 = 0;
        fwrite(&temp_unsigned_32, sizeof(temp_unsigned_32), 1, fp);

        for (i = 0; i < PALETTE_SIZE; ++i) {
            reserved = 0;
            red = palette[PALETTE_STRIDE * i + 0] * 4;
            green = palette[PALETTE_STRIDE * i + 1] * 4;
            blue = palette[PALETTE_STRIDE * i + 2] * 4;

            fwrite(&blue, 1, 1, fp);
            fwrite(&green, 1, 1, fp);
            fwrite(&red, 1, 1, fp);
            fwrite(&reserved, 1, 1, fp);
        }

        for (i = length - 1; i >= 0; --i) {
            fwrite(&buffer[width * i], sizeof(uint8_t), width, fp);
            fwrite(padding_bytes, sizeof(uint8_t), row_padding, fp);
        }

        fflush(fp);
        fclose(fp);

        result = 0;
    } else {
        result = -1;
    }

    return result;
}

void screendump_register(int32_t new_screendump_key, ScreenDumpFunc new_screendump_func) {
    screendump_key = new_screendump_key;

    if (new_screendump_func) {
        screendump_function = new_screendump_func;
    } else {
        screendump_function = screendump_default;
    }
}

int32_t screendump_get_key() { return screendump_key; }

void screendump_dump_screen(void) {
    int32_t width;
    int32_t length;

    width = scr_size.lrx - scr_size.ulx + 1;
    length = scr_size.lry - scr_size.uly + 1;

    screendump_buffer = (uint8_t*)SDL_malloc(length * width);

    if (screendump_buffer) {
        uint8_t surface_palette[PALETTE_STRIDE * PALETTE_SIZE];

        if (Svga_GetPaletteSurfaceData(screendump_buffer, length * width, surface_palette)) {
            Cursor_RenderToBuffer(screendump_buffer, width, length);
            screendump_function(width, length, screendump_buffer, surface_palette);
        }

        SDL_free(screendump_buffer);
        screendump_buffer = NULL;
    }
}
