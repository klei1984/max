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

#include "complex.hpp"

#include "registerarray.hpp"
#include "unitinfo.hpp"

Complex::Complex(short id) : material(0), fuel(0), gold(0), power(0), workers(0), buildings(0), id(id) {}

Complex::~Complex() {}

TextFileObject* Complex::Allocate() { return new (std::nothrow) Complex(0); }

short Complex::GetId() const { return id; }

static unsigned short Complex_TypeIndex;
static MAXRegisterClass Complex_ClassRegister("Complex", &Complex_TypeIndex, &Complex::Allocate);

unsigned short Complex::GetTypeIndex() const { return Complex_TypeIndex; }

void Complex::FileLoad(SmartFileReader& file) {
    file.Read(material);
    file.Read(fuel);
    file.Read(gold);
    file.Read(power);
    file.Read(workers);
    file.Read(buildings);
    file.Read(id);
}

void Complex::FileSave(SmartFileWriter& file) {
    file.Write(material);
    file.Write(fuel);
    file.Write(gold);
    file.Write(power);
    file.Write(workers);
    file.Write(buildings);
    file.Write(id);
}

void Complex::TextLoad(TextStructure& object) {
    material = object.ReadInt("material");
    fuel = object.ReadInt("fuel");
    gold = object.ReadInt("gold");
    power = object.ReadInt("power");
    workers = object.ReadInt("workers");
    buildings = object.ReadInt("buildings");
    id = object.ReadInt("id");
}

void Complex::TextSave(SmartTextfileWriter& file) {
    file.WriteInt("material", material);
    file.WriteInt("fuel", fuel);
    file.WriteInt("gold", gold);
    file.WriteInt("power", power);
    file.WriteInt("workers", workers);
    file.WriteInt("buildings", buildings);
    file.WriteInt("id", id);
}

int Complex::WritePacket(void* buffer) {
    /// \todo Fill network packet with complex data

    ((short*)buffer)[0] = material;
    ((short*)buffer)[1] = fuel;
    ((short*)buffer)[2] = gold;
    ((short*)buffer)[3] = power;
    ((short*)buffer)[4] = workers;
    ((short*)buffer)[5] = buildings;
    ((short*)buffer)[6] = id;

    return 12;
}

void Complex::ReadPacket(void* buffer) {
    /// \todo Update complex from network packet

    material = ((short*)buffer)[0];
    fuel = ((short*)buffer)[1];
    gold = ((short*)buffer)[2];
    power = ((short*)buffer)[3];
    workers = ((short*)buffer)[4];
    buildings = ((short*)buffer)[5];
    id = ((short*)buffer)[6];
}

void Complex::AddBuilding(UnitInfo& unit) { ++buildings; }

void Complex::RemoveBuilding(UnitInfo& unit) {
    --buildings;

    if (0 == buildings) {
        /// \todo Remove complex from CTinfo)
    }
}
