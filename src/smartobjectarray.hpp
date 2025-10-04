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
#include <type_traits>

#include "smartfile.hpp"

template <class T>
class ObjectArray {
    static_assert(std::is_trivially_copyable<T>::value, "T required to be a trivially copyable type.");
    static constexpr uint32_t MINIMUM_GROWTH_FACTOR = 5uL;
    static constexpr uint32_t DEFAULT_GROWTH_FACTOR = 20uL;

    uint32_t growth_factor;
    uint32_t count{0};
    uint32_t capacity{0};
    T* object_array{nullptr};

public:
    explicit ObjectArray(const uint32_t growth_factor = DEFAULT_GROWTH_FACTOR) noexcept
        : growth_factor(growth_factor < MINIMUM_GROWTH_FACTOR ? MINIMUM_GROWTH_FACTOR : growth_factor) {}
    ObjectArray(const ObjectArray<T>& other) noexcept
        : growth_factor(other.growth_factor), count(other.count), capacity(other.count) {
        if (capacity != UINT16_C(0)) {
            object_array = new (std::nothrow) T[capacity];
            memcpy(object_array, other.object_array, capacity * sizeof(T));

        } else {
            object_array = nullptr;
        }
    }
    virtual ~ObjectArray() noexcept { delete[] object_array; }

    inline uint32_t Insert(const T* const object, uint32_t position) noexcept {
        if (position > count) {
            position = count;
        }

        if (count >= capacity) {
            SDL_assert(capacity + growth_factor <= UINT32_MAX);

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

        return position;
    }
    inline void Append(const T* const object) noexcept { Insert(object, count); }
    inline bool Remove(const uint32_t position) noexcept {
        bool result{false};

        if (position < count) {
            if (position < (count - 1)) {
                memmove(&object_array[position], &object_array[position + 1], (count - position - 1) * sizeof(T));
            }

            --count;
            result = true;
        }

        return result;
    }
    [[nodiscard]] inline int64_t Find(const T* const object) const noexcept {
        for (uint32_t index = 0; index < count; ++index) {
            if (!memcmp(&object_array[index], object, sizeof(T))) {
                return index;
            }
        }

        return -1;
    }
    inline void Clear() noexcept { count = 0; }
    [[nodiscard]] inline uint32_t GetCount() const noexcept { return count; }
    [[nodiscard]] inline T* operator[](uint32_t position) const noexcept {
        return (position < count) ? &object_array[position] : nullptr;
    }
};

template <class T>
class ObjectSmartArrayData : public SmartObject, public ObjectArray<T> {
public:
    explicit ObjectSmartArrayData(const uint32_t growth_factor) noexcept : ObjectArray<T>(growth_factor) {}
    ObjectSmartArrayData(const ObjectSmartArrayData<T>& other) noexcept : ObjectArray<T>(other) {}
    ~ObjectSmartArrayData() noexcept override = default;
};

template <class T>
class SmartObjectArray : public SmartPointer<ObjectSmartArrayData<T>> {
    static constexpr uint32_t DEFAULT_GROWTH_FACTOR = 20uL;

    [[nodiscard]] ObjectArray<T>* GetArray() const noexcept { return this->Get(); }

public:
    explicit SmartObjectArray(const uint32_t growth_factor = DEFAULT_GROWTH_FACTOR) noexcept
        : SmartPointer<ObjectSmartArrayData<T>>(*new (std::nothrow) ObjectSmartArrayData<T>(growth_factor)) {}

    SmartObjectArray(const SmartObjectArray& other, const bool deep_copy = false) noexcept
        : SmartPointer<ObjectSmartArrayData<T>>(deep_copy ? (*new (std::nothrow) ObjectSmartArrayData<T>(*other.Get()))
                                                          : (*other.Get())) {}

    inline void Clear() noexcept { GetArray()->Clear(); }
    [[nodiscard]] inline T* operator[](uint32_t position) const noexcept { return (*GetArray())[position]; }
    [[nodiscard]] inline uint32_t GetCount() const noexcept { return GetArray()->GetCount(); }
    inline uint32_t Insert(const T* const object, const uint32_t position) noexcept {
        return GetArray()->Insert(object, position);
    }
    inline bool Remove(const uint32_t position) noexcept { return GetArray()->Remove(position); }
    inline void PushBack(const T* const object) noexcept { (void)Insert(object, GetCount()); }
};

#endif /* SMARTOBJECTARRAY_HPP */
