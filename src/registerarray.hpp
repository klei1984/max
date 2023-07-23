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

class MAXRegisterClass : public SmartObject {
    CharSortKey sortkey;
    FileObject* (*allocator)();
    uint16_t* type_index;

    void Insert();

public:
    MAXRegisterClass(const char* class_name, uint16_t* type_index, FileObject* (*allocator)())
        : sortkey(class_name), allocator(allocator), type_index(type_index) {
        Insert();
    }
    MAXRegisterClass(const MAXRegisterClass& other)
        : sortkey(other.sortkey), allocator(other.allocator), type_index(other.type_index) {}
    ~MAXRegisterClass() {}

    FileObject* Allocate() const { return allocator(); }
    uint16_t* GetTypeIndexPointer() const { return type_index; }
    static void SetTypeIndex(uint16_t* type_index_pointer, uint16_t value) { *type_index_pointer = value; }
    SortKey& GetSortKey() { return sortkey; }
    const char* GetClassName() const { return sortkey.GetKey(); }
};

class RegisterArray : public SortedArray<MAXRegisterClass> {
public:
    RegisterArray(uint16_t growth_factor) : SortedArray<MAXRegisterClass>(growth_factor) {}

    void Insert(MAXRegisterClass& object) {
        for (uint16_t position = SortedArray<MAXRegisterClass>::Insert(object); position < GetCount();
             ++position) {
            MAXRegisterClass::SetTypeIndex(operator[](position).GetTypeIndexPointer(), position + 1);
        }
    }

    SortKey& GetSortKey(MAXRegisterClass& object) const { return object.GetSortKey(); }
};

extern RegisterArray* registered_classes;

#endif /* REGISTERARRAY_HPP */
