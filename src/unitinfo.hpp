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

#include <vector>

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

struct SoundElement {
    uint8_t type;
    uint16_t resource_id;
};

struct PopupButtons {
    uint8_t popup_count;
    uint8_t position[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    uint8_t key_code[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    const char* caption[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    bool state[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    ButtonFunc r_func[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    Button* buttons[UNITINFO_MAX_POPUP_BUTTON_COUNT];
    uint16_t ulx;
    uint16_t uly;
    uint16_t width;
    uint16_t height;
};

class UnitInfoGroup;

class UnitInfo : public FileObject {
private:
    Complex* CreateComplex(uint16_t team);
    static struct ImageMultiFrameHeader* GetSpriteFrame(struct ImageMultiHeader* sprite, uint16_t image_index);
    static void UpdateSpriteFrameBounds(Rect* bounds, int32_t x, int32_t y, struct ImageMultiHeader* sprite,
                                        uint16_t image_index);
    void DrawSpriteTurretFrame(uint16_t turret_image_index);
    void GainExperience(int32_t experience);
    void Build();
    UnitInfo* GetConnectedBuilding(uint32_t connector);
    void AttachComplex(Complex* complex);
    void TestConnections();
    UnitInfo* GetFirstUntestedConnection();
    void UpdatePinsFromLists(int32_t grid_x, int32_t grid_y, SmartList<UnitInfo>* units, int32_t pin_units);
    void FindTarget(int32_t grid_x, int32_t grid_y, SmartList<UnitInfo>* units);
    void RadarPing();
    void UpgradeInt();
    static void CalcRomanDigit(char* text, int32_t value, const char* digit1, const char* digit2, const char* digit3);
    void Regenerate();
    void StepMoveUnit(Point position);
    void PrepareConstructionSite(ResourceID unit_type);
    void RenderShadow(Point point, int32_t image_id, Rect* bounds);
    void RenderSprite(Point point, int32_t image_base, Rect* bounds);
    int32_t GetTargetUnitAngle();
    void UpdateInfoDisplay();
    void ClearInTransitFlag();

    static const uint8_t ExpResearchTopics[];

    SmartPointer<UnitInfo> parent_unit;
    char* name;
    uint8_t sound;
    ResourceID unit_type;
    UnitOrderType orders;
    UnitOrderType prior_orders;
    UnitOrderStateType state;
    UnitOrderStateType prior_state;

public:
    static constexpr uint32_t AI_STATE_NO_RETREAT = 0x00000001;
    static constexpr uint32_t AI_STATE_TARGET_COOLDOWN_MASK = 0x000000FE;
    static constexpr uint32_t AI_STATE_MOVE_FINISHED_REMINDER = 0x00000100;

    /**
     * The default constructor is only usable in special use cases.
     */
    UnitInfo();

    /**
     * Construct a particular unit type for a given team.
     *
     * \param unit_type one of the ResourceID enum values below UNIT_END
     * \param team unit one of the PlayerTeamType enum values
     * \param id unit hash between 0x0000 - 0xFFFE, 0xFFFF is the invalid id
     * \param angle one of the UnitAngle enum values
     */
    UnitInfo(ResourceID unit_type, uint16_t team, uint16_t id, uint8_t angle = 0);

    /** Copy constructor. */
    UnitInfo(const UnitInfo& other);

    /** Destructor. */
    ~UnitInfo();

    /**
     * Get UTF-8 encoded null terminated name of unit without mark segment.
     *
     * \param text a plain char* buffer to hold the returned unit name
     * \param size buffer size in bytes
     */
    void GetName(char* const text, const size_t size) const noexcept;

    /**
     * Get UTF-8 encoded null terminated full name of unit.
     *
     * \param text a plain char* buffer to hold the returned unit name
     * \param size buffer size in bytes
     */
    void GetDisplayName(char* const text, const size_t size) const noexcept;

    /**
     * Set UTF-8 encoded null terminated name of unit without mark segment.
     *
     * \param text a plain char* buffer that holds the name to set
     */
    void SetName(const char* const text) noexcept;

    /**
     * Get sound effect type being played by the sound manager for the unit.
     * SFX_TYPE_INVALID means no sound effect is associated with the unit.
     *
     * \returns one of the SfxType enum values
     */
    [[nodiscard]] uint8_t GetSfxType() const noexcept;

    /**
     * Set sound effect type being played by the sound manager for the unit.
     *
     * \param sound one of the SfxType enum values
     * \returns previous sound type
     */
    uint8_t SetSfxType(uint8_t sound) noexcept;

    /**
     * Get type of unit.
     *
     * \returns one of the ResourceID enum values below UNIT_END
     */
    [[nodiscard]] ResourceID GetUnitType() const noexcept;

    /**
     * Set type of unit.
     *
     * \param unit_type one of the ResourceID enum values below UNIT_END
     */
    void SetUnitType(ResourceID unit_type) noexcept;

    /**
     * Get the current order of unit.
     *
     * \returns one of the UnitOrderType enum values.
     */
    [[nodiscard]] UnitOrderType GetOrder() const noexcept;

    /**
     * Set current order of unit. The prior order is not changed.
     *
     * \param order one of the UnitOrderType enum values.
     * \returns the previous value of the current unit order.
     */
    UnitOrderType SetOrder(UnitOrderType order) noexcept;

    /**
     * Get the prior order of unit.
     *
     * \returns one of the UnitOrderType enum values.
     */
    [[nodiscard]] UnitOrderType GetPriorOrder() const noexcept;

    /**
     * Set prior order of unit. The current order is not changed.
     *
     * \param order one of the UnitOrderType enum values.
     * \returns the previous value of the prior unit order.
     */
    UnitOrderType SetPriorOrder(UnitOrderType order) noexcept;

    /**
     * Get the current order state of unit.
     *
     * \returns one of the UnitOrderStateType enum values.
     */
    [[nodiscard]] UnitOrderStateType GetOrderState() const noexcept;

    /**
     * Set current order state of unit. The prior order state is not changed.
     *
     * \param order_state one of the UnitOrderStateType enum values.
     * \returns the previous value of the current unit order state.
     */
    UnitOrderStateType SetOrderState(UnitOrderStateType order_state) noexcept;

    /**
     * Get the prior order state of unit.
     *
     * \returns one of the UnitOrderStateType enum values.
     */
    [[nodiscard]] UnitOrderStateType GetPriorOrderState() const noexcept;

    /**
     * Set prior order state of unit. The current order state is not changed.
     *
     * \param order_state one of the UnitOrderStateType enum values.
     * \returns the previous value of the prior unit order state.
     */
    UnitOrderStateType SetPriorOrderState(UnitOrderStateType order_state) noexcept;

    static FileObject* Allocate() noexcept;
    uint32_t GetTypeIndex() const;
    void FileLoad(SmartFileReader& file) noexcept;
    void FileSave(SmartFileWriter& file) noexcept;
    static int32_t GetDrawLayer(ResourceID unit_type);
    bool IsVisibleToTeam(uint16_t team) const;
    void SetEnemy(UnitInfo* enemy);
    UnitInfo* GetEnemy() const;
    uint16_t GetId() const;
    UnitInfo* GetFirstFromUnitList() const;
    SmartList<UnitInfo>* GetUnitList() const;
    uint32_t GetAiStateBits() const;
    void SetAiStateBits(const uint32_t value);
    void ChangeAiStateBits(const uint32_t value, const bool mode);
    uint16_t GetImageIndex() const;
    void SetBaseValues(UnitValues* unit_values);
    UnitValues* GetBaseValues() const;
    bool IsDetectedByTeam(uint16_t team) const;
    Complex* GetComplex() const;
    int32_t GetLayingState() const;
    void SetLayingState(int32_t state);
    void ClearPins();
    void SetBuildRate(int32_t value);
    int32_t GetBuildRate() const;
    void SetRepeatBuildState(bool value);
    bool GetRepeatBuildState() const;
    bool AreTherePins();
    void UpdateSpriteFrame(uint16_t image_base, uint16_t image_index_max);
    void DrawSpriteFrame(uint16_t image_index);
    void OffsetDrawZones(int32_t offset_x, int32_t offset_y);
    void UpdateUnitDrawZones();
    void RefreshScreen();
    void UpdateAngle(uint16_t image_index);
    void AddToDrawList(uint32_t override_flags = 0);
    void SetPosition(int32_t grid_x, int32_t grid_y, bool skip_map_status_update);
    void RemoveDelayedTasks();
    void AttackUnit(UnitInfo* enemy, int32_t attack_potential, int32_t direction);
    bool ExecuteMove();
    void ClearBuildListAndPath();
    void Move();
    void AllocateUnitList();
    void AssignUnitList(UnitInfo* unit);
    void ClearUnitList();
    void MoveToFrontInUnitList();
    void AttachToPrimaryComplex();
    void DetachComplex();
    void UpdateTurretAngle(int32_t turret_angle, bool redraw = false);
    void UpdatePinCount(int32_t grid_x, int32_t grid_y, int32_t pin_units);
    void Attack(int32_t grid_x, int32_t grid_y);
    SmartPointer<UnitInfo> MakeCopy();
    void MoveFinished(bool mode = true);
    void BlockedOnPathRequest(bool mode = true, bool skip_notification = false);
    void RestoreOrders();
    void FollowUnit();
    void InitStealthStatus();
    void Draw(uint16_t team);
    void DrawStealth(uint16_t team);
    void StopMovement();
    void SpotByTeam(uint16_t team);
    static void GetVersion(char* text, int32_t version);
    int32_t GetRaw();
    int32_t GetRawFreeCapacity();
    void TransferRaw(int32_t amount);
    int32_t GetFuel();
    int32_t GetFuelFreeCapacity();
    void TransferFuel(int32_t amount);
    int32_t GetGold();
    int32_t GetGoldFreeCapacity();
    void TransferGold(int32_t amount);
    int32_t GetTurnsToRepair();
    int32_t Repair(int32_t materials);
    void Resupply();
    int32_t GetExperience();
    int32_t GetRawConsumptionRate();
    void UpdateProduction();
    int32_t GetNormalRateBuildCost() const;
    SmartObjectArray<ResourceID> GetBuildList();
    ResourceID GetConstructedUnitType() const;
    void CancelBuilding();
    bool IsUpgradeAvailable();
    void DeployConstructionSiteUtilities(ResourceID unit_type);
    int32_t GetMaxAllowedBuildRate();
    void StartBuilding();
    void SpawnNewUnit();
    void ReadPacket(NetPacket& packet);
    void WritePacket(NetPacket& packet);
    void Reload(UnitInfo* parent);
    bool Upgrade(UnitInfo* parent);
    void BuildOrder();
    void MoveInTransitUnitInMapHash(int32_t grid_x, int32_t grid_y);
    void RemoveInTransitUnitFromMapHash();
    void GetBounds(Rect* bounds);
    void BusyWaitOrder();
    void ScheduleDelayedTasks(bool priority);
    Task* GetTask() const;
    SmartList<Task>& GetTasks();
    void AddDelayedTask(Task* task);
    void RemoveDelayedTask(Task* task);
    void AddTask(Task* task);
    void RemoveTasks();
    void RemoveTask(Task* task, bool mode = true);
    bool IsReadyForOrders(Task* task);
    int32_t GetAttackRange();
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
    void ChangeTeam(uint16_t target_team);
    bool AttemptSideStep(int32_t grid_x, int32_t grid_y, int32_t angle);
    int32_t GetTurnsToBuild(ResourceID unit_type, int32_t build_speed_multiplier, int32_t* turns_to_build);
    void Init();
    [[nodiscard]] UnitInfo* GetParent() const noexcept;
    void SetParent(UnitInfo* const parent) noexcept;

    struct PopupFunctions* popup;
    const std::vector<SoundElement>* sound_table;
    uint32_t flags;
    uint16_t x;
    uint16_t y;
    int16_t grid_x;
    int16_t grid_y;
    Point attack_site;
    ColorIndex* color_cycling_lut;
    uint8_t team;
    uint8_t unit_id;
    uint8_t brightness;
    uint8_t angle;
    uint8_t max_velocity;
    uint8_t velocity;
    uint8_t scaler_adjust;
    uint8_t turret_angle;
    int8_t turret_offset_x;
    int8_t turret_offset_y;
    int16_t total_images;
    int16_t image_base;
    int16_t turret_image_base;
    int16_t firing_image_base;
    int16_t connector_image_base;
    int16_t image_index_max;
    int16_t move_to_grid_x;
    int16_t move_to_grid_y;
    int16_t fire_on_grid_x;
    int16_t fire_on_grid_y;
    uint8_t build_time;
    uint8_t total_mining;
    uint8_t raw_mining;
    uint8_t fuel_mining;
    uint8_t gold_mining;
    uint8_t raw_mining_max;
    uint8_t gold_mining_max;
    uint8_t fuel_mining_max;
    uint8_t hits;
    uint8_t speed;
    uint8_t group_speed;
    uint8_t shots;
    uint8_t move_and_fire;
    int16_t storage;
    int16_t experience;
    int16_t transfer_cargo;
    uint8_t stealth_dice_roll;
    uint8_t ammo;
    uint8_t targeting_mode;
    uint8_t enter_mode;
    uint8_t cursor;
    int8_t recoil_delay;
    uint8_t delayed_reaction;
    bool damaged_this_turn;
    bool disabled_reaction_fire;
    bool auto_survey;
    uint8_t research_topic;
    uint8_t moved;
    bool bobbed;
    uint8_t engine;
    uint8_t weapon;
    uint8_t move_fraction;
    SmartPointer<UnitPath> path;
    uint16_t connectors;
    uint8_t shake_effect_state;
    SmartPointer<UnitValues> base_values;
    SmartPointer<Complex> complex;
    SmartObjectArray<ResourceID> build_list;
    uint16_t build_rate;
    uint8_t repeat_build;
    uint16_t id;
    SmartList<UnitInfo>* unit_list;
    SmartPointer<UnitInfo> enemy_unit;
    SmartList<Task> tasks;
    SmartList<Task> delayed_tasks;
    bool in_transit;
    Point last_target;
    int16_t pin_count;
    bool tasks_enabled;
    uint8_t laying_state;
    uint8_t visible_to_team[PLAYER_TEAM_MAX];
    uint8_t spotted_by_team[PLAYER_TEAM_MAX];
    Rect sprite_bounds;
    Rect shadow_bounds;
    int16_t image_index;
    int16_t turret_image_index;
    Point shadow_offset;
    uint32_t ai_state_bits;
};

#endif /* UNITINFO_HPP */
