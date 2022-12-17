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

#include "button.hpp"
#include "complex.hpp"
#include "paths.hpp"
#include "point.hpp"
#include "smartlist.hpp"
#include "smartobjectarray.hpp"
#include "task.hpp"
#include "unitvalues.hpp"

#define UNITINFO_MAX_POPUP_BUTTON_COUNT 10

class NetPacket;

struct __attribute__((packed)) SoundElement {
    unsigned char type;
    unsigned short resource_id;
};

struct __attribute__((packed)) SoundTable {
    unsigned char count;
    struct SoundElement item[];
};

struct PopupButtons {
    unsigned char popup_count;
    unsigned char position[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    unsigned char key_code[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    const char* caption[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    bool state[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    ButtonFunc r_func[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    Button* buttons[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    unsigned short ulx;
    unsigned short uly;
    unsigned short width;
    unsigned short height;
};

class UnitInfoGroup;

class UnitInfo : public TextFileObject {
private:
    Complex* CreateComplex(unsigned short team);
    static struct ImageMultiFrameHeader* GetSpriteFrame(struct ImageMultiHeader* sprite, unsigned short image_index);
    static void UpdateSpriteFrameBounds(Rect* bounds, int x, int y, struct ImageMultiHeader* sprite,
                                        unsigned short image_index);
    void DrawSpriteTurretFrame(unsigned short turret_image_index);
    static int GetDrawLayer(ResourceID unit_type);
    void GainExperience(int experience);
    void Build();
    UnitInfo* GetConnectedBuilding(unsigned int connector);
    void AttachComplex(Complex* complex);
    void TestConnections();
    UnitInfo* GetFirstUntestedConnection();
    void UpdatePinsFromLists(int grid_x, int grid_y, SmartList<UnitInfo>* units, int pin_units);
    void FindTarget(int grid_x, int grid_y, SmartList<UnitInfo>* units);
    void RadarPing();
    void UpgradeInt();
    static void CalcRomanDigit(char* text, int value, const char* digit1, const char* digit2, const char* digit3);
    void Regenerate();
    void StepMoveUnit(Point position);
    void PrepareConstructionSite(ResourceID unit_type);
    void RenderShadow(Point point, int image_id, Rect* bounds);
    void RenderSprite(Point point, int image_base, Rect* bounds);
    int GetTargetUnitAngle();
    void UpdateInfoDisplay();
    void ClearInTransitFlag();

    static const unsigned char ExpResearchTopics[];

public:
    UnitInfo();
    UnitInfo(ResourceID unit_type, unsigned short team, unsigned short id, unsigned char angle = 0);
    UnitInfo(const UnitInfo& other);
    ~UnitInfo();

    static TextFileObject* Allocate();

    unsigned short GetTypeIndex() const;
    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);

    bool IsVisibleToTeam(unsigned short team) const;
    void SetEnemy(UnitInfo* enemy);
    UnitInfo* GetEnemy() const;
    UnitInfo* GetParent() const;
    unsigned short GetId() const;
    UnitInfo* GetFirstFromUnitList() const;
    SmartList<UnitInfo>* GetUnitList() const;
    unsigned int GetField221() const;
    void SetField221(unsigned int value);
    void ChangeField221(unsigned int flags, bool mode);
    unsigned short GetImageIndex() const;
    void SetParent(UnitInfo* parent);
    void SetBaseValues(UnitValues* unit_values);
    UnitValues* GetBaseValues() const;
    bool IsDetectedByTeam(unsigned short team) const;
    Complex* GetComplex() const;
    int GetLayingState() const;
    void SetLayingState(int state);
    void ClearPins();
    void SetBuildRate(int value);
    int GetBuildRate() const;
    void SetRepeatBuildState(bool value);
    bool GetRepeatBuildState() const;
    bool AreTherePins();
    void UpdateSpriteFrame(unsigned short image_base, unsigned short image_index_max);
    void DrawSpriteFrame(unsigned short image_index);
    void OffsetDrawZones(int offset_x, int offset_y);
    void UpdateUnitDrawZones();
    void RefreshScreen();
    void UpdateAngle(unsigned short image_index);
    void AddToDrawList(unsigned int override_flags = 0);
    void SetPosition(int grid_x, int grid_y, bool skip_map_status_update);
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
    void UpdateTurretAngle(int turret_angle, bool redraw = false);
    void UpdatePinCount(int grid_x, int grid_y, int pin_units);
    void Attack(int grid_x, int grid_y);
    SmartPointer<UnitInfo> MakeCopy();
    void MoveFinished(bool mode = true);
    void BlockedOnPathRequest(bool mode = true);
    void RestoreOrders();
    void FollowUnit();
    void InitStealthStatus();
    void Draw(unsigned short team);
    void DrawStealth(unsigned short team);
    void TakePathStep();
    void SpotByTeam(unsigned short team);
    static void GetVersion(char* text, int version);
    void GetName(char* text) const;
    void GetDisplayName(char* text) const;
    void SetName(char* text);
    int GetRaw();
    int GetRawFreeCapacity();
    void TransferRaw(int amount);
    int GetFuel();
    int GetFuelFreeCapacity();
    void TransferFuel(int amount);
    int GetGold();
    int GetGoldFreeCapacity();
    void TransferGold(int amount);
    int GetTurnsToRepair();
    int Repair(int materials);
    void Resupply();
    int GetExperience();
    int GetRawConsumptionRate();
    void UpdateProduction();
    int GetNormalRateBuildCost() const;
    SmartObjectArray<ResourceID> GetBuildList();
    ResourceID GetConstructedUnitType() const;
    void CancelBuilding();
    bool IsUpgradeAvailable();
    void DeployConstructionSiteMarkers(ResourceID unit_type);
    int GetMaxAllowedBuildRate();
    void StartBuilding();
    void SpawnNewUnit();
    void ReadPacket(NetPacket& packet);
    void WritePacket(NetPacket& packet);
    void Refuel(UnitInfo* parent);
    void Reload(UnitInfo* parent);
    void Upgrade(UnitInfo* parent);
    void BuildOrder();
    void MoveInTransitUnitInMapHash(int grid_x, int grid_y);
    void RemoveInTransitUnitFromMapHash();
    void GetBounds(Rect* bounds);
    void BusyWaitOrder();
    void AddReminders(bool priority);
    Task* GetTask() const;
    SmartList<Task>::Iterator GetTask1ListIterator();
    void PushBackTask2List(Task* task);
    void RemoveFromTask2List(Task* task);
    void PushFrontTask1List(Task* task);
    void ClearFromTaskLists();
    void RemoveTask(Task* task, bool mode = true);
    bool IsReadyForOrders(Task* task);
    int GetAttackRange();
    bool IsBridgeElevated() const;
    bool IsInGroupZone(UnitInfoGroup* group);
    void RenderAirShadow(Rect* bounds);
    void Render(Rect* bounds);
    void RenderWithConnectors(Rect* bounds);
    void PositionInTape();
    void PlaceMine();
    void PickUpMine();
    bool ShakeWater();
    bool ShakeAir();
    void ShakeSabotage();
    bool Land();
    bool Take();
    void PrepareFire();
    void ProgressFire();
    void SpinningTurretAdvanceAnimation();
    void Redraw();
    void ChangeTeam(unsigned short target_team);
    bool AttemptSideStep(int grid_x, int grid_y, int angle);
    int GetTurnsToBuild(ResourceID unit_type, int build_speed_multiplier, int* turns_to_build);
    void Init();

    ResourceID unit_type;
    struct PopupFunctions* popup;
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
    short target_grid_x;
    short target_grid_y;
    unsigned char build_time;
    unsigned char total_mining;
    unsigned char raw_mining;
    unsigned char fuel_mining;
    unsigned char gold_mining;
    unsigned char raw_mining_max;
    unsigned char gold_mining_max;
    unsigned char fuel_mining_max;
    unsigned char hits;
    signed char speed;
    unsigned char group_speed;
    unsigned char shots;
    unsigned char move_and_fire;
    unsigned short storage;
    unsigned char ammo;
    unsigned char targeting_mode;
    unsigned char enter_mode;
    unsigned char cursor;
    signed char recoil_delay;
    unsigned char delayed_reaction;
    bool damaged_this_turn;
    bool disabled_reaction_fire;
    bool auto_survey;
    unsigned char research_topic;
    unsigned char moved;
    bool bobbed;
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
    bool in_transit;
    Point last_target;
    short pin_count;
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
