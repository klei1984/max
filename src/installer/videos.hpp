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

#ifndef VIDEOS_HPP
#define VIDEOS_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Installer {

/**
 * \brief Source file information for a single language variant of an MVE video requiring remuxing.
 */
struct VideoLanguageSource {
    std::string language_tag;         ///< ISO language tag (e.g., "en-US", "de-DE")
    uint8_t stream_index{0};          ///< Audio stream index (0-15) in output MVE file
    uint16_t stream_mask{0};          ///< 16-bit bitmask for audio frame filtering (e.g., 0x0001, 0x0002)
    std::filesystem::path file_path;  ///< Absolute path to source MVE file for this language
};

/**
 * \brief Processing mode for an MVE video resource.
 */
enum class VideoProcessingMode {
    Copy,  ///< Simple file copy (language-independent videos)
    Remux  ///< Multi-language audio remuxing (videos with language-specific dubs)
};

/**
 * \brief Complete input specification for processing a single MVE video resource.
 *
 * This structure encapsulates all metadata required by the Videos format-parser-manager to process
 * one MVE video. The installer module creates these structures and passes them to Videos, following
 * the established design pattern used by Controls, Panels, Flics, and other format managers.
 */
struct VideoInput {
    std::string resource_name;                          ///< ResourceID identifier
    std::string output_filename;                        ///< Output MVE filename
    VideoProcessingMode processing_mode;                ///< Copy or Remux mode
    std::filesystem::path source_path;                  ///< Source file path (for Copy mode)
    std::vector<VideoLanguageSource> language_sources;  ///< Language sources (for Remux mode)
};

/**
 * \brief Metadata for a single audio language stream within a multi-language MVE video file.
 */
struct VideoLanguageStream {
    std::string language_tag;  ///< ISO language tag (e.g., "en-US", "de-DE", "fr-FR", "it-IT", "es-ES")
    uint8_t stream_index{0};   ///< Audio stream index (0-15) in MVE file for track selection during playback
    uint16_t stream_mask{0};   ///< 16-bit bitmask (e.g., 0x0001, 0x0002, 0x0004) for audio frame filtering
};

/**
 * \brief Complete metadata for a single MVE video file including multi-language audio stream mapping.
 */
struct VideoMetadata {
    std::string resource_name;                       ///< ResourceID identifier string
    std::string filename;                            ///< Output MVE filename
    std::string sha256_hash;                         ///< SHA256 checksum of final remuxed/copied MVE file
    bool has_multi_language_audio{false};            ///< True if video contains multiple audio streams (remuxed)
    std::vector<VideoLanguageStream> audio_streams;  ///< Audio language stream mappings (empty if no audio)
};

/**
 * \brief Orchestrates MVE video installation: remuxing multi-language audio streams and copying other
 * language-independent videos.
 *
 * The class manages the MVE video asset installation pipeline. For videos with multi-language audio, it delegates audio
 * stream remuxing to the MveMultiplexer component. For language-independent videos it performs a simple file copy. The
 * class generates a json metadata file describing resource-to-filename mappings, SHA256 checksums, and audio stream
 * assignments for runtime language selection during video playback.
 */
class Videos {
public:
    /**
     * \brief Constructs a Videos processor for the specified video inputs and output directory.
     *
     * \param video_inputs Vector of complete video processing specifications from installer module.
     * \param output_path Directory path where processed MVE files and json metadata will be written.
     */
    explicit Videos(std::vector<VideoInput> video_inputs, const std::filesystem::path& output_path) noexcept;

    ~Videos() noexcept;

    Videos(const Videos&) = delete;
    Videos& operator=(const Videos&) = delete;
    Videos(Videos&&) noexcept = default;
    Videos& operator=(Videos&&) noexcept = default;

    /**
     * \brief Validates that the installer is properly configured with valid English input and output directory.
     *
     * \return True if English game data path exists, output directory is writable, and resource readers initialized.
     */
    [[nodiscard]] bool IsValid() const noexcept;

    /**
     * \brief Executes the complete video installation pipeline: remuxing and copying videos, then generates json with
     * metadata for runtime asset loading and language selection.
     *
     * \return True if all videos processed successfully and json written, false on any error.
     */
    [[nodiscard]] bool ProcessVideos() noexcept;

    /**
     * \brief Retrieves the generated metadata for all processed video resources after successful ProcessVideos call.
     *
     * \return Vector of VideoMetadata structures describing each video's filename, SHA256, and audio stream mappings.
     */
    [[nodiscard]] const std::vector<VideoMetadata>& GetVideoMetadata() const noexcept;

private:
    /**
     * \brief Processes a single video resource based on its VideoInput specification.
     *
     * \param video_input Complete video processing specification from installer module.
     * \return True if video processed successfully, false on error.
     */
    [[nodiscard]] bool ProcessSingleVideo(const VideoInput& video_input) noexcept;

    /**
     * \brief Remuxes multi-language audio streams from multiple MVE files into a single MVE with parallel audio tracks.
     *
     * \param video_input Video input containing resource name, output filename, and language sources.
     * \return True if remuxing completed successfully, false on error.
     */
    [[nodiscard]] bool RemuxMultiLanguageVideo(const VideoInput& video_input) noexcept;

    /**
     * \brief Copies an MVE video file from source path to output directory.
     *
     * \param video_input Video input containing resource name, output filename, and source path.
     * \return True if file copied successfully and SHA256 calculated, false on error.
     */
    [[nodiscard]] bool CopyVideo(const VideoInput& video_input) noexcept;

    /**
     * \brief Generates and writes the json metadata file describing all processed video resources.
     *
     * \return True if JSON file written successfully, false on error.
     */
    [[nodiscard]] bool WriteMetadataJson() noexcept;

    std::vector<VideoInput> m_video_inputs;       ///< Video processing specifications from installer module
    std::filesystem::path m_output_path;          ///< Output directory for MVE files and json
    std::vector<VideoMetadata> m_video_metadata;  ///< Metadata for all processed videos
    bool m_is_valid{false};                       ///< Initialization validation flag
};

}  // namespace Installer

#endif /* VIDEOS_HPP */
