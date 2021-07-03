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

#include "smartptr.hpp"

#include "SDL_assert.h"

SmartObject::SmartObject() : reference_count(0) {}

SmartObject::SmartObject(const SmartObject& other) : reference_count(0) {}

SmartObject::~SmartObject() { SDL_assert(reference_count == 0); }

SmartPointer::SmartPointer() : object_pointer(nullptr) {}

SmartPointer::SmartPointer(SmartObject* object = nullptr) : object_pointer(object) { Increment(); }

SmartPointer::SmartPointer(const SmartPointer& other) : object_pointer(other.object_pointer) { Increment(); }

SmartPointer::~SmartPointer() { Decrement(); }

void SmartPointer::Increment() const {
    if (object_pointer) {
        ++object_pointer->reference_count;
    }
}

void SmartPointer::Decrement() {
    if (object_pointer && --object_pointer->reference_count == 0) {
        delete object_pointer;
        object_pointer = nullptr;
    }
}

SmartObject* SmartPointer::operator->() const { return object_pointer; }

SmartObject& SmartPointer::operator*() const { return *object_pointer; }

SmartPointer& SmartPointer::operator=(const SmartPointer& other) {
    other.Increment();
    Decrement();
    object_pointer = other.object_pointer;

    return *this;
}

SmartPointer& SmartPointer::operator=(SmartObject& other) {
    ++other.reference_count;
    Decrement();
    object_pointer = &other;

    return *this;
}

bool SmartPointer::check(bool test_is_null_mode) const {
    return (test_is_null_mode) ? object_pointer == nullptr : object_pointer != nullptr;
}

bool SmartPointer::check_inverse(bool test_is_not_null_mode) const {
    return (test_is_not_null_mode) ? object_pointer != nullptr : object_pointer == nullptr;
}
