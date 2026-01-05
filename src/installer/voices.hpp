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

#ifndef VOICES_HPP
#define VOICES_HPP

#include <cstdint>
#include <vector>

#include "resource.hpp"
#include "sound.hpp"

namespace Installer {

/**
 * \\struct VoiceMetadata
 * \\brief Metadata describing a single voice sample's position, format, and loop characteristics in the PCM data file.
 */
struct VoiceMetadata {
    std::string resource_name;     ///< ResourceID string identifier
    uint64_t pcm_start_frame{0};   ///< Starting PCM frame position in the concatenated voice data stream (0-based)
    uint64_t pcm_end_frame{0};     ///< Ending PCM frame position (exclusive) in the voice data stream
    uint64_t loop_start_frame{0};  ///< Loop region start frame relative to pcm_start_frame (0 = no loop)
    uint64_t loop_end_frame{0};    ///< Loop region end frame relative to pcm_start_frame (0 = no loop)
    uint8_t priority{0};           ///< Voice priority group (1-5) for playback scheduling
};

/**
 * \class Voices
 * \brief Converts original RIFF WAV voice assets to a single PCM stream with JSON metadata for optimized runtime use.
 *
 * The class orchestrates the voice asset conversion pipeline: reading voice file paths from MAX.RES, validating and
 * decoding RIFF WAV files, extracting loop points, merging PCM data into a continuous stream, and generating metadata
 * JSON files with SHA256 checksums for integrity verification. Each language variant (en-US, de-DE, etc.) produces a
 * separate dat and json file.
 */
class Voices {
public:
    /**
     * \brief Constructs a voices converter for the specified game data path, language tag, output dir, and resources.
     *
     * \param game_data_path UTF-8 encoded path to the original game data directory containing MAX.RES and voice WAVs.
     * \param language_tag IETF BCP 47 language tag (e.g., "en-US", "de-DE") identifying the voice variant to convert.
     * \param output_path UTF-8 encoded path to the directory where dat and json files will be written on conversion.
     * \param voice_resources Const reference to the centralized voice resource table vector (thread-safe read-only).
     */
    Voices(const std::filesystem::path& game_data_path, const std::string& language_tag,
           const std::filesystem::path& output_path, const std::vector<ResourceEntry>& voice_resources) noexcept;

    /**
     * \brief Destructs the Voices converter and releases all allocated resources and temporary buffers.
     */
    ~Voices() noexcept;

    /**
     * \brief Executes the full voice asset conversion pipeline, producing language-specific dat and json output.
     *
     * This method iterates through all voice resources defined in the resource mapping table, reads file paths from
     * MAX.RES, validates source RIFF WAV files, decodes to PCM, concatenates into a single stream, and generates the
     * metadata JSON file with SHA256 checksums.
     *
     * \return True if conversion completed successfully and output files were written, false on critical failure.
     */
    bool Convert() noexcept;

    /**
     * \brief Checks if the game data path contains valid resource files required for voice asset conversion.
     *
     * \return True if MAX.RES is accessible and the output directory is writable, false otherwise.
     */
    bool IsValid() const noexcept;

private:
    std::vector<VoiceMetadata> m_metadata;
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_output_path;
    Resource m_resource_reader;
    std::string m_language_tag;
    std::string m_dat_filename;
    std::string m_dat_sha256;
    const std::vector<ResourceEntry>& m_voice_resources;
    AudioFormat m_output_format;
    bool m_is_valid{false};

    bool ProcessVoiceResource(const ResourceEntry& entry, uint64_t& current_frame_position, SDL_IOStream* dat_file,
                              bool& format_detected) noexcept;
    bool WriteDataFile(const std::vector<uint8_t>& pcm_data) noexcept;
    bool WriteMetadataJson() noexcept;
};

}  // namespace Installer

#endif /* VOICES_HPP */
