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

#include <gtest/gtest.h>

#include "testobject.hpp"

TEST(SmartPointer, Ctor) {
    SmartPointer<SmartObject> sp1(new (std::nothrow) SmartObject());
    SmartPointer<SmartObject> sp2;
    SmartPointer<SmartObject> sp3(sp1);

    EXPECT_NE(sp1.Get(), nullptr);
    EXPECT_NE(&*sp1, nullptr);
    EXPECT_EQ(sp2.Get(), nullptr);
    EXPECT_EQ(&*sp2, nullptr);
    EXPECT_NE(sp3.Get(), nullptr);
    EXPECT_EQ(sp3.Get(), sp1.Get());
}

TEST(SmartPointer, Assign) {
    SmartPointer<SmartObject> sp1(new (std::nothrow) SmartObject());
    SmartPointer<SmartObject> sp2 = sp1;
    SmartPointer<SmartObject> sp3 = sp1.Get();
    SmartPointer<SmartObject> sp4 = *sp1.Get();
    SmartPointer<SmartObject> sp5;
    SmartPointer<SmartObject> sp6 = nullptr;
    SmartPointer<SmartObject> sp7(nullptr);

    EXPECT_EQ(sp2.Get(), sp1.Get());
    EXPECT_EQ(sp3.Get(), sp1.Get());
    EXPECT_EQ(sp4.Get(), sp1.Get());
    EXPECT_EQ(sp5.Get(), nullptr);
    EXPECT_EQ(sp6.Get(), nullptr);

    sp5 = sp1;
    sp6 = sp1.Get();
    sp7 = *sp1.Get();

    EXPECT_EQ(sp5.Get(), sp1.Get());
    EXPECT_EQ(sp6.Get(), sp1.Get());
    EXPECT_EQ(sp7.Get(), sp1.Get());
}

TEST(SmartPointer, Dtor) {
    uint32_t status{TEST_CLASS_UNDEFINED};

    {
        TestObject* obj = new (std::nothrow) TestObject(&status);
        SmartPointer<TestObject> sp1(obj);

        EXPECT_EQ(sp1.Get(), obj);
        EXPECT_EQ(status, TEST_CLASS_CONSTRUCTED);
    }

    EXPECT_EQ(status, TEST_CLASS_DESTRUCTED);
}

TEST(SmartPointer, Shared) {
    uint32_t status{TEST_CLASS_UNDEFINED};
    TestObject* obj = new (std::nothrow) TestObject(&status);
    SmartPointer<TestObject> sp1(obj);

    EXPECT_EQ(sp1.Get(), obj);
    EXPECT_EQ(status, TEST_CLASS_CONSTRUCTED);

    {
        SmartPointer<TestObject> sp2(sp1);

        EXPECT_EQ(sp2.Get(), obj);
        EXPECT_EQ(status, TEST_CLASS_CONSTRUCTED);
    }

    EXPECT_EQ(status, TEST_CLASS_CONSTRUCTED);

    sp1 = nullptr;

    EXPECT_EQ(status, TEST_CLASS_DESTRUCTED);
}

TEST(SmartPointer, Compare) {
    TestObject* obj1 = new (std::nothrow) TestObject();
    TestObject* obj2 = new (std::nothrow) TestObject();

    SmartPointer<TestObject> sp1(obj1);
    SmartPointer<TestObject> sp2(obj2);
    SmartPointer<TestObject> sp3;

    obj1->Set(1);
    obj2->Set(2);

    EXPECT_EQ(sp1 == sp2, false);
    EXPECT_EQ(sp1 != sp2, true);
    EXPECT_EQ(sp1.Get() == sp2, false);
    EXPECT_EQ(sp1.Get() != sp2, true);
    EXPECT_EQ(sp1 == sp2.Get(), false);
    EXPECT_EQ(sp1 != sp2.Get(), true);
    EXPECT_EQ(sp1 == nullptr, false);
    EXPECT_EQ(sp1 != nullptr, true);
    EXPECT_EQ(nullptr == sp1, false);
    EXPECT_EQ(nullptr != sp1, true);
    EXPECT_EQ(nullptr == sp3, true);
    EXPECT_EQ(sp1 == sp2.Get(), false);
    EXPECT_EQ(sp1 != sp2.Get(), true);
    EXPECT_EQ(*sp1.Get() == sp2, false);
    EXPECT_EQ(*sp1.Get() != sp2, true);
    EXPECT_EQ(sp1 == *sp2.Get(), false);
    EXPECT_EQ(sp1 != *sp2.Get(), true);

    EXPECT_NE(sp1, sp2);
    EXPECT_NE(sp1.Get(), sp2);
    EXPECT_NE(sp1, sp2.Get());
    EXPECT_NE(sp1, nullptr);
    EXPECT_NE(nullptr, sp1);
    EXPECT_EQ(nullptr, sp3);
    EXPECT_NE(*sp1.Get(), sp2);
    EXPECT_NE(sp1, *sp2.Get());

    if (sp1) {
        SUCCEED() << "if(sp1){}";
    }

    if (!sp3) {
        SUCCEED() << "if(!sp3){}";
    }
}
