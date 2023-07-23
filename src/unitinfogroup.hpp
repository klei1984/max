/* Copyright (c) 2022 M.A.X. Port Team
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

#ifndef UNITINFOGROUP_HPP
#define UNITINFOGROUP_HPP

#include "unitinfo.hpp"

#define SMARTFILE_OBJECT_ARRAY_DEFAULT_CAPACITY 10

class UnitInfoArray {
    SmartArray<UnitInfo> array;

public:
    UnitInfoArray(uint16_t growth_factor = SMARTFILE_OBJECT_ARRAY_DEFAULT_CAPACITY);
    ~UnitInfoArray();

    void Insert(UnitInfo& unit);
    int32_t GetCount() const;
    UnitInfo& operator[](int32_t index) const;
    void Release();
};

class UnitInfoGroup {
    Rect bounds1;
    Rect bounds2;

    UnitInfoArray torpedos;
    UnitInfoArray water_platform;
    UnitInfoArray passeable_ground_cover;
    UnitInfoArray land_rubbles;
    UnitInfoArray sea_land_mines;
    UnitInfoArray ground_covers;
    UnitInfoArray sea_land_units;
    UnitInfoArray elevated_bridge;
    UnitInfoArray air_units_on_ground;
    UnitInfoArray air_units_in_air;
    UnitInfoArray air_particles_explosions;
    UnitInfoArray rockets;

    static bool IsRelevant(UnitInfo* unit, UnitInfoGroup* group);
    bool Populate();
    void RenderGroups();
    void RenderList(SmartList<UnitInfo>* units);
    void RenderLists();

public:
    UnitInfoGroup();
    ~UnitInfoGroup();

    Rect* GetBounds1();
    Rect* GetBounds2();

    void ProcessDirtyZone(Rect* bounds);
};

#endif /* UNITINFOGROUP_HPP */
