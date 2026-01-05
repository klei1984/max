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

#include "portraits.hpp"

#include <SDL3/SDL.h>
#include <rectpack2D/finders_interface.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

#include "palette.hpp"
#include "procedures.hpp"
#include "sprite.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

Portraits::Portraits(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
                     const std::vector<const char*>& resources) noexcept
    : m_game_data_path(game_data_path),
      m_output_path(output_path),
      m_resources(resources),
      m_resource_reader(game_data_path, ""),
      m_is_valid(false) {
    if (!m_resource_reader.IsValid()) {
        return;
    }

    m_is_valid = true;
}

Portraits::~Portraits() noexcept {}

bool Portraits::IsValid() const noexcept { return m_is_valid; }

bool Portraits::Convert() noexcept {
    if (!m_is_valid) {
        return false;
    }

    m_dat_filename = "portraits.gfx.dat";

    // Create output .dat file
    auto dat_filepath = m_output_path / m_dat_filename;
    SDL_IOStream* dat_file = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!dat_file) {
        return false;
    }

    // Process each interface unit: decode sprites, collect all frames for packing
    std::vector<SpriteFrame> all_frames;
    std::vector<size_t> frame_unit_indices;   // Which portrait owns each frame
    std::vector<size_t> frame_local_indices;  // Local frame index within the portrait

    // Reserve space for all portraits
    m_portraits.reserve(m_resources.size());

    for (size_t portrait_idx = 0; portrait_idx < m_resources.size(); ++portrait_idx) {
        const char* resource_name = m_resources[portrait_idx];

        // Create portrait data entry
        PortraitData portrait_data;
        portrait_data.resource_name = resource_name;

        // Load sprite resource (continue on missing/invalid to keep converting remaining resources)
        std::vector<uint8_t> sprite_resource = m_resource_reader.ReadResource(resource_name);

        if (sprite_resource.empty()) {
            m_portraits.push_back(std::move(portrait_data));
            continue;
        }

        size_t sprite_size = sprite_resource.size();
        Sprite sprite_decoder(sprite_resource.data(), sprite_size);

        if (!sprite_decoder.IsValid()) {
            m_portraits.push_back(std::move(portrait_data));
            continue;
        }

        std::vector<SpriteFrame> sprite_frames = sprite_decoder.DecodeAllFrames();

        if (sprite_frames.empty()) {
            m_portraits.push_back(std::move(portrait_data));
            continue;
        }

        // Store frames for packing
        for (size_t i = 0; i < sprite_frames.size(); ++i) {
            all_frames.push_back(sprite_frames[i]);
            frame_unit_indices.push_back(portrait_idx);
            frame_local_indices.push_back(i);
        }

        m_portraits.push_back(std::move(portrait_data));
    }

    // Ensure we decoded at least one sprite frame
    if (all_frames.empty()) {
        SDL_CloseIO(dat_file);
        return false;
    }

    // Pack all frames into fixed 1024x1024 atlases

    if (!PackAllFramesIntoAtlases(all_frames, frame_unit_indices, frame_local_indices)) {
        SDL_CloseIO(dat_file);
        return false;
    }

    // Write compressed atlases
    if (!WriteCompressedAtlases(dat_file)) {
        SDL_CloseIO(dat_file);
        return false;
    }

    SDL_CloseIO(dat_file);

    m_dat_sha256 = ComputeFileSHA256(dat_filepath);

    if (m_dat_sha256.empty()) {
        return false;
    }

    // Generate metadata JSON
    if (!GenerateMetadataJSON()) {
        return false;
    }

    return true;
}

bool Portraits::PackAllFramesIntoAtlases(const std::vector<SpriteFrame>& all_frames,
                                         const std::vector<size_t>& frame_unit_indices,
                                         const std::vector<size_t>& frame_local_indices) noexcept {
    using rect_type = rectpack2D::output_rect_t<rectpack2D::empty_spaces<true>>;

    // Prepare dimensions array for packing
    std::vector<std::pair<uint16_t, uint16_t>> dimensions;
    dimensions.reserve(all_frames.size());

    for (const auto& frame : all_frames) {
        dimensions.push_back({frame.width, frame.height});
    }

    std::vector<size_t> remaining_indices;
    remaining_indices.reserve(all_frames.size());

    for (size_t i = 0; i < all_frames.size(); ++i) {
        remaining_indices.push_back(i);
    }

    // Pack frames into atlases until all are placed
    while (!remaining_indices.empty()) {
        std::vector<rect_type> rectangles;

        auto pack_result = PackRectanglesIntoAtlas<true>(dimensions, remaining_indices, MAX_ATLAS_SIZE, rectangles);

        if (!pack_result.success) {
            // No frames fit - this means at least one frame is larger than MAX_ATLAS_SIZE
            return false;
        }

        uint32_t actual_width = pack_result.atlas_width;
        uint32_t actual_height = pack_result.atlas_height;

        // Create atlas with actual required size
        AtlasTexture atlas;

        atlas.texture_id = static_cast<uint32_t>(m_atlases.size());
        atlas.width = actual_width;
        atlas.height = actual_height;
        atlas.pixels.resize(atlas.width * atlas.height, 0);  // Initialize with transparent

        // Blit packed frames into atlas and update portrait metadata
        for (size_t rect_idx : pack_result.packed_indices) {
            size_t global_frame_idx = remaining_indices[rect_idx];
            const auto& frame = all_frames[global_frame_idx];
            const auto& rect = rectangles[rect_idx];

            size_t portrait_idx = frame_unit_indices[global_frame_idx];
            size_t local_frame_idx = frame_local_indices[global_frame_idx];

            // Copy frame pixels to atlas (rotate 90° CW if flipped)
            if (rect.flipped) {
                BlitR8Rotated90CW(frame.data.data(), frame.width, frame.height, atlas.pixels.data(), atlas.width,
                                  static_cast<uint16_t>(rect.x), static_cast<uint16_t>(rect.y));
            }

            else {
                BlitR8(frame.data.data(), frame.width, frame.height, atlas.pixels.data(), atlas.width,
                       static_cast<uint16_t>(rect.x), static_cast<uint16_t>(rect.y));
            }

            // Update portrait's packed frame metadata
            std::vector<PackedFrame>& portrait_frames = m_portraits[portrait_idx].frames;

            // Ensure vector is large enough
            if (local_frame_idx >= portrait_frames.size()) {
                portrait_frames.resize(local_frame_idx + 1);
            }

            portrait_frames[local_frame_idx].frame_index = static_cast<uint16_t>(local_frame_idx);
            portrait_frames[local_frame_idx].texture_id = atlas.texture_id;
            portrait_frames[local_frame_idx].atlas_x = static_cast<uint16_t>(rect.x);
            portrait_frames[local_frame_idx].atlas_y = static_cast<uint16_t>(rect.y);

            // Store atlas dimensions (swapped if flipped)
            portrait_frames[local_frame_idx].width = rect.flipped ? frame.height : frame.width;
            portrait_frames[local_frame_idx].height = rect.flipped ? frame.width : frame.height;
            portrait_frames[local_frame_idx].hotspot_x = frame.hotspot_x;
            portrait_frames[local_frame_idx].hotspot_y = frame.hotspot_y;
            portrait_frames[local_frame_idx].flipped = rect.flipped;
        }

        atlas.uncompressed_size = static_cast<uint32_t>(atlas.pixels.size());
        m_atlases.push_back(std::move(atlas));

        RemovePackedIndices(remaining_indices, pack_result.packed_indices);
    }

    return true;
}

bool Portraits::WriteCompressedAtlases(SDL_IOStream* dat_file) noexcept {
    for (auto& atlas : m_atlases) {
        auto compress_result = CompressLZ4HC(atlas.pixels.data(), atlas.pixels.size());

        if (!compress_result.success) {
            return false;
        }

        atlas.compressed_offset = SDL_TellIO(dat_file);
        atlas.compressed_size = compress_result.compressed_size;

        // Write compressed data
        if (!SDL_WriteU32LE(dat_file, atlas.width) || !SDL_WriteU32LE(dat_file, atlas.height) ||
            !SDL_WriteU32LE(dat_file, atlas.uncompressed_size) || !SDL_WriteU32LE(dat_file, atlas.compressed_size) ||
            SDL_WriteIO(dat_file, compress_result.compressed_data.data(), compress_result.compressed_size) !=
                static_cast<size_t>(compress_result.compressed_size)) {
            return false;
        }
    }

    return true;
}

bool Portraits::GenerateMetadataJSON() noexcept {
    json output;

    output["$schema"] = "http://json-schema.org/draft-07/schema#";
    output["schema_version"] = "1.0.0";
    output["author"] = "Interplay Productions";
    output["copyright"] = "(c) 1996 Interplay Productions";
    output["license"] = "All rights reserved";
    output["description"] = "Interface unit sprites for build menus and selection dialogs";
    output["dat_filename"] = m_dat_filename;
    output["dat_sha256"] = m_dat_sha256;
    output["pixel_format"] = "R8";
    output["compression"] = "lz4hc";

    // Write atlas metadata
    output["atlases"] = json::array();

    for (const auto& atlas : m_atlases) {
        json atlas_info;
        atlas_info["data_offset"] = atlas.compressed_offset;
        atlas_info["compressed_size"] = atlas.compressed_size;
        atlas_info["uncompressed_size"] = atlas.uncompressed_size;
        atlas_info["width"] = atlas.width;
        atlas_info["height"] = atlas.height;
        output["atlases"].push_back(atlas_info);
    }

    // Write interface unit metadata with complete frame coordinates
    output["portraits"] = json::object();

    for (const auto& portrait : m_portraits) {
        json portrait_info;

        // Add frames with complete atlas coordinates
        portrait_info["frames"] = json::array();
        for (const auto& frame : portrait.frames) {
            json frame_json;
            frame_json["frame_index"] = frame.frame_index;
            frame_json["texture_id"] = frame.texture_id;
            frame_json["atlas_x"] = frame.atlas_x;
            frame_json["atlas_y"] = frame.atlas_y;
            frame_json["width"] = frame.width;
            frame_json["height"] = frame.height;
            frame_json["hotspot_x"] = frame.hotspot_x;
            frame_json["hotspot_y"] = frame.hotspot_y;
            frame_json["flipped"] = frame.flipped;
            portrait_info["frames"].push_back(frame_json);
        }

        output["portraits"][portrait.resource_name] = portrait_info;
    }

    // Write JSON file
    auto json_filepath = m_output_path / "portraits.gfx.json";
    std::ofstream json_file(json_filepath);

    if (!json_file.is_open()) {
        return false;
    }

    json_file << output.dump(1, '\t') << std::endl;
    json_file.close();

    return true;
}

}  // namespace Installer
