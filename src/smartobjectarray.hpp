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

#ifndef SMARTOBJECTARRAY_HPP
#define SMARTOBJECTARRAY_HPP

#include <cstdint>
#include <cstring>

#include "smartfile.hpp"

#define OBJECTARRAY_DEFAULT_GROWTH_FACTOR 5
#define SMARTOBJECTARRAY_DEFAULT_GROWTH_FACTOR 5

template <class T>
class ObjectArray {
    unsigned short count;
    unsigned short capacity;
    unsigned short growth_factor;
    T* object_array;

public:
    ObjectArray(unsigned short growth_factor = OBJECTARRAY_DEFAULT_GROWTH_FACTOR)
        : growth_factor(growth_factor), count(0), capacity(0), object_array(nullptr) {}

    ObjectArray(const ObjectArray<T>& other)
        : growth_factor(other.growth_factor), count(other.count), capacity(other.count) {
        if (capacity) {
            object_array = new (std::nothrow) T[capacity];
            memcpy(object_array, other.object_array, capacity * sizeof(T));
        } else {
            object_array = nullptr;
        }
    }

    virtual ~ObjectArray() { delete[] object_array; }

    void Insert(T* object, unsigned short position) {
        if (position > count) {
            position = count;
        }

        if (count >= capacity) {
            SDL_assert(capacity + growth_factor <= UINT16_MAX);

            capacity += growth_factor;
            T* new_object_array = new (std::nothrow) T[capacity];

            if (position > 0) {
                memcpy(new_object_array, object_array, position * sizeof(T));
            }

            if (position < count) {
                memcpy(&new_object_array[position + 1], &object_array[position], (count - position) * sizeof(T));
            }

            delete[] object_array;

            object_array = new_object_array;
        } else if (position < count) {
            memmove(&object_array[position + 1], &object_array[position], (count - position) * sizeof(T));
        }

        memcpy(&object_array[position], object, sizeof(T));
        ++count;
    }

    void Append(T* object) { Insert(object, count); }

    void Remove(unsigned short position) {
        if (position < count) {
            if (position < (count - 1)) {
                memmove(&object_array[position], &object_array[position + 1], (count - position - 1) * sizeof(T));
            }

            SDL_assert(count != 0);

            --count;
        }
    }

    int Find(T* object) const {
        for (unsigned short index = 0; index < count; ++index) {
            if (!memcmp(&object_array[index], object, sizeof(T))) {
                return index;
            }
        }

        return -1;
    }

    void Clear() { count = 0; }

    unsigned short GetCount() const { return count; }

    T* operator[](unsigned short position) const {
        SDL_assert(position < count);
        return &object_array[position];
    }
};

template <class T>
class ObjectSmartArrayData : public SmartObject, public ObjectArray<T> {
public:
    ObjectSmartArrayData(unsigned short growth_factor) : ObjectArray<T>(growth_factor) {}

    ObjectSmartArrayData(const ObjectSmartArrayData<T>& other) : ObjectArray<T>(other) {}

    ~ObjectSmartArrayData() {}
};

template <class T>
class SmartObjectArray : public SmartPointer<ObjectSmartArrayData<T>> {
    ObjectArray<T>* GetArray() const { return this->object_pointer; }

public:
    SmartObjectArray(unsigned short growth_factor = SMARTOBJECTARRAY_DEFAULT_GROWTH_FACTOR)
        : SmartPointer<ObjectSmartArrayData<T>>(*new(std::nothrow) ObjectSmartArrayData<T>(growth_factor)) {}

    SmartObjectArray(const SmartObjectArray& other, bool deep_copy = false)
        : SmartPointer<ObjectSmartArrayData<T>>(
              deep_copy ? (*new(std::nothrow) ObjectSmartArrayData<T>(*other.object_pointer))
                        : (*other.object_pointer)) {}

    void Clear() { GetArray()->Clear(); }

    T* operator[](unsigned short position) const { return (*GetArray())[position]; }

    unsigned short GetCount() const { return GetArray()->GetCount(); }

    void Insert(T* object, unsigned short position) { GetArray()->Insert(object, position); }

    void Remove(unsigned short position) { GetArray()->Remove(position); }

    void PushBack(T* object) { Insert(object, GetCount()); }
};

#endif /* SMARTOBJECTARRAY_HPP */
