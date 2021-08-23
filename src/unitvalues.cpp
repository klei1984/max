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

UnitValues::UnitValues()
    : turns(0),
      hits(0),
      armor(0),
      attack(0),
      speed(0),
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

unsigned short UnitValues::GetTypeIndex() { return 1; }

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

void UnitValues::FileSave(SmartFileWriter& file) {}

void UnitValues::TextLoad() {}

void UnitValues::TextSave() {}
