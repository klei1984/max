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

#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#include <cstdint>
#include <functional>
#include <vector>

#include "control.hpp"
#include "resource.hpp"

namespace Installer {

/**
 * \brief Orchestrates MAX_FORMAT_SIMPLE control texture conversion pipeline: decoding, atlas packing, LZ4 compression.
 */
class Controls {
public:
    /**
     * \brief Constructs a controls converter for the specified game data path and output directory.
     *
     * \param game_data_path UTF-8 encoded path to the original game data directory containing MAX.RES.
     * \param output_path UTF-8 encoded path to the directory where .dat and .json files will be written.
     * \param control_resources Const reference to the centralized control resource table vector (thread-safe
     * read-only).
     */
    Controls(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
             const std::vector<ControlResourceEntry>& control_resources) noexcept;

    /**
     * \brief Destructs the controls converter and releases all allocated resources and temporary buffers.
     */
    ~Controls() noexcept;

    /**
     * \brief Executes the full control texture conversion pipeline, producing controls.dat and controls.json files.
     *
     * \return True if conversion completed successfully and output files were written, false on critical failure.
     */
    bool Convert() noexcept;

    /**
     * \brief Checks if the resource reader was successfully initialized for control texture conversion.
     *
     * \return True if MAX.RES is accessible and the output directory is writable, false otherwise.
     */
    bool IsValid() const noexcept { return m_is_valid; }

private:
    struct ControlMetadata {
        std::string resource_name;
        uint16_t atlas_index{0};
        uint16_t x{0};
        uint16_t y{0};
        uint16_t width{0};
        uint16_t height{0};
        int16_t hotspot_x{0};
        int16_t hotspot_y{0};
        bool rotated{false};
    };

    struct AtlasMetadata {
        uint64_t data_offset{0};
        uint64_t compressed_size{0};
        uint64_t uncompressed_size{0};
        uint16_t width{0};
        uint16_t height{0};
    };

    static constexpr uint32_t MAX_ATLAS_SIZE = 1024;

    std::vector<ControlMetadata> m_control_metadata;
    std::vector<AtlasMetadata> m_atlas_metadata;
    const std::vector<ControlResourceEntry>& m_resources;
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_output_path;
    Resource m_resource_reader;
    std::string m_dat_filename;
    std::string m_dat_sha256;
    bool m_is_valid{false};

    bool LoadAllControls(std::vector<ControlData>& controls) noexcept;
    bool PackAndWriteAtlases(std::vector<ControlData>& controls, SDL_IOStream* dat_file) noexcept;
    bool WriteMetadataJson() noexcept;
};

}  // namespace Installer

#endif /* CONTROLS_HPP */
