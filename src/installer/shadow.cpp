/* Copyright (c) 2026 M.A.X. Port Team
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

#include "shadow.hpp"

#include <SDL3/SDL.h>

namespace Installer {

constexpr size_t FRAME_HEADER_SIZE = 8;

Shadow::Shadow(const uint8_t* resource_data, size_t resource_size) noexcept
    : m_resource_data(resource_data), m_resource_size(resource_size), m_frame_count(0), m_is_valid(false) {
    if (!m_resource_data || m_resource_size < sizeof(uint16_t)) {
        return;
    }

    SDL_IOStream* header_io = SDL_IOFromConstMem(m_resource_data, sizeof(uint16_t));

    SDL_ReadU16LE(header_io, &m_frame_count);
    SDL_CloseIO(header_io);

    if (m_frame_count == 0) {
        return;
    }

    const size_t min_size = sizeof(uint16_t) + m_frame_count * sizeof(void*);

    if (m_resource_size < min_size) {
        return;
    }

    m_is_valid = true;
}

Shadow::~Shadow() noexcept {}

bool Shadow::IsValid() const noexcept { return m_is_valid; }

bool Shadow::DecodeFrame(uint16_t frame_index, SpriteFrame& frame_out) noexcept {
    if (!m_is_valid || frame_index >= m_frame_count) {
        return false;
    }

    SDL_IOStream* frame_ptr_io =
        SDL_IOFromConstMem(m_resource_data + sizeof(uint16_t) + frame_index * sizeof(uint32_t), sizeof(uint32_t));
    uint32_t frame_offset;

    SDL_ReadU32LE(frame_ptr_io, &frame_offset);

    SDL_CloseIO(frame_ptr_io);

    if (frame_offset >= m_resource_size) {
        return false;
    }

    SDL_IOStream* frame_header_io = SDL_IOFromConstMem(m_resource_data + frame_offset, FRAME_HEADER_SIZE);
    int16_t width, height, hotx, hoty;

    SDL_ReadS16LE(frame_header_io, &width);
    SDL_ReadS16LE(frame_header_io, &height);
    SDL_ReadS16LE(frame_header_io, &hotx);
    SDL_ReadS16LE(frame_header_io, &hoty);

    SDL_CloseIO(frame_header_io);

    frame_out.width = width;
    frame_out.height = height;
    frame_out.hotspot_x = hotx;
    frame_out.hotspot_y = hoty;

    if (frame_out.width == 0 || frame_out.height == 0) {
        return false;
    }

    // Allocate output buffer - initialize to 0 (transparent/no shadow)
    const size_t pixel_count = static_cast<size_t>(frame_out.width) * frame_out.height;

    frame_out.data.resize(pixel_count, 0);

    // Decode each row
    for (uint16_t y = 0; y < frame_out.height; ++y) {
        // Row offsets are relative to resource start
        SDL_IOStream* row_offset_io = SDL_IOFromConstMem(
            m_resource_data + frame_offset + FRAME_HEADER_SIZE + y * sizeof(uint32_t), sizeof(uint32_t));
        uint32_t row_offset;

        SDL_ReadU32LE(row_offset_io, &row_offset);

        SDL_CloseIO(row_offset_io);

        if (row_offset >= m_resource_size) {
            return false;
        }

        const uint8_t* row_data = m_resource_data + row_offset;
        size_t bytes_available = m_resource_size - row_offset;
        uint8_t* output_row = frame_out.data.data() + y * frame_out.width;
        size_t bytes_consumed = DecodeShadowRLERow(row_data, bytes_available, frame_out.width, output_row);

        if (bytes_consumed == 0) {
            return false;
        }
    }

    return true;
}

std::vector<SpriteFrame> Shadow::DecodeAllFrames() noexcept {
    std::vector<SpriteFrame> frames;

    if (!m_is_valid) {
        return frames;
    }

    frames.reserve(m_frame_count);

    for (uint16_t i = 0; i < m_frame_count; ++i) {
        SpriteFrame frame;

        if (DecodeFrame(i, frame)) {
            frames.push_back(std::move(frame));
        }
    }

    return frames;
}

size_t Shadow::DecodeShadowRLERow(const uint8_t* row_data, size_t row_size, uint16_t width,
                                  uint8_t* output_row) noexcept {
    constexpr uint8_t ROW_DELIMITER = 0xFF;
    constexpr uint8_t SHADOW_PIXEL = 0xFF;  // Mark shadow pixels as 0xFF in output
    size_t bytes_consumed = 0;
    uint16_t pixel_position = 0;

    // Shadow RLE format: [transparent_count, shadow_count] pairs, NO pixel data
    while (bytes_consumed < row_size) {
        // Check for row delimiter
        if (row_data[bytes_consumed] == ROW_DELIMITER) {
            bytes_consumed++;
            return bytes_consumed;  // Success - found delimiter
        }

        // Read transparent_count (pixels to skip, leave as 0x00)
        uint8_t transparent_count = row_data[bytes_consumed++];

        // Read shadow_count (pixels that ARE shadow)
        if (bytes_consumed >= row_size) {
            return 0;
        }
        uint8_t shadow_count = row_data[bytes_consumed++];

        // Skip transparent pixels (output already initialized to 0)
        pixel_position += transparent_count;

        // Write shadow pixels (mark as 0xFF)
        for (uint8_t i = 0; i < shadow_count; ++i) {
            if (pixel_position < width) {
                output_row[pixel_position] = SHADOW_PIXEL;
            }
            pixel_position++;
        }
    }

    return 0;
}

}  // namespace Installer
