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

#ifndef MVE_PARSER_HPP
#define MVE_PARSER_HPP

#include <SDL3/SDL.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Installer {

namespace MveFormat {
constexpr char TAG[] = "Interplay MVE File\x1A";
constexpr size_t TAG_SIZE = 20;
constexpr uint16_t MAGIC_SEPARATOR = 0x1A;
constexpr uint16_t FORMAT_VERSION = 0x100;
}  // namespace MveFormat

/**
 * \brief MVE opcode types defining operations within video chunks.
 */
enum MveOpcode : uint8_t {
    MVE_OPCODE_END_OF_STREAM = 0x00,        ///< Signals end of video stream
    MVE_OPCODE_END_OF_CHUNK = 0x01,         ///< Signals end of current chunk
    MVE_OPCODE_CREATE_TIMER = 0x02,         ///< Initialize playback timer
    MVE_OPCODE_ALLOC_AUDIO_BUFFERS = 0x03,  ///< Allocate audio playback buffers
    MVE_OPCODE_SYNCH_AUDIO = 0x04,          ///< Synchronize audio with video frame
    MVE_OPCODE_ALLOC_VIDEO_BUFFERS = 0x05,  ///< Allocate video frame buffers
    MVE_OPCODE_DECOMP_VIDEO0 = 0x06,        ///< Decompress video frame (method 0)
    MVE_OPCODE_SHOW_VIDEO_FRAME = 0x07,     ///< Display current video frame
    MVE_OPCODE_AUDIO_FRAME = 0x08,          ///< Audio frame with ADPCM compressed data
    MVE_OPCODE_SILENCE_FRAME = 0x09,        ///< Silent audio frame (placeholder)
};

/**
 * \brief MVE opcode header structure (4 bytes, little-endian).
 */
struct __attribute__((packed)) MveOpcodeHeader {
    uint16_t size;    ///< Opcode data size (excluding this 4-byte header)
    uint8_t opcode;   ///< Opcode type (MveOpcode enum value)
    uint8_t version;  ///< Opcode version for format compatibility
};

/**
 * \brief MVE audio frame opcode payload structure (6 bytes, little-endian).
 */
struct __attribute__((packed)) MveOpcodeAudioFrame {
    uint16_t seq_index;  ///< Sequential audio frame index for decoder state
    uint16_t mask;       ///< Stream mask bitmask (1 << stream_index)
    uint16_t length;     ///< Audio data length in bytes (may differ from opcode size)
};

/**
 * \brief Describes a single language audio source for MVE multiplexing operations.
 */
struct MveLanguageSource {
    std::string language_tag;         ///< ISO language tag (e.g., "en-US", "de-DE")
    uint8_t stream_index{0};          ///< Audio stream index (0-15) in output MVE
    uint16_t stream_mask{0};          ///< Bitmask for AUDIO_FRAME opcode (1 << stream_index)
    std::filesystem::path file_path;  ///< Path to source MVE file for this language
};

/**
 * \brief Result of an MVE multiplexing operation with stream metadata.
 */
struct MveRemuxResult {
    bool success{false};                               ///< True if remuxing completed successfully
    std::vector<MveLanguageSource> processed_streams;  ///< List of language streams in output file
    size_t chunks_processed{0};                        ///< Number of MVE chunks processed
};

/**
 * \brief MVE video format parser and multi-language audio multiplexer for Interplay MVE files.
 *
 * This class provides low-level MVE file format handling including parsing, validation, and remuxing of audio streams
 * from multiple single-language MVE files into a unified multi-language MVE file.
 *
 * Key design details:
 * - N AUDIO opcodes + 1 SILENCE opcode (inverted mask)
 * - Preserves original ADPCM sequence numbers (critical for per-language compression state)
 * - Writes SYNCH_AUDIO once per frame (frame-level timing, not language-level)
 * - Results in 4 opcodes per frame for 3 languages (vs 2 opcodes for 1 language)
 */
class MveMultiplexer {
public:
    /**
     * \brief Constructs an MveMultiplexer with specified language sources and output path.
     *
     * \param sources Vector of language source definitions (file paths, stream indices, masks). First source is
     * primary.
     * \param output_path Output file path for the remuxed multi-language MVE file.
     */
    explicit MveMultiplexer(std::vector<MveLanguageSource> sources, std::filesystem::path output_path) noexcept;

    ~MveMultiplexer() noexcept;

    MveMultiplexer(const MveMultiplexer&) = delete;
    MveMultiplexer& operator=(const MveMultiplexer&) = delete;
    MveMultiplexer(MveMultiplexer&&) noexcept = default;
    MveMultiplexer& operator=(MveMultiplexer&&) noexcept = default;

    /**
     * \brief Executes the MVE remuxing operation, combining audio streams from all sources into output file.
     *
     * Progress is reported to the main thread via SDL events.
     *
     * \return MveRemuxResult containing success status, error message (if failed), and processed stream metadata.
     */
    [[nodiscard]] MveRemuxResult Execute() noexcept;

private:
    /**
     * \brief Internal state tracking for each language source during remuxing.
     */
    struct SourceState {
        SDL_IOStream* file{nullptr};  ///< Open file handle for reading
        struct {
            uint16_t size{0};    ///< Next chunk size from control block
            uint8_t opcode{0};   ///< Next chunk type
            uint8_t version{0};  ///< Next chunk version
        } current_control;       ///< Current MVE chunk control block for forward chaining
    };

    /**
     * \brief Opens all source files and validates MVE headers.
     *
     * \param source_states Output vector to populate with initialized source states.
     * \param header_tags Output: MVE header tags from each source (for writing to output).
     * \param next_chunk_types Output: First chunk types from each source header.
     * \param next_chunk_versions Output: First chunk versions from each source header.
     * \return True if all sources opened and validated successfully, false on error.
     */
    [[nodiscard]] bool OpenAndValidateSources(std::vector<SourceState>& source_states,
                                              std::vector<char[MveFormat::TAG_SIZE]>& header_tags,
                                              std::vector<uint8_t>& next_chunk_types,
                                              std::vector<uint8_t>& next_chunk_versions) noexcept;

    /**
     * \brief Reads chunk data from all source files in parallel.
     *
     * \param source_states Source state objects with open file handles.
     * \param chunk_buffers Output buffers for chunk data from each source.
     * \param current_sizes Saved chunk sizes before reading (for correct parsing).
     * \param end_of_stream Output: Set to true if end-of-stream reached.
     * \return True if chunks read successfully from all sources, false on error.
     */
    [[nodiscard]] bool ReadChunksFromAllSources(std::vector<SourceState>& source_states,
                                                std::vector<std::vector<uint8_t>>& chunk_buffers,
                                                std::vector<uint16_t>& current_sizes, bool& end_of_stream) noexcept;

    /**
     * \brief Remuxes opcodes from a single chunk, merging audio from all language sources.
     *
     * \param chunk_buffers Chunk data from all language sources (index 0 is primary).
     * \param data_size Size of chunk data (excluding trailing control block).
     * \param remuxed_chunk Output buffer for remuxed chunk data.
     * \param end_of_stream Output: Set to true if END_OF_STREAM opcode encountered.
     * \return True if chunk remuxed successfully, false on error.
     */
    [[nodiscard]] bool RemuxChunkOpcodes(const std::vector<std::vector<uint8_t>>& chunk_buffers, size_t data_size,
                                         std::vector<uint8_t>& remuxed_chunk, bool& end_of_stream) noexcept;

    /**
     * \brief Writes the final output file with all remuxed chunks and corrected control blocks.
     *
     * \param all_remuxed_chunks Vector of all remuxed chunk buffers.
     * \param header_tag MVE header tag to write (from primary source).
     * \param first_chunk_type First chunk type byte for header.
     * \param first_chunk_version First chunk version byte for header.
     * \return True if file written successfully, false on error.
     */
    [[nodiscard]] bool WriteOutputFile(const std::vector<std::vector<uint8_t>>& all_remuxed_chunks,
                                       const char header_tag[20], uint8_t first_chunk_type,
                                       uint8_t first_chunk_version) noexcept;

    /**
     * \brief Closes all source files and cleans up resources.
     *
     * \param source_states Source state objects with file handles to close.
     */
    void CloseSources(std::vector<SourceState>& source_states) noexcept;

    std::vector<MveLanguageSource> m_sources;  ///< Language source definitions
    std::filesystem::path m_output_path;       ///< Output file path
};

}  // namespace Installer

#endif /* MVE_PARSER_HPP */
