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

#ifndef GAMESETUP_HPP
#define GAMESETUP_HPP

#include <filesystem>

/**
 * \brief Tests whether a folder holds the original game assets.
 *
 * A folder qualifies when it contains the MAX.RES resource file. Both upper and lower case file
 * names are accepted, as original media and existing installations use either.
 *
 * \param path Folder to test.
 * \return True if the folder holds the original game assets.
 */
[[nodiscard]] bool GameSetup_IsGameDataFolder(const std::filesystem::path& path);

/**
 * \brief Runs the interactive first run setup to select the original game assets folder.
 *
 * The player is guided by native message boxes and a native folder picker to the folder that holds
 * the original game assets, an original M.A.X. CD-ROM or an existing full installation. Both the
 * selected folder itself and its MAX subfolder are accepted, mirroring the Windows installer. The
 * accepted folder is persisted into the game settings, and save files found next to the game assets
 * are copied into the preferences folder once, without overwriting existing files.
 *
 * The SDL video subsystem must be initialized, and the game settings, language and preferences
 * infrastructure must be available before calling this.
 *
 * \param game_data_path Receives the accepted game assets folder.
 * \return True when a valid folder was configured, false when the player quit the setup.
 */
[[nodiscard]] bool GameSetup_Run(std::filesystem::path& game_data_path);

#endif /* GAMESETUP_HPP */
