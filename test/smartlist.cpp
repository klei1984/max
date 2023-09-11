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

#include "smartlist.hpp"

#include <gtest/gtest.h>

#include "testobject.hpp"

class SmartListTest : public ::testing::Test {
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

        list1.PushBack(*to1);
        list1.PushBack(*to2);
        list1.PushBack(*to3);
        list1.PushFront(*to4);
        list1.PushFront(*to5);
        list1.PushFront(*to2);
    }

    void TearDown() override {}

public:
    SmartPointer<TestObject> to1;
    SmartPointer<TestObject> to2;
    SmartPointer<TestObject> to3;
    SmartPointer<TestObject> to4;
    SmartPointer<TestObject> to5;
    SmartList<TestObject> list1;
};

TEST_F(SmartListTest, Ctor) {
    SmartList<TestObject> list;

    EXPECT_EQ(list.GetCount(), 0);
    EXPECT_EQ(list.Begin(), list.End());
}

TEST_F(SmartListTest, CopyCtor) {
    SmartList<TestObject> list2(list1);

    EXPECT_EQ(list1.GetCount(), 6);
    EXPECT_EQ(list2.GetCount(), 6);
}

TEST_F(SmartListTest, PushOrder) {
    EXPECT_EQ(list1.GetCount(), 6);
    EXPECT_EQ(list1[0].Get(), 2);
    EXPECT_EQ(list1[1].Get(), 5);
    EXPECT_EQ(list1[2].Get(), 4);
    EXPECT_EQ(list1[3].Get(), 1);
    EXPECT_EQ(list1[4].Get(), 2);
    EXPECT_EQ(list1[5].Get(), 3);
}

TEST_F(SmartListTest, IteratorCreate) {
    SmartList<TestObject>::Iterator it1{list1.Begin()};
    SmartList<TestObject>::Iterator it2{list1.Begin()};
    SmartList<TestObject>::Iterator it3(it2);
    SmartList<TestObject>::Iterator it4;

    EXPECT_EQ(it1, it2);
    EXPECT_EQ(it1, it3);
    EXPECT_EQ(it4 == list1.End(), false);

    it4 = list1.End();

    EXPECT_EQ(it4 == list1.End(), true);
}

TEST_F(SmartListTest, IteratorUse) {
    const uint32_t results[]{2, 5, 4, 1, 2, 3};
    int32_t index{0};

    for (auto it{list1.Begin()}; it != list1.End(); ++it) {
        EXPECT_EQ(it.Get()->Get()->Get(), results[index++]);
    }

    index = 0;
    for (SmartList<TestObject>::Iterator it{list1.Begin()}; it != list1.End(); ++it) {
        EXPECT_EQ(it.Get()->Get()->Get(), results[index++]);
    }

    index = 0;
    for (SmartList<TestObject>::Iterator it{--list1.End()}; it != list1.End(); --it) {
        EXPECT_EQ(it.Get()->Get()->Get(), results[std::size(results) - ++index]);
    }

    index = 0;
    for (auto& it : list1) {
        EXPECT_EQ(it.Get(), results[index++]);
    }
}

TEST_F(SmartListTest, FindRemove) {
    SmartList<TestObject>::Iterator it1{list1.Find(*to2)};

    list1.Remove(it1);

    EXPECT_EQ(list1.GetCount(), 5);
    EXPECT_EQ(list1[0].Get(), 5);
    EXPECT_EQ(list1[1].Get(), 4);
    EXPECT_EQ(list1[2].Get(), 1);
    EXPECT_EQ(list1[3].Get(), 2);
    EXPECT_EQ(list1[4].Get(), 3);

    SmartList<TestObject>::Iterator it2{list1.Find(*to2)};

    list1.Remove(it2);

    EXPECT_EQ(list1.GetCount(), 4);
    EXPECT_EQ(list1[0].Get(), 5);
    EXPECT_EQ(list1[1].Get(), 4);
    EXPECT_EQ(list1[2].Get(), 1);
    EXPECT_EQ(list1[3].Get(), 3);

    SmartList<TestObject>::Iterator it3{list1.Find(*to2)};

    EXPECT_EQ(list1.GetCount(), 4);
    EXPECT_EQ(it3, list1.End());

    SmartList<TestObject>::Iterator it4{list1.Find(*to3)};

    bool result1 = list1.Remove(*it4);

    EXPECT_EQ(result1, true);
    EXPECT_EQ(list1.GetCount(), 3);
    EXPECT_EQ(list1[0].Get(), 5);
    EXPECT_EQ(list1[1].Get(), 4);
    EXPECT_EQ(list1[2].Get(), 1);

    bool result2 = list1.Remove(*it4);

    EXPECT_EQ(result2, false);
    EXPECT_EQ(list1.GetCount(), 3);
}

TEST_F(SmartListTest, IteratorInsertBefore) {
    SmartList<TestObject>::Iterator it{list1.Find(*to4)};

    list1.InsertBefore(it, *to5);

    EXPECT_EQ(list1.GetCount(), 7);
    EXPECT_EQ(list1[1].Get(), 5);
    EXPECT_EQ(list1[2].Get(), 5);
    EXPECT_EQ(list1[3].Get(), 4);
    EXPECT_EQ(list1[4].Get(), 1);
}

TEST_F(SmartListTest, IteratorInsertAfter) {
    SmartList<TestObject>::Iterator it{list1.Find(*to4)};

    list1.InsertAfter(it, *to5);

    EXPECT_EQ(list1.GetCount(), 7);
    EXPECT_EQ(list1[1].Get(), 5);
    EXPECT_EQ(list1[2].Get(), 4);
    EXPECT_EQ(list1[3].Get(), 5);
    EXPECT_EQ(list1[4].Get(), 1);
}

TEST_F(SmartListTest, IteratorAssign) {
    SmartList<TestObject> list2;

    EXPECT_EQ(list2.GetCount(), 0);

    list2.PushBack(*to1);

    EXPECT_EQ(list1.GetCount(), 6);
    EXPECT_EQ(list2.GetCount(), 1);

    list2 = list1;

    EXPECT_EQ(list2.GetCount(), 6);

    // self assignment corner case
    list2 = list2;

    EXPECT_EQ(list2.GetCount(), 6);
}

TEST_F(SmartListTest, IteratorRemoveAll) {
    SmartList<TestObject> list2;

    list2 = list1;

    EXPECT_EQ(list2.GetCount(), 6);

    for (auto it = list2.Begin(); it != list2.End(); ++it) {
        list2.Remove(it);
    }

    EXPECT_EQ(list2.GetCount(), 0);

    list2 = list1;

    EXPECT_EQ(list2.GetCount(), 6);

    for (auto it = list2.Begin(); it != list2.End(); ++it) {
        list2.Remove(*it);
    }

    EXPECT_EQ(list2.GetCount(), 0);
}

TEST_F(SmartListTest, InsertEnd) {
    SmartList<TestObject>::Iterator it_end = list1.End();

    list1.InsertAfter(it_end, *to5);

    EXPECT_EQ(list1.GetCount(), 7);
    EXPECT_EQ(list1[0].Get(), 5);
    EXPECT_EQ(list1[1].Get(), 2);
    EXPECT_EQ(list1[2].Get(), 5);
    EXPECT_EQ(list1[3].Get(), 4);
    EXPECT_EQ(list1[4].Get(), 1);
    EXPECT_EQ(list1[5].Get(), 2);
    EXPECT_EQ(list1[6].Get(), 3);

    list1.InsertBefore(it_end, *to3);

    EXPECT_EQ(list1.GetCount(), 8);
    EXPECT_EQ(list1[0].Get(), 5);
    EXPECT_EQ(list1[1].Get(), 2);
    EXPECT_EQ(list1[2].Get(), 5);
    EXPECT_EQ(list1[3].Get(), 4);
    EXPECT_EQ(list1[4].Get(), 1);
    EXPECT_EQ(list1[5].Get(), 2);
    EXPECT_EQ(list1[6].Get(), 3);
    EXPECT_EQ(list1[7].Get(), 3);

    list1.Clear();

    EXPECT_EQ(list1.GetCount(), 0);

    list1.InsertAfter(it_end, *to2);

    EXPECT_EQ(list1.GetCount(), 1);
    EXPECT_EQ(list1[0].Get(), 2);

    list1.Clear();

    EXPECT_EQ(list1.GetCount(), 0);

    list1.InsertBefore(it_end, *to3);

    EXPECT_EQ(list1.GetCount(), 1);
    EXPECT_EQ(list1[0].Get(), 3);

    list1.InsertBefore(it_end, *to2);

    EXPECT_EQ(list1.GetCount(), 2);
    EXPECT_EQ(list1[0].Get(), 3);
    EXPECT_EQ(list1[1].Get(), 2);
}

TEST_F(SmartListTest, Sort) {
    struct TestSortAscend {
        static bool Compare(const SmartList<TestObject>::Iterator& lhs, const SmartList<TestObject>::Iterator& rhs) {
            return lhs->Get()->Get() <= rhs->Get()->Get();
        }
    };

    struct TestSortDescend {
        static bool Compare(const SmartList<TestObject>::Iterator& lhs, const SmartList<TestObject>::Iterator& rhs) {
            return lhs->Get()->Get() >= rhs->Get()->Get();
        }
    };

    EXPECT_EQ(list1.GetCount(), 6);
    EXPECT_EQ(list1[0].Get(), 2);
    EXPECT_EQ(list1[1].Get(), 5);
    EXPECT_EQ(list1[2].Get(), 4);
    EXPECT_EQ(list1[3].Get(), 1);
    EXPECT_EQ(list1[4].Get(), 2);
    EXPECT_EQ(list1[5].Get(), 3);

    list1.Sort(TestSortAscend::Compare);

    EXPECT_EQ(list1.GetCount(), 6);
    EXPECT_EQ(list1[0].Get(), 1);
    EXPECT_EQ(list1[1].Get(), 2);
    EXPECT_EQ(list1[2].Get(), 2);
    EXPECT_EQ(list1[3].Get(), 3);
    EXPECT_EQ(list1[4].Get(), 4);
    EXPECT_EQ(list1[5].Get(), 5);

    list1.Sort(TestSortDescend::Compare);

    EXPECT_EQ(list1.GetCount(), 6);
    EXPECT_EQ(list1[0].Get(), 5);
    EXPECT_EQ(list1[1].Get(), 4);
    EXPECT_EQ(list1[2].Get(), 3);
    EXPECT_EQ(list1[3].Get(), 2);
    EXPECT_EQ(list1[4].Get(), 2);
    EXPECT_EQ(list1[5].Get(), 1);
}

TEST_F(SmartListTest, IteratorMisuse) {
    SmartList<TestObject> list2;
    SmartPointer<TestObject> to(new (std::nothrow) TestObject());

    list2.PushBack(*to);

    EXPECT_EQ(list2.GetCount(), 1);

    list2.Remove(list1.End());

    EXPECT_EQ(list1.GetCount(), 6);

    EXPECT_EQ(list1[0].Get(), 2);
    EXPECT_EQ(list1[1].Get(), 5);
    EXPECT_EQ(list1[2].Get(), 4);
    EXPECT_EQ(list1[3].Get(), 1);
    EXPECT_EQ(list1[4].Get(), 2);
    EXPECT_EQ(list1[5].Get(), 3);
}
