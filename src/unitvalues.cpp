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

#include "unitvalues.hpp"

#include <SDL.h>

#include "registerarray.hpp"

UnitValues::UnitValues()
    : turns(0),
      hits(0),
      armor(0),
      attack(0),
      speed(0),
      fuel(0),
      range(0),
      rounds(0),
      move_and_fire(0),
      scan(0),
      storage(0),
      ammo(0),
      attack_radius(0),
      agent_adjust(0),
      version(1),
      units_built(0) {}

UnitValues::UnitValues(const UnitValues& other)
    : turns(other.turns),
      hits(other.hits),
      armor(other.armor),
      attack(other.attack),
      speed(other.speed),
      fuel(other.speed),
      range(other.range),
      rounds(other.rounds),
      move_and_fire(other.move_and_fire),
      scan(other.scan),
      storage(other.storage),
      ammo(other.ammo),
      attack_radius(other.attack_radius),
      agent_adjust(other.agent_adjust),
      version(other.version),
      units_built(other.units_built) {}

UnitValues::~UnitValues() {}

FileObject* UnitValues::Allocate() { return new (std::nothrow) UnitValues(); }

static unsigned short UnitValues_TypeIndex;
static MAXRegisterClass UnitValues_ClassRegister("UnitValues", &UnitValues_TypeIndex, &UnitValues::Allocate);

unsigned short UnitValues::GetTypeIndex() const { return UnitValues_TypeIndex; }

void UnitValues::FileLoad(SmartFileReader& file) {
    file.Read(turns);
    file.Read(hits);
    file.Read(armor);
    file.Read(attack);
    file.Read(speed);
    file.Read(range);
    file.Read(rounds);
    file.Read(move_and_fire);
    file.Read(scan);
    file.Read(storage);
    file.Read(ammo);
    file.Read(attack_radius);
    file.Read(agent_adjust);
    file.Read(version);
    file.Read(units_built);
}

void UnitValues::FileSave(SmartFileWriter& file) {
    file.Write(turns);
    file.Write(hits);
    file.Write(armor);
    file.Write(attack);
    file.Write(speed);
    file.Write(range);
    file.Write(rounds);
    file.Write(move_and_fire);
    file.Write(scan);
    file.Write(storage);
    file.Write(ammo);
    file.Write(attack_radius);
    file.Write(agent_adjust);
    file.Write(version);
    file.Write(units_built);
}

int UnitValues::GetAttribute(char attribute) {
    int result;

    switch (attribute) {
        case ATTRIB_TURNS:
            result = turns;
            break;
        case ATTRIB_HITS:
            result = hits;
            break;
        case ATTRIB_ARMOR:
            result = armor;
            break;
        case ATTRIB_ATTACK:
            result = attack;
            break;
        case ATTRIB_MOVE_AND_FIRE:
            result = move_and_fire;
            break;
        case ATTRIB_SPEED:
            result = speed;
            break;
        case ATTRIB_FUEL:
            result = fuel;
            break;
        case ATTRIB_RANGE:
            result = range;
            break;
        case ATTRIB_ROUNDS:
            result = rounds;
            break;
        case ATTRIB_SCAN:
            result = scan;
            break;
        case ATTRIB_STORAGE:
            result = storage;
            break;
        case ATTRIB_AMMO:
            result = ammo;
            break;
        case ATTRIB_ATTACK_RADIUS:
            result = attack_radius;
            break;
        case ATTRIB_AGENT_ADJUST:
            result = agent_adjust;
            break;
        default:
            SDL_Log("UnitValues::GetAttribute called with invalid index.");
            SDL_assert(0);
            result = 0;
            break;
    }

    return result;
}

unsigned short* UnitValues::GetAttributeAddress(char attribute) {
    unsigned short* result;

    switch (attribute) {
        case ATTRIB_TURNS:
            result = &turns;
            break;
        case ATTRIB_HITS:
            result = &hits;
            break;
        case ATTRIB_ARMOR:
            result = &armor;
            break;
        case ATTRIB_ATTACK:
            result = &attack;
            break;
        case ATTRIB_SPEED:
            result = &speed;
            break;
        case ATTRIB_FUEL:
            result = &fuel;
            break;
        case ATTRIB_RANGE:
            result = &range;
            break;
        case ATTRIB_ROUNDS:
            result = &rounds;
            break;
        case ATTRIB_SCAN:
            result = &scan;
            break;
        case ATTRIB_STORAGE:
            result = &storage;
            break;
        case ATTRIB_AMMO:
            result = &ammo;
            break;
        case ATTRIB_ATTACK_RADIUS:
            result = &attack_radius;
            break;
        case ATTRIB_AGENT_ADJUST:
            result = &agent_adjust;
            break;
        default:
            SDL_Log("UnitValues::GetAttributeAddress called with invalid index.");
            SDL_assert(0);
            result = nullptr;
            break;
    }

    return result;
}

void UnitValues::SetAttribute(char attribute, int value) {
    switch (attribute) {
        case ATTRIB_TURNS:
            turns = value;
            break;
        case ATTRIB_HITS:
            hits = value;
            break;
        case ATTRIB_ARMOR:
            armor = value;
            break;
        case ATTRIB_ATTACK:
            attack = value;
            break;
        case ATTRIB_MOVE_AND_FIRE:
            move_and_fire = value;
            break;
        case ATTRIB_SPEED:
            speed = value;
            break;
        case ATTRIB_FUEL:
            fuel = value;
            break;
        case ATTRIB_RANGE:
            range = value;
            break;
        case ATTRIB_ROUNDS:
            rounds = value;
            break;
        case ATTRIB_SCAN:
            scan = value;
            break;
        case ATTRIB_STORAGE:
            storage = value;
            break;
        case ATTRIB_AMMO:
            ammo = value;
            break;
        case ATTRIB_ATTACK_RADIUS:
            attack_radius = value;
            break;
        case ATTRIB_AGENT_ADJUST:
            agent_adjust = value;
            break;
        default:
            SDL_Log("UnitValues::SetAttribute called with invalid index.");
            SDL_assert(0);
            break;
    }
}
void UnitValues::AddAttribute(char attribute, int value) {
    switch (attribute) {
        case ATTRIB_TURNS:
            turns += value;
            break;
        case ATTRIB_HITS:
            hits += value;
            break;
        case ATTRIB_ARMOR:
            armor += value;
            break;
        case ATTRIB_ATTACK:
            attack += value;
            break;
        case ATTRIB_MOVE_AND_FIRE:
            move_and_fire += value;
            break;
        case ATTRIB_SPEED:
            speed += value;
            break;
        case ATTRIB_FUEL:
            fuel += value;
            break;
        case ATTRIB_RANGE:
            range += value;
            break;
        case ATTRIB_ROUNDS:
            rounds += value;
            break;
        case ATTRIB_SCAN:
            scan += value;
            break;
        case ATTRIB_STORAGE:
            storage += value;
            break;
        case ATTRIB_AMMO:
            ammo += value;
            break;
        case ATTRIB_ATTACK_RADIUS:
            attack_radius += value;
            break;
        case ATTRIB_AGENT_ADJUST:
            agent_adjust += value;
            break;
        default:
            SDL_Log("UnitValues::AddAttribute called with invalid index.");
            SDL_assert(0);
            break;
    }
}

void UnitValues::UpdateVersion() {
    if (units_built) {
        ++version;
    }
}

int UnitValues::GetVersion() const { return version; }

void UnitValues::SetUnitsBuilt(unsigned char count) { units_built = count; }

bool UnitValues::operator==(const UnitValues& other) const {
    return turns == other.turns && hits == other.hits && armor == other.armor && attack == other.attack &&
           speed == other.speed && range == other.range && rounds == other.rounds &&
           move_and_fire == other.move_and_fire && scan == other.scan && storage == other.storage &&
           ammo == other.ammo && attack_radius == other.attack_radius && version == other.version &&
           units_built == other.units_built && agent_adjust == other.agent_adjust;
}

bool UnitValues::operator!=(const UnitValues& other) const { return !this->operator==(other); }
