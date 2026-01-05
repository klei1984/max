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

#ifndef FLIC_HPP
#define FLIC_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

struct SDL_IOStream;

namespace Installer {

/**
 * \brief Contains decoded RGBA8888 pixel data and timing information for a single animation frame.
 */
struct FlicFrameData {
    std::vector<uint8_t> rgba_data;  ///< RGBA8888 pixel data (width × height × 4 bytes)
    uint32_t duration_ms{0};         ///< Frame display duration in milliseconds
};

/**
 * \brief FLIC animation format parser that decodes .FLC files to RGBA8888 frames with palette tracking.
 *
 * Convert FLIC animation format used for M.A.X. unit portraits and menu decorations. The class decodes compressed frame
 * data (DELTA_FLC, BYTE_RUN, LITERAL chunks), tracks per-frame palette changes (COLOR_256, COLOR_64 chunks), and
 * converts 8-bit indexed color frames to 32-bit RGBA8888 format suitable for OpenGL 3.3 texture upload. The decoder
 * preserves timing information from the FLIC header for accurate playback.
 */
class Flic {
public:
    /**
     * \brief Constructs a FLIC parser and loads the animation file from the specified path.
     *
     * \param flic_path UTF-8 encoded absolute or relative path to the .FLC animation file to parse and decode.
     */
    explicit Flic(const std::filesystem::path& flic_path) noexcept;

    /**
     * \brief Destructs the FLIC parser and releases all allocated frame buffers and temporary decode resources.
     */
    ~Flic() noexcept;

    /**
     * \brief Checks if the FLIC file was successfully loaded and the header was parsed without errors.
     *
     * \return True if the FLIC header is valid and frame data is accessible, false if file open or parse failed.
     */
    bool IsValid() const noexcept { return m_is_valid; }

    /**
     * \brief Retrieves the animation frame width in pixels from the FLIC header.
     *
     * \return Frame width in pixels, or 0 if the FLIC file is invalid or header parsing failed.
     */
    uint32_t GetWidth() const noexcept { return m_width; }

    /**
     * \brief Retrieves the animation frame height in pixels from the FLIC header.
     *
     * \return Frame height in pixels, or 0 if the FLIC file is invalid or header parsing failed.
     */
    uint32_t GetHeight() const noexcept { return m_height; }

    /**
     * \brief Decodes all animation frames.
     *
     * \return Vector of decoded frames with RGBA8888 pixel data and per-frame timing, or empty vector on failure.
     */
    std::vector<FlicFrameData> DecodeAllFrames() noexcept;

private:
    std::filesystem::path m_flic_path;
    uint32_t m_width{0};
    uint32_t m_height{0};
    uint32_t m_frame_count{0};
    uint32_t m_frame_duration_ms{0};
    int32_t m_first_frame_offset{0};
    bool m_is_valid{false};

    bool LoadHeader() noexcept;
    bool DecodeFrame(SDL_IOStream* io, std::vector<uint8_t>& indexed_buffer, uint8_t* palette) noexcept;
    void DecodeColorChunk(const uint8_t* chunk_data, size_t chunk_data_size, uint8_t* palette, int32_t shift) noexcept;
    void DecodeDeltaFlc(const uint8_t* chunk_data, size_t chunk_data_size,
                        std::vector<uint8_t>& indexed_buffer) noexcept;
    void DecodeByteRun(const uint8_t* chunk_data, size_t chunk_data_size,
                       std::vector<uint8_t>& indexed_buffer) noexcept;
    void ConvertIndexedToRgba(const std::vector<uint8_t>& indexed_buffer, const uint8_t* palette,
                              std::vector<uint8_t>& rgba_buffer) noexcept;
};

}  // namespace Installer

#endif /* FLIC_HPP */
