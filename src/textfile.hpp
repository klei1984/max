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

#ifndef TEXTFILE_HPP
#define TEXTFILE_HPP

#include "enums.hpp"
#include "smartfile.hpp"
#include "smartlist.hpp"
#include "smartstring.hpp"
#include "sortedenum.hpp"
#include "textfileobject.hpp"

class ErrorLogEntry {
public:
    ErrorLogEntry();
    ErrorLogEntry(const ErrorLogEntry& other);
    ~ErrorLogEntry();

    SmartString error_message;
    unsigned int line;
    unsigned short column;
};

class TextStructure;

class TextItem : public SmartObject {
    void FatalError(const char* error_message);
    void DrawErrorMessage(const char* error_message);

protected:
    SmartString text;
    ErrorLogEntry error;

public:
    TextItem(SmartString& text, ErrorLogEntry& error);
    virtual ~TextItem();

    const char* GetText() const;

    virtual int GetInteger();
    virtual SmartString GetString();
    virtual SmartString GetIdentifier();
    virtual TextFileObject* GetPointer();
    virtual TextStructure* GetStruct();
    virtual void AddMembers();
};

class TextInteger : public TextItem {
    int integer;

public:
    TextInteger(SmartString& text, ErrorLogEntry& error, int integer);
    ~TextInteger();

    int GetInteger();
};

class TextString : public TextItem {
    SmartString string;

public:
    TextString(SmartString& text, ErrorLogEntry& error, SmartString& string);
    ~TextString();

    SmartString GetString();
};

class TextIdentifier : public TextItem {
    SmartString string;

public:
    TextIdentifier(SmartString& text, ErrorLogEntry& error, SmartString& string);
    ~TextIdentifier();

    SmartString GetIdentifier();
};

class TextPointer : public TextItem {
    SmartPointer<TextFileObject> object;

public:
    TextPointer(SmartString& text, ErrorLogEntry& error, SmartPointer<TextFileObject>& object);
    ~TextPointer();

    TextFileObject* GetPointer();
};

class SmartTextfileReader;

class TextStructure : public TextItem {
    SmartTextfileReader* file;
    SmartPointer<TextItem> object;
    SmartList<TextItem> members;
    bool field_38;
    char delimiter;

    void CheckDelimiter();
    bool ReadMember();

public:
    TextStructure(SmartString& text, ErrorLogEntry& error, char delimiter, SmartTextfileReader* file);
    ~TextStructure();

    TextStructure* GetStruct();
    void AddMembers();

    SmartPointer<TextItem> Find(const char* field);

    bool GetField(const char* field, bool* value);
    bool GetField(const char* field, short* value);
    bool GetField(const char* field, int* value);
    bool GetField(const char* field, char* str, unsigned short num);
    bool GetField(const char* field, SortedEnum& sorted_enum, unsigned short* value);

    bool ReadBool(const char* field);
    int ReadInt(const char* field);
    SmartString ReadString(const char* field);
    SmartPointer<TextFileObject> ReadPointer(const char* field);
    SmartPointer<TextStructure> ReadStructure(const char* field);
    unsigned short ReadEnum(const char* field, SortedEnum& sorted_enum);
};

class SmartTextfileReader : public SmartFileReader {
    SmartPointer<TextStructure> object;
    ErrorLogEntry log;
    int current_character;
    int file_size;

    bool Seek();

public:
    SmartTextfileReader();
    ~SmartTextfileReader();

    bool Open(const char* const path);
    void Close();
    char GetCurrentCharacter();
    void ReadNextCharacter();
    SmartPointer<TextItem> ReadNext();

    void FatalError(const char* format, ...);
    void DrawErrorMessage(const char* format, ...);

    bool ReadIdentifier(SmartString& string);
    SmartPointer<TextItem> ReadField(SmartString& field);
    SmartPointer<TextItem> ReadPointer(SmartString& field);
    SmartPointer<TextStructure> GetObject();
};

class SmartTextfileWriter : public SmartFileWriter {
    unsigned short alignment_width;
    static const char* GetClassName(unsigned short type_index);

public:
    SmartTextfileWriter();
    ~SmartTextfileWriter();

    bool Open(const char* const path);
    void WriteAlignment();
    void WriteIdentifier(const char* identifier);
    void WriteDelimiter();
    void WriteString(const char* field, const char* value);
    void WriteInt(const char* field, int value);
    void WritePointer(const char* field, TextFileObject* object);
    void WriteTextObject(const char* field, TextFileObject* object);
    void WriteEnum(const char* field, SortedEnum& sorted_enum, int value);
    void WriteBool(const char* field, bool value);
};

#endif /* TEXTFILE_HPP */
