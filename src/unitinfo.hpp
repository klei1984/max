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

class UnitInfoGroup;

class UnitInfo : public TextFileObject {
private:
    static void CalcRomanDigit(char* text, int value, const char* digit1, const char* digit2, const char* digit3);
    Complex* CreateComplex(unsigned short team);
    static struct ImageMultiFrameHeader* GetSpriteFrame(struct ImageMultiHeader* sprite, unsigned short image_index);
    static void UpdateSpriteFrameBounds(Rect* bounds, int x, int y, struct ImageMultiHeader* sprite,
                                        unsigned short image_index);
    static int GetDrawLayer(ResourceID unit_type);
    void SetPosition(int grid_x, int grid_y, bool skip_map_status_update);
    void GainExperience(int experience);
    void Build();
    UnitInfo* GetConnectedBuilding(unsigned int connector);
    void AttachComplex(Complex* complex);
    void TestConnections();
    UnitInfo* GetFirstUntestedConnection();

    static const unsigned char ExpResearchTopics[];

public:
    UnitInfo();
    UnitInfo(ResourceID unit_type, unsigned short team, unsigned short id, unsigned char angle = 0);
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
    void UpdateSpriteFrame(unsigned short image_base, unsigned short image_index_max);
    void DrawSpriteFrame(unsigned short image_index);
    void DrawSpriteTurretFrame(unsigned short turret_image_index);
    void OffsetDrawZones(int offset_x, int offset_y);
    void UpdateUnitDrawZones();
    void RefreshScreen();
    void UpdateUnitAngle(unsigned short image_index);
    void AddToDrawList(unsigned int override_flags = 0);
    void ProcessTaskList();
    void AttackUnit(UnitInfo* enemy, int attack_potential, int direction);
    bool ExpectAttack();
    void ClearBuildListAndPath();
    void Move();
    void AllocateUnitList();
    void AssignUnitList(UnitInfo* unit);
    void ClearUnitList();
    void MoveToFrontInUnitList();
    void AttachToPrimaryComplex();
    void DetachComplex();
    void UpdateTurretAngle(int turrent_angle, bool redraw = false);

    void Attack(int grid_x, int grid_y);
    void StartBuilding();

    bool IsVisibleToTeam(unsigned short team) const;
    void SetEnemy(UnitInfo* enemy);
    UnitInfo* GetEnemy() const;
    UnitInfo* GetParent() const;
    unsigned short GetId() const;
    UnitInfo* GetFirstFromUnitList() const;
    SmartList<UnitInfo>* GetUnitList() const;
    unsigned int GetField221() const;
    unsigned short GetImageIndex() const;
    void PushFrontTask1List(Task* task);
    void ClearTask1List();
    Task* GetTask1ListFront() const;
    void SetParent(UnitInfo* parent);
    void SetBaseValues(UnitValues* unit_values);
    UnitValues* GetBaseValues() const;
    bool IsDetectedByTeam(unsigned short team) const;
    Complex* GetComplex() const;
    bool UnitInfo_sub_430A2(short grid_x, short grid_y);
    SmartPointer<UnitInfo> MakeCopy();
    void GetName(char* text) const;
    void GetDisplayName(char* text) const;
    static void GetVersion(char* text, int version);
    void SetName(char* text);
    void InitStealthStatus();
    void SpotByTeam(unsigned short team);
    void Draw(unsigned short team);
    void DrawStealth(unsigned short team);
    void Resupply();
    int GetRawConsumptionRate();
    void UpdateProduction();
    int GetNormalRateBuildCost() const;
    SmartObjectArray<ResourceID> GetBuildList();
    ResourceID GetConstructedUnitType() const;
    bool IsBridgeElevated() const;
    bool IsInGroupZone(UnitInfoGroup* group);
    void RenderShadow(Point point, int image_id, Rect* bounds);
    void RenderAirShadow(Rect* bounds);
    void RenderSprite(Point point, int image_base, Rect* bounds);
    void Render(Rect* bounds);
    void RenderWithConnectors(Rect* bounds);
    int GetMaxAllowedBuildRate();
    void TakePathStep();
    void SetLayingState(int state);
    void ClearPins();

    ResourceID unit_type;
    void (*sound_function)(UnitInfo* unit, struct PopupButtons* buttons);
    struct SoundTable* sound_table;
    unsigned int flags;
    unsigned short x;
    unsigned short y;
    short grid_x;
    short grid_y;
    Point point;
    ColorIndex* color_cycling_lut;
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
    unsigned char group_speed;
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
    bool disabled_reaction_fire;
    bool auto_survey;
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
    unsigned char shake_effect_state;
    SmartPointer<UnitValues> base_values;
    SmartPointer<Complex> complex;
    SmartObjectArray<ResourceID> build_list;
    unsigned short build_rate;
    unsigned char repeat_build;
    bool energized;
    unsigned short id;
    SmartList<UnitInfo>* unit_list;
    SmartPointer<UnitInfo> parent_unit;
    SmartPointer<UnitInfo> enemy_unit;
    SmartList<Task> task_list1;
    SmartList<Task> task_list2;
    bool field_158;
    Point unknown_point;
    unsigned short pin_count;
    bool field_165;
    unsigned char laying_state;
    char* name;
    unsigned char visible_to_team[PLAYER_TEAM_MAX];
    unsigned char spotted_by_team[PLAYER_TEAM_MAX];
    Rect sprite_bounds;
    Rect shadow_bounds;
    unsigned short image_index;
    unsigned short turret_image_index;
    Point shadow_offset;
    unsigned int field_221;
};

#endif /* UNITINFO_HPP */
