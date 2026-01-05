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

#ifndef SPRITES_HPP
#define SPRITES_HPP

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

#include "resource.hpp"
#include "sprite.hpp"

namespace Installer {

/** \brief Lightweight 2D point with 16-bit signed coordinates (POD struct). */
struct Point {
    int16_t x;
    int16_t y;
};

/**
 * \brief Orchestrates unit sprite and shadow asset conversions.
 *
 * Manages the complete conversion pipeline from MAX_FORMAT_MULTI and MAX_FORMAT_SHADOW to texture atlases.
 */
class Sprites {
public:
    /// Maximum texture atlas dimensions.
    static constexpr uint32_t MAX_ATLAS_SIZE = 1024;

    /**
     * \brief Constructs sprite installer with source game data and output paths.
     *
     * \param game_data_path Path to MAX game data directory containing MAX.RES.
     * \param output_path Directory where dat and json will be written.
     * \param units_json_path Path to units.json database file.
     */
    Sprites(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
            const std::filesystem::path& units_json_path) noexcept;

    /**
     * \brief Destructor releases resources and closes open files.
     */
    ~Sprites() noexcept;

    /**
     * \brief Checks if installer initialized successfully and is ready for conversion.
     *
     * \return True if resource reader valid and units.json parsed successfully, false otherwise.
     */
    bool IsValid() const noexcept;

    /**
     * \brief Executes the complete sprite conversion pipeline.
     *
     * Processes all units defined in units.json database file. Converts sprites and shadows to atlases, compresses with
     * LZ4HC, and generates metadata json.
     *
     * \return True if all sprites and shadows converted successfully, false on any error.
     */
    bool Convert() noexcept;

private:
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_output_path;
    std::filesystem::path m_units_json_path;
    Resource m_resource_reader;
    bool m_is_valid;

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

    struct FrameInfo {
        int16_t image_base;
        int16_t image_count;
        int16_t turret_image_base;
        int16_t turret_image_count;
        int16_t firing_image_base;
        int16_t firing_image_count;
        int16_t connector_image_base;
        int16_t connector_image_count;
        Point angle_offsets[8];
    };

    struct UnitData {
        std::string resource_name;
        uint32_t flags;
        std::vector<PackedFrame> sprite_frames;
        std::vector<PackedFrame> shadow_frames;
        std::string data_resource_name;
        std::string shadow_resource_name;
        FrameInfo frame_info;
        bool has_frame_info;
    };

    std::vector<AtlasTexture> m_sprite_atlases;
    std::vector<AtlasTexture> m_shadow_atlases;
    std::vector<UnitData> m_units;
    std::string m_dat_filename;
    std::string m_dat_sha256;

    bool LoadUnitsJSON() noexcept;
    bool PackAllFramesIntoAtlases(const std::vector<SpriteFrame>& all_frames,
                                  const std::vector<size_t>& frame_unit_indices,
                                  const std::vector<size_t>& frame_local_indices, std::vector<AtlasTexture>& atlases,
                                  std::vector<UnitData>& units, bool is_shadow) noexcept;
    bool DecodeFrameInfo(const std::string& resource_name, uint32_t flags, FrameInfo& frame_info) noexcept;
    bool WriteCompressedAtlases(SDL_IOStream* dat_file, std::vector<AtlasTexture>& atlases) noexcept;
    bool GenerateMetadataJSON() noexcept;
};

}  // namespace Installer

#endif  // SPRITES_HPP
