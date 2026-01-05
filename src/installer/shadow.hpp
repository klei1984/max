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

#ifndef SHADOW_HPP
#define SHADOW_HPP

#include <filesystem>

#include "sprite.hpp"

namespace Installer {

/**
 * \brief Decoder for shadow sprite resources (RLE-compressed shadow masks).
 *
 * Decodes the shadow format used by unit graphics. The shadow format encodes "transparent" regions (no shadow) and
 * "shadow" regions (pixels to darken) without storing any palette indices.
 */
class Shadow {
public:
    /**
     * \brief Constructs shadow decoder for the specified resource buffer.
     *
     * \param resource_data Pointer to raw shadow resource data.
     * \param resource_size Size of resource buffer in bytes.
     */
    Shadow(const uint8_t* resource_data, size_t resource_size) noexcept;

    /**
     * \brief Destructor releases internal state.
     */
    ~Shadow() noexcept;

    /**
     * \brief Checks if shadow resource was successfully parsed and is ready for decoding.
     *
     * \return True if valid header found with frame_count > 0, false otherwise.
     */
    bool IsValid() const noexcept;

    /**
     * \brief Decodes a specific frame from the shadow resource to GL_R8 mask format.
     *
     * Decompresses shadow RLE data to a binary mask. Transparent pixels are stored as 0x00, shadow pixels as 0xFF.
     * During rendering, shadow pixels should be looked up in a brightness table to darken the underlying pixels.
     *
     * \param frame_index Zero-based frame index.
     * \param frame_out Output structure to receive decoded frame data (data vector resized automatically).
     * \return True if frame decoded successfully, false on index out of range or decode error.
     */
    bool DecodeFrame(uint16_t frame_index, SpriteFrame& frame_out) noexcept;

    /**
     * \brief Decodes all frames in the shadow resource.
     *
     * Convenience method for batch decoding. Equivalent to calling DecodeFrame() for each frame index.
     *
     * \return Vector of decoded frames (empty if resource invalid or decoding failed).
     */
    std::vector<SpriteFrame> DecodeAllFrames() noexcept;

private:
    const uint8_t* m_resource_data;  ///< Raw resource buffer (not owned, caller-managed).
    size_t m_resource_size;          ///< Size of resource buffer in bytes.
    uint16_t m_frame_count;          ///< Number of frames in shadow.
    bool m_is_valid;                 ///< True if resource parsed successfully.

    /**
     * \brief Decodes shadow RLE row data to binary mask.
     *
     * Processes one row of shadow data using shadow RLE format. Unlike sprite RLE, shadow format only contains
     * counts - no pixel data. Each segment is just [transparent_count, shadow_count], then the next segment follows
     * immediately.
     *
     * \param row_data Pointer to start of shadow RLE row data.
     * \param row_size Maximum bytes available in row_data buffer.
     * \param width Row width in pixels.
     * \param output_row Pre-allocated buffer for decoded row (must be at least width bytes, initialized to 0).
     * \return Number of bytes consumed from row_data, or 0 on error.
     */
    size_t DecodeShadowRLERow(const uint8_t* row_data, size_t row_size, uint16_t width, uint8_t* output_row) noexcept;
};

}  // namespace Installer

#endif  // SHADOW_HPP
