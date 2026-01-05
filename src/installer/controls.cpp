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

#include "controls.hpp"

#include <rectpack2D/finders_interface.h>

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#include "procedures.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

Controls::Controls(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
                   const std::vector<ControlResourceEntry>& resources) noexcept
    : m_game_data_path(game_data_path),
      m_output_path(output_path),
      m_resources(resources),
      m_resource_reader(game_data_path, "") {
    m_is_valid = m_resource_reader.IsValid() && std::filesystem::is_directory(output_path) && !m_resources.empty();
}

Controls::~Controls() noexcept {}

bool Controls::Convert() noexcept {
    if (!m_is_valid) {
        return false;
    }

    std::vector<ControlData> controls;

    controls.reserve(m_resources.size());

    if (!LoadAllControls(controls)) {
        return false;
    }

    if (controls.empty()) {
        return false;
    }

    m_dat_filename = "controls.dat";

    auto dat_filepath = m_output_path / m_dat_filename;
    SDL_IOStream* dat_file = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!dat_file) {
        return false;
    }

    if (!PackAndWriteAtlases(controls, dat_file)) {
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

bool Controls::LoadAllControls(std::vector<ControlData>& controls) noexcept {
    const size_t total_count = m_resources.size();
    size_t completed_count = 0;

    for (size_t i = 0; i < total_count; ++i) {
        const auto& entry = m_resources[i];
        auto resource_data = m_resource_reader.ReadResource(entry.resource_name);

        if (resource_data.empty()) {
            continue;
        }

        Control decoder(entry.resource_name, resource_data, entry.palette);

        if (!decoder.IsValid()) {
            continue;
        }

        controls.push_back(std::move(decoder.GetControlData()));
        ++completed_count;
    }

    return !controls.empty();
}

bool Controls::PackAndWriteAtlases(std::vector<ControlData>& images, SDL_IOStream* dat_file) noexcept {
    using rect_type = rectpack2D::output_rect_t<rectpack2D::empty_spaces<true>>;
    std::vector<std::pair<uint16_t, uint16_t>> dimensions;
    std::vector<size_t> remaining_indices;

    dimensions.reserve(images.size());

    for (const auto& img : images) {
        dimensions.push_back({img.width, img.height});
    }

    remaining_indices.reserve(images.size());

    for (size_t i = 0; i < images.size(); ++i) {
        remaining_indices.push_back(i);
    }

    // Pack images into atlases until all are placed
    while (!remaining_indices.empty()) {
        std::vector<rect_type> rectangles;

        auto pack_result = PackRectanglesIntoAtlas<true>(dimensions, remaining_indices, MAX_ATLAS_SIZE, rectangles);

        if (!pack_result.success) {
            // No images fit - at least one image is larger than MAX_ATLAS_SIZE
            return false;
        }

        uint16_t atlas_index = static_cast<uint16_t>(m_atlas_metadata.size());
        std::vector<uint8_t> atlas_rgba(static_cast<size_t>(pack_result.atlas_width) * pack_result.atlas_height * 4, 0);

        // Blit packed images into atlas and create metadata
        for (size_t rect_idx : pack_result.packed_indices) {
            size_t img_idx = remaining_indices[rect_idx];
            const auto& img = images[img_idx];
            const auto& rect = rectangles[rect_idx];

            bool is_rotated = rect.flipped;

            if (is_rotated) {
                BlitRGBA8888Rotated90CW(img.rgba_data.data(), img.width, img.height, atlas_rgba.data(),
                                        pack_result.atlas_width, static_cast<uint16_t>(rect.x),
                                        static_cast<uint16_t>(rect.y));
            }

            else {
                BlitRGBA8888(img.rgba_data.data(), img.width, img.height, atlas_rgba.data(), pack_result.atlas_width,
                             static_cast<uint16_t>(rect.x), static_cast<uint16_t>(rect.y));
            }

            // Create metadata entry for this control
            ControlMetadata meta;

            meta.resource_name = img.resource_name;
            meta.atlas_index = atlas_index;
            meta.x = static_cast<uint16_t>(rect.x);
            meta.y = static_cast<uint16_t>(rect.y);
            meta.width = img.width;
            meta.height = img.height;
            meta.hotspot_x = img.hotspot_x;
            meta.hotspot_y = img.hotspot_y;
            meta.rotated = is_rotated;

            m_control_metadata.push_back(meta);
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

        // Create atlas metadata entry
        AtlasMetadata atlas_meta;

        atlas_meta.data_offset = write_result.data_offset;
        atlas_meta.compressed_size = compress_result.compressed_size;
        atlas_meta.uncompressed_size = compress_result.uncompressed_size;
        atlas_meta.width = pack_result.atlas_width;
        atlas_meta.height = pack_result.atlas_height;

        m_atlas_metadata.push_back(atlas_meta);

        RemovePackedIndices(remaining_indices, pack_result.packed_indices);
    }

    return true;
}

bool Controls::WriteMetadataJson() noexcept {
    auto json_filepath = m_output_path / "controls.json";

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

        // Write controls object
        json controls_object = json::object();

        for (const auto& ctrl : m_control_metadata) {
            controls_object[ctrl.resource_name] = {{"atlas_index", ctrl.atlas_index},
                                                   {"x", ctrl.x},
                                                   {"y", ctrl.y},
                                                   {"width", ctrl.width},
                                                   {"height", ctrl.height},
                                                   {"hotspot_x", ctrl.hotspot_x},
                                                   {"hotspot_y", ctrl.hotspot_y},
                                                   {"rotated", ctrl.rotated}};
        }

        j["controls"] = controls_object;

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
