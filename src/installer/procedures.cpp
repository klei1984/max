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

#include "procedures.hpp"

#include <lz4hc.h>

#include <algorithm>
#include <fstream>
#include <mutex>

#include "sha2.h"

namespace Installer {

static std::mutex g_directory_creation_mutex;

CompressionResult CompressLZ4HC(const void* data, size_t data_size) noexcept {
    CompressionResult result;

    if (!data || data_size == 0) {
        return result;
    }

    // Calculate maximum possible compressed size
    const int max_compressed_size = LZ4_compressBound(static_cast<int>(data_size));

    if (max_compressed_size <= 0) {
        return result;
    }

    // Allocate buffer for compressed data
    result.compressed_data.resize(max_compressed_size);

    // Compress with maximum LZ4HC compression level for best ratio
    const int compressed_size = LZ4_compress_HC(static_cast<const char*>(data), result.compressed_data.data(),
                                                static_cast<int>(data_size), max_compressed_size, LZ4HC_CLEVEL_MAX);

    if (compressed_size <= 0) {
        result.compressed_data.clear();
        return result;
    }

    // Shrink buffer to actual compressed size to minimize memory usage
    result.compressed_data.resize(compressed_size);
    result.compressed_size = static_cast<uint32_t>(compressed_size);
    result.uncompressed_size = static_cast<uint32_t>(data_size);
    result.success = true;

    return result;
}

WriteResult WriteCompressedData(SDL_IOStream* file, const void* compressed_data, size_t compressed_size) noexcept {
    WriteResult result;

    if (!file || !compressed_data || compressed_size == 0) {
        return result;
    }

    // Capture current file position for metadata (this is where the data will start)
    result.data_offset = static_cast<uint64_t>(SDL_TellIO(file));

    // Write compressed data to file
    const size_t bytes_written = SDL_WriteIO(file, compressed_data, compressed_size);

    if (bytes_written != compressed_size) {
        return result;
    }

    result.success = true;

    return result;
}

std::string ComputeFileSHA256(const std::filesystem::path& file_path) noexcept {
    SDL_IOStream* io = SDL_IOFromFile(file_path.string().c_str(), "rb");

    if (io) {
        sha256_ctx ctx;
        sha256_init(&ctx);

        constexpr size_t BUFFER_SIZE = 8192;
        uint8_t buffer[BUFFER_SIZE];
        size_t bytes_read;

        while ((bytes_read = SDL_ReadIO(io, buffer, BUFFER_SIZE)) > 0) {
            sha256_update(&ctx, buffer, bytes_read);
        }

        SDL_CloseIO(io);

        uint8_t hash[SHA256_DIGEST_SIZE];
        sha256_final(&ctx, hash);

        // Convert to lowercase hex string
        char hex_str[SHA256_DIGEST_SIZE * 2 + 1];

        for (int i = 0; i < SHA256_DIGEST_SIZE; ++i) {
            SDL_snprintf(&hex_str[i * 2], 3, "%02x", hash[i]);
        }

        hex_str[SHA256_DIGEST_SIZE * 2] = '\0';

        return std::string(hex_str);

    } else {
        return "";
    }
}

std::string ComputeBufferSHA256(const void* data, size_t data_size) noexcept {
    if (!data || data_size == 0) {
        return "";
    }

    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, static_cast<const uint8_t*>(data), data_size);

    uint8_t hash[SHA256_DIGEST_SIZE];
    sha256_final(&ctx, hash);

    // Convert to lowercase hex string
    char hex_str[SHA256_DIGEST_SIZE * 2 + 1];

    for (int i = 0; i < SHA256_DIGEST_SIZE; ++i) {
        SDL_snprintf(&hex_str[i * 2], 3, "%02x", hash[i]);
    }

    hex_str[SHA256_DIGEST_SIZE * 2] = '\0';

    return std::string(hex_str);
}

bool WriteJsonFile(const std::filesystem::path& file_path, const std::string& json_string) noexcept {
    // Create parent directories if they don't exist (mutex protected for thread safety)
    std::error_code ec;

    if (file_path.has_parent_path()) {
        std::lock_guard<std::mutex> lock(g_directory_creation_mutex);
        std::filesystem::create_directories(file_path.parent_path(), ec);

        if (ec) {
            return false;
        }
    }

    // Open file for text writing
    std::ofstream output_file(file_path);

    if (!output_file.is_open()) {
        return false;
    }

    // Write JSON content
    output_file << json_string << std::endl;

    // Check for write errors
    if (!output_file.good()) {
        output_file.close();

        return false;
    }

    output_file.close();

    // Ensure file is writable (remove read-only flag)
    std::filesystem::permissions(file_path, std::filesystem::perms::owner_write | std::filesystem::perms::group_write,
                                 std::filesystem::perm_options::add, ec);

    return true;
}

SDL_IOStream* OpenDatFile(const std::filesystem::path& file_path) noexcept {
    // Create parent directories if they don't exist (mutex protected for thread safety)
    std::error_code ec;

    if (file_path.has_parent_path()) {
        std::lock_guard<std::mutex> lock(g_directory_creation_mutex);
        std::filesystem::create_directories(file_path.parent_path(), ec);

        if (ec) {
            return nullptr;
        }
    }

    // Open file for binary writing
    return SDL_IOFromFile(file_path.string().c_str(), "wb");
}

void BlitRGBA8888(const uint8_t* src_rgba, uint16_t src_width, uint16_t src_height, uint8_t* dst_rgba,
                  uint16_t dst_width, uint16_t dst_x, uint16_t dst_y) noexcept {
    for (uint16_t row = 0; row < src_height; ++row) {
        const uint32_t src_offset = row * src_width * 4;
        const uint32_t dst_offset = ((dst_y + row) * dst_width + dst_x) * 4;

        SDL_memcpy(&dst_rgba[dst_offset], &src_rgba[src_offset], src_width * 4);
    }
}

void BlitRGBA8888Rotated90CW(const uint8_t* src_rgba, uint16_t src_width, uint16_t src_height, uint8_t* dst_rgba,
                             uint16_t dst_width, uint16_t dst_x, uint16_t dst_y) noexcept {
    // 90° CW rotation: src[y][x] -> dst[x][height-1-y]
    // After rotation: output width = src_height, output height = src_width
    for (uint16_t src_y = 0; src_y < src_height; ++src_y) {
        for (uint16_t src_x = 0; src_x < src_width; ++src_x) {
            const uint32_t src_offset = (src_y * src_width + src_x) * 4;
            const uint16_t dst_x_rot = dst_x + src_y;
            const uint16_t dst_y_rot = dst_y + (src_width - 1 - src_x);
            const uint32_t dst_offset = (dst_y_rot * dst_width + dst_x_rot) * 4;

            SDL_memcpy(&dst_rgba[dst_offset], &src_rgba[src_offset], 4);
        }
    }
}

void BlitR8(const uint8_t* src_r8, uint16_t src_width, uint16_t src_height, uint8_t* dst_r8, uint16_t dst_width,
            uint16_t dst_x, uint16_t dst_y) noexcept {
    for (uint16_t row = 0; row < src_height; ++row) {
        const uint8_t* src = src_r8 + row * src_width;
        uint8_t* dst = dst_r8 + (dst_y + row) * dst_width + dst_x;

        SDL_memcpy(dst, src, src_width);
    }
}

void BlitR8Rotated90CW(const uint8_t* src_r8, uint16_t src_width, uint16_t src_height, uint8_t* dst_r8,
                       uint16_t dst_width, uint16_t dst_x, uint16_t dst_y) noexcept {
    // 90° CW rotation: src[y][x] -> dst[x][height-1-y]
    // After rotation: output width = src_height, output height = src_width
    for (uint16_t src_y = 0; src_y < src_height; ++src_y) {
        for (uint16_t src_x = 0; src_x < src_width; ++src_x) {
            const uint16_t dst_x_rot = dst_x + src_y;
            const uint16_t dst_y_rot = dst_y + (src_width - 1 - src_x);

            dst_r8[dst_y_rot * dst_width + dst_x_rot] = src_r8[src_y * src_width + src_x];
        }
    }
}

void RemovePackedIndices(std::vector<size_t>& remaining_indices, const std::vector<size_t>& packed_indices) noexcept {
    // Sort in descending order to enable swap-and-pop without index invalidation
    std::vector<size_t> sorted_indices = packed_indices;

    std::sort(sorted_indices.begin(), sorted_indices.end(), std::greater<size_t>());

    for (size_t rect_idx : sorted_indices) {
        std::swap(remaining_indices[rect_idx], remaining_indices.back());
        remaining_indices.pop_back();
    }
}

}  // namespace Installer
