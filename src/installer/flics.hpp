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

#ifndef FLICS_HPP
#define FLICS_HPP

#include <cstdint>
#include <functional>
#include <vector>

#include "flic.hpp"
#include "resource.hpp"

namespace Installer {

/**
 * \brief Maps a ResourceID name to its processing priority for FLIC animation conversion ordering.
 */
struct FlicResourceEntry {
    const char* resource_name;  ///< Human-readable resource identifier
};

/**
 * \brief Describes a single frame's position within the packed atlas texture.
 */
struct FlicFrameMetadata {
    uint16_t x{0};        ///< X coordinate of frame top-left corner in atlas (pixels)
    uint16_t y{0};        ///< Y coordinate of frame top-left corner in atlas (pixels)
    uint16_t width{0};    ///< Frame width in pixels
    uint16_t height{0};   ///< Frame height in pixels
    bool rotated{false};  ///< True if frame is rotated 90° CW in atlas
};

/**
 * \brief Complete metadata for a single FLIC animation including atlas packing and timing information.
 */
struct FlicAnimationMetadata {
    std::string resource_name;              ///< ResourceID identifier string
    uint64_t data_offset{0};                ///< Byte offset of LZ4-compressed atlas in .dat file
    uint64_t compressed_size{0};            ///< Size of LZ4-compressed atlas data in bytes
    uint64_t uncompressed_size{0};          ///< Size of decompressed RGBA8888 atlas data in bytes
    uint32_t frame_count{0};                ///< Total number of animation frames (typically 30)
    uint32_t frame_duration_ms{0};          ///< Display duration per frame in milliseconds
    uint16_t atlas_width{0};                ///< Total atlas texture width in pixels
    uint16_t atlas_height{0};               ///< Total atlas texture height in pixels
    std::vector<FlicFrameMetadata> frames;  ///< Frame positions within atlas for UV coordinate calculation
};

/**
 * \brief Orchestrates FLIC animation conversion pipeline: resource loading, decoding, atlas packing, LZ4 compression.
 */
class Flics {
public:
    /**
     * \brief Constructs an Flics converter for the specified game data path and output directory.
     *
     * \param game_data_path UTF-8 encoded path to the original game data directory containing MAX.RES and FLIC files.
     * \param output_path UTF-8 encoded path to the directory where .dat and .json files will be written on conversion.
     * \param resources Vector of FLIC resource entries to convert.
     */
    Flics(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
          const std::vector<FlicResourceEntry>& resources) noexcept;

    /**
     * \brief Destructs the Flics converter and releases all allocated resources and temporary buffers.
     */
    ~Flics() noexcept;

    /**
     * \brief Executes the full FLIC animation conversion pipeline, producing animations.dat and animations.json.
     *
     * \return True if conversion completed successfully and output files were written, false on critical failure.
     */
    bool Convert() noexcept;

    /**
     * \brief Checks if the game data path contains valid resource files required for FLIC animation conversion.
     *
     * \return True if MAX.RES is accessible and the output directory is writable, false otherwise.
     */
    bool IsValid() const noexcept;

private:
    static constexpr uint32_t MAX_ATLAS_SIZE = 1024;  ///< Maximum atlas dimension (width or height) in pixels

    std::vector<FlicAnimationMetadata> m_metadata;
    std::vector<FlicResourceEntry> m_resources;
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_output_path;
    Resource m_resource_reader;
    std::string m_dat_filename;
    std::string m_dat_sha256;
    bool m_is_valid{false};

    bool ProcessFlicResource(const FlicResourceEntry& entry, SDL_IOStream* dat_file) noexcept;
    bool WriteMetadataJson() noexcept;
};

}  // namespace Installer

#endif /* FLICS_HPP */
