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

#include "hash.hpp"

#define HASH_HASH_SIZE 512

Hash Hash_UnitInfo(HASH_HASH_SIZE);

void SmartList_UnitInfo_FileLoad(SmartList<UnitInfo>& list, SmartFileReader& file) {
    list.Clear();

    for (unsigned short count = file.ReadObjectCount(); count; --count) {
        UnitInfo* unit = dynamic_cast<UnitInfo*>(file.ReadObject());

        list.PushBack(*unit);

        if (unit->flags & HASH_4) {
            //            unit->field_34 = dword_1770A0;
        } else if (unit->flags & HASH_3) {
            //            unit->field_34 = dword_1770A4;
        } else if (unit->flags & HASH_2) {
            //            unit->field_34 = dword_1770A8;
        } else if (unit->flags & HASH_1) {
            //            unit->field_34 = dword_1770AC;
        } else {
            //            unit->field_34 = dword_1770B0;
        }

        unit->sound = SFX_TYPE_INVALID;
    }
}

void SmartList_UnitInfo_FileSave(SmartList<UnitInfo>& list, SmartFileWriter& file) {
    unsigned short count = list.GetCount();

    file.Write(count);
    for (SmartList<UnitInfo>::Iterator unit = list.Begin(); unit != nullptr; ++unit) {
        file.WriteObject(&*unit);
    }
}

void SmartList_UnitInfo_TextLoad(SmartList<UnitInfo>& list, TextStructure& object) {
    list.Clear();

    for (UnitInfo* unit; (unit = dynamic_cast<UnitInfo*>(&*object.ReadPointer("unit"))), unit != nullptr;) {
        list.PushBack(*unit);

        if (unit->flags & HASH_4) {
            //            unit->field_34 = dword_1770A0;
        } else if (unit->flags & HASH_3) {
            //            unit->field_34 = dword_1770A4;
        } else if (unit->flags & HASH_2) {
            //            unit->field_34 = dword_1770A8;
        } else if (unit->flags & HASH_1) {
            //            unit->field_34 = dword_1770AC;
        } else {
            //            unit->field_34 = dword_1770B0;
        }

        unit->sound = SFX_TYPE_INVALID;
    }
}

void SmartList_UnitInfo_TextSave(SmartList<UnitInfo>& list, SmartTextfileWriter& file) {
    for (SmartList<UnitInfo>::Iterator unit = list.Begin(); unit != nullptr; ++unit) {
        file.WritePointer("unit", &*unit);
    }
}

Hash::Hash(unsigned short hash_size) : hash_size(hash_size), list(new (std::nothrow) SmartList<UnitInfo>[hash_size]) {}
Hash::~Hash() { delete[] list; }

void Hash::PushBack(UnitInfo* unit) {
    SDL_assert(unit != nullptr);

    list[unit->GetId() % hash_size].PushBack(*unit);
}
void Hash::Remove(UnitInfo* unit) {
    SDL_assert(unit != nullptr);

    list[unit->GetId() % hash_size].Remove(*unit);
}

void Hash::Clear() {
    for (int index = 0; index < hash_size; ++index) {
        for (SmartList<UnitInfo>::Iterator unit = list[index].Begin(); unit != nullptr; ++unit) {
            (*unit).ClearUnitList();
            (*unit).SetParent(nullptr);
        }

        list[index].Clear();
    }
}

void Hash::FileLoad(SmartFileReader& file) {
    Clear();
    delete[] list;

    file.Read(hash_size);
    list = new (std::nothrow) SmartList<UnitInfo>[hash_size];

    for (int index = 0; index < hash_size; ++index) {
        SmartList_UnitInfo_FileLoad(list[index], file);
    }
}

void Hash::FileSave(SmartFileWriter& file) {
    file.Write(hash_size);

    for (int index = 0; index < hash_size; ++index) {
        SmartList_UnitInfo_FileSave(list[index], file);
    }
}

void Hash::TextLoad(TextStructure& object) {
    Clear();
    delete[] list;

    hash_size = object.ReadInt("hash_size");
    list = new (std::nothrow) SmartList<UnitInfo>[hash_size];

    SmartPointer<TextStructure> list_reader = nullptr;

    for (;;) {
        list_reader = object.ReadStructure("list");

        if (list_reader == nullptr) {
            break;
        }

        SmartList_UnitInfo_TextLoad(list[list_reader->ReadInt("index")], *list_reader);
    }
}

void Hash::TextSave(SmartTextfileWriter& file) {
    file.WriteInt("hash_size", hash_size);

    for (int index = 0; index < hash_size; ++index) {
        if (list[index].GetCount()) {
            file.WriteIdentifier("list");
            file.WriteInt("index", index);
            SmartList_UnitInfo_TextSave(list[index], file);
            file.WriteDelimiter();
        }
        SmartList_UnitInfo_FileSave(list[index], file);
    }
}

UnitInfo* Hash::operator[](const unsigned short& key) {
    for (SmartList<UnitInfo>::Iterator unit = list[key % hash_size].Begin(); unit != nullptr; ++unit) {
        if (key == (*unit).GetId()) {
            return &(*unit);
        }
    }

    return nullptr;
}
