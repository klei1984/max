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

#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <SDL3/SDL.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "palette.hpp"

namespace Installer {

/**
 * \brief Unified resource entry structure for all installer manager modules.
 *
 * This structure supports all resource types with optional fields for specialized cases:
 * - All entries use resource_name
 * - Voices use priority field (1-5, 0 = unused)
 * - Controls use palette field (pointer to palette colors array, nullptr = unused)
 */
struct ResourceEntry {
    const char* resource_name;            ///< Human-readable resource identifier
    uint8_t priority{0};                  ///< Voice playback priority (1=highest, 5=lowest, 0=unused)
    const PaletteRGBA* palette{nullptr};  ///< Palette for Controls (nullptr=unused)
};

/**
 * \brief Lightweight reader for RES resource files used during asset conversion.
 *
 * The class provides minimal read-only access to RES file archives.
 * Supports reading from both the main game data directory (MAX.RES) and the base directory (PATCHES.RES, optional).
 * PATCHES.RES entries override MAX.RES entries when present.
 */
class Resource {
public:
    /**
     * \brief Constructs resource reader with paths to game data directory (MAX.RES) and base directory (PATCHES.RES).
     *
     * \param game_data_path Path to directory containing MAX.RES (required, typically original game installation dir).
     * \param base_path Path to directory containing PATCHES.RES (optional, typically executable directory for patches).
     */
    Resource(const std::filesystem::path& game_data_path, const std::filesystem::path& base_path) noexcept;

    ~Resource() noexcept;

    // Non-copyable (SDL_Mutex* requires manual lifetime management)
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    // Movable (required for std::map emplace)
    Resource(Resource&& other) noexcept;
    Resource& operator=(Resource&& other) noexcept;

    /**
     * \brief Checks if resource files were successfully opened and indexed for reading operations.
     *
     * \return True if at least MAX.RES is available and valid, false if initialization failed.
     */
    bool IsValid() const noexcept { return m_is_valid; }

    /**
     * \brief Reads raw resource data by 8-character tag name from RES archives.
     *
     * \param resource_tag 8-character resource identifier (null-terminated, case-sensitive matching against RES index).
     * \return Vector containing resource data bytes (empty vector if resource not found), includes size() method.
     */
    std::vector<uint8_t> ReadResource(const std::string& resource_tag) noexcept;

private:
    struct ResIndex {
        char tag[8];
        int32_t data_offset;
        int32_t data_size;
    };

    struct ResHeader {
        char id[4];
        int32_t offset;
        int32_t size;
    };

    struct ResFileEntry {
        SDL_IOStream* file_handle;
        std::vector<ResIndex> index_table;
    };

    std::vector<ResFileEntry> m_res_files;
    std::unordered_map<std::string, size_t> m_resource_lookup;
    bool m_is_valid{false};
    mutable SDL_Mutex* m_file_mutex{nullptr};

    bool LoadResourceFile(const std::filesystem::path& file_path) noexcept;
};

}  // namespace Installer

#endif /* RESOURCE_HPP */
