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

#ifndef SMARTPOINTER_HPP
#define SMARTPOINTER_HPP

#include <SDL_assert.h>

#include <new>

class SmartObject {
    template <class T>
    friend class SmartPointer;
    unsigned short reference_count;

protected:
    inline void Increment() {
        if (this) {
            ++reference_count;
        }
    }

    inline void Decrement() {
        if (this && --reference_count == 0) {
            delete this;
        }
    }

public:
    SmartObject() : reference_count(0) {}
    SmartObject(const SmartObject& other) : reference_count(0) {}
    virtual ~SmartObject() { SDL_assert(reference_count == 0); }
};

template <class T>
class SmartPointer {
protected:
    T* object_pointer;

public:
    SmartPointer() : object_pointer(nullptr) {}
    SmartPointer(T* object) : object_pointer(object) { object_pointer->Increment(); }
    SmartPointer(T& object) : object_pointer(&object) { object_pointer->Increment(); }
    SmartPointer(const SmartPointer<T>& other) : object_pointer(other.object_pointer) { object_pointer->Increment(); }
    ~SmartPointer() { object_pointer->Decrement(); }

    T* operator->() const { return object_pointer; }

    T& operator*() const { return *object_pointer; }

    SmartPointer<T>& operator=(const SmartPointer<T>& other) {
        other.object_pointer->Increment();
        object_pointer->Decrement();
        object_pointer = other.object_pointer;

        return *this;
    }

    SmartPointer<T>& operator=(T* other) {
        other->Increment();
        object_pointer->Decrement();
        object_pointer = other;

        return *this;
    }

    SmartPointer<T>& operator=(T& other) {
        other.Increment();
        object_pointer->Decrement();
        object_pointer = &other;

        return *this;
    }

    bool operator==(const SmartPointer<T>& t) const { return object_pointer == t.object_pointer; }

    bool operator==(const T* t) const { return object_pointer == t; }

    bool operator!=(const SmartPointer<T>& t) const { return object_pointer != t.object_pointer; }

    bool operator!=(const T* t) const { return object_pointer != t; }
};

#endif /* SMARTPOINTER_HPP */
