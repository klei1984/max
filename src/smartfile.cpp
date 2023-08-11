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

SmartFileReader::SmartFileReader() : file(nullptr) {}

SmartFileReader::SmartFileReader(const char* const path) : file(nullptr) { Open(path); }

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

void SmartFileReader::Read(void* buffer, int32_t size) {
    int32_t read_count;

    read_count = fread(buffer, size, 1, file);
    SDL_assert(read_count == 1);
}

void SmartFileReader::LoadObject(FileObject& object) {
    read_objects.Insert(&object);

    object.FileLoad(*this);
}

uint16_t SmartFileReader::ReadIndex() {
    uint16_t value;

    Read(value);

    return value;
}

uint16_t SmartFileReader::ReadObjectCount() {
    uint16_t value;

    Read(value);

    return value;
}

FileObject* SmartFileReader::ReadObject() {
    uint16_t object_index;
    uint16_t type_index;
    FileObject* object;

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
            SDL_assert(type_index <= RegisterClass::GetRegister().GetCount());

            object = RegisterClass::GetRegister()[type_index - 1].Allocate();
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
    for (SmartList<FileObject>::Iterator it = objects.Begin(); it != objects.End(); ++it) {
        (*it).SetIndex(0);
    }

    objects.Clear();

    if (file) {
        fclose(file);
        file = nullptr;
    }
}

void SmartFileWriter::Write(void* buffer, int32_t size) {
    int32_t write_count;
    write_count = fwrite(buffer, size, 1, file);
    SDL_assert(write_count == 1);
}

void SmartFileWriter::AddObject(FileObject* object) {
    objects.PushBack(*object);
    object->SetIndex(objects.GetCount());
}

void SmartFileWriter::SaveObject(FileObject* object) {
    AddObject(object);
    object->FileSave(*this);
}

void SmartFileWriter::WriteIndex(uint16_t index) { Write(index); }

void SmartFileWriter::WriteObjectCount(uint16_t count) { Write(count); }

void SmartFileWriter::WriteObject(FileObject* object) {
    if (nullptr == object) {
        WriteIndex(0);
    } else {
        uint16_t object_index;

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
