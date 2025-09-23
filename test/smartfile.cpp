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

#include "smartfile.hpp"

#include <gtest/gtest.h>

#include "point.hpp"
#include "registerarray.hpp"

static uint32_t TestSmartFileObject_TypeIndex{0};

class TestSmartFileObject : public FileObject {
    int8_t int8{0};
    uint8_t uint8{0};
    int16_t int16{0};
    uint16_t uint16{0};
    int32_t int32{0};
    uint32_t uint32a{0};
    uint32_t uint32b{0};
    Point point;

public:
    TestSmartFileObject() noexcept = default;
    ~TestSmartFileObject() noexcept = default;

    static FileObject* Allocate() noexcept;

    [[nodiscard]] uint32_t GetTypeIndex() const override { return TestSmartFileObject_TypeIndex; }
    void FileLoad(SmartFileReader& file) noexcept override {
        file.Read(int8);
        file.Read(uint8);
        file.Read(int16);
        file.Read(uint16);
        file.Read(int32);
        file.Read(uint32a);
        file.Read(uint32b);
        file.Read(point);
    }
    void FileSave(SmartFileWriter& file) noexcept override {
        file.Write(int8);
        file.Write(uint8);
        file.Write(int16);
        file.Write(uint16);
        file.Write(int32);
        file.Write(uint32a);
        file.Write(uint32b);
        file.Write(point);
    }

    int8_t GetInt8() const { return int8; }
    uint8_t GetUint8() const { return uint8; }
    int16_t GetInt16() const { return int16; }
    int32_t GetInt32() const { return int32; }
    uint16_t GetUint16() const { return uint16; }
    uint32_t GetUint32a() const { return uint32a; }
    uint32_t GetUint32b() const { return uint32b; }
    const Point& GetPoint() const { return point; }

    void SetInt8(int8_t _int8) { int8 = _int8; }
    void SetUint8(uint8_t _uint8) { uint8 = _uint8; }
    void SetInt16(int16_t _int16) { int16 = _int16; }
    void SetUint16(uint16_t _uint16) { uint16 = _uint16; }
    void SetInt32(int32_t _int32) { int32 = _int32; }
    void SetUint32a(uint32_t _uint32a) { uint32a = _uint32a; }
    void SetUint32b(uint32_t _uint32b) { uint32b = _uint32b; }
    void SetPoint(const Point& _point) { point = _point; }
};

FileObject* TestSmartFileObject::Allocate() noexcept { return new (std::nothrow) TestSmartFileObject(); }

class SmartFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        file_path = testing::TempDir() + "smartfile.dat";

        Test_ClassRegister = new (std::nothrow)
            RegisterClass("TestSmartFileObject", &TestSmartFileObject_TypeIndex, &TestSmartFileObject::Allocate);
    }

    void TearDown() override {
        Test_ClassRegister->GetRegister().Release();
        TestSmartFileObject_TypeIndex = 0;

        delete Test_ClassRegister;
    }

public:
    std::string file_path;
    RegisterClass* Test_ClassRegister;
};

TEST_F(SmartFileTest, ReadWrite) {
    uint32_t integer{123456};
    uint16_t count{111};
    SmartPointer<TestSmartFileObject> object = dynamic_cast<TestSmartFileObject*>(TestSmartFileObject::Allocate());

    object->SetInt8(INT8_MIN);
    object->SetUint8(UINT8_MAX);
    object->SetInt16(INT16_MIN);
    object->SetUint16(UINT16_MAX);
    object->SetInt32(INT32_MIN);
    object->SetUint32a(INT32_MAX);
    object->SetUint32b(UINT32_MAX);
    object->SetPoint(Point(-1, -1));

    SmartFileWriter writer;
    EXPECT_EQ(writer.Open(file_path.c_str()), true);
    EXPECT_EQ(writer.Write(integer), true);
    writer.WriteObjectCount(count);
    writer.WriteObject(object.Get());
    writer.WriteObject(object.Get());
    EXPECT_EQ(writer.Close(), true);

    integer = 0;
    count = 0;

    SmartFileReader reader;
    EXPECT_EQ(reader.Open(file_path.c_str()), true);
    EXPECT_EQ(reader.Read(integer), true);
    count = reader.ReadObjectCount();
    SmartPointer<TestSmartFileObject> object_readback = dynamic_cast<TestSmartFileObject*>(reader.ReadObject());
    SmartPointer<TestSmartFileObject> object_readback_copy = dynamic_cast<TestSmartFileObject*>(reader.ReadObject());
    EXPECT_EQ(reader.Close(), true);

    EXPECT_EQ(integer, 123456);
    EXPECT_EQ(count, 111);
    EXPECT_EQ(object_readback->GetInt8(), INT8_MIN);
    EXPECT_EQ(object_readback->GetUint8(), UINT8_MAX);
    EXPECT_EQ(object_readback->GetInt16(), INT16_MIN);
    EXPECT_EQ(object_readback->GetUint16(), UINT16_MAX);
    EXPECT_EQ(object_readback->GetInt32(), INT32_MIN);
    EXPECT_EQ(object_readback->GetUint32a(), INT32_MAX);
    EXPECT_EQ(object_readback->GetUint32b(), UINT32_MAX);
    EXPECT_EQ(object_readback->GetPoint(), Point(-1, -1));

    EXPECT_EQ(object_readback == object_readback_copy, true);
}
