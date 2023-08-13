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

#include "registerarray.hpp"

#include <gtest/gtest.h>

class TestFileObject : public FileObject {
public:
    TestFileObject() noexcept = default;
    ~TestFileObject() noexcept = default;
    static FileObject* Allocate() noexcept;
    [[nodiscard]] uint16_t GetTypeIndex() const noexcept override { return 0; }
    void FileLoad(SmartFileReader& file) noexcept override {}
    void FileSave(SmartFileWriter& file) noexcept override {}
};

FileObject* TestFileObject::Allocate() noexcept { return new (std::nothrow) TestFileObject(); }

class RegisterArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        Test_ClassRegister1 =
            new (std::nothrow) RegisterClass("Mechanized", &Test_TypeIndex1, &TestFileObject::Allocate);
        Test_ClassRegister2 = new (std::nothrow) RegisterClass("Assault", &Test_TypeIndex2, &TestFileObject::Allocate);
        Test_ClassRegister3 =
            new (std::nothrow) RegisterClass("Exploration", &Test_TypeIndex3, &TestFileObject::Allocate);
    }

    void TearDown() override {
        Test_ClassRegister1->GetRegister().Release();

        delete Test_ClassRegister1;
        delete Test_ClassRegister2;
        delete Test_ClassRegister3;
    }

public:
    uint16_t Test_TypeIndex1{0};
    uint16_t Test_TypeIndex2{0};
    uint16_t Test_TypeIndex3{0};

    RegisterClass* Test_ClassRegister1;
    RegisterClass* Test_ClassRegister2;
    RegisterClass* Test_ClassRegister3;
};

TEST_F(RegisterArrayTest, Interfaces) {
    EXPECT_EQ(Test_TypeIndex1, 3);
    EXPECT_EQ(Test_TypeIndex2, 1);
    EXPECT_EQ(Test_TypeIndex3, 2);

    EXPECT_EQ(Test_ClassRegister1->GetRegister().GetCount(), 3);
    EXPECT_STREQ(Test_ClassRegister1->GetRegister()[0].GetClassName(), "Assault");

    EXPECT_EQ(Test_ClassRegister2->GetRegister().GetCount(), 3);
    EXPECT_STREQ(Test_ClassRegister2->GetRegister()[1].GetClassName(), "Exploration");

    EXPECT_EQ(Test_ClassRegister3->GetRegister().GetCount(), 3);
    EXPECT_STREQ(Test_ClassRegister3->GetRegister()[2].GetClassName(), "Mechanized");
}
