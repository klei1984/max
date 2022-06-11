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

#include "textfile.hpp"

#include "gnw.h"
#include "message_manager.hpp"
#include "registerarray.hpp"

#ifdef __unix__
#define stricmp strcasecmp
#endif

static SmartString BuildErrorMessage(const char* error_message, const char* error_log, unsigned int line,
                                     unsigned short column);

ErrorLogEntry::ErrorLogEntry() : error_message(), line(0), column(0) {}

ErrorLogEntry::ErrorLogEntry(const ErrorLogEntry& other)
    : error_message(other.error_message), line(other.line), column(other.column) {}

ErrorLogEntry::~ErrorLogEntry() {}

void TextItem::FatalError(const char* error_message) {
    SDL_TriggerBreakpoint();
    win_exit();
    reset_mode();

    SmartString string = BuildErrorMessage(error_message, error.error_message.GetCStr(), error.line, error.column);
    SDL_Log(string.GetCStr());
    exit(10);
}

SmartString BuildErrorMessage(const char* error_message, const char* error_log, unsigned int line,
                              unsigned short column) {
    SmartString log;

    log.Sprintf(80, "Error in line %li, column %i:\n", line, column);
    log += error_message;
    if (log[log.GetLength() - 1] != '\n') {
        log += '\n';
    }

    for (--column; column != 0; --column) {
        log += '*';
        ++error_log;
    }

    log += error_log;

    return SmartString(log);
}

void TextItem::DrawErrorMessage(const char* error_message) {
    SDL_TriggerBreakpoint();

    SmartString string = BuildErrorMessage(error_message, error.error_message.GetCStr(), error.line, error.column);
    MessageManager_DrawMessage(string.GetCStr(), 2, 1);
}

TextItem::TextItem(SmartString& text, ErrorLogEntry& error) : text(text), error(error) {}

TextItem::~TextItem() {}

const char* TextItem::GetText() const { return text.GetCStr(); }

int TextItem::GetInteger() {
    FatalError("Expecting a number");

    return 0;
}

SmartString TextItem::GetString() {
    FatalError("Expecting a string");

    return SmartString();
}

SmartString TextItem::GetIdentifier() {
    FatalError("Expecting a word");

    return SmartString();
}

TextFileObject* TextItem::GetPointer() {
    FatalError("Expecting a pointer");

    return nullptr;
}

TextStructure* TextItem::GetStruct() {
    FatalError("Expecting a {");

    return nullptr;
}

void TextItem::AddMembers() {}

TextInteger::TextInteger(SmartString& text, ErrorLogEntry& error, int integer)
    : TextItem(text, error), integer(integer) {}

TextInteger::~TextInteger() {}

int TextInteger::GetInteger() { return integer; }

TextString::TextString(SmartString& text, ErrorLogEntry& error, SmartString& string)
    : TextItem(text, error), string(string) {}

TextString::~TextString() {}

SmartString TextString::GetString() { return SmartString(this->string); }

TextIdentifier::TextIdentifier(SmartString& text, ErrorLogEntry& error, SmartString& string)
    : TextItem(text, error), string(string) {}

TextIdentifier::~TextIdentifier() {}

SmartString TextIdentifier::GetIdentifier() { return SmartString(this->string); }

TextPointer::TextPointer(SmartString& text, ErrorLogEntry& error, SmartPointer<TextFileObject>& object)
    : TextItem(text, error), object(object) {}

TextPointer::~TextPointer() {}

TextFileObject* TextPointer::GetPointer() { return &*object; }

TextStructure::TextStructure(SmartString& text, ErrorLogEntry& error, char delimiter, SmartTextfileReader* file)
    : TextItem(text, error), delimiter(delimiter), file(file), field_38(false) {}

TextStructure::~TextStructure() {
    while (ReadMember()) {
        ;
    }
}

TextStructure* TextStructure::GetStruct() { return this; }

void TextStructure::AddMembers() {
    while (ReadMember()) {
        members.PushBack(*object);
    }
}

void TextStructure::CheckDelimiter() {
    if (strlen(text.GetCStr())) {
        if (delimiter == file->GetCurrentCharacter()) {
            file->ReadNextCharacter();
        } else {
            file->DrawErrorMessage("Expecting an '%c'", delimiter);
        }
    }

    field_38 = true;
}

SmartPointer<TextItem> TextStructure::Find(const char* field) {
    for (SmartList<TextItem>::Iterator it = members.Begin(); it != members.End(); ++it) {
        if (!stricmp((*it).GetText(), field)) {
            members.Remove(it);

            return SmartPointer<TextItem>(*it);
        }
    }

    while (ReadMember()) {
        if (!stricmp((*object).GetText(), field)) {
            return SmartPointer<TextItem>(object);
        } else {
            members.PushBack(*object);
        }
    }

    return SmartPointer<TextItem>(nullptr);
}

bool TextStructure::GetField(const char* field, bool* value) {
    bool result;
    SmartPointer<TextItem> item = Find(field);

    if (item != nullptr) {
        if (!stricmp((*item).GetIdentifier().GetCStr(), "TRUE")) {
            *value = true;
        } else {
            *value = false;
        }

        result = true;
    } else {
        result = false;
    }

    return result;
}

bool TextStructure::GetField(const char* field, short* value) {
    bool result;
    SmartPointer<TextItem> item = Find(field);

    if (item != nullptr) {
        *value = (*item).GetInteger();
        result = true;
    } else {
        result = false;
    }

    return result;
}

bool TextStructure::GetField(const char* field, int* value) {
    bool result;
    SmartPointer<TextItem> item = Find(field);

    if (item != nullptr) {
        *value = (*item).GetInteger();
        result = true;
    } else {
        result = false;
    }

    return result;
}

bool TextStructure::GetField(const char* field, char* str, unsigned short num) {
    bool result;
    SmartPointer<TextItem> item = Find(field);

    if (item != nullptr) {
        strncpy(str, (*item).GetString().GetCStr(), num);
        result = true;
    } else {
        result = false;
    }

    return result;
}

bool TextStructure::GetField(const char* field, SortedEnum& sorted_enum, unsigned short* value) {
    bool result;
    SmartPointer<TextItem> item = Find(field);

    if (item != nullptr) {
        result = sorted_enum.GetValue((*item).GetIdentifier().GetCStr(), *value);
    } else {
        result = false;
    }

    return result;
}

bool TextStructure::ReadBool(const char* field) {
    bool result;

    if (!GetField(field, &result)) {
        result = false;
    }

    return false;
}

int TextStructure::ReadInt(const char* field) {
    int result;

    if (!GetField(field, &result)) {
        result = 0;
    }

    return result;
}

SmartString TextStructure::ReadString(const char* field) {
    SmartString result;
    SmartPointer<TextItem> item = Find(field);

    if (item != nullptr) {
        result = (*item).GetString();
    }

    return SmartString(result);
}

SmartPointer<TextFileObject> TextStructure::ReadPointer(const char* field) {
    SmartPointer<TextFileObject> result;
    SmartPointer<TextItem> item = Find(field);

    if (item != nullptr) {
        result = (*item).GetPointer();
    }

    return SmartPointer<TextFileObject>(result);
}

SmartPointer<TextStructure> TextStructure::ReadStructure(const char* field) {
    SmartPointer<TextStructure> result;
    SmartPointer<TextItem> item = Find(field);

    if (item != nullptr) {
        result = (*item).GetStruct();
    }

    return SmartPointer<TextStructure>(result);
}

unsigned short TextStructure::ReadEnum(const char* field, SortedEnum& sorted_enum) {
    unsigned short result;

    if (!GetField(field, sorted_enum, &result)) {
        result = 0;
    }

    return result;
}

bool TextStructure::ReadMember() {
    if (!field_38) {
        if (object != nullptr) {
            (*object).AddMembers();
        }

        object = file->ReadNext();

        if (object == nullptr) {
            CheckDelimiter();
        }
    }

    return field_38 == false;
}

SmartTextfileWriter::SmartTextfileWriter() : alignment_width(0) {}
SmartTextfileWriter::~SmartTextfileWriter() {}

bool SmartTextfileWriter::Open(const char* const path) {
    bool result;

    Close();

    file = fopen(path, "wt");
    alignment_width = 0;

    if (file) {
        result = true;
    } else {
        result = false;
    }

    return result;
}

void SmartTextfileWriter::WriteAlignment() {
    for (int count = alignment_width; --count >= 0;) {
        fputc(' ', file);
    }
}

void SmartTextfileWriter::WriteIdentifier(const char* identifier) {
    WriteAlignment();
    fputs(identifier, file);
    fputc('\n', file);
    WriteAlignment();
    fputs("{\n", file);
    alignment_width += 2;
}

void SmartTextfileWriter::WriteDelimiter() {
    alignment_width -= 2;
    WriteAlignment();
    fputs("}\n", file);
}

void SmartTextfileWriter::WriteString(const char* field, const char* value) {
    WriteAlignment();
    fputs(field, file);
    fputs(" = \"", file);
    if (value) {
        fputs(value, file);
    }
    fputs("\"\n", file);
}

void SmartTextfileWriter::WriteInt(const char* field, int value) {
    SmartString string;

    WriteAlignment();
    fputs(field, file);
    fputs(string.Sprintf(15, " = %i\n", value).GetCStr(), file);
}

void SmartTextfileWriter::WritePointer(const char* field, TextFileObject* object) {
    WriteAlignment();
    fputs(field, file);
    fputc('\n', file);
    WriteAlignment();
    fputs("<\n", file);
    alignment_width += 2;
    if (object) {
        unsigned short index = object->GetIndex();
        if (index) {
            WriteInt("index", index);
        } else {
            WriteInt("index", objects.GetCount() + 1);
            WriteString("Type", GetClassName(object->GetTypeIndex() - 1));
            WriteTextObject("Object", object);
        }
    } else {
        WriteInt("index", 0);
    }
    alignment_width -= 2;
    WriteAlignment();
    fputs(">\n", file);
}

const char* SmartTextfileWriter::GetClassName(unsigned short type_index) {
    SDL_assert(type_index < registered_classes->GetCount());

    return (*registered_classes)[type_index].GetClassName();
}

void SmartTextfileWriter::WriteTextObject(const char* field, TextFileObject* object) {
    WriteIdentifier(field);
    AddObject(object);
    object->TextSave(*this);
    WriteDelimiter();
}

void SmartTextfileWriter::WriteEnum(const char* field, SortedEnum& sorted_enum, int value) {
    const char* name;

    WriteAlignment();
    fputs(field, file);
    fputs(" = ", file);
    name = sorted_enum[value];
    if (name && name[0] != '\0') {
        fputs(name, file);
    } else {
        fputs("UNKNOWN", file);
    }
    fputc('\n', file);
}

void SmartTextfileWriter::WriteBool(const char* field, bool value) {
    WriteAlignment();
    fputs(field, file);
    fputs(" = ", file);
    if (value) {
        fputs("true\n", file);
    } else {
        fputs("false\n", file);
    }
}

SmartTextfileReader::SmartTextfileReader() : current_character(0), file_size(0) {}

SmartTextfileReader::~SmartTextfileReader() {}

bool SmartTextfileReader::Open(const char* const path) {
    bool result;
    SmartString string = "";

    Close();

    file = fopen(path, "rt");

    if (file) {
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        result = true;
        fseek(file, 0, SEEK_SET);

        log.line = 0;
        log.column = 0;
        current_character = ' ';

        object = new (std::nothrow) TextStructure(string, log, '}', this);
    } else {
        result = false;
    }

    return result;
}

void SmartTextfileReader::Close() {
    SmartFileReader::Close();
    object = nullptr;
    current_character = -1;
}

void SmartTextfileReader::ReadNextCharacter() {
    if (current_character != -1) {
        if (log.error_message.GetLength() <= log.column) {
            int character;

            ++log.line;
            log.column = 0;
            log.error_message = "";

            for (character = fgetc(file); character != '\n' && character != -1; character = fgetc(file)) {
                log.error_message += character;
            }

            if (character == -1) {
                if (!log.error_message.GetLength()) {
                    current_character = -1;
                    return;
                }
            } else {
                log.error_message += character;
            }
        }

        current_character = log.error_message[log.column++];
    }
}

bool SmartTextfileReader::Seek() {
    bool stop;

    do {
        stop = false;
        while (current_character == ' ' || current_character == '\n' || current_character == '\t') {
            ReadNextCharacter();
        }

        if (current_character == '/' && log.error_message.GetLength() > log.column &&
            log.error_message[log.column] == '/') {
            while (current_character != '\n' && current_character != -1) {
                ReadNextCharacter();
            }

            stop = true;
        }
    } while (stop);

    return current_character != -1;
}

char SmartTextfileReader::GetCurrentCharacter() { return current_character; }

SmartPointer<TextItem> SmartTextfileReader::ReadNext() {
    SmartString string;

    if (!Seek() || current_character == '}' || current_character == '>' || !ReadIdentifier(string) || !Seek()) {
        return SmartPointer<TextItem>();
    }

    switch (current_character) {
        case '=':
            ReadNextCharacter();
            return ReadField(string);
            break;
        case '<':
            ReadNextCharacter();
            return ReadPointer(string);
            break;
        case '{':
            ReadNextCharacter();
            return SmartPointer<TextItem>(new (std::nothrow) TextStructure(string, log, '}', this));
        default:
            FatalError("Expecting {, =, or <");
            break;
    }

    return SmartPointer<TextItem>();
}

void SmartTextfileReader::FatalError(const char* format, ...) {
    SmartString string;
    va_list args;

    SDL_TriggerBreakpoint();
    win_exit();
    reset_mode();

    va_start(args, format);
    string.Sprintf(300, format, args);
    va_end(args);

    SmartString message = BuildErrorMessage(string.GetCStr(), log.error_message.GetCStr(), log.line, log.column);

    SDL_Log(message.GetCStr());
    exit(10);
}

void SmartTextfileReader::DrawErrorMessage(const char* format, ...) {
    SmartString string;
    va_list args;

    SDL_TriggerBreakpoint();

    va_start(args, format);
    string.Sprintf(300, format, args);
    va_end(args);

    SmartString message = BuildErrorMessage(string.GetCStr(), log.error_message.GetCStr(), log.line, log.column);
    MessageManager_DrawMessage(message.GetCStr(), 2, 1);
}

bool SmartTextfileReader::ReadIdentifier(SmartString& string) {
    bool result;

    if (Seek()) {
        if (!isalpha(current_character)) {
            FatalError("Expected a letter");
        }

        while (isalnum(current_character) || current_character == '_') {
            string += current_character;
            ReadNextCharacter();
        }

        result = true;
    } else {
        result = false;
    }

    return result;
}

SmartPointer<TextItem> SmartTextfileReader::ReadField(SmartString& field) {
    SmartString string;
    ErrorLogEntry error_log = log;

    if (Seek()) {
        if (current_character == '"') {
            ReadNextCharacter();

            while (current_character != '"' && current_character != -1 && current_character != '\n') {
                string += current_character;
                ReadNextCharacter();
            }

            if (current_character == '"') {
                ReadNextCharacter();
            } else {
                DrawErrorMessage("Expecting an end quote ('\"')");
            }

            return SmartPointer<TextItem>(new (std::nothrow) TextString(field, error_log, string));

        } else if (isalpha(current_character)) {
            if (!ReadIdentifier(string)) {
                DrawErrorMessage("Expecting a word");
            }

            return SmartPointer<TextItem>(new (std::nothrow) TextIdentifier(field, error_log, string));

        } else {
            if (current_character == '-') {
                string += current_character;
                ReadNextCharacter();
            }

            if (current_character < '0' || current_character > '9') {
                FatalError("Expecting a number or string");
            }

            while (current_character >= '0' && current_character <= '9') {
                string += current_character;
                ReadNextCharacter();
            }

            return SmartPointer<TextItem>(new (std::nothrow)
                                              TextInteger(field, error_log, strtol(string.GetCStr(), nullptr, 10)));
        }
    }

    return SmartPointer<TextItem>();
}

SmartPointer<TextItem> SmartTextfileReader::ReadPointer(SmartString& field) {
    SmartPointer<TextStructure> item(new (std::nothrow) TextStructure(field, log, '>', this));
    ErrorLogEntry error_log;
    SmartPointer<TextFileObject> file_object;
    int object_count;

    object_count = item->ReadInt("Index");

    if (object_count) {
        int read_count = read_objects.GetCount();

        if (read_count < object_count) {
            if (object_count != (read_count + 1)) {
                FatalError("Index refers to unknown object");
            }

            CharSortKey key(item->ReadString("Type").GetCStr());
            MAXRegisterClass& class_type = (*registered_classes)[key];
            if (&class_type) {
                SmartPointer<TextStructure> class_object = item->ReadStructure("Object");
                if (class_object != nullptr) {
                    file_object = class_type.Allocate();
                    read_objects.Insert(*file_object);
                    file_object->TextLoad(*class_object);
                } else {
                    FatalError("No Object structure");
                }
            } else {
                FatalError("Unknown type");
            }

        } else {
            file_object = read_objects[object_count - 1];
        }
    } else {
        file_object = nullptr;
    }

    return SmartPointer<TextItem>(new (std::nothrow) TextPointer(field, error_log, file_object));
}

SmartPointer<TextStructure> SmartTextfileReader::GetObject() { return SmartPointer<TextStructure>(object); }
