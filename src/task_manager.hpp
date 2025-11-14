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

#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#include "reminders.hpp"
#include "taskobtainunits.hpp"
#include "unitinfo.hpp"

/**
 * \class TaskManager
 * \brief Central coordinator for AI task execution and scheduling.
 *
 * The TaskManager orchestrates AI behavior by managing a collection of tasks that represent high-level strategic goals
 * (attack, defend, build, mine, etc.). It implements a callback-based scheduling system where tasks schedule their own
 * activities through reminders - deferred callbacks that execute task-specific logic at appropriate times.
 *
 * Core Architecture:
 * - **Tasks**: Autonomous AI goals that manage units and make strategic decisions. Each task independently determines
 * what actions to perform and when to schedule them.
 * - **Reminders**: Deferred task callbacks that execute task activities at scheduled times. They represent task work
 * items awaiting execution. Types include: turn start/end, unit availability, movement completion, and attack actions.
 * - **Priority Queues**: Separate queues for normal and priority reminders ensure critical operations (e.g., combat
 * reactions) are processed before routine activities.
 * - **Unit Requests**: Coordinate unit production and allocation across competing tasks.
 *
 * Scheduling Model:
 * Tasks schedule their own activities by creating reminders. BeginTurn/EndTurn are lifecycle hooks that trigger tasks
 * to schedule turn-start and turn-end work. The actual execution timing is controlled by the game loop calling
 * ExecuteReminders(), which processes pending task callbacks until time budget exhausted.
 *
 * Main Responsibilities:
 * - Manage task lifecycle (registration, execution coordination, removal)
 * - Process reminder queues to execute pending task activities
 * - Monitor and reassign units when battlefield conditions change
 * - Coordinate unit production requests between tasks
 * - Assign idle units to appropriate tasks based on strategic needs
 */
class TaskManager {
    SmartList<Task> tasks;
    SmartList<TaskObtainUnits> unit_requests;
    SmartList<Reminder> normal_reminders;
    SmartList<Reminder> priority_reminders;
    SmartList<UnitInfo> units_to_check;
    uint32_t reminder_counter;

    bool IsUnitNeeded(ResourceID unit_type, uint16_t team, uint16_t task_priority);

public:
    /**
     * \brief Constructs a new TaskManager instance.
     *
     * Initializes all internal lists and counters for task and reminder management.
     */
    TaskManager();

    /**
     * \brief Destructs the TaskManager instance.
     *
     * Cleans up all managed tasks, reminders, and associated resources.
     */
    ~TaskManager();

    /**
     * \brief Checks if any tasks for the specified team are currently processing.
     *
     * \param team The team ID to check for active task processing.
     * \return True if tasks are actively thinking/processing, false otherwise.
     */
    bool AreTasksThinking(uint16_t team);

    /**
     * \brief Processes computer-controlled units that require reaction checks.
     *
     * Evaluates queued units to determine if they need task reassignment or
     * reactions to changed battlefield conditions.
     */
    void CheckComputerReactions();

    /**
     * \brief Identifies and collects potential enemy targets for the specified unit.
     *
     * \param unit Pointer to the unit for which to gather potential attack targets.
     */
    void CollectPotentialAttackTargets(UnitInfo* unit);

    /**
     * \brief Adds a unit to the queue for later reaction evaluation.
     *
     * \param unit Reference to the unit that should be checked during the next reaction phase.
     */
    void EnqueueUnitForReactionCheck(UnitInfo& unit);

    /**
     * \brief Requests construction of a building at the specified location.
     *
     * \param unit_type The resource ID of the building type to construct.
     * \param team The team ID that will own the building.
     * \param site The map coordinates where the building should be placed.
     * \param task Pointer to the task requesting the building construction.
     */
    void CreateBuilding(ResourceID unit_type, uint16_t team, Point site, Task* task);

    /**
     * \brief Requests creation of a mobile unit at the specified location.
     *
     * \param unit_type The resource ID of the unit type to create.
     * \param team The team ID that will own the unit.
     * \param site The map coordinates where the unit should be created.
     * \param task Pointer to the task requesting the unit creation.
     */
    void CreateUnit(ResourceID unit_type, uint16_t team, Point site, Task* task);

    /**
     * \brief Adds a task to the active task list.
     *
     * \param task Reference to the task to be registered and managed.
     */
    void AppendTask(Task& task);

    /**
     * \brief Schedules a task callback (reminder) for deferred execution.
     *
     * Reminders are task-created callbacks that represent scheduled work items. They are not timer-based; instead, they
     * queue task activities to be executed when the game loop calls ExecuteReminders(). Priority reminders (e.g.,
     * combat reactions) are processed before normal reminders.
     *
     * \param reminder Pointer to the reminder (task callback) to schedule.
     * \param priority If true, adds to priority queue for critical operations. Default is false.
     */
    void AppendReminder(Reminder* reminder, bool priority = false);

    /**
     * \brief Executes pending task callbacks (reminders) in priority order.
     *
     * Processes the reminder queues, executing task-scheduled activities. Priority reminders (typically combat and
     * reaction operations) are processed first, followed by normal reminders. Execution continues until all reminders
     * are processed or the frame time budget is exhausted.
     *
     * \return True if reminders were executed successfully, false otherwise.
     */
    bool ExecuteReminders();

    /**
     * \brief Lifecycle hook that signals tasks to schedule turn-start activities.
     *
     * Called once when a team's game turn begins. Marks all tasks (except transport tasks) as needing processing by
     * setting their processing gate flags, then triggers each task's RemindTurnStart() method to schedule turn-start
     * callbacks (reminders) for later execution. This ensures that each task's turn-start logic will execute once when
     * ExecuteReminders() processes the scheduled callbacks. The processing gate prevents redundant execution until the
     * next turn.
     *
     * \param team The team ID beginning their game turn.
     */
    void BeginTurn(uint16_t team);

    /**
     * \brief Lifecycle hook that signals tasks to schedule turn-end activities.
     *
     * Called late in a team's game turn. It triggers each task's RemindTurnEnd() method, which schedules turn-end
     * callbacks (reminders) for cleanup and finalization work. The actual work executes later when ExecuteReminders()
     * is called.
     *
     * \param team The team ID ending their game turn.
     */
    void EndTurn(uint16_t team);

    /**
     * \brief Marks all tasks belonging to the specified team as needing processing.
     *
     * Sets the processing gate flag for all tasks owned by the team, allowing their conditional logic to execute on
     * the next processing cycle. This is typically called when team flags or strategic conditions change mid-turn,
     * requiring tasks to re-evaluate their state and potentially perform work that would otherwise be gated until the
     * next turn. Unlike BeginTurn(), this only sets the processing flags without scheduling turn-start reminders.
     *
     * \param team The team ID whose tasks should be marked for processing.
     */
    void MarkTasksForProcessing(uint16_t team);

    /**
     * \brief Clears all tasks, reminders, and pending operations.
     *
     * Removes all managed data structures, typically used when starting a new game or resetting the AI state.
     */
    void Clear();

    /**
     * \brief Removes all tasks associated with a unit and schedules availability notification.
     *
     * \param unit Pointer to the unit whose tasks should be cleared.
     * \param priority If true, schedules availability reminder with priority. Default is false.
     */
    void ClearUnitTasksAndRemindAvailable(UnitInfo* unit, bool priority = false);

    /**
     * \brief Assigns an appropriate task to an idle or unassigned unit.
     *
     * Evaluates the unit's capabilities and current strategic needs to determine
     * the most suitable task assignment.
     *
     * \param unit Pointer to the unit requiring task assignment.
     */
    void FindTaskForUnit(UnitInfo* unit);

    /**
     * \brief Removes a task from the active task list.
     *
     * \param task Reference to the task to be removed and deregistered.
     */
    void RemoveTask(Task& task);

    /**
     * \brief Handles cleanup when a unit is destroyed.
     *
     * Removes the unit from all tasks, reminders, and pending queues.
     *
     * \param unit Pointer to the unit that has been destroyed.
     */
    void RemoveDestroyedUnit(UnitInfo* unit);

    /**
     * \brief Notifies the task manager that a unit has been spotted by a team.
     *
     * Triggers evaluation of whether any tasks need to respond to the newly
     * spotted unit (e.g., attack or defense tasks).
     *
     * \param unit Pointer to the unit that was spotted.
     */
    void AddSpottedUnit(UnitInfo* unit);

    /**
     * \brief Gets the total count of pending reminders.
     *
     * \return The combined count of normal and priority reminders.
     */
    uint32_t GetRemindersCount() const;

    /**
     * \brief Provides access to the internal task list.
     *
     * \return Reference to the smart list containing all active tasks.
     */
    SmartList<Task>& GetTaskList();
};

extern class TaskManager TaskManager;

#endif /* TASK_MANAGER_HPP */
