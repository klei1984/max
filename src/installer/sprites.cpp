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

#include "sprites.hpp"

#include <SDL3/SDL.h>
#include <rectpack2D/finders_interface.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>

#include "palette.hpp"
#include "procedures.hpp"
#include "shadow.hpp"
#include "sprite.hpp"

namespace Installer {

using json = nlohmann::ordered_json;

enum Flags : uint32_t {
    GROUND_COVER = 0x1,
    EXPLODING = 0x2,
    ANIMATED = 0x4,
    CONNECTOR_UNIT = 0x8,
    BUILDING = 0x10,
    MISSILE_UNIT = 0x20,
    MOBILE_AIR_UNIT = 0x40,
    MOBILE_SEA_UNIT = 0x80,
    MOBILE_LAND_UNIT = 0x100,
    STATIONARY = 0x200,
    UPGRADABLE = 0x4000,
    HOVERING = 0x10000,
    HAS_FIRING_SPRITE = 0x20000,
    FIRES_MISSILES = 0x40000,
    CONSTRUCTOR_UNIT = 0x80000,
    ELECTRONIC_UNIT = 0x200000,
    SELECTABLE = 0x400000,
    STANDALONE = 0x800000,
    REQUIRES_SLAB = 0x1000000,
    TURRET_SPRITE = 0x2000000,
    SENTRY_UNIT = 0x4000000,
    SPINNING_TURRET = 0x8000000,
    REGENERATING_UNIT = 0x10000000
};

static const std::unordered_map<std::string, uint32_t> FlagsMap = {{"GROUND_COVER", GROUND_COVER},
                                                                   {"EXPLODING", EXPLODING},
                                                                   {"ANIMATED", ANIMATED},
                                                                   {"CONNECTOR_UNIT", CONNECTOR_UNIT},
                                                                   {"BUILDING", BUILDING},
                                                                   {"MISSILE_UNIT", MISSILE_UNIT},
                                                                   {"MOBILE_AIR_UNIT", MOBILE_AIR_UNIT},
                                                                   {"MOBILE_SEA_UNIT", MOBILE_SEA_UNIT},
                                                                   {"MOBILE_LAND_UNIT", MOBILE_LAND_UNIT},
                                                                   {"STATIONARY", STATIONARY},
                                                                   {"UPGRADABLE", UPGRADABLE},
                                                                   {"HOVERING", HOVERING},
                                                                   {"HAS_FIRING_SPRITE", HAS_FIRING_SPRITE},
                                                                   {"FIRES_MISSILES", FIRES_MISSILES},
                                                                   {"CONSTRUCTOR_UNIT", CONSTRUCTOR_UNIT},
                                                                   {"ELECTRONIC_UNIT", ELECTRONIC_UNIT},
                                                                   {"SELECTABLE", SELECTABLE},
                                                                   {"STANDALONE", STANDALONE},
                                                                   {"REQUIRES_SLAB", REQUIRES_SLAB},
                                                                   {"TURRET_SPRITE", TURRET_SPRITE},
                                                                   {"SENTRY_UNIT", SENTRY_UNIT},
                                                                   {"SPINNING_TURRET", SPINNING_TURRET},
                                                                   {"REGENERATING_UNIT", REGENERATING_UNIT}};

/*
 * \brief Computes a SHA-256 hash string from frame pixel data for deduplication.
 *
 * The hash includes frame dimensions to avoid collisions between frames with identical pixel patterns but different
 * sizes.
 *
 * \param frame The sprite frame to hash.
 * \return A SHA-256 hash string (64 hex characters).
 */
static inline std::string ComputeFrameHash(const SpriteFrame& frame) {
    std::vector<uint8_t> hash_buffer;

    hash_buffer.reserve(sizeof(frame.width) + sizeof(frame.height) + frame.data.size());

    hash_buffer.push_back(static_cast<uint8_t>(frame.width & 0xFF));
    hash_buffer.push_back(static_cast<uint8_t>((frame.width >> 8) & 0xFF));
    hash_buffer.push_back(static_cast<uint8_t>(frame.height & 0xFF));
    hash_buffer.push_back(static_cast<uint8_t>((frame.height >> 8) & 0xFF));

    hash_buffer.insert(hash_buffer.end(), frame.data.begin(), frame.data.end());

    return ComputeBufferSHA256(hash_buffer.data(), hash_buffer.size());
}

Sprites::Sprites(const std::filesystem::path& game_data_path, const std::filesystem::path& output_path,
                 const std::filesystem::path& units_json_path) noexcept
    : m_game_data_path(game_data_path),
      m_output_path(output_path),
      m_units_json_path(units_json_path),
      m_resource_reader(game_data_path, ""),
      m_is_valid(false) {
    if (!m_resource_reader.IsValid()) {
        return;
    }

    if (!LoadUnitsJSON()) {
        return;
    }

    m_is_valid = true;
}

Sprites::~Sprites() noexcept {}

bool Sprites::IsValid() const noexcept { return m_is_valid; }

bool Sprites::LoadUnitsJSON() noexcept {
    std::ifstream json_file(m_units_json_path);

    if (!json_file.is_open()) {
        return false;
    }

    try {
        json units_data = json::parse(json_file);

        if (!units_data.contains("units") || !units_data["units"].is_object()) {
            return false;
        }

        const auto& units_object = units_data["units"];

        m_units.reserve(units_object.size());

        for (const auto& [unit_name, unit] : units_object.items()) {
            UnitData unit_data;

            unit_data.resource_name = unit_name;
            unit_data.flags = 0;
            unit_data.has_frame_info = false;

            // Parse flags array from JSON
            if (unit.contains("flags") && unit["flags"].is_array()) {
                for (const auto& flag_str : unit["flags"]) {
                    if (flag_str.is_string()) {
                        std::string flag_name = flag_str.get<std::string>();
                        auto it = FlagsMap.find(flag_name);

                        if (it != FlagsMap.end()) {
                            unit_data.flags |= it->second;
                        }
                    }
                }
            }

            // Extract data ResourceID for FrameInfo metadata
            if (unit.contains("data") && unit["data"].is_string()) {
                unit_data.data_resource_name = unit["data"].get<std::string>();
            }

            // Extract shadow ResourceID
            if (unit.contains("shadow") && unit["shadow"].is_string()) {
                unit_data.shadow_resource_name = unit["shadow"].get<std::string>();
            }

            m_units.push_back(std::move(unit_data));
        }

        return true;

    } catch (const json::exception& e) {
        return false;
    }
}

bool Sprites::Convert() noexcept {
    if (!m_is_valid) {
        return false;
    }

    m_dat_filename = "units.gfx.dat";

    auto dat_filepath = m_output_path / m_dat_filename;
    SDL_IOStream* dat_file = SDL_IOFromFile(dat_filepath.string().c_str(), "wb");

    if (!dat_file) {
        return false;
    }

    // Process each unit: decode sprites, collect all frames for packing
    std::vector<SpriteFrame> all_sprite_frames;
    std::vector<size_t> sprite_frame_unit_indices;   // Which unit owns each sprite frame
    std::vector<size_t> sprite_frame_local_indices;  // Local frame index within the unit

    // Shadow frame deduplication structures
    std::vector<SpriteFrame> unique_shadow_frames;    // Only unique shadow pixel content
    std::vector<size_t> unique_shadow_unit_indices;   // Unit that owns each unique shadow frame
    std::vector<size_t> unique_shadow_local_indices;  // Local frame index of first occurrence

    // Maps SHA256 hash -> index in unique_shadow_frames for deduplication lookup
    std::unordered_map<std::string, size_t> shadow_hash_to_unique_index;

    // Maps (unit_idx, local_frame_idx) -> index in unique_shadow_frames for duplicate frame references
    // This allows duplicate frames to share atlas coordinates with their canonical frame
    std::unordered_map<uint64_t, size_t> shadow_frame_to_unique_index;

    for (size_t unit_idx = 0; unit_idx < m_units.size(); ++unit_idx) {
        auto& unit = m_units[unit_idx];

        // Load sprite resource (continue on missing/invalid to keep converting remaining units)
        std::vector<uint8_t> sprite_resource = m_resource_reader.ReadResource(unit.resource_name);

        if (sprite_resource.empty()) {
            continue;
        }

        size_t sprite_size = sprite_resource.size();
        Sprite sprite_decoder(sprite_resource.data(), sprite_size);

        if (!sprite_decoder.IsValid()) {
            continue;
        }

        std::vector<SpriteFrame> sprite_frames = sprite_decoder.DecodeAllFrames();

        if (sprite_frames.empty()) {
            continue;
        }

        // Validate frame sizes against MAX_ATLAS_SIZE (reject frames too large to fit)
        bool has_oversized_frame = false;
        for (const auto& frame : sprite_frames) {
            if (frame.width > MAX_ATLAS_SIZE || frame.height > MAX_ATLAS_SIZE) {
                has_oversized_frame = true;
                break;
            }
        }

        if (has_oversized_frame) {
            continue;  // Skip this unit - at least one frame exceeds atlas size
        }

        // Load shadow resource from units.json metadata
        if (!unit.shadow_resource_name.empty() && unit.shadow_resource_name != "INVALID_ID") {
            std::vector<uint8_t> shadow_resource = m_resource_reader.ReadResource(unit.shadow_resource_name);

            if (!shadow_resource.empty()) {
                size_t shadow_size = shadow_resource.size();
                Shadow shadow_decoder(shadow_resource.data(), shadow_size);

                if (shadow_decoder.IsValid()) {
                    std::vector<SpriteFrame> shadow_frames = shadow_decoder.DecodeAllFrames();

                    if (!shadow_frames.empty()) {
                        // Validate shadow frame sizes against MAX_ATLAS_SIZE
                        bool has_oversized_shadow = false;
                        for (const auto& frame : shadow_frames) {
                            if (frame.width > MAX_ATLAS_SIZE || frame.height > MAX_ATLAS_SIZE) {
                                has_oversized_shadow = true;
                                break;
                            }
                        }

                        if (has_oversized_shadow) {
                            // Skip shadow frames for this unit, but continue processing the unit
                            shadow_frames.clear();
                        }
                    }

                    if (!shadow_frames.empty()) {
                        // Pre-allocate shadow_frames array for this unit to store all frame metadata
                        unit.shadow_frames.resize(shadow_frames.size());

                        // Store shadow frames with deduplication based on pixel content hash
                        for (size_t i = 0; i < shadow_frames.size(); ++i) {
                            // Initialize frame metadata that's unique to each frame (even duplicates)
                            unit.shadow_frames[i].frame_index = static_cast<uint16_t>(i);
                            unit.shadow_frames[i].hotspot_x = shadow_frames[i].hotspot_x;
                            unit.shadow_frames[i].hotspot_y = shadow_frames[i].hotspot_y;

                            auto hash = ComputeFrameHash(shadow_frames[i]);
                            auto it = shadow_hash_to_unique_index.find(hash);

                            // Create key for this frame's reference mapping
                            uint64_t frame_key = (static_cast<uint64_t>(unit_idx) << 32) | static_cast<uint64_t>(i);

                            if (it != shadow_hash_to_unique_index.end()) {
                                // Duplicate found - map this frame to the existing unique frame
                                shadow_frame_to_unique_index[frame_key] = it->second;

                            } else {
                                // New unique frame - add to unique list and create mapping
                                size_t unique_idx = unique_shadow_frames.size();
                                shadow_hash_to_unique_index[hash] = unique_idx;
                                shadow_frame_to_unique_index[frame_key] = unique_idx;

                                unique_shadow_frames.push_back(std::move(shadow_frames[i]));
                                unique_shadow_unit_indices.push_back(unit_idx);
                                unique_shadow_local_indices.push_back(i);
                            }
                        }
                    }
                }
            }
        }

        // Store sprite frames for packing
        for (size_t i = 0; i < sprite_frames.size(); ++i) {
            all_sprite_frames.push_back(sprite_frames[i]);
            sprite_frame_unit_indices.push_back(unit_idx);
            sprite_frame_local_indices.push_back(i);
        }

        // Load FrameInfo metadata from data resource if available
        if (!unit.data_resource_name.empty() && unit.data_resource_name != "INVALID_ID") {
            unit.has_frame_info = DecodeFrameInfo(unit.data_resource_name, unit.flags, unit.frame_info);
        }
    }

    // Ensure we decoded at least one sprite frame
    if (all_sprite_frames.empty()) {
        SDL_CloseIO(dat_file);

        return false;
    }

    // Pack all sprite frames into fixed 1024x1024 atlases (frames can span multiple atlases)
    if (!PackAllFramesIntoAtlases(all_sprite_frames, sprite_frame_unit_indices, sprite_frame_local_indices,
                                  m_sprite_atlases, m_units, false)) {
        SDL_CloseIO(dat_file);

        return false;
    }

    // Pack deduplicated shadow frames into atlases, then resolve references for duplicate frames
    if (!unique_shadow_frames.empty()) {
        if (!PackAllFramesIntoAtlases(unique_shadow_frames, unique_shadow_unit_indices, unique_shadow_local_indices,
                                      m_shadow_atlases, m_units, true)) {
            SDL_CloseIO(dat_file);

            return false;
        }

        // Resolve duplicate shadow frame references: copy atlas metadata from canonical frames to duplicates
        for (size_t unit_idx = 0; unit_idx < m_units.size(); ++unit_idx) {
            auto& unit = m_units[unit_idx];

            // Skip units without shadow frames
            if (unit.shadow_frames.empty()) {
                continue;
            }

            // For each shadow frame slot in this unit, look up its unique frame reference
            for (size_t local_idx = 0; local_idx < unit.shadow_frames.size(); ++local_idx) {
                uint64_t frame_key = (static_cast<uint64_t>(unit_idx) << 32) | static_cast<uint64_t>(local_idx);
                auto ref_it = shadow_frame_to_unique_index.find(frame_key);

                if (ref_it == shadow_frame_to_unique_index.end()) {
                    // Frame not found in mapping - should not happen
                    continue;
                }

                size_t unique_idx = ref_it->second;

                // Get the canonical frame's unit and local index
                size_t canonical_unit_idx = unique_shadow_unit_indices[unique_idx];
                size_t canonical_local_idx = unique_shadow_local_indices[unique_idx];

                // If this frame is the canonical one, it already has correct atlas coordinates
                if (canonical_unit_idx == unit_idx && canonical_local_idx == local_idx) {
                    continue;
                }

                // Copy atlas coordinates from canonical frame to this duplicate frame
                const auto& canonical_frame = m_units[canonical_unit_idx].shadow_frames[canonical_local_idx];

                unit.shadow_frames[local_idx].texture_id = canonical_frame.texture_id;
                unit.shadow_frames[local_idx].atlas_x = canonical_frame.atlas_x;
                unit.shadow_frames[local_idx].atlas_y = canonical_frame.atlas_y;
                unit.shadow_frames[local_idx].width = canonical_frame.width;
                unit.shadow_frames[local_idx].height = canonical_frame.height;
                unit.shadow_frames[local_idx].flipped = canonical_frame.flipped;
            }
        }
    }

    if (!WriteCompressedAtlases(dat_file, m_sprite_atlases)) {
        SDL_CloseIO(dat_file);

        return false;
    }

    if (!WriteCompressedAtlases(dat_file, m_shadow_atlases)) {
        SDL_CloseIO(dat_file);

        return false;
    }

    SDL_CloseIO(dat_file);

    m_dat_sha256 = ComputeFileSHA256(dat_filepath);

    if (m_dat_sha256.empty()) {
        return false;
    }

    if (!GenerateMetadataJSON()) {
        return false;
    }

    return true;
}

bool Sprites::PackAllFramesIntoAtlases(const std::vector<SpriteFrame>& all_frames,
                                       const std::vector<size_t>& frame_unit_indices,
                                       const std::vector<size_t>& frame_local_indices,
                                       std::vector<AtlasTexture>& atlases, std::vector<UnitData>& units,
                                       bool is_shadow) noexcept {
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

    using rect_type = rectpack2D::output_rect_t<rectpack2D::empty_spaces<true>>;

    while (!remaining_indices.empty()) {
        std::vector<rect_type> rectangles;

        auto pack_result = PackRectanglesIntoAtlas<true>(dimensions, remaining_indices, MAX_ATLAS_SIZE, rectangles);

        if (!pack_result.success) {
            // No frames fit - frames were already validated, so this indicates a packing algorithm failure
            return false;
        }

        uint32_t actual_width = pack_result.atlas_width;
        uint32_t actual_height = pack_result.atlas_height;

        // Create atlas with actual required size (not fixed ATLAS_SIZE)
        AtlasTexture atlas;

        atlas.texture_id = static_cast<uint32_t>(atlases.size());
        atlas.width = actual_width;
        atlas.height = actual_height;
        atlas.pixels.resize(atlas.width * atlas.height, 0);  // Initialize with transparent

        // Blit packed frames into atlas and update unit metadata
        for (size_t rect_idx : pack_result.packed_indices) {
            size_t global_frame_idx = remaining_indices[rect_idx];
            const auto& frame = all_frames[global_frame_idx];
            const auto& rect = rectangles[rect_idx];
            size_t unit_idx = frame_unit_indices[global_frame_idx];
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

            // Update unit's packed frame metadata
            std::vector<PackedFrame>& unit_packed_frames =
                (is_shadow) ? units[unit_idx].shadow_frames : units[unit_idx].sprite_frames;

            // Ensure vector is large enough
            if (local_frame_idx >= unit_packed_frames.size()) {
                unit_packed_frames.resize(local_frame_idx + 1);
            }

            unit_packed_frames[local_frame_idx].frame_index = static_cast<uint16_t>(local_frame_idx);
            unit_packed_frames[local_frame_idx].texture_id = atlas.texture_id;
            unit_packed_frames[local_frame_idx].atlas_x = static_cast<uint16_t>(rect.x);
            unit_packed_frames[local_frame_idx].atlas_y = static_cast<uint16_t>(rect.y);

            // Store atlas dimensions (swapped if flipped)
            unit_packed_frames[local_frame_idx].width = rect.flipped ? frame.height : frame.width;
            unit_packed_frames[local_frame_idx].height = rect.flipped ? frame.width : frame.height;
            unit_packed_frames[local_frame_idx].hotspot_x = frame.hotspot_x;
            unit_packed_frames[local_frame_idx].hotspot_y = frame.hotspot_y;
            unit_packed_frames[local_frame_idx].flipped = rect.flipped;
        }

        atlas.uncompressed_size = static_cast<uint32_t>(atlas.pixels.size());
        atlases.push_back(std::move(atlas));

        RemovePackedIndices(remaining_indices, pack_result.packed_indices);
    }

    return true;
}

bool Sprites::WriteCompressedAtlases(SDL_IOStream* dat_file, std::vector<AtlasTexture>& atlases) noexcept {
    for (auto& atlas : atlases) {
        auto compress_result = CompressLZ4HC(atlas.pixels.data(), atlas.pixels.size());

        if (!compress_result.success) {
            return false;
        }

        atlas.compressed_offset = SDL_TellIO(dat_file);
        atlas.compressed_size = compress_result.compressed_size;

        if (!SDL_WriteU32LE(dat_file, atlas.width) || !SDL_WriteU32LE(dat_file, atlas.height) ||
            !SDL_WriteU32LE(dat_file, atlas.uncompressed_size) || !SDL_WriteU32LE(dat_file, atlas.compressed_size) ||
            SDL_WriteIO(dat_file, compress_result.compressed_data.data(), compress_result.compressed_size) !=
                static_cast<size_t>(compress_result.compressed_size)) {
            return false;
        }
    }

    return true;
}

bool Sprites::GenerateMetadataJSON() noexcept {
    try {
        json j;

        j["$schema"] = "http://json-schema.org/draft-07/schema#";
        j["schema_version"] = "1.0.0";
        j["author"] = "Interplay Productions";
        j["copyright"] = "(c) 1996 Interplay Productions";
        j["license"] = "All rights reserved";
        j["dat_filename"] = m_dat_filename;
        j["dat_sha256"] = m_dat_sha256;
        j["pixel_format"] = "R8";
        j["compression"] = "lz4hc";

        // Write atlas metadata
        j["atlases"] = json::array();

        for (const auto& atlas : m_sprite_atlases) {
            json atlas_info;

            atlas_info["type"] = "sprite";
            atlas_info["data_offset"] = atlas.compressed_offset;
            atlas_info["compressed_size"] = atlas.compressed_size;
            atlas_info["uncompressed_size"] = atlas.uncompressed_size;
            atlas_info["width"] = atlas.width;
            atlas_info["height"] = atlas.height;

            j["atlases"].push_back(atlas_info);
        }

        for (const auto& atlas : m_shadow_atlases) {
            json atlas_info;

            atlas_info["type"] = "shadow";
            atlas_info["data_offset"] = atlas.compressed_offset;
            atlas_info["compressed_size"] = atlas.compressed_size;
            atlas_info["uncompressed_size"] = atlas.uncompressed_size;
            atlas_info["width"] = atlas.width;
            atlas_info["height"] = atlas.height;

            j["atlases"].push_back(atlas_info);
        }

        // Write unit metadata with complete frame coordinates and FrameInfo
        j["units"] = json::object();

        for (const auto& unit : m_units) {
            json unit_info;

            // Add FrameInfo if available
            if (unit.has_frame_info) {
                json frame_info_json;

                frame_info_json["image_base"] = unit.frame_info.image_base;
                frame_info_json["image_count"] = unit.frame_info.image_count;
                frame_info_json["turret_image_base"] = unit.frame_info.turret_image_base;
                frame_info_json["turret_image_count"] = unit.frame_info.turret_image_count;
                frame_info_json["firing_image_base"] = unit.frame_info.firing_image_base;
                frame_info_json["firing_image_count"] = unit.frame_info.firing_image_count;
                frame_info_json["connector_image_base"] = unit.frame_info.connector_image_base;
                frame_info_json["connector_image_count"] = unit.frame_info.connector_image_count;

                unit_info["frame_info"] = frame_info_json;
            }

            // Add sprite frames with complete atlas coordinates
            unit_info["sprite_frames"] = json::array();

            for (const auto& frame : unit.sprite_frames) {
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

                unit_info["sprite_frames"].push_back(frame_json);
            }

            // Add shadow frames if available
            if (!unit.shadow_frames.empty()) {
                unit_info["shadow_frames"] = json::array();

                for (const auto& frame : unit.shadow_frames) {
                    json frame_json;

                    frame_json["frame_index"] = frame.frame_index;
                    // Shadow texture_ids are offset by sprite atlas count
                    frame_json["texture_id"] = frame.texture_id + static_cast<uint32_t>(m_sprite_atlases.size());
                    frame_json["atlas_x"] = frame.atlas_x;
                    frame_json["atlas_y"] = frame.atlas_y;
                    frame_json["width"] = frame.width;
                    frame_json["height"] = frame.height;
                    frame_json["hotspot_x"] = frame.hotspot_x;
                    frame_json["hotspot_y"] = frame.hotspot_y;
                    frame_json["flipped"] = frame.flipped;

                    unit_info["shadow_frames"].push_back(frame_json);
                }
            }

            if (!unit.data_resource_name.empty()) {
                unit_info["data_resource"] = unit.data_resource_name;
            }

            j["units"][unit.resource_name] = unit_info;
        }

        // Write JSON file
        auto json_filepath = m_output_path / "units.gfx.json";
        std::ofstream json_file(json_filepath);

        if (!json_file.is_open()) {
            return false;
        }

        json_file << j.dump(1, '\t') << std::endl;
        json_file.close();

        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool Sprites::DecodeFrameInfo(const std::string& resource_name, uint32_t flags, FrameInfo& frame_info) noexcept {
    std::vector<uint8_t> resource_data = m_resource_reader.ReadResource(resource_name);

    if (resource_data.empty()) {
        return false;
    }

    SDL_IOStream* stream = SDL_IOFromConstMem(resource_data.data(), resource_data.size());

    if (!stream) {
        return false;
    }

    {
        int8_t image_base, image_count, turret_image_base, turret_image_count, firing_image_base, firing_image_count,
            connector_image_base, connector_image_count;

        if (!SDL_ReadS8(stream, &image_base) || !SDL_ReadS8(stream, &image_count) ||
            !SDL_ReadS8(stream, &turret_image_base) || !SDL_ReadS8(stream, &turret_image_count) ||
            !SDL_ReadS8(stream, &firing_image_base) || !SDL_ReadS8(stream, &firing_image_count) ||
            !SDL_ReadS8(stream, &connector_image_base) || !SDL_ReadS8(stream, &connector_image_count)) {
            SDL_CloseIO(stream);

            return false;
        }

        frame_info.image_base = image_base;
        frame_info.image_count = image_count;
        frame_info.turret_image_base = turret_image_base;
        frame_info.turret_image_count = turret_image_count;
        frame_info.firing_image_base = firing_image_base;
        frame_info.firing_image_count = firing_image_count;
        frame_info.connector_image_base = connector_image_base;
        frame_info.connector_image_count = connector_image_count;
    }

    if (flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        for (int i = 0; i < 8; ++i) {
            int8_t x, y;

            if (!SDL_ReadS8(stream, &x) || !SDL_ReadS8(stream, &y)) {
                SDL_CloseIO(stream);

                return false;
            }

            frame_info.angle_offsets[i].x = x;
            frame_info.angle_offsets[i].y = y;
        }

    } else {
        // No turret sprite, zero out angle_offsets
        for (int i = 0; i < 8; ++i) {
            frame_info.angle_offsets[i].x = 0;
            frame_info.angle_offsets[i].y = 0;
        }
    }

    SDL_CloseIO(stream);

    return true;
}

}  // namespace Installer
