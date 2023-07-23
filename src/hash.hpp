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

#ifndef HASH_HPP
#define HASH_HPP

#include "unitinfo.hpp"

class MapHashObject;

class MapHash {
    uint16_t hash_size;
    uint16_t x_shift;
    SmartList<MapHashObject>* entry;

    void AddEx(UnitInfo* unit, uint16_t grid_x, uint16_t grid_y, bool mode);
    void RemoveEx(UnitInfo* unit, uint16_t grid_x, uint16_t grid_y);

public:
    MapHash(uint16_t hash_size);
    ~MapHash();

    void Add(UnitInfo* unit, bool mode = false);
    void Remove(UnitInfo* unit);
    void Clear();

    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);

    SmartList<UnitInfo>* operator[](const Point& key);
};

class UnitHash {
    uint16_t hash_size;
    SmartList<UnitInfo>* list;

public:
    UnitHash(uint16_t hash_size);
    ~UnitHash();

    void PushBack(UnitInfo* unit);
    void Remove(UnitInfo* unit);
    void Clear();

    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);

    UnitInfo* operator[](const uint16_t& key);
};

extern UnitHash Hash_UnitHash;
extern MapHash Hash_MapHash;

void SmartList_UnitInfo_FileLoad(SmartList<UnitInfo>& list, SmartFileReader& file);
void SmartList_UnitInfo_FileSave(SmartList<UnitInfo>& list, SmartFileWriter& file);

#endif /* HASH_HPP */
