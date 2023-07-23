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

#include <cassert>
#include <cstdio>

#include "color.h"
#include "resource_manager.hpp"

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

int32_t screendump_pcx(int32_t width, int32_t length, uint8_t *buf, uint8_t *pal) {
    PcxHeader pcx_header;
    char filename[PATH_MAX];
    int32_t file_index;
    uint8_t palette[3 * PALETTE_SIZE];
    FILE *fp;

    file_index = 0;

    do {
        sprintf(filename, "%sMAX%4.4i.PCX", ResourceManager_FilePathGameInstall, file_index);
        file_index++;

        fp = fopen(filename, "rb");

        if (fp) {
            fclose(fp);
        }
    } while (fp);

    fp = fopen(filename, "wb");

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

    for (int32_t i = 0; i < (3 * PALETTE_SIZE); i++) {
        palette[i] = pal[i] * 4;
    }

    memcpy(pcx_header.Palette, palette, sizeof(pcx_header.Palette));

    fwrite(&pcx_header, 1, sizeof(PcxHeader), fp);

    while (length) {
        int32_t scan_line_pos;
        int32_t repeat_count;

        scan_line_pos = 0;
        do {
            for (repeat_count = 1; ((scan_line_pos + repeat_count) < width) && (repeat_count < 63) &&
                                   (buf[scan_line_pos + repeat_count - 1] == buf[repeat_count + scan_line_pos]);
                 ++repeat_count) {
                ;
            }

            if ((repeat_count > 1) || (buf[scan_line_pos] & 0xC0)) {
                fputc(repeat_count | 0xC0, fp);
            }

            fputc(buf[scan_line_pos], fp);
            scan_line_pos += repeat_count;
        } while (scan_line_pos < width);

        buf += width;
        --length;
    }

    fputc(0x0C, fp);

    fwrite(palette, 1, sizeof(palette), fp);

    fclose(fp);

    return 0;
}
