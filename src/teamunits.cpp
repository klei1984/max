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

TeamUnits::TeamUnits() : gold(0), hash_team_id(0), color_index_table(nullptr) {}

TeamUnits::~TeamUnits() {}

void TeamUnits::Init() {
    UnitAttributes attribs;

    for (int32_t id = 0; id < UNIT_END; ++id) {
        SmartPointer<UnitValues> unitvalues = new (std::nothrow) UnitValues();

        if (ResourceManager_GetUnitAttributes(id, &attribs)) {
            unitvalues->SetAttribute(ATTRIB_TURNS, attribs.turns_to_build);
            unitvalues->SetAttribute(ATTRIB_HITS, std::min(attribs.hit_points, static_cast<uint32_t>(UINT16_MAX)));
            unitvalues->SetAttribute(ATTRIB_ARMOR, attribs.armor_rating);
            unitvalues->SetAttribute(ATTRIB_ATTACK, attribs.attack_rating);
            unitvalues->SetAttribute(ATTRIB_MOVE_AND_FIRE, attribs.move_and_fire);
            unitvalues->SetAttribute(ATTRIB_SPEED,
                                     std::min(attribs.movement_points, static_cast<uint32_t>(UINT16_MAX)));
            unitvalues->SetAttribute(ATTRIB_FUEL, 0uL);
            unitvalues->SetAttribute(ATTRIB_RANGE, attribs.attack_range);
            unitvalues->SetAttribute(ATTRIB_ROUNDS, attribs.shots_per_turn);
            unitvalues->SetAttribute(ATTRIB_SCAN, attribs.scan_range);
            unitvalues->SetAttribute(ATTRIB_STORAGE, attribs.storage_capacity);
            unitvalues->SetAttribute(ATTRIB_AMMO, attribs.ammunition);
            unitvalues->SetAttribute(ATTRIB_ATTACK_RADIUS, attribs.blast_radius);

            SetBaseUnitValues(id, *unitvalues);

        } else {
            SDL_assert(0);
        }
    }
}

void TeamUnits::FileLoad(SmartFileReader& file) {
    if (file.GetFormat() == SmartFileFormat::V70) {
        uint16_t gold_v70;

        file.Read(gold_v70);

        gold = static_cast<uint32_t>(gold_v70);

    } else {
        file.Read(gold);
    }

    for (uint32_t unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        base_values[unit_id] = dynamic_cast<UnitValues*>(file.ReadObject());
    }

    for (uint32_t unit_id = 0; unit_id < UNIT_END; ++unit_id) {
        current_values[unit_id] = dynamic_cast<UnitValues*>(file.ReadObject());
    }

    complexes.Clear();

    uint32_t complex_count = file.ReadObjectCount();

    for (uint32_t i = 0; i < complex_count; ++i) {
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

uint32_t TeamUnits::GetGold() { return gold; }

void TeamUnits::SetGold(uint32_t value) { gold = value; }

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
