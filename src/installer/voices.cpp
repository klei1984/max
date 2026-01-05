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

#include "voices.hpp"

#include <SDL3/SDL.h>

#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>

#include "procedures.hpp"
#include "progress_event.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

Voices::Voices(const std::filesystem::path& game_data_path, const std::string& language_tag,
               const std::filesystem::path& output_path, const std::vector<ResourceEntry>& voice_resources) noexcept
    : m_game_data_path(game_data_path),
      m_language_tag(language_tag),
      m_output_path(output_path),
      m_voice_resources(voice_resources),
      m_resource_reader(game_data_path, "") {
    m_is_valid =
        m_resource_reader.IsValid() && std::filesystem::is_directory(output_path) && !m_voice_resources.empty();
}

Voices::~Voices() noexcept {}

bool Voices::IsValid() const noexcept { return m_is_valid; }

bool Voices::Convert() noexcept {
    if (!m_is_valid) {
        return false;
    }

    auto dat_filename = m_language_tag + ".voices.dat";
    auto dat_filepath = m_output_path / dat_filename;

    SDL_IOStream* dat_file = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!dat_file) {
        return false;
    }

    uint64_t current_frame_position = 0;
    size_t processed_count = 0;
    size_t success_count = 0;
    size_t fail_count = 0;
    const size_t total_count = m_voice_resources.size();
    uint64_t last_progress_time = 0;
    bool format_detected = false;

    for (const auto& entry : m_voice_resources) {
        const uint64_t current_time = SDL_GetTicks();

        if (current_time - last_progress_time >= 250) {
            PushProgressDetailedEvent(success_count, fail_count, total_count);
            last_progress_time = current_time;
        }

        if (ProcessVoiceResource(entry, current_frame_position, dat_file, format_detected)) {
            ++success_count;

        } else {
            ++fail_count;
        }

        ++processed_count;
    }

    PushProgressDetailedEvent(success_count, fail_count, total_count);

    SDL_CloseIO(dat_file);

    if (m_metadata.empty()) {
        // No voice resources were successfully processed, delete the empty file
        std::filesystem::remove(dat_filepath);
        return false;
    }

    m_dat_filename = m_language_tag + ".voices.dat";
    m_dat_sha256 = ComputeFileSHA256(dat_filepath);

    if (m_dat_sha256.empty()) {
        return false;
    }

    if (!WriteMetadataJson()) {
        return false;
    }

    return true;
}

bool Voices::ProcessVoiceResource(const ResourceEntry& entry, uint64_t& current_frame_position, SDL_IOStream* dat_file,
                                  bool& format_detected) noexcept {
    auto resource_data = m_resource_reader.ReadResource(entry.resource_name);

    if (resource_data.empty()) {
        return false;
    }

    std::string voice_file_path_relative(reinterpret_cast<const char*>(resource_data.data()));
    auto voice_file_path = m_game_data_path / voice_file_path_relative;

    if (!std::filesystem::exists(voice_file_path)) {
        return false;
    }

    Sound decoder(voice_file_path);

    if (!decoder.IsValid()) {
        return false;
    }

    AudioFormat file_format = decoder.GetFormat();

    if (!format_detected) {
        m_output_format = file_format;
        format_detected = true;

    } else if (file_format != m_output_format) {
        return false;
    }

    uint64_t frame_count;
    LoopPoints loop_points = decoder.GetLoopPoints();

    if (!decoder.DecodeToFile(dat_file, frame_count)) {
        return false;
    }

    if (frame_count == 0) {
        return false;
    }

    // Create metadata entry
    VoiceMetadata metadata;

    metadata.resource_name = entry.resource_name;
    metadata.pcm_start_frame = current_frame_position;
    metadata.pcm_end_frame = current_frame_position + frame_count;
    metadata.loop_start_frame = loop_points.start_frame;
    metadata.loop_end_frame = loop_points.end_frame;
    metadata.priority = entry.priority;

    m_metadata.push_back(metadata);

    // Update current position for next sample
    current_frame_position += frame_count;

    return true;
}

bool Voices::WriteDataFile(const std::vector<uint8_t>& pcm_data) noexcept {
    auto dat_filename = m_language_tag + ".voices.dat";
    auto dat_filepath = m_output_path / dat_filename;
    SDL_IOStream* io = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!io) {
        return false;
    }

    size_t bytes_written = SDL_WriteIO(io, pcm_data.data(), pcm_data.size());

    SDL_CloseIO(io);

    if (bytes_written != pcm_data.size()) {
        return false;
    }

    return true;
}

bool Voices::WriteMetadataJson() noexcept {
    auto json_filename = m_language_tag + ".voices.json";
    auto json_filepath = m_output_path / json_filename;

    try {
        json j;

        j["$schema"] = "http://json-schema.org/draft-07/schema#";
        j["schema_version"] = "1.0.0";
        j["author"] = "Interplay Productions";
        j["copyright"] = "(c) 1996 Interplay Productions";
        j["license"] = "All rights reserved";
        j["language"] = m_language_tag;
        j["dat_filename"] = m_dat_filename;
        j["dat_sha256"] = m_dat_sha256;

        j["audio_format"] = {
            {"sample_rate", m_output_format.sample_rate},
            {"channels", m_output_format.channels},
            {"bit_depth", m_output_format.bit_depth},
        };

        json voices_object = json::object();

        for (const auto& meta : m_metadata) {
            json voice_entry = {
                {"pcm_start_frame", meta.pcm_start_frame},
                {"pcm_end_frame", meta.pcm_end_frame},
                {"priority", meta.priority},
            };

            // Add loop points only if present
            if (meta.loop_end_frame > meta.loop_start_frame) {
                voice_entry["loop_start_frame"] = meta.loop_start_frame;
                voice_entry["loop_end_frame"] = meta.loop_end_frame;
            }

            voices_object[meta.resource_name] = voice_entry;
        }

        j["voices"] = voices_object;

        std::ofstream output_file(json_filepath);

        if (!output_file.is_open()) {
            return false;
        }

        output_file << j.dump(1, '\t') << std::endl;
        output_file.close();

        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

}  // namespace Installer
