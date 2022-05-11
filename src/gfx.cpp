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

#include "gfx.hpp"

#include "resource_manager.hpp"

struct RowMeta {
    unsigned char* buffer;
    unsigned short data_size;
};

struct SpriteMeta {
    unsigned short width;
    unsigned short height;
    Point hot_spot;
    struct RowMeta* row_data;
};

static void Gfx_RescaleSpriteRow(unsigned char* row_data, struct RowMeta* meta, unsigned char* frame_buffer, int mode,
                                 int factor);

unsigned char* Gfx_ResourceBuffer;
unsigned int* Gfx_SpriteRowAddresses;
unsigned char Gfx_TeamColorIndexBase;
ColorIndex* Gfx_ColorIndices;
unsigned short Gfx_UnitBrightnessBase;
unsigned int Gfx_MapBrightness;
unsigned char* Gfx_MapWindowBuffer;
unsigned int Gfx_ZoomLevel;
int Gfx_MapScalingFactor;
int Gfx_MapWindowUlx;
int Gfx_MapWindowUly;
unsigned short Gfx_MapWindowWidth = 640;
Point Gfx_ScaledOffset;
unsigned short Gfx_ScaledWidth;
unsigned short Gfx_ScaledHeight;
unsigned int Gfx_ScalingFactorWidth;
unsigned int Gfx_ScalingFactorHeight;
int Gfx_TargetScreenBufferOffset;
unsigned int Gfx_SpriteRowIndex;
unsigned char* Gfx_SpriteRowAddress;
ColorIndex* Gfx_Decode_ColorMap;
short Gfx_PixelCount;
short Gfx_word_1686D6;
short Gfx_word_1686D8;
short Gfx_word_1686DA;

bool Gfx_DecodeSpriteSetup(Point point, unsigned char* buffer, int divisor, Rect* bounds) {
    bool result;
    unsigned short width;
    unsigned short height;
    unsigned short hotx;
    unsigned short hoty;
    Rect scaled_bounds;
    Rect target_bounds;

    width = reinterpret_cast<unsigned short*>(buffer)[0];
    height = reinterpret_cast<unsigned short*>(buffer)[1];
    hotx = reinterpret_cast<unsigned short*>(buffer)[2];
    hoty = reinterpret_cast<unsigned short*>(buffer)[3];

    scaled_bounds.ulx = point.x - ((hotx * 2) / divisor);
    scaled_bounds.uly = point.y - ((hoty * 2) / divisor);
    scaled_bounds.lrx = scaled_bounds.ulx + ((width * 2) / divisor);
    scaled_bounds.lry = scaled_bounds.uly + ((height * 2) / divisor);

    scaled_bounds.ulx = ((scaled_bounds.ulx << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx;
    scaled_bounds.uly = ((scaled_bounds.uly << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly;
    scaled_bounds.lrx = (((scaled_bounds.lrx - 1) << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUlx + 1;
    scaled_bounds.lry = (((scaled_bounds.lry - 1) << 16) / Gfx_MapScalingFactor) - Gfx_MapWindowUly + 1;

    target_bounds.ulx = std::max(scaled_bounds.ulx, bounds->ulx);
    target_bounds.uly = std::max(scaled_bounds.uly, bounds->uly);
    target_bounds.lrx = std::min(scaled_bounds.lrx, bounds->lrx);
    target_bounds.lry = std::min(scaled_bounds.lry, bounds->lry);

    if (target_bounds.ulx >= target_bounds.lrx || target_bounds.uly >= target_bounds.lry) {
        result = false;
    } else {
        Gfx_ScaledWidth = scaled_bounds.lrx - scaled_bounds.ulx;
        Gfx_ScaledHeight = scaled_bounds.lry - scaled_bounds.uly;

        if (Gfx_ScaledWidth < 2) {
            Gfx_ScaledWidth = 2;
        }

        if (Gfx_ScaledHeight < 2) {
            Gfx_ScaledHeight = 2;
        }

        Gfx_ScalingFactorWidth = (((width - 1) << 16) / (Gfx_ScaledWidth - 1)) + 8;
        Gfx_ScalingFactorHeight = (((height - 1) << 16) / (Gfx_ScaledHeight - 1)) + 8;
        Gfx_ScaledOffset.x = target_bounds.ulx - scaled_bounds.ulx;
        Gfx_ScaledOffset.y = target_bounds.uly - scaled_bounds.uly;
        Gfx_ScaledWidth = target_bounds.lrx - scaled_bounds.ulx;
        Gfx_ScaledHeight = target_bounds.lry - scaled_bounds.uly;

        Gfx_TargetScreenBufferOffset = Gfx_MapWindowWidth * target_bounds.uly + target_bounds.ulx;

        result = true;
    }

    return result;
}

void Gfx_DecodeSprite() {
    unsigned int offset;
    const unsigned char row_delimiter = 0xFF;
    int transparent_count;
    int pixel_count;
    int ebp;
    unsigned char* row_address;

    Gfx_Decode_ColorMap = &ResourceManager_ColorIndexTable13x8[(Gfx_UnitBrightnessBase & 0xE0) * 8];
    Gfx_SpriteRowIndex = Gfx_ScaledOffset.y * Gfx_ScalingFactorHeight;

    for (;;) {
        Gfx_SpriteRowAddress = &Gfx_ResourceBuffer[Gfx_SpriteRowAddresses[Gfx_SpriteRowIndex >> 16]];

        offset = Gfx_TargetScreenBufferOffset;
        Gfx_PixelCount = 0;
        Gfx_word_1686D6 = 0;
        Gfx_word_1686D8 = Gfx_ScaledOffset.x;
        Gfx_word_1686DA = Gfx_ScaledWidth;

        for (;;) {
            if (*Gfx_SpriteRowAddress == row_delimiter) {
                Gfx_TargetScreenBufferOffset += Gfx_MapWindowWidth;

                if (!--Gfx_ScaledHeight) {
                    return;
                }

                Gfx_SpriteRowIndex = Gfx_SpriteRowIndex + Gfx_ScalingFactorHeight;
                break;

            } else {
                int temp;

                transparent_count = *Gfx_SpriteRowAddress++;
                pixel_count = *Gfx_SpriteRowAddress++;

                row_address = Gfx_SpriteRowAddress;
                Gfx_SpriteRowAddress += pixel_count;
                Gfx_PixelCount += transparent_count;

                if (Gfx_PixelCount) {
                    temp = (((Gfx_PixelCount << 16) / Gfx_ScalingFactorWidth) + 1) - Gfx_word_1686D6;
                    Gfx_word_1686D6 += temp;

                    if (Gfx_word_1686D8) {
                        Gfx_word_1686D8 -= temp;

                        if (Gfx_word_1686D8 < 0) {
                            temp = -Gfx_word_1686D8;
                            Gfx_word_1686D8 = 0;
                        }
                    } else {
                        Gfx_word_1686DA -= temp;

                        if (Gfx_word_1686DA > 0) {
                            offset += temp;
                        } else {
                            continue;
                        }
                    }
                }

                row_address -= Gfx_PixelCount;
                Gfx_PixelCount += pixel_count;
                ebp = Gfx_word_1686D6;
                temp = (((Gfx_PixelCount << 16) / Gfx_ScalingFactorWidth) + 1) - ebp;
                Gfx_word_1686D6 += temp;

                if (Gfx_word_1686D8) {
                    Gfx_word_1686D8 -= temp;

                    if (Gfx_word_1686D8 >= 0) {
                        continue;
                    } else {
                        ebp += Gfx_word_1686D8 + temp;
                        temp = -Gfx_word_1686D8;
                        Gfx_word_1686D8 = 0;
                    }
                }

                Gfx_word_1686DA -= temp;

                if (Gfx_word_1686DA < 0) {
                    temp += Gfx_word_1686DA;
                    Gfx_word_1686DA = 0;
                }

                if (temp) {
                    unsigned char* address;
                    address = &Gfx_MapWindowBuffer[offset];

                    if (Gfx_TeamColorIndexBase) {
                        memset(address,
                               reinterpret_cast<unsigned char*>((reinterpret_cast<uintptr_t>(Gfx_Decode_ColorMap) &
                                                                 0xFFFFFF00))[Gfx_TeamColorIndexBase],
                               temp);
                        offset += temp;
                    } else {
                        ebp *= Gfx_ScalingFactorWidth;

                        for (int i = 0; i < temp; ++i) {
                            address[i] = reinterpret_cast<unsigned char*>(
                                (reinterpret_cast<uintptr_t>(Gfx_Decode_ColorMap) & 0xFFFFFF00))
                                [reinterpret_cast<unsigned char*>((reinterpret_cast<uintptr_t>(Gfx_ColorIndices) &
                                                                   0xFFFFFF00))[row_address[ebp >> 16]]];
                            ebp += Gfx_ScalingFactorWidth;
                        }

                        offset += temp;
                    }
                }
            }
        }
    }
}

unsigned char* Gfx_RescaleSprite(unsigned char* buffer, unsigned int* data_size, int mode, int scaling_factor) {
    unsigned short image_count;
    unsigned char* image_frame;
    unsigned char* frame_buffer;
    unsigned int* image_frame_row_offsets;
    unsigned int pixel_buffer_size;
    unsigned int pixel_buffer_size_max;
    struct SpriteMeta* frames;
    unsigned short frame_width;
    unsigned short frame_height;
    unsigned short frame_hotx;
    unsigned short frame_hoty;
    int row_index;
    unsigned char* decoded_image_buffer;
    unsigned char* decoded_image_frame;
    unsigned char* decoded_image_frame_row_data;

    frame_buffer = nullptr;
    pixel_buffer_size_max = 0;
    image_count = reinterpret_cast<unsigned short*>(buffer)[0];

    frames = new (std::nothrow) struct SpriteMeta[image_count];
    *data_size = image_count * sizeof(unsigned int) + sizeof(unsigned short);

    for (int i = 0; i < image_count; ++i) {
        image_frame = &buffer[reinterpret_cast<unsigned int*>(&buffer[sizeof(unsigned short)])[i]];
        pixel_buffer_size = reinterpret_cast<unsigned short*>(image_frame)[0] * 3 + 1;

        if (pixel_buffer_size > pixel_buffer_size_max) {
            pixel_buffer_size_max = pixel_buffer_size;

            delete[] frame_buffer;
            frame_buffer = new (std::nothrow) unsigned char[pixel_buffer_size];
        }

        frame_width = reinterpret_cast<unsigned short*>(image_frame)[0];
        frame_height = reinterpret_cast<unsigned short*>(image_frame)[1];
        frame_hotx = reinterpret_cast<unsigned short*>(image_frame)[2];
        frame_hoty = reinterpret_cast<unsigned short*>(image_frame)[3];

        frames[i].width = (frame_width + scaling_factor - 1) / scaling_factor;
        frames[i].height = (frame_height + scaling_factor - 1) / scaling_factor;
        frames[i].hot_spot.x = frame_hotx / scaling_factor;
        frames[i].hot_spot.y = frame_hoty / scaling_factor;
        frames[i].row_data = new (std::nothrow) struct RowMeta[frames[i].height];

        *data_size += frames[i].height * sizeof(unsigned int) + sizeof(unsigned short) * 4;
        image_frame_row_offsets = reinterpret_cast<unsigned int*>(&image_frame[sizeof(unsigned short) * 4]);
        row_index = 0;

        for (int j = 0; j < frames[i].height; ++j) {
            Gfx_RescaleSpriteRow(&buffer[image_frame_row_offsets[row_index]], &frames[i].row_data[j], frame_buffer,
                                 mode, scaling_factor);
            *data_size += frames[i].row_data[j].data_size;
            row_index += scaling_factor;
        }
    }

    delete[] frame_buffer;

    decoded_image_buffer = new (std::nothrow) unsigned char[*data_size];
    reinterpret_cast<unsigned short*>(decoded_image_buffer)[0] = image_count;

    decoded_image_frame = &decoded_image_buffer[sizeof(unsigned short) + image_count * sizeof(unsigned int)];

    for (int i = 0; i < image_count; ++i) {
        reinterpret_cast<unsigned int*>(&decoded_image_buffer[sizeof(unsigned short)])[i] =
            static_cast<unsigned int>(decoded_image_frame - decoded_image_buffer);

        reinterpret_cast<unsigned short*>(decoded_image_frame)[0] = frames[i].width;
        reinterpret_cast<unsigned short*>(decoded_image_frame)[1] = frames[i].height;
        reinterpret_cast<unsigned short*>(decoded_image_frame)[2] = frames[i].hot_spot.x;
        reinterpret_cast<unsigned short*>(decoded_image_frame)[3] = frames[i].hot_spot.y;

        image_frame_row_offsets = reinterpret_cast<unsigned int*>(&decoded_image_frame[sizeof(unsigned short) * 4]);
        decoded_image_frame =
            &decoded_image_frame[sizeof(unsigned short) * 4 + sizeof(unsigned int) * frames[i].height];

        for (int j = 0; j < frames[i].height; ++j) {
            image_frame_row_offsets[j] = static_cast<unsigned int>(decoded_image_frame - decoded_image_buffer);
            memcpy(decoded_image_frame, frames[i].row_data[j].buffer, frames[i].row_data[j].data_size);
            decoded_image_frame = &decoded_image_frame[frames[i].row_data[j].data_size];
            delete[] frames[i].row_data[j].buffer;
        }

        delete[] frames[i].row_data;
    }

    delete[] frames;

    SDL_assert((decoded_image_frame - decoded_image_buffer) == *data_size);

    return decoded_image_buffer;
}

void Gfx_RescaleSpriteRow(unsigned char* row_data, struct RowMeta* meta, unsigned char* frame_buffer, int mode,
                          int factor) {
    int transparent_count;
    int pixel_count;
    int factor_max;
    int factor_mask;
    unsigned char* address;
    const unsigned char row_delimiter = 0xFF;

    transparent_count = 0;
    factor_mask = factor - 1;
    factor_max = factor - 1;
    address = frame_buffer;

    while (*row_data != row_delimiter) {
        transparent_count += *row_data++;
        pixel_count = *row_data++;

        if (((factor_mask & (factor_max + transparent_count)) + pixel_count) / factor) {
            *address++ = ((factor_max & factor_mask) + transparent_count) / factor;
            factor_max += transparent_count;
            *address++ = ((factor_max & factor_mask) + pixel_count) / factor;

            if (mode) {
                factor_max += pixel_count;
            } else {
                transparent_count = factor_mask - (factor_mask & factor_max);
                row_data = &row_data[transparent_count];
                factor_max += pixel_count;
                pixel_count -= transparent_count + 1;
                *address++ = *row_data++;

                while (pixel_count > factor_mask) {
                    *address++ = *row_data;
                    row_data = &row_data[factor];
                    pixel_count -= factor;
                }

                row_data = &row_data[pixel_count];
            }

            transparent_count = 0;

        } else {
            transparent_count += pixel_count;

            if (!mode) {
                row_data = &row_data[pixel_count];
            }
        }
    }

    *address++ = row_delimiter;

    meta->data_size = address - frame_buffer;
    meta->buffer = new (std::nothrow) unsigned char[meta->data_size];
    memcpy(meta->buffer, frame_buffer, meta->data_size);
}
