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

#include "smartstring.hpp"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <new>

#ifdef __unix__
#define stricmp strcasecmp
#endif

#include "SDL_assert.h"

StringObject::StringObject(unsigned short size) : size(size), length(0), reference_count(1) {
    buffer = new (std::nothrow) char[size + 1];
    buffer[0] = '\0';
}

StringObject::StringObject(const char *cstring) : size(strlen(cstring)), length(size), reference_count(1) {
    buffer = new (std::nothrow) char[size + 1];
    strcpy(buffer, cstring);
}

StringObject::StringObject(const StringObject &other)
    : size(other.GetSize()), length(other.GetLength()), reference_count(1) {
    buffer = new (std::nothrow) char[size + 1];
    strcpy(buffer, other.GetCStr());
}

StringObject::~StringObject() {
    SDL_assert(reference_count == 0);
    delete[] buffer;
}

void StringObject::Resize(unsigned short size, bool keep) {
    if (size != this->size) {
        this->size = size;

        if (keep) {
            char *buffer;

            buffer = new (std::nothrow) char[size + 1];

            if (size >= this->length) {
                strcpy(buffer, this->buffer);
            } else {
                strncpy(buffer, this->buffer, size);
                buffer[size] = '\0';
                this->length = size;
            }

            delete[] this->buffer;
            this->buffer = buffer;
        } else {
            delete[] this->buffer;
            this->buffer = new (std::nothrow) char[size + 1];
            *this->buffer = '\0';
            this->length = 0;
        }
    }
}

unsigned short StringObject::GetLength() const { return length; }

unsigned short StringObject::GetSize() const { return size; }

char *StringObject::GetCStr() const { return buffer; }

void StringObject::SetLength(unsigned short length) { this->length = length; }

SmartString::SmartString() {
    static SmartString *empty_string = new (std::nothrow) SmartString(static_cast<unsigned short>(0));
    object_pointer = empty_string->object_pointer;
    Increment();
}

SmartString::SmartString(unsigned short size) { object_pointer = new (std::nothrow) StringObject(size); }

SmartString::SmartString(const char *cstring) { object_pointer = new (std::nothrow) StringObject(cstring); }

SmartString::SmartString(const SmartString &other) : object_pointer(other.object_pointer) { Increment(); }

SmartString::~SmartString() { Decrement(); }

void SmartString::Increment() const {
    if (object_pointer) {
        ++object_pointer->reference_count;
    }
}

void SmartString::Decrement() {
    if (object_pointer && --object_pointer->reference_count == 0) {
        delete object_pointer;
        object_pointer = nullptr;
    }
}

void SmartString::Resize(unsigned short size, bool keep) {
    Copy();
    object_pointer->Resize(size, keep);
}

void SmartString::Copy() {
    if (!IsLastReference()) {
        Decrement();
        object_pointer = new (std::nothrow) StringObject(*object_pointer);
    }
}

bool SmartString::IsLastReference() { return 1 == object_pointer->reference_count; }

unsigned short SmartString::CalcOptimalCapacity(unsigned short needed_capacity) {
    return needed_capacity / 64u * 64u + 64u;
}

char *SmartString::GetCStr() const { return object_pointer->GetCStr(); }

unsigned short SmartString::GetLength() const { return object_pointer->GetLength(); }

/// \todo This API seems to handle length inconsistently
SmartString SmartString::Substr(unsigned short position, unsigned short length) {
    int size;
    int max_length;
    SmartString result;
    static SmartString empty_string;

    max_length = length;

    if (length > object_pointer->GetLength() - 1) {
        max_length = object_pointer->GetLength() - 1;
    }

    if (max_length >= position) {
        size = max_length - position + 1;
        SmartString string(size + 1);
        strncpy(string.GetCStr(), &GetCStr()[position], size);
        string[size] = '\0';
        string.object_pointer->SetLength(size);
        result = string;

    } else {
        result = empty_string;
    }

    return result;
}

char &SmartString::operator[](unsigned short position) {
    SDL_assert(position < object_pointer->GetSize());
    return object_pointer->GetCStr()[position];
}

SmartString &SmartString::operator+=(SmartString const &other) {
    unsigned short length;

    Copy();
    length = object_pointer->GetLength() + other.object_pointer->GetLength();

    if (object_pointer->GetSize() < length) {
        Resize(CalcOptimalCapacity(length));
    }

    strcpy(&GetCStr()[object_pointer->GetLength()], other.GetCStr());
    object_pointer->SetLength(length);

    return *this;
}

SmartString &SmartString::operator+=(const char *cstring) {
    unsigned short length;

    Copy();
    length = object_pointer->GetLength() + strlen(cstring);

    if (object_pointer->GetSize() < length) {
        Resize(CalcOptimalCapacity(length));
    }

    strcpy(&GetCStr()[object_pointer->GetLength()], cstring);
    object_pointer->SetLength(length);

    return *this;
}

SmartString &SmartString::operator+=(const char character) {
    unsigned short length;

    Copy();
    length = object_pointer->GetLength() + sizeof(char);

    if (object_pointer->GetSize() < length) {
        Resize(CalcOptimalCapacity(length));
    }

    GetCStr()[length - sizeof(char)] = character;
    GetCStr()[length] = '\0';
    object_pointer->SetLength(length);

    return *this;
}

char *SmartString::StrStr(const char *cstring) const { return strstr(GetCStr(), cstring); }

char *SmartString::StrStr(const SmartString &other) const { return strstr(GetCStr(), other.GetCStr()); }

SmartString &SmartString::Clear() {
    Copy();
    GetCStr()[0] = '\0';
    object_pointer->SetLength(0);

    return *this;
}

SmartString &SmartString::Toupper() {
    Copy();

    for (char *cstring = GetCStr(); *cstring; ++cstring) {
        *cstring = toupper(*cstring);
    }

    return *this;
}

SmartString &SmartString::Tolower() {
    Copy();

    for (char *cstring = GetCStr(); *cstring; ++cstring) {
        *cstring = tolower(*cstring);
    }

    return *this;
}

SmartString &SmartString::Sprintf(unsigned short size, const char *format, ...) {
    char *buffer;
    va_list args;

    buffer = new (std::nothrow) char[size + 1];

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    Decrement();

    object_pointer = new (std::nothrow) StringObject(buffer);
    delete[] buffer;

    return *this;
}

int SmartString::Strcmp(const char *cstring, bool case_sensitive) const {
    int result;

    if (case_sensitive) {
        result = strcmp(GetCStr(), cstring);
    } else {
        result = stricmp(GetCStr(), cstring);
    }

    return result;
}

SmartString &SmartString::operator=(const SmartString &rhs) {
    rhs.Increment();
    Decrement();
    object_pointer = rhs.object_pointer;

    return *this;
}

SmartString &SmartString::operator=(const char *rhs) {
    Decrement();
    object_pointer = new (std::nothrow) StringObject(rhs);

    return *this;
}

bool SmartString::IsEqual(const char *cstring) { return !Strcmp(cstring); }
