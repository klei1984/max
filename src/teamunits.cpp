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

#include "teamunits.hpp"

TeamUnits::TeamUnits() : gold(0), color_field_0(0), color_field_2(0) {}

TeamUnits::~TeamUnits() {}

void TeamUnits::FileLoad(SmartFileReader& file) {
    file.Read(gold);

    for (int unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        base_values[unit_id] = dynamic_cast<UnitValues*>(file.ReadObject());
    }

    for (int unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        current_values[unit_id] = dynamic_cast<UnitValues*>(file.ReadObject());
    }

    complexes.Clear();

    for (int complex_count = file.ReadObjectCount(); complex_count > 0; --complex_count) {
        complexes.PushBack(*dynamic_cast<Complex*>(file.ReadObject()));
    }
}

void TeamUnits::FileSave(SmartFileWriter& file) {
    file.Write(gold);

    for (int unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        file.WriteObject(&*(base_values[unit_id]));
    }

    for (int unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        file.WriteObject(&*(current_values[unit_id]));
    }

    file.WriteObjectCount(complexes.GetCount());

    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        file.WriteObject(&*it);
    }
}

void TeamUnits::TextLoad(TextStructure& object) {
    gold = object.ReadInt("gold");

    SmartPointer<TextStructure> bv_reader = object.ReadStructure("base_values");

    if (bv_reader != nullptr) {
        for (SmartPointer<TextStructure> unit = bv_reader->ReadStructure("unit"); unit != nullptr;
             unit = bv_reader->ReadStructure("unit")) {
            unsigned short unit_type;

            if (unit->GetField("type", Enums_UnitType, &unit_type)) {
                base_values[unit_type] = dynamic_cast<UnitValues*>(&*unit->ReadPointer("values"));
            }
        }
    }

    SmartPointer<TextStructure> cv_reader = object.ReadStructure("current_values");

    if (cv_reader != nullptr) {
        for (SmartPointer<TextStructure> unit = cv_reader->ReadStructure("unit"); unit != nullptr;
             unit = cv_reader->ReadStructure("unit")) {
            unsigned short unit_type;

            if (unit->GetField("type", Enums_UnitType, &unit_type)) {
                base_values[unit_type] = dynamic_cast<UnitValues*>(&*unit->ReadPointer("values"));
            }
        }
    }

    complexes.Clear();

    SmartPointer<TextStructure> cp_reader = object.ReadStructure("complexes");

    if (cp_reader != nullptr) {
        for (SmartPointer<TextItem> complex = cp_reader->Find("complex"); complex != nullptr;
             complex = cp_reader->Find("complex")) {
            complexes.PushBack(*dynamic_cast<Complex*>(complex->GetPointer()));
        }
    }
}

void TeamUnits::TextSave(SmartTextfileWriter& file) {
    file.Write(gold);

    file.WriteIdentifier("base_values");
    for (int unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        file.WriteIdentifier("unit");
        file.WriteEnum("type", Enums_UnitType, unit_id);
        file.WritePointer("values", &*base_values[unit_id]);
        file.WriteDelimiter();
    }
    file.WriteDelimiter();

    file.WriteIdentifier("current_values");
    for (int unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        file.WriteIdentifier("unit");
        file.WriteEnum("type", Enums_UnitType, unit_id);
        file.WritePointer("values", &*current_values[unit_id]);
        file.WriteDelimiter();
    }
    file.WriteDelimiter();

    file.WriteIdentifier("complexes");
    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        file.WritePointer("complex", &*it);
    }
    file.WriteDelimiter();
}

int TeamUnits::WriteComplexPacket(unsigned short complex_id, void* buffer) {
    /// \todo Clean up network packet code
    struct __attribute__((packed)) packet {
        unsigned short complex_id;
        void* complex;
    }* packet = (struct packet*)buffer;

    packet->complex_id = complex_id;
    int packet_size = GetComplex(complex_id)->WritePacket(&packet->complex) + 2;

    return packet_size;
}

void TeamUnits::ReadComplexPacket(void* buffer) {
    /// \todo Clean up network packet code
    struct __attribute__((packed)) packet {
        unsigned short complex_id;
        void* complex;
    }* packet = (struct packet*)buffer;

    GetComplex(packet->complex_id)->ReadPacket(&packet->complex);
}

unsigned short TeamUnits::GetGold() { return gold; }

void TeamUnits::SetGold(unsigned short value) { gold = value; }

Complex* TeamUnits::CreateComplex() {
    unsigned short complex_id = 0;
    Complex* result;

    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        if ((*it).GetId() == complex_id) {
            ++complex_id;
        } else {
            result = new (std::nothrow) Complex(complex_id);
            complexes.InsertAfter(it, *result);
        }
    }

    return result;
}

Complex* TeamUnits::GetComplex(unsigned short complex_id) {
    Complex* result = nullptr;

    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        if ((*it).GetId() == complex_id) {
            result = &*it;
            break;
        }
    }

    return result;
}

void TeamUnits::sub_7F3DC(unsigned short team) {
    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        /// \todo Implement methods
        //        (*it).Complex_sub_135AC();
        //        sub_41EA3(team, (*it), 0, 0);
    }
}

void TeamUnits::RemoveComplex(Complex& object) { complexes.Remove(object); }

UnitValues* TeamUnits::GetBaseUnitValues(unsigned short id) { return &*base_values[id]; }

UnitValues* TeamUnits::GetCurrentUnitValues(unsigned short id) { return &*current_values[id]; }

void TeamUnits::SetCurrentUnitValues(unsigned short id, UnitValues& object) { current_values[id] = object; }
