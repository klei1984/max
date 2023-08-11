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

#ifndef REGISTERARRAY_HPP
#define REGISTERARRAY_HPP

#include "fileobject.hpp"
#include "sortedarray.hpp"

class RegisterArray;

class RegisterClass : public SmartObject {
    static constexpr uint16_t DEFAULT_GROWTH_FACTOR = UINT16_C(5);
    static RegisterArray* registered_classes;
    CharSortKey sortkey;
    FileObject* (*allocator)();
    uint16_t* type_index;

    void Insert() noexcept;

public:
    RegisterClass(const char* class_name, uint16_t* type_index, FileObject* (*allocator)()) noexcept
        : sortkey(class_name), allocator(allocator), type_index(type_index) {
        Insert();
    }
    RegisterClass(const RegisterClass& other) noexcept
        : sortkey(other.sortkey), allocator(other.allocator), type_index(other.type_index) {}
    ~RegisterClass() noexcept override = default;

    [[nodiscard]] inline FileObject* Allocate() const noexcept { return allocator(); }
    [[nodiscard]] inline uint16_t* GetTypeIndexPointer() const noexcept { return type_index; }
    static void SetTypeIndex(uint16_t* type_index_pointer, uint16_t value) noexcept { *type_index_pointer = value; }
    [[nodiscard]] inline SortKey& GetSortKey() noexcept { return sortkey; }
    [[nodiscard]] inline const char* GetClassName() const noexcept { return sortkey.GetKey(); }
    [[nodiscard]] static inline RegisterArray& GetRegister() noexcept { return *registered_classes; }
};

class RegisterArray : public SortedArray<RegisterClass> {
public:
    explicit RegisterArray(uint16_t growth_factor) noexcept : SortedArray<RegisterClass>(growth_factor) {}
    ~RegisterArray() noexcept override = default;

    inline void Insert(RegisterClass& object) noexcept {
        for (uint16_t position = SortedArray<RegisterClass>::Insert(object); position < GetCount(); ++position) {
            RegisterClass::SetTypeIndex(operator[](position).GetTypeIndexPointer(), position + 1);
        }
    }
    [[nodiscard]] inline SortKey& GetSortKey(RegisterClass& object) const noexcept override {
        return object.GetSortKey();
    }
};

#endif /* REGISTERARRAY_HPP */
