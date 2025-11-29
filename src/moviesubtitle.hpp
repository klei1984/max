/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef MOVIESUBTITLE_HPP
#define MOVIESUBTITLE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "resource_manager.hpp"

struct CaptionEntry {
    int32_t start_frame;
    int32_t end_frame;
    std::string caption;
};

/**
 * \brief Manages subtitle loading, caching, and rendering for MVE video playback.
 *
 * This class loads subtitle data from JSON files and provides lookup by frame number. It also handles rendering of
 * subtitles with text outline onto a video frame buffer.
 */
class MovieSubtitle {
public:
    /**
     * \brief Constructs a MovieSubtitle object.
     *
     * \param language Reference to the current language setting (e.g., "en-US").
     */
    explicit MovieSubtitle(const std::string& language);

    ~MovieSubtitle();

    MovieSubtitle(const MovieSubtitle&) = delete;
    MovieSubtitle& operator=(const MovieSubtitle&) = delete;
    MovieSubtitle(MovieSubtitle&&) noexcept = default;
    MovieSubtitle& operator=(MovieSubtitle&&) noexcept = default;

    /**
     * \brief Loads subtitle data from a resource.
     *
     * The resource contains the filename of the subtitle JSON file. The file is loaded from
     * ResourceManager_FilePathGameBase / filename.
     *
     * \param resource_id The resource ID containing the subtitle filename.
     * \return True if loading succeeded, false otherwise.
     */
    [[nodiscard]] bool LoadFromResource(ResourceID resource_id);

    /**
     * \brief Gets the subtitle text for a specific frame.
     *
     * \param frame The current video frame index.
     * \return Pointer to the subtitle text, or nullptr if no subtitle for this frame.
     */
    [[nodiscard]] const std::string* GetSubtitleForFrame(int32_t frame) const;

    /**
     * \brief Renders the current subtitle for a video frame.
     *
     * Renders the subtitle text centered horizontally at the bottom of the frame.
     *
     * \param buffer_width Width of the buffer in pixels.
     * \param buffer_height Height of the buffer in pixels.
     * \param frame Current video frame index.
     * \return True if a subtitle was rendered, false if no subtitle for this frame.
     */
    bool RenderSubtitle(int32_t buffer_width, int32_t buffer_height, int32_t frame);

    /**
     * \brief Checks if subtitles are loaded.
     *
     * \return True if subtitles were loaded successfully, false otherwise.
     */
    [[nodiscard]] bool IsLoaded() const { return m_loaded; }

    /**
     * \brief Gets the cached RGBA subtitle buffer.
     *
     * \return Pointer to the RGBA buffer, or nullptr if no subtitle is cached.
     */
    [[nodiscard]] uint32_t* GetCacheBuffer() const { return m_cache_buffer.get(); }

    /**
     * \brief Gets the width of the cached subtitle.
     */
    [[nodiscard]] int32_t GetCacheWidth() const { return m_cache_width; }

    /**
     * \brief Gets the height of the cached subtitle.
     */
    [[nodiscard]] int32_t GetCacheHeight() const { return m_cache_height; }

    /**
     * \brief Gets the computed destination X coordinate after RenderSubtitle.
     */
    [[nodiscard]] int32_t GetDestX() const { return m_dst_x; }

    /**
     * \brief Gets the computed destination Y coordinate after RenderSubtitle.
     */
    [[nodiscard]] int32_t GetDestY() const { return m_dst_y; }

    /**
     * \brief Gets the computed copy width after RenderSubtitle.
     */
    [[nodiscard]] int32_t GetCopyWidth() const { return m_copy_width; }

    /**
     * \brief Gets the computed copy height after RenderSubtitle.
     */
    [[nodiscard]] int32_t GetCopyHeight() const { return m_copy_height; }

private:
    static std::string LoadSchema();
    bool ParseJson(const std::string& json_content, const std::string& schema_content = {});
    void RenderTextToCache(const std::string& text, int32_t max_width);
    static size_t FindBestSplitPoint(const std::string& text);
    void ClearCache();

    std::string m_language;
    std::vector<CaptionEntry> m_captions;

    bool m_loaded{false};

    std::string m_cached_text;
    std::unique_ptr<uint32_t[]> m_cache_buffer;

    int32_t m_cache_width{0};
    int32_t m_cache_height{0};
    int32_t m_current_index{-1};

    int32_t m_dst_x{0};
    int32_t m_dst_y{0};
    int32_t m_copy_width{0};
    int32_t m_copy_height{0};
};

#endif /* MOVIESUBTITLE_HPP */
