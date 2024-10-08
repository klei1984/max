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
#include "ailog.hpp"
#include "inifile.hpp"
#include "net_packet.hpp"
#include "production_manager.hpp"
#include "researchmenu.hpp"
#include "resource_manager.hpp"
#include "units_manager.hpp"
#include "upgradecontrol.hpp"

AbstractUnit::AbstractUnit(uint32_t flags, ResourceID sprite, ResourceID shadows, ResourceID data, ResourceID flics,
                           ResourceID portrait, ResourceID icon, ResourceID armory_portrait, ResourceID field_18,
                           uint8_t land_type, uint8_t cargo_type, char new_gender, const char* singular_name,
                           const char* plural_name, const char* description, const char* tutorial)
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
            AiLog log("Incorrect gender for %s", singular_name);
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

int32_t TeamUnits::GetParam(char* string, int32_t* offset) {
    int32_t number;

    while (string[*offset] == ' ') {
        ++*offset;
    }

    number = strtol(&string[*offset], nullptr, 10);

    while (string[*offset] != ' ' && string[*offset] != '\0') {
        ++*offset;
    }

    return number;
}

TeamUnits::TeamUnits() : gold(0), hash_team_id(0), color_index_table(nullptr) {}

TeamUnits::~TeamUnits() {}

void TeamUnits::Init() {
    char* attribs;

    attribs = reinterpret_cast<char*>(ResourceManager_ReadResource(ATTRIBS));

    if (attribs) {
        uint32_t file_size;
        uint32_t file_pos;

        file_size = ResourceManager_GetResourceSize(ATTRIBS);
        file_pos = 0;

        for (int32_t id = 0; id < UNIT_END;) {
            char buffer[100];
            int32_t position;

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
                unitvalues->SetAttribute(ATTRIB_HITS, std::min(GetParam(buffer, &position), UINT8_MAX));
                unitvalues->SetAttribute(ATTRIB_ARMOR, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_ATTACK, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_MOVE_AND_FIRE, GetParam(buffer, &position));
                unitvalues->SetAttribute(ATTRIB_SPEED, std::min(GetParam(buffer, &position), UINT8_MAX));
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

    for (int32_t unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        base_values[unit_id] = dynamic_cast<UnitValues*>(file.ReadObject());
    }

    for (int32_t unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        current_values[unit_id] = dynamic_cast<UnitValues*>(file.ReadObject());
    }

    complexes.Clear();

    int32_t complex_count = file.ReadObjectCount();

    for (int32_t i = 0; i < complex_count; ++i) {
        complexes.PushBack(*dynamic_cast<Complex*>(file.ReadObject()));
    }
}

void TeamUnits::FileSave(SmartFileWriter& file) {
    file.Write(gold);

    for (int32_t unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        file.WriteObject(&*(base_values[unit_id]));
    }

    for (int32_t unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        file.WriteObject(&*(current_values[unit_id]));
    }

    file.WriteObjectCount(complexes.GetCount());

    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        file.WriteObject(&*it);
    }
}

void TeamUnits::WriteComplexPacket(uint16_t complex_id, NetPacket& packet) {
    packet << complex_id;

    GetComplex(complex_id)->WritePacket(packet);
}

void TeamUnits::ReadComplexPacket(NetPacket& packet) {
    uint16_t complex_id;

    packet >> complex_id;

    GetComplex(complex_id)->ReadPacket(packet);
}

uint16_t TeamUnits::GetGold() { return gold; }

void TeamUnits::SetGold(uint16_t value) { gold = value; }

Complex* TeamUnits::CreateComplex() {
    uint16_t complex_id;
    Complex* result;
    SmartList<Complex>::Iterator it;

    complex_id = 1;
    it = complexes.Begin();

    for (; it != complexes.End() && (*it).GetId() == complex_id; ++it, ++complex_id) {
    }

    result = new (std::nothrow) Complex(complex_id);
    complexes.InsertBefore(it, *result);

    return result;
}

SmartList<Complex>& TeamUnits::GetComplexes() { return complexes; }

Complex* TeamUnits::GetComplex(uint16_t complex_id) {
    Complex* result = nullptr;

    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        if ((*it).GetId() == complex_id) {
            result = &*it;
            break;
        }
    }

    return result;
}

void TeamUnits::OptimizeComplexes(uint16_t team) {
    for (SmartList<Complex>::Iterator it = complexes.Begin(); it != complexes.End(); ++it) {
        Access_UpdateResourcesTotal(&*it);
        ProductionManager_OptimizeProduction(team, &*it, nullptr, false);
    }
}

void TeamUnits::RemoveComplex(Complex& object) { complexes.Remove(object); }

void TeamUnits::ClearComplexes() { complexes.Clear(); }

UnitValues* TeamUnits::GetBaseUnitValues(uint16_t id) { return &*base_values[id]; }

void TeamUnits::SetBaseUnitValues(uint16_t id, UnitValues& object) { base_values[id] = object; }

UnitValues* TeamUnits::GetCurrentUnitValues(uint16_t id) { return &*current_values[id]; }

void TeamUnits::SetCurrentUnitValues(uint16_t id, UnitValues& object) { current_values[id] = object; }

int32_t TeamUnits_GetUpgradeCost(uint16_t team, ResourceID unit_type, int32_t attribute) {
    SmartPointer<UnitValues> base_values(UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type));
    SmartPointer<UnitValues> current_values(UnitsManager_TeamInfo[team].team_units->GetCurrentUnitValues(unit_type));
    int32_t upgrade_offset_factor = TeamUnits_UpgradeOffsetFactor(team, unit_type, attribute);
    int32_t factor;
    int32_t base_value;
    int32_t current_value;
    int32_t upgrade_cost;

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

int32_t TeamUnits_UpgradeOffsetFactor(uint16_t team, ResourceID unit_type, int32_t attribute) {
    int32_t value;
    int32_t result;

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
