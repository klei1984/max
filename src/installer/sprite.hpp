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

#ifndef SPRITE_HPP
#define SPRITE_HPP

#include <cstdint>
#include <vector>

namespace Installer {

/**
 * \brief Represents a single decoded sprite frame with metadata.
 *
 * Contains pixel data in R8 format (palette indices) plus hotspot coordinates for proper sprite positioning during
 * rendering. Hotspot is the reference point (anchor) for sprite placement in world coordinates.
 */
struct SpriteFrame {
    uint16_t width;             ///< Frame width in pixels.
    uint16_t height;            ///< Frame height in pixels.
    int16_t hotspot_x;          ///< Horizontal hotspot offset from top-left corner (pixels).
    int16_t hotspot_y;          ///< Vertical hotspot offset from top-left corner (pixels).
    std::vector<uint8_t> data;  ///< Pixel data in R8 format (palette indices, width * height bytes).
};

/**
 * \brief Decoder for MAX_FORMAT_MULTI sprite resources (RLE-compressed multi-frame sprites).
 *
 * Each sprite resource contains one or more frames stored with run-length encoding (RLE) for transparency compression.
 * Decoded frames are converted to R8 texture format (raw palette indices) suitable for OpenGL shader-based rendering
 * with dynamic palette lookup.
 */
class Sprite {
public:
    /**
     * \brief Constructs sprite decoder for the specified resource buffer.
     *
     * \param resource_data Pointer to raw MAX_FORMAT_MULTI resource data.
     * \param resource_size Size of resource buffer in bytes.
     */
    Sprite(const uint8_t* resource_data, size_t resource_size) noexcept;

    /**
     * \brief Destructor releases internal state (does not free resource_data, caller-owned).
     */
    ~Sprite() noexcept;

    /**
     * \brief Checks if sprite resource was successfully parsed and is ready for decoding.
     *
     * \return True if valid sprite resource found with frame_count > 0, false otherwise.
     */
    bool IsValid() const noexcept;

    /**
     * \brief Decodes a specific frame from the sprite resource to R8 format.
     *
     * \param frame_index Zero-based frame index.
     * \param frame_out Output structure to receive decoded frame data (data vector resized automatically).
     * \return True if frame decoded successfully, false on index out of range or decode error.
     */
    bool DecodeFrame(uint16_t frame_index, SpriteFrame& frame_out) noexcept;

    /**
     * \brief Decodes all frames in the sprite resource.
     *
     * Convenience method for batch decoding. Equivalent to calling DecodeFrame() for each frame index.
     *
     * \return Vector of decoded frames (empty if resource invalid or decoding failed).
     */
    std::vector<SpriteFrame> DecodeAllFrames() noexcept;

private:
    const uint8_t* m_resource_data;
    size_t m_resource_size;
    uint16_t m_frame_count;
    bool m_is_valid;

    size_t DecodeRLERow(const uint8_t* row_data, size_t row_size, uint16_t width, uint8_t* output_row) noexcept;
};

}  // namespace Installer

#endif  // SPRITE_HPP
