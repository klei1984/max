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

#include "panel.hpp"

#include <SDL3/SDL.h>

namespace Installer {

Panel::Panel(const std::string& resource_name, const std::vector<uint8_t>& data) noexcept {
    m_panel_data.resource_name = resource_name;
    m_is_valid = DecodeImage(data);
}

Panel::~Panel() noexcept {}

bool Panel::DecodeImage(const std::vector<uint8_t>& data) noexcept {
    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());

    if (!io) {
        return false;
    }

    int16_t ulx, uly, width, height;

    if (!SDL_ReadS16LE(io, &ulx) || !SDL_ReadS16LE(io, &uly) || !SDL_ReadS16LE(io, &width) ||
        !SDL_ReadS16LE(io, &height)) {
        SDL_CloseIO(io);

        return false;
    }

    // Validate dimensions
    if (width <= 0 || height <= 0) {
        SDL_CloseIO(io);

        return false;
    }

    if (SDL_ReadIO(io, m_palette, PALETTE_RGB_SIZE) != PALETTE_RGB_SIZE) {
        SDL_CloseIO(io);

        return false;
    }

    // Store metadata
    m_panel_data.width = static_cast<uint16_t>(width);
    m_panel_data.height = static_cast<uint16_t>(height);
    m_panel_data.hotspot_x = ulx;
    m_panel_data.hotspot_y = uly;

    // Allocate RGBA8888 buffer
    size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
    m_panel_data.rgba_data.resize(pixel_count * 4);

    // Allocate temporary indexed pixel buffer
    std::vector<uint8_t> indexed_pixels(pixel_count);

    // Decode RLE-compressed pixel data
    // The RLE format uses int16_t control words:
    // - Positive value N: copy next N literal bytes
    // - Negative value -N: repeat next single byte N times
    size_t dst_offset = 0;
    int32_t pitch = width;

    for (int32_t line = 0; line < height; ++line) {
        int32_t line_position = 0;

        while (line_position < width) {
            int16_t opt_word;

            if (!SDL_ReadS16LE(io, &opt_word)) {
                SDL_CloseIO(io);

                return false;
            }

            if (opt_word > 0) {
                // Literal run: copy opt_word bytes
                if (SDL_ReadIO(io, &indexed_pixels[dst_offset + line_position], opt_word) !=
                    static_cast<size_t>(opt_word)) {
                    SDL_CloseIO(io);

                    return false;
                }

                line_position += opt_word;

            } else {
                // Fill run: repeat single byte -opt_word times
                int16_t count = -opt_word;
                uint8_t fill_byte;

                if (SDL_ReadIO(io, &fill_byte, 1) != 1) {
                    SDL_CloseIO(io);

                    return false;
                }

                SDL_memset(&indexed_pixels[dst_offset + line_position], fill_byte, count);
                line_position += count;
            }
        }

        dst_offset += pitch;
    }

    SDL_CloseIO(io);

    // Convert palette indices to RGBA8888 using embedded palette (RGB888)
    uint8_t* rgba_out = m_panel_data.rgba_data.data();

    for (size_t i = 0; i < pixel_count; ++i) {
        const uint8_t index = indexed_pixels[i];
        const uint8_t* rgb = &m_palette[index * 3];

        // RGBA8888 layout: [R, G, B, A] - all pixels are fully opaque
        rgba_out[i * 4 + 0] = rgb[0];  // R
        rgba_out[i * 4 + 1] = rgb[1];  // G
        rgba_out[i * 4 + 2] = rgb[2];  // B
        rgba_out[i * 4 + 3] = 255;     // A (fully opaque)
    }

    return true;
}

bool Panel::DecodeMetadata(const std::string& resource_name, const std::vector<uint8_t>& data, uint16_t& out_width,
                           uint16_t& out_height, int16_t& out_hotspot_x, int16_t& out_hotspot_y) noexcept {
    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());

    if (!io) {
        return false;
    }

    int16_t ulx, uly, width, height;

    if (!SDL_ReadS16LE(io, &ulx) || !SDL_ReadS16LE(io, &uly) || !SDL_ReadS16LE(io, &width) ||
        !SDL_ReadS16LE(io, &height)) {
        SDL_CloseIO(io);

        return false;
    }

    SDL_CloseIO(io);

    // Validate dimensions
    if (width <= 0 || height <= 0) {
        return false;
    }

    out_width = static_cast<uint16_t>(width);
    out_height = static_cast<uint16_t>(height);
    out_hotspot_x = ulx;
    out_hotspot_y = uly;

    return true;
}

}  // namespace Installer
