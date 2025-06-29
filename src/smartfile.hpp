/* Copyright (c) 2021 M.A.X. Port Team
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

#ifndef SMARTFILE_HPP
#define SMARTFILE_HPP

#include <cstdio>

#include "fileobject.hpp"
#include "smartarray.hpp"
#include "smartlist.hpp"

enum class SmartFileFormat : uint16_t {
    UNSPECIFIED = 0,
    V70 = 70,
    V71 = 71,
    LATEST = 71,
    UNSUPPORTED = 0xFFFF,
};

class SmartFileReader {
    uint16_t m_format;

    void LoadObject(FileObject& object) noexcept;
    [[nodiscard]] uint16_t ReadIndex() noexcept;
    void SetFormat(const uint16_t format) noexcept;

protected:
    FILE* file{nullptr};
    SmartArray<FileObject> read_objects;

public:
    SmartFileReader() noexcept;
    explicit SmartFileReader(const char* path) noexcept;
    ~SmartFileReader() noexcept;

    bool Open(const char* path) noexcept;
    bool Close() noexcept;
    bool Read(void* buffer, size_t size) noexcept;
    template <typename T>
    bool Read(T& buffer) noexcept;
    [[nodiscard]] uint16_t ReadObjectCount() noexcept;
    [[nodiscard]] FileObject* ReadObject() noexcept;
    [[nodiscard]] SmartFileFormat GetFormat() noexcept;
};

class SmartFileWriter {
    uint16_t m_format;

    void SaveObject(FileObject* object) noexcept;
    void WriteIndex(uint16_t index) noexcept;

protected:
    FILE* file{nullptr};
    SmartList<FileObject> objects;

    void AddObject(FileObject* object) noexcept;

public:
    SmartFileWriter() noexcept;
    explicit SmartFileWriter(const char* path) noexcept;
    ~SmartFileWriter() noexcept;

    bool Open(const char* path) noexcept;
    bool Close() noexcept;
    bool Write(const void* buffer, size_t size) noexcept;
    template <typename T>
    bool Write(const T& buffer) noexcept;
    void WriteObjectCount(uint16_t count) noexcept;
    void WriteObject(FileObject* object) noexcept;
    [[nodiscard]] SmartFileFormat GetFormat() noexcept;
    [[nodiscard]] bool SetFormat(const uint16_t format) noexcept;
};

template <typename T>
inline bool SmartFileReader::Read(T& buffer) noexcept {
    return Read(&buffer, sizeof(T));
}

template <typename T>
inline bool SmartFileWriter::Write(const T& buffer) noexcept {
    return Write(&buffer, sizeof(T));
}

#endif /* SMARTFILE_HPP */
