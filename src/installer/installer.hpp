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

#ifndef INSTALLER_HPP
#define INSTALLER_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace Installer {

enum class ExitCode {
    Success = 0, /**< Installer completed successfully */
    Failure = 1, /**< Installer encountered an error */
    Skipped = 2  /**< Installer mode not requested, continue to normal game mode */
};

/**
 * \brief Input specification for multi-language video processing defining paths and language tag for a single language
 *        source.
 */
struct LanguageInput {
    std::filesystem::path game_data_path; /**< Path to the game data directory containing video source files. */
    std::filesystem::path base_path;      /**< Path to the base directory containing PATCHES.RES and other assets. */
    std::string language_tag;             /**< IETF language tag (e.g., "en-US", "de-DE", "fr-FR") for this input. */
};

/**
 * \brief Manages all M.A.X. Port asset installation and conversion steps including audio, video, graphics, and sprite
 *        processing.
 *
 * This class provides a unified interface for converting game assets from their legacy formats to modern cross-platform
 * formats.
 */
class MaxInstaller {
public:
    /**
     * \brief Constructs an Installer instance with default configuration paths derived from executable location.
     */
    MaxInstaller();

    /**
     * \brief Constructs an Installer instance with explicit game data and output paths for asset conversion operations.
     *
     * \param game_data_path Path to the directory containing game data files.
     * \param base_path Path to the directory containing PATCHES.RES.
     * \param output_path Destination directory where converted asset files will be written.
     */
    MaxInstaller(const std::filesystem::path& game_data_path, const std::filesystem::path& base_path,
                 const std::filesystem::path& output_path);

    /**
     * \brief Executes the full installation workflow converting all game assets.
     *
     * \param argc Argument count from main() function call.
     * \param argv Argument vector from main() function call containing command-line flags and paths.
     * \return One of the ExitCode enumerated values.
     */
    ExitCode Run(int argc, char* argv[]);

private:
    std::filesystem::path m_game_data_path;
    std::filesystem::path m_base_path;
    std::filesystem::path m_output_path;
    std::string m_language_tag;
    std::string m_mode;
    bool m_show_progress;
    int m_max_threads;
    std::vector<LanguageInput> m_video_language_inputs;
};

}  // namespace Installer

#endif /* INSTALLER_HPP */
