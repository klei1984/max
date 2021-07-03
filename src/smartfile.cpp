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

#include "smartfile.hpp"

#define SMARTFILE_OBJECT_ARRAY_DEFAULT_CAPACITY 5

FileObject::FileObject() : field_6(0) {}
FileObject::FileObject(const FileObject& other) : field_6(other.field_6) {}
FileObject::~FileObject() {}

SmartFile::SmartFile() : file(nullptr), read_objects(SMARTFILE_OBJECT_ARRAY_DEFAULT_CAPACITY) {}

SmartFile::SmartFile(const char* const path) : file(nullptr), read_objects(SMARTFILE_OBJECT_ARRAY_DEFAULT_CAPACITY) {
    Open(path);
}

SmartFile::~SmartFile() { Close(); }

bool SmartFile::Open(const char* const path) {
    Close();

    file = fopen(path, "rb");

    return file != nullptr;
}

void SmartFile::Close() {
    read_objects.Release();

    if (file) {
        fclose(file);
        file = nullptr;
    }
}

void SmartFile::Read(void* buffer, int size) { fread(buffer, size, sizeof(char), file); }
