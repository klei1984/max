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

#ifndef UNITVALUES_HPP
#define UNITVALUES_HPP

#include "textfile.hpp"

class UnitValues : public TextFileObject {
    unsigned short turns;
    unsigned short hits;
    unsigned short armor;
    unsigned short attack;
    unsigned short speed;
    unsigned short fuel;
    unsigned short range;
    unsigned short rounds;
    unsigned char move_and_fire;
    unsigned short scan;
    unsigned short storage;
    unsigned short ammo;
    unsigned short attack_radius;
    unsigned short agent_adjust;
    unsigned short version;
    unsigned char units_built;

public:
    UnitValues();
    UnitValues(const UnitValues& other);
    ~UnitValues();

    static TextFileObject* Allocate();
    int GetAttribute(char attribute);
    void SetAttribute(char attribute, int value);

    unsigned short GetTypeIndex() const;
    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);

    bool operator==(const UnitValues& other) const;
    bool operator!=(const UnitValues& other) const;
};

enum : char {
    ATTRIB_TURNS,
    ATTRIB_HITS,
    ATTRIB_ARMOR,
    ATTRIB_ATTACK,
    ATTRIB_MOVE_AND_FIRE,
    ATTRIB_SPEED,
    ATTRIB_FUEL,
    ATTRIB_RANGE,
    ATTRIB_ROUNDS,
    ATTRIB_SCAN,
    ATTRIB_STORAGE,
    ATTRIB_AMMO,
    ATTRIB_ATTACK_RADIUS
};

#endif /* UNITVALUES_HPP */
