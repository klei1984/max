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

#ifndef TASKS_HPP
#define TASKS_HPP

#include <string>
#include <string_view>

#include "resource_manager.hpp"
#include "smartobjectarray.hpp"
#include "smartpointer.hpp"
#include "zone.hpp"

enum : uint8_t {
    TaskType_TaskActivate = 0,
    TaskType_TaskAssistMove = 1,
    TaskType_TaskAttack = 2,
    TaskType_TaskAttackReserve = 3,
    TaskType_TaskAutoSurvey = 4,

    TaskType_TaskCheckAssaults = 6,
    TaskType_TaskClearZone = 7,
    TaskType_TaskCreateUnit = 8,
    TaskType_TaskCreateBuilding = 9,
    TaskType_TaskEscort = 10,
    TaskType_TaskExplore = 11,
    TaskType_TaskDefenseAssistant = 12,
    TaskType_TaskDefenseReserve = 13,
    TaskType_TaskDump = 14,
    TaskType_TaskFindMines = 15,
    TaskType_TaskFindPath = 16,
    TaskType_TaskFrontalAttack = 17,
    TaskType_TaskGetMaterials = 18,
    TaskType_TaskHabitatAssistant = 19,
    TaskType_TaskKillUnit = 20,
    TaskType_TaskManageBuildings = 21,
    TaskType_TaskMineAssisstant = 22,
    TaskType_TaskMove = 23,
    TaskType_TaskMoveHome = 24,
    TaskType_TaskObtainUnits = 25,
    TaskType_TaskPlaceMines = 26,
    TaskType_TaskConnectionAssistant = 27,
    TaskType_TaskRadarAssistant = 28,
    TaskType_TaskFrontierAssistant = 29,
    TaskType_TaskPowerAssistant = 30,

    TaskType_TaskReload = 33,
    TaskType_TaskRemoveMines = 34,
    TaskType_TaskRemoveRubble = 35,
    TaskType_TaskRendezvous = 36,
    TaskType_TaskRepair = 37,
    TaskType_TaskRetreat = 38,
    TaskType_TaskSearchDestination = 39,
    TaskType_TaskScavenge = 40,

    TaskType_TaskSupportAttack = 42,
    TaskType_TaskSurvey = 43,

    TaskType_TaskTransport = 45,
    TaskType_TaskUpdateTerrain = 46,
    TaskType_TaskUpgrade = 47,
    TaskType_TaskWaitToAttack = 48
};

// Task Urgency Scale (the original binary calls this field the task "flags").
//
// A priority is one 16-bit scalar; the LOWER the value, the MORE urgent the task. The high byte is the urgency tier
// (rank), the low byte a fine bias within a tier, always applied with +/-. Urgency decides unit contention, not
// execution order: TaskManager's task list is FIFO. A task may take a unit from another task only if the owner's
// value is numerically greater (Task::ComparePriority > 0), and a freed unit goes to the pending request with the
// numerically smallest value. ComparePriority compares coarsely by the static tier outside the
// +/-TASK_PRIORITY_TOLERANCE window (saturating to +/-1) and finely by the dynamic virtual GetPriority() inside it.
enum : uint16_t {
    // Comparability window of Task::ComparePriority; also the "least urgent in my tier" penalty a task without
    // targets reports through GetPriority()
    TASK_PRIORITY_TOLERANCE = 0xFF,

    // Extracts the tier byte from an effective priority (used to merge attack tasks of the same tier)
    TASK_PRIORITY_MASK = 0xFF00,

    // Low-byte offsets (within-tier bias)
    TASK_PRIORITY_OFFSET_HALF_TIER = 0x80,     // subtracted: a prerequisite child build (water platform) outranks its
                                               // parent building; added: boardwalks trail TASK_PRIORITY_BRIDGE_BASE
    TASK_PRIORITY_OFFSET_BRIDGE = 0xC8,        // subtracted: access bridges outrank the factory they serve; each
                                               // further bridge of the chain adds +1 on top
    TASK_PRIORITY_OFFSET_CENSUS_SLACK = 0xFA,  // added to the query priority when censusing pending work, so peers up
                                               // to ~one tier less urgent still count against the cap

    // Emergency reactions (most urgent)
    TASK_PRIORITY_EMERGENCY = 0x0000,       // retreat from danger, clear a landing zone, terrain bookkeeping
    TASK_PRIORITY_CHECK_ASSAULTS = 0x001E,  // air assault monitoring; comparable with the emergency tier, just behind

    // Site clearing - blocked build sites stall the whole economy
    TASK_PRIORITY_REMOVE_RUBBLE_LOW = 0x0100,  // land mine removal at build sites
    TASK_PRIORITY_REMOVE_RUBBLE = 0x0200,      // rubble removal at build sites

    // Combat operations and base construction
    TASK_PRIORITY_FRONTAL_ATTACK = 0x0400,        // massed frontal assault
    TASK_PRIORITY_BUILDING_LOW = 0x0500,          // first mining station of a new base
    TASK_PRIORITY_BUILDING_LIGHT_PLANT = 0x0700,  // light plant construction
    TASK_PRIORITY_BUILDING_CONNECTOR = 0x0800,    // connector (CNCT_4W) construction
    TASK_PRIORITY_BUILDING_DUMP = 0x0A00,         // storage (ADUMP, FDUMP, GOLDSM) construction
    TASK_PRIORITY_BUILDING_POWER = 0x0B00,        // power station/generator construction
    TASK_PRIORITY_BUILDING_HABITAT = 0x0C00,      // habitat construction
    TASK_PRIORITY_BUILDING_RADAR = 0x0D00,        // radar construction
    TASK_PRIORITY_BUILDING_MEDIUM = 0x0E00,       // replacement mining station
    TASK_PRIORITY_ESCORT = 0x0F00,                // unit escort and protection
    TASK_PRIORITY_ATTACK_MOBILE = 0x1000,         // attack escalated against mobile threats; also the fallback tier
                                                  // of a parentless TaskMove
    TASK_PRIORITY_REPAIR = 0x1100,                // unit repair, reload and upgrade
    TASK_PRIORITY_BUILDING_DEFENSE = 0x1200,      // defense turret construction

    // 0x1300 plays two roles; both names denote the same tier
    TASK_PRIORITY_BUILDING_HEAVY_PLANT = 0x1300,   // tier: first land/air/ship factory and training hall
    TASK_PRIORITY_LAND_ACCESS_THRESHOLD = 0x1300,  // threshold: requests at least this urgent waive the land-access
                                                   // siting constraint

    TASK_PRIORITY_BUILDING_REPAIR_SHOP = 0x1400,    // repair shop (DEPOT, HANGAR, DOCK) construction
    TASK_PRIORITY_BUILDING_UPGRADES = 0x1500,       // research facility construction
    TASK_PRIORITY_BUILDING_GREENHOUSE = 0x1600,     // greenhouse construction
    TASK_PRIORITY_ATTACK_PRIORITY_TARGET = 0x1700,  // attack escalated against GREENHSE/MININGST of a leading enemy

    // 0x1800 plays two roles; both names denote the same tier
    TASK_PRIORITY_BUILDING_EXPANSION = 0x1800,  // tier: strategy-driven factory expansion
    TASK_PRIORITY_FOLLOW_DEFENSE = 0x1800,      // threshold: defense reserve units follow attackers this urgent

    TASK_PRIORITY_DEFENSE_RESERVE = 0x1900,   // defensive unit reserve
    TASK_PRIORITY_FRONTIER = 0x1A00,          // frontier mining and mine laying
    TASK_PRIORITY_EXPLORE = 0x1B00,           // map exploration
    TASK_PRIORITY_BRIDGE_BASE = 0x1C00,       // bridge construction base tier
    TASK_PRIORITY_MANAGE_BUILDINGS = 0x1D00,  // building management (also BARRACKS construction)
    TASK_PRIORITY_ATTACK_DEFAULT = 0x1F00,    // default attack task tier

    // 0x2000 plays two roles; both names denote the same tier
    TASK_PRIORITY_FIND_MINES = 0x2000,     // tier: enemy minefield sweeping
    TASK_PRIORITY_FOLLOW_ATTACK = 0x2000,  // threshold: attack reserve units follow attackers this urgent

    // Recon and logistics (weakest claims on units)
    TASK_PRIORITY_SCAVENGE = 0x2100,             // resource scavenging
    TASK_PRIORITY_AUTO_SURVEY = 0x2200,          // surveying (TaskAutoSurvey and TaskSurvey)
    TASK_PRIORITY_GET_RESOURCE = 0x2300,         // orphan default of resource fetching; replaced by the parent's
                                                 // priority when one exists
    TASK_PRIORITY_SUPPORT_ATTACK = 0x2500,       // attack support
    TASK_PRIORITY_ATTACK_RESERVE_POOL = 0x2600,  // attack reserve pool
    TASK_PRIORITY_ASSIST_MOVE = 0x2800,          // air transport assistance
    TASK_PRIORITY_MOVE = 0x2900,                 // weakest claim: bare callback moves and orphaned unit requests
};

class UnitInfo;

std::string_view Task_GetName(Task* task);
void Task_RemindMoveFinished(UnitInfo* unit, bool priority = false);
bool Task_IsReadyToTakeOrders(UnitInfo* unit);
void Task_RemoveMovementTasks(UnitInfo* unit);
bool Task_ShouldReserveShot(UnitInfo* unit, Point site);
bool Task_IsUnitDoomedToDestruction(UnitInfo* unit, int32_t caution_level);
bool Task_RetreatIfNecessary(Task* task, UnitInfo* unit, int32_t caution_level);
bool Task_RetreatFromDanger(Task* task, UnitInfo* unit, int32_t caution_level);
bool Task_IsAdjacent(UnitInfo* unit, int16_t grid_x, int16_t grid_y);
int32_t Task_EstimateTurnsTillMissionEnd();
int32_t Task_GetReadyUnitsCount(uint16_t team, ResourceID unit_type);

/**
 * \brief Abstract base class for all AI task types.
 *
 * The Task class provides the foundation for the AI's task-based decision system. Tasks represent strategic and
 * tactical objectives that the AI needs to accomplish, such as building structures, attacking enemies, defending
 * positions, or gathering resources.
 *
 * Tasks are organized in a hierarchy where parent tasks can delegate work to child tasks. Each task has a priority that
 * determines resource allocation and unit assignment precedence. The AI processes tasks at the beginning and end of
 * each turn, allowing them to evaluate conditions, issue orders to units, and respond to game events.
 *
 * Concrete task implementations must override the pure virtual methods to define their specific behavior and
 * requirements.
 *
 * \note This is a pure virtual class - you must derive from it to create concrete task types.
 * \see TaskType enum for all available task type identifiers.
 * \see TASK_PRIORITY constants for standard priority values.
 */
class Task : public SmartObject {
    static uint32_t task_count;
    static uint32_t task_id;

    bool m_needs_processing;
    bool m_scheduled_for_turn_start;
    bool m_scheduled_for_turn_end;

protected:
    uint32_t m_id;
    uint16_t m_team;
    uint16_t m_base_priority;
    SmartPointer<Task> m_parent;

public:
    /**
     * \brief Constructs a new Task with specified team, parent, and priority.
     *
     * \param team The team ID (0-3) that owns this task.
     * \param parent Pointer to the parent task, or nullptr if this is a root task.
     * \param priority Base priority value for this task. See TASK_PRIORITY constants.
     */
    Task(uint16_t team, Task* parent, uint16_t priority);

    /**
     * \brief Copy constructor for creating a duplicate task.
     *
     * \param other The task to copy from.
     */
    Task(const Task& other);

    /**
     * \brief Virtual destructor for proper cleanup of derived task types.
     */
    virtual ~Task();

    /**
     * \brief Schedules this task to be processed at the end of the current turn.
     *
     * \param priority If true, adds the task to the front of the processing queue.
     *                 If false, adds it to the back.
     */
    void RemindTurnEnd(bool priority = false);

    /**
     * \brief Schedules this task to be processed at the start of the next turn.
     *
     * \param priority If true, adds the task to the front of the processing queue.
     *                 If false, adds it to the back.
     */
    void RemindTurnStart(bool priority = false);

    /**
     * \brief Checks if this task needs immediate processing.
     *
     * \return true if the task requires processing, false otherwise.
     */
    bool IsProcessingNeeded() const;

    /**
     * \brief Sets whether this task needs processing.
     *
     * \param value true to mark as needing processing, false otherwise.
     */
    void SetProcessingNeeded(bool value);

    /**
     * \brief Checks if this task is scheduled for turn start processing.
     *
     * \return true if scheduled for turn start, false otherwise.
     */
    bool IsScheduledForTurnStart() const;

    /**
     * \brief Changes the turn start scheduling status for this task.
     *
     * \param value true to schedule for turn start, false to unschedule.
     */
    void ChangeIsScheduledForTurnStart(bool value);

    /**
     * \brief Checks if this task is scheduled for turn end processing.
     *
     * \return true if scheduled for turn end, false otherwise.
     */
    bool IsScheduledForTurnEnd() const;

    /**
     * \brief Changes the turn end scheduling status for this task.
     *
     * \param value true to schedule for turn end, false to unschedule.
     */
    void ChangeIsScheduledForTurnEnd(bool value);

    /**
     * \brief Gets the team that owns this task.
     *
     * \return Team ID.
     */
    uint16_t GetTeam() const;

    /**
     * \brief Gets the unique identifier for this task instance.
     *
     * \return Unique task ID assigned during construction.
     */
    uint32_t GetId() const;

    /**
     * \brief Gets the parent task in the task hierarchy.
     *
     * \return Pointer to parent task, or nullptr if this is a root task.
     */
    Task* GetParent();

    /**
     * \brief Sets or changes the parent task for this task.
     *
     * \param task Pointer to the new parent task, or nullptr to clear the parent.
     */
    void SetParent(Task* task);

    /**
     * \brief Sets the base priority for this task.
     *
     * \param priority New priority value. Lower values indicate more urgent tasks.
     *                 See TASK_PRIORITY constants for standard values.
     */
    void SetPriority(uint16_t priority);

    /**
     * \brief Compares this task's urgency against another priority value.
     *
     * Outside the +/-TASK_PRIORITY_TOLERANCE window the result saturates to +/-1 based on the static base priority;
     * inside it, the signed difference of the dynamic virtual GetPriority() and the argument is returned.
     *
     * \param priority The priority value to compare against.
     * \return Negative if this task is more urgent (numerically lower), 0 if equal, positive if less urgent.
     */
    int16_t ComparePriority(uint16_t priority);

    /**
     * \brief Determines the primary position/location associated with this task.
     *
     * \return Point containing grid coordinates (x, y) of the task's focus area.
     */
    Point DeterminePosition();

    /**
     * \brief Checks if a unit can be transferred from this task to another task.
     *
     * \param unit The unit to check for transferability.
     * \return true if the unit can be transferred away from this task, false otherwise.
     */
    virtual bool IsUnitTransferable(UnitInfo& unit);

    /**
     * \brief Checks if a unit is suitable for use by this task.
     *
     * \param unit The unit to evaluate.
     * \return true if the unit meets the task's requirements, false otherwise.
     */
    virtual bool IsUnitUsable(UnitInfo& unit);

    /**
     * \brief Gets the caution level this task should use for the specified unit.
     *
     * Caution level affects risk assessment and retreat decisions. Higher values make the unit more cautious and likely
     * to retreat from danger.
     *
     * \param unit The unit to get caution level for.
     * \return Caution level value (typically 0-10, where 0 is reckless and 10 is very cautious).
     */
    virtual int32_t GetCautionLevel(UnitInfo& unit);

    /**
     * \brief Gets the current effective priority of this task.
     *
     * The effective priority may differ from the base priority depending on task state and conditions.
     *
     * \return Current priority value.
     */
    virtual uint16_t GetPriority() const;

    /**
     * \brief Writes a brief status description of the task.
     *
     * Used for debugging and logging the current state of the task.
     *
     * \return String containing the status description.
     * \note Pure virtual - must be implemented by derived classes.
     */
    virtual std::string WriteStatusLog() const = 0;

    /**
     * \brief Gets the bounding rectangle that encompasses this task's area of operation.
     *
     * \param bounds Pointer to a Rect structure to fill with the bounds.
     * \return Pointer to the bounds parameter (allows chaining).
     */
    virtual Rect* GetBounds(Rect* bounds);

    /**
     * \brief Gets the task type identifier.
     *
     * \return Task type constant from the TaskType enum (e.g., TaskType_TaskAttack).
     * \note Pure virtual - must be implemented by derived classes.
     */
    virtual uint8_t GetType() const = 0;

    /**
     * \brief Checks if this task is still needed and should continue execution.
     *
     * Tasks may become obsolete due to completed objectives, destroyed targets, or changed strategic priorities.
     *
     * \return true if the task should continue, false if it should be removed.
     */
    virtual bool IsNeeded();

    /**
     * \brief Checks if this task is currently in a thinking/planning state.
     *
     * Thinking tasks may be waiting for information or planning their next actions.
     *
     * \return true if the task is thinking, false otherwise.
     */
    virtual bool IsThinking();

    /**
     * \brief Adds a unit to this task's control.
     *
     * The task will direct this unit to work toward the task's objectives.
     *
     * \param unit Reference to the unit to add to this task.
     */
    virtual void AddUnit(UnitInfo& unit);

    /**
     * \brief Initializes the task after construction.
     *
     * Called once to set up initial state, create child tasks, or perform initial analysis.
     */
    virtual void Init();

    /**
     * \brief Processes task logic at the beginning of a turn.
     *
     * Called each turn to evaluate conditions, issue orders, and update state before units execute their orders.
     */
    virtual void BeginTurn();

    /**
     * \brief Notifies this task that a child task has completed.
     *
     * Allows parent tasks to react to child task completion, potentially creating new child tasks or updating their own
     * state.
     *
     * \param task Pointer to the child task that completed.
     */
    virtual void ChildComplete(Task* task);

    /**
     * \brief Processes task logic at the end of a turn.
     *
     * Called each turn after units have executed their orders, allowing tasks to evaluate results and plan for the next
     * turn.
     */
    virtual void EndTurn();

    /**
     * \brief Attempts to exchange the operator of a building or complex unit.
     *
     * \param unit Reference to the unit whose operator should be exchanged.
     * \return true if the operator was successfully exchanged, false otherwise.
     */
    virtual bool ExchangeOperator(UnitInfo& unit);

    /**
     * \brief Executes task-specific orders for a unit.
     *
     * Called to have the task issue immediate orders to the specified unit.
     *
     * \param unit Reference to the unit to issue orders to.
     * \return true if orders were issued, false otherwise.
     */
    virtual bool Execute(UnitInfo& unit);

    /**
     * \brief Removes this task from the task system.
     *
     * Cleans up resources, removes all units from the task, and notifies the parent task if one exists.
     */
    virtual void RemoveSelf();

    /**
     * \brief Checks for and handles reactive behaviors.
     *
     * Allows tasks to respond to immediate threats or opportunities that require quick reaction.
     *
     * \return true if a reaction was triggered, false otherwise.
     */
    virtual bool CheckReactions();

    /**
     * \brief Removes a unit from this task's control.
     *
     * \param unit Reference to the unit to remove.
     */
    virtual void RemoveUnit(UnitInfo& unit);

    /**
     * \brief Handles the event when cargo is transferred involving a task unit.
     *
     * \param unit Reference to the unit involved in the cargo transfer.
     */
    virtual void EventCargoTransfer(UnitInfo& unit);

    /**
     * \brief Handles the event when a unit assigned to this task is destroyed.
     *
     * \param unit Reference to the unit that was destroyed.
     */
    virtual void EventUnitDestroyed(UnitInfo& unit);

    /**
     * \brief Handles the event when a unit is loaded into a transport.
     *
     * \param unit1 Reference to the unit being loaded.
     * \param unit2 Reference to the transport unit.
     */
    virtual void EventUnitLoaded(UnitInfo& unit1, UnitInfo& unit2);

    /**
     * \brief Handles the event when an enemy unit is spotted.
     *
     * \param unit Reference to the enemy unit that was spotted.
     */
    virtual void EventEnemyUnitSpotted(UnitInfo& unit);

    /**
     * \brief Handles the event when a unit is unloaded from a transport.
     *
     * \param unit1 Reference to the unit being unloaded.
     * \param unit2 Reference to the transport unit.
     */
    virtual void EventUnitUnloaded(UnitInfo& unit1, UnitInfo& unit2);

    /**
     * \brief Handles the event when a zone is cleared or fails to be cleared.
     *
     * \param zone Pointer to the zone that was processed.
     * \param status true if the zone was successfully cleared, false otherwise.
     */
    virtual void EventZoneCleared(Zone* zone, bool status);
};

#endif /* TASKS_HPP */
