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
#include "window_manager.hpp"

struct RowMeta {
    uint8_t* buffer;
    uint16_t data_size;
};

struct SpriteMeta {
    uint16_t width;
    uint16_t height;
    Point hot_spot;
    struct RowMeta* row_data;
};

static void Gfx_RescaleSpriteRow(uint8_t* row_data, struct RowMeta* meta, uint8_t* frame_buffer, int32_t mode,
                                 int32_t factor);

uint8_t* Gfx_ResourceBuffer;
uint32_t* Gfx_SpriteRowAddresses;
uint8_t Gfx_TeamColorIndexBase;
ColorIndex* Gfx_ColorIndices;
uint8_t Gfx_UnitBrightnessBase;
uint32_t Gfx_MapBrightness;
uint8_t* Gfx_MapWindowBuffer;
uint32_t Gfx_ZoomLevel;
int32_t Gfx_MapScalingFactor;
int32_t Gfx_MapWindowUlx;
int32_t Gfx_MapWindowUly;
Point Gfx_ScaledOffset;
uint16_t Gfx_ScaledWidth;
uint16_t Gfx_ScaledHeight;
uint32_t Gfx_ScalingFactorWidth;
uint32_t Gfx_ScalingFactorHeight;
int32_t Gfx_TargetScreenBufferOffset;

const Rect Gfx_DirectionCorrections[8] = {
    {1, 0, 0, 1},   {0, -1, -1, 0}, {0, 1, -1, 0}, {1, 0, 0, -1},
    {-1, 0, 0, -1}, {0, 1, 1, 0},   {0, -1, 1, 0}, {-1, 0, 0, 1},
};

bool Gfx_DecodeSpriteSetup(Point point, uint8_t* buffer, int32_t divisor, Rect* bounds) {
    bool result;
    int16_t width;
    int16_t height;
    int16_t hotx;
    int16_t hoty;
    Rect scaled_bounds;
    Rect target_bounds;

    width = reinterpret_cast<int16_t*>(buffer)[0];
    height = reinterpret_cast<int16_t*>(buffer)[1];
    hotx = reinterpret_cast<int16_t*>(buffer)[2];
    hoty = reinterpret_cast<int16_t*>(buffer)[3];

    scaled_bounds.ulx = point.x - ((hotx * 2) / divisor);
    scaled_bounds.uly = point.y - ((hoty * 2) / divisor);
    scaled_bounds.lrx = scaled_bounds.ulx + ((width * 2) / divisor);
    scaled_bounds.lry = scaled_bounds.uly + ((height * 2) / divisor);

    scaled_bounds.ulx = Gfx_ScaleInt32(scaled_bounds.ulx) - Gfx_MapWindowUlx;
    scaled_bounds.uly = Gfx_ScaleInt32(scaled_bounds.uly) - Gfx_MapWindowUly;
    scaled_bounds.lrx = Gfx_ScaleInt32(scaled_bounds.lrx - 1) - Gfx_MapWindowUlx + 1;
    scaled_bounds.lry = Gfx_ScaleInt32(scaled_bounds.lry - 1) - Gfx_MapWindowUly + 1;

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

        Gfx_ScalingFactorWidth = (((width - 1) << GFX_SCALE_BASE) / (Gfx_ScaledWidth - 1)) + 8;
        Gfx_ScalingFactorHeight = (((height - 1) << GFX_SCALE_BASE) / (Gfx_ScaledHeight - 1)) + 8;
        Gfx_ScaledOffset.x = target_bounds.ulx - scaled_bounds.ulx;
        Gfx_ScaledOffset.y = target_bounds.uly - scaled_bounds.uly;
        Gfx_ScaledWidth = target_bounds.lrx - target_bounds.ulx;
        Gfx_ScaledHeight = target_bounds.lry - target_bounds.uly;

        Gfx_TargetScreenBufferOffset = WindowManager_WindowWidth * target_bounds.uly + target_bounds.ulx;

        result = true;
    }

    return result;
}

void Gfx_DecodeMapTile(const Rect* const pixel_bounds, const uint32_t tile_size, const uint32_t tile_base,
                       const uint8_t quotient) {
    if (pixel_bounds->lry - 1 > pixel_bounds->uly && pixel_bounds->lrx - 1 > pixel_bounds->ulx) {
        const uint32_t map_tile_zoom_factor{(((tile_size - 1) << GFX_SCALE_BASE) / (Gfx_ZoomLevel - 1)) + 8};

        const ColorIndex* color_table{&ResourceManager_ColorIndexTable13x8[(Gfx_MapBrightness & (~31)) * 8]};

        const Rect clipped_bounds = {.ulx = pixel_bounds->ulx & (~63),
                                     .uly = pixel_bounds->uly & (~63),
                                     .lrx = ((pixel_bounds->lrx - 1) & (~63)) + 63,
                                     .lry = ((pixel_bounds->lry - 1) & (~63)) + 63};

        const int32_t tile_count_x{(clipped_bounds.lrx - clipped_bounds.ulx) / 64 + 1};

        const int32_t tile_count_y{(clipped_bounds.lry - clipped_bounds.uly) / 64 + 1};

        const int32_t scaling_error_ulx = Gfx_ScaleInt32(pixel_bounds->ulx) - Gfx_ScaleInt32(clipped_bounds.ulx);

        const int32_t scaling_error_ulx_factor = scaling_error_ulx * map_tile_zoom_factor;

        const int32_t scaling_error_uly = Gfx_ScaleInt32(pixel_bounds->uly) - Gfx_ScaleInt32(clipped_bounds.uly);

        const int32_t scaling_error_uly_factor = scaling_error_uly * map_tile_zoom_factor;

        const int32_t scaling_error_lrx = Gfx_ScaleInt32(clipped_bounds.lrx) - Gfx_ScaleInt32(pixel_bounds->lrx - 1);

        const int32_t scaling_error_lry = Gfx_ScaleInt32(clipped_bounds.lry) - Gfx_ScaleInt32(pixel_bounds->lry - 1);

        uint8_t* map_buffer{Gfx_MapWindowBuffer};
        int32_t tile_position{0};

        for (int32_t y{tile_count_y}; y > 0; --y) {
            uint32_t tile_stride_y{Gfx_ZoomLevel};
            int32_t offset_y{0};
            uint8_t* map_address_y{map_buffer};

            if (y == tile_count_y) {
                tile_stride_y -= scaling_error_uly;
                offset_y = scaling_error_uly_factor;
            }

            if (y == 1) {
                tile_stride_y -= scaling_error_lry;
            }

            map_buffer = &map_buffer[WindowManager_WindowWidth * tile_stride_y];

            for (int32_t x{tile_count_x}; x > 0; --x) {
                uint8_t* const tile_buffer{
                    &ResourceManager_MapTileBuffer[ResourceManager_MapTileIds[tile_base + tile_position] << quotient]};
                uint8_t* map_tile_buffer = &tile_buffer[(offset_y >> GFX_SCALE_BASE) << (quotient >> 1)];

                uint32_t tile_stride_x{Gfx_ZoomLevel};
                int32_t offset_x{0};
                uint8_t* map_address_x{map_address_y};

                if (x == tile_count_x) {
                    tile_stride_x -= scaling_error_ulx;
                    offset_x = scaling_error_ulx_factor;
                }

                if (x == 1) {
                    tile_stride_x -= scaling_error_lrx;
                }

                map_address_y = &map_address_y[tile_stride_x];

                for (uint32_t j{0}; j < tile_stride_y; ++j) {
                    for (uint32_t i{0}; i < tile_stride_x; ++i) {
                        map_address_x[i] =
                            color_table[map_tile_buffer[(offset_x + i * map_tile_zoom_factor) >> GFX_SCALE_BASE]];
                    }

                    map_address_x = &map_address_x[WindowManager_WindowWidth];
                    map_tile_buffer = &tile_buffer[((offset_y + (j + 1) * map_tile_zoom_factor) >> GFX_SCALE_BASE)
                                                   << (quotient >> 1)];
                }

                ++tile_position;
            }

            tile_position += ResourceManager_MapSize.x - tile_count_x;
        }
    }
}

void Gfx_DecodeSprite() {
    const ColorIndex* color_table{&ResourceManager_ColorIndexTable13x8[(Gfx_UnitBrightnessBase & (~31)) * 8]};

    for (uint32_t Gfx_SpriteRowIndex = Gfx_ScaledOffset.y * Gfx_ScalingFactorHeight;;
         Gfx_SpriteRowIndex += Gfx_ScalingFactorHeight) {
        uint8_t* Gfx_SpriteRowAddress =
            &Gfx_ResourceBuffer[Gfx_SpriteRowAddresses[Gfx_SpriteRowIndex >> GFX_SCALE_BASE]];
        uint32_t offset = Gfx_TargetScreenBufferOffset;
        int16_t Gfx_PixelCount = 0;
        int16_t Gfx_word_1686D6 = 0;
        int16_t Gfx_word_1686D8 = Gfx_ScaledOffset.x;
        int16_t Gfx_word_1686DA = Gfx_ScaledWidth;
        constexpr uint8_t row_delimiter = 0xFF;
        int32_t ebp;

        for (;;) {
            if (*Gfx_SpriteRowAddress == row_delimiter) {
                Gfx_TargetScreenBufferOffset += WindowManager_WindowWidth;

                if (!--Gfx_ScaledHeight) {
                    return;
                }

                break;

            } else {
                int32_t temp;
                const int32_t transparent_count = *Gfx_SpriteRowAddress++;
                const int32_t pixel_count = *Gfx_SpriteRowAddress++;

                uint8_t* row_address = Gfx_SpriteRowAddress;
                Gfx_SpriteRowAddress += pixel_count;
                Gfx_PixelCount += transparent_count;

                if (Gfx_PixelCount) {
                    temp = (((Gfx_PixelCount << GFX_SCALE_BASE) / Gfx_ScalingFactorWidth) + 1) - Gfx_word_1686D6;
                    Gfx_word_1686D6 += temp;

                    if (Gfx_word_1686D8) {
                        Gfx_word_1686D8 -= temp;

                        if (Gfx_word_1686D8 < 0) {
                            temp = -Gfx_word_1686D8;
                            Gfx_word_1686D8 = 0;

                            Gfx_word_1686DA -= temp;

                            if (Gfx_word_1686DA > 0) {
                                offset += temp;

                            } else {
                                Gfx_TargetScreenBufferOffset += WindowManager_WindowWidth;

                                if (!--Gfx_ScaledHeight) {
                                    return;
                                }

                                break;
                            }
                        }

                    } else {
                        Gfx_word_1686DA -= temp;

                        if (Gfx_word_1686DA > 0) {
                            offset += temp;

                        } else {
                            Gfx_TargetScreenBufferOffset += WindowManager_WindowWidth;

                            if (!--Gfx_ScaledHeight) {
                                return;
                            }

                            break;
                        }
                    }
                }

                row_address -= Gfx_PixelCount;
                Gfx_PixelCount += pixel_count;
                ebp = Gfx_word_1686D6;
                temp = (((Gfx_PixelCount << GFX_SCALE_BASE) / Gfx_ScalingFactorWidth) + 1) - ebp;
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
                    uint8_t* address;
                    address = &Gfx_MapWindowBuffer[offset];

                    if (Gfx_TeamColorIndexBase) {
                        memset(address,
                               reinterpret_cast<uint8_t*>(
                                   (reinterpret_cast<uintptr_t>(color_table) & ~0xFF))[Gfx_TeamColorIndexBase],
                               temp);
                        offset += temp;
                    } else {
                        ebp *= Gfx_ScalingFactorWidth;

                        for (int32_t i = 0; i < temp; ++i) {
                            address[i] = reinterpret_cast<uint8_t*>(
                                (reinterpret_cast<uintptr_t>(color_table) &
                                 ~0xFF))[reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(Gfx_ColorIndices) &
                                                                     ~0xFF))[row_address[ebp >> GFX_SCALE_BASE]]];
                            ebp += Gfx_ScalingFactorWidth;
                        }

                        offset += temp;
                    }
                }
            }
        }
    }
}

void Gfx_DecodeShadow() {
    const uint8_t row_delimiter = 0xFF;
    uint8_t* buffer;
    int32_t offset;
    int32_t rescaled_pixel_count;
    uint8_t shadow_count = 0;

    for (uint32_t Gfx_SpriteRowIndex = Gfx_ScalingFactorHeight * Gfx_ScaledOffset.y; Gfx_ScaledHeight;
         Gfx_SpriteRowIndex += Gfx_ScalingFactorHeight) {
        buffer = &Gfx_ResourceBuffer[Gfx_SpriteRowAddresses[Gfx_SpriteRowIndex >> GFX_SCALE_BASE]];
        offset = Gfx_TargetScreenBufferOffset;
        int16_t Gfx_PixelCount = 0;
        int16_t Gfx_word_1686D6 = 0;
        int16_t Gfx_word_1686D8 = Gfx_ScaledOffset.x;
        int16_t Gfx_word_1686DA = Gfx_ScaledWidth;

        for (;;) {
            if (buffer[0] == 0xFF) {
                break;
            }

            Gfx_PixelCount += buffer[0];
            shadow_count = buffer[1];
            buffer = &buffer[2];

            if (Gfx_PixelCount) {
                rescaled_pixel_count =
                    (Gfx_PixelCount << GFX_SCALE_BASE) / Gfx_ScalingFactorWidth + 1 - Gfx_word_1686D6;
                Gfx_word_1686D6 += rescaled_pixel_count;

                if (Gfx_word_1686D8 != 0) {
                    Gfx_word_1686D8 -= rescaled_pixel_count;

                    if (Gfx_word_1686D8 < 0) {
                        rescaled_pixel_count = -Gfx_word_1686D8;
                        Gfx_word_1686D8 = 0;
                        Gfx_word_1686DA -= rescaled_pixel_count;

                        if (Gfx_word_1686DA <= 0) {
                            break;
                        }

                        offset += rescaled_pixel_count;
                    }

                } else {
                    Gfx_word_1686DA -= rescaled_pixel_count;

                    if (Gfx_word_1686DA <= 0) {
                        break;
                    }

                    offset += rescaled_pixel_count;
                }
            }

            Gfx_PixelCount += shadow_count;

            rescaled_pixel_count = (Gfx_PixelCount << GFX_SCALE_BASE) / Gfx_ScalingFactorWidth + 1 - Gfx_word_1686D6;
            Gfx_word_1686D6 += rescaled_pixel_count;

            if (Gfx_word_1686D8 != 0) {
                Gfx_word_1686D8 -= rescaled_pixel_count;

                if (Gfx_word_1686D8 >= 0) {
                    continue;
                }

                rescaled_pixel_count = -Gfx_word_1686D8;
                Gfx_word_1686D8 = 0;
            }

            Gfx_word_1686DA -= rescaled_pixel_count;

            if (Gfx_word_1686DA < 0) {
                rescaled_pixel_count += Gfx_word_1686DA;
                Gfx_word_1686DA = 0;
            }

            {
                ColorIndex* index_table = &ResourceManager_ColorIndexTable13x8[5 * PALETTE_SIZE];
                uint8_t* window_buffer = &Gfx_MapWindowBuffer[offset];

                for (int32_t i = 0; i < rescaled_pixel_count; ++i) {
                    window_buffer[i] = index_table[window_buffer[i]];
                }

                offset += rescaled_pixel_count;
            }
        }

        Gfx_TargetScreenBufferOffset += WindowManager_WindowWidth;
        --Gfx_ScaledHeight;
    }
}

uint8_t* Gfx_RescaleSprite(uint8_t* buffer, uint32_t* data_size, int32_t mode, int32_t scaling_factor) {
    uint16_t image_count;
    uint8_t* image_frame;
    uint8_t* frame_buffer;
    uint32_t* image_frame_row_offsets;
    uint32_t pixel_buffer_size;
    uint32_t pixel_buffer_size_max;
    struct SpriteMeta* frames;
    uint16_t frame_width;
    uint16_t frame_height;
    uint16_t frame_hotx;
    uint16_t frame_hoty;
    int32_t row_index;
    uint8_t* decoded_image_buffer;
    uint8_t* decoded_image_frame;
    uint8_t* decoded_image_frame_row_data;

    frame_buffer = nullptr;
    pixel_buffer_size_max = 0;
    image_count = reinterpret_cast<uint16_t*>(buffer)[0];

    frames = new (std::nothrow) struct SpriteMeta[image_count];
    *data_size = image_count * sizeof(uint32_t) + sizeof(uint16_t);

    for (int32_t i = 0; i < image_count; ++i) {
        image_frame = &buffer[reinterpret_cast<uint32_t*>(&buffer[sizeof(uint16_t)])[i]];
        pixel_buffer_size = reinterpret_cast<uint16_t*>(image_frame)[0] * 3 + 1;

        if (pixel_buffer_size > pixel_buffer_size_max) {
            pixel_buffer_size_max = pixel_buffer_size;

            delete[] frame_buffer;
            frame_buffer = new (std::nothrow) uint8_t[pixel_buffer_size];
        }

        frame_width = reinterpret_cast<uint16_t*>(image_frame)[0];
        frame_height = reinterpret_cast<uint16_t*>(image_frame)[1];
        frame_hotx = reinterpret_cast<uint16_t*>(image_frame)[2];
        frame_hoty = reinterpret_cast<uint16_t*>(image_frame)[3];

        frames[i].width = (frame_width + scaling_factor - 1) / scaling_factor;
        frames[i].height = (frame_height + scaling_factor - 1) / scaling_factor;
        frames[i].hot_spot.x = frame_hotx / scaling_factor;
        frames[i].hot_spot.y = frame_hoty / scaling_factor;
        frames[i].row_data = new (std::nothrow) struct RowMeta[frames[i].height];

        *data_size += frames[i].height * sizeof(uint32_t) + sizeof(uint16_t) * 4;
        image_frame_row_offsets = reinterpret_cast<uint32_t*>(&image_frame[sizeof(uint16_t) * 4]);
        row_index = 0;

        for (int32_t j = 0; j < frames[i].height; ++j) {
            Gfx_RescaleSpriteRow(&buffer[image_frame_row_offsets[row_index]], &frames[i].row_data[j], frame_buffer,
                                 mode, scaling_factor);
            *data_size += frames[i].row_data[j].data_size;
            row_index += scaling_factor;
        }
    }

    delete[] frame_buffer;

    decoded_image_buffer = new (std::nothrow) uint8_t[*data_size];
    reinterpret_cast<uint16_t*>(decoded_image_buffer)[0] = image_count;

    decoded_image_frame = &decoded_image_buffer[sizeof(uint16_t) + image_count * sizeof(uint32_t)];

    for (int32_t i = 0; i < image_count; ++i) {
        reinterpret_cast<uint32_t*>(&decoded_image_buffer[sizeof(uint16_t)])[i] =
            static_cast<uint32_t>(decoded_image_frame - decoded_image_buffer);

        reinterpret_cast<uint16_t*>(decoded_image_frame)[0] = frames[i].width;
        reinterpret_cast<uint16_t*>(decoded_image_frame)[1] = frames[i].height;
        reinterpret_cast<uint16_t*>(decoded_image_frame)[2] = frames[i].hot_spot.x;
        reinterpret_cast<uint16_t*>(decoded_image_frame)[3] = frames[i].hot_spot.y;

        image_frame_row_offsets = reinterpret_cast<uint32_t*>(&decoded_image_frame[sizeof(uint16_t) * 4]);
        decoded_image_frame = &decoded_image_frame[sizeof(uint16_t) * 4 + sizeof(uint32_t) * frames[i].height];

        for (int32_t j = 0; j < frames[i].height; ++j) {
            image_frame_row_offsets[j] = static_cast<uint32_t>(decoded_image_frame - decoded_image_buffer);
            memcpy(decoded_image_frame, frames[i].row_data[j].buffer, frames[i].row_data[j].data_size);
            decoded_image_frame = &decoded_image_frame[frames[i].row_data[j].data_size];
            delete[] frames[i].row_data[j].buffer;
        }

        delete[] frames[i].row_data;
    }

    delete[] frames;

    SDL_assert(static_cast<uint32_t>(decoded_image_frame - decoded_image_buffer) == *data_size);

    return decoded_image_buffer;
}

void Gfx_RenderCircle(uint8_t* buffer, int32_t full_width, int32_t width, int32_t height, int32_t xc, int32_t yc,
                      int32_t radius, int32_t color) {
#define Gfx_RenderCirclePoint(x, y) \
    if ((x) < width && (y) < height && (x) >= 0 && (y) >= 0) buffer[(y) * full_width + (x)] = color

    int32_t x = 0;
    int32_t y = radius;
    int32_t d = 3 + radius * -2;

    Gfx_RenderCirclePoint(x + xc, y + yc);
    Gfx_RenderCirclePoint(x + xc, -y + yc);
    Gfx_RenderCirclePoint(-x + xc, -y + yc);
    Gfx_RenderCirclePoint(-x + xc, y + yc);
    Gfx_RenderCirclePoint(y + xc, x + yc);
    Gfx_RenderCirclePoint(y + xc, -x + yc);
    Gfx_RenderCirclePoint(-y + xc, -x + yc);
    Gfx_RenderCirclePoint(-y + xc, x + yc);

    while (x <= y) {
        if (d <= 0) {
            d += 4 * x + 6;

        } else {
            d += 4 * (x - y) + 10;
            --y;
        }

        ++x;

        Gfx_RenderCirclePoint(x + xc, y + yc);
        Gfx_RenderCirclePoint(x + xc, -y + yc);
        Gfx_RenderCirclePoint(-x + xc, -y + yc);
        Gfx_RenderCirclePoint(-x + xc, y + yc);
        Gfx_RenderCirclePoint(y + xc, x + yc);
        Gfx_RenderCirclePoint(y + xc, -x + yc);
        Gfx_RenderCirclePoint(-y + xc, -x + yc);
        Gfx_RenderCirclePoint(-y + xc, x + yc);
    }
}

void Gfx_RescaleSpriteRow(uint8_t* row_data, struct RowMeta* meta, uint8_t* frame_buffer, int32_t mode,
                          int32_t factor) {
    int32_t transparent_count;
    int32_t pixel_count;
    int32_t factor_max;
    int32_t factor_mask;
    uint8_t* address;
    const uint8_t row_delimiter = 0xFF;

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
    meta->buffer = new (std::nothrow) uint8_t[meta->data_size];
    memcpy(meta->buffer, frame_buffer, meta->data_size);
}
