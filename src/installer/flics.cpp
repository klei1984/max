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

#include "flics.hpp"

#include <rectpack2D/finders_interface.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include "procedures.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

Flics::Flics(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
             const std::vector<FlicResourceEntry>& resources) noexcept
    : m_game_data_path(game_data_path),
      m_output_path(output_path),
      m_resources(resources),
      m_resource_reader(game_data_path, "") {
    m_is_valid = m_resource_reader.IsValid() && std::filesystem::is_directory(output_path) && !m_resources.empty();
}

Flics::~Flics() noexcept {}

bool Flics::IsValid() const noexcept { return m_is_valid; }

bool Flics::Convert() noexcept {
    if (!m_is_valid) {
        return false;
    }

    m_dat_filename = "animations.dat";

    auto dat_filepath = m_output_path / m_dat_filename;

    SDL_IOStream* dat_file = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!dat_file) {
        return false;
    }

    size_t completed_count = 0;
    const size_t total_count = m_resources.size();

    for (size_t i = 0; i < total_count; ++i) {
        const auto& entry = m_resources[i];

        if (ProcessFlicResource(entry, dat_file)) {
            ++completed_count;
        }
    }

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

bool Flics::ProcessFlicResource(const FlicResourceEntry& entry, SDL_IOStream* dat_file) noexcept {
    using rect_type = rectpack2D::output_rect_t<rectpack2D::empty_spaces<true>>;

    auto resource_data = m_resource_reader.ReadResource(entry.resource_name);

    if (resource_data.empty()) {
        return false;
    }

    std::string flic_file_path_relative(reinterpret_cast<const char*>(resource_data.data()));
    auto flic_file_path = m_game_data_path / flic_file_path_relative;

    if (!std::filesystem::exists(flic_file_path)) {
        return false;
    }

    Flic flic_decoder(flic_file_path);

    if (!flic_decoder.IsValid()) {
        return false;
    }

    std::vector<FlicFrameData> frames = flic_decoder.DecodeAllFrames();

    if (frames.empty()) {
        return false;
    }

    uint32_t frame_width = flic_decoder.GetWidth();
    uint32_t frame_height = flic_decoder.GetHeight();
    uint32_t frame_count = static_cast<uint32_t>(frames.size());
    std::vector<std::pair<uint16_t, uint16_t>> dimensions;
    std::vector<size_t> all_indices;

    dimensions.reserve(frame_count);

    for (uint32_t i = 0; i < frame_count; ++i) {
        dimensions.push_back({static_cast<uint16_t>(frame_width), static_cast<uint16_t>(frame_height)});
    }

    all_indices.reserve(frame_count);

    for (uint32_t i = 0; i < frame_count; ++i) {
        all_indices.push_back(i);
    }

    std::vector<rect_type> rectangles;

    // Pack all frames into a single atlas (FLIC animations are small, typically fit in one atlas)
    auto pack_result = PackRectanglesIntoAtlas<true>(dimensions, all_indices, MAX_ATLAS_SIZE, rectangles);

    if (!pack_result.success || pack_result.packed_indices.size() != frame_count) {
        return false;  // All frames must fit in a single atlas for FLIC animations
    }

    uint32_t atlas_width = pack_result.atlas_width;
    uint32_t atlas_height = pack_result.atlas_height;

    // Create atlas and blit frames
    std::vector<uint8_t> atlas_rgba(atlas_width * atlas_height * 4, 0);

    for (uint32_t i = 0; i < frame_count; ++i) {
        const auto& rect = rectangles[i];
        const auto& frame = frames[i].rgba_data;
        bool is_rotated = rect.flipped;

        if (is_rotated) {
            BlitRGBA8888Rotated90CW(frame.data(), frame_width, frame_height, atlas_rgba.data(), atlas_width,
                                    static_cast<uint16_t>(rect.x), static_cast<uint16_t>(rect.y));
        }

        else {
            BlitRGBA8888(frame.data(), frame_width, frame_height, atlas_rgba.data(), atlas_width,
                         static_cast<uint16_t>(rect.x), static_cast<uint16_t>(rect.y));
        }
    }

    // Compress atlas with LZ4HC
    auto compress_result = CompressLZ4HC(atlas_rgba.data(), atlas_rgba.size());

    if (!compress_result.success) {
        return false;
    }

    // Write compressed data to .dat file
    auto write_result =
        WriteCompressedData(dat_file, compress_result.compressed_data.data(), compress_result.compressed_size);

    if (!write_result.success) {
        return false;
    }

    // Create metadata entry
    FlicAnimationMetadata metadata;

    metadata.resource_name = entry.resource_name;
    metadata.data_offset = write_result.data_offset;
    metadata.compressed_size = compress_result.compressed_size;
    metadata.uncompressed_size = compress_result.uncompressed_size;
    metadata.frame_count = frame_count;
    metadata.frame_duration_ms = frames[0].duration_ms;
    metadata.atlas_width = atlas_width;
    metadata.atlas_height = atlas_height;
    metadata.frames.reserve(frame_count);

    for (uint32_t i = 0; i < frame_count; ++i) {
        FlicFrameMetadata frame_meta;

        frame_meta.x = rectangles[i].x;
        frame_meta.y = rectangles[i].y;
        frame_meta.width = frame_width;
        frame_meta.height = frame_height;
        frame_meta.rotated = rectangles[i].flipped;

        metadata.frames.push_back(frame_meta);
    }

    m_metadata.push_back(metadata);

    return true;
}

bool Flics::WriteMetadataJson() noexcept {
    auto json_filename = "animations.json";
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
        j["pixel_format"] = "RGBA8888";
        j["compression"] = "lz4hc";

        json animations_object = json::object();

        for (const auto& meta : m_metadata) {
            json anim_entry = {{"data_offset", meta.data_offset},
                               {"compressed_size", meta.compressed_size},
                               {"uncompressed_size", meta.uncompressed_size},
                               {"frame_count", meta.frame_count},
                               {"frame_duration_ms", meta.frame_duration_ms},
                               {"atlas_width", meta.atlas_width},
                               {"atlas_height", meta.atlas_height}};

            json frames_array = json::array();

            for (const auto& frame : meta.frames) {
                frames_array.push_back({{"x", frame.x},
                                        {"y", frame.y},
                                        {"w", frame.width},
                                        {"h", frame.height},
                                        {"rotated", frame.rotated}});
            }

            anim_entry["frames"] = frames_array;
            animations_object[meta.resource_name] = anim_entry;
        }

        j["animations"] = animations_object;

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
