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

#include "enums.hpp"
#include "mission.hpp"

struct SaveFileInfo {
    std::filesystem::path file_path;
    std::vector<uint8_t> script;
    std::string mission;
    std::string world;
    std::string save_name;
    std::string file_name;
    std::string team_names[PLAYER_TEAM_MAX];
    uint32_t version;
    uint32_t save_file_category;
    uint32_t team_type[PLAYER_TEAM_MAX];
    uint32_t team_clan[PLAYER_TEAM_MAX];
    uint32_t random_seed;
};

bool SaveLoad_LoadIniOptions(const MissionCategory mission_category, const int32_t save_slot);
bool SaveLoad_GetSaveFileInfo(const MissionCategory mission_category, const int32_t save_slot,
                              struct SaveFileInfo &save_file_info);
[[nodiscard]] bool SaveLoad_IsSaveFileFormatSupported(const uint32_t format_version);
bool SaveLoad_Save(const std::filesystem::path &filepath, const char *const save_name, const uint32_t rng_seed);
bool SaveLoad_Load(const std::filesystem::path &filepath, const MissionCategory mission_category, bool ini_load_mode,
                   bool is_remote_game);
std::string SaveLoad_GetSaveFileName(const MissionCategory mission_category, const uint32_t save_slot);
[[nodiscard]] MissionCategory SaveLoad_GetSaveFileCategory(const MissionCategory mission_category);

#endif /* SAVELOAD_HPP */
