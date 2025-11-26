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

#ifndef SMARTSTRING_HPP
#define SMARTSTRING_HPP

#include <SDL3/SDL.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <new>

class SmartString {
    class StringObject {
        friend class SmartString;

        char* buffer{nullptr};
        uint32_t reference_count{0};
        size_t size{0};
        size_t length{0};

    public:
        StringObject(const size_t size) noexcept : reference_count(1), size(size) {
            buffer = new (std::nothrow) char[size + 1];
            buffer[0] = '\0';
        }

        StringObject(const char* const cstring) noexcept
            : reference_count(1), size(std::strlen(cstring)), length(size) {
            buffer = new (std::nothrow) char[size + 1];
            (void)std::strcpy(buffer, cstring);
        }

        StringObject(const StringObject& other) noexcept
            : reference_count(1), size(other.GetSize()), length(other.GetLength()) {
            buffer = new (std::nothrow) char[size + 1];
            (void)std::strcpy(buffer, other.GetCStr());
        }

        ~StringObject() noexcept {
            SDL_assert(reference_count == 0);
            delete[] buffer;
        }

        inline void Resize(const size_t new_size, const bool keep) noexcept {
            if (new_size != this->size) {
                this->size = new_size;
                if (keep) {
                    char* new_buffer = new (std::nothrow) char[new_size + 1];

                    if (new_size >= this->length) {
                        (void)std::strcpy(new_buffer, this->buffer);

                    } else {
                        this->length = SDL_utf8strlcpy(new_buffer, this->buffer, new_size);
                    }

                    delete[] this->buffer;
                    this->buffer = new_buffer;

                } else {
                    delete[] this->buffer;
                    this->buffer = new (std::nothrow) char[new_size + 1];
                    this->buffer[0] = '\0';
                    this->length = 0;
                }
            }
        }

        [[nodiscard]] inline size_t GetLength() const noexcept { return length; }

        [[nodiscard]] inline size_t GetSize() const noexcept { return size; }

        [[nodiscard]] inline char* GetCStr() const noexcept { return buffer; }

        inline void SetLength(size_t new_length) noexcept { this->length = new_length; }
    };

    StringObject* object_pointer{nullptr};

    inline void Increment() const noexcept {
        if (object_pointer) {
            ++object_pointer->reference_count;
        }
    }

    inline void Decrement() noexcept {
        if (object_pointer && --object_pointer->reference_count == 0) {
            delete object_pointer;
            object_pointer = nullptr;
        }
    }

    inline void Resize(const size_t size, bool keep = true) noexcept {
        Copy();
        object_pointer->Resize(size, keep);
    }

    inline void Copy() noexcept {
        if (!IsLastReference()) {
            Decrement();
            object_pointer = new (std::nothrow) StringObject(*object_pointer);
        }
    }

    [[nodiscard]] inline bool IsLastReference() noexcept { return 1 == object_pointer->reference_count; }

    [[nodiscard]] static inline size_t CalcOptimalCapacity(const size_t needed_capacity) noexcept {
        return needed_capacity / 64u * 64u + 64u;
    }

public:
    SmartString() noexcept {
        static SmartString* const empty_string = new (std::nothrow) SmartString(static_cast<size_t>(0));
        object_pointer = empty_string->object_pointer;
        Increment();
    }

    explicit SmartString(const size_t size) noexcept { object_pointer = new (std::nothrow) StringObject(size); }

    explicit SmartString(const char* const cstring) noexcept {
        object_pointer = new (std::nothrow) StringObject(cstring);
    }

    SmartString(const SmartString& other) noexcept : object_pointer(other.object_pointer) { Increment(); }

    ~SmartString() noexcept { Decrement(); }

    [[nodiscard]] inline char* GetCStr() const noexcept { return object_pointer->GetCStr(); }

    [[nodiscard]] inline size_t GetLength() const noexcept { return object_pointer->GetLength(); }

    SmartString Substr(const size_t position, const size_t length) noexcept {
        SmartString result;

        if (object_pointer->GetLength() > position) {
            size_t size{object_pointer->GetLength() - position};

            if (size > length) {
                size = length;
            }

            if (size > 0) {
                SmartString string(size + 1);
                string.object_pointer->SetLength(SDL_utf8strlcpy(string.GetCStr(), &GetCStr()[position], size + 1));

                result = string;

            } else {
                result = SmartString();
            }

        } else {
            result = SmartString();
        }

        return result;
    }

    [[nodiscard]] inline char& operator[](const size_t position) const noexcept {
        SDL_assert(position < object_pointer->GetSize());
        return object_pointer->GetCStr()[position];
    }

    inline SmartString& operator+=(const SmartString& other) noexcept {
        Copy();

        const size_t length = object_pointer->GetLength() + other.object_pointer->GetLength();

        if (object_pointer->GetSize() < length) {
            Resize(CalcOptimalCapacity(length));
        }

        (void)strcpy(&GetCStr()[object_pointer->GetLength()], other.GetCStr());

        object_pointer->SetLength(length);

        return *this;
    }

    inline SmartString& operator+=(const char* const cstring) noexcept {
        Copy();

        const size_t length = object_pointer->GetLength() + strlen(cstring);

        if (object_pointer->GetSize() < length) {
            Resize(CalcOptimalCapacity(length));
        }

        (void)strcpy(&GetCStr()[object_pointer->GetLength()], cstring);

        object_pointer->SetLength(length);

        return *this;
    }

    inline SmartString& operator+=(const char character) noexcept {
        Copy();

        const size_t length = object_pointer->GetLength() + sizeof(char);

        if (object_pointer->GetSize() < length) {
            Resize(CalcOptimalCapacity(length));
        }

        GetCStr()[length - sizeof(char)] = character;
        GetCStr()[length] = '\0';

        object_pointer->SetLength(length);

        return *this;
    }

    [[nodiscard]] inline char* StrStr(const char* const cstring) const noexcept { return strstr(GetCStr(), cstring); }

    [[nodiscard]] inline char* StrStr(const SmartString& other) const noexcept {
        return strstr(GetCStr(), other.GetCStr());
    }

    inline SmartString& Clear() noexcept {
        Copy();

        GetCStr()[0] = '\0';
        object_pointer->SetLength(0);

        return *this;
    }

    inline SmartString& Toupper() noexcept {
        Copy();

        for (char* cstring = GetCStr(); *cstring; ++cstring) {
            *cstring = static_cast<char>(toupper(*cstring));
        }

        return *this;
    }

    inline SmartString& Tolower() noexcept {
        Copy();
        for (char* cstring = GetCStr(); *cstring; ++cstring) {
            *cstring = static_cast<char>(tolower(*cstring));
        }
        return *this;
    }

    inline SmartString& Sprintf(const size_t size, const char* const format, ...) noexcept {
        va_list args;
        va_start(args, format);
        VSprintf(size, format, args);
        va_end(args);

        return *this;
    }

    inline SmartString& VSprintf(const size_t size, const char* const format, va_list args) noexcept {
        char* const buffer = new (std::nothrow) char[size + 1];

        buffer[size] = '\0';

        const auto result = vsnprintf(buffer, size, format, args);

        if (result > 0) {
            Decrement();

            object_pointer = new (std::nothrow) StringObject(buffer);

        } else {
            Clear();
        }

        delete[] buffer;

        return *this;
    }

    [[nodiscard]] inline int32_t Strcmp(const char* const cstring, const bool case_sensitive = true) const noexcept {
        int32_t result;

        if (case_sensitive) {
            result = SDL_strcmp(GetCStr(), cstring);

        } else {
            result = SDL_strcasecmp(GetCStr(), cstring);
        }

        return result;
    }

    inline SmartString& operator=(const SmartString& rhs) noexcept {
        rhs.Increment();
        Decrement();

        object_pointer = rhs.object_pointer;

        return *this;
    }

    inline SmartString& operator=(const char* const rhs) noexcept {
        Decrement();

        object_pointer = new (std::nothrow) StringObject(rhs);

        return *this;
    }

    [[nodiscard]] inline bool IsEqual(const char* const cstring) const noexcept { return !Strcmp(cstring); }
};

#endif /* SMARTSTRING_HPP */
