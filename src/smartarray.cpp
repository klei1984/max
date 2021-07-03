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

#include "smartarray.hpp"

#include <new>

#include "SDL_assert.h"

SmartArray::SmartArray(unsigned short growth_factor)
    : capacity(0), growth_factor(growth_factor), count(0), smartarray(nullptr) {}

SmartArray::~SmartArray() {
    Release();

    if (smartarray) {
        delete[] smartarray;
    }
}

void SmartArray::Insert(SmartObject &object, unsigned short index) {
    SmartPointer *array;

    if (count == capacity) {
        array = new (std::nothrow) SmartPointer[growth_factor + capacity + 1];

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

    smartarray[index] = object;
    ++count;
}

void SmartArray::Erase(unsigned short index) {
    SDL_assert(index < count);

    for (int i = index; i < count - 1; ++i) {
        smartarray[i] = smartarray[i + 1];
    }

    SmartPointer sp;
    smartarray[count - 1] = sp;
    --count;
}

void SmartArray::Release() {
    for (int i = 0; i < count; ++i) {
        SmartPointer sp;
        smartarray[i] = sp;
    }

    count = 0;
}

unsigned short SmartArray::GetCount() const { return count; }
