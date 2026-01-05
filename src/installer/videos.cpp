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

#include "videos.hpp"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#include "mve_parser.hpp"
#include "procedures.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

Videos::Videos(std::vector<VideoInput> video_inputs, const std::filesystem::path& output_path) noexcept
    : m_video_inputs(std::move(video_inputs)), m_output_path(output_path) {
    if (m_video_inputs.empty()) {
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(output_path, ec);

    if (ec) {
        return;
    }

    m_is_valid = true;
}

Videos::~Videos() noexcept {}

bool Videos::IsValid() const noexcept { return m_is_valid; }

const std::vector<VideoMetadata>& Videos::GetVideoMetadata() const noexcept { return m_video_metadata; }

bool Videos::ProcessVideos() noexcept {
    if (!m_is_valid) {
        return false;
    }

    m_video_metadata.clear();

    for (const auto& video_input : m_video_inputs) {
        if (!ProcessSingleVideo(video_input)) {
            return false;
        }
    }

    if (!WriteMetadataJson()) {
        return false;
    }

    return true;
}

bool Videos::ProcessSingleVideo(const VideoInput& video_input) noexcept {
    bool result;

    switch (video_input.processing_mode) {
        case VideoProcessingMode::Copy: {
            result = CopyVideo(video_input);
        } break;

        case VideoProcessingMode::Remux: {
            result = RemuxMultiLanguageVideo(video_input);
        } break;

        default: {
            result = false;
        } break;
    }

    return result;
}

bool Videos::CopyVideo(const VideoInput& video_input) noexcept {
    if (!std::filesystem::exists(video_input.source_path)) {
        return false;
    }

    const auto dest_path = m_output_path / video_input.output_filename;

    // Copy file with overwrite
    try {
        std::filesystem::copy_file(video_input.source_path, dest_path,
                                   std::filesystem::copy_options::overwrite_existing);

        // Ensure file is writable (remove read-only flag)
        std::error_code ec;
        std::filesystem::permissions(dest_path,
                                     std::filesystem::perms::owner_write | std::filesystem::perms::group_write,
                                     std::filesystem::perm_options::add, ec);

    } catch (const std::filesystem::filesystem_error& e) {
        return false;
    }

    const auto sha256_hash = ComputeFileSHA256(dest_path);

    if (sha256_hash.empty()) {
        return false;
    }

    // Create metadata entry
    VideoMetadata metadata;

    metadata.resource_name = video_input.resource_name;
    metadata.filename = video_input.output_filename;
    metadata.sha256_hash = sha256_hash;
    metadata.has_multi_language_audio = false;

    m_video_metadata.push_back(std::move(metadata));

    return true;
}

bool Videos::RemuxMultiLanguageVideo(const VideoInput& video_input) noexcept {
    if (video_input.language_sources.empty()) {
        return false;
    }

    // Build MveLanguageSource vector from VideoInput language sources
    std::vector<MveLanguageSource> sources;

    sources.reserve(video_input.language_sources.size());

    for (const auto& lang_source : video_input.language_sources) {
        // Validate source file exists
        if (!std::filesystem::exists(lang_source.file_path)) {
            // English is required, other languages can be skipped
            if (lang_source.language_tag == "en-US") {
                return false;
            }

            continue;
        }

        MveLanguageSource source;

        source.language_tag = lang_source.language_tag;
        source.stream_index = lang_source.stream_index;
        source.stream_mask = lang_source.stream_mask;
        source.file_path = lang_source.file_path;
        sources.push_back(std::move(source));
    }

    if (sources.empty()) {
        return false;
    }

    const auto dest_path = m_output_path / video_input.output_filename;

    MveMultiplexer multiplexer(std::move(sources), dest_path);

    auto result = multiplexer.Execute();

    if (!result.success) {
        return false;
    }

    const auto output_sha256 = ComputeFileSHA256(dest_path);

    if (output_sha256.empty()) {
        return false;
    }

    // Create metadata entry
    VideoMetadata metadata;

    metadata.resource_name = video_input.resource_name;
    metadata.filename = video_input.output_filename;
    metadata.sha256_hash = output_sha256;
    metadata.has_multi_language_audio = true;

    for (const auto& stream : result.processed_streams) {
        VideoLanguageStream lang_stream;

        lang_stream.language_tag = stream.language_tag;
        lang_stream.stream_index = stream.stream_index;
        lang_stream.stream_mask = stream.stream_mask;

        metadata.audio_streams.push_back(std::move(lang_stream));
    }

    m_video_metadata.push_back(std::move(metadata));

    return true;
}

bool Videos::WriteMetadataJson() noexcept {
    auto json_filepath = m_output_path / "videos.json";

    try {
        json j = json::object();

        j["$schema"] = "http://json-schema.org/draft-07/schema#";
        j["schema_version"] = "1.0.0";
        j["author"] = "Interplay Productions";
        j["copyright"] = "(c) 1996 Interplay Productions";
        j["license"] = "All rights reserved";

        // Video resources array
        json videos_array = json::array();

        for (const auto& video : m_video_metadata) {
            json video_obj = json::object();
            video_obj["resource_name"] = video.resource_name;
            video_obj["filename"] = video.filename;
            video_obj["sha256"] = video.sha256_hash;
            video_obj["has_multi_language_audio"] = video.has_multi_language_audio;

            if (!video.audio_streams.empty()) {
                json streams_array = json::array();

                for (const auto& stream : video.audio_streams) {
                    json stream_obj = json::object();
                    stream_obj["language_tag"] = stream.language_tag;
                    stream_obj["stream_index"] = stream.stream_index;
                    stream_obj["stream_mask"] = stream.stream_mask;

                    streams_array.push_back(std::move(stream_obj));
                }

                video_obj["audio_streams"] = std::move(streams_array);
            }

            videos_array.push_back(std::move(video_obj));
        }

        j["videos"] = std::move(videos_array);

        // Write to file with pretty printing
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
