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

#include "flic.hpp"

#include <SDL3/SDL.h>

namespace Installer {

constexpr uint16_t FLIC_FLC_TYPE = 0xAF12u;
constexpr uint16_t FLIC_FRAME_TYPE = 0xF1FAu;
constexpr uint16_t FLI_FRAME_TYPE = 0xF100u;
constexpr uint16_t FLIC_PALETTE_SIZE = 256u;

enum FlicChunkType {
    FLIC_CHUNK_COLOR_256 = 4,
    FLIC_CHUNK_DELTA_FLC = 7,
    FLIC_CHUNK_COLOR_64 = 11,
    FLIC_CHUNK_BLACK = 13,
    FLIC_CHUNK_BYTE_RUN = 15,
    FLIC_CHUNK_LITERAL = 16
};

struct FlicHeader {
    int32_t size;
    uint16_t type;
    uint16_t frames;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t flags;
    int32_t speed;
};

struct FrameHeader {
    int32_t size;
    uint16_t type;
    int16_t chunks;
};

struct ChunkHeader {
    int32_t size;
    uint16_t type;
};

Flic::Flic(const std::filesystem::path& flic_path) noexcept : m_flic_path(flic_path) { m_is_valid = LoadHeader(); }

Flic::~Flic() noexcept {}

bool Flic::LoadHeader() noexcept {
    SDL_IOStream* io = SDL_IOFromFile(m_flic_path.string().c_str(), "rb");

    if (!io) {
        return false;
    }

    // Read complete FLIC header (128 bytes) for endianness safety
    int32_t size;
    uint16_t type, frames, width, height, depth, flags;
    int32_t speed;
    int16_t reserved1;
    uint32_t created, creator, updated, updater;
    uint16_t aspect_dx, aspect_dy;
    uint8_t reserved2[38];
    int32_t oframe1, oframe2;
    uint8_t reserved3[40];

    if (!SDL_ReadS32LE(io, &size) || !SDL_ReadU16LE(io, &type) || !SDL_ReadU16LE(io, &frames) ||
        !SDL_ReadU16LE(io, &width) || !SDL_ReadU16LE(io, &height) || !SDL_ReadU16LE(io, &depth) ||
        !SDL_ReadU16LE(io, &flags) || !SDL_ReadS32LE(io, &speed) || !SDL_ReadS16LE(io, &reserved1) ||
        !SDL_ReadU32LE(io, &created) || !SDL_ReadU32LE(io, &creator) || !SDL_ReadU32LE(io, &updated) ||
        !SDL_ReadU32LE(io, &updater) || !SDL_ReadU16LE(io, &aspect_dx) || !SDL_ReadU16LE(io, &aspect_dy) ||
        SDL_ReadIO(io, reserved2, 38) != 38 || !SDL_ReadS32LE(io, &oframe1) || !SDL_ReadS32LE(io, &oframe2) ||
        SDL_ReadIO(io, reserved3, 40) != 40) {
        SDL_CloseIO(io);
        return false;
    }

    SDL_CloseIO(io);

    if (type != FLIC_FLC_TYPE) {
        return false;
    }

    m_width = width;
    m_height = height;
    m_frame_count = frames;
    m_frame_duration_ms = speed;
    m_first_frame_offset = oframe1;

    return true;
}

std::vector<FlicFrameData> Flic::DecodeAllFrames() noexcept {
    if (!m_is_valid) {
        return {};
    }

    SDL_IOStream* io = SDL_IOFromFile(m_flic_path.string().c_str(), "rb");

    if (!io) {
        return {};
    }

    // Seek to first frame position as specified in header
    if (SDL_SeekIO(io, m_first_frame_offset, SDL_IO_SEEK_SET) < 0) {
        SDL_CloseIO(io);
        return {};
    }

    uint8_t palette[FLIC_PALETTE_SIZE * 3];

    SDL_memset(palette, 0, sizeof(palette));

    std::vector<uint8_t> indexed_buffer(m_width * m_height);
    std::vector<FlicFrameData> frames;

    frames.reserve(m_frame_count);

    for (uint32_t frame_idx = 0; frame_idx < m_frame_count; ++frame_idx) {
        if (!DecodeFrame(io, indexed_buffer, palette)) {
            break;
        }

        // Convert indexed to RGBA8888
        FlicFrameData frame_data;

        frame_data.rgba_data.resize(m_width * m_height * 4);
        frame_data.duration_ms = m_frame_duration_ms;

        ConvertIndexedToRgba(indexed_buffer, palette, frame_data.rgba_data);

        frames.push_back(std::move(frame_data));
    }

    SDL_CloseIO(io);

    return frames;
}

bool Flic::DecodeFrame(SDL_IOStream* io, std::vector<uint8_t>& indexed_buffer, uint8_t* palette) noexcept {
    int32_t frame_size;
    uint16_t frame_type;
    int16_t chunk_count;

    if (!SDL_ReadS32LE(io, &frame_size) || !SDL_ReadU16LE(io, &frame_type) || !SDL_ReadS16LE(io, &chunk_count)) {
        return false;
    }

    // Accept both FLC (0xF1FA) and FLI (0xF100) frame types
    if (frame_type != FLIC_FRAME_TYPE && frame_type != FLI_FRAME_TYPE) {
        return false;
    }

    // Skip reserved bytes (8 bytes) to complete the 16-byte FrameHeader
    uint8_t reserved[8];

    if (SDL_ReadIO(io, reserved, 8) != 8) {
        return false;
    }

    // Calculate frame data size (total frame size minus 16-byte header)
    size_t frame_data_size = frame_size - 16;

    std::vector<uint8_t> frame_buffer(frame_data_size);

    size_t bytes_read = SDL_ReadIO(io, frame_buffer.data(), frame_data_size);

    if (bytes_read != frame_data_size) {
        return false;
    }

    // Parse chunks from frame buffer using SDL_IOStream for endianness safety
    SDL_IOStream* frame_io = SDL_IOFromMem(frame_buffer.data(), frame_data_size);

    if (!frame_io) {
        return false;
    }

    for (int16_t chunk_idx = 0; chunk_idx < chunk_count; ++chunk_idx) {
        int64_t chunk_start = SDL_TellIO(frame_io);

        if (chunk_start + 6 > static_cast<int64_t>(frame_data_size)) {
            SDL_CloseIO(frame_io);

            return false;
        }

        int32_t chunk_size;
        uint16_t chunk_type;

        if (!SDL_ReadS32LE(frame_io, &chunk_size) || !SDL_ReadU16LE(frame_io, &chunk_type)) {
            SDL_CloseIO(frame_io);

            return false;
        }

        if (chunk_size < 6) {
            SDL_CloseIO(frame_io);

            return false;
        }

        // Calculate chunk data size and position
        size_t chunk_data_size = chunk_size - 6;
        int64_t current_pos = SDL_TellIO(frame_io);

        if (current_pos + static_cast<int64_t>(chunk_data_size) > static_cast<int64_t>(frame_data_size)) {
            SDL_CloseIO(frame_io);

            return false;
        }

        // Get pointer to chunk data in the frame buffer
        const uint8_t* chunk_data = &frame_buffer[current_pos];

        switch (chunk_type) {
            case FLIC_CHUNK_COLOR_256:
                DecodeColorChunk(chunk_data, chunk_data_size, palette, 2);
                break;

            case FLIC_CHUNK_COLOR_64:
                DecodeColorChunk(chunk_data, chunk_data_size, palette, 0);
                break;

            case FLIC_CHUNK_DELTA_FLC:
                DecodeDeltaFlc(chunk_data, chunk_data_size, indexed_buffer);
                break;

            case FLIC_CHUNK_BYTE_RUN:
                DecodeByteRun(chunk_data, chunk_data_size, indexed_buffer);
                break;

            case FLIC_CHUNK_LITERAL:
                if (chunk_data_size >= indexed_buffer.size()) {
                    SDL_memcpy(indexed_buffer.data(), chunk_data, indexed_buffer.size());
                }
                break;

            case FLIC_CHUNK_BLACK:
                // Fill with color index 0 (black)
                SDL_memset(indexed_buffer.data(), 0, indexed_buffer.size());
                break;

            default:
                break;
        }

        // Seek to next chunk position
        if (SDL_SeekIO(frame_io, chunk_start + chunk_size, SDL_IO_SEEK_SET) < 0) {
            SDL_CloseIO(frame_io);
            return false;
        }
    }

    SDL_CloseIO(frame_io);
    return true;
}

void Flic::DecodeColorChunk(const uint8_t* chunk_data, size_t chunk_data_size, uint8_t* palette,
                            int32_t shift) noexcept {
    SDL_IOStream* io = SDL_IOFromMem(const_cast<uint8_t*>(chunk_data), chunk_data_size);

    if (!io) {
        return;
    }

    uint16_t packet_count_u16;

    if (!SDL_ReadU16LE(io, &packet_count_u16)) {
        SDL_CloseIO(io);
        return;
    }

    int16_t packet_count = static_cast<int16_t>(packet_count_u16);
    int32_t entry = 0;

    for (int16_t i = 0; i < packet_count; ++i) {
        uint8_t skip_count;
        uint8_t color_count;

        if (!SDL_ReadU8(io, &skip_count) || !SDL_ReadU8(io, &color_count)) {
            SDL_CloseIO(io);
            return;
        }

        entry += skip_count;

        int32_t count = color_count;

        if (count == 0) {
            count = FLIC_PALETTE_SIZE;
        }

        for (int32_t j = 0; j < count; ++j) {
            uint8_t r, g, b;

            if (!SDL_ReadU8(io, &r) || !SDL_ReadU8(io, &g) || !SDL_ReadU8(io, &b)) {
                SDL_CloseIO(io);
                return;
            }

            palette[entry * 3 + 0] = r >> shift;  // R
            palette[entry * 3 + 1] = g >> shift;  // G
            palette[entry * 3 + 2] = b >> shift;  // B

            ++entry;
        }
    }

    SDL_CloseIO(io);
}

void Flic::DecodeDeltaFlc(const uint8_t* chunk_data, size_t chunk_data_size,
                          std::vector<uint8_t>& indexed_buffer) noexcept {
    SDL_IOStream* io = SDL_IOFromMem(const_cast<uint8_t*>(chunk_data), chunk_data_size);

    if (!io) {
        return;
    }

    uint16_t line_count_u16;

    if (!SDL_ReadU16LE(io, &line_count_u16)) {
        SDL_CloseIO(io);
        return;
    }

    int16_t line_count = static_cast<int16_t>(line_count_u16);
    uint8_t* line_address = indexed_buffer.data();

    while (--line_count >= 0) {
        uint16_t opt_word_u16;

        if (!SDL_ReadU16LE(io, &opt_word_u16)) {
            SDL_CloseIO(io);
            return;
        }

        int16_t opt_word = static_cast<int16_t>(opt_word_u16);

        // Handle optional words (skip lines or set last byte)
        while ((uint16_t)opt_word & 0x8000) {
            if ((uint16_t)opt_word & 0x4000) {
                // Line skip count
                opt_word = -opt_word;
                line_address += opt_word * m_width;

            } else {
                // Set last byte of line
                line_address[m_width - 1] = opt_word & 0xFF;
            }

            if (!SDL_ReadU16LE(io, &opt_word_u16)) {
                SDL_CloseIO(io);
                return;
            }

            opt_word = static_cast<int16_t>(opt_word_u16);
        }

        int16_t packet_count = opt_word;
        uint32_t offset = 0;

        for (int16_t i = 0; i < packet_count; ++i) {
            uint8_t column_skip;
            uint8_t packet_type_u8;

            if (!SDL_ReadU8(io, &column_skip) || !SDL_ReadU8(io, &packet_type_u8)) {
                SDL_CloseIO(io);
                return;
            }

            int8_t packet_type = static_cast<int8_t>(packet_type_u8);
            offset += column_skip;

            if (packet_type >= 0) {
                // Copy literal pixels
                uint32_t byte_count = packet_type * 2;

                if (SDL_ReadIO(io, &line_address[offset], byte_count) != byte_count) {
                    SDL_CloseIO(io);
                    return;
                }

                offset += byte_count;

            } else {
                uint16_t word_u16;

                if (!SDL_ReadU16LE(io, &word_u16)) {
                    SDL_CloseIO(io);
                    return;
                }

                packet_type = -packet_type;
                for (int8_t j = 0; j < packet_type; ++j) {
                    line_address[offset] = word_u16 & 0xFF;
                    line_address[offset + 1] = (word_u16 >> 8) & 0xFF;
                    offset += 2;
                }
            }
        }

        line_address += m_width;
    }

    SDL_CloseIO(io);
}

void Flic::DecodeByteRun(const uint8_t* chunk_data, size_t chunk_data_size,
                         std::vector<uint8_t>& indexed_buffer) noexcept {
    SDL_IOStream* io = SDL_IOFromMem(const_cast<uint8_t*>(chunk_data), chunk_data_size);

    if (!io) {
        return;
    }

    uint8_t* dst = indexed_buffer.data();

    for (uint32_t y = 0; y < m_height; ++y) {
        uint8_t skip_byte;

        if (!SDL_ReadU8(io, &skip_byte)) {
            SDL_CloseIO(io);
            return;
        }

        uint32_t x = 0;

        while (x < m_width) {
            uint8_t packet_size_u8;

            if (!SDL_ReadU8(io, &packet_size_u8)) {
                SDL_CloseIO(io);
                return;
            }

            int8_t packet_size = static_cast<int8_t>(packet_size_u8);

            if (packet_size >= 0) {
                // RLE: repeat next byte
                uint8_t color;

                if (!SDL_ReadU8(io, &color)) {
                    SDL_CloseIO(io);
                    return;
                }

                SDL_memset(&dst[x], color, packet_size);
                x += packet_size;

            } else {
                // Copy literal bytes
                packet_size = -packet_size;

                if (SDL_ReadIO(io, &dst[x], packet_size) != static_cast<size_t>(packet_size)) {
                    SDL_CloseIO(io);
                    return;
                }

                x += packet_size;
            }
        }

        dst += m_width;
    }

    SDL_CloseIO(io);
}

void Flic::ConvertIndexedToRgba(const std::vector<uint8_t>& indexed_buffer, const uint8_t* palette,
                                std::vector<uint8_t>& rgba_buffer) noexcept {
    for (size_t i = 0; i < indexed_buffer.size(); ++i) {
        uint8_t index = indexed_buffer[i];

        // Convert 6-bit VGA palette to 8-bit RGB
        uint8_t r = palette[index * 3 + 0] << 2;
        uint8_t g = palette[index * 3 + 1] << 2;
        uint8_t b = palette[index * 3 + 2] << 2;
        uint8_t a = 255;

        // RGBA8888 format
        rgba_buffer[i * 4 + 0] = r;
        rgba_buffer[i * 4 + 1] = g;
        rgba_buffer[i * 4 + 2] = b;
        rgba_buffer[i * 4 + 3] = a;
    }
}

}  // namespace Installer
