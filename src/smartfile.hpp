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

#include "smartarray.hpp"

class FileObject;

class SmartFile {
    FILE* file;
    SmartArray<FileObject> read_objects;

public:
    SmartFile();
    SmartFile(const char* const path);
    ~SmartFile();

    bool Open(const char* const path);
    void Close();
    void Read(void* buffer, int size);
    template <typename T>
    void Read(T& buffer);
};

template <typename T>
void SmartFile::Read(T& buffer) {
    Read(&buffer, sizeof(T));
}

class FileObject : public SmartObject {
    unsigned short field_6;

public:
    FileObject();
    FileObject(const FileObject& other);
    ~FileObject();

    virtual void Unknown() = 0;
    virtual void FileLoad(SmartFile& file) = 0;
    virtual void FileSave() = 0;
};

#endif /* SMARTFILE_HPP */
