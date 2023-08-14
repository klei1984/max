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

#include "smartobjectarray.hpp"

#include <gtest/gtest.h>

#include "point.hpp"

TEST(ObjectArrayTest, Ctor) {
    ObjectArray<uint16_t> array1;
    ObjectArray<bool> array2(-1);
    ObjectArray<float> array3(0);
    ObjectArray<Point> array4(100);

    EXPECT_EQ(array1.GetCount(), 0);
    EXPECT_EQ(array2.GetCount(), 0);
    EXPECT_EQ(array3.GetCount(), 0);
    EXPECT_EQ(array4.GetCount(), 0);
};

TEST(ObjectArrayTest, CopyCtor) {
    ObjectArray<uint16_t> array1;
    ObjectArray<uint16_t> array2;
    uint16_t variable{100};

    array1.Insert(&variable, UINT16_MAX);

    EXPECT_EQ(array1.GetCount(), 1);
    EXPECT_EQ(array2.GetCount(), 0);

    ObjectArray<uint16_t> array3(array1);
    ObjectArray<uint16_t> array4(array2);

    EXPECT_EQ(array3.GetCount(), 1);
    EXPECT_EQ(array4.GetCount(), 0);
};

TEST(ObjectArrayTest, Insert) {
    ObjectArray<Point> array(6);
    Point point1{INT16_MIN, INT16_MAX};
    Point point2{0, 0};
    Point point3{-1, 1};
    Point point4{-2, 2};
    Point point5{-3, 3};
    Point point6{-4, 4};
    Point point7{-5, 5};

    array.Insert(&point1, 5);
    array.Insert(&point2, 4);
    array.Insert(&point3, 3);
    array.Insert(&point4, 2);
    array.Insert(&point5, 1);
    array.Insert(&point6, 0);
    array.Insert(&point7, 4);

    EXPECT_EQ(array.GetCount(), 7);

    EXPECT_EQ(*array[0], Point(-4, 4));
    EXPECT_EQ(*array[1], Point(INT16_MIN, INT16_MAX));
    EXPECT_EQ(*array[2], Point(-3, 3));
    EXPECT_EQ(*array[3], Point(0, 0));
    EXPECT_EQ(*array[4], Point(-5, 5));
    EXPECT_EQ(*array[5], Point(-2, 2));
    EXPECT_EQ(*array[6], Point(-1, 1));
    EXPECT_EQ(array[7], nullptr);
}

TEST(ObjectArrayTest, Append) {
    ObjectArray<int64_t> array;
    int64_t int1{INT64_MIN};
    int64_t int2{INT64_MAX};
    int64_t int3{INT32_MIN};
    int64_t int4{INT32_MAX};
    int64_t int5{INT16_MIN};
    int64_t int6{INT16_MAX};

    array.Append(&int1);
    array.Append(&int2);
    array.Append(&int3);
    array.Append(&int4);
    array.Append(&int5);
    array.Append(&int6);

    EXPECT_EQ(array.GetCount(), 6);
    EXPECT_EQ(*array[0], int1);
    EXPECT_EQ(*array[1], int2);
    EXPECT_EQ(*array[2], int3);
    EXPECT_EQ(*array[3], int4);
    EXPECT_EQ(*array[4], int5);
    EXPECT_EQ(*array[5], int6);
}

TEST(ObjectArrayTest, MixedUse) {
    ObjectArray<int16_t> array1;

    const int16_t int1{INT16_MIN};
    const int16_t int2{INT16_MAX};
    const int16_t int3{0};
    const int16_t int4{-1};
    const int16_t int5{1};
    const int16_t int6{-2};
    const int16_t int7{2};
    const int16_t int8{-3};
    const int16_t int9{3};
    const int16_t int10{4};

    EXPECT_EQ(array1.GetCount(), 0);

    array1.Append(&int1);
    array1.Append(&int2);
    array1.Insert(&int3, 1);
    array1.Append(&int4);
    array1.Append(&int5);
    array1.Insert(&int6, 2);
    array1.Insert(&int7, 0);

    ObjectArray<int16_t> array2(array1);

    EXPECT_EQ(array1.GetCount(), 7);
    EXPECT_EQ(array2.GetCount(), 7);

    EXPECT_EQ(*array2[0], int7);
    EXPECT_EQ(*array2[1], int1);
    EXPECT_EQ(*array2[2], int3);
    EXPECT_EQ(*array2[3], int6);
    EXPECT_EQ(*array2[4], int2);
    EXPECT_EQ(*array2[5], int4);
    EXPECT_EQ(*array2[6], int5);

    array2.Clear();

    EXPECT_EQ(array1.GetCount(), 7);
    EXPECT_EQ(array2.GetCount(), 0);

    array1.Append(&int2);

    EXPECT_EQ(array1.Find(&int2), 4);
    EXPECT_EQ(array1.Remove(4), true);
    EXPECT_EQ(array1.Find(&int2), 6);
    EXPECT_EQ(array1.Remove(6), true);
    EXPECT_EQ(array1.Find(&int2), -1);
    EXPECT_EQ(array1.Remove(6), false);

    array1.Append(&int8);
    array1.Append(&int9);
    array1.Append(&int10);

    EXPECT_EQ(*array1[0], int7);
    EXPECT_EQ(*array1[1], int1);
    EXPECT_EQ(*array1[2], int3);
    EXPECT_EQ(*array1[3], int6);
    EXPECT_EQ(*array1[4], int4);
    EXPECT_EQ(*array1[5], int5);
    EXPECT_EQ(*array1[6], int8);
    EXPECT_EQ(*array1[7], int9);
    EXPECT_EQ(*array1[8], int10);
    EXPECT_EQ(array1[9], nullptr);
}

TEST(SmartObjectArrayTest, Ctor) {
    SmartObjectArray<uint16_t> array1;
    SmartObjectArray<bool> array2(-1);
    SmartObjectArray<float> array3(0);
    SmartObjectArray<Point> array4(100);

    EXPECT_EQ(array1.GetCount(), 0);
    EXPECT_EQ(array2.GetCount(), 0);
    EXPECT_EQ(array3.GetCount(), 0);
    EXPECT_EQ(array4.GetCount(), 0);
}

TEST(SmartObjectArrayTest, CopyCtor) {
    SmartObjectArray<uint16_t> array1;
    SmartObjectArray<uint16_t> array2(array1);
    const uint16_t uint1{UINT16_MAX};
    const uint16_t uint2{0};
    const uint16_t uint3{INT16_MAX};

    array1.Insert(&uint1, UINT16_MAX);
    array1.Insert(&uint2, 0);

    EXPECT_EQ(array1.GetCount(), 2);
    EXPECT_EQ(array2.GetCount(), 2);

    SmartObjectArray<uint16_t> array3(array1, true);
    SmartObjectArray<uint16_t> array4(array2, false);

    EXPECT_EQ(array3.GetCount(), 2);
    EXPECT_EQ(array4.GetCount(), 2);

    array1.Clear();

    EXPECT_EQ(array1.GetCount(), 0);
    EXPECT_EQ(array2.GetCount(), 0);
    EXPECT_EQ(array3.GetCount(), 2);
    EXPECT_EQ(array4.GetCount(), 0);

    EXPECT_EQ(*array3[0], uint2);
    EXPECT_EQ(*array3[1], uint1);
};

TEST(SmartObjectArrayTest, MixedUse) {
    SmartObjectArray<int16_t> array;

    const int16_t int1{INT16_MIN};
    const int16_t int2{INT16_MAX};
    const int16_t int3{0};

    array.PushBack(&int1);
    array.Insert(&int2, 0);

    EXPECT_EQ(array.GetCount(), 2);
    EXPECT_EQ(*array[0], int2);
    EXPECT_EQ(*array[1], int1);

    EXPECT_EQ(array.Remove(2), false);
    EXPECT_EQ(array.Insert(&int3, 0), 0);
    EXPECT_EQ(array.Remove(0), true);

    EXPECT_EQ(array.GetCount(), 2);
    EXPECT_EQ(*array[0], int2);
    EXPECT_EQ(*array[1], int1);
}
