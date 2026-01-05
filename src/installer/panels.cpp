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

#include "panels.hpp"

#include <rectpack2D/finders_interface.h>

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#include "procedures.hpp"
#include "progress_event.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

Panels::Panels(const std::filesystem::path& game_data_path, const std::filesystem::path& base_path,
               const std::filesystem::path& output_path, const std::vector<ResourceEntry>& panel_resources) noexcept
    : m_game_data_path(game_data_path),
      m_output_path(output_path),
      m_resource_reader(game_data_path, base_path),
      m_panel_resources(panel_resources) {
    m_is_valid = m_resource_reader.IsValid();
}

Panels::~Panels() noexcept {}

bool Panels::Convert() noexcept {
    if (!m_is_valid) {
        return false;
    }

    std::vector<PanelDimensionInfo> dimensions;

    dimensions.reserve(m_panel_resources.size());

    if (!PreloadDimensions(dimensions)) {
        return false;
    }

    if (dimensions.empty()) {
        return false;
    }

    m_dat_filename = "panels.dat";

    auto dat_filepath = m_output_path / m_dat_filename;
    SDL_IOStream* dat_file = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!dat_file) {
        return false;
    }

    if (!StreamProcessAndWriteAtlases(dimensions, dat_file)) {
        SDL_CloseIO(dat_file);
        return false;
    }

    SDL_CloseIO(dat_file);

    m_dat_sha256 = ComputeFileSHA256(dat_filepath);

    if (m_dat_sha256.empty()) {
        return false;
    }

    if (!WriteMetadataJson()) {
        return false;
    }

    return true;
}

bool Panels::PreloadDimensions(std::vector<PanelDimensionInfo>& dimensions) noexcept {
    const size_t total_count = m_panel_resources.size();
    size_t processed_count = 0;
    size_t success_count = 0;
    size_t fail_count = 0;
    uint64_t last_progress_time = 0;

    for (size_t i = 0; i < m_panel_resources.size(); ++i) {
        const auto& entry = m_panel_resources[i];

        const uint64_t current_time = SDL_GetTicks();

        if (current_time - last_progress_time >= 250) {
            PushProgressDetailedEvent(success_count, fail_count, total_count);
            last_progress_time = current_time;
        }

        auto resource_data = m_resource_reader.ReadResource(entry.resource_name);

        if (resource_data.empty()) {
            ++fail_count;
            ++processed_count;

            continue;
        }

        PanelDimensionInfo dim_info;

        dim_info.resource_name = entry.resource_name;
        dim_info.resource_index = i;

        if (!Panel::DecodeMetadata(entry.resource_name, resource_data, dim_info.width, dim_info.height,
                                   dim_info.hotspot_x, dim_info.hotspot_y)) {
            ++fail_count;
            ++processed_count;

            continue;
        }

        dimensions.push_back(dim_info);

        ++success_count;
        ++processed_count;
    }

    PushProgressDetailedEvent(success_count, fail_count, total_count);

    return !dimensions.empty();
}

bool Panels::StreamProcessAndWriteAtlases(const std::vector<PanelDimensionInfo>& dimensions,
                                          SDL_IOStream* dat_file) noexcept {
    using rect_type = rectpack2D::output_rect_t<rectpack2D::empty_spaces<true>>;

    // Prepare dimensions array for packing
    std::vector<std::pair<uint16_t, uint16_t>> dim_pairs;
    dim_pairs.reserve(dimensions.size());

    for (const auto& dim : dimensions) {
        dim_pairs.push_back({dim.width, dim.height});
    }

    std::vector<size_t> remaining_indices;
    remaining_indices.reserve(dimensions.size());

    for (size_t i = 0; i < dimensions.size(); ++i) {
        remaining_indices.push_back(i);
    }

    // Sort indices by image area (width * height) in decreasing order for optimal packing
    std::sort(remaining_indices.begin(), remaining_indices.end(), [&dimensions](size_t a, size_t b) {
        const auto& dim_a = dimensions[a];
        const auto& dim_b = dimensions[b];
        const size_t area_a = static_cast<size_t>(dim_a.width) * dim_a.height;
        const size_t area_b = static_cast<size_t>(dim_b.width) * dim_b.height;

        return area_a > area_b;  // Decreasing order (largest first)
    });

    // Progress tracking for streaming processing
    const size_t total_image_count = dimensions.size();
    size_t processed_image_count = 0;
    size_t success_image_count = 0;
    size_t fail_image_count = 0;
    uint64_t last_progress_time = 0;

    // Pack images into atlases until all are placed
    while (!remaining_indices.empty()) {
        std::vector<rect_type> rectangles;

        auto pack_result = PackRectanglesIntoAtlas<true>(dim_pairs, remaining_indices, MAX_ATLAS_SIZE, rectangles);

        if (!pack_result.success) {
            // No images fit - handle oversized image case
            size_t oversized_idx = remaining_indices[0];
            const auto& oversized_dim = dimensions[oversized_idx];

            if (oversized_dim.width > MAX_ATLAS_SIZE || oversized_dim.height > MAX_ATLAS_SIZE) {
                return false;
            }

            // The image fits in an atlas but not with others - give it its own atlas
            uint16_t atlas_index = static_cast<uint16_t>(m_atlas_metadata.size());
            std::vector<uint8_t> atlas_rgba(static_cast<size_t>(oversized_dim.width) * oversized_dim.height * 4, 0);
            PanelData panel_data;

            if (!ProcessAndBlitImage(oversized_dim, atlas_rgba, oversized_dim.width, 0, 0, false, panel_data)) {
                ++fail_image_count;
                ++processed_image_count;
                remaining_indices.erase(remaining_indices.begin());

                continue;
            }

            m_panel_metadata.push_back(CreatePanelMetadata(panel_data, atlas_index, 0, 0, false));

            if (!CompressAndWriteAtlas(atlas_rgba, dat_file, oversized_dim.width, oversized_dim.height)) {
                return false;
            }

            ++success_image_count;
            ++processed_image_count;

            ReportProgress(last_progress_time, success_image_count, fail_image_count, total_image_count);

            remaining_indices.erase(remaining_indices.begin());

            continue;
        }

        uint16_t atlas_index = static_cast<uint16_t>(m_atlas_metadata.size());
        std::vector<uint8_t> atlas_rgba(static_cast<size_t>(pack_result.atlas_width) * pack_result.atlas_height * 4, 0);

        // Stream-decode and blit packed images into atlas one by one
        for (size_t rect_idx : pack_result.packed_indices) {
            size_t dim_idx = remaining_indices[rect_idx];
            const auto& dim = dimensions[dim_idx];
            const auto& rect = rectangles[rect_idx];

            ReportProgress(last_progress_time, success_image_count, fail_image_count, total_image_count);

            PanelData panel_data;
            bool is_rotated = rect.flipped;

            if (!ProcessAndBlitImage(dim, atlas_rgba, pack_result.atlas_width, static_cast<uint16_t>(rect.x),
                                     static_cast<uint16_t>(rect.y), is_rotated, panel_data)) {
                ++fail_image_count;
                ++processed_image_count;

                continue;  // Skip this image but continue processing atlas
            }

            m_panel_metadata.push_back(CreatePanelMetadata(panel_data, atlas_index, static_cast<uint16_t>(rect.x),
                                                           static_cast<uint16_t>(rect.y), is_rotated));

            ++success_image_count;
            ++processed_image_count;
        }

        if (!CompressAndWriteAtlas(atlas_rgba, dat_file, pack_result.atlas_width, pack_result.atlas_height)) {
            return false;
        }

        RemovePackedIndices(remaining_indices, pack_result.packed_indices);
    }

    // Push final progress event to ensure 100% completion is reported
    PushProgressDetailedEvent(success_image_count, fail_image_count, total_image_count);

    return true;
}

bool Panels::ProcessAndBlitImage(const PanelDimensionInfo& dim, std::vector<uint8_t>& atlas_rgba, uint16_t atlas_width,
                                 uint16_t x, uint16_t y, bool rotated, PanelData& out_panel_data) noexcept {
    const auto& resource_entry = m_panel_resources[dim.resource_index];
    auto resource_data = m_resource_reader.ReadResource(resource_entry.resource_name);

    if (resource_data.empty()) {
        return false;
    }

    Panel decoder(resource_entry.resource_name, resource_data);

    if (!decoder.IsValid()) {
        return false;
    }

    out_panel_data = std::move(decoder.GetPanelData());

    if (rotated) {
        BlitRGBA8888Rotated90CW(out_panel_data.rgba_data.data(), out_panel_data.width, out_panel_data.height,
                                atlas_rgba.data(), atlas_width, x, y);
    }

    else {
        BlitRGBA8888(out_panel_data.rgba_data.data(), out_panel_data.width, out_panel_data.height, atlas_rgba.data(),
                     atlas_width, x, y);
    }

    return true;
}

bool Panels::CompressAndWriteAtlas(const std::vector<uint8_t>& atlas_rgba, SDL_IOStream* dat_file, uint16_t atlas_width,
                                   uint16_t atlas_height) noexcept {
    auto compress_result = CompressLZ4HC(atlas_rgba.data(), atlas_rgba.size());

    if (!compress_result.success) {
        return false;
    }

    auto write_result =
        WriteCompressedData(dat_file, compress_result.compressed_data.data(), compress_result.compressed_size);

    if (!write_result.success) {
        return false;
    }

    PanelAtlasMetadata atlas_meta;

    atlas_meta.data_offset = write_result.data_offset;
    atlas_meta.compressed_size = compress_result.compressed_size;
    atlas_meta.uncompressed_size = compress_result.uncompressed_size;
    atlas_meta.width = atlas_width;
    atlas_meta.height = atlas_height;

    m_atlas_metadata.push_back(atlas_meta);

    return true;
}

PanelMetadata Panels::CreatePanelMetadata(const PanelData& panel_data, uint16_t atlas_index, uint16_t x, uint16_t y,
                                          bool rotated) noexcept {
    PanelMetadata meta;

    meta.resource_name = panel_data.resource_name;
    meta.atlas_index = atlas_index;
    meta.x = x;
    meta.y = y;
    meta.width = panel_data.width;
    meta.height = panel_data.height;
    meta.hotspot_x = panel_data.hotspot_x;
    meta.hotspot_y = panel_data.hotspot_y;
    meta.rotated = rotated;

    return meta;
}

void Panels::ReportProgress(uint64_t& last_progress_time, size_t success_count, size_t fail_count,
                            size_t total_count) noexcept {
    const uint64_t current_time = SDL_GetTicks();

    if (current_time - last_progress_time >= 250) {
        PushProgressDetailedEvent(success_count, fail_count, total_count);
        last_progress_time = current_time;
    }
}

bool Panels::WriteMetadataJson() noexcept {
    auto json_filepath = m_output_path / "panels.json";

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

        // Write atlases array
        json atlases_array = json::array();

        for (const auto& atlas : m_atlas_metadata) {
            atlases_array.push_back({{"data_offset", atlas.data_offset},
                                     {"compressed_size", atlas.compressed_size},
                                     {"uncompressed_size", atlas.uncompressed_size},
                                     {"width", atlas.width},
                                     {"height", atlas.height}});
        }

        j["atlases"] = atlases_array;

        // Write arts object
        json arts_object = json::object();

        for (const auto& art : m_panel_metadata) {
            arts_object[art.resource_name] = {{"atlas_index", art.atlas_index},
                                              {"x", art.x},
                                              {"y", art.y},
                                              {"width", art.width},
                                              {"height", art.height},
                                              {"hotspot_x", art.hotspot_x},
                                              {"hotspot_y", art.hotspot_y},
                                              {"rotated", art.rotated}};
        }

        j["panels"] = arts_object;

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
