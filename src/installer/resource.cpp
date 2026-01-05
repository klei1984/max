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

#include "resource.hpp"

#include <SDL3/SDL.h>

namespace Installer {

constexpr int32_t RES_INDEX_SIZE = 16;

Resource::Resource(const std::filesystem::path& game_data_path, const std::filesystem::path& base_path) noexcept {
    m_file_mutex = SDL_CreateMutex();

    auto max_res_path = game_data_path / "MAX.RES";

    if (!std::filesystem::exists(max_res_path)) {
        return;
    }

    if (!LoadResourceFile(max_res_path)) {
        return;
    }

    m_is_valid = true;

    auto patches_res_path = base_path / "PATCHES.RES";

    if (std::filesystem::exists(patches_res_path)) {
        LoadResourceFile(patches_res_path);
    }
}

Resource::~Resource() noexcept {
    for (auto& entry : m_res_files) {
        if (entry.file_handle) {
            SDL_CloseIO(entry.file_handle);

            entry.file_handle = nullptr;
        }
    }

    if (m_file_mutex) {
        SDL_DestroyMutex(m_file_mutex);

        m_file_mutex = nullptr;
    }
}

Resource::Resource(Resource&& other) noexcept
    : m_res_files(std::move(other.m_res_files)),
      m_resource_lookup(std::move(other.m_resource_lookup)),
      m_is_valid(other.m_is_valid),
      m_file_mutex(other.m_file_mutex) {
    // Transfer ownership of SDL_Mutex pointer
    other.m_file_mutex = nullptr;
    other.m_is_valid = false;
}

Resource& Resource::operator=(Resource&& other) noexcept {
    if (this != &other) {
        // Close existing file handles before moving
        for (auto& entry : m_res_files) {
            if (entry.file_handle) {
                SDL_CloseIO(entry.file_handle);
            }
        }

        // Destroy existing mutex before taking ownership of the other's mutex
        if (m_file_mutex) {
            SDL_DestroyMutex(m_file_mutex);
        }

        m_res_files = std::move(other.m_res_files);
        m_resource_lookup = std::move(other.m_resource_lookup);
        m_is_valid = other.m_is_valid;
        m_file_mutex = other.m_file_mutex;

        other.m_file_mutex = nullptr;
        other.m_is_valid = false;
    }

    return *this;
}

bool Resource::LoadResourceFile(const std::filesystem::path& file_path) noexcept {
    SDL_IOStream* io = SDL_IOFromFile(file_path.string().c_str(), "rb");

    if (!io) {
        return false;
    }

    ResHeader header;

    if (SDL_ReadIO(io, header.id, sizeof(header.id)) != sizeof(header.id) || !SDL_ReadS32LE(io, &header.offset) ||
        !SDL_ReadS32LE(io, &header.size)) {
        SDL_CloseIO(io);

        return false;
    }

    if (SDL_strncmp("RES0", header.id, sizeof(header.id)) != 0) {
        SDL_CloseIO(io);

        return false;
    }

    // Seek to index table
    if (SDL_SeekIO(io, header.offset, SDL_IO_SEEK_SET) < 0) {
        SDL_CloseIO(io);

        return false;
    }

    // Calculate number of index entries
    const int32_t num_entries = header.size / RES_INDEX_SIZE;

    if (num_entries <= 0 || header.size % RES_INDEX_SIZE != 0) {
        SDL_CloseIO(io);
        return false;
    }

    // Read index table entries individually for endianness safety
    std::vector<ResIndex> index_table;

    index_table.reserve(num_entries);

    for (int32_t i = 0; i < num_entries; ++i) {
        ResIndex entry;

        if (SDL_ReadIO(io, entry.tag, sizeof(entry.tag)) != sizeof(entry.tag) ||
            !SDL_ReadS32LE(io, &entry.data_offset) || !SDL_ReadS32LE(io, &entry.data_size)) {
            SDL_CloseIO(io);

            return false;
        }

        index_table.push_back(entry);
    }

    // Add to resource files list
    const size_t file_index = m_res_files.size();

    m_res_files.push_back({io, std::move(index_table)});

    // Build lookup table (later entries override earlier ones, so PATCHES.RES overrides MAX.RES)
    for (const auto& entry : m_res_files[file_index].index_table) {
        std::string tag(entry.tag, SDL_strnlen(entry.tag, sizeof(entry.tag)));
        m_resource_lookup[tag] = file_index;
    }

    return true;
}

std::vector<uint8_t> Resource::ReadResource(const std::string& resource_tag) noexcept {
    if (m_file_mutex) {
        SDL_LockMutex(m_file_mutex);
    }

    // Lookup resource in index
    auto it = m_resource_lookup.find(resource_tag);

    if (it == m_resource_lookup.end()) {
        if (m_file_mutex) {
            SDL_UnlockMutex(m_file_mutex);
        }

        return {};
    }

    const size_t file_index = it->second;
    const auto& res_file = m_res_files[file_index];

    // Find the index entry
    const ResIndex* index_entry = nullptr;

    for (const auto& entry : res_file.index_table) {
        if (SDL_strncmp(entry.tag, resource_tag.c_str(), sizeof(entry.tag)) == 0) {
            index_entry = &entry;

            break;
        }
    }

    if (!index_entry) {
        if (m_file_mutex) {
            SDL_UnlockMutex(m_file_mutex);
        }

        return {};
    }

    // Seek to resource data
    if (SDL_SeekIO(res_file.file_handle, index_entry->data_offset, SDL_IO_SEEK_SET) < 0) {
        if (m_file_mutex) {
            SDL_UnlockMutex(m_file_mutex);
        }

        return {};
    }

    // Allocate buffer with exact resource size
    std::vector<uint8_t> buffer(index_entry->data_size);

    if (SDL_ReadIO(res_file.file_handle, buffer.data(), index_entry->data_size) !=
        static_cast<size_t>(index_entry->data_size)) {
        if (m_file_mutex) {
            SDL_UnlockMutex(m_file_mutex);
        }

        return {};
    }

    if (m_file_mutex) {
        SDL_UnlockMutex(m_file_mutex);
    }
    return buffer;
}

}  // namespace Installer
