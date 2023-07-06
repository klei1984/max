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

#include "smartpointer.hpp"

#include <gtest/gtest.h>

enum {
    TEST_CLASS_UNDEFINED,
    TEST_CLASS_CONSTRUCTED,
    TEST_CLASS_DESTRUCTED,
};

class TestObject : public SmartObject {
    uint32_t index{0};
    uint32_t* status{nullptr};

public:
    TestObject() = default;

    TestObject(uint32_t* _status) : status(_status) {
        if (status) {
            *status = TEST_CLASS_CONSTRUCTED;
        }
    }

    ~TestObject() {
        if (status) {
            *status = TEST_CLASS_DESTRUCTED;
        }
    }

    uint32_t Get() const noexcept { return index; }

    void Set(uint32_t idx) noexcept { index = idx; }
};

TEST(SmartPointer, ctor) {
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

TEST(SmartPointer, assign) {
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

TEST(SmartPointer, dtor) {
    uint32_t status{TEST_CLASS_UNDEFINED};

    {
        TestObject* obj = new (std::nothrow) TestObject(&status);
        SmartPointer<TestObject> sp1(obj);

        EXPECT_EQ(sp1.Get(), obj);
        EXPECT_EQ(status, TEST_CLASS_CONSTRUCTED);
    }

    EXPECT_EQ(status, TEST_CLASS_DESTRUCTED);
}

TEST(SmartPointer, shared) {
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
