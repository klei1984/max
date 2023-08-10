/* Copyright (c) 2023 M.A.X. Port Team
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

#include "sortedarray.hpp"

#include <gtest/gtest.h>

class CharSortTestObject : public SmartObject {
    CharSortKey sortkey;

public:
    CharSortTestObject(const char* key) : sortkey(key) {}
    ~CharSortTestObject() = default;
    SortKey& GetSortKey() { return sortkey; }
};

class ShortSortTestObject : public SmartObject {
    ShortSortKey sortkey;

public:
    ShortSortTestObject(uint16_t key) : sortkey(key) {}
    ~ShortSortTestObject() = default;
    SortKey& GetSortKey() { return sortkey; }
};

template <class T>
class TestSortedArray : public SortedArray<T> {
public:
    TestSortedArray(uint16_t growth_factor) : SortedArray<T>(growth_factor) {}
    SortKey& GetSortKey(T& object) const { return object.GetSortKey(); }
};

class SortedArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        toa = new (std::nothrow) CharSortTestObject("a");
        tob = new (std::nothrow) CharSortTestObject("b");
        toc = new (std::nothrow) CharSortTestObject("c");
        tod = new (std::nothrow) CharSortTestObject("d");
        toe = new (std::nothrow) CharSortTestObject("e");

        to1 = new (std::nothrow) ShortSortTestObject(1);
        to2 = new (std::nothrow) ShortSortTestObject(2);
        to3 = new (std::nothrow) ShortSortTestObject(3);
        to4 = new (std::nothrow) ShortSortTestObject(4);
        to5 = new (std::nothrow) ShortSortTestObject(5);
    }

    void TearDown() override {}

public:
    SmartPointer<CharSortTestObject> toa;
    SmartPointer<CharSortTestObject> tob;
    SmartPointer<CharSortTestObject> toc;
    SmartPointer<CharSortTestObject> tod;
    SmartPointer<CharSortTestObject> toe;

    SmartPointer<ShortSortTestObject> to1;
    SmartPointer<ShortSortTestObject> to2;
    SmartPointer<ShortSortTestObject> to3;
    SmartPointer<ShortSortTestObject> to4;
    SmartPointer<ShortSortTestObject> to5;
};

TEST_F(SortedArrayTest, CharSortCtor) {
    TestSortedArray<CharSortTestObject> array(5);

    EXPECT_EQ(array.GetCount(), 0);
}

TEST_F(SortedArrayTest, CharSortInsert) {
    TestSortedArray<CharSortTestObject> array(5);

    EXPECT_EQ(array.GetCount(), 0);

    array.Insert(*tod);
    array.Insert(*toc);
    array.Insert(*toa);
    array.Insert(*toe);
    array.Insert(*tob);

    EXPECT_STREQ(dynamic_cast<CharSortKey*>(&array[0].GetSortKey())->GetKey(), "a");
    EXPECT_STREQ(dynamic_cast<CharSortKey*>(&array[1].GetSortKey())->GetKey(), "b");
    EXPECT_STREQ(dynamic_cast<CharSortKey*>(&array[2].GetSortKey())->GetKey(), "c");
    EXPECT_STREQ(dynamic_cast<CharSortKey*>(&array[3].GetSortKey())->GetKey(), "d");
    EXPECT_STREQ(dynamic_cast<CharSortKey*>(&array[4].GetSortKey())->GetKey(), "e");
}

TEST_F(SortedArrayTest, ShortSortCtor) {
    TestSortedArray<ShortSortTestObject> array(5);

    EXPECT_EQ(array.GetCount(), 0);
}

TEST_F(SortedArrayTest, ShortSortInsert) {
    TestSortedArray<ShortSortTestObject> array(5);

    EXPECT_EQ(array.GetCount(), 0);

    array.Insert(*to4);
    array.Insert(*to3);
    array.Insert(*to1);
    array.Insert(*to5);
    array.Insert(*to2);

    EXPECT_EQ(dynamic_cast<ShortSortKey*>(&array[0].GetSortKey())->GetKey(), 1);
    EXPECT_EQ(dynamic_cast<ShortSortKey*>(&array[1].GetSortKey())->GetKey(), 2);
    EXPECT_EQ(dynamic_cast<ShortSortKey*>(&array[2].GetSortKey())->GetKey(), 3);
    EXPECT_EQ(dynamic_cast<ShortSortKey*>(&array[3].GetSortKey())->GetKey(), 4);
    EXPECT_EQ(dynamic_cast<ShortSortKey*>(&array[4].GetSortKey())->GetKey(), 5);
}

TEST_F(SortedArrayTest, SortKeyOperator) {
    TestSortedArray<CharSortTestObject> carray(5);
    TestSortedArray<ShortSortTestObject> sarray(5);

    EXPECT_EQ(carray.GetCount(), 0);
    EXPECT_EQ(sarray.GetCount(), 0);

    carray.Insert(*tod);
    carray.Insert(*toc);
    carray.Insert(*toa);
    carray.Insert(*toe);

    sarray.Insert(*to4);
    sarray.Insert(*to3);
    sarray.Insert(*to1);
    sarray.Insert(*to5);

    EXPECT_EQ(carray[tod->GetSortKey()], tod.Get());
    EXPECT_NE(carray[toa->GetSortKey()], tod.Get());
    EXPECT_EQ(carray[tob->GetSortKey()], nullptr);

    EXPECT_EQ(sarray[to3->GetSortKey()], to3.Get());
    EXPECT_NE(sarray[to1->GetSortKey()], to3.Get());
    EXPECT_EQ(sarray[to2->GetSortKey()], nullptr);
}
