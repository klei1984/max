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

#ifndef MUSIC_HPP
#define MUSIC_HPP

#include <cstdint>
#include <unordered_map>

#include "resource.hpp"
#include "sound.hpp"

namespace Installer {

/**
 * \brief Defines a single music track audio resource mapping from ResourceID to source path for conversion processing.
 */
struct MusicTrackResourceEntry {
    const char* resource_name;  ///< Human-readable resource identifier
};

/**
 * \brief Converts RIFF WAV music track assets to a single PCM stream with JSON metadata for optimized runtime.
 */
class Music {
public:
    /**
     * \brief Constructs a music converter for the specified game data path, output directory, and resource table.
     *
     * \param game_data_path UTF-8 encoded path to the original game data directory containing MAX.RES and music WAVs.
     * \param output_path UTF-8 encoded path to the directory where .dat and .json files will be written on conversion.
     * \param music_resources Const reference to an exclusive music track resource table.
     */
    Music(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
          const std::vector<MusicTrackResourceEntry>& music_resources) noexcept;

    /**
     * \brief Destructs the music converter and releases all allocated resources and temporary buffers.
     */
    ~Music() noexcept;

    /**
     * \brief Executes the full music track asset conversion pipeline, producing .dat and .json output files.
     *
     * This method iterates through all music track resources defined in the resource mapping table, reads file paths
     * from MAX.RES, validates source RIFF WAV files, decodes to PCM, concatenates into a single stream, and generates
     * the metadata JSON file with SHA256 checksums. Duplicate source files are automatically detected and reused to
     * avoid redundant storage. Missing or corrupted source files trigger warnings but do not abort the installation;
     * they are simply skipped and logged for user review. Progress is reported to the main thread via SDL events.
     *
     * \return True if conversion completed successfully and output files were written, false on critical failure.
     */
    bool Convert() noexcept;

    /**
     * \brief Checks if the game data path contains valid resource files required for music track asset conversion.
     *
     * \return True if MAX.RES is accessible and the output directory is writable, false otherwise.
     */
    bool IsValid() const noexcept;

private:
    struct MusicTrackMetadata {
        std::string resource_name;
        uint32_t pcm_start_frame{0};
        uint32_t pcm_end_frame{0};
        uint32_t loop_start_frame{0};
        uint32_t loop_end_frame{0};
    };

    std::vector<MusicTrackMetadata> m_metadata;
    const std::vector<MusicTrackResourceEntry>& m_music_resources;
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_output_path;
    AudioFormat m_format;
    Resource m_resource_reader;
    std::string m_dat_filename;
    std::string m_dat_sha256;
    bool m_is_valid{false};

    bool ProcessMusicTrackResource(const MusicTrackResourceEntry& entry, uint32_t& current_frame_position,
                                   SDL_IOStream* dat_file, AudioFormat* expected_format) noexcept;
    bool WriteMetadataJson() noexcept;
};

}  // namespace Installer

#endif /* MUSIC_HPP */
