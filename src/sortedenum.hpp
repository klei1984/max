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

#ifndef SORTEDENUM_HPP
#define SORTEDENUM_HPP

#include "sortedarray.hpp"

class EnumObject : public SmartObject {
    CharSortKey name;
    ShortSortKey value;

public:
    EnumObject(const char* name, unsigned short value) : name(name), value(value) {}
    virtual ~EnumObject() {}

    CharSortKey& GetNameKey() { return name; }
    ShortSortKey& GetValueKey() { return value; }
    const char* GetName() const { return name.GetKey(); }
    unsigned short GetValue() const { return value.GetKey(); }
};

class EnumsSortedByName : public SortedArray<EnumObject> {
public:
    EnumsSortedByName(unsigned short growth_factor) : SortedArray<EnumObject>(growth_factor) {}

    SortKey& GetSortKey(EnumObject& object) const { return object.GetNameKey(); }

    EnumObject& operator[](const char* name) const {
        CharSortKey key(name);

        return SortedArray<EnumObject>::operator[](key);
    }
};

class EnumsSortedByValue : public SortedArray<EnumObject> {
public:
    EnumsSortedByValue(unsigned short growth_factor) : SortedArray<EnumObject>(growth_factor) {}

    SortKey& GetSortKey(EnumObject& object) const { return object.GetValueKey(); }

    EnumObject& operator[](unsigned short value) const {
        ShortSortKey key(value);

        return SortedArray<EnumObject>::operator[](key);
    }
};

class SortedEnum {
    EnumsSortedByName enums_name;
    EnumsSortedByValue enums_value;

public:
    SortedEnum(unsigned short growth_factor) : enums_name(growth_factor), enums_value(growth_factor) {}
    ~SortedEnum() {}

    void Insert(const char* name, unsigned short value) {
        EnumObject* object = new (std::nothrow) EnumObject(name, value);

        enums_name.Insert(*object);
        enums_value.Insert(*object);
    };

    unsigned short operator[](const char* name) const {
        unsigned short value;
        EnumObject& object = enums_name[name];

        if (&object != nullptr) {
            value = object.GetValue();
        } else {
            value = 0;
        }

        return value;
    }

    const char* operator[](unsigned short value) const {
        const char* name;
        EnumObject& object = enums_value[value];

        if (&object != nullptr) {
            name = object.GetName();
        } else {
            name = nullptr;
        }

        return name;
    }

    bool GetValue(const char* name, unsigned short& value) {
        bool result;
        EnumObject& object = enums_name[name];

        if (&object != nullptr) {
            value = object.GetValue();
            result = true;
        } else {
            value = 0;
            result = false;
        }

        return result;
    }
};

#endif /* SORTEDENUM_HPP */
