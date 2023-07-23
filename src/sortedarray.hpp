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

#ifndef SORTEDARRAY_HPP
#define SORTEDARRAY_HPP

#include <cstring>

#include "smartarray.hpp"

class SortKey {
public:
    SortKey() {}
    SortKey(const SortKey& other) {}
    virtual ~SortKey() {}

    virtual int32_t Compare(const SortKey& other) const = 0;
};

class CharSortKey : public SortKey {
    const char* name;

public:
    CharSortKey(const char* name) : name(name) {}
    CharSortKey(const CharSortKey& other) : name(other.name) {}

    int32_t Compare(const SortKey& other) const { return strcmp(name, dynamic_cast<const CharSortKey&>(other).name); }
    const char* GetKey() const { return name; }
};

class ShortSortKey : public SortKey {
    uint16_t key;

public:
    ShortSortKey(uint16_t key) : key(key) {}
    ShortSortKey(const ShortSortKey& other) : key(other.key) {}

    int32_t Compare(const SortKey& other) const { return key - dynamic_cast<const ShortSortKey&>(other).key; }
    uint16_t GetKey() const { return key; }
};

template <class T>
class SortedArray : public SmartArray<T> {
    int32_t Find(SortKey& sort_key, bool mode) const {
        int32_t last = this->GetCount() - 1;
        int32_t first = 0;
        int32_t position;

        while (first <= last) {
            int32_t comparison_result;

            position = (first + last) / 2;

            comparison_result = sort_key.Compare(GetSortKey(operator[](position)));

            if (0 == comparison_result) {
                return position;
            } else if (comparison_result < 0) {
                last = position - 1;
            } else {
                first = position + 1;
            }
        }

        if (mode) {
            position = -1;
        } else {
            position = first;
        }

        return position;
    }

public:
    SortedArray(uint16_t growth_factor) : SmartArray<T>(growth_factor) {}
    virtual ~SortedArray() {}

    uint16_t Insert(T& object) {
        uint16_t position;

        position = Find(GetSortKey(object), false);
        SmartArray<T>::Insert(&object, position);

        return position;
    }

    virtual SortKey& GetSortKey(T& object) const = 0;
    T& operator[](SortKey& sort_key) const {
        T* object;
        int32_t position;

        position = Find(sort_key, true);
        if (position >= 0) {
            object = &(SmartArray<T>::operator[](position));
        } else {
            object = nullptr;
        }

        return *object;
    }
    T& operator[](uint16_t index) const { return SmartArray<T>::operator[](index); }
};

#endif /* SORTEDARRAY_HPP */
