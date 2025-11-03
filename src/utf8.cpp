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

#include "utf8.hpp"

#include <SDL.h>
#include <utf8proc.h>

#include <stdexcept>
#include <vector>

static std::vector<utf8proc_int32_t> utf8_to_codepoints(const std::string& input);
static std::string codepoints_to_utf8(const std::vector<utf8proc_int32_t>& codepoints);

std::vector<utf8proc_int32_t> utf8_to_codepoints(const std::string& input) {
    const auto data = reinterpret_cast<const utf8proc_uint8_t*>(input.data());
    utf8proc_ssize_t length = input.size();
    std::vector<utf8proc_int32_t> codepoints;

    for (utf8proc_ssize_t i = 0, bytes_read; i < length; i += bytes_read) {
        utf8proc_int32_t codepoint;

        bytes_read = utf8proc_iterate(data + i, length - i, &codepoint);

        if (bytes_read < 0) {
            throw std::runtime_error("Failed to read UTF-8 codepoint.");
        }

        codepoints.push_back(codepoint);
    }

    return codepoints;
}

std::string codepoints_to_utf8(const std::vector<utf8proc_int32_t>& codepoints) {
    std::string result;
    utf8proc_uint8_t buf[sizeof(utf8proc_int32_t)];

    for (const auto codepoint : codepoints) {
        const utf8proc_ssize_t bytes_written = utf8proc_encode_char(codepoint, buf);

        if (bytes_written <= 0) {
            throw std::runtime_error("Failed to encode UTF-8 codepoint.");
        }

        result.append(reinterpret_cast<char*>(buf), bytes_written);
    }

    return result;
}

std::string utf8_tolower_str(const std::string& input) {
    std::string result = input;

    try {
        auto codepoints = utf8_to_codepoints(input);

        for (auto& codepoint : codepoints) {
            codepoint = utf8proc_tolower(codepoint);
        }

        result = codepoints_to_utf8(codepoints);

    } catch (const std::exception& e) {
        SDL_Log("\n%s\n", (std::string("UTF-8 parse error: ") + e.what()).c_str());
    }

    return result;
}

std::string utf8_toupper_str(const std::string& input) {
    std::string result = input;

    try {
        auto codepoints = utf8_to_codepoints(input);

        for (auto& codepoint : codepoints) {
            codepoint = utf8proc_toupper(codepoint);
        }

        result = codepoints_to_utf8(codepoints);

    } catch (const std::exception& e) {
        SDL_Log("\n%s\n", (std::string("UTF-8 parse error: ") + e.what()).c_str());
    }

    return result;
}

size_t utf8_strlen(const char* str) {
    if (!str) {
        return 0;
    }

    try {
        auto codepoints = utf8_to_codepoints(std::string(str));

        return codepoints.size();

    } catch (const std::exception& e) {
        SDL_Log("\nUTF-8 strlen error: %s\n", e.what());

        return strlen(str);
    }
}

size_t utf8_byte_offset(const char* str, size_t codepoint_index) {
    if (!str) {
        return 0;
    }

    const auto data = reinterpret_cast<const utf8proc_uint8_t*>(str);
    utf8proc_ssize_t length = strlen(str);
    size_t current_codepoint = 0;

    for (utf8proc_ssize_t i = 0; i < length && current_codepoint < codepoint_index;) {
        utf8proc_int32_t codepoint;
        utf8proc_ssize_t bytes_read = utf8proc_iterate(data + i, length - i, &codepoint);

        if (bytes_read <= 0) {
            break;
        }

        i += bytes_read;
        current_codepoint++;

        if (current_codepoint == codepoint_index) {
            return i;
        }
    }

    return length; /* Return end of string if index out of range */
}

size_t utf8_prev_char_offset(const char* str, size_t byte_offset) {
    if (!str || byte_offset == 0) {
        return 0;
    }

    /* Walk backward to find the start of the previous UTF-8 character */
    size_t pos = byte_offset;

    /* Move back at least one byte */
    if (pos > 0) {
        --pos;
    }

    /* Skip continuation bytes (0x80-0xBF) */
    while (pos > 0 && (static_cast<unsigned char>(str[pos]) & 0xC0) == 0x80) {
        --pos;
    }

    return pos;
}

size_t utf8_next_char_offset(const char* str, size_t byte_offset) {
    if (!str) {
        return 0;
    }

    size_t length = strlen(str);

    if (byte_offset >= length) {
        return length;
    }

    const auto data = reinterpret_cast<const utf8proc_uint8_t*>(str);

    utf8proc_int32_t codepoint;
    utf8proc_ssize_t bytes_read = utf8proc_iterate(data + byte_offset, length - byte_offset, &codepoint);

    if (bytes_read > 0) {
        return byte_offset + bytes_read;
    }

    return byte_offset + 1; /* Fallback: move one byte */
}
