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
#include "smartlist.hpp"
#include "textfileobject.hpp"

class SmartFileReader {
    void LoadObject(TextFileObject& object);
    unsigned short ReadIndex();

protected:
    FILE* file;
    SmartArray<TextFileObject> read_objects;

public:
    SmartFileReader();
    SmartFileReader(const char* const path);
    ~SmartFileReader();

    bool Open(const char* const path);
    void Close();
    void Read(void* buffer, int size);
    template <typename T>
    void Read(T& buffer);
    unsigned short ReadObjectCount();
    TextFileObject* ReadObject();
};

class SmartFileWriter {
    void SaveObject(TextFileObject* object);
    void WriteIndex(unsigned short index);

protected:
    SmartList<TextFileObject> objects;
    FILE* file;

    void AddObject(TextFileObject* object);

public:
    SmartFileWriter();
    SmartFileWriter(const char* const path);
    ~SmartFileWriter();

    bool Open(const char* const path);
    void Close();
    void Write(void* buffer, int size);
    template <typename T>
    void Write(T& buffer);
    void WriteObjectCount(unsigned short count);
    void WriteObject(TextFileObject* object);
};

template <typename T>
void SmartFileReader::Read(T& buffer) {
    Read(&buffer, sizeof(T));
}

template <typename T>
void SmartFileWriter::Write(T& buffer) {
    Write(&buffer, sizeof(T));
}

#endif /* SMARTFILE_HPP */
