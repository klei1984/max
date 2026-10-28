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

#include "logwriter.hpp"

#if !defined(NDEBUG)

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <system_error>
#include <thread>

static std::mutex LogWriter_RegistryMutex;
static std::vector<LogWriter*> LogWriter_Registry;
static std::thread LogWriter_Thread;
static std::atomic<bool> LogWriter_Stop{false};

static void LogWriter_WriteAll() {
    std::lock_guard<std::mutex> lock(LogWriter_RegistryMutex);

    for (auto writer : LogWriter_Registry) {
        writer->WriteBatches();
    }
}

static void LogWriter_Loop() {
    while (!LogWriter_Stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        LogWriter_WriteAll();
    }

    LogWriter_WriteAll();
}

static void LogWriter_StopThread() {
    LogWriter_Stop = true;

    if (LogWriter_Thread.joinable()) {
        LogWriter_Thread.join();
    }
}

static void LogWriter_Register(LogWriter* writer) {
    static std::once_flag thread_once;

    {
        std::lock_guard<std::mutex> lock(LogWriter_RegistryMutex);

        if (std::find(LogWriter_Registry.begin(), LogWriter_Registry.end(), writer) == LogWriter_Registry.end()) {
            LogWriter_Registry.push_back(writer);
        }
    }

    std::call_once(thread_once, []() {
        LogWriter_Thread = std::thread(LogWriter_Loop);

        /* the batches still waiting reach their files when the game exits */
        std::atexit(LogWriter_StopThread);
    });
}

LogWriter::Batch& LogWriter::GetBatch() {
    if (m_batches.empty() || m_batches.back().roll || m_batches.back().close) {
        m_batches.emplace_back();
    }

    return m_batches.back();
}

void LogWriter::Open(const std::filesystem::path& path, const std::filesystem::path& rolled_path, bool binary) {
    LogWriter_Register(this);

    std::lock_guard<std::mutex> lock(m_mutex);
    Batch& batch = m_batches.emplace_back();

    batch.open = true;
    batch.path = path;
    batch.rolled_path = rolled_path;
    batch.binary = binary;
}

void LogWriter::Write(std::string_view text) {
    std::lock_guard<std::mutex> lock(m_mutex);

    GetBatch().text += text;
}

void LogWriter::Roll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    GetBatch().roll = true;
}

void LogWriter::Close() {
    std::lock_guard<std::mutex> lock(m_mutex);

    GetBatch().close = true;
}

void LogWriter::OpenFile() {
    std::error_code error;

    /* Roll the last file aside rather than reopening it truncated, whether it filled up or an earlier run or
     * session wrote it: the entries leading up to a crash or a restart are the ones worth keeping.
     */
    std::filesystem::rename(m_path, m_rolled_path, error);

    m_file.open(m_path, m_binary ? std::ios::out | std::ios::binary : std::ios::out);

    if (!m_file.is_open()) {
        SDL_Log("Log writer: cannot create %s.", m_path.string().c_str());
    }
}

void LogWriter::WriteBatches() {
    std::vector<Batch> batches;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        batches.swap(m_batches);
    }

    for (auto& batch : batches) {
        if (batch.open) {
            m_file.close();

            m_path = batch.path;
            m_rolled_path = batch.rolled_path;
            m_binary = batch.binary;

            OpenFile();
        }

        if (m_file.is_open()) {
            m_file << batch.text;
        }

        if (batch.roll || batch.close) {
            m_file.close();

            if (batch.roll) {
                OpenFile();
            }
        }
    }

    /* a crash loses at most the entries of the last few milliseconds */
    if (m_file.is_open()) {
        m_file.flush();
    }
}

#endif /* !defined(NDEBUG) */
