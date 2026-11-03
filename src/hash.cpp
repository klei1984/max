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

#include "hash.hpp"

#include "resource_manager.hpp"

#define HASH_UNIT_HASH_SIZE 8192
#define HASH_MAP_HASH_SIZE_MIN 512
#define HASH_MAP_HASH_SIZE_MAX 32768

UnitHash Hash_UnitHash(HASH_UNIT_HASH_SIZE);
MapHash Hash_MapHash(HASH_MAP_HASH_SIZE_MIN);

void SmartList_UnitInfo_FileLoad(SmartList<UnitInfo>& list, SmartFileReader& file) {
    list.Clear();

    for (int64_t count = file.ReadObjectCount(); count; --count) {
        UnitInfo* unit = dynamic_cast<UnitInfo*>(file.ReadObject());

        list.PushBack(*unit);

        if (unit->flags & HASH_TEAM_RED) {
            unit->color_cycling_lut = ResourceManager_TeamRedColorIndexTable;
        } else if (unit->flags & HASH_TEAM_GREEN) {
            unit->color_cycling_lut = ResourceManager_TeamGreenColorIndexTable;
        } else if (unit->flags & HASH_TEAM_BLUE) {
            unit->color_cycling_lut = ResourceManager_TeamBlueColorIndexTable;
        } else if (unit->flags & HASH_TEAM_GRAY) {
            unit->color_cycling_lut = ResourceManager_TeamGrayColorIndexTable;
        } else {
            unit->color_cycling_lut = ResourceManager_TeamDerelictColorIndexTable;
        }

        unit->SetSfxType(Unit::SFX_TYPE_INVALID);
    }
}

void SmartList_UnitInfo_FileSave(SmartList<UnitInfo>& list, SmartFileWriter& file) {
    uint32_t count = list.GetCount();

    file.Write(count);
    for (auto unit = list.Begin(), unit_end = list.End(); unit != unit_end; ++unit) {
        file.WriteObject(unit->Get());
    }
}

void SmartList_UnitInfo_Clear(SmartList<UnitInfo>& list) {
    for (auto unit = list.Begin(), unit_end = list.End(); unit != unit_end; ++unit) {
        unit->Get()->SetParent(nullptr);
        unit->Get()->SetEnemy(nullptr);
        unit->Get()->RemoveTasks();
    }

    list.Clear();
}

class MapHashObject : public SmartObject {
    SmartList<UnitInfo> list;
    uint16_t x;
    uint16_t y;

public:
    MapHashObject(uint16_t grid_x, uint16_t grid_y);
    ~MapHashObject();

    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);

    uint16_t GetX() const;
    uint16_t GetY() const;
    SmartList<UnitInfo>& GetList();
    void PushFront(UnitInfo* unit);
    void PushBack(UnitInfo* unit);
    void Remove(UnitInfo* unit);
};

MapHashObject::MapHashObject(uint16_t grid_x, uint16_t grid_y) : x(grid_x), y(grid_y) {}

MapHashObject::~MapHashObject() {}

void MapHashObject::FileLoad(SmartFileReader& file) {
    file.Read(x);
    file.Read(y);
    SmartList_UnitInfo_FileLoad(list, file);
}

void MapHashObject::FileSave(SmartFileWriter& file) {
    file.Write(x);
    file.Write(y);
    SmartList_UnitInfo_FileSave(list, file);
}

uint16_t MapHashObject::GetX() const { return x; }

uint16_t MapHashObject::GetY() const { return y; }

SmartList<UnitInfo>& MapHashObject::GetList() { return list; }

void MapHashObject::PushFront(UnitInfo* unit) { list.PushFront(*unit); }

void MapHashObject::PushBack(UnitInfo* unit) { list.PushBack(*unit); }

void MapHashObject::Remove(UnitInfo* unit) { list.Remove(*unit); }

MapHash::MapHash(uint16_t hash_size) : hash_size(0), x_shift(0), entry(nullptr) { Init(hash_size); }

MapHash::~MapHash() { delete[] entry; }

void MapHash::Init(uint16_t size) {
    Clear();
    delete[] entry;

    hash_size = size;
    x_shift = 0;
    entry = new (std::nothrow) SmartList<MapHashObject>[hash_size];

    while (size > 128) {
        ++x_shift;
        size >>= 1;
    }
}

void MapHash::SetMapSize(const Point& map_size) {
    uint16_t size = HASH_MAP_HASH_SIZE_MIN;

    // about 8 map cells per bucket
    while (size < HASH_MAP_HASH_SIZE_MAX && size * 8 < map_size.x * map_size.y) {
        size <<= 1;
    }

    Init(size);
}

void MapHash::AddEx(UnitInfo* unit, uint16_t grid_x, uint16_t grid_y, bool mode) {
    SmartList<MapHashObject>* list = &entry[(grid_y ^ (grid_x << x_shift)) % hash_size];
    auto object = list->Begin(), object_end = list->End();

    while (object != object_end) {
        if (grid_x == (*object).GetX() && grid_y == (*object).GetY()) {
            break;
        }

        ++object;
    }

    if (object == object_end) {
        list->PushFront(*new (std::nothrow) MapHashObject(grid_x, grid_y));
        object = list->Begin();
    }

    if (!mode || unit->GetUnitType() == LRGTAPE || unit->GetUnitType() == SMLTAPE) {
        (*object).PushFront(unit);
    } else {
        (*object).PushBack(unit);
    }
}

void MapHash::Add(UnitInfo* unit, bool mode) {
    uint16_t grid_x;
    uint16_t grid_y;

    SDL_assert(unit != nullptr);

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    AddEx(unit, grid_x, grid_y, mode);

    if (unit->flags & BUILDING) {
        AddEx(unit, grid_x + 1, grid_y, mode);
        AddEx(unit, grid_x, grid_y + 1, mode);
        AddEx(unit, grid_x + 1, grid_y + 1, mode);
    }
}

void MapHash::RemoveEx(UnitInfo* unit, uint16_t grid_x, uint16_t grid_y) {
    SmartList<MapHashObject>* list = &entry[(grid_y ^ (grid_x << x_shift)) % hash_size];
    auto object = list->Begin(), object_end = list->End();

    while (object != object_end) {
        if (grid_x == (*object).GetX() && grid_y == (*object).GetY()) {
            break;
        }

        ++object;
    }

    if (object != object_end) {
        (*object).Remove(unit);

        if (!(*object).GetList().GetCount()) {
            list->Remove(object);
        }
    }
}

void MapHash::Remove(UnitInfo* unit) {
    uint16_t grid_x;
    uint16_t grid_y;

    SDL_assert(unit != nullptr);

    grid_x = unit->grid_x;
    grid_y = unit->grid_y;

    RemoveEx(unit, grid_x, grid_y);

    if (unit->flags & BUILDING) {
        RemoveEx(unit, grid_x + 1, grid_y);
        RemoveEx(unit, grid_x, grid_y + 1);
        RemoveEx(unit, grid_x + 1, grid_y + 1);
    }
}

void MapHash::Clear() {
    for (int32_t index = 0; index < hash_size; ++index) {
        entry[index].Clear();
    }
}

void MapHash::FileLoad(SmartFileReader& file) {
    uint16_t saved_hash_size;
    uint16_t saved_x_shift;

    Clear();

    file.Read(saved_hash_size);
    file.Read(saved_x_shift);

    // saves of another hash size are rehashed into the size of the loaded map
    for (uint32_t index = 0; index < saved_hash_size; ++index) {
        for (int64_t count = file.ReadObjectCount(); count; --count) {
            MapHashObject* object = new (std::nothrow) MapHashObject(0, 0);

            object->FileLoad(file);
            entry[(object->GetY() ^ (object->GetX() << x_shift)) % hash_size].PushBack(*object);
        }
    }
}

void MapHash::FileSave(SmartFileWriter& file) {
    file.Write(hash_size);
    file.Write(x_shift);

    for (uint32_t index = 0; index < hash_size; ++index) {
        uint32_t count = entry[index].GetCount();
        file.Write(count);

        for (auto object = entry[index].Begin(), object_end = entry[index].End(); object != object_end; ++object) {
            (*object).FileSave(file);
        }
    }
}

SmartList<UnitInfo>* MapHash::operator[](const Point& key) {
    SDL_assert(key.x >= 0 && key.y >= 0);

    SmartList<UnitInfo>* result{nullptr};
    SmartList<MapHashObject>& list = entry[(key.y ^ (key.x << x_shift)) % hash_size];

    for (auto& object : list) {
        if (key.x == object.GetX() && key.y == object.GetY()) {
            result = &object.GetList();
            break;
        }
    }

    return result;
}

UnitHash::UnitHash(uint16_t hash_size)
    : hash_size(hash_size), list(new (std::nothrow) SmartList<UnitInfo>[hash_size]) {}
UnitHash::~UnitHash() { delete[] list; }

void UnitHash::PushBack(UnitInfo* unit) {
    SDL_assert(unit != nullptr);

    list[unit->GetId() % hash_size].PushBack(*unit);
}
void UnitHash::Remove(UnitInfo* unit) {
    SDL_assert(unit != nullptr);

    list[unit->GetId() % hash_size].Remove(*unit);
}

void UnitHash::Clear() {
    for (int32_t index = 0; index < hash_size; ++index) {
        for (auto unit = list[index].Begin(), unit_end = list[index].End(); unit != unit_end; ++unit) {
            (*unit).ClearUnitList();
            (*unit).SetParent(nullptr);
        }

        list[index].Clear();
    }
}

void UnitHash::FileLoad(SmartFileReader& file) {
    uint16_t saved_hash_size;

    Clear();

    file.Read(saved_hash_size);

    SDL_assert(saved_hash_size > 0);

    // saves of another hash size are rehashed
    for (int32_t index = 0; index < saved_hash_size; ++index) {
        SmartList<UnitInfo> units;

        SmartList_UnitInfo_FileLoad(units, file);

        for (auto unit = units.Begin(), unit_end = units.End(); unit != unit_end; ++unit) {
            PushBack(unit->Get());
        }
    }
}

void UnitHash::FileSave(SmartFileWriter& file) {
    file.Write(hash_size);

    for (int32_t index = 0; index < hash_size; ++index) {
        SmartList_UnitInfo_FileSave(list[index], file);
    }
}

UnitInfo* UnitHash::operator[](const uint32_t& key) {
    const auto& units = list[key % hash_size];

    for (auto unit = units.Begin(), unit_end = units.End(); unit != unit_end; ++unit) {
        if (key == (*unit).GetId()) {
            return unit->Get();
        }
    }

    return nullptr;
}
