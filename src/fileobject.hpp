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

#ifndef FILEOBJECT_HPP
#define FILEOBJECT_HPP

#include "smartpointer.hpp"

class SmartFileReader;
class SmartFileWriter;

class FileObject : public SmartObject {
    uint32_t object_index{0uL};

public:
    FileObject() noexcept = default;
    FileObject(const FileObject& other) noexcept : object_index(other.object_index) {}
    ~FileObject() noexcept override = default;

    [[nodiscard]] virtual uint32_t GetTypeIndex() const = 0;
    virtual void FileLoad(SmartFileReader& file) noexcept = 0;
    virtual void FileSave(SmartFileWriter& file) noexcept = 0;

    [[nodiscard]] inline uint32_t GetIndex() const noexcept { return object_index; }
    inline void SetIndex(const uint32_t index) noexcept { object_index = index; }
};

#endif /* FILEOBJECT_HPP */
