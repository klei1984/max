/* Copyright (c) 2026 M.A.X. Port Team
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

#include "order_log.hpp"

#if !defined(NDEBUG)

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "game_manager.hpp"
#include "hash.hpp"
#include "logwriter.hpp"
#include "resource_manager.hpp"
#include "resourcetable.hpp"
#include "settings.hpp"
#include "unitinfo.hpp"
#include "units_manager.hpp"
#include "world.hpp"

/* order_log.txt holds one record per line. Fields are tab separated decimal numbers unless noted, lines starting
 * with # are comments. The header names the orders, order states, unit types and anomaly kinds.
 *
 * S tick turn play_mode team_type...  a game started or was loaded, followed by its baseline anomalies
 * E tick turn                          the game ended
 * T tick turn active_team units        the turn or the active team changed
 * X tick id type team order0 state0 order1 state1 prior_order prior_state parent hits writer
 *                                      order or order state changed between two ticks, movement steps excluded,
 *                                      at most 64 per unit and turn
 * H turn order0 state0 order1 state1 writer count
 *                                      transitions since the previous H block, movement steps included
 * A tick kind baseline id a b          an anomaly, reported once per kind and unit, followed by U and R records
 * U id type team grid_x grid_y order state prior_order prior_state parent hits storage pins in_transit flags
 * R id tick order0 state0 order1 state1 writer
 *                                      the last transitions of a unit, oldest first
 *
 * A tick ends when the outermost UnitsManager_ProcessOrders() call returns. writer is the hexadecimal return
 * address of the outermost order API call or order setter that wrote the unit last in the tick, 0 when the write
 * bypassed the hooked functions. The anchor comment gives the runtime address of OrderLog_GameStart() to relocate
 * writer addresses for addr2line. flags is hexadecimal. parent is -1 without a parent. The unit lists of the
 * LISTED_TWICE kind are 0 mobile land and sea, 1 mobile air, 2 stationary, 3 ground cover, 4 particles.
 *
 * ai_log.txt starts the entries of each tick with a "# tick N" line, N matching the records of that tick here.
 */

static constexpr uint32_t ORDER_LOG_HISTORY_DEPTH = 16;
static constexpr uint32_t ORDER_LOG_UNIT_TURN_LINES = 64;
static constexpr uint32_t ORDER_LOG_STALL_TICKS = 2400;
static constexpr uint32_t ORDER_LOG_QUEUE_STALL_TICKS = 1200;
static constexpr uint32_t ORDER_LOG_STALE_PIN_TICKS = 240;
static constexpr uint32_t ORDER_LOG_PARENT_CHECK_TICKS = 8;
static constexpr uint32_t ORDER_LOG_ANOMALY_LIMIT = 20000;
static constexpr uint64_t ORDER_LOG_BULK_LIMIT = 256ull * 1024ull * 1024ull;
static constexpr uint64_t ORDER_LOG_SIZE_LIMIT = 320ull * 1024ull * 1024ull;

enum OrderLogAnomaly : uint8_t {
    ORDER_LOG_STALL,
    ORDER_LOG_QUEUE_STALL,
    ORDER_LOG_STORAGE,
    ORDER_LOG_ORPHAN_HELD,
    ORDER_LOG_ORPHAN_ENTERING,
    ORDER_LOG_ORPHAN_LANDING,
    ORDER_LOG_ORPHAN_UTILITY,
    ORDER_LOG_DANGLING_TARGET,
    ORDER_LOG_TARGET_MOVED,
    ORDER_LOG_TARGET_STORED,
    ORDER_LOG_DANGLING_PASSENGER,
    ORDER_LOG_PASSENGER_MOVED,
    ORDER_LOG_DANGLING_CLIENT,
    ORDER_LOG_PIN_NEGATIVE,
    ORDER_LOG_PIN_STALE,
    ORDER_LOG_ZOMBIE,
    ORDER_LOG_DEAD_ACTIVE,
    ORDER_LOG_GHOST_DEAD,
    ORDER_LOG_GHOST_CELL,
    ORDER_LOG_HELD_ON_MAP,
    ORDER_LOG_LISTED_TWICE,
};

static const char* const OrderLog_AnomalyNames[] = {
    "STALL",           "QUEUE_STALL",     "STORAGE",      "ORPHAN_HELD",   "ORPHAN_ENTERING",    "ORPHAN_LANDING",
    "ORPHAN_UTILITY",  "DANGLING_TARGET", "TARGET_MOVED", "TARGET_STORED", "DANGLING_PASSENGER", "PASSENGER_MOVED",
    "DANGLING_CLIENT", "PIN_NEGATIVE",    "PIN_STALE",    "ZOMBIE",        "DEAD_ACTIVE",        "GHOST_DEAD",
    "GHOST_CELL",      "HELD_ON_MAP",     "LISTED_TWICE",
};

static const char* const OrderLog_AnomalyHelp[] = {
    "unit blocks the end of turn in one order and state, a = ticks since its last change",
    "attack queue head did not change, a = ticks, b = queue length",
    "container storage differs from its held units, a = storage, b = held units",
    "stored unit of a destroyed container",
    "unit shrinking into a destroyed container",
    "aircraft landing into a destroyed hangar",
    "tape or cones of a destroyed builder",
    "stealth action against a destroyed target",
    "stealth target is not adjacent, a = distance",
    "stealth target is stored in a container",
    "air transport lifts a destroyed unit",
    "air transport lifts a unit from another cell",
    "air transport unloads a destroyed unit",
    "negative pin count, a = pins",
    "pinned while no attack is pending, a = pins, b = quiet ticks",
    "dying unit has hits, a = hits",
    "unit without hits is not dying",
    "map hash holds a destroyed unit, a b = cell",
    "map hash holds a unit at a cell it does not occupy, a b = cell",
    "unit stored in a container is still in the map hash, a b = cell",
    "unit is listed a second time, a = list of the first entry, b = list of the second entry",
};

static_assert(std::size(OrderLog_AnomalyNames) == ORDER_LOG_LISTED_TWICE + 1);
static_assert(std::size(OrderLog_AnomalyHelp) == ORDER_LOG_LISTED_TWICE + 1);

static const char* const OrderLog_OrderNames[] = {
    "AWAIT",
    "TRANSFORM",
    "MOVE",
    "FIRE",
    "BUILD",
    "ACTIVATE",
    "NEW_ALLOCATE",
    "POWER_ON",
    "POWER_OFF",
    "EXPLODE",
    "UNLOAD",
    "CLEAR",
    "SENTRY",
    "LAND",
    "TAKE_OFF",
    "LOAD",
    "IDLE",
    "REPAIR",
    "ORDER_17",
    "RELOAD",
    "TRANSFER",
    "HALT_BUILDING",
    "AWAIT_SCALING",
    "AWAIT_TAPE_POSITIONING",
    "AWAIT_STEAL_UNIT",
    "AWAIT_DISABLE_UNIT",
    "DISABLE",
    "MOVE_TO_UNIT",
    "UPGRADE",
    "LAY_MINE",
    "MOVE_TO_ATTACK",
    "HALT_BUILDING_2",
};

static_assert(std::size(OrderLog_OrderNames) == ORDER_COUNT_MAX);

static const char* const OrderLog_StateNames[] = {
    "INIT",
    "EXECUTING_ORDER",
    "READY_TO_EXECUTE_ORDER",
    "STORE",
    "PREPARE_STORE",
    "IN_PROGRESS",
    "IN_TRANSITION",
    "ISSUING_PATH",
    "READY_TO_FIRE",
    "FIRE_IN_PROGRESS",
    "STATE_10",
    "BUILD_IN_PROGRESS",
    "PATH_REQUEST_CANCEL",
    "BUILD_CANCEL",
    "DESTROY",
    "LOADING_IN_PROGRESS",
    "FINISH_LOADING",
    "UNLOADING_IN_PROGRESS",
    "FINISH_UNLOADING",
    "FINISH_LANDING",
    "FINISH_TAKE_OFF",
    "STATE_21",
    "PROGRESS_TRANSFORMING",
    "FINISH_TRANSFORMING",
    "CLEAR_PATH",
    "SELECT_SITE",
    "BUILD_CLEARING",
    "EXPLODE",
    "MOVE_INIT",
    "MOVE_GETTING_PATH",
    "BUILDING_READY",
    "UNIT_READY",
    "EXPAND",
    "SHRINK",
    "TAPE_POSITIONING_INIT",
    "TAPE_POSITIONING_ENTER",
    "TAPE_POSITIONING_DEINIT",
    "TAPE_POSITIONING_LEAVE",
    "ELEVATE",
    "LOWER",
    "ATTACK_PENDING",
    "ATTACK_BEGINNING",
    "NEW_ORDER",
    "INACTIVE",
    "PLACING_MINES",
    "REMOVING_MINES",
    "BUILD_ABORT",
};

static_assert(std::size(OrderLog_StateNames) == ORDER_STATE_BUILD_ABORT + 1);

struct OrderLogStep {
    uint32_t tick;
    uint8_t order[2];
    uint8_t state[2];
    uintptr_t writer;
};

struct OrderLogShadow {
    uint16_t id;
    uint16_t unit_type;
    uint8_t order;
    uint8_t state;
    uint8_t list_index;
    uint32_t changed_tick;
    uint32_t seen_tick;
    uint32_t quiet_pinned_ticks;
    uint32_t turn_serial;
    uint32_t turn_lines;
    uintptr_t writer;
    uint32_t step_count;
    OrderLogStep steps[ORDER_LOG_HISTORY_DEPTH];
};

struct OrderLogTransition {
    uint32_t pairs;
    uintptr_t writer;

    bool operator==(const OrderLogTransition& other) const noexcept = default;
};

struct OrderLogTransitionHash {
    size_t operator()(const OrderLogTransition& key) const noexcept {
        return std::hash<uint64_t>{}((static_cast<uint64_t>(key.pairs) << 32) ^ static_cast<uint64_t>(key.writer));
    }
};

struct OrderLogReport {
    OrderLogAnomaly kind;
    const UnitInfo* unit;

    bool operator==(const OrderLogReport& other) const noexcept = default;
};

struct OrderLogReportHash {
    size_t operator()(const OrderLogReport& key) const noexcept {
        return std::hash<const void*>{}(key.unit) ^ (static_cast<size_t>(key.kind) << 3);
    }
};

static LogWriter OrderLog_Writer;
static bool OrderLog_IsOpen;
static bool OrderLog_Enabled;
static bool OrderLog_BulkStopped;
static bool OrderLog_Stopped;
static uint64_t OrderLog_Size;
static SDL_ThreadID OrderLog_MainThread;
static uint32_t OrderLog_Tick;
static std::atomic<uint32_t> OrderLog_TickInProgress;
static int32_t OrderLog_TickDepth;
static int32_t OrderLog_IssuerDepth;
static const void* OrderLog_Issuer;
static uint32_t OrderLog_AnomalyCount;
static int32_t OrderLog_Turn;
static int32_t OrderLog_ActiveTeam;
static uint32_t OrderLog_TurnSerial;
static const UnitInfo* OrderLog_QueueHead;
static uint32_t OrderLog_QueueHeadTick;
static bool OrderLog_AttacksQuiet;

static std::unordered_map<const UnitInfo*, OrderLogShadow> OrderLog_Shadows;
static std::unordered_map<OrderLogTransition, uint32_t, OrderLogTransitionHash> OrderLog_Transitions;
static std::unordered_set<OrderLogReport, OrderLogReportHash> OrderLog_Reports;
static std::unordered_map<const UnitInfo*, int32_t> OrderLog_HeldUnits;
static std::vector<const UnitInfo*> OrderLog_Containers;

static SmartList<UnitInfo>* const OrderLog_UnitLists[] = {&UnitsManager_MobileLandSeaUnits,
                                                          &UnitsManager_MobileAirUnits, &UnitsManager_StationaryUnits,
                                                          &UnitsManager_GroundCoverUnits, &UnitsManager_ParticleUnits};

static void OrderLog_Line(bool is_bulk, const char* format, ...) noexcept {
    if (!OrderLog_IsOpen || OrderLog_Stopped || (is_bulk && OrderLog_BulkStopped)) {
        return;
    }

    char buffer[512];
    va_list args;

    va_start(args, format);
    int32_t length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (length <= 0) {
        return;
    }

    length = std::min<int32_t>(length, sizeof(buffer) - 1);

    OrderLog_Writer.Write(std::string_view(buffer, length));

    OrderLog_Size += length;

    if (!OrderLog_BulkStopped && OrderLog_Size >= ORDER_LOG_BULK_LIMIT) {
        OrderLog_BulkStopped = true;

        OrderLog_Writer.Write("# size limit reached, X and H records stop\n");
    }

    if (OrderLog_Size >= ORDER_LOG_SIZE_LIMIT) {
        OrderLog_Stopped = true;

        OrderLog_Writer.Write("# size limit reached, all records stop\n");
    }
}

static bool OrderLog_Open() noexcept {
    /* the writer thread creates the file and reports when it cannot */
    OrderLog_Writer.Open((ResourceManager_FilePathGamePref / "order_log.txt").lexically_normal(),
                         (ResourceManager_FilePathGamePref / "order_log.prev.txt").lexically_normal(), true);

    OrderLog_IsOpen = true;
    OrderLog_Size = 0;
    OrderLog_BulkStopped = false;
    OrderLog_Stopped = false;

    OrderLog_Line(false, "# max-port order log, format 1\n");
    OrderLog_Line(false, "# S tick turn play_mode team_type...\n");
    OrderLog_Line(false, "# W world map_width map_height\n");
    OrderLog_Line(false, "# E tick turn\n");
    OrderLog_Line(false, "# T tick turn active_team units\n");
    OrderLog_Line(false,
                  "# X tick id type team order0 state0 order1 state1 prior_order prior_state parent hits writer\n");
    OrderLog_Line(false, "# H turn order0 state0 order1 state1 writer count\n");
    OrderLog_Line(false, "# A tick kind baseline id a b\n");
    OrderLog_Line(false,
                  "# U id type team grid_x grid_y order state prior_order prior_state parent hits storage pins "
                  "in_transit flags\n");
    OrderLog_Line(false, "# R id tick order0 state0 order1 state1 writer\n");
    OrderLog_Line(false, "# X skips movement steps and stops after %u per unit and turn, H counts every transition\n",
                  ORDER_LOG_UNIT_TURN_LINES);
    OrderLog_Line(false, "# anchor %" PRIxPTR "\n", reinterpret_cast<uintptr_t>(&OrderLog_GameStart));

    for (uint32_t i = 0; i < std::size(OrderLog_OrderNames); ++i) {
        OrderLog_Line(false, "# order %u %s\n", i, OrderLog_OrderNames[i]);
    }

    for (uint32_t i = 0; i < std::size(OrderLog_StateNames); ++i) {
        OrderLog_Line(false, "# state %u %s\n", i, OrderLog_StateNames[i]);
    }

    for (uint32_t i = UNIT_START; i < UNIT_END; ++i) {
        OrderLog_Line(false, "# type %u %s\n", i, ResourceManager_ResourceIdList[i]);
    }

    for (uint32_t i = 0; i < std::size(OrderLog_AnomalyNames); ++i) {
        OrderLog_Line(false, "# kind %s %s\n", OrderLog_AnomalyNames[i], OrderLog_AnomalyHelp[i]);
    }

    return true;
}

static bool OrderLog_IsAlive(const UnitInfo* unit) noexcept {
    return unit->GetId() != 0xFFFF && Hash_UnitHash[unit->GetId()] == unit;
}

static bool OrderLog_IsStored(const UnitInfo* unit) noexcept {
    return unit->GetOrder() == ORDER_IDLE &&
           (unit->GetOrderState() == ORDER_STATE_STORE || unit->GetOrderState() == ORDER_STATE_PREPARE_STORE) &&
           unit->GetParent();
}

static bool OrderLog_IsContainer(ResourceID unit_type) noexcept {
    return unit_type == DEPOT || unit_type == HANGAR || unit_type == DOCK || unit_type == BARRACKS ||
           unit_type == SEATRANS || unit_type == AIRTRANS || unit_type == CLNTRANS;
}

static bool OrderLog_IsUtility(ResourceID unit_type) noexcept {
    return unit_type == LRGTAPE || unit_type == SMLTAPE || unit_type == LRGCONES || unit_type == SMLCONES;
}

/* the per unit conditions of Access_AreTaskEventsPending() */
static bool OrderLog_IsBlockingEndTurn(const UnitInfo* unit) noexcept {
    const auto order = unit->GetOrder();
    const auto state = unit->GetOrderState();

    return order == ORDER_FIRE || order == ORDER_EXPLODE || order == ORDER_ACTIVATE ||
           order == ORDER_AWAIT_TAPE_POSITIONING || order == ORDER_AWAIT_DISABLE_UNIT ||
           order == ORDER_AWAIT_STEAL_UNIT || order == ORDER_AWAIT_SCALING || state == ORDER_STATE_DESTROY ||
           ((order == ORDER_MOVE || order == ORDER_MOVE_TO_UNIT || order == ORDER_MOVE_TO_ATTACK) &&
            state != ORDER_STATE_EXECUTING_ORDER) ||
           state == ORDER_STATE_INIT;
}

static int32_t OrderLog_GetDistance(const UnitInfo* unit, const UnitInfo* target) noexcept {
    const int32_t size = (target->flags & BUILDING) ? 2 : 1;
    const int32_t distance_x = std::max({0, target->grid_x - unit->grid_x, unit->grid_x - (target->grid_x + size - 1)});
    const int32_t distance_y = std::max({0, target->grid_y - unit->grid_y, unit->grid_y - (target->grid_y + size - 1)});

    return std::max(distance_x, distance_y);
}

static void OrderLog_WriteUnit(const UnitInfo* unit) noexcept {
    const UnitInfo* parent = unit->GetParent();

    OrderLog_Line(false, "U\t%u\t%u\t%u\t%d\t%d\t%u\t%u\t%u\t%u\t%d\t%u\t%d\t%d\t%u\t%" PRIx32 "\n",
                  static_cast<uint32_t>(unit->GetId()), static_cast<uint32_t>(unit->GetUnitType()),
                  static_cast<uint32_t>(unit->team), static_cast<int32_t>(unit->grid_x),
                  static_cast<int32_t>(unit->grid_y), static_cast<uint32_t>(unit->GetOrder()),
                  static_cast<uint32_t>(unit->GetOrderState()), static_cast<uint32_t>(unit->GetPriorOrder()),
                  static_cast<uint32_t>(unit->GetPriorOrderState()),
                  parent ? static_cast<int32_t>(parent->GetId()) : -1, static_cast<uint32_t>(unit->hits),
                  static_cast<int32_t>(unit->storage), static_cast<int32_t>(unit->pin_count),
                  static_cast<uint32_t>(unit->in_transit), static_cast<uint32_t>(unit->flags));
}

static void OrderLog_WriteHistory(const UnitInfo* unit) noexcept {
    const auto it = OrderLog_Shadows.find(unit);

    if (it != OrderLog_Shadows.end()) {
        const OrderLogShadow& shadow = it->second;
        const uint32_t count = std::min(shadow.step_count, ORDER_LOG_HISTORY_DEPTH);

        for (uint32_t i = shadow.step_count - count; i < shadow.step_count; ++i) {
            const OrderLogStep& step = shadow.steps[i % ORDER_LOG_HISTORY_DEPTH];

            OrderLog_Line(false, "R\t%u\t%u\t%u\t%u\t%u\t%u\t%" PRIxPTR "\n", static_cast<uint32_t>(unit->GetId()),
                          step.tick, static_cast<uint32_t>(step.order[0]), static_cast<uint32_t>(step.state[0]),
                          static_cast<uint32_t>(step.order[1]), static_cast<uint32_t>(step.state[1]), step.writer);
        }
    }
}

static void OrderLog_Report(OrderLogAnomaly kind, const UnitInfo* unit, int32_t a, int32_t b, const UnitInfo* partner,
                            bool is_baseline) noexcept {
    if (OrderLog_AnomalyCount < ORDER_LOG_ANOMALY_LIMIT && OrderLog_Reports.insert({kind, unit}).second) {
        ++OrderLog_AnomalyCount;

        OrderLog_Line(false, "A\t%u\t%s\t%u\t%u\t%d\t%d\n", OrderLog_Tick, OrderLog_AnomalyNames[kind],
                      static_cast<uint32_t>(is_baseline), static_cast<uint32_t>(unit->GetId()), a, b);

        OrderLog_WriteUnit(unit);
        OrderLog_WriteHistory(unit);

        if (partner) {
            OrderLog_WriteUnit(partner);
            OrderLog_WriteHistory(partner);
        }
    }
}

static OrderLogShadow& OrderLog_Track(const UnitInfo* unit, bool* is_new) noexcept {
    OrderLogShadow& shadow = OrderLog_Shadows[unit];

    /* a destroyed unit's memory can be reused by a new unit */
    *is_new = shadow.seen_tick == 0 || shadow.id != unit->GetId() || shadow.unit_type != unit->GetUnitType();

    if (*is_new) {
        shadow = OrderLogShadow{};
        shadow.id = unit->GetId();
        shadow.unit_type = unit->GetUnitType();
        shadow.order = unit->GetOrder();
        shadow.state = unit->GetOrderState();
        shadow.changed_tick = OrderLog_Tick;
    }

    return shadow;
}

static void OrderLog_RecordTransition(const UnitInfo* unit, OrderLogShadow& shadow) noexcept {
    const uint8_t order = unit->GetOrder();
    const uint8_t state = unit->GetOrderState();
    const OrderLogStep step{OrderLog_Tick, {shadow.order, order}, {shadow.state, state}, shadow.writer};
    const uint32_t pairs = (static_cast<uint32_t>(shadow.order) << 24) | (static_cast<uint32_t>(shadow.state) << 16) |
                           (static_cast<uint32_t>(order) << 8) | state;

    /* movement alternates two states on every step, those are only counted */
    const bool is_step =
        shadow.order == order && ((shadow.state == ORDER_STATE_IN_PROGRESS && state == ORDER_STATE_IN_TRANSITION) ||
                                  (shadow.state == ORDER_STATE_IN_TRANSITION && state == ORDER_STATE_IN_PROGRESS));

    shadow.steps[shadow.step_count % ORDER_LOG_HISTORY_DEPTH] = step;
    ++shadow.step_count;

    ++OrderLog_Transitions[{pairs, shadow.writer}];

    if (shadow.turn_serial != OrderLog_TurnSerial) {
        shadow.turn_serial = OrderLog_TurnSerial;
        shadow.turn_lines = 0;
    }

    /* a unit that cycles its orders, for example retrying a blocked path, is only counted after its budget */
    if (!is_step && shadow.turn_lines < ORDER_LOG_UNIT_TURN_LINES) {
        ++shadow.turn_lines;

        const UnitInfo* parent = unit->GetParent();

        OrderLog_Line(true, "X\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%u\t%" PRIxPTR "\n", OrderLog_Tick,
                      static_cast<uint32_t>(unit->GetId()), static_cast<uint32_t>(unit->GetUnitType()),
                      static_cast<uint32_t>(unit->team), static_cast<uint32_t>(shadow.order),
                      static_cast<uint32_t>(shadow.state), static_cast<uint32_t>(order), static_cast<uint32_t>(state),
                      static_cast<uint32_t>(unit->GetPriorOrder()), static_cast<uint32_t>(unit->GetPriorOrderState()),
                      parent ? static_cast<int32_t>(parent->GetId()) : -1, static_cast<uint32_t>(unit->hits),
                      shadow.writer);
    }

    shadow.order = order;
    shadow.state = state;
    shadow.changed_tick = OrderLog_Tick;
}

static void OrderLog_CheckUnit(const UnitInfo* unit, OrderLogShadow& shadow, bool is_baseline) noexcept {
    const auto order = unit->GetOrder();
    const auto state = unit->GetOrderState();
    const UnitInfo* parent = unit->GetParent();

    /* utilities like tapes are selectable but have no hits by design */
    if ((unit->flags & SELECTABLE) && unit->GetBaseValues()->GetAttribute(ATTRIB_HITS) > 0) {
        if (state == ORDER_STATE_DESTROY && unit->hits > 0) {
            OrderLog_Report(ORDER_LOG_ZOMBIE, unit, unit->hits, 0, nullptr, is_baseline);

        } else if (unit->hits == 0 && order != ORDER_EXPLODE && state != ORDER_STATE_DESTROY) {
            OrderLog_Report(ORDER_LOG_DEAD_ACTIVE, unit, 0, 0, nullptr, is_baseline);
        }
    }

    /* the unit hash lookup makes the parent checks the costliest part of the scan, a lasting fault shows every few
     * ticks as well
     */
    const bool check_parent = parent && (is_baseline || OrderLog_Tick % ORDER_LOG_PARENT_CHECK_TICKS == 0);

    if (check_parent && !OrderLog_IsAlive(parent)) {
        if (OrderLog_IsStored(unit)) {
            OrderLog_Report(ORDER_LOG_ORPHAN_HELD, unit, parent->GetId(), 0, parent, is_baseline);

        } else if (order == ORDER_AWAIT_SCALING && state == ORDER_STATE_SHRINK && unit->GetPriorOrder() == ORDER_IDLE) {
            OrderLog_Report(ORDER_LOG_ORPHAN_ENTERING, unit, parent->GetId(), 0, parent, is_baseline);

        } else if (order == ORDER_LAND) {
            OrderLog_Report(ORDER_LOG_ORPHAN_LANDING, unit, parent->GetId(), 0, parent, is_baseline);

        } else if (order == ORDER_AWAIT_STEAL_UNIT || order == ORDER_AWAIT_DISABLE_UNIT) {
            OrderLog_Report(ORDER_LOG_DANGLING_TARGET, unit, parent->GetId(), 0, parent, is_baseline);

        } else if (order == ORDER_LOAD) {
            OrderLog_Report(ORDER_LOG_DANGLING_PASSENGER, unit, parent->GetId(), 0, parent, is_baseline);

        } else if (order == ORDER_UNLOAD) {
            OrderLog_Report(ORDER_LOG_DANGLING_CLIENT, unit, parent->GetId(), 0, parent, is_baseline);

        } else if (OrderLog_IsUtility(unit->GetUnitType())) {
            OrderLog_Report(ORDER_LOG_ORPHAN_UTILITY, unit, parent->GetId(), 0, parent, is_baseline);
        }

    } else if (check_parent) {
        if (order == ORDER_AWAIT_STEAL_UNIT || order == ORDER_AWAIT_DISABLE_UNIT) {
            if (OrderLog_IsStored(parent)) {
                OrderLog_Report(ORDER_LOG_TARGET_STORED, unit, parent->GetId(), 0, parent, is_baseline);

            } else if (OrderLog_GetDistance(unit, parent) > 1) {
                OrderLog_Report(ORDER_LOG_TARGET_MOVED, unit, OrderLog_GetDistance(unit, parent), 0, parent,
                                is_baseline);
            }

        } else if (order == ORDER_LOAD && state == ORDER_STATE_LOADING_IN_PROGRESS &&
                   (parent->grid_x != unit->grid_x || parent->grid_y != unit->grid_y)) {
            OrderLog_Report(ORDER_LOG_PASSENGER_MOVED, unit, parent->grid_x, parent->grid_y, parent, is_baseline);
        }
    }

    if (unit->pin_count < 0) {
        OrderLog_Report(ORDER_LOG_PIN_NEGATIVE, unit, unit->pin_count, 0, nullptr, is_baseline);
    }

    if (!is_baseline) {
        if (unit->pin_count > 0 && OrderLog_AttacksQuiet) {
            if (++shadow.quiet_pinned_ticks >= ORDER_LOG_STALE_PIN_TICKS) {
                OrderLog_Report(ORDER_LOG_PIN_STALE, unit, unit->pin_count, shadow.quiet_pinned_ticks, nullptr, false);
            }

        } else {
            shadow.quiet_pinned_ticks = 0;
        }

        if (OrderLog_IsBlockingEndTurn(unit) && OrderLog_Tick - shadow.changed_tick >= ORDER_LOG_STALL_TICKS) {
            OrderLog_Report(ORDER_LOG_STALL, unit, OrderLog_Tick - shadow.changed_tick, 0, nullptr, false);
        }
    }
}

static void OrderLog_CheckContainers(bool is_baseline) noexcept {
    for (const UnitInfo* container : OrderLog_Containers) {
        const auto it = OrderLog_HeldUnits.find(container);
        const int32_t held_units = (it != OrderLog_HeldUnits.end()) ? it->second : 0;

        if (container->storage != held_units) {
            OrderLog_Report(ORDER_LOG_STORAGE, container, container->storage, held_units, nullptr, is_baseline);
        }
    }
}

static void OrderLog_CheckMapHash(bool is_baseline) noexcept {
    for (int32_t grid_y = 0; grid_y < ResourceManager_MapSize.y; ++grid_y) {
        for (int32_t grid_x = 0; grid_x < ResourceManager_MapSize.x; ++grid_x) {
            const auto units = Hash_MapHash[Point(grid_x, grid_y)];

            if (units) {
                for (auto it = units->Begin(), it_end = units->End(); it != it_end; ++it) {
                    const UnitInfo* unit = it->Get();

                    if (unit->GetId() == 0xFFFF) {
                        continue;
                    }

                    if (!OrderLog_IsAlive(unit)) {
                        OrderLog_Report(ORDER_LOG_GHOST_DEAD, unit, grid_x, grid_y, nullptr, is_baseline);

                    } else if (OrderLog_IsStored(unit) && OrderLog_IsContainer(unit->GetParent()->GetUnitType())) {
                        /* a factory keeps its finished unit on the map, a container takes it off */
                        OrderLog_Report(ORDER_LOG_HELD_ON_MAP, unit, grid_x, grid_y, unit->GetParent(), is_baseline);

                    } else if (!unit->in_transit) {
                        const int32_t size = (unit->flags & BUILDING) ? 2 : 1;

                        if (grid_x < unit->grid_x || grid_x >= unit->grid_x + size || grid_y < unit->grid_y ||
                            grid_y >= unit->grid_y + size) {
                            OrderLog_Report(ORDER_LOG_GHOST_CELL, unit, grid_x, grid_y, nullptr, is_baseline);
                        }
                    }
                }
            }
        }
    }
}

static void OrderLog_FlushTransitions() noexcept {
    for (const auto& [key, count] : OrderLog_Transitions) {
        OrderLog_Line(true, "H\t%d\t%u\t%u\t%u\t%u\t%" PRIxPTR "\t%u\n", OrderLog_Turn, (key.pairs >> 24) & 0xFF,
                      (key.pairs >> 16) & 0xFF, (key.pairs >> 8) & 0xFF, key.pairs & 0xFF, key.writer, count);
    }

    OrderLog_Transitions.clear();
}

static void OrderLog_Scan(bool is_baseline) noexcept {
    bool is_firing = false;

    OrderLog_HeldUnits.clear();
    OrderLog_Containers.clear();

    for (uint8_t list_index = 0; list_index < std::size(OrderLog_UnitLists); ++list_index) {
        SmartList<UnitInfo>* units = OrderLog_UnitLists[list_index];

        for (auto it = units->Begin(), it_end = units->End(); it != it_end; ++it) {
            const UnitInfo* unit = it->Get();
            bool is_new;
            OrderLogShadow& shadow = OrderLog_Track(unit, &is_new);

            /* the first entry of the tick did the checks, a second one would count the unit twice */
            if (!is_new && shadow.seen_tick == OrderLog_Tick) {
                OrderLog_Report(ORDER_LOG_LISTED_TWICE, unit, shadow.list_index, list_index, nullptr, is_baseline);

                continue;
            }

            if (!is_new && (shadow.order != unit->GetOrder() || shadow.state != unit->GetOrderState())) {
                OrderLog_RecordTransition(unit, shadow);
            }

            shadow.writer = 0;
            shadow.seen_tick = OrderLog_Tick;
            shadow.list_index = list_index;

            if (unit->GetOrder() == ORDER_FIRE) {
                is_firing = true;
            }

            if (unit->GetOrder() == ORDER_IDLE && unit->GetParent()) {
                ++OrderLog_HeldUnits[unit->GetParent()];
            }

            if (OrderLog_IsContainer(unit->GetUnitType())) {
                OrderLog_Containers.push_back(unit);
            }

            OrderLog_CheckUnit(unit, shadow, is_baseline);
        }
    }

    OrderLog_CheckContainers(is_baseline);

    std::erase_if(OrderLog_Shadows, [](const auto& item) { return item.second.seen_tick != OrderLog_Tick; });

    if (UnitsManager_PendingAttacks.GetCount() > 0) {
        const UnitInfo* head = UnitsManager_PendingAttacks.Begin()->Get();

        if (head != OrderLog_QueueHead) {
            OrderLog_QueueHead = head;
            OrderLog_QueueHeadTick = OrderLog_Tick;

        } else if (!is_baseline && OrderLog_Tick - OrderLog_QueueHeadTick >= ORDER_LOG_QUEUE_STALL_TICKS) {
            OrderLog_Report(ORDER_LOG_QUEUE_STALL, head, OrderLog_Tick - OrderLog_QueueHeadTick,
                            UnitsManager_PendingAttacks.GetCount(), nullptr, false);
        }

    } else {
        OrderLog_QueueHead = nullptr;
    }

    OrderLog_AttacksQuiet = UnitsManager_PendingAttacks.GetCount() == 0 && !is_firing;

    if (is_baseline) {
        OrderLog_CheckMapHash(true);

    } else if (OrderLog_Turn != GameManager_TurnCounter || OrderLog_ActiveTeam != GameManager_ActiveTurnTeam) {
        OrderLog_FlushTransitions();

        OrderLog_Turn = GameManager_TurnCounter;
        OrderLog_ActiveTeam = GameManager_ActiveTurnTeam;

        ++OrderLog_TurnSerial;

        OrderLog_Line(true, "T\t%u\t%d\t%u\t%u\n", OrderLog_Tick, OrderLog_Turn,
                      static_cast<uint32_t>(OrderLog_ActiveTeam), static_cast<uint32_t>(OrderLog_Shadows.size()));

        OrderLog_CheckMapHash(false);
    }
}

void OrderLog_GameStart() noexcept {
    OrderLog_Enabled = ResourceManager_GetSettings()->GetNumericValue("order_log") != 0;

    if (OrderLog_Enabled && (OrderLog_IsOpen || OrderLog_Open())) {
        OrderLog_MainThread = SDL_GetCurrentThreadID();
        OrderLog_Shadows.clear();
        OrderLog_Transitions.clear();
        OrderLog_Reports.clear();
        OrderLog_AnomalyCount = 0;
        OrderLog_QueueHead = nullptr;
        OrderLog_AttacksQuiet = false;
        OrderLog_Turn = GameManager_TurnCounter;
        OrderLog_ActiveTeam = GameManager_ActiveTurnTeam;

        ++OrderLog_Tick;

        OrderLog_Line(false, "S\t%u\t%d\t%u", OrderLog_Tick, OrderLog_Turn,
                      static_cast<uint32_t>(GameManager_PlayMode));

        for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX; ++team) {
            OrderLog_Line(false, "\t%u", static_cast<uint32_t>(UnitsManager_TeamInfo[team].team_type));
        }

        OrderLog_Line(false, "\n");

        if (const World* world = ResourceManager_GetActiveWorld()) {
            OrderLog_Line(false, "W\t%s\t%d\t%d\n", ResourceManager_GetResourceID(world->GetResourceId()),
                          world->GetMapSize().x, world->GetMapSize().y);
        }

        /* missions and saves already carry defects of the original game, they are reported as the baseline */
        OrderLog_Scan(true);

        OrderLog_TickInProgress.store(OrderLog_Tick + 1, std::memory_order_relaxed);

    } else {
        OrderLog_Enabled = false;
    }
}

void OrderLog_GameEnd() noexcept {
    if (OrderLog_Enabled) {
        OrderLog_FlushTransitions();

        OrderLog_Line(false, "E\t%u\t%d\n", OrderLog_Tick, static_cast<int32_t>(GameManager_TurnCounter));

        OrderLog_Shadows.clear();
        OrderLog_Reports.clear();
        OrderLog_HeldUnits.clear();
        OrderLog_Containers.clear();
        OrderLog_QueueHead = nullptr;
        OrderLog_Enabled = false;
    }
}

void OrderLog_Shutdown() noexcept {
    OrderLog_GameEnd();

    if (OrderLog_IsOpen) {
        OrderLog_Writer.Close();
        OrderLog_IsOpen = false;
    }
}

void OrderLog_NoteWrite(const UnitInfo* unit, const void* writer) noexcept {
    if (OrderLog_Enabled && SDL_GetCurrentThreadID() == OrderLog_MainThread) {
        const auto it = OrderLog_Shadows.find(unit);

        if (it != OrderLog_Shadows.end()) {
            it->second.writer = reinterpret_cast<uintptr_t>(OrderLog_IssuerDepth > 0 ? OrderLog_Issuer : writer);
        }
    }
}

OrderLogTick::OrderLogTick() noexcept { ++OrderLog_TickDepth; }

OrderLogTick::~OrderLogTick() noexcept {
    /* a nested tick runs inside another unit's order processing, the state is only consistent at the outermost */
    if (--OrderLog_TickDepth == 0 && OrderLog_Enabled) {
        ++OrderLog_Tick;

        OrderLog_Scan(false);

        OrderLog_TickInProgress.store(OrderLog_Tick + 1, std::memory_order_relaxed);
    }
}

uint32_t OrderLog_GetTick() noexcept { return OrderLog_TickInProgress.load(std::memory_order_relaxed); }

OrderLogIssuer::OrderLogIssuer(const void* issuer) noexcept {
    if (OrderLog_IssuerDepth++ == 0) {
        OrderLog_Issuer = issuer;
    }
}

OrderLogIssuer::~OrderLogIssuer() noexcept {
    if (--OrderLog_IssuerDepth == 0) {
        OrderLog_Issuer = nullptr;
    }
}

#endif /* !defined(NDEBUG) */
