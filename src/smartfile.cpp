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

SmartFileReader::SmartFileReader() noexcept : m_format(static_cast<uint16_t>(SmartFileFormat::UNSPECIFIED)) {};

SmartFileReader::SmartFileReader(const char* const path) noexcept
    : m_format(static_cast<uint16_t>(SmartFileFormat::UNSPECIFIED)) {
    Open(path);
}

SmartFileReader::~SmartFileReader() noexcept { Close(); }

void SmartFileReader::SetFormat(const uint16_t format) noexcept {
    switch (format) {
        case static_cast<uint16_t>(SmartFileFormat::V70): {
            m_format = static_cast<uint16_t>(SmartFileFormat::V70);
        } break;

        case static_cast<uint16_t>(SmartFileFormat::V71): {
            m_format = static_cast<uint16_t>(SmartFileFormat::V71);
        } break;

        default: {
            m_format = static_cast<uint16_t>(SmartFileFormat::UNSUPPORTED);
        } break;
    }
}

bool SmartFileReader::Open(const char* const path) noexcept {
    Close();

    file = fopen(path, "rb");

    if (file) {
        uint16_t format;

        if (fread(&format, sizeof(format), 1, file) != 1) {
            format = static_cast<uint16_t>(SmartFileFormat::UNSPECIFIED);
        }

        fseek(file, 0, SEEK_SET);

        SetFormat(format);
    }

    return file != nullptr;
}

bool SmartFileReader::Close() noexcept {
    bool result{false};

    read_objects.Release();

    if (file != nullptr) {
        result = fclose(file) != EOF;
        file = nullptr;
    }

    return result;
}

bool SmartFileReader::Read(void* const buffer, const size_t size) noexcept { return fread(buffer, size, 1, file) == 1; }

void SmartFileReader::LoadObject(FileObject& object) noexcept {
    read_objects.Insert(&object);

    object.FileLoad(*this);
}

[[nodiscard]] uint16_t SmartFileReader::ReadIndex() noexcept {
    uint16_t value{0};

    return Read(value) ? value : UINT16_C(0);
}

[[nodiscard]] uint16_t SmartFileReader::ReadObjectCount() noexcept {
    uint16_t value{0};

    return Read(value) ? value : UINT16_C(0);
}

[[nodiscard]] FileObject* SmartFileReader::ReadObject() noexcept {
    const uint16_t object_index = ReadIndex();
    FileObject* object{nullptr};

    if (object_index != UINT16_C(0)) {
        if (read_objects.GetCount() >= object_index) {
            object = &read_objects[object_index - 1];

        } else {
            const uint16_t type_index = ReadIndex();

            SDL_assert(object_index == read_objects.GetCount() + 1);
            SDL_assert(type_index != UINT16_C(0));
            SDL_assert(type_index <= RegisterClass::GetRegister().GetCount());

            object = RegisterClass::GetRegister()[type_index - 1].Allocate();
            LoadObject(*object);
        }
    }

    return object;
}

SmartFileWriter::SmartFileWriter() noexcept : m_format(static_cast<uint16_t>(SmartFileFormat::LATEST)) {};

SmartFileWriter::SmartFileWriter(const char* const path) noexcept
    : m_format(static_cast<uint16_t>(SmartFileFormat::LATEST)), file(fopen(path, "wb")) {}

SmartFileWriter::~SmartFileWriter() noexcept { Close(); }

bool SmartFileWriter::Open(const char* const path) noexcept {
    Close();

    file = fopen(path, "wb");

    return file != nullptr;
}

bool SmartFileWriter::Close() noexcept {
    bool result{false};

    for (auto it = objects.Begin(); it != objects.End(); ++it) {
        (*it).SetIndex(0);
    }

    objects.Clear();

    if (file != nullptr) {
        result = fclose(file) != EOF;
        file = nullptr;
    }

    return result;
}

bool SmartFileWriter::Write(const void* const buffer, const size_t size) noexcept {
    return fwrite(buffer, size, 1, file) == 1;
}

void SmartFileWriter::AddObject(FileObject* const object) noexcept {
    objects.PushBack(*object);
    object->SetIndex(objects.GetCount());
}

void SmartFileWriter::SaveObject(FileObject* const object) noexcept {
    AddObject(object);
    object->FileSave(*this);
}

void SmartFileWriter::WriteIndex(const uint16_t index) noexcept { Write(index); }

void SmartFileWriter::WriteObjectCount(const uint16_t count) noexcept { Write(count); }

void SmartFileWriter::WriteObject(FileObject* const object) noexcept {
    if (nullptr == object) {
        WriteIndex(0);

    } else {
        const uint16_t object_index = object->GetIndex();

        if (object_index != UINT16_C(0)) {
            Write(object_index);

        } else {
            WriteIndex(objects.GetCount() + 1);
            WriteIndex(object->GetTypeIndex());
            SaveObject(object);
        }
    }
}

SmartFileFormat SmartFileReader::GetFormat() noexcept { return static_cast<SmartFileFormat>(m_format); }

SmartFileFormat SmartFileWriter::GetFormat() noexcept { return static_cast<SmartFileFormat>(m_format); }

[[nodiscard]] bool SmartFileWriter::SetFormat(const uint16_t format) noexcept {
    bool result = true;

    switch (format) {
        case static_cast<uint16_t>(SmartFileFormat::V70): {
            m_format = static_cast<uint16_t>(SmartFileFormat::V70);
        } break;

        case static_cast<uint16_t>(SmartFileFormat::V71): {
            m_format = static_cast<uint16_t>(SmartFileFormat::V71);
        } break;

        default: {
            result = false;
        } break;
    }

    return result;
}
