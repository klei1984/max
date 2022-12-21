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

class StringObject {
    friend class SmartString;

    char* buffer;
    unsigned short size;
    unsigned short length;
    unsigned int reference_count;

public:
    StringObject(unsigned short size);
    StringObject(const char* cstring);
    StringObject(const StringObject& other);
    ~StringObject();

    void Resize(unsigned short size, bool keep);
    unsigned short GetLength() const;
    unsigned short GetSize() const;
    char* GetCStr() const;
    void SetLength(unsigned short length);
};

class SmartString {
    StringObject* object_pointer;

    void Increment() const;
    void Decrement();
    void Resize(unsigned short size, bool keep = true);
    void Copy();
    bool IsLastReference();
    static unsigned short CalcOptimalCapacity(unsigned short needed_capacity);

public:
    SmartString();
    SmartString(unsigned short size);
    SmartString(const char* cstring);
    SmartString(const SmartString& other);
    ~SmartString();

    char* GetCStr() const;
    unsigned short GetLength() const;
    SmartString Substr(unsigned short position, unsigned short length);
    char& operator[](unsigned short position);
    SmartString& operator+=(SmartString const& other);
    SmartString& operator+=(const char* cstring);
    SmartString& operator+=(const char character);
    char* StrStr(const char* cstring) const;
    char* StrStr(const SmartString& other) const;
    SmartString& Clear();
    SmartString& Toupper();
    SmartString& Tolower();
    SmartString& Sprintf(unsigned short size, const char* format, ...);
    int Strcmp(const char* cstring, bool case_sensitive = true) const;
    SmartString& operator=(SmartString const& rhs);
    SmartString& operator=(char const* rhs);
    bool IsEqual(const char* cstring);
};

#endif /* SMARTSTRING_HPP */
