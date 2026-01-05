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

#ifndef SOUND_HPP
#define SOUND_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

struct ma_decoder;
struct SDL_IOStream;

namespace Installer {

/**
 * \struct AudioFormat
 * \brief Describes the PCM audio format characteristics detected from a source RIFF WAV file during decoding.
 */
struct AudioFormat {
    uint32_t sample_rate{0};  ///< Samples per second (e.g., 22050, 44100, 48000 Hz)
    uint32_t channels{0};     ///< Number of audio channels (1 = mono, 2 = stereo)
    uint32_t bit_depth{0};    ///< Bits per sample (8, 16, 24, 32)

    bool operator==(const AudioFormat& other) const noexcept {
        return sample_rate == other.sample_rate && channels == other.channels && bit_depth == other.bit_depth;
    }

    bool operator!=(const AudioFormat& other) const noexcept { return !(*this == other); }
};

/**
 * \struct LoopPoints
 * \brief Contains sample-accurate loop point information extracted from a RIFF WAV file's 'smpl' chunk if present.
 */
struct LoopPoints {
    uint32_t start_frame{0};  ///< PCM frame position where the loop region begins (0-based index)
    uint32_t end_frame{0};    ///< PCM frame position where the loop region ends (exclusive, 0 = no loop)

    bool HasLoop() const noexcept { return end_frame > start_frame; }
};

/**
 * \class Sound
 * \brief Audio decoder for RIFF WAV files using miniaudio that extracts PCM data and loop point metadata for installer.
 */
class Sound {
public:
    /**
     * \brief Constructs a sound decoder for the specified RIFF WAV file path.
     *
     * \param file_path UTF-8 encoded filesystem path to the source RIFF WAV audio file to decode.
     */
    explicit Sound(const std::filesystem::path& file_path) noexcept;

    /**
     * \brief Destructs the sound decoder and releases the miniaudio decoder instance and allocated buffers.
     */
    ~Sound() noexcept;

    /**
     * \brief Retrieves the detected audio format characteristics from the source RIFF WAV file.
     *
     * \return AudioFormat structure containing sample rate, channel count, and bit depth of the audio stream.
     */
    AudioFormat GetFormat() const noexcept { return m_format; }

    /**
     * \brief Extracts loop point metadata from the RIFF WAV file's 'smpl' chunk if present, converted to PCM frames.
     *
     * Loop points are converted from sample offsets to PCM frame positions at the file's native sample rate.
     *
     * \return LoopPoints structure containing start/end frame positions, or default (no loop) if smpl chunk absent.
     */
    LoopPoints GetLoopPoints() const noexcept { return m_loop_points; }

    /**
     * \brief Decodes RIFF WAV file to raw PCM data, writing directly to an SDL_IOStream in memory-efficient chunks.
     *
     * \param output_stream SDL_IOStream to write decoded PCM data to (must be opened for writing).
     * \param out_frame_count Output parameter receiving the total number of PCM frames decoded (samples per channel).
     * \return True if decoding and writing succeeded, false on any error during decode or write operations.
     */
    bool DecodeToFile(SDL_IOStream* output_stream, uint64_t& out_frame_count) noexcept;

    /**
     * \brief Checks if the RIFF WAV file was successfully opened and is valid for decoding operations.
     *
     * \return True if the audio file is accessible and miniaudio decoder initialization succeeded, false otherwise.
     */
    bool IsValid() const noexcept { return m_is_valid; }

private:
    std::filesystem::path m_file_path;
    AudioFormat m_format;
    LoopPoints m_loop_points;
    std::unique_ptr<ma_decoder> m_decoder;
    bool m_is_valid{false};

    bool InitializeDecoder() noexcept;
    bool ExtractLoopPointsFromFile() noexcept;
};

}  // namespace Installer

#endif /* SOUND_HPP */
