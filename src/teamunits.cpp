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

#include <SDL.h>

#include <cctype>
#include <cstdlib>

#include "access.hpp"
#include "inifile.hpp"
#include "net_packet.hpp"
#include "production_manager.hpp"
#include "researchmenu.hpp"
#include "resource_manager.hpp"
#include "units_manager.hpp"
#include "upgradecontrol.hpp"

AbstractUnit::AbstractUnit(unsigned int flags, ResourceID sprite, ResourceID shadows, ResourceID data, ResourceID flics,
                           ResourceID portrait, ResourceID icon, ResourceID armory_portrait, ResourceID field_18,
                           unsigned char land_type, unsigned char cargo_type, char new_gender,
                           const char* singular_name, const char* plural_name, const char* description,
                           const char* tutorial)
    : flags(flags),
      sprite(sprite),
      shadows(shadows),
      data(data),
      flics(flics),
      portrait(portrait),
      icon(icon),
      armory_portrait(armory_portrait),
      field_18(field_18),
      cargo_type(cargo_type),
      land_type(land_type),
      singular_name(singular_name),
      plural_name(plural_name),
      description(description),
      tutorial(tutorial),
      field_6(0) {
    new_gender = toupper(new_gender);
    switch (new_gender) {
        case 'F':
            gender = 2;
            break;
        case 'M':
            gender = 1;
            break;
        case 'N':
            gender = 0;
            break;
        default:
            SDL_Log("Incorrect gender for %s", singular_name);
            SDL_assert(new_gender == 'M' || new_gender == 'F' || new_gender == 'N');
    }
}

BaseUnit::BaseUnit() : sprite(nullptr), shadows(nullptr), field_47(nullptr) {}

void BaseUnit::Init(AbstractUnit* unit) {
    sprite = nullptr;
    shadows = nullptr;
    field_47 = nullptr;

    data = unit->data;

    data_buffer = ResourceManager_LoadResource(data);

    flags = unit->flags;
    flics = unit->flics;
    portrait = unit->portrait;
    icon = unit->icon;
    armory_portrait = unit->armory_portrait;
    field_18 = unit->field_18;

    land_type = unit->land_type;
    cargo_type = unit->cargo_type;
    gender = unit->gender;

    singular_name = unit->singular_name;
    plural_name = unit->plural_name;
    description = unit->description;
    tutorial = unit->tutorial;
}

int TeamUnits::GetParam(char* string, int* offset) {
    int number;

    while (string[*offset] == ' ') {
        ++*offset;
    }

    number = strtol(&string[*offset], NULL, 10);

    while (string[*offset] != ' ' && string[*offset] != '\0') {
        ++*offset;
    }

    return number;
}

TeamUnits::TeamUnits() : gold(0), hash_team_id(0), color_index_table(0) {}

TeamUnits::~TeamUnits() {}

void TeamUnits::Init() {
    char* attribs;

    attribs = reinterpret_cast<char*>(ResourceManager_ReadResource(ATTRIBS));

    if (attribs) {
        unsigned int file_size;
        unsigned int file_pos;

        file_size = ResourceManager_GetResourceSize(ATTRIBS);
        file_pos = 0;

        for (int id = 0; id < UNIT_END;) {
            char buffer[100];
            int position;

            position = 0;

            do {
                buffer[position] = attribs[file_pos++];

                if (buffer[position] == '\r') {
                    --position;
                }

                if (!--file_size) {
                    break;
                }

            } while (buffer[position++] != '\n');
            buffer[position] = '\0';

            if (buffer[0] != ';' && buffer[0] != '\n') {
                SmartPointer<UnitValues> unitvalues = new (std::nothrow) UnitValues();

                position = 0;
                unitvalues->SetAttribute(ATTRIB_TURNS, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_HITS, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_ARMOR, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_ATTACK, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_MOVE_AND_FIRE, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_SPEED, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_FUEL, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_RANGE, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_ROUNDS, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_SCAN, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_STORAGE, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_AMMO, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_ATTACK_RADIUS, GetParam(buffer, &position));

                SetBaseUnitValues(id, *unitvalues);
                ++id;
            }
        }

        delete[] attribs;
    }
}

void TeamUnits::FileLoad(SmartFileReader& file) {
    file.Read(gold);

    for (int unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        base_values[unit_id] = dynamic_cast<UnitValues*>(file.ReadObject());
    }

    for (int unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        current_values[unit_id] = dynamic_cast<UnitValues*>(file.ReadObject());
    }

    complexes.Clear();

    int complex_count = file.ReadObjectCount();

    for (int i = 0; i < complex_count; ++i) {
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
    file.WriteInt("gold", gold);

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

void TeamUnits::WriteComplexPacket(unsigned short complex_id, NetPacket& packet) {
    packet << complex_id;

    GetComplex(complex_id)->WritePacket(packet);
}

void TeamUnits::ReadComplexPacket(NetPacket& packet) {
    unsigned short complex_id;

    packet >> complex_id;

    GetComplex(complex_id)->ReadPacket(packet);
}

unsigned short TeamUnits::GetGold() { return gold; }

void TeamUnits::SetGold(unsigned short value) { gold = value; }

Complex* TeamUnits::CreateComplex() {
    unsigned short complex_id;
    Complex* result;
    SmartList<Complex>::Iterator it;

    complex_id = 1;
    it = complexes.Begin();

    for (; it != complexes.End() && (*it).GetId() == complex_id; ++it, ++complex_id) {
    }

    result = new (std::nothrow) Complex(complex_id);
    complexes.InsertAfter(it, *result);

    return result;
}

SmartList<Complex>::Iterator TeamUnits::GetFrontComplex() { return complexes.Begin(); }

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

void TeamUnits::OptimizeComplexes(unsigned short team) {
    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        Access_UpdateResourcesTotal(&*it);
        ProductionManager_ManageMining(team, &*it, nullptr, false);
    }
}

void TeamUnits::RemoveComplex(Complex& object) { complexes.Remove(object); }

void TeamUnits::ClearComplexes() { complexes.Clear(); }

UnitValues* TeamUnits::GetBaseUnitValues(unsigned short id) { return &*base_values[id]; }

void TeamUnits::SetBaseUnitValues(unsigned short id, UnitValues& object) { base_values[id] = object; }

UnitValues* TeamUnits::GetCurrentUnitValues(unsigned short id) { return &*current_values[id]; }

void TeamUnits::SetCurrentUnitValues(unsigned short id, UnitValues& object) { current_values[id] = object; }

int TeamUnits_GetUpgradeCost(unsigned short team, ResourceID unit_type, int attribute) {
    SmartPointer<UnitValues> base_values(UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type));
    SmartPointer<UnitValues> current_values(UnitsManager_TeamInfo[team].team_units->GetCurrentUnitValues(unit_type));
    int upgrade_offset_factor = TeamUnits_UpgradeOffsetFactor(team, unit_type, attribute);
    int factor;
    int base_value;
    int current_value;
    int upgrade_cost;

    switch (attribute) {
        case ATTRIB_ATTACK: {
            factor = ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_ATTACK, unit_type);
        } break;

        case ATTRIB_ROUNDS: {
            factor = ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_SHOTS, unit_type);
        } break;

        case ATTRIB_RANGE: {
            factor = ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_RANGE, unit_type);
        } break;

        case ATTRIB_ARMOR: {
            factor = ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_ARMOR, unit_type);
        } break;

        case ATTRIB_HITS: {
            factor = ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_HITS, unit_type);
        } break;

        case ATTRIB_SPEED: {
            factor = ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_SPEED, unit_type);
        } break;

        case ATTRIB_SCAN: {
            factor = ResearchMenu_CalculateFactor(team, RESEARCH_TOPIC_SCAN, unit_type);
        } break;

        default: {
            factor = 0;
        } break;
    }

    base_value = base_values->GetAttribute(attribute);
    current_value = current_values->GetAttribute(attribute);

    upgrade_cost = UpgradeControl_CalculateCost(attribute, current_value, factor, base_value);

    if (attribute == ATTRIB_TURNS) {
        current_value -= upgrade_offset_factor;

    } else {
        current_value += upgrade_offset_factor;
    }

    upgrade_cost =
        UpgradeControl_CalculateCost(attribute, current_value, factor, base_values->GetAttribute(attribute)) -
        upgrade_cost;

    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        switch (ini_get_setting(INI_OPPONENT)) {
            case OPPONENT_TYPE_MASTER: {
                upgrade_cost = (upgrade_cost * 4) / 5;
            } break;

            case OPPONENT_TYPE_GOD: {
                upgrade_cost = (upgrade_cost * 2) / 3;
            } break;

            case OPPONENT_TYPE_CLUELESS: {
                upgrade_cost = (upgrade_cost * 5) / 4;
            } break;
        }
    }

    if (upgrade_cost < 1) {
        upgrade_cost = 1;
    }

    return upgrade_cost;
}

int TeamUnits_UpgradeOffsetFactor(unsigned short team, ResourceID unit_type, int attribute) {
    int value;
    int result;

    value = UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type)->GetAttribute(attribute);

    if (value < 10) {
        result = 1;
    } else if (value < 25) {
        result = 2;
    } else if (value < 50) {
        result = 5;
    } else {
        result = 10;
    }

    return result;
}
