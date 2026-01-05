/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef PORTRAITS_HPP
#define PORTRAITS_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "resource.hpp"
#include "sprite.hpp"

namespace Installer {

/**
 * \brief Orchestrates MAX_FORMAT_MULTI unit portrait sprite texture conversion pipeline: decoding, atlas packing, LZ4
 * compression.
 */
class Portraits {
public:
    /**
     * \brief Constructs a unit portrait sprite converter for the specified game data path and output directory.
     *
     * \param game_data_path UTF-8 encoded path to the original game data directory containing MAX.RES.
     * \param output_path UTF-8 encoded path to the directory where .dat and .json files will be written.
     * \param resources Const reference to the centralized portrait resource name table vector (thread-safe read-only).
     */
    Portraits(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
              const std::vector<const char*>& resources) noexcept;

    /**
     * \brief Destructs the unit portrait sprite converter and releases all allocated resources and temporary buffers.
     */
    ~Portraits() noexcept;

    /**
     * \brief Executes the full unit portrait texture conversion pipeline.
     *
     * \return True if conversion completed successfully and output files were written, false on critical failure.
     */
    bool Convert() noexcept;

    /**
     * \brief Checks if installer initialized successfully and is ready for conversion.
     *
     * \return True if MAX.RES is accessible and the output directory is writable, false otherwise.
     */
    bool IsValid() const noexcept;

private:
    static constexpr uint32_t MAX_ATLAS_SIZE = 1024;

    struct AtlasTexture {
        uint32_t texture_id;
        uint32_t width;
        uint32_t height;
        std::vector<uint8_t> pixels;
        uint64_t compressed_offset;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
    };

    struct PackedFrame {
        uint16_t frame_index;
        uint32_t texture_id;
        uint16_t atlas_x;
        uint16_t atlas_y;
        uint16_t width;
        uint16_t height;
        int16_t hotspot_x;
        int16_t hotspot_y;
        bool flipped;
    };

    struct PortraitData {
        std::string resource_name;
        std::vector<PackedFrame> frames;
    };

    std::vector<AtlasTexture> m_atlases;
    std::vector<PortraitData> m_portraits;
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_output_path;
    Resource m_resource_reader;
    const std::vector<const char*>& m_resources;
    std::string m_dat_filename;
    std::string m_dat_sha256;
    bool m_is_valid;

    bool PackAllFramesIntoAtlases(const std::vector<SpriteFrame>& all_frames,
                                  const std::vector<size_t>& frame_unit_indices,
                                  const std::vector<size_t>& frame_local_indices) noexcept;
    bool WriteCompressedAtlases(SDL_IOStream* dat_file) noexcept;
    bool GenerateMetadataJSON() noexcept;
};

}  // namespace Installer

#endif  // PORTRAITS_HPP
