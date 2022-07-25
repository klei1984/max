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
#include "survey.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"

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

void Complex::GetCargoMinable(Cargo& capacity) {
    capacity.Init();

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetComplex() == this && (*it).orders != ORDER_POWER_OFF && (*it).orders != ORDER_DISABLE &&
            (*it).orders != ORDER_IDLE) {
            Cargo cargo;

            Cargo_GetCargoDemand(&*it, &cargo);

            if ((*it).unit_type == MININGST) {
                cargo.gold -= (*it).gold_mining;
                cargo.raw -= (*it).raw_mining;
                cargo.fuel -= (*it).fuel_mining;
            }

            capacity.gold -= cargo.gold;
            capacity.raw -= cargo.raw;
            capacity.fuel -= cargo.fuel;
        }
    }
}

void Complex::GetCargoMining(Cargo& materials, Cargo& capacity) {
    short cargo_raw;
    short cargo_fuel;
    short cargo_gold;

    materials.Init();
    capacity.Init();

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetComplex() == this && (*it).unit_type == MININGST && (*it).orders != ORDER_POWER_OFF &&
            (*it).orders != ORDER_DISABLE && (*it).orders != ORDER_IDLE) {
            materials.gold += (*it).gold_mining;
            materials.raw += (*it).raw_mining;
            materials.fuel += (*it).fuel_mining;
            materials.free_capacity += 16 - ((*it).gold_mining + (*it).raw_mining + (*it).fuel_mining);

            Survey_GetResourcesInArea((*it).grid_x, (*it).grid_y, 1, 16, &cargo_raw, &cargo_gold, &cargo_fuel, true,
                                      (*it).team);

            capacity.raw += cargo_raw;
            capacity.fuel += cargo_fuel;
            capacity.gold += cargo_gold;
        }
    }
}

void Complex::GetCargoInfo(Cargo& materials, Cargo& capacity) {
    Cargo cargo;

    materials.Init();
    capacity.Init();

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).GetComplex() == this) {
            materials += *Cargo_GetCargo(&*it, &cargo);
            capacity += *Cargo_GetCargoCapacity(&*it, &cargo);
        }
    }
}

void Complex::Grow(UnitInfo& unit) { ++buildings; }

void Complex::Shrink(UnitInfo& unit) {
    --buildings;

    if (!buildings) {
        UnitsManager_TeamInfo[unit.team].team_units->RemoveComplex(*this);
    }
}
