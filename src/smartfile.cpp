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

#include "registerarray.hpp"

#define SMARTFILE_OBJECT_ARRAY_DEFAULT_CAPACITY 5

FileObject::FileObject() : object_index(0) {}
FileObject::FileObject(const FileObject& other) : object_index(other.object_index) {}
FileObject::~FileObject() {}

TextFileObject::TextFileObject() {}
TextFileObject::TextFileObject(const TextFileObject& other) {}
TextFileObject::~TextFileObject() {}

SmartFileReader::SmartFileReader() : file(nullptr), read_objects(SMARTFILE_OBJECT_ARRAY_DEFAULT_CAPACITY) {}

SmartFileReader::SmartFileReader(const char* const path)
    : file(nullptr), read_objects(SMARTFILE_OBJECT_ARRAY_DEFAULT_CAPACITY) {
    Open(path);
}

SmartFileReader::~SmartFileReader() { Close(); }

bool SmartFileReader::Open(const char* const path) {
    Close();

    file = fopen(path, "rb");

    return file != nullptr;
}

void SmartFileReader::Close() {
    read_objects.Release();

    if (file) {
        fclose(file);
        file = nullptr;
    }
}

void SmartFileReader::Read(void* buffer, int size) {
    int read_count;

    read_count = fread(buffer, size, 1, file);
    SDL_assert(read_count == 1);
}

void SmartFileReader::LoadObject(TextFileObject& object) {
    read_objects.Insert(object);

    object.FileLoad(*this);
}

unsigned short SmartFileReader::ReadIndex() {
    unsigned short value;

    Read(value);

    return value;
}

TextFileObject* SmartFileReader::ReadObject() {
    unsigned short object_index;
    unsigned short type_index;
    TextFileObject* object;

    object_index = ReadIndex();

    if (0 == object_index) {
        object = nullptr;
    } else {
        if (read_objects.GetCount() >= object_index) {
            object = &read_objects[object_index - 1];
        } else {
            SDL_assert(object_index == read_objects.GetCount() + 1);
            type_index = ReadIndex();
            SDL_assert(type_index != 0);
            SDL_assert(type_index <= registered_classes->GetCount());

            object = (*registered_classes)[type_index - 1].Allocate();
            LoadObject(*object);
        }
    }

    return object;
}

SmartFileWriter::SmartFileWriter() : file(nullptr), objects() {}

SmartFileWriter::SmartFileWriter(const char* const path) : file(nullptr), objects() { file = fopen(path, "wb"); }

SmartFileWriter::~SmartFileWriter() { Close(); }

bool SmartFileWriter::Open(const char* const path) {
    Close();

    file = fopen(path, "wb");

    return file != nullptr;
}

void SmartFileWriter::Close() {
    for (SmartList<TextFileObject>::Iterator it = objects.Begin(); it != objects.End(); ++it) {
        (*it).SetIndex(0);
    }

    objects.Clear();

    if (file) {
        fclose(file);
        file = nullptr;
    }
}

void SmartFileWriter::Write(void* buffer, int size) {
    int write_count;
    write_count = fwrite(buffer, size, 1, file);
    SDL_assert(write_count == 1);
}

void SmartFileWriter::AddObject(TextFileObject* object) {
    objects.PushBack(*object);
    object->SetIndex(objects.GetCount());
}

void SmartFileWriter::SaveObject(TextFileObject* object) {
    AddObject(object);
    object->FileSave(*this);
}

void SmartFileWriter::WriteIndex(unsigned short index) { Write(index); }

void SmartFileWriter::WriteObject(TextFileObject* object) {
    if (nullptr == object) {
        WriteIndex(0);
    } else {
        unsigned short object_index;

        object_index = object->GetIndex();

        if (object_index) {
            Write(object_index);
        } else {
            WriteIndex(objects.GetCount() + 1);
            WriteIndex(object->GetTypeIndex());
            SaveObject(object);
        }
    }
}
