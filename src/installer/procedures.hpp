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

#ifndef PROCEDURES_HPP
#define PROCEDURES_HPP

#include <SDL3/SDL.h>
#include <rectpack2D/finders_interface.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Installer {

/**
 * \file installer_procedures.hpp
 * \brief Shared utility procedures for installer asset conversion pipelines supporting concurrent worker thread
 * execution.
 *
 * This module provides common operations used across multiple installer manager modules (sprites, panels, controls,
 * flics, soundeffects, music, voices). All functions are designed to be reentrant and thread-safe when operating on
 * mutually exclusive contexts (file handles, buffers). No global state is used - all context is passed explicitly via
 * parameters.
 *
 * \note These procedures are designed for worker thread execution. Ensure file handles and buffers passed to these
 *       functions are not shared between threads without external synchronization.
 */

/**
 * \brief Result of an LZ4HC compression operation containing compressed data and metadata for file storage.
 *
 * This structure holds the outcome of compressing a raw data buffer using LZ4HC maximum compression level. It contains
 * the compressed bytes ready for disk storage along with size information needed to reconstruct the original data
 * during decompression at runtime.
 */
struct CompressionResult {
    std::vector<char> compressed_data;  ///< LZ4HC compressed bytes ready for file writing
    uint32_t compressed_size{0};        ///< Actual size of compressed data in bytes
    uint32_t uncompressed_size{0};      ///< Original uncompressed data size in bytes (for decompression allocation)
    bool success{false};                ///< True if compression succeeded, false on error (e.g., buffer too small)
};

/**
 * \brief Result of writing compressed data to a dat file with offset tracking for json metadata generation.
 *
 * This structure captures the file position where compressed data was written, enabling the json metadata generator
 * to record the exact byte offset needed for runtime random-access loading of individual assets from the dat file.
 */
struct WriteResult {
    uint64_t data_offset{0};  ///< Byte offset in .dat file where compressed data begins (for JSON metadata)
    bool success{false};      ///< True if write completed successfully, false on I/O error
};

/**
 * \brief Compresses raw pixel/audio data using LZ4HC maximum compression for optimal storage efficiency.
 *
 * This function wraps LZ4HC compression with automatic buffer allocation and error handling. It uses the maximum
 * compression level (LZ4HC_CLEVEL_MAX = 12) to achieve smallest possible output size..
 *
 * The function is reentrant and thread-safe when operating on separate input buffers. No internal state is maintained
 * between calls, making it suitable for parallel compression of multiple assets in worker threads.
 *
 * \param[in] data         Pointer to raw uncompressed data buffer (pixels, PCM audio, etc.)
 * \param[in] data_size    Size of the input data buffer in bytes
 *
 * \return CompressionResult containing compressed data and metadata, with success=false on error
 */
CompressionResult CompressLZ4HC(const void* data, size_t data_size) noexcept;

/**
 * \brief Writes compressed data to an open SDL_IOStream file and returns the byte offset for metadata tracking.
 *
 * This function performs atomic write of a compressed data buffer to the current position in an SDL_IOStream,
 * capturing the pre-write file offset for inclusion in json metadata. The offset allows runtime code to seek directly
 * to specific assets within a large consolidated dat file.
 *
 * The function is reentrant when operating on separate file handles. Each file handle maintains independent position
 * state, so multiple worker threads can write to different dat files concurrently without synchronization.
 *
 * \param[in,out] file            Open SDL_IOStream in write mode ("wb") positioned at the desired write location
 * \param[in]     compressed_data Pointer to compressed data buffer (typically from CompressLZ4HC)
 * \param[in]     compressed_size Number of bytes to write from the compressed data buffer
 *
 * \return WriteResult with data_offset set to the file position before writing, success=false on I/O error
 */
WriteResult WriteCompressedData(SDL_IOStream* file, const void* compressed_data, size_t compressed_size) noexcept;

/**
 * \brief Computes SHA-256 hash of a file's contents for integrity verification and content-addressable storage.
 *
 * This function reads an entire file in buffered chunks and computes its SHA-256 cryptographic hash, returning the
 * result as a lowercase hexadecimal string (64 characters). The hash can be used for integrity validation during
 * runtime loading or for content-addressable storage systems.
 *
 * The function handles files of any size efficiently using streaming reads with a fixed-size internal buffer. It is
 * reentrant when operating on separate file paths, as each call creates its own file handle and hash context.
 *
 * \param[in] file_path Path to the file to hash (must exist and be readable)
 *
 * \return 64-character lowercase hexadecimal SHA-256 hash string, or empty string on error (file not found, read error)
 */
std::string ComputeFileSHA256(const std::filesystem::path& file_path) noexcept;

/**
 * \brief Computes SHA-256 hash of an in-memory buffer for content deduplication and integrity verification.
 *
 * This function computes a SHA-256 hash of raw memory data, returning the result as a lowercase hexadecimal string.
 * It is useful for deduplicating identical assets during conversion (e.g., identical sprite frames, shadow textures)
 * before writing to disk.
 *
 * \param[in] data      Pointer to the data buffer to hash
 * \param[in] data_size Size of the data buffer in bytes
 *
 * \return 64-character lowercase hexadecimal SHA-256 hash string, or empty string if data is null or size is 0
 */
std::string ComputeBufferSHA256(const void* data, size_t data_size) noexcept;

/**
 * \brief Writes a JSON string to a file with error handling for metadata output in installer pipelines.
 *
 * This function handles the common pattern of writing JSON metadata files with proper error checking and file closure.
 * It uses std::ofstream for text-mode writing with proper newline handling across platforms. Directory creation is
 * mutex-protected to prevent race conditions when multiple threads write to files in the same parent directory.
 *
 * \param[in] file_path Path where the json file should be written (will be created or overwritten)
 * \param[in] json_string Pre-formatted json string to write (typically from nlohmann::json::dump())
 *
 * \return True if the file was written successfully, false on I/O error
 */
bool WriteJsonFile(const std::filesystem::path& file_path, const std::string& json_string) noexcept;

/**
 * \brief Opens a binary file for writing with automatic directory creation.
 *
 * This function creates the parent directory structure if it doesn't exist, then opens the file for binary writing.
 * It wraps SDL_IOFromFile with error handling and returns the open handle for subsequent write operations. Directory
 * creation is mutex-protected to prevent race conditions when multiple threads open files in the same parent directory.
 *
 * \param[in] file_path Absolute or relative path to the file to create/open
 *
 * \return Valid SDL_IOStream* on success, nullptr on error (directory creation failed or file open failed)
 */
SDL_IOStream* OpenDatFile(const std::filesystem::path& file_path) noexcept;

/**
 * \brief Copies RGBA8888 texture data to an atlas buffer without rotation.
 *
 * \param src_rgba Source RGBA8888 pixel data (32 bits per pixel).
 * \param src_width Source image width in pixels.
 * \param src_height Source image height in pixels.
 * \param dst_rgba Destination atlas RGBA8888 pixel buffer.
 * \param dst_width Destination atlas width in pixels.
 * \param dst_x X coordinate in atlas where image should be placed.
 * \param dst_y Y coordinate in atlas where image should be placed.
 */
void BlitRGBA8888(const uint8_t* src_rgba, uint16_t src_width, uint16_t src_height, uint8_t* dst_rgba,
                  uint16_t dst_width, uint16_t dst_x, uint16_t dst_y) noexcept;

/**
 * \brief Copies RGBA8888 texture data to an atlas buffer with 90° clockwise rotation.
 *
 * After rotation, the image dimensions are swapped: output width = input height, output height = input width.
 * The caller must ensure the destination rectangle has sufficient space for the rotated dimensions.
 *
 * \param src_rgba Source RGBA8888 pixel data (32 bits per pixel).
 * \param src_width Source image width in pixels (becomes height after rotation).
 * \param src_height Source image height in pixels (becomes width after rotation).
 * \param dst_rgba Destination atlas RGBA8888 pixel buffer.
 * \param dst_width Destination atlas width in pixels.
 * \param dst_x X coordinate in atlas where rotated image top-left should be placed.
 * \param dst_y Y coordinate in atlas where rotated image top-left should be placed.
 */
void BlitRGBA8888Rotated90CW(const uint8_t* src_rgba, uint16_t src_width, uint16_t src_height, uint8_t* dst_rgba,
                             uint16_t dst_width, uint16_t dst_x, uint16_t dst_y) noexcept;

/**
 * \brief Copies R8 texture data to an atlas buffer without rotation.
 *
 * \param src_r8 Source R8 pixel data (8 bits per pixel).
 * \param src_width Source image width in pixels.
 * \param src_height Source image height in pixels.
 * \param dst_r8 Destination atlas R8 pixel buffer.
 * \param dst_width Destination atlas width in pixels.
 * \param dst_x X coordinate in atlas where image should be placed.
 * \param dst_y Y coordinate in atlas where image should be placed.
 */
void BlitR8(const uint8_t* src_r8, uint16_t src_width, uint16_t src_height, uint8_t* dst_r8, uint16_t dst_width,
            uint16_t dst_x, uint16_t dst_y) noexcept;

/**
 * \brief Copies R8 texture data to an atlas buffer with 90° clockwise rotation.
 *
 * After rotation, the image dimensions are swapped: output width = input height, output height = input width.
 * The caller must ensure the destination rectangle has sufficient space for the rotated dimensions.
 *
 * \param src_r8 Source R8 pixel data (8 bits per pixel).
 * \param src_width Source image width in pixels (becomes height after rotation).
 * \param src_height Source image height in pixels (becomes width after rotation).
 * \param dst_r8 Destination atlas R8 pixel buffer.
 * \param dst_width Destination atlas width in pixels.
 * \param dst_x X coordinate in atlas where rotated image top-left should be placed.
 * \param dst_y Y coordinate in atlas where rotated image top-left should be placed.
 */
void BlitR8Rotated90CW(const uint8_t* src_r8, uint16_t src_width, uint16_t src_height, uint8_t* dst_r8,
                       uint16_t dst_width, uint16_t dst_x, uint16_t dst_y) noexcept;

/**
 * \brief Removes packed rectangle indices from remaining_indices vector using efficient swap-and-pop.
 *
 * This helper function removes all indices listed in packed_indices from the remaining_indices vector.
 * It uses a swap-and-pop algorithm in descending order to avoid index invalidation during removal.
 * This is a common operation in multi-bin rectpack2D packing loops.
 *
 * \param[in,out] remaining_indices Vector of indices still needing to be packed (modified in-place).
 * \param[in] packed_indices Vector of indices (relative to remaining_indices) that were successfully packed.
 */
void RemovePackedIndices(std::vector<size_t>& remaining_indices, const std::vector<size_t>& packed_indices) noexcept;

/**
 * \brief Result of a single atlas packing operation containing packed rectangle information.
 */
template <typename RectType>
struct PackingResult {
    std::vector<size_t> packed_indices;  ///< Indices (into remaining_indices) that were packed
    uint16_t atlas_width{0};             ///< Actual width of packed atlas
    uint16_t atlas_height{0};            ///< Actual height of packed atlas
    bool success{false};                 ///< True if at least one rectangle was packed
};

/**
 * \brief Packs rectangles into a single fixed-size atlas bin using rectpack2D with rotation support.
 *
 * This function encapsulates the common rectpack2D packing pattern used across all installer managers. It uses a
 * sentinel-based approach to detect which rectangles were successfully packed, and computes the actual bounding box of
 * the packed result for optimal atlas size allocation.
 *
 * \tparam EnableFlipping If true, uses empty_spaces<true> and allows 90° rotation; if false, uses empty_spaces<false>
 * \param dimensions Array of (width, height) pairs for items to pack
 * \param remaining_indices Indices into dimensions array for items still needing packing
 * \param max_atlas_size Maximum atlas dimension (width or height) in pixels
 * \param[out] rectangles Output rectangles with packed positions (x, y, w, h, flipped flag)
 * \return PackingResult with packed indices and actual atlas dimensions
 */
template <bool EnableFlipping>
PackingResult<rectpack2D::output_rect_t<rectpack2D::empty_spaces<EnableFlipping>>> PackRectanglesIntoAtlas(
    const std::vector<std::pair<uint16_t, uint16_t>>& dimensions, const std::vector<size_t>& remaining_indices,
    uint32_t max_atlas_size,
    std::vector<rectpack2D::output_rect_t<rectpack2D::empty_spaces<EnableFlipping>>>& rectangles) noexcept {
    using spaces_type = rectpack2D::empty_spaces<EnableFlipping>;
    using rect_type = rectpack2D::output_rect_t<spaces_type>;

    PackingResult<rect_type> result;

    // Sentinel value to detect unpacked rectangles (rectpack2D sets valid x,y on successful pack)
    constexpr int UNPACKED_SENTINEL = -1;

    // Prepare rectangles for remaining items with sentinel x,y values
    rectangles.clear();
    rectangles.reserve(remaining_indices.size());

    for (size_t idx : remaining_indices) {
        const auto& [width, height] = dimensions[idx];
        rectangles.emplace_back(UNPACKED_SENTINEL, UNPACKED_SENTINEL, static_cast<int>(width), static_cast<int>(height),
                                false);
    }

    // Count successful packs
    size_t successful_count = 0;

    auto report_successful = [&](rect_type&) {
        ++successful_count;

        return rectpack2D::callback_result::CONTINUE_PACKING;
    };

    auto report_unsuccessful = [&](rect_type&) { return rectpack2D::callback_result::CONTINUE_PACKING; };

    const int discard_step = 1;

    // Pack into a fixed-size bin - rectpack2D modifies rectangles in-place
    (void)rectpack2D::find_best_packing<spaces_type>(
        rectangles, rectpack2D::make_finder_input(
                        max_atlas_size, discard_step, report_successful, report_unsuccessful,
                        EnableFlipping ? rectpack2D::flipping_option::ENABLED : rectpack2D::flipping_option::DISABLED));

    if (successful_count == 0) {
        return result;  // No rectangles fit
    }

    // Determine which rectangles were packed by checking for valid (non-sentinel) coordinates
    result.packed_indices.reserve(successful_count);

    uint16_t actual_width = 0;
    uint16_t actual_height = 0;

    for (size_t i = 0; i < rectangles.size(); ++i) {
        if (rectangles[i].x != UNPACKED_SENTINEL && rectangles[i].y != UNPACKED_SENTINEL) {
            result.packed_indices.push_back(i);

            uint16_t right = static_cast<uint16_t>(rectangles[i].x + rectangles[i].w);
            uint16_t bottom = static_cast<uint16_t>(rectangles[i].y + rectangles[i].h);

            actual_width = std::max(actual_width, right);
            actual_height = std::max(actual_height, bottom);
        }
    }

    result.atlas_width = actual_width;
    result.atlas_height = actual_height;
    result.success = true;

    return result;
}

}  // namespace Installer

#endif /* PROCEDURES_HPP */
