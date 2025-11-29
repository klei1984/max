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

#include "moviesubtitle.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <fstream>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "gnw.h"
#include "grbuf.h"
#include "settings.hpp"
#include "text.h"

using json = nlohmann::json;
using validator = nlohmann::json_schema::json_validator;

static constexpr float SUBTITLE_VERTICAL_POSITION = 0.90f;
static constexpr int32_t SUBTITLE_BOTTOM_PADDING = 16;
static constexpr int32_t SUBTITLE_OUTLINE_PADDING = 2;

// Palette indices used for intermediate INDEX8 rendering
static constexpr uint8_t SUBTITLE_INDEX_TEXT = 0x02;
static constexpr uint8_t SUBTITLE_INDEX_OUTLINE = 0x01;
static constexpr uint8_t SUBTITLE_INDEX_TRANSPARENT = 0x00;

// RGBA colors for final subtitle rendering (ARGB format: 0xAARRGGBB)
static constexpr uint32_t SUBTITLE_RGBA_TEXT = 0xFFFFFFFF;         // White text, fully opaque
static constexpr uint32_t SUBTITLE_RGBA_OUTLINE = 0xFF000000;      // Black outline, fully opaque
static constexpr uint32_t SUBTITLE_RGBA_TRANSPARENT = 0x00000000;  // Transparent

MovieSubtitle::MovieSubtitle(const std::string& language) : m_language(language) {}

MovieSubtitle::~MovieSubtitle() { ClearCache(); }

std::string MovieSubtitle::LoadSchema() {
    uint32_t file_size = ResourceManager_GetResourceSize(SC_SCHEE);
    uint8_t* file_base = ResourceManager_ReadResource(SC_SCHEE);

    SDL_assert(file_size && file_base);

    for (size_t i = 0; i < file_size; ++i) {
        file_base[i] = file_base[i] ^ ResourceManager_GenericTable[i % sizeof(ResourceManager_GenericTable)];
    }

    std::string schema(reinterpret_cast<const char*>(file_base), file_size);

    delete[] file_base;

    return schema;
}

bool MovieSubtitle::LoadFromResource(ResourceID resource_id) {
    if (resource_id == INVALID_ID) {
        return false;
    }

    char* filename = reinterpret_cast<char*>(ResourceManager_ReadResource(resource_id));

    if (!filename) {
        SDL_Log("MovieSubtitle: Failed to read filename from resource %d\n", resource_id);
        return false;
    }

    auto filepath = (ResourceManager_FilePathGameBase / filename).lexically_normal();

    delete[] filename;

    std::ifstream fs(filepath);

    if (!fs) {
        SDL_Log("MovieSubtitle: Failed to open file: %s\n", filepath.string().c_str());
        return false;
    }

    std::string json_content((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

    return ParseJson(json_content, LoadSchema());
}

bool MovieSubtitle::ParseJson(const std::string& json_content, const std::string& schema_content) {
    try {
        json j = json::parse(json_content);

        if (!schema_content.empty()) {
            try {
                json jschema = json::parse(schema_content);
                validator schema_validator;

                schema_validator.set_root_schema(jschema);
                schema_validator.validate(j);

            } catch (const std::exception& e) {
                SDL_Log("MovieSubtitle: Schema validation failed: %s\n", e.what());

                return false;
            }
        }

        if (!j.contains("subtitle") || !j["subtitle"].is_object()) {
            SDL_Log("MovieSubtitle: JSON missing 'subtitle' object\n");

            return false;
        }

        const auto& subtitle_obj = j["subtitle"];

        std::string selected_language;

        if (subtitle_obj.contains(m_language)) {
            selected_language = m_language;

        } else if (subtitle_obj.contains("en-US")) {
            selected_language = "en-US";

        } else {
            SDL_Log("MovieSubtitle: No subtitle languages available\n");

            return false;
        }

        m_captions.clear();

        for (const auto& entry : subtitle_obj[selected_language]) {
            CaptionEntry caption;

            caption.start_frame = entry["start"].get<int32_t>();
            caption.end_frame = entry["end"].get<int32_t>();
            caption.caption = entry["caption"].get<std::string>();

            if (caption.start_frame >= caption.end_frame) {
                SDL_Log("MovieSubtitle: Skipping entry with start >= end: '%s' (start=%d, end=%d)\n",
                        caption.caption.substr(0, 40).c_str(), caption.start_frame, caption.end_frame);
                continue;
            }

            m_captions.push_back(std::move(caption));
        }

        std::sort(m_captions.begin(), m_captions.end(),
                  [](const CaptionEntry& a, const CaptionEntry& b) { return a.start_frame < b.start_frame; });

        m_loaded = !m_captions.empty();
        m_current_index = -1;

        return m_loaded;

    } catch (const json::parse_error& e) {
        SDL_Log("MovieSubtitle: JSON parse error: %s\n", e.what());
        return false;

    } catch (const std::exception& e) {
        SDL_Log("MovieSubtitle: Error parsing subtitles: %s\n", e.what());
        return false;
    }
}

const std::string* MovieSubtitle::GetSubtitleForFrame(int32_t frame) const {
    if (!m_loaded || m_captions.empty()) {
        return nullptr;
    }

    auto it = std::upper_bound(m_captions.begin(), m_captions.end(), frame,
                               [](int32_t f, const CaptionEntry& entry) { return f < entry.start_frame; });

    if (it != m_captions.begin()) {
        --it;
        if (frame >= it->start_frame && frame < it->end_frame) {
            return &it->caption;
        }
    }

    return nullptr;
}

void MovieSubtitle::ClearCache() {
    m_cache_buffer.reset();
    m_cached_text.clear();
    m_cache_width = 0;
    m_cache_height = 0;
}

size_t MovieSubtitle::FindBestSplitPoint(const std::string& text) {
    const size_t target = text.length() / 2;

    // Search for a space near the middle of the string
    size_t best_pos = std::string::npos;
    size_t best_distance = std::string::npos;

    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == ' ') {
            const size_t distance = (i > target) ? (i - target) : (target - i);

            if (distance < best_distance) {
                best_distance = distance;
                best_pos = i;
            }
        }
    }

    return best_pos;
}

void MovieSubtitle::RenderTextToCache(const std::string& text, int32_t max_width) {
    const int32_t old_font = Text_GetFont();

    Text_SetFont(GNW_TEXT_FONT_5);

    const int32_t text_width = Text_GetWidth(text.c_str());
    const int32_t text_height = Text_GetHeight();
    const int32_t split_threshold = max_width / 2;  // Split if text exceeds 50% of buffer width

    // Check if we need to split into two lines
    std::string line1;
    std::string line2;
    int32_t line1_width;
    int32_t line2_width;
    int32_t num_lines;

    if (text_width > split_threshold) {
        const size_t split_pos = FindBestSplitPoint(text);

        if (split_pos != std::string::npos) {
            line1 = text.substr(0, split_pos);
            line2 = text.substr(split_pos + 1);  // Skip the space
            line1_width = Text_GetWidth(line1.c_str());
            line2_width = Text_GetWidth(line2.c_str());
            num_lines = 2;

        } else {
            // No space found, render as single line (will be clipped)
            line1 = text;
            line1_width = text_width;
            line2_width = 0;
            num_lines = 1;
        }

    } else {
        line1 = text;
        line1_width = text_width;
        line2_width = 0;
        num_lines = 1;
    }

    const int32_t render_width = std::max(line1_width, line2_width);

    m_cache_width = render_width + SUBTITLE_OUTLINE_PADDING * 2;
    m_cache_height = text_height * num_lines + SUBTITLE_OUTLINE_PADDING * 2;

    // First render to indexed buffer using distinct indices
    const size_t index8_size = m_cache_width * m_cache_height;
    auto index8_buffer = std::make_unique<uint8_t[]>(index8_size);

    memset(index8_buffer.get(), SUBTITLE_INDEX_TRANSPARENT, index8_size);

    static const int32_t offsets[][2] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};

    // Render first line centered
    const int32_t line1_x_offset = (render_width - line1_width) / 2;
    uint8_t* buf_line1 = index8_buffer.get() + SUBTITLE_OUTLINE_PADDING * m_cache_width + SUBTITLE_OUTLINE_PADDING;

    for (const auto& offset : offsets) {
        uint8_t* outline_buf = buf_line1 + offset[1] * m_cache_width + offset[0] + line1_x_offset;

        Text_Blit(outline_buf, line1.c_str(), line1_width, m_cache_width, SUBTITLE_INDEX_OUTLINE);
    }

    Text_Blit(buf_line1 + line1_x_offset, line1.c_str(), line1_width, m_cache_width, SUBTITLE_INDEX_TEXT);

    // Render second line if present, centered
    if (num_lines == 2) {
        const int32_t line2_x_offset = (render_width - line2_width) / 2;
        uint8_t* buf_line2 = buf_line1 + text_height * m_cache_width;

        for (const auto& offset : offsets) {
            uint8_t* outline_buf = buf_line2 + offset[1] * m_cache_width + offset[0] + line2_x_offset;

            Text_Blit(outline_buf, line2.c_str(), line2_width, m_cache_width, SUBTITLE_INDEX_OUTLINE);
        }

        Text_Blit(buf_line2 + line2_x_offset, line2.c_str(), line2_width, m_cache_width, SUBTITLE_INDEX_TEXT);
    }

    // Convert indexed to RGBA using configurable colors from settings
    m_cache_buffer = std::make_unique<uint32_t[]>(index8_size);

    const auto& settings = ResourceManager_GetSettings();
    const uint32_t rgba_text =
        static_cast<uint32_t>(settings->GetNumericValue("subtitle_text_color", SUBTITLE_RGBA_TEXT));
    const uint32_t rgba_outline =
        static_cast<uint32_t>(settings->GetNumericValue("subtitle_outline_color", SUBTITLE_RGBA_OUTLINE));
    const uint32_t rgba_transparent =
        static_cast<uint32_t>(settings->GetNumericValue("subtitle_bg_color", SUBTITLE_RGBA_TRANSPARENT));

    for (size_t i = 0; i < index8_size; ++i) {
        switch (index8_buffer[i]) {
            case SUBTITLE_INDEX_TEXT:
                m_cache_buffer[i] = rgba_text;
                break;
            case SUBTITLE_INDEX_OUTLINE:
                m_cache_buffer[i] = rgba_outline;
                break;
            default:
                m_cache_buffer[i] = rgba_transparent;
                break;
        }
    }

    m_cached_text = text;

    Text_SetFont(old_font);
}

bool MovieSubtitle::RenderSubtitle(int32_t buffer_width, int32_t buffer_height, int32_t frame) {
    const std::string* text = GetSubtitleForFrame(frame);

    if (!text || text->empty()) {
        // No subtitle for this frame - clear cache
        if (!m_cached_text.empty()) {
            ClearCache();
        }

        return false;
    }

    if (*text != m_cached_text) {
        RenderTextToCache(*text, buffer_width);
    }

    if (!m_cache_buffer || m_cache_width == 0 || m_cache_height == 0) {
        return false;
    }

    int32_t dst_x = (buffer_width - m_cache_width) / 2;
    int32_t dst_y = static_cast<int32_t>(buffer_height * SUBTITLE_VERTICAL_POSITION) - m_cache_height / 2;

    if (dst_y + m_cache_height > buffer_height - SUBTITLE_BOTTOM_PADDING) {
        dst_y = buffer_height - SUBTITLE_BOTTOM_PADDING - m_cache_height;
    }

    if (dst_x < 0) {
        dst_x = 0;
    }
    if (dst_y < 0) {
        dst_y = 0;
    }

    const int32_t copy_width = std::min(m_cache_width, buffer_width - dst_x);
    const int32_t copy_height = std::min(m_cache_height, buffer_height - dst_y);

    if (copy_width <= 0 || copy_height <= 0) {
        return false;
    }

    // Store computed destination for caller to use with Svga_BlitRGBA
    m_dst_x = dst_x;
    m_dst_y = dst_y;
    m_copy_width = copy_width;
    m_copy_height = copy_height;

    return true;
}
