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

#ifndef SOUNDEFFECTS_HPP
#define SOUNDEFFECTS_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "resource.hpp"
#include "sound.hpp"

namespace Installer {

/**
 * \brief Metadata describing a single sound effect sample's position, format, and loop characteristics in PCM data
 * file.
 */
struct SoundEffectMetadata {
    std::string resource_name;  ///< ResourceID string identifier
    uint64_t pcm_start_frame{
        0};                     ///< Starting PCM frame position in the concatenated sound effects data stream (0-based)
    uint64_t pcm_end_frame{0};  ///< Ending PCM frame position (exclusive) in the sound effects data stream
    uint64_t loop_start_frame{0};  ///< Loop region start frame relative to pcm_start_frame (0 = no loop)
    uint64_t loop_end_frame{0};    ///< Loop region end frame relative to pcm_start_frame (0 = no loop)
};

/**
 * \brief Converts original RIFF WAV sound effect assets to a single PCM stream with JSON metadata for optimized
 * runtime.
 *
 * The class orchestrates the sound effect asset conversion pipeline: reading sound effect file paths from MAX.RES,
 * validating and decoding RIFF WAV files, extracting loop points, merging PCM data into a continuous stream, and
 * generating metadata JSON files with SHA256 checksums for integrity verification. All sound effects are consolidated
 * into a single language-agnostic dat and json file.
 */
class SoundEffects {
public:
    /**
     * \brief Constructs a sound effects converter for the specified game data path, output directory, and resource set.
     *
     * \param game_data_path UTF-8 encoded path to the original game data directory containing MAX.RES and sound WAVs.
     * \param output_path UTF-8 encoded path to the directory where dat and json files will be written on conversion.
     * \param soundeffect_resources Const reference to centralized vector of ResourceEntry structures (thread-safe).
     */
    SoundEffects(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
                 const std::vector<ResourceEntry>& soundeffect_resources) noexcept;

    /**
     * \brief Destructs the SoundEffects converter and releases all allocated resources and temporary buffers.
     */
    ~SoundEffects() noexcept;

    /**
     * \brief Executes the full sound effect asset conversion pipeline, producing dat and json output files.
     *
     * This method iterates through all sound effect resources defined in the resource mapping table, reads file paths
     * from MAX.RES, validates source RIFF WAV files, decodes to PCM, concatenates into a single stream, and generates
     * the metadata JSON file with SHA256 checksums.
     *
     * \return True if conversion completed successfully and output files were written, false on critical failure.
     */
    bool Convert() noexcept;

    /**
     * \brief Checks if the game data path contains valid resource files required for sound effect asset conversion.
     *
     * \return True if MAX.RES is accessible and the output directory is writable, false otherwise.
     */
    bool IsValid() const noexcept;

private:
    std::unordered_map<std::string, SoundEffectMetadata> m_converted_files;
    std::vector<SoundEffectMetadata> m_metadata;
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_output_path;
    AudioFormat m_output_format;
    Resource m_resource_reader;
    std::string m_dat_filename;
    std::string m_dat_sha256;
    const std::vector<ResourceEntry>& m_soundeffect_resources;
    bool m_is_valid{false};

    bool ProcessSoundEffectResource(const ResourceEntry& entry, uint64_t& current_frame_position,
                                    SDL_IOStream* dat_file) noexcept;
    bool WriteMetadataJson() noexcept;
};

}  // namespace Installer

#endif /* SOUNDEFFECTS_HPP */
