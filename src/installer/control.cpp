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

#include "control.hpp"

#include <SDL3/SDL.h>

namespace Installer {

static constexpr size_t SIMPLE_HEADER_SIZE = 8;

Control::Control(const std::string& resource_name, const std::vector<uint8_t>& data,
                 const PaletteRGBA (&palette)[PALETTE_RGBA_SIZE]) noexcept {
    m_control_data.resource_name = resource_name;
    m_is_valid = DecodeImage(data, palette);
}

Control::~Control() noexcept {}

bool Control::DecodeImage(const std::vector<uint8_t>& data, const PaletteRGBA (&palette)[PALETTE_RGBA_SIZE]) noexcept {
    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());

    if (!io) {
        return false;
    }

    uint16_t width, height;
    int16_t ulx, uly;

    if (!SDL_ReadU16LE(io, &width) || !SDL_ReadU16LE(io, &height) || !SDL_ReadS16LE(io, &ulx) ||
        !SDL_ReadS16LE(io, &uly)) {
        SDL_CloseIO(io);

        return false;
    }

    if (width == 0 || height == 0) {
        SDL_CloseIO(io);

        return false;
    }

    size_t expected_pixel_data_size = static_cast<size_t>(width) * static_cast<size_t>(height);
    size_t expected_total_size = SIMPLE_HEADER_SIZE + expected_pixel_data_size;

    if (data.size() < expected_total_size) {
        SDL_CloseIO(io);

        return false;
    }

    const uint8_t* pixel_indices = data.data() + SIMPLE_HEADER_SIZE;
    uint8_t transparent_color = pixel_indices[0];

    SDL_CloseIO(io);

    m_control_data.width = width;
    m_control_data.height = height;
    m_control_data.hotspot_x = ulx;
    m_control_data.hotspot_y = uly;
    m_control_data.transparent_color = transparent_color;

    // Allocate RGBA8888 buffer
    size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
    m_control_data.rgba_data.resize(pixel_count * sizeof(PaletteRGBA));

    // Convert palette indices to RGBA8888 using pre-multiplied palette
    uint8_t* rgba_out = m_control_data.rgba_data.data();

    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t index = pixel_indices[i];
        const PaletteRGBA color = Palette_GetRGBA(palette, index);

        if (index == transparent_color) {
            // Fully transparent pixel
            rgba_out[i * 4 + 0] = 0;  // R
            rgba_out[i * 4 + 1] = 0;  // G
            rgba_out[i * 4 + 2] = 0;  // B
            rgba_out[i * 4 + 3] = 0;  // A

        } else {
            // Opaque pixel (pre-multiplied)
            rgba_out[i * 4 + 0] = color.r;  // R
            rgba_out[i * 4 + 1] = color.g;  // G
            rgba_out[i * 4 + 2] = color.b;  // B
            rgba_out[i * 4 + 3] = color.a;  // A (255)
        }
    }

    return true;
}

}  // namespace Installer
