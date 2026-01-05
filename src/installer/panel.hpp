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

#ifndef PANEL_HPP
#define PANEL_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace Installer {

/**
 * \brief Contains decoded RGBA8888 pixel data and metadata for a single MAX_FORMAT_BIG image.
 */
struct PanelData {
    std::vector<uint8_t> rgba_data;  ///< RGBA8888 pixel data
    std::string resource_name;       ///< Resource identifier
    uint16_t width{0};               ///< Image width in pixels
    uint16_t height{0};              ///< Image height in pixels
    int16_t hotspot_x{0};            ///< X coordinate of the image hotspot
    int16_t hotspot_y{0};            ///< Y coordinate of the image hotspot
};

/**
 * \brief MAX_FORMAT_BIG image decoder that converts palette-indexed images with embedded palette to RGBA8888 format.
 */
class Panel {
public:
    /// Number of entries in palette
    static constexpr uint32_t PALETTE_RGB_SIZE = 3 * 256u;

    /**
     * \brief Constructs an art decoder for raw MAX_FORMAT_BIG data.
     *
     * \param resource_name Human-readable resource identifier for logging and metadata.
     * \param data Raw resource data containing the MAX_FORMAT_BIG image.
     */
    Panel(const std::string& resource_name, const std::vector<uint8_t>& data) noexcept;

    /**
     * \brief Destructs the art decoder and releases decoded pixel buffer.
     */
    ~Panel() noexcept;

    /**
     * \brief Checks if the image was successfully decoded from the raw resource data.
     *
     * \return True if header parsing and pixel decoding succeeded, false if data was malformed or too small.
     */
    bool IsValid() const noexcept { return m_is_valid; }

    /**
     * \brief Retrieves the decoded image data including RGBA8888 pixels and metadata.
     *
     * \return Reference to the decoded image data structure, or empty structure if decoding failed.
     */
    PanelData& GetPanelData() noexcept { return m_panel_data; }

    /**
     * \brief Extracts only metadata (dimensions, hotspot) from raw MAX_FORMAT_BIG data without decoding pixels.
     *
     * \param resource_name Human-readable resource identifier for logging and metadata.
     * \param data Raw resource data containing the MAX_FORMAT_BIG image header.
     * \param out_width Output parameter for image width.
     * \param out_height Output parameter for image height.
     * \param out_hotspot_x Output parameter for hotspot X coordinate.
     * \param out_hotspot_y Output parameter for hotspot Y coordinate.
     * \return True if header was successfully parsed, false if data was malformed or too small.
     */
    static bool DecodeMetadata(const std::string& resource_name, const std::vector<uint8_t>& data, uint16_t& out_width,
                               uint16_t& out_height, int16_t& out_hotspot_x, int16_t& out_hotspot_y) noexcept;

private:
    PanelData m_panel_data;
    uint8_t m_palette[PALETTE_RGB_SIZE];
    bool m_is_valid{false};

    bool DecodeImage(const std::vector<uint8_t>& data) noexcept;
};

}  // namespace Installer

#endif /* PANEL_HPP */
