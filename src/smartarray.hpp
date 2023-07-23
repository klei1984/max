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

#define SMARTARRAY_DEFAULT_GROWTH_FACTOR 5

template <class T>
class SmartArray {
    uint16_t capacity;
    uint16_t growth_factor;
    uint16_t count;
    SmartPointer<T>* smartarray;

public:
    SmartArray(uint16_t growth_factor = SMARTARRAY_DEFAULT_GROWTH_FACTOR)
        : capacity(0), growth_factor(growth_factor), count(0), smartarray(nullptr) {}
    ~SmartArray() {
        Release();

        delete[] smartarray;
    }

    void Insert(T* object, uint16_t index = SHRT_MAX) {
        SmartPointer<T>* array;

        if (count == capacity) {
            array = new (std::nothrow) SmartPointer<T>[growth_factor + capacity + 1];

            for (int32_t i = 0; i < count; ++i) {
                array[i] = smartarray[i];
            }

            delete[] smartarray;

            smartarray = array;
            capacity += growth_factor;
        }

        if (index > count) {
            index = count;
        }

        for (int32_t i = count; i > index; --i) {
            smartarray[i] = smartarray[i - 1];
        }

        smartarray[index] = object;
        ++count;
    }

    void Erase(uint16_t index) {
        SDL_assert(index < count);

        for (int32_t i = index; i < count - 1; ++i) {
            smartarray[i] = smartarray[i + 1];
        }

        SmartPointer<T> sp;
        smartarray[count - 1] = sp;
        --count;
    }

    void Release() {
        for (int32_t i = 0; i < count; ++i) {
            SmartPointer<T> sp;
            smartarray[i] = sp;
        }

        count = 0;
    }

    uint16_t GetCount() const { return count; }

    T& operator[](uint16_t index) const { return *smartarray[index]; }
};

#endif /* SMARTARRAY_HPP */
