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

#ifndef SAVELOAD_HPP
#define SAVELOAD_HPP

#include <cstdint>
#include <filesystem>
#include <variant>

constexpr size_t SaveLoad_HashSize{64u};
constexpr size_t SaveLoad_TeamCount{5u};

struct SaveFileInfo {
    std::string save_name;
    std::string file_name;
    std::string team_names[SaveLoad_TeamCount];
    uint32_t version;
    uint32_t save_game_type;
    uint32_t team_type[SaveLoad_TeamCount];
    uint32_t team_clan[SaveLoad_TeamCount];
    uint32_t rng_seed;
    uint8_t mission[SaveLoad_HashSize];
    uint8_t world[SaveLoad_HashSize];
};

bool SaveLoad_LoadIniOptions(const int32_t save_slot, const int32_t game_file_type);
bool SaveLoad_GetSaveFileInfo(const int32_t save_slot, const int32_t game_file_type,
                              struct SaveFileInfo &save_file_header);
[[nodiscard]] bool SaveLoad_IsSaveFileFormatSupported(const uint32_t format_version);
void SaveLoad_Save(const std::filesystem::path &filepath, const char *const save_name, const uint32_t rng_seed);
bool SaveLoad_Load(const std::filesystem::path &filepath, int32_t save_slot, int32_t game_file_type, bool ini_load_mode,
                   bool is_remote_game);

#endif /* SAVELOAD_HPP */
