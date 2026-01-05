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

#include "soundeffects.hpp"

#include <SDL3/SDL.h>

#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>

#include "procedures.hpp"
#include "progress_event.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

SoundEffects::SoundEffects(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
                           const std::vector<ResourceEntry>& soundeffect_resources) noexcept
    : m_game_data_path(game_data_path),
      m_output_path(output_path),
      m_resource_reader(game_data_path, ""),
      m_soundeffect_resources(soundeffect_resources) {
    m_is_valid = m_resource_reader.IsValid() && std::filesystem::is_directory(output_path);
}

SoundEffects::~SoundEffects() noexcept {}

bool SoundEffects::IsValid() const noexcept { return m_is_valid; }

bool SoundEffects::Convert() noexcept {
    if (!m_is_valid) {
        return false;
    }

    auto dat_filename = std::string("soundeffects.dat");
    auto dat_filepath = m_output_path / dat_filename;

    SDL_IOStream* dat_file = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!dat_file) {
        return false;
    }

    uint64_t current_frame_position = 0;
    size_t processed_count = 0;
    size_t success_count = 0;
    size_t fail_count = 0;
    const size_t total_count = m_soundeffect_resources.size();
    uint64_t last_progress_time = 0;

    for (const auto& entry : m_soundeffect_resources) {
        const uint64_t current_time = SDL_GetTicks();

        if (current_time - last_progress_time >= 250) {
            PushProgressDetailedEvent(success_count, fail_count, total_count);
            last_progress_time = current_time;
        }

        if (ProcessSoundEffectResource(entry, current_frame_position, dat_file)) {
            ++success_count;

        } else {
            ++fail_count;
        }

        ++processed_count;
    }

    PushProgressDetailedEvent(success_count, fail_count, total_count);

    SDL_CloseIO(dat_file);

    if (m_metadata.empty()) {
        return false;
    }

    m_dat_filename = "soundeffects.dat";
    m_dat_sha256 = ComputeFileSHA256(dat_filepath);

    if (m_dat_sha256.empty()) {
        return false;
    }

    if (!WriteMetadataJson()) {
        return false;
    }

    return true;
}

bool SoundEffects::ProcessSoundEffectResource(const ResourceEntry& entry, uint64_t& current_frame_position,
                                              SDL_IOStream* dat_file) noexcept {
    auto resource_data = m_resource_reader.ReadResource(entry.resource_name);

    if (resource_data.empty()) {
        return false;
    }

    std::string soundeffect_file_path_relative(reinterpret_cast<const char*>(resource_data.data()));
    auto soundeffect_file_path = m_game_data_path / soundeffect_file_path_relative;

    if (!std::filesystem::exists(soundeffect_file_path)) {
        return false;
    }

    // Check if this source file has already been converted (deduplication)
    std::string canonical_file_path = soundeffect_file_path.string();
    auto existing_entry = m_converted_files.find(canonical_file_path);

    if (existing_entry != m_converted_files.end()) {
        // Reuse existing PCM data and metadata from the already-converted file
        SoundEffectMetadata metadata;

        metadata.resource_name = entry.resource_name;
        metadata.pcm_start_frame = existing_entry->second.pcm_start_frame;
        metadata.pcm_end_frame = existing_entry->second.pcm_end_frame;
        metadata.loop_start_frame = existing_entry->second.loop_start_frame;
        metadata.loop_end_frame = existing_entry->second.loop_end_frame;

        m_metadata.push_back(metadata);

        return true;
    }

    Sound decoder(soundeffect_file_path);

    if (!decoder.IsValid()) {
        return false;
    }

    AudioFormat file_format = decoder.GetFormat();

    if (m_output_format.sample_rate == 0) {
        m_output_format = file_format;

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
    SoundEffectMetadata metadata;

    metadata.resource_name = entry.resource_name;
    metadata.pcm_start_frame = current_frame_position;
    metadata.pcm_end_frame = current_frame_position + frame_count;
    metadata.loop_start_frame = loop_points.start_frame;
    metadata.loop_end_frame = loop_points.end_frame;

    m_metadata.push_back(metadata);
    m_converted_files[canonical_file_path] = metadata;
    current_frame_position += frame_count;

    return true;
}

bool SoundEffects::WriteMetadataJson() noexcept {
    auto json_filename = std::string("soundeffects.json");
    auto json_filepath = m_output_path / json_filename;

    try {
        json j;

        j["$schema"] = "http://json-schema.org/draft-07/schema#";
        j["schema_version"] = "1.0.0";
        j["author"] = "Interplay Productions";
        j["copyright"] = "(c) 1996 Interplay Productions";
        j["license"] = "All rights reserved";
        j["dat_filename"] = m_dat_filename;
        j["dat_sha256"] = m_dat_sha256;
        j["audio_format"] = {
            {"sample_rate", m_output_format.sample_rate},
            {"channels", m_output_format.channels},
            {"bit_depth", m_output_format.bit_depth},
        };

        json soundeffects_object = json::object();

        for (const auto& meta : m_metadata) {
            json soundeffect_entry = {
                {"pcm_start_frame", meta.pcm_start_frame},
                {"pcm_end_frame", meta.pcm_end_frame},
            };

            if (meta.loop_end_frame > meta.loop_start_frame) {
                soundeffect_entry["loop_start_frame"] = meta.loop_start_frame;
                soundeffect_entry["loop_end_frame"] = meta.loop_end_frame;
            }

            soundeffects_object[meta.resource_name] = soundeffect_entry;
        }

        j["sound_effects"] = soundeffects_object;

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
