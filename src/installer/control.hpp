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

#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "palette.hpp"

namespace Installer {

/**
 * \brief Maps a resource name to its processing configuration for MAX_FORMAT_SIMPLE control texture conversion.
 */
struct ControlResourceEntry {
    const char* resource_name;                        ///< Human-readable resource identifier
    const PaletteRGBA (&palette)[PALETTE_RGBA_SIZE];  ///< Palette to use for decoding this resource
};

/**
 * \brief Contains decoded RGBA8888 pixel data and metadata for a MAX_FORMAT_SIMPLE texture.
 */
struct ControlData {
    std::vector<uint8_t> rgba_data;  ///< RGBA8888 pixel data (width × height × 4 bytes)
    uint16_t width{0};               ///< Image width in pixels
    uint16_t height{0};              ///< Image height in pixels
    int16_t hotspot_x{0};            ///< X coordinate of the image hotspot
    int16_t hotspot_y{0};            ///< Y coordinate of the image hotspot
    uint8_t transparent_color{0};    ///< Transparent color index
    std::string resource_name;       ///< Resource identifier
};

/**
 * \brief Decoder for MAX_FORMAT_SIMPLE palette-indexed control textures with transparency support.
 */
class Control {
public:
    /**
     * \brief Decodes MAX_FORMAT_SIMPLE image data using the specified palette for color lookup.
     *
     * \param resource_name Human-readable resource identifier
     * \param data Raw resource data
     * \param palette Reference to a 256-entry RGBA palette array for index-to-color conversion.
     */
    Control(const std::string& resource_name, const std::vector<uint8_t>& data,
            const PaletteRGBA (&palette)[PALETTE_RGBA_SIZE]) noexcept;

    /**
     * \brief Destructs the control texture decoder and releases decoded pixel buffer.
     */
    ~Control() noexcept;

    /**
     * \brief Checks if the image was successfully decoded from the raw resource data.
     *
     * \return True if header parsing and pixel decoding succeeded, false if data was malformed or too small.
     */
    bool IsValid() const noexcept { return m_is_valid; }

    /**
     * \brief Retrieves the decoded control texture data including RGBA8888 pixels and metadata.
     *
     * \return Reference to the decoded control data structure, or empty structure if decoding failed.
     */
    const ControlData& GetControlData() const noexcept { return m_control_data; }

private:
    ControlData m_control_data;
    bool m_is_valid{false};

    bool DecodeImage(const std::vector<uint8_t>& data, const PaletteRGBA (&palette)[PALETTE_RGBA_SIZE]) noexcept;
};

}  // namespace Installer

#endif /* CONTROL_HPP */
