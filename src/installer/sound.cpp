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

#include "sound.hpp"

#include <SDL3/SDL.h>

#ifdef MAX_INSTALLER_SHARED
#define MINIAUDIO_IMPLEMENTATION
#endif

#include <miniaudio.h>

namespace Installer {

static inline uint32_t MapFormatToBitDepth(ma_format format) noexcept {
    uint32_t result;

    switch (format) {
        case ma_format_u8: {
            result = 8;
        } break;

        case ma_format_s16: {
            result = 16;
        } break;

        case ma_format_s24: {
            result = 24;
        } break;

        case ma_format_s32: {
            result = 32;
        } break;

        default: {
            result = 0;
        } break;
    }

    return result;
}

Sound::Sound(const std::filesystem::path& file_path) noexcept : m_file_path(file_path) {
    m_is_valid = InitializeDecoder();

    if (m_is_valid) {
        ExtractLoopPointsFromFile();
    }
}

Sound::~Sound() noexcept {
    if (m_decoder) {
        ma_decoder_uninit(m_decoder.get());
    }
}

bool Sound::InitializeDecoder() noexcept {
    if (!std::filesystem::exists(m_file_path) || !std::filesystem::is_regular_file(m_file_path)) {
        return false;
    }

    m_decoder = std::make_unique<ma_decoder>();

    if (!m_decoder) {
        return false;
    }

    // Configure decoder to use native format
    ma_decoder_config config = ma_decoder_config_init_default();

    // Initialize decoder from file
    ma_result result = ma_decoder_init_file(m_file_path.string().c_str(), &config, m_decoder.get());

    if (result != MA_SUCCESS) {
        m_decoder.reset();

        return false;
    }

    // Query actual input format
    ma_format input_format;
    ma_uint32 input_channels;
    ma_uint32 input_sample_rate;

    result =
        ma_decoder_get_data_format(m_decoder.get(), &input_format, &input_channels, &input_sample_rate, nullptr, 0);

    if (result != MA_SUCCESS) {
        ma_decoder_uninit(m_decoder.get());
        m_decoder.reset();

        return false;
    }

    // Store detected format
    m_format.bit_depth = MapFormatToBitDepth(input_format);
    m_format.channels = input_channels;
    m_format.sample_rate = input_sample_rate;

    return true;
}

bool Sound::ExtractLoopPointsFromFile() noexcept {
    // Extract loop points from RIFF WAV smpl chunk (adapted from SoundManager::LoadLoopPoints)
    SDL_IOStream* io = SDL_IOFromFile(m_file_path.string().c_str(), "rb");

    if (!io) {
        return false;
    }

    char chunk_id[4];
    uint32_t chunk_size;

    m_loop_points = LoopPoints{};

    // Read RIFF header
    if (SDL_ReadIO(io, chunk_id, sizeof(chunk_id)) != sizeof(chunk_id)) {
        SDL_CloseIO(io);

        return false;
    }

    if (SDL_strncmp(chunk_id, "RIFF", sizeof(chunk_id)) != 0) {
        SDL_CloseIO(io);

        return false;
    }

    // Read RIFF chunk size
    if (!SDL_ReadU32LE(io, &chunk_size)) {
        SDL_CloseIO(io);

        return false;
    }

    // Read WAVE format
    if (SDL_ReadIO(io, chunk_id, sizeof(chunk_id)) != sizeof(chunk_id)) {
        SDL_CloseIO(io);

        return false;
    }

    if (SDL_strncmp(chunk_id, "WAVE", sizeof(chunk_id)) != 0) {
        SDL_CloseIO(io);

        return false;
    }

    // Iterate through chunks looking for 'smpl'
    while (SDL_ReadIO(io, chunk_id, sizeof(chunk_id)) == sizeof(chunk_id)) {
        if (!SDL_ReadU32LE(io, &chunk_size)) {
            break;
        }

        if (SDL_strncmp(chunk_id, "smpl", sizeof(chunk_id)) == 0) {
            // Found sampler chunk - read fields individually for endianness safety
            uint32_t manufacturer, product, sample_period, midi_unity_note, midi_pitch_fraction, smpte_format,
                smpte_offset, num_sample_loops, sampler_data;

            if (!SDL_ReadU32LE(io, &manufacturer) || !SDL_ReadU32LE(io, &product) ||
                !SDL_ReadU32LE(io, &sample_period) || !SDL_ReadU32LE(io, &midi_unity_note) ||
                !SDL_ReadU32LE(io, &midi_pitch_fraction) || !SDL_ReadU32LE(io, &smpte_format) ||
                !SDL_ReadU32LE(io, &smpte_offset) || !SDL_ReadU32LE(io, &num_sample_loops) ||
                !SDL_ReadU32LE(io, &sampler_data)) {
                break;
            }

            // Read first loop (only one loop supported)
            if (num_sample_loops > 0) {
                uint32_t cue_point_id, type, start, end, fraction, play_count;

                if (SDL_ReadU32LE(io, &cue_point_id) && SDL_ReadU32LE(io, &type) && SDL_ReadU32LE(io, &start) &&
                    SDL_ReadU32LE(io, &end) && SDL_ReadU32LE(io, &fraction) && SDL_ReadU32LE(io, &play_count)) {
                    // Convert sample offsets to PCM frames at native sample rate
                    m_loop_points.start_frame =
                        (static_cast<uint64_t>(start) * m_format.sample_rate * sample_period) / 1000000000ULL;

                    m_loop_points.end_frame =
                        (static_cast<uint64_t>(end) * m_format.sample_rate * sample_period) / 1000000000ULL;
                }
            }

            break;
        }

        // Skip to next chunk
        if (SDL_SeekIO(io, chunk_size, SDL_IO_SEEK_CUR) < 0) {
            break;
        }
    }

    SDL_CloseIO(io);

    return true;
}

bool Sound::DecodeToFile(SDL_IOStream* output_stream, uint64_t& out_frame_count) noexcept {
    if (!m_is_valid || !m_decoder || !output_stream) {
        out_frame_count = 0;
        return false;
    }

    constexpr size_t kChunkSizeBytes = 1024u * 1024u;
    const size_t bytes_per_frame = m_format.channels * (m_format.bit_depth / 8);
    const size_t frames_per_chunk = kChunkSizeBytes / bytes_per_frame;

    if (frames_per_chunk == 0) {
        out_frame_count = 0;
        return false;
    }

    // Allocate single reusable chunk buffer
    std::vector<uint8_t> chunk_buffer(frames_per_chunk * bytes_per_frame);

    ma_uint64 total_frames_decoded = 0;
    ma_result result = MA_SUCCESS;

    // Decode and write in chunks
    while (result == MA_SUCCESS) {
        ma_uint64 frames_read = 0;

        result = ma_decoder_read_pcm_frames(m_decoder.get(), chunk_buffer.data(), frames_per_chunk, &frames_read);

        if (frames_read > 0) {
            const size_t bytes_to_write = frames_read * bytes_per_frame;
            const size_t bytes_written = SDL_WriteIO(output_stream, chunk_buffer.data(), bytes_to_write);

            if (bytes_written != bytes_to_write) {
                out_frame_count = 0;

                return false;
            }

            total_frames_decoded += frames_read;
        }

        // Break on end of stream or error (except AT_END which is normal termination)
        if (result != MA_SUCCESS && result != MA_AT_END) {
            out_frame_count = 0;

            return false;
        }

        if (result == MA_AT_END || frames_read == 0) {
            break;
        }
    }

    out_frame_count = total_frames_decoded;

    return true;
}

}  // namespace Installer
