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

#include <cstdarg>
#include <cstdint>

class StringObject {
    friend class SmartString;

    char* buffer;
    uint16_t size;
    uint16_t length;
    uint32_t reference_count;

public:
    StringObject(uint16_t size);
    StringObject(const char* cstring);
    StringObject(const StringObject& other);
    ~StringObject();

    void Resize(uint16_t size, bool keep);
    uint16_t GetLength() const;
    uint16_t GetSize() const;
    char* GetCStr() const;
    void SetLength(uint16_t length);
};

class SmartString {
    StringObject* object_pointer;

    void Increment() const;
    void Decrement();
    void Resize(uint16_t size, bool keep = true);
    void Copy();
    bool IsLastReference();
    static uint16_t CalcOptimalCapacity(uint16_t needed_capacity);

public:
    SmartString();
    SmartString(uint16_t size);
    SmartString(const char* cstring);
    SmartString(const SmartString& other);
    ~SmartString();

    char* GetCStr() const;
    uint16_t GetLength() const;
    SmartString Substr(uint16_t position, uint16_t length);
    char& operator[](uint16_t position);
    SmartString& operator+=(SmartString const& other);
    SmartString& operator+=(const char* cstring);
    SmartString& operator+=(const char character);
    char* StrStr(const char* cstring) const;
    char* StrStr(const SmartString& other) const;
    SmartString& Clear();
    SmartString& Toupper();
    SmartString& Tolower();
    SmartString& Sprintf(uint16_t size, const char* format, ...);
    SmartString& VSprintf(uint16_t size, const char* format, va_list args);
    int32_t Strcmp(const char* cstring, bool case_sensitive = true) const;
    SmartString& operator=(SmartString const& rhs);
    SmartString& operator=(char const* rhs);
    bool IsEqual(const char* cstring);
};

#endif /* SMARTSTRING_HPP */
