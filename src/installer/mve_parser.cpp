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

#include "mve_parser.hpp"

namespace Installer {

MveMultiplexer::MveMultiplexer(std::vector<MveLanguageSource> sources, std::filesystem::path output_path) noexcept
    : m_sources(std::move(sources)), m_output_path(std::move(output_path)) {}

MveMultiplexer::~MveMultiplexer() noexcept {}

MveRemuxResult MveMultiplexer::Execute() noexcept {
    MveRemuxResult result;

    if (m_sources.empty()) {
        return result;
    }

    std::vector<SourceState> source_states(m_sources.size());
    std::vector<char[MveFormat::TAG_SIZE]> header_tags(m_sources.size());
    std::vector<uint8_t> next_chunk_types(m_sources.size());
    std::vector<uint8_t> next_chunk_versions(m_sources.size());

    // Open and validate all source files
    if (!OpenAndValidateSources(source_states, header_tags, next_chunk_types, next_chunk_versions)) {
        CloseSources(source_states);

        return result;
    }

    // Build all remuxed chunks
    std::vector<std::vector<uint8_t>> all_remuxed_chunks;
    std::vector<std::vector<uint8_t>> all_chunk_buffers(m_sources.size());
    bool end_of_stream = false;
    size_t chunks_processed = 0;
    bool success = true;

    while (success && !end_of_stream) {
        // Save current control block sizes before reading
        std::vector<uint16_t> current_sizes(m_sources.size());

        for (size_t i = 0; i < m_sources.size(); ++i) {
            current_sizes[i] = source_states[i].current_control.size;
        }

        // Read chunk data from all sources
        if (!ReadChunksFromAllSources(source_states, all_chunk_buffers, current_sizes, end_of_stream)) {
            success = false;

            break;
        }

        if (end_of_stream) {
            break;
        }

        // Remux opcodes in this chunk
        std::vector<uint8_t> remuxed_chunk;

        remuxed_chunk.reserve(all_chunk_buffers[0].size() * m_sources.size());

        if (!RemuxChunkOpcodes(all_chunk_buffers, current_sizes[0], remuxed_chunk, end_of_stream)) {
            success = false;

            break;
        }

        // Extract next control block from primary source
        uint16_t next_control_size;
        uint8_t next_control_opcode, next_control_version;

        if (!end_of_stream) {
            SDL_IOStream* next_control_io =
                SDL_IOFromConstMem(all_chunk_buffers[0].data() + current_sizes[0], sizeof(MveOpcodeHeader));

            SDL_ReadU16LE(next_control_io, &next_control_size);
            SDL_ReadU8(next_control_io, &next_control_opcode);
            SDL_ReadU8(next_control_io, &next_control_version);
            SDL_CloseIO(next_control_io);

        } else {
            next_control_size = 0;
            next_control_opcode = 0;
            next_control_version = 0;
        }

        // Append next control block to remuxed chunk
        size_t control_start = remuxed_chunk.size();

        remuxed_chunk.resize(remuxed_chunk.size() + sizeof(MveOpcodeHeader));

        SDL_IOStream* control_write_io = SDL_IOFromMem(remuxed_chunk.data() + control_start, sizeof(MveOpcodeHeader));

        SDL_WriteU16LE(control_write_io, next_control_size);
        SDL_WriteU8(control_write_io, next_control_opcode);
        SDL_WriteU8(control_write_io, next_control_version);
        SDL_CloseIO(control_write_io);

        all_remuxed_chunks.push_back(std::move(remuxed_chunk));

        ++chunks_processed;
    }

    CloseSources(source_states);

    if (!success) {
        return result;
    }

    // Write output file
    if (!all_remuxed_chunks.empty()) {
        if (!WriteOutputFile(all_remuxed_chunks, header_tags[0], next_chunk_types[0], next_chunk_versions[0])) {
            return result;
        }
    }

    // Build successful result
    result.success = true;
    result.chunks_processed = chunks_processed;
    result.processed_streams = m_sources;

    return result;
}

bool MveMultiplexer::OpenAndValidateSources(std::vector<SourceState>& source_states,
                                            std::vector<char[MveFormat::TAG_SIZE]>& header_tags,
                                            std::vector<uint8_t>& next_chunk_types,
                                            std::vector<uint8_t>& next_chunk_versions) noexcept {
    std::vector<uint16_t> next_chunk_sizes(m_sources.size());

    for (size_t i = 0; i < m_sources.size(); ++i) {
        source_states[i].file = SDL_IOFromFile(m_sources[i].file_path.string().c_str(), "rb");

        if (!source_states[i].file) {
            return false;
        }

        if (SDL_ReadIO(source_states[i].file, header_tags[i], MveFormat::TAG_SIZE) != MveFormat::TAG_SIZE) {
            return false;
        }

        uint16_t magic_separator, format_version, checksum;

        SDL_ReadU16LE(source_states[i].file, &magic_separator);
        SDL_ReadU16LE(source_states[i].file, &format_version);
        SDL_ReadU16LE(source_states[i].file, &checksum);
        SDL_ReadU16LE(source_states[i].file, &next_chunk_sizes[i]);
        SDL_ReadU8(source_states[i].file, &next_chunk_types[i]);
        SDL_ReadU8(source_states[i].file, &next_chunk_versions[i]);

        if (SDL_strncmp(header_tags[i], MveFormat::TAG, MveFormat::TAG_SIZE) != 0 ||
            format_version != MveFormat::FORMAT_VERSION) {
            return false;
        }

        // Initialize control block for chunk chaining
        source_states[i].current_control.size = next_chunk_sizes[i];
        source_states[i].current_control.opcode = next_chunk_types[i];
        source_states[i].current_control.version = next_chunk_versions[i];
    }

    return true;
}

bool MveMultiplexer::ReadChunksFromAllSources(std::vector<SourceState>& source_states,
                                              std::vector<std::vector<uint8_t>>& chunk_buffers,
                                              std::vector<uint16_t>& current_sizes, bool& end_of_stream) noexcept {
    for (size_t i = 0; i < m_sources.size(); ++i) {
        const size_t total_read_size = current_sizes[i] + sizeof(MveOpcodeHeader);

        chunk_buffers[i].resize(total_read_size);

        if (total_read_size > 0) {
            size_t bytes_read = SDL_ReadIO(source_states[i].file, chunk_buffers[i].data(), total_read_size);

            if (bytes_read != total_read_size) {
                if (bytes_read == 0 && SDL_GetIOStatus(source_states[i].file) == SDL_IO_STATUS_EOF) {
                    end_of_stream = true;

                    return true;
                }

                return false;
            }
        }

        // Update control block for next iteration
        SDL_IOStream* control_io =
            SDL_IOFromConstMem(chunk_buffers[i].data() + current_sizes[i], sizeof(MveOpcodeHeader));

        SDL_ReadU16LE(control_io, &source_states[i].current_control.size);
        SDL_ReadU8(control_io, &source_states[i].current_control.opcode);
        SDL_ReadU8(control_io, &source_states[i].current_control.version);

        SDL_CloseIO(control_io);
    }

    return true;
}

bool MveMultiplexer::RemuxChunkOpcodes(const std::vector<std::vector<uint8_t>>& chunk_buffers, size_t data_size,
                                       std::vector<uint8_t>& remuxed_chunk, bool& end_of_stream) noexcept {
    const auto& primary_chunk = chunk_buffers[0];
    size_t parse_offset = 0;

    while (parse_offset < data_size) {
        if (parse_offset + sizeof(MveOpcodeHeader) > data_size) {
            return false;
        }

        // Read opcode header
        SDL_IOStream* opcode_io = SDL_IOFromConstMem(primary_chunk.data() + parse_offset, sizeof(MveOpcodeHeader));
        uint16_t opcode_size;
        uint8_t opcode_type, opcode_version;

        SDL_ReadU16LE(opcode_io, &opcode_size);
        SDL_ReadU8(opcode_io, &opcode_type);
        SDL_ReadU8(opcode_io, &opcode_version);
        SDL_CloseIO(opcode_io);

        const size_t opcode_total_size = sizeof(MveOpcodeHeader) + opcode_size;

        if (parse_offset + opcode_total_size > data_size) {
            return false;
        }

        if (opcode_type == MVE_OPCODE_AUDIO_FRAME) {
            // AUDIO FRAME: Write one opcode per language with language-specific ADPCM data
            const size_t audio_data_size = opcode_size - sizeof(MveOpcodeAudioFrame);

            for (size_t lang_idx = 0; lang_idx < m_sources.size(); ++lang_idx) {
                const auto& lang_chunk = chunk_buffers[lang_idx];

                // Read source audio frame header
                SDL_IOStream* audio_frame_io = SDL_IOFromConstMem(
                    lang_chunk.data() + parse_offset + sizeof(MveOpcodeHeader), sizeof(MveOpcodeAudioFrame));
                uint16_t source_seq_index, source_mask, source_length;

                SDL_ReadU16LE(audio_frame_io, &source_seq_index);
                SDL_ReadU16LE(audio_frame_io, &source_mask);
                SDL_ReadU16LE(audio_frame_io, &source_length);
                SDL_CloseIO(audio_frame_io);

                const uint8_t* lang_audio_data_ptr =
                    lang_chunk.data() + parse_offset + sizeof(MveOpcodeHeader) + sizeof(MveOpcodeAudioFrame);

                // Write opcode header
                size_t opcode_start = remuxed_chunk.size();
                remuxed_chunk.resize(remuxed_chunk.size() + sizeof(MveOpcodeHeader));
                SDL_IOStream* opcode_write_io =
                    SDL_IOFromMem(remuxed_chunk.data() + opcode_start, sizeof(MveOpcodeHeader));

                SDL_WriteU16LE(opcode_write_io, opcode_size);
                SDL_WriteU8(opcode_write_io, opcode_type);
                SDL_WriteU8(opcode_write_io, opcode_version);
                SDL_CloseIO(opcode_write_io);

                // Write audio frame header with language-specific mask
                size_t audio_frame_start = remuxed_chunk.size();
                remuxed_chunk.resize(remuxed_chunk.size() + sizeof(MveOpcodeAudioFrame));
                SDL_IOStream* audio_frame_write_io =
                    SDL_IOFromMem(remuxed_chunk.data() + audio_frame_start, sizeof(MveOpcodeAudioFrame));

                SDL_WriteU16LE(audio_frame_write_io, source_seq_index);
                SDL_WriteU16LE(audio_frame_write_io, m_sources[lang_idx].stream_mask);
                SDL_WriteU16LE(audio_frame_write_io, source_length);
                SDL_CloseIO(audio_frame_write_io);

                // Append audio data
                remuxed_chunk.insert(remuxed_chunk.end(), lang_audio_data_ptr, lang_audio_data_ptr + audio_data_size);
            }

        } else if (opcode_type == MVE_OPCODE_SILENCE_FRAME) {
            // SILENCE FRAME: Write ONE opcode with INVERTED mask (Baldur's Gate pattern)
            uint16_t used_languages_mask = 0;

            for (const auto& source : m_sources) {
                used_languages_mask |= source.stream_mask;
            }

            uint16_t unused_languages_mask = ~used_languages_mask;

            // Read silence parameters from primary source
            SDL_IOStream* silence_frame_io = SDL_IOFromConstMem(
                primary_chunk.data() + parse_offset + sizeof(MveOpcodeHeader), sizeof(MveOpcodeAudioFrame));
            uint16_t silence_seq_index, silence_mask, silence_length;

            SDL_ReadU16LE(silence_frame_io, &silence_seq_index);
            SDL_ReadU16LE(silence_frame_io, &silence_mask);
            SDL_ReadU16LE(silence_frame_io, &silence_length);
            SDL_CloseIO(silence_frame_io);

            // Write silence opcode header
            size_t silence_opcode_start = remuxed_chunk.size();
            remuxed_chunk.resize(remuxed_chunk.size() + sizeof(MveOpcodeHeader));
            SDL_IOStream* silence_opcode_write_io =
                SDL_IOFromMem(remuxed_chunk.data() + silence_opcode_start, sizeof(MveOpcodeHeader));

            SDL_WriteU16LE(silence_opcode_write_io, opcode_size);
            SDL_WriteU8(silence_opcode_write_io, opcode_type);
            SDL_WriteU8(silence_opcode_write_io, opcode_version);
            SDL_CloseIO(silence_opcode_write_io);

            // Write silence frame with inverted mask
            size_t silence_frame_start = remuxed_chunk.size();
            remuxed_chunk.resize(remuxed_chunk.size() + sizeof(MveOpcodeAudioFrame));
            SDL_IOStream* silence_frame_write_io =
                SDL_IOFromMem(remuxed_chunk.data() + silence_frame_start, sizeof(MveOpcodeAudioFrame));

            SDL_WriteU16LE(silence_frame_write_io, silence_seq_index);
            SDL_WriteU16LE(silence_frame_write_io, unused_languages_mask);
            SDL_WriteU16LE(silence_frame_write_io, silence_length);
            SDL_CloseIO(silence_frame_write_io);

            // Check for following SYNCH_AUDIO opcode
            const size_t next_opcode_offset = parse_offset + opcode_total_size;

            if (next_opcode_offset + sizeof(MveOpcodeHeader) <= data_size) {
                SDL_IOStream* next_opcode_io =
                    SDL_IOFromConstMem(primary_chunk.data() + next_opcode_offset, sizeof(MveOpcodeHeader));
                uint16_t next_opcode_size;
                uint8_t next_opcode_type, next_opcode_version;

                SDL_ReadU16LE(next_opcode_io, &next_opcode_size);
                SDL_ReadU8(next_opcode_io, &next_opcode_type);
                SDL_ReadU8(next_opcode_io, &next_opcode_version);
                SDL_CloseIO(next_opcode_io);

                if (next_opcode_type == MVE_OPCODE_SYNCH_AUDIO) {
                    const size_t sync_total_size = sizeof(MveOpcodeHeader) + next_opcode_size;

                    remuxed_chunk.insert(remuxed_chunk.end(), primary_chunk.data() + next_opcode_offset,
                                         primary_chunk.data() + next_opcode_offset + sync_total_size);
                    parse_offset += sync_total_size;
                }
            }

        } else if (opcode_type == MVE_OPCODE_END_OF_STREAM) {
            // Copy end of stream opcode
            remuxed_chunk.insert(remuxed_chunk.end(), primary_chunk.data() + parse_offset,
                                 primary_chunk.data() + parse_offset + opcode_total_size);
            end_of_stream = true;

            break;

        } else if (opcode_type == MVE_OPCODE_END_OF_CHUNK) {
            // End of chunk marker - copy and stop
            remuxed_chunk.insert(remuxed_chunk.end(), primary_chunk.data() + parse_offset,
                                 primary_chunk.data() + parse_offset + opcode_total_size);
            parse_offset += opcode_total_size;

            break;

        } else {
            // Copy all other opcodes directly (video, sync, timer, palette, etc.)
            remuxed_chunk.insert(remuxed_chunk.end(), primary_chunk.data() + parse_offset,
                                 primary_chunk.data() + parse_offset + opcode_total_size);
        }

        parse_offset += opcode_total_size;
    }

    return true;
}

bool MveMultiplexer::WriteOutputFile(const std::vector<std::vector<uint8_t>>& all_remuxed_chunks,
                                     const char header_tag[MveFormat::TAG_SIZE], uint8_t first_chunk_type,
                                     uint8_t first_chunk_version) noexcept {
    SDL_IOStream* output_file = SDL_IOFromFile(m_output_path.string().c_str(), "wb");

    if (!output_file) {
        return false;
    }

    bool success = true;

    // Calculate first chunk size
    const uint16_t first_chunk_size = static_cast<uint16_t>(all_remuxed_chunks[0].size() - sizeof(MveOpcodeHeader));

    // Write header
    if (SDL_WriteIO(output_file, header_tag, MveFormat::TAG_SIZE) != MveFormat::TAG_SIZE ||
        !SDL_WriteU16LE(output_file, MveFormat::MAGIC_SEPARATOR) ||
        !SDL_WriteU16LE(output_file, MveFormat::FORMAT_VERSION) ||
        !SDL_WriteU16LE(output_file, static_cast<uint16_t>(~MveFormat::FORMAT_VERSION + 0x1234)) ||
        !SDL_WriteU16LE(output_file, first_chunk_size) || !SDL_WriteU8(output_file, first_chunk_type) ||
        !SDL_WriteU8(output_file, first_chunk_version)) {
        success = false;
    }

    // Write all chunks with corrected trailing control blocks
    for (size_t i = 0; i < all_remuxed_chunks.size() && success; ++i) {
        // Make a mutable copy of the chunk for control block update
        std::vector<uint8_t> chunk = all_remuxed_chunks[i];

        // Update trailing control block size to match next chunk
        if (i + 1 < all_remuxed_chunks.size()) {
            const uint16_t next_chunk_size =
                static_cast<uint16_t>(all_remuxed_chunks[i + 1].size() - sizeof(MveOpcodeHeader));
            SDL_IOStream* trailing_control_io =
                SDL_IOFromMem(chunk.data() + chunk.size() - sizeof(MveOpcodeHeader), sizeof(MveOpcodeHeader));

            SDL_WriteU16LE(trailing_control_io, next_chunk_size);
            SDL_CloseIO(trailing_control_io);
        }

        // Write chunk
        if (SDL_WriteIO(output_file, chunk.data(), chunk.size()) != chunk.size()) {
            success = false;
        }
    }

    SDL_CloseIO(output_file);

    if (!success) {
        std::filesystem::remove(m_output_path);
    }

    return success;
}

void MveMultiplexer::CloseSources(std::vector<SourceState>& source_states) noexcept {
    for (auto& state : source_states) {
        if (state.file) {
            SDL_CloseIO(state.file);
            state.file = nullptr;
        }
    }
}

}  // namespace Installer
