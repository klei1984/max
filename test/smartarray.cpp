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

#include "smartarray.hpp"

#include <gtest/gtest.h>

#include "testobject.hpp"

class SmartArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        to1 = new (std::nothrow) TestObject();
        to2 = new (std::nothrow) TestObject();
        to3 = new (std::nothrow) TestObject();
        to4 = new (std::nothrow) TestObject();
        to5 = new (std::nothrow) TestObject();

        to1->Set(1);
        to2->Set(2);
        to3->Set(3);
        to4->Set(4);
        to5->Set(5);
    }

    void TearDown() override {}

public:
    SmartPointer<TestObject> to1;
    SmartPointer<TestObject> to2;
    SmartPointer<TestObject> to3;
    SmartPointer<TestObject> to4;
    SmartPointer<TestObject> to5;
};

TEST_F(SmartArrayTest, Ctor) {
    SmartArray<TestObject> array1;
    SmartArray<TestObject> array2(10);
    SmartArray<TestObject> array3(0);
    SmartArray<TestObject> array4(-1);

    EXPECT_EQ(array1.GetCount(), 0);
    EXPECT_EQ(array2.GetCount(), 0);
    EXPECT_EQ(array3.GetCount(), 0);
    EXPECT_EQ(array4.GetCount(), 0);
}

TEST_F(SmartArrayTest, Growth) {
    SmartArray<TestObject> array1;
    SmartArray<TestObject> array2(7);
    SmartArray<TestObject> array3(0);
    SmartArray<TestObject> array4(-10);

    static constexpr uint16_t OBJECT_COUNT = UINT16_C(32767);

    for (int32_t i = 0; i < OBJECT_COUNT; ++i) {
        array1.Insert(to1.Get());
        array2.Insert(to2.Get());
        array3.Insert(to3.Get());
        array4.Insert(to4.Get());
    }

    EXPECT_EQ(array1.GetCount(), OBJECT_COUNT);
    EXPECT_EQ(array2.GetCount(), OBJECT_COUNT);
    EXPECT_EQ(array3.GetCount(), OBJECT_COUNT);
    EXPECT_EQ(array4.GetCount(), OBJECT_COUNT);
}

TEST_F(SmartArrayTest, InsertOrder) {
    SmartArray<TestObject> array;

    array.Insert(to1.Get());
    array.Insert(to2.Get());
    array.Insert(to3.Get());
    array.Insert(to4.Get());
    array.Insert(to5.Get());
    array.Insert(to4.Get());
    array.Insert(to3.Get());
    array.Insert(to2.Get());
    array.Insert(to1.Get());
    array.Insert(to2.Get());

    array.Insert(to5.Get(), 0);

    EXPECT_EQ(array.GetCount(), 11);
    EXPECT_EQ(array[0].Get(), 5);
    EXPECT_EQ(array[1].Get(), 1);
    EXPECT_EQ(array[2].Get(), 2);

    array.Insert(to5.Get(), 6);

    EXPECT_EQ(array.GetCount(), 12);
    EXPECT_EQ(array[5].Get(), 5);
    EXPECT_EQ(array[6].Get(), 5);
    EXPECT_EQ(array[7].Get(), 4);

    array.Insert(to5.Get(), 12);

    EXPECT_EQ(array.GetCount(), 13);
    EXPECT_EQ(array[11].Get(), 2);
    EXPECT_EQ(array[12].Get(), 5);

    array.Insert(to1.Get(), 14);

    EXPECT_EQ(array.GetCount(), 14);
    EXPECT_EQ(array[12].Get(), 5);
    EXPECT_EQ(array[13].Get(), 1);
}

TEST_F(SmartArrayTest, Remove) {
    SmartArray<TestObject> array;

    array.Insert(to1.Get());
    array.Insert(to2.Get());
    array.Insert(to3.Get());
    array.Insert(to4.Get());
    array.Insert(to5.Get());

    EXPECT_EQ(array.GetCount(), 5);

    array.Erase(0);

    EXPECT_EQ(array.GetCount(), 4);
    EXPECT_EQ(array[0].Get(), 2);
    EXPECT_EQ(array[3].Get(), 5);

    array.Erase(3);

    EXPECT_EQ(array.GetCount(), 3);
    EXPECT_EQ(array[0].Get(), 2);
    EXPECT_EQ(array[2].Get(), 4);

    array.Erase(1);

    EXPECT_EQ(array.GetCount(), 2);
    EXPECT_EQ(array[0].Get(), 2);
    EXPECT_EQ(array[1].Get(), 4);

    array.Erase(0);
    array.Erase(0);

    EXPECT_EQ(array.GetCount(), 0);
}

TEST_F(SmartArrayTest, Release) {
    SmartArray<TestObject> array;

    array.Release();

    EXPECT_EQ(array.GetCount(), 0);

    array.Insert(to1.Get());
    array.Insert(to2.Get());
    array.Insert(to3.Get());
    array.Insert(to4.Get());
    array.Insert(to5.Get());

    EXPECT_EQ(array.GetCount(), 5);

    array.Release();

    EXPECT_EQ(array.GetCount(), 0);
}
