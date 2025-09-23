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

#include <cstdint>
#include <new>

class SmartObject {
    template <class T>
    friend class SmartPointer;
    uint32_t reference_count{0};

protected:
    inline void Increment() noexcept {
        SDL_assert(reference_count != UINT32_MAX);

        ++reference_count;
    }

    inline void Decrement() noexcept {
        SDL_assert(reference_count != 0);

        --reference_count;
        if (reference_count == 0) {
            delete this;
        }
    }

public:
    SmartObject() noexcept = default;
    SmartObject(const SmartObject& /* unused */) noexcept = default;
    virtual ~SmartObject() noexcept { SDL_assert(reference_count == 0); }
};

template <class T>
class SmartPointer {
    T* object_pointer{nullptr};

public:
    using Reference = SmartPointer<T>&;
    using ConstReference = const SmartPointer<T>&;

    SmartPointer() noexcept = default;

    SmartPointer(T* object) noexcept : object_pointer(object) {
        if (object_pointer) {
            object_pointer->Increment();
        }
    }

    SmartPointer(T& object) noexcept : object_pointer(&object) {
        if (object_pointer) {
            object_pointer->Increment();
        }
    }

    SmartPointer(ConstReference other) noexcept : object_pointer(other.object_pointer) {
        if (object_pointer) {
            object_pointer->Increment();
        }
    }

    ~SmartPointer() noexcept {
        if (object_pointer) {
            object_pointer->Decrement();
        }
    }

    inline T* operator->() const noexcept { return object_pointer; }

    inline T& operator*() const noexcept { return *object_pointer; }

    Reference operator=(ConstReference other) noexcept {
        if (other.object_pointer) {
            other.object_pointer->Increment();
        }

        if (object_pointer) {
            object_pointer->Decrement();
        }

        object_pointer = other.object_pointer;

        return *this;
    }

    Reference operator=(T* other) noexcept {
        if (other) {
            other->Increment();
        }

        if (object_pointer) {
            object_pointer->Decrement();
        }

        object_pointer = other;

        return *this;
    }

    Reference operator=(T& other) noexcept {
        other.Increment();

        if (object_pointer) {
            object_pointer->Decrement();
        }

        object_pointer = &other;

        return *this;
    }

    inline explicit operator bool() const noexcept { return object_pointer != nullptr; }

    inline T* Get() const noexcept { return object_pointer; };

    friend inline bool operator==(ConstReference lhs, ConstReference rhs) noexcept { return lhs.Get() == rhs.Get(); }

    friend inline bool operator==(ConstReference lhs, const T* rhs) noexcept { return lhs.Get() == rhs; }

    friend inline bool operator==(const T* lhs, ConstReference rhs) noexcept { return lhs == rhs.Get(); }

    friend inline bool operator==(ConstReference lhs, const T& rhs) noexcept { return lhs.Get() == &rhs; }

    friend inline bool operator==(const T& lhs, ConstReference rhs) noexcept { return &lhs == rhs.Get(); }

    friend inline bool operator==(ConstReference lhs, std::nullptr_t) noexcept { return !lhs.Get(); }

    friend inline bool operator==(std::nullptr_t, ConstReference rhs) noexcept { return !rhs.Get(); }

    friend inline bool operator!=(ConstReference lhs, ConstReference rhs) noexcept { return !(lhs == rhs); }

    friend inline bool operator!=(ConstReference lhs, const T* rhs) noexcept { return !(lhs.Get() == rhs); }

    friend inline bool operator!=(const T* lhs, ConstReference rhs) noexcept { return !(lhs == rhs.Get()); }

    friend inline bool operator!=(ConstReference lhs, const T& rhs) noexcept { return !(lhs.Get() == &rhs); }

    friend inline bool operator!=(const T& lhs, ConstReference rhs) noexcept { return !(&lhs == rhs.Get()); }

    friend inline bool operator!=(ConstReference lhs, std::nullptr_t) noexcept { return lhs.Get(); }

    friend inline bool operator!=(std::nullptr_t, ConstReference rhs) noexcept { return rhs.Get(); }
};

#endif /* SMARTPOINTER_HPP */
