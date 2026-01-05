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

#include "music.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "procedures.hpp"
#include "progress_event.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

Music::Music(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
             const std::vector<MusicTrackResourceEntry>& music_resources) noexcept
    : m_game_data_path(game_data_path),
      m_output_path(output_path),
      m_music_resources(music_resources),
      m_resource_reader(game_data_path, "") {
    m_is_valid =
        m_resource_reader.IsValid() && std::filesystem::is_directory(m_output_path) && !m_music_resources.empty();
}

Music::~Music() noexcept {}

bool Music::IsValid() const noexcept { return m_is_valid; }

bool Music::Convert() noexcept {
    if (!m_is_valid) {
        return false;
    }

    m_dat_filename = "music.dat";

    auto dat_filepath = m_output_path / m_dat_filename;
    SDL_IOStream* dat_file = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!dat_file) {
        return false;
    }

    const size_t total_count = m_music_resources.size();
    size_t processed_count = 0;
    size_t success_count = 0;
    size_t fail_count = 0;
    uint64_t last_progress_time = 0;
    uint32_t current_frame_position = 0;
    AudioFormat* expected_format = nullptr;

    for (const auto& entry : m_music_resources) {
        const uint64_t current_time = SDL_GetTicks();

        if (current_time - last_progress_time >= 250) {
            PushProgressDetailedEvent(success_count, fail_count, total_count);

            last_progress_time = current_time;
        }

        ++processed_count;

        if (ProcessMusicTrackResource(entry, current_frame_position, dat_file, expected_format)) {
            ++success_count;

            if (expected_format == nullptr) {
                expected_format = &m_format;
            }

        } else {
            ++fail_count;
        }
    }

    PushProgressDetailedEvent(success_count, fail_count, total_count);

    SDL_CloseIO(dat_file);

    if (m_metadata.empty()) {
        return false;
    }

    m_dat_sha256 = ComputeFileSHA256(dat_filepath);

    if (m_dat_sha256.empty()) {
        return false;
    }

    if (!WriteMetadataJson()) {
        return false;
    }

    return true;
}

bool Music::ProcessMusicTrackResource(const MusicTrackResourceEntry& entry, uint32_t& current_frame_position,
                                      SDL_IOStream* dat_file, AudioFormat* expected_format) noexcept {
    auto resource_data = m_resource_reader.ReadResource(entry.resource_name);

    if (resource_data.empty()) {
        return false;
    }

    std::string music_file_path_relative(reinterpret_cast<const char*>(resource_data.data()));
    auto music_file_path = m_game_data_path / music_file_path_relative;

    if (!std::filesystem::exists(music_file_path)) {
        return false;
    }

    Sound decoder(music_file_path);

    if (!decoder.IsValid()) {
        return false;
    }

    AudioFormat track_format = decoder.GetFormat();

    if (expected_format == nullptr) {
        m_format = track_format;

    } else if (track_format != *expected_format) {
        return false;
    }

    LoopPoints loop_points = decoder.GetLoopPoints();
    uint64_t frame_count;

    if (!decoder.DecodeToFile(dat_file, frame_count)) {
        return false;
    }

    if (frame_count == 0) {
        return false;
    }

    MusicTrackMetadata metadata;

    metadata.resource_name = entry.resource_name;
    metadata.pcm_start_frame = current_frame_position;
    metadata.pcm_end_frame = current_frame_position + frame_count;
    metadata.loop_start_frame = loop_points.start_frame;
    metadata.loop_end_frame = loop_points.end_frame;

    m_metadata.push_back(metadata);

    current_frame_position += frame_count;

    return true;
}

bool Music::WriteMetadataJson() noexcept {
    auto json_filepath = m_output_path / std::string("music.json");

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
            {"sample_rate", m_format.sample_rate},
            {"channels", m_format.channels},
            {"bit_depth", m_format.bit_depth},
        };

        json music_tracks_object = json::object();

        for (const auto& meta : m_metadata) {
            json music_entry = {
                {"pcm_start_frame", meta.pcm_start_frame},
                {"pcm_end_frame", meta.pcm_end_frame},
            };

            if (meta.loop_end_frame > meta.loop_start_frame) {
                music_entry["loop_start_frame"] = meta.loop_start_frame;
                music_entry["loop_end_frame"] = meta.loop_end_frame;
            }

            music_tracks_object[meta.resource_name] = music_entry;
        }

        j["music_tracks"] = music_tracks_object;

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
