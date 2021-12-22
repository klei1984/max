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

#ifndef UNITINFO_HPP
#define UNITINFO_HPP

#include "complex.hpp"
#include "paths.hpp"
#include "point.hpp"
#include "smartlist.hpp"
#include "smartobjectarray.hpp"
#include "tasks.hpp"
#include "unitvalues.hpp"

struct __attribute__((packed)) SoundElement {
    unsigned char type;
    unsigned short resource_id;
};

struct __attribute__((packed)) SoundTable {
    unsigned char count;
    struct SoundElement item[];
};

enum : unsigned char {
    SFX_TYPE_INVALID,
    SFX_TYPE_IDLE,
    SFX_TYPE_WATER_IDLE,
    SFX_TYPE_DRIVE,
    SFX_TYPE_WATER_DRIVE,
    SFX_TYPE_STOP,
    SFX_TYPE_WATER_STOP,
    SFX_TYPE_TRANSFORM,
    SFX_TYPE_BUILDING,
    SFX_TYPE_SHRINK,
    SFX_TYPE_EXPAND,
    SFX_TYPE_TURRET,
    SFX_TYPE_FIRE,
    SFX_TYPE_HIT,
    SFX_TYPE_EXPLOAD,
    SFX_TYPE_POWER_CONSUMPTION_START,
    SFX_TYPE_POWER_CONSUMPTION_END,
    SFX_TYPE_LAND,
    SFX_TYPE_TAKE,
    SFX_TYPE_LIMIT
};

class UnitInfo : public TextFileObject {
public:
    ResourceID unit_type;
    void (*sound_function)();
    struct SoundTable* sound_table;
    unsigned int flags;
    unsigned short x;
    unsigned short y;
    short grid_x;
    short grid_y;
    unsigned char field_30[4];
    char* color_cycling_lut;
    unsigned char team;
    unsigned char unit_id;
    unsigned char brightness;
    unsigned char angle;
    unsigned char max_velocity;
    unsigned char velocity;
    unsigned char sound;
    unsigned char scaler_adjust;
    unsigned char turret_angle;
    unsigned char turret_offset_x;
    unsigned char turret_offset_y;
    unsigned short total_images;
    unsigned short image_base;
    unsigned short turret_image_base;
    unsigned short firing_image_base;
    unsigned short connector_image_base;
    unsigned short image_index_max;
    unsigned char orders;
    unsigned char state;
    unsigned char prior_orders;
    unsigned char prior_state;
    unsigned short target_grid_x;
    unsigned short target_grid_y;
    unsigned char build_time;
    unsigned char total_mining;
    unsigned char raw_mining;
    unsigned char fuel_mining;
    unsigned char gold_mining;
    unsigned char raw_mining_max;
    unsigned char gold_mining_max;
    unsigned char fuel_mining_max;
    unsigned char hits;
    unsigned char speed;
    unsigned char field_79;
    unsigned char shots;
    unsigned char move_and_fire;
    unsigned short storage;
    unsigned char ammo;
    unsigned char targeting_mode;
    unsigned char enter_mode;
    unsigned char cursor;
    unsigned char recoil_delay;
    unsigned char delayed_reaction;
    unsigned char damaged_this_turn;
    unsigned char disabled_reaction_fire;
    unsigned char auto_survey;
    unsigned char research_topic;
    unsigned char moved;
    unsigned char bobbed;
    unsigned char engine;
    unsigned char weapon;
    unsigned char comm;
    unsigned char fuel_distance;
    unsigned char move_fraction;
    SmartPointer<UnitPath> path;
    unsigned short connectors;
    unsigned char field_107;
    SmartPointer<UnitValues> base_values;
    SmartPointer<Complex> complex;
    SmartObjectArray<unsigned short> build_list;
    unsigned short build_rate;
    unsigned char repeat_build;
    unsigned char energized;
    unsigned short id;
    SmartList<UnitInfo>* unit_list;
    SmartPointer<UnitInfo> parent_unit;
    SmartPointer<UnitInfo> enemy_unit;
    SmartList<Task> task_list1;
    SmartList<Task> task_list2;
    unsigned char field_158;
    Point field_159;
    unsigned short field_163;
    unsigned char field_165;
    unsigned char laying_state;
    char* name;
    unsigned char visible_to_team[5];
    unsigned char spotted_by_team[5];
    Rect sprite_bounds;
    Rect shadow_bounds;
    unsigned short image_index;
    unsigned short turret_image_index;
    Point shadow_offset;
    unsigned int field_221;

    UnitInfo();
    UnitInfo(const UnitInfo& other);
    ~UnitInfo();

    static TextFileObject* Allocate();
    unsigned short* GetAttribute(char index);

    unsigned short GetTypeIndex() const;
    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);

    void Init();
    bool IsVisibleToTeam(unsigned short team) const;
    unsigned short GetId() const;
    unsigned int GetField221() const;
    unsigned short GetImageIndex() const;
    void ClearUnitList();
    void PushFrontTask1List(Task* task);
    void ClearTask1List();
    Task* GetTask1ListFront() const;
    void SetParent(UnitInfo* parent);
    UnitInfo* GetParent() const;
    void SetEnemy(UnitInfo* enemy);
    UnitValues* GetBaseValues() const;
    Complex* GetComplex() const;
    bool UnitInfo_sub_430A2(short grid_x, short grid_y);
    SmartPointer<UnitInfo> MakeCopy();
    void OffsetDrawZones(int offset_x, int offset_y);
};

#endif /* UNITINFO_HPP */
