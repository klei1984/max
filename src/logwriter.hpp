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

#ifndef LOGWRITER_HPP
#define LOGWRITER_HPP

/* Log files of debug builds. One writer thread does all file access in batches, so the game threads only append text
 * and never wait for the disk. Release builds do not have it.
 */

#if !defined(NDEBUG)

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

class LogWriter {
    // text and what the writer does around it, kept in the order the log asked for
    struct Batch {
        bool open{false};
        std::filesystem::path path;
        std::filesystem::path rolled_path;
        bool binary{false};
        std::string text;
        bool roll{false};
        bool close{false};
    };

    std::mutex m_mutex;
    std::vector<Batch> m_batches;

    // only the writer thread uses these
    std::ofstream m_file;
    std::filesystem::path m_path;
    std::filesystem::path m_rolled_path;
    bool m_binary{false};

    Batch& GetBatch();
    void OpenFile();

public:
    /* Rolls an earlier file aside and starts a new one, text written after this goes to it.
     */
    void Open(const std::filesystem::path& path, const std::filesystem::path& rolled_path, bool binary);

    void Write(std::string_view text);

    /* Rolls the file aside and continues in a new one.
     */
    void Roll();

    void Close();

    /* Writes the waiting batches, called by the writer thread.
     */
    void WriteBatches();
};

#endif /* !defined(NDEBUG) */

#endif /* LOGWRITER_HPP */
