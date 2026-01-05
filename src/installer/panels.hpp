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

#ifndef PANELS_HPP
#define PANELS_HPP

#include <cstdint>
#include <functional>
#include <vector>

#include "panel.hpp"
#include "resource.hpp"

namespace Installer {

/**
 * \brief Lightweight metadata for a panel image during preload phase, used for atlas packing calculations.
 */
struct PanelDimensionInfo {
    std::string resource_name;  ///< ResourceID identifier string
    uint16_t width{0};          ///< Image width in pixels
    uint16_t height{0};         ///< Image height in pixels
    int16_t hotspot_x{0};       ///< Hotspot X offset (for positioning)
    int16_t hotspot_y{0};       ///< Hotspot Y offset (for positioning)
    size_t resource_index{0};   ///< Index in m_panel_resources for later retrieval
};

/**
 * \brief Describes a single art image's position within a packed atlas texture for RmlUI rendering via UV mapping.
 */
struct PanelMetadata {
    std::string resource_name;  ///< ResourceID identifier string
    uint16_t atlas_index{0};    ///< Index of the atlas containing this image (0-based)
    uint16_t x{0};              ///< X coordinate of image top-left corner in atlas (pixels)
    uint16_t y{0};              ///< Y coordinate of image top-left corner in atlas (pixels)
    uint16_t width{0};          ///< Image width in pixels
    uint16_t height{0};         ///< Image height in pixels
    int16_t hotspot_x{0};       ///< Hotspot X offset (for positioning)
    int16_t hotspot_y{0};       ///< Hotspot Y offset (for positioning)
    bool rotated{false};        ///< True if image is rotated 90° CW in atlas
};

/**
 * \brief Describes a single texture atlas containing multiple packed art images.
 */
struct PanelAtlasMetadata {
    uint64_t data_offset{0};        ///< Byte offset of LZ4-compressed atlas in .dat file
    uint64_t compressed_size{0};    ///< Size of LZ4-compressed atlas data in bytes
    uint64_t uncompressed_size{0};  ///< Size of decompressed RGBA8888 atlas data in bytes
    uint16_t width{0};              ///< Atlas texture width in pixels
    uint16_t height{0};             ///< Atlas texture height in pixels
};

/**
 * \brief Orchestrates MAX_FORMAT_BIG art image conversion pipeline: decoding, atlas packing, LZ4 compression.
 */
class Panels {
public:
    /**
     * \brief Constructs an Panels converter for the specified game data path, output directory, and resource set.
     *
     * \param game_data_path UTF-8 encoded path to the original game data directory containing MAX.RES.
     * \param base_path UTF-8 encoded path to the executable directory where PATCHES.RES is located (optional patches).
     * \param output_path UTF-8 encoded path to the directory where .dat and .json files will be written.
     * \param panel_resources Const reference to centralized vector of ResourceEntry structures (thread-safe).
     */
    Panels(const std::filesystem::path& game_data_path, const std::filesystem::path& base_path,
           const std::filesystem::path& output_path, const std::vector<ResourceEntry>& panel_resources) noexcept;

    /**
     * \brief Destructs the Panels converter and releases all allocated resources and temporary buffers.
     */
    ~Panels() noexcept;

    /**
     * \brief Executes the full art image conversion pipeline, producing arts.dat and arts.json output files.
     *
     * This method iterates through all MAX_FORMAT_BIG resources, decodes each image using its embedded palette, packs
     * them into optimal 1024x1024 texture atlases, compresses atlases with LZ4HC at maximum level, and generates the
     * JSON metadata file with image positions and hotspot offsets.
     *
     * \return True if conversion completed successfully and output files were written, false on critical failure.
     */
    bool Convert() noexcept;

    /**
     * \brief Checks if the resource reader was successfully initialized for art conversion.
     *
     * \return True if MAX.RES is accessible and the output directory is writable, false otherwise.
     */
    bool IsValid() const noexcept { return m_is_valid; }

private:
    static constexpr uint32_t MAX_ATLAS_SIZE = 1024;  ///< Maximum atlas dimension (width or height) in pixels
    std::vector<PanelMetadata> m_panel_metadata;
    std::vector<PanelAtlasMetadata> m_atlas_metadata;
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_output_path;
    Resource m_resource_reader;
    std::string m_dat_filename;
    std::string m_dat_sha256;
    const std::vector<ResourceEntry>& m_panel_resources;
    bool m_is_valid{false};

    bool PreloadDimensions(std::vector<PanelDimensionInfo>& dimensions) noexcept;
    bool StreamProcessAndWriteAtlases(const std::vector<PanelDimensionInfo>& dimensions,
                                      SDL_IOStream* dat_file) noexcept;
    bool WriteMetadataJson() noexcept;

    bool ProcessAndBlitImage(const PanelDimensionInfo& dim, std::vector<uint8_t>& atlas_rgba, uint16_t atlas_width,
                             uint16_t x, uint16_t y, bool rotated, PanelData& out_panel_data) noexcept;
    bool CompressAndWriteAtlas(const std::vector<uint8_t>& atlas_rgba, SDL_IOStream* dat_file, uint16_t atlas_width,
                               uint16_t atlas_height) noexcept;
    PanelMetadata CreatePanelMetadata(const PanelData& panel_data, uint16_t atlas_index, uint16_t x, uint16_t y,
                                      bool rotated) noexcept;
    void ReportProgress(uint64_t& last_progress_time, size_t success_count, size_t fail_count,
                        size_t total_count) noexcept;
};

}  // namespace Installer

#endif /* PANELS_HPP */
