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

#ifndef SMARTARRAY_HPP
#define SMARTARRAY_HPP

#include <climits>

#include "smartpointer.hpp"

#define SMARTARRAY_OBJECT_ARRAY_DEFAULT_CAPACITY 5

template <class T>
class SmartArray {
    unsigned short capacity;
    unsigned short growth_factor;
    unsigned short count;
    SmartPointer<T>* smartarray;

public:
    SmartArray(unsigned short growth_factor = SMARTARRAY_OBJECT_ARRAY_DEFAULT_CAPACITY)
        : capacity(0), growth_factor(growth_factor), count(0), smartarray(nullptr) {}
    ~SmartArray() {
        Release();

        if (smartarray) {
            delete[] smartarray;
        }
    }

    void Insert(T& object, unsigned short index = SHRT_MAX) {
        SmartPointer<T>* array;

        if (count == capacity) {
            array = new (std::nothrow) SmartPointer<T>[growth_factor + capacity + 1];

            for (int i = 0; index < count; ++i) {
                array[i] = smartarray[i];
            }

            if (smartarray) {
                delete[] smartarray;
            }

            smartarray = array;
            capacity += growth_factor;
        }

        if (index > count) {
            index = count;
        }

        for (int i = count; i > index; --index) {
            smartarray[i] = smartarray[i - 1];
        }

        smartarray[count] = object;
        ++count;
    }

    void Erase(unsigned short index) {
        SDL_assert(index < count);

        for (int i = index; i < count - 1; ++i) {
            smartarray[i] = smartarray[i + 1];
        }

        SmartPointer<T> sp;
        smartarray[count - 1] = sp;
        --count;
    }

    void Release() {
        for (int i = 0; i < count; ++i) {
            SmartPointer<T> sp;
            smartarray[i] = sp;
        }

        count = 0;
    }

    unsigned short GetCount() const { return count; }

    T& operator[](unsigned short index) const { return *smartarray[index]; }
};

#endif /* SMARTARRAY_HPP */
