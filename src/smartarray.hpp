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

#include <cstdint>

#include "smartpointer.hpp"

template <class T>
class SmartArray {
    static constexpr uint32_t MINIMUM_GROWTH_FACTOR = 5uL;
    static constexpr uint32_t DEFAULT_GROWTH_FACTOR = 20uL;

    SmartPointer<T>* smartarray{nullptr};
    uint32_t growth_factor;
    uint32_t capacity{0uL};
    uint32_t count{0uL};

public:
    SmartArray(uint32_t growth_factor = DEFAULT_GROWTH_FACTOR) noexcept
        : growth_factor(growth_factor < MINIMUM_GROWTH_FACTOR ? MINIMUM_GROWTH_FACTOR : growth_factor) {}
    virtual ~SmartArray() noexcept {
        Release();

        delete[] smartarray;
    }

    inline uint32_t Insert(T* const object, uint32_t index = UINT32_MAX) noexcept {
        if (count == capacity) {
            auto* array = new (std::nothrow) SmartPointer<T>[growth_factor + capacity];

            for (uint32_t i = 0uL; i < count; ++i) {
                array[i] = smartarray[i];
                smartarray[i] = nullptr;
            }

            delete[] smartarray;

            smartarray = array;
            capacity += growth_factor;
        }

        if (index > count) {
            index = count;
        }

        for (uint32_t i = count; i > index; --i) {
            smartarray[i] = smartarray[i - 1];
        }

        smartarray[index] = object;
        ++count;

        return index;
    }

    inline void Erase(const uint32_t index) noexcept {
        SDL_assert(index < count);

        for (uint32_t i = index; i < count - 1; ++i) {
            smartarray[i] = smartarray[i + 1];
        }

        smartarray[count - 1] = nullptr;
        --count;
    }

    inline void Release() noexcept {
        for (uint32_t i = 0uL; i < count; ++i) {
            smartarray[i] = nullptr;
        }

        count = 0uL;
    }

    [[nodiscard]] inline uint32_t GetCount() const noexcept { return count; }

    [[nodiscard]] inline T& operator[](const uint32_t index) const noexcept {
        SDL_assert(index < capacity);

        return *smartarray[index];
    }
};

#endif /* SMARTARRAY_HPP */
