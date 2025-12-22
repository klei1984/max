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

#include "unitinfo.hpp"

#include <cmath>

#include "access.hpp"
#include "ai.hpp"
#include "ailog.hpp"
#include "builder.hpp"
#include "buildmenu.hpp"
#include "cargo.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "hash.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "paths_manager.hpp"
#include "randomizer.hpp"
#include "registerarray.hpp"
#include "remote.hpp"
#include "researchmenu.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "sound_manager.hpp"
#include "task_manager.hpp"
#include "unit.hpp"
#include "unitinfogroup.hpp"
#include "units_manager.hpp"
#include "zonewalker.hpp"

const uint8_t UnitInfo::ExpResearchTopics[] = {RESEARCH_TOPIC_ATTACK, RESEARCH_TOPIC_SHOTS, RESEARCH_TOPIC_RANGE,
                                               RESEARCH_TOPIC_ARMOR, RESEARCH_TOPIC_HITS};

static void UnitInfo_BuildList_FileLoad(SmartObjectArray<ResourceID>* build_list, SmartFileReader& file);
static void UnitInfo_BuildList_FileSave(SmartObjectArray<ResourceID>* build_list, SmartFileWriter& file);

void UnitInfo_BuildList_FileLoad(SmartObjectArray<ResourceID>* build_list, SmartFileReader& file) {
    ResourceID unit_type;

    build_list->Clear();

    for (int64_t count = file.ReadObjectCount(); count > 0; --count) {
        file.Read(unit_type);
        build_list->PushBack(&unit_type);
    }
}

void UnitInfo_BuildList_FileSave(SmartObjectArray<ResourceID>* build_list, SmartFileWriter& file) {
    ResourceID unit_type;
    int32_t count;

    count = build_list->GetCount();

    file.WriteObjectCount(count);

    for (int32_t i = 0; i < count; ++i) {
        unit_type = *((*build_list)[i]);

        file.Write(unit_type);
    }
}

UnitInfo::UnitInfo()
    : name(nullptr),
      sound(Unit::SFX_TYPE_INVALID),
      unit_type(INVALID_ID),
      popup(nullptr),
      flags(0),
      x(-1),
      y(-1),
      grid_x(-1),
      grid_y(-1),
      color_cycling_lut(nullptr),
      team(-1),
      unit_id(0),
      brightness(UINT8_MAX),
      angle(0),
      max_velocity(0),
      velocity(0),
      scaler_adjust(0),
      turret_angle(0),
      turret_offset_x(0),
      turret_offset_y(0),
      total_images(0),
      image_base(0),
      turret_image_base(0),
      firing_image_base(0),
      connector_image_base(0),
      image_index_max(0),
      orders(ORDER_AWAIT),
      state(ORDER_STATE_INIT),
      prior_orders(ORDER_AWAIT),
      prior_state(ORDER_STATE_INIT),
      move_to_grid_x(0),
      move_to_grid_y(0),
      fire_on_grid_x(0),
      fire_on_grid_y(0),
      build_time(0),
      total_mining(0),
      raw_mining(0),
      fuel_mining(0),
      gold_mining(0),
      raw_mining_max(0),
      gold_mining_max(0),
      fuel_mining_max(0),
      hits(0),
      speed(0),
      group_speed(0),
      shots(0),
      move_and_fire(0),
      storage(0),
      experience(0),
      transfer_cargo(0),
      stealth_dice_roll(0),
      ammo(0),
      targeting_mode(0),
      enter_mode(0),
      cursor(CURSOR_HIDDEN),
      firing_recoil_frames(0),
      disabled_turns_remaining(0),
      delayed_reaction(0),
      damaged_this_turn(false),
      disabled_reaction_fire(false),
      auto_survey(false),
      research_topic(false),
      moved(0),
      bobbed(false),
      engine(0),
      weapon(0),
      move_fraction(0),
      connectors(0),
      shake_effect_state(0),
      build_rate(0),
      repeat_build(0),
      id(0),
      unit_list(nullptr),
      in_transit(false),
      pin_count(0),
      tasks_enabled(true),
      laying_state(0),
      visible_to_team(),
      spotted_by_team(),
      sprite_bounds(),
      shadow_bounds(),
      image_index(0),
      turret_image_index(0),
      ai_state_bits(0) {}

UnitInfo::UnitInfo(ResourceID unit_type, uint16_t team, uint16_t id, uint8_t angle)
    : name(nullptr),
      sound(Unit::SFX_TYPE_INVALID),
      unit_type(unit_type),
      popup(nullptr),
      flags(ResourceManager_GetUnit(unit_type).GetFlags() | UnitsManager_TeamInfo[team].team_units->hash_team_id),
      x(-1),
      y(-1),
      grid_x(-1),
      grid_y(-1),
      color_cycling_lut(UnitsManager_TeamInfo[team].team_units->color_index_table),
      team(team),
      unit_id(0),
      brightness(UINT8_MAX),
      angle(angle),
      max_velocity(0),
      velocity(0),
      scaler_adjust(0),
      turret_angle(0),
      turret_offset_x(0),
      turret_offset_y(0),
      total_images(0),
      image_base(0),
      turret_image_base(0),
      firing_image_base(0),
      connector_image_base(0),
      image_index_max(0),
      orders(ORDER_AWAIT),
      state(ORDER_STATE_EXECUTING_ORDER),
      prior_orders(ORDER_AWAIT),
      prior_state(ORDER_STATE_EXECUTING_ORDER),
      move_to_grid_x(0),
      move_to_grid_y(0),
      fire_on_grid_x(0),
      fire_on_grid_y(0),
      build_time(0),
      total_mining(0),
      raw_mining(0),
      fuel_mining(0),
      gold_mining(0),
      raw_mining_max(0),
      gold_mining_max(0),
      fuel_mining_max(0),
      hits(0),
      speed(0),
      group_speed(0),
      shots(0),
      move_and_fire(0),
      storage(0),
      experience(0),
      transfer_cargo(0),
      stealth_dice_roll(0),
      ammo(0),
      targeting_mode(0),
      enter_mode(0),
      cursor(CURSOR_HIDDEN),
      firing_recoil_frames(0),
      disabled_turns_remaining(0),
      delayed_reaction(0),
      damaged_this_turn(false),
      disabled_reaction_fire(false),
      auto_survey(false),
      research_topic(false),
      moved(0),
      bobbed(false),
      engine(0),
      weapon(0),
      move_fraction(0),
      connectors(0),
      shake_effect_state(0),
      base_values(UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)),
      build_rate(1),
      repeat_build(0),
      id(id),
      unit_list(nullptr),
      in_transit(false),
      pin_count(0),
      tasks_enabled(true),
      laying_state(0),
      visible_to_team(),
      spotted_by_team(),
      sprite_bounds(),
      shadow_bounds(),
      image_index(0),
      turret_image_index(0),
      ai_state_bits(0) {
    const Unit& unit = ResourceManager_GetUnit(unit_type);

    rect_init(&sprite_bounds, 0, 0, 0, 0);
    rect_init(&shadow_bounds, 0, 0, 0, 0);

    Init();

    if (unit.GetSpriteData()) {
        total_images = reinterpret_cast<struct ImageMultiHeader*>(unit.GetSpriteData())->image_count;

    } else {
        total_images = 0;
    }

    image_base = unit.GetFrameInfo().image_base;
    turret_image_base = unit.GetFrameInfo().turret_image_base;
    firing_image_base = unit.GetFrameInfo().firing_image_base;
    connector_image_base = unit.GetFrameInfo().connector_image_base;

    if (unit_type == MININGST) {
        image_base = (UnitsManager_TeamInfo[team].team_clan - 1) * sizeof(uint16_t);
    }

    image_index = image_base + angle;
    turret_image_index = turret_image_base + angle;

    if (unit_type == COMMANDO) {
        image_index_max = 103;
    } else if (unit_type == INFANTRY) {
        image_index_max = 103;
    } else {
        image_index_max = unit.GetFrameInfo().image_count + image_index - 1;
    }

    hits = base_values->GetAttribute(ATTRIB_HITS);
    speed = base_values->GetAttribute(ATTRIB_SPEED);
    shots = base_values->GetAttribute(ATTRIB_ROUNDS);
    move_and_fire = base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE);
    ammo = base_values->GetAttribute(ATTRIB_AMMO);
    storage = base_values->GetAttribute(ATTRIB_STORAGE);
    engine = 2;

    if (base_values->GetAttribute(ATTRIB_ATTACK)) {
        weapon = 2;
    } else {
        weapon = 0;
    }

    moved = 0;
    bobbed = false;
    shake_effect_state = 0;

    InitStealthStatus();

    if ((flags & (CONNECTOR_UNIT | BUILDING | STANDALONE)) && !(flags & GROUND_COVER) && id != 0xFFFF) {
        AttachComplex(CreateComplex(team));
    }
}

UnitInfo::UnitInfo(const UnitInfo& other)
    : unit_type(other.unit_type),
      popup(other.popup),
      flags(other.flags),
      x(other.x),
      y(other.y),
      grid_x(other.grid_x),
      grid_y(other.grid_y),
      attack_site(other.attack_site),
      color_cycling_lut(other.color_cycling_lut),
      team(other.team),
      unit_id(other.unit_id),
      brightness(other.brightness),
      angle(other.angle),
      max_velocity(other.max_velocity),
      velocity(other.velocity),
      sound(other.sound),
      scaler_adjust(other.scaler_adjust),
      turret_angle(other.turret_angle),
      turret_offset_x(other.turret_offset_x),
      turret_offset_y(other.turret_offset_y),
      total_images(other.total_images),
      image_base(other.image_base),
      turret_image_base(other.turret_image_base),
      firing_image_base(other.firing_image_base),
      connector_image_base(other.connector_image_base),
      image_index_max(other.image_index_max),
      orders(other.orders),
      state(other.state),
      prior_orders(other.prior_orders),
      prior_state(other.prior_state),
      move_to_grid_x(other.move_to_grid_x),
      move_to_grid_y(other.move_to_grid_y),
      fire_on_grid_x(other.fire_on_grid_x),
      fire_on_grid_y(other.fire_on_grid_y),
      build_time(other.build_time),
      total_mining(other.total_mining),
      raw_mining(other.raw_mining),
      fuel_mining(other.fuel_mining),
      gold_mining(other.gold_mining),
      raw_mining_max(other.raw_mining_max),
      gold_mining_max(other.gold_mining_max),
      fuel_mining_max(other.fuel_mining_max),
      hits(other.hits),
      speed(other.speed),
      group_speed(other.group_speed),
      shots(other.shots),
      move_and_fire(other.move_and_fire),
      storage(other.storage),
      experience(other.experience),
      transfer_cargo(other.transfer_cargo),
      stealth_dice_roll(other.stealth_dice_roll),
      ammo(other.ammo),
      targeting_mode(other.targeting_mode),
      enter_mode(other.enter_mode),
      cursor(other.cursor),
      firing_recoil_frames(other.firing_recoil_frames),
      disabled_turns_remaining(other.disabled_turns_remaining),
      delayed_reaction(other.delayed_reaction),
      damaged_this_turn(other.damaged_this_turn),
      disabled_reaction_fire(other.disabled_reaction_fire),
      auto_survey(other.auto_survey),
      research_topic(other.research_topic),
      moved(other.moved),
      bobbed(other.bobbed),
      engine(other.engine),
      weapon(other.weapon),
      move_fraction(other.move_fraction),
      path(other.path),
      connectors(other.connectors),
      shake_effect_state(other.shake_effect_state),
      base_values(other.base_values),
      complex(other.complex),
      build_list(other.build_list),
      build_rate(other.build_rate),
      repeat_build(other.repeat_build),
      id(other.id),
      unit_list(other.unit_list),
      parent_unit(other.GetParent()),
      enemy_unit(other.enemy_unit),
      tasks(other.tasks),
      in_transit(other.in_transit),
      last_target(other.last_target),
      pin_count(other.pin_count),
      tasks_enabled(other.tasks_enabled),
      laying_state(other.laying_state),
      name(nullptr),
      sprite_bounds(other.sprite_bounds),
      shadow_bounds(other.shadow_bounds),
      image_index(other.image_index),
      turret_image_index(other.turret_image_index),
      shadow_offset(other.shadow_offset),
      ai_state_bits(other.ai_state_bits) {
    memcpy(visible_to_team, other.visible_to_team, sizeof(visible_to_team));
    memcpy(spotted_by_team, other.spotted_by_team, sizeof(spotted_by_team));
}

UnitInfo::~UnitInfo() { delete[] name; }

FileObject* UnitInfo::Allocate() noexcept { return new (std::nothrow) UnitInfo(); }

static uint32_t UnitInfo_TypeIndex;
static RegisterClass UnitInfo_ClassRegister("UnitInfo", &UnitInfo_TypeIndex, &UnitInfo::Allocate);

uint32_t UnitInfo::GetTypeIndex() const { return UnitInfo_TypeIndex; }

void UnitInfo::Init() {
    Unit& unit = ResourceManager_GetUnit(unit_type);
    uint32_t data_size;

    if (!unit.GetSpriteData()) {
        uint8_t* sprite_data = ResourceManager_LoadResource(unit.GetSprite());
        uint8_t* shadow_data = ResourceManager_LoadResource(unit.GetShadow());

        if (ResourceManager_DisableEnhancedGraphics) {
            if (sprite_data) {
                sprite_data = Gfx_RescaleSprite(sprite_data, &data_size, 0, 2);
                ResourceManager_Realloc(unit.GetSprite(), sprite_data, data_size);
            }

            if (shadow_data) {
                shadow_data = Gfx_RescaleSprite(shadow_data, &data_size, 1, 2);
                ResourceManager_Realloc(unit.GetShadow(), shadow_data, data_size);
            }
        }

        unit.SetSpriteData(sprite_data);
        unit.SetShadowData(shadow_data);
    }

    switch (unit_type) {
        case COMMTWR: {
            popup = &UnitsManager_PopupCallbacks[22];
        } break;

        case POWERSTN:
        case POWGEN: {
            popup = &UnitsManager_PopupCallbacks[19];
        } break;

        case BARRACKS:
        case DEPOT:
        case HANGAR:
        case DOCK: {
            popup = &UnitsManager_PopupCallbacks[12];
        } break;

        case ADUMP: {
            popup = &UnitsManager_PopupCallbacks[9];
        } break;

        case FDUMP: {
            popup = &UnitsManager_PopupCallbacks[10];
        } break;

        case SHIPYARD:
        case LIGHTPLT:
        case LANDPLT:
        case AIRPLT:
        case TRAINHAL: {
            popup = &UnitsManager_PopupCallbacks[14];
        } break;

        case RESEARCH: {
            popup = &UnitsManager_PopupCallbacks[18];
        } break;

        case GREENHSE: {
            popup = &UnitsManager_PopupCallbacks[17];
        } break;

        case RECCENTR: {
            popup = &UnitsManager_PopupCallbacks[15];
        } break;

        case GUNTURRT:
        case ANTIAIR:
        case ARTYTRRT:
        case ANTIMSSL:
        case SCOUT:
        case TANK:
        case ARTILLRY:
        case ROCKTLCH:
        case MISSLLCH:
        case SP_FLAK:
        case INFANTRY:
        case FASTBOAT:
        case CORVETTE:
        case BATTLSHP:
        case SUBMARNE:
        case MSSLBOAT:
        case FIGHTER:
        case BOMBER:
        case JUGGRNT:
        case ALNTANK:
        case ALNASGUN:
        case ALNPLANE: {
            popup = &UnitsManager_PopupCallbacks[3];
        } break;

        case MININGST: {
            popup = &UnitsManager_PopupCallbacks[16];
        } break;

        case MASTER: {
            popup = &UnitsManager_PopupCallbacks[21];
        } break;

        case CONSTRCT:
        case ENGINEER: {
            popup = &UnitsManager_PopupCallbacks[13];
        } break;

        case MINELAYR:
        case SEAMNLYR: {
            popup = &UnitsManager_PopupCallbacks[6];
        } break;

        case SURVEYOR: {
            popup = &UnitsManager_PopupCallbacks[2];
        } break;

        case SCANNER:
        case GOLDTRCK: {
            popup = &UnitsManager_PopupCallbacks[1];
        } break;

        case SPLYTRCK:
        case CARGOSHP: {
            popup = &UnitsManager_PopupCallbacks[5];
        } break;

        case BULLDOZR: {
            popup = &UnitsManager_PopupCallbacks[20];
        } break;

        case REPAIR: {
            popup = &UnitsManager_PopupCallbacks[8];
        } break;

        case FUELTRCK: {
            popup = &UnitsManager_PopupCallbacks[7];
        } break;

        case CLNTRANS:
        case SEATRANS:
        case AIRTRANS: {
            popup = &UnitsManager_PopupCallbacks[11];
        } break;

        case COMMANDO: {
            popup = &UnitsManager_PopupCallbacks[4];
        } break;

        default: {
            popup = &UnitsManager_PopupCallbacks[0];
        } break;
    }
}

bool UnitInfo::IsVisibleToTeam(uint16_t team) const { return visible_to_team[team]; }

void UnitInfo::SetEnemy(UnitInfo* enemy) { enemy_unit = enemy; }

UnitInfo* UnitInfo::GetEnemy() const { return &*enemy_unit; }

uint16_t UnitInfo::GetId() const { return id; }

UnitInfo* UnitInfo::GetFirstFromUnitList() const {
    UnitInfo* result;

    if (unit_list != nullptr) {
        result = &*(unit_list->Begin());
    } else {
        result = nullptr;
    }

    return result;
}

SmartList<UnitInfo>* UnitInfo::GetUnitList() const { return unit_list; }

uint32_t UnitInfo::GetAiStateBits() const { return ai_state_bits; }

void UnitInfo::SetAiStateBits(const uint32_t value) { ai_state_bits = value; }

void UnitInfo::ChangeAiStateBits(const uint32_t value, const bool mode) {
    if (mode) {
        ai_state_bits |= value;

    } else {
        ai_state_bits &= ~value;
    }
}

uint16_t UnitInfo::GetImageIndex() const { return image_index; }

void UnitInfo::AddTask(Task* task) {
    SmartPointer<Task> old_task(GetTask());

    AILOG(log, "Adding task to {} {}: {}", ResourceManager_GetUnit(unit_type).GetSingularName().data(), unit_id,
          task->WriteStatusLog());

    tasks.PushFront(*task);

    if (old_task) {
        AILOG_LOG(log, "Old topmost task: {}", old_task->WriteStatusLog());
    }
}

void UnitInfo::ScheduleDelayedTasks(bool priority) {
    if (tasks_enabled) {
        for (SmartList<Task>::Iterator it = delayed_tasks.Begin(); it != delayed_tasks.End(); ++it) {
            (*it).RemindTurnEnd(true);
        }
    }

    if (GetTask() && tasks_enabled) {
        Task_RemindMoveFinished(this, priority);
    }
}

Task* UnitInfo::GetTask() const {
    Task* task;

    if (tasks.GetCount()) {
        task = &tasks[0];
    } else {
        task = nullptr;
    }

    return task;
}

SmartList<Task>& UnitInfo::GetTasks() { return tasks; }

void UnitInfo::SetBaseValues(UnitValues* unit_values) { base_values = unit_values; }

UnitValues* UnitInfo::GetBaseValues() const { return &*base_values; }

bool UnitInfo::IsDetectedByTeam(uint16_t team) const { return (spotted_by_team[team] || visible_to_team[team]); }

Complex* UnitInfo::GetComplex() const { return &*complex; }

SmartPointer<UnitInfo> UnitInfo::MakeCopy() {
    SmartPointer<UnitInfo> copy = new (std::nothrow) UnitInfo(*this);
    copy->path = nullptr;

    return copy;
}

void UnitInfo::OffsetDrawZones(int32_t offset_x, int32_t offset_y) {
    x += offset_x;
    sprite_bounds.ulx += offset_x;
    sprite_bounds.lrx += offset_x;
    shadow_bounds.ulx += offset_x;
    shadow_bounds.lrx += offset_x;

    y += offset_y;
    sprite_bounds.uly += offset_y;
    sprite_bounds.lry += offset_y;
    shadow_bounds.uly += offset_y;
    shadow_bounds.lry += offset_y;
}

void UnitInfo::UpdateUnitDrawZones() {
    const Unit& unit = ResourceManager_GetUnit(unit_type);

    if (unit.GetSpriteData()) {
        Point position;
        int32_t unit_size;

        position.x = x;
        position.y = y;

        if (flags & BUILDING) {
            unit_size = GFX_MAP_TILE_SIZE;

        } else {
            unit_size = GFX_MAP_TILE_SIZE / 2;
        }

        sprite_bounds.ulx = position.x - unit_size;
        sprite_bounds.uly = position.y - unit_size;
        sprite_bounds.lrx = position.x + unit_size - 1;
        sprite_bounds.lry = position.y + unit_size - 1;

        UpdateSpriteFrameBounds(&sprite_bounds, position.x, position.y,
                                reinterpret_cast<struct ImageMultiHeader*>(unit.GetSpriteData()), image_index);

        if (flags & (SPINNING_TURRET | TURRET_SPRITE)) {
            UpdateSpriteFrameBounds(&sprite_bounds, position.x + turret_offset_x, position.y + turret_offset_y,
                                    reinterpret_cast<struct ImageMultiHeader*>(unit.GetSpriteData()),
                                    turret_image_index);
        }

        shadow_bounds.ulx = 32000;
        shadow_bounds.uly = 32000;
        shadow_bounds.lrx = -32000;
        shadow_bounds.lry = -32000;

        position -= shadow_offset;

        UpdateSpriteFrameBounds(&shadow_bounds, position.x, position.y,
                                reinterpret_cast<struct ImageMultiHeader*>(unit.GetShadowData()), image_index);

        if (flags & (SPINNING_TURRET | TURRET_SPRITE)) {
            UpdateSpriteFrameBounds(&shadow_bounds, position.x + turret_offset_x, position.y + turret_offset_y,
                                    reinterpret_cast<struct ImageMultiHeader*>(unit.GetShadowData()),
                                    turret_image_index);
        }

        if (shadow_bounds.ulx > shadow_bounds.lrx || shadow_bounds.uly > shadow_bounds.lry ||
            shadow_bounds.ulx - 16 < sprite_bounds.ulx || shadow_bounds.uly - 16 < sprite_bounds.uly) {
            shadow_bounds.ulx = std::min(shadow_bounds.ulx, sprite_bounds.ulx);
            shadow_bounds.uly = std::min(shadow_bounds.uly, sprite_bounds.uly);
            shadow_bounds.lrx = std::max(shadow_bounds.lrx, sprite_bounds.lrx);
            shadow_bounds.lry = std::max(shadow_bounds.lry, sprite_bounds.lry);
        }
    }
}

void UnitInfo::GetName(char* const text, const size_t size) const noexcept {
    if (text && size > 0) {
        if (name) {
            SDL_utf8strlcpy(text, name, size);
        } else {
            const auto name_length{
                snprintf(nullptr, 0, "%s %i", ResourceManager_GetUnit(unit_type).GetSingularName().data(), unit_id)};

            if (name_length > 0) {
                auto buffer{new (std::nothrow) char[name_length + sizeof(char)]};

                snprintf(buffer, name_length + sizeof(char), "%s %i",
                         ResourceManager_GetUnit(unit_type).GetSingularName().data(), unit_id);
                SDL_utf8strlcpy(text, buffer, size);

                delete[] buffer;

            } else {
                text[0] = '\0';
            }
        }
    }
}

void UnitInfo::GetDisplayName(char* text, const size_t size) const noexcept {
    if (text && size > 0) {
        char unit_name[40];
        char unit_mark[20];

        GetName(unit_name, sizeof(unit_name));
        GetVersion(unit_mark, base_values->GetVersion());

        const auto name_length{snprintf(nullptr, 0, "%s %s %s", _(660b), unit_mark, unit_name)};

        if (name_length > 0) {
            auto buffer{new (std::nothrow) char[name_length + sizeof(char)]};

            snprintf(buffer, name_length + sizeof(char), "%s %s %s", _(660b), unit_mark, unit_name);
            SDL_utf8strlcpy(text, buffer, size);

            delete[] buffer;

        } else {
            text[0] = '\0';
        }
    }
}

void UnitInfo::CalcRomanDigit(char* text, int32_t value, const char* digit1, const char* digit2, const char* digit3) {
    if (value == 9) {
        strcat(text, digit1);
        strcat(text, digit3);
    } else {
        if (value >= 5) {
            strcat(text, digit2);
            value -= 5;
        }

        if (value >= 4) {
            strcat(text, digit1);
            strcat(text, digit2);
            value -= 4;
        }

        for (int32_t i = 0; i < value; ++i) {
            strcat(text, digit1);
        }
    }
}

Complex* UnitInfo::CreateComplex(uint16_t team) { return UnitsManager_TeamInfo[team].team_units->CreateComplex(); }

struct ImageMultiFrameHeader* UnitInfo::GetSpriteFrame(struct ImageMultiHeader* sprite, uint16_t image_index) {
    uintptr_t offset;

    SDL_assert(sprite);
    SDL_assert(image_index >= 0 && image_index < sprite->image_count);

    offset =
        reinterpret_cast<int32_t*>(&(reinterpret_cast<uint8_t*>(sprite)[sizeof(sprite->image_count)]))[image_index];

    return reinterpret_cast<ImageMultiFrameHeader*>(&(reinterpret_cast<uint8_t*>(sprite)[offset]));
}

void UnitInfo::UpdateSpriteFrameBounds(Rect* bounds, int32_t x, int32_t y, struct ImageMultiHeader* sprite,
                                       uint16_t image_index) {
    if (sprite) {
        struct ImageMultiFrameHeader* frame;
        int32_t scaling_factor;
        Rect frame_bounds;

        frame = GetSpriteFrame(sprite, image_index);

        if (frame->width > 0 && frame->height > 0) {
            if (ResourceManager_DisableEnhancedGraphics) {
                scaling_factor = 2;
            } else {
                scaling_factor = 1;
            }

            frame_bounds.ulx = x - (frame->hotx * scaling_factor);
            frame_bounds.uly = y - (frame->hoty * scaling_factor);
            frame_bounds.lrx = frame->width * scaling_factor + frame_bounds.ulx - 1;
            frame_bounds.lry = frame->height * scaling_factor + frame_bounds.uly - 1;

            bounds->ulx = std::min(bounds->ulx, frame_bounds.ulx);
            bounds->uly = std::min(bounds->uly, frame_bounds.uly);
            bounds->lrx = std::max(bounds->lrx, frame_bounds.lrx);
            bounds->lry = std::max(bounds->lry, frame_bounds.lry);
        }
    }
}

void UnitInfo::UpdateSpriteFrame(uint16_t image_base, uint16_t image_index_max) {
    this->image_base = image_base;
    this->image_index_max = image_index_max;
    DrawSpriteFrame(image_base + angle);
}

void UnitInfo::DrawSpriteFrame(uint16_t image_index) {
    if (this->image_index != image_index) {
        bool is_visible;

        is_visible = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

        if (is_visible) {
            RefreshScreen();
        }

        this->image_index = image_index;
        UpdateUnitDrawZones();

        if (is_visible) {
            RefreshScreen();
        }
    }
}

void UnitInfo::DrawSpriteTurretFrame(uint16_t turret_image_index) {
    if (this->turret_image_index != turret_image_index) {
        bool is_visible;

        is_visible = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

        if (is_visible) {
            RefreshScreen();
        }

        this->turret_image_index = turret_image_index;
        UpdateUnitDrawZones();

        if (is_visible) {
            RefreshScreen();
        }
    }
}

void UnitInfo::GetVersion(char* text, int32_t version) {
    text[0] = '\0';

    CalcRomanDigit(text, version / 100, "C", "D", "M");
    version %= 100;

    CalcRomanDigit(text, version / 10, "X", "L", "C");
    version %= 10;

    CalcRomanDigit(text, version, "I", "V", "X");
}

void UnitInfo::SetName(const char* const text) noexcept {
    delete[] name;

    if (text && strlen(text)) {
        name = new (std::nothrow) char[strlen(text) + 1];
        strcpy(name, text);

    } else {
        name = nullptr;
    }
}

int32_t UnitInfo::GetRaw() {
    int32_t result;

    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_RAW) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = materials.raw;

        } else {
            result = storage;
        }

    } else {
        result = 0;
    }

    return result;
}

int32_t UnitInfo::GetRawFreeCapacity() {
    int32_t result;

    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_RAW) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = capacity.raw - materials.raw;

        } else {
            result = GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - storage;
        }

    } else {
        result = 0;
    }

    return result;
}

void UnitInfo::TransferRaw(int32_t amount) {
    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_RAW) {
        storage += amount;

        if (complex != nullptr) {
            const int32_t storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->material += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->material -= amount;

                complex->Transfer(amount, 0, 0);
            }
        }
    }
}

int32_t UnitInfo::GetFuel() {
    int32_t result;

    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_FUEL) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = materials.fuel;

        } else {
            result = storage;
        }

    } else {
        result = 0;
    }

    return result;
}

int32_t UnitInfo::GetFuelFreeCapacity() {
    int32_t result;

    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_FUEL) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = capacity.fuel - materials.fuel;

        } else {
            result = GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - storage;
        }

    } else {
        result = 0;
    }

    return result;
}

void UnitInfo::TransferFuel(int32_t amount) {
    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_FUEL) {
        storage += amount;

        if (complex != nullptr) {
            int32_t storage_capacity;

            storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->fuel += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->fuel -= amount;

                complex->Transfer(0, amount, 0);
            }
        }
    }
}

int32_t UnitInfo::GetGold() {
    int32_t result;

    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_GOLD) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = materials.gold;

        } else {
            result = storage;
        }

    } else {
        result = 0;
    }

    return result;
}

int32_t UnitInfo::GetGoldFreeCapacity() {
    int32_t result;

    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_GOLD) {
        if (complex != nullptr) {
            Cargo materials;
            Cargo capacity;

            complex->GetCargoInfo(materials, capacity);

            result = capacity.gold - materials.gold;

        } else {
            result = GetBaseValues()->GetAttribute(ATTRIB_STORAGE) - storage;
        }

    } else {
        result = 0;
    }

    return result;
}

void UnitInfo::TransferGold(int32_t amount) {
    if (ResourceManager_GetUnit(unit_type).GetCargoType() == Unit::CargoType::CARGO_TYPE_GOLD) {
        storage += amount;

        if (complex != nullptr) {
            int32_t storage_capacity;

            storage_capacity = GetBaseValues()->GetAttribute(ATTRIB_STORAGE);

            complex->gold += amount;

            if (storage > storage_capacity) {
                amount = storage - storage_capacity;
                storage = storage_capacity;
                complex->gold -= amount;

                complex->Transfer(0, 0, amount);
            }
        }
    }
}

int32_t UnitInfo::GetTurnsToRepair() {
    int32_t hits_damage;
    int32_t base_hits;

    base_hits = base_values->GetAttribute(ATTRIB_HITS);
    hits_damage = base_hits - hits;

    return (base_hits * 4 + GetNormalRateBuildCost() * hits_damage - 1) / (base_hits * 4);
}

void UnitInfo::RefreshScreen() {
    GameManager_AddDrawBounds(&shadow_bounds);
    GameManager_AddDrawBounds(&sprite_bounds);
}

void UnitInfo::UpdateAngle(uint16_t image_index) {
    int32_t image_diff;

    image_diff = image_index - angle;

    if (image_diff) {
        bool is_visible;

        is_visible = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

        if (is_visible) {
            RefreshScreen();
        }

        angle = image_index & 0x07;
        this->image_index = (this->image_index & 0xF8) + angle;

        if (flags & TURRET_SPRITE) {
            UpdateTurretAngle((turret_angle + image_diff) & 0x07);
            turret_image_index = turret_image_base + turret_angle;
        }

        UpdateUnitDrawZones();

        if (is_visible) {
            RefreshScreen();
        }
    }
}

int32_t UnitInfo::GetDrawLayer(ResourceID unit_type) {
    int32_t result;

    switch (unit_type) {
        case TORPEDO:
        case TRPBUBLE: {
            result = 1;
        } break;

        case WTRPLTFM: {
            result = 2;
        } break;

        case ROAD:
        case BRIDGE:
        case WALDO:
        case LRGSLAB:
        case SMLSLAB: {
            result = 3;
        } break;

        case SMLRUBLE:
        case LRGRUBLE: {
            result = 4;
        } break;

        case LANDMINE:
        case SEAMINE: {
            result = 5;
        } break;

        default: {
            result = 6;
        } break;
    }

    return result;
}

void UnitInfo::AddToDrawList(uint32_t override_flags) {
    uint32_t unit_flags;

    if (override_flags) {
        unit_flags = override_flags;
    } else {
        unit_flags = flags;
    }

    if (unit_flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
        UnitsManager_MobileLandSeaUnits.PushFront(*this);

    } else if (unit_flags & STATIONARY) {
        if (unit_flags & GROUND_COVER) {
            int32_t layer_index;
            SmartList<UnitInfo>::Iterator it = UnitsManager_GroundCoverUnits.Begin();

            layer_index = GetDrawLayer(unit_type);

            for (; it != UnitsManager_GroundCoverUnits.End(); ++it) {
                if (GetDrawLayer((*it).unit_type) >= layer_index) {
                    break;
                }
            }

            UnitsManager_GroundCoverUnits.InsertBefore(it, *this);

        } else {
            int32_t reference_y;
            SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();

            reference_y = y;

            for (; it != UnitsManager_StationaryUnits.End(); ++it) {
                if ((*it).y >= reference_y) {
                    break;
                }
            }

            UnitsManager_StationaryUnits.InsertBefore(it, *this);
        }

    } else if (unit_flags & MOBILE_AIR_UNIT) {
        SmartList<UnitInfo>::Iterator it = UnitsManager_MobileAirUnits.Begin();

        for (; it != UnitsManager_MobileAirUnits.End(); ++it) {
            if ((*it).flags & HOVERING) {
                break;
            }
        }

        UnitsManager_MobileAirUnits.InsertBefore(it, *this);

    } else if (unit_flags & MISSILE_UNIT) {
        if (unit_flags & GROUND_COVER) {
            UnitsManager_GroundCoverUnits.PushFront(*this);

        } else {
            SmartList<UnitInfo>::Iterator it = UnitsManager_ParticleUnits.Begin();

            for (; it != UnitsManager_ParticleUnits.End(); ++it) {
                if (((*it).flags & EXPLODING) && (*it).unit_type != RKTSMOKE) {
                    break;
                }
            }

            UnitsManager_ParticleUnits.InsertBefore(it, *this);
        }
    }
}

void UnitInfo::SetPosition(int32_t grid_x, int32_t grid_y, bool skip_map_status_update) {
    x = grid_x * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;
    y = grid_y * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;

    this->grid_x = x / GFX_MAP_TILE_SIZE;
    this->grid_y = y / GFX_MAP_TILE_SIZE;

    if (flags & BUILDING) {
        x += (GFX_MAP_TILE_SIZE / 2) - 1;
        y += (GFX_MAP_TILE_SIZE / 2) - 1;
    }

    UpdateUnitDrawZones();
    RefreshScreen();

    Hash_MapHash.Add(this, flags & GROUND_COVER);

    if (!skip_map_status_update && ((GetId() != 0xFFFF) || (flags & (EXPLODING | MISSILE_UNIT)))) {
        Access_UpdateMapStatus(this, true);
    }

    AddToDrawList();
}

void UnitInfo::UpdatePinCount(int32_t grid_x, int32_t grid_y, int32_t pin_units) {
    if (base_values->GetAttribute(ATTRIB_ATTACK_RADIUS)) {
        UpdatePinsFromLists(grid_x, grid_y, &UnitsManager_GroundCoverUnits, pin_units);
        UpdatePinsFromLists(grid_x, grid_y, &UnitsManager_StationaryUnits, pin_units);
        UpdatePinsFromLists(grid_x, grid_y, &UnitsManager_MobileLandSeaUnits, pin_units);

    } else {
        SmartPointer<UnitInfo> target(Access_GetAttackTarget(this, grid_x, grid_y));

        if (target) {
            target->pin_count += pin_units;
        }
    }
}

void UnitInfo::RemoveTasks() {
    SmartList<Task> backup_tasks(tasks);

    tasks.Clear();

    for (SmartList<Task>::Iterator it = backup_tasks.Begin(); it != backup_tasks.End(); ++it) {
        (*it).RemoveUnit(*this);
    }
}

void UnitInfo::MoveInTransitUnitInMapHash(int32_t grid_x, int32_t grid_y) {
    RemoveInTransitUnitFromMapHash();

    in_transit = true;

    last_target.x = grid_x;
    last_target.y = grid_y;

    {
        int32_t backup_grid_x;
        int32_t backup_grid_y;

        backup_grid_x = this->grid_x;
        backup_grid_y = this->grid_y;

        this->grid_x = grid_x;
        this->grid_y = grid_y;

        Hash_MapHash.Add(this);

        this->grid_x = backup_grid_x;
        this->grid_y = backup_grid_y;
    }
}

void UnitInfo::RemoveInTransitUnitFromMapHash() {
    if (in_transit) {
        int32_t backup_grid_x;
        int32_t backup_grid_y;

        backup_grid_x = this->grid_x;
        backup_grid_y = this->grid_y;

        this->grid_x = last_target.x;
        this->grid_y = last_target.y;

        Hash_MapHash.Remove(this);

        this->grid_x = backup_grid_x;
        this->grid_y = backup_grid_y;

        in_transit = false;
    }
}

void UnitInfo::BuildOrder() {
    if (orders != ORDER_HALT_BUILDING && orders != ORDER_HALT_BUILDING_2) {
        build_time = BuildMenu_GetTurnsToBuild(GetConstructedUnitType(), team);
    }

    orders = ORDER_AWAIT;
    state = ORDER_STATE_EXECUTING_ORDER;

    UnitsManager_SetNewOrder(this, ORDER_BUILD, ORDER_STATE_INIT);
}

void UnitInfo::GetBounds(Rect* bounds) {
    bounds->ulx = x / GFX_MAP_TILE_SIZE;
    bounds->uly = y / GFX_MAP_TILE_SIZE;
    bounds->lrx = bounds->ulx + 1;
    bounds->lry = bounds->uly + 1;

    if (flags & BUILDING) {
        ++bounds->lrx;
        ++bounds->lry;
    }
}

bool UnitInfo::HasUpgradeableAttributes() const {
    UnitValues* current_values = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type);

    if (base_values == current_values) {
        return false;
    }

    // Check if ANY attribute can be improved (including ATTRIB_TURNS where lower is better)
    return (base_values->GetAttribute(ATTRIB_HITS) < current_values->GetAttribute(ATTRIB_HITS)) ||
           (base_values->GetAttribute(ATTRIB_ATTACK) < current_values->GetAttribute(ATTRIB_ATTACK)) ||
           (base_values->GetAttribute(ATTRIB_ARMOR) < current_values->GetAttribute(ATTRIB_ARMOR)) ||
           (base_values->GetAttribute(ATTRIB_SPEED) < current_values->GetAttribute(ATTRIB_SPEED)) ||
           (base_values->GetAttribute(ATTRIB_RANGE) < current_values->GetAttribute(ATTRIB_RANGE)) ||
           (base_values->GetAttribute(ATTRIB_ROUNDS) < current_values->GetAttribute(ATTRIB_ROUNDS)) ||
           (base_values->GetAttribute(ATTRIB_SCAN) < current_values->GetAttribute(ATTRIB_SCAN)) ||
           (base_values->GetAttribute(ATTRIB_AMMO) < current_values->GetAttribute(ATTRIB_AMMO)) ||
           (base_values->GetAttribute(ATTRIB_TURNS) > current_values->GetAttribute(ATTRIB_TURNS) &&
            current_values->GetAttribute(ATTRIB_TURNS) >= 1);
}

bool UnitInfo::IsUpgradeAvailable() {
    if (!(flags & STATIONARY)) {
        return false;
    }

    return HasUpgradeableAttributes();
}

void UnitInfo::Redraw() {
    bool team_visibility = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

    if (team_visibility) {
        RefreshScreen();
    }

    int32_t offset_x = (grid_x * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2) - x;
    int32_t offset_y = (grid_y * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2) - y;

    OffsetDrawZones(offset_x, offset_y);

    if ((flags & MOBILE_AIR_UNIT) && (flags & HOVERING)) {
        shadow_offset.x = -GFX_MAP_TILE_SIZE;
        shadow_offset.y = -GFX_MAP_TILE_SIZE;

    } else {
        shadow_offset.x = 0;
        shadow_offset.y = 0;
    }

    UpdateUnitDrawZones();

    if (team_visibility) {
        RefreshScreen();
    }
}

void UnitInfo::GainExperience(int32_t experience_gain) {
    if (flags & REGENERATING_UNIT) {
        experience += experience_gain;

        if (experience >= 15) {
            int32_t upgrade_topic;
            int32_t upgrade_cost;
            int32_t upgrade_level;
            bool is_upgraded;

            SmartPointer<UnitValues> unit_values =
                UnitsManager_TeamInfo[team].team_units->GetCurrentUnitValues(unit_type);

            is_upgraded = false;

            UnitsManager_TeamInfo[team].team_units->SetCurrentUnitValues(unit_type, *base_values);

            upgrade_topic = ExpResearchTopics[Randomizer_Generate(sizeof(ExpResearchTopics))];
            upgrade_cost = TeamUnits_GetUpgradeCost(team, unit_type, upgrade_topic);

            while (upgrade_cost <= experience) {
                upgrade_level = TeamUnits_UpgradeOffsetFactor(team, unit_type, upgrade_topic);

                if (upgrade_topic == RESEARCH_TOPIC_HITS || upgrade_topic == RESEARCH_TOPIC_SPEED) {
                    auto current_level = base_values->GetAttribute(upgrade_topic);

                    if (current_level == UINT16_MAX) {
                        upgrade_topic = ExpResearchTopics[Randomizer_Generate(sizeof(ExpResearchTopics))];
                        upgrade_cost = TeamUnits_GetUpgradeCost(team, unit_type, upgrade_topic);

                        continue;
                    }

                    if (current_level + upgrade_level > UINT16_MAX) {
                        upgrade_cost = static_cast<float>(upgrade_cost) * static_cast<float>(upgrade_level) /
                                           (UINT16_MAX - current_level) +
                                       0.5f;
                        upgrade_level = UINT16_MAX - current_level;
                    }
                }

                experience -= upgrade_cost;

                if (!is_upgraded) {
                    base_values = new UnitValues(*base_values);
                    is_upgraded = true;
                }

                base_values->AddAttribute(upgrade_topic, upgrade_level);

                upgrade_topic = ExpResearchTopics[Randomizer_Generate(sizeof(ExpResearchTopics))];
                upgrade_cost = TeamUnits_GetUpgradeCost(team, unit_type, upgrade_topic);
            }

            if (is_upgraded) {
                if (team == GameManager_PlayerTeam) {
                    SmartString string;

                    string.Sprintf(80, _(d6a7), ResourceManager_GetUnit(unit_type).GetSingularName().data(), grid_x + 1,
                                   grid_y + 1);
                    MessageManager_DrawMessage(string.GetCStr(), 0, this, Point(grid_x, grid_y));
                }

                base_values->UpdateVersion();
                base_values->MarkAsInUse();

                if (Remote_IsNetworkGame) {
                    Remote_SendNetPacket_20(this);
                }

                if (GameManager_SelectedUnit == this) {
                    GameManager_MenuUnitSelect(this);
                }
            }

            UnitsManager_TeamInfo[team].team_units->SetCurrentUnitValues(unit_type, *unit_values);
        }
    }
}

void UnitInfo::RemoveDelayedTasks() {
    for (SmartList<Task>::Iterator it = delayed_tasks.Begin(); it != delayed_tasks.End(); ++it) {
        (*it).RemoveUnit(*this);
    }

    delayed_tasks.Clear();
}

void UnitInfo::AttackUnit(UnitInfo* enemy, int32_t attack_potential, int32_t direction) {
    int32_t attack_damage = UnitsManager_GetAttackDamage(enemy, this, attack_potential);

    if (hits > 0) {
        damaged_this_turn = true;

        if (!enemy->IsVisibleToTeam(team)) {
            Ai_UnitSpotted(enemy, team);
        }

        --pin_count;

        if (pin_count < 0) {
            pin_count = 0;
        }

        if (attack_damage > hits) {
            attack_damage = hits;
        }

        hits = std::max<uint16_t>(0, hits - attack_damage);

        if (hits > 0) {
            ScheduleDelayedTasks(true);

            if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                Ai_AddUnitToTrackerList(this);
            }
        }

        if (hits == 0) {
            UnitsManager_DelayedAttackTargets[team].Remove(*this);
            UnitsManager_PendingAttacks.Remove(*this);

            RemoveTasks();
            RemoveDelayedTasks();

            Ai_RemoveUnit(this);

            if (unit_type == INFANTRY || unit_type == COMMANDO) {
                angle = (direction + 5) & 0x07;
            }
        }

        if (team == GameManager_PlayerTeam) {
            if (enemy->team != team) {
                GameManager_NotifyEvent(this, true);
            }

        } else if (IsVisibleToTeam(GameManager_PlayerTeam) && hits == 0) {
            const char* formats[] = {_(e6d7), _(2962), _(01c2)};

            const Unit& base_unit = ResourceManager_GetUnit(unit_type);
            Point position(grid_x, grid_y);
            SmartString message;

            message.Sprintf(80, formats[base_unit.GetGender()], base_unit.GetSingularName().data(), grid_x + 1,
                            grid_y + 1);

            MessageManager_DrawMessage(message.GetCStr(), 0, this, position);
        }

        CheckIfDestroyed();

        if (hits == 0 || (orders != ORDER_EXPLODE && state != ORDER_STATE_DESTROY)) {
            UnitsManager_SetNewOrderInt(this, ORDER_EXPLODE, ORDER_STATE_INIT);
        }

        FollowUnit();

        if (UnitsManager_TeamInfo[enemy->team].team_type != TEAM_TYPE_REMOTE &&
            UnitsManager_TeamInfo[enemy->team].team_type != TEAM_TYPE_ELIMINATED) {
            enemy->GainExperience(
                (GetBaseValues()->GetAttribute(ATTRIB_TURNS) * attack_damage * 5) /
                (enemy->GetBaseValues()->GetAttribute(ATTRIB_TURNS) * GetBaseValues()->GetAttribute(ATTRIB_HITS)));
        }

        if (GameManager_SelectedUnit == this && hits > 0) {
            GameManager_UpdateInfoDisplay(this);
        }
    }
}

bool UnitInfo::ExecuteMove() {
    bool result;

    RefreshScreen();

    if (pin_count > 0) {
        result = false;
    } else {
        if (path) {
            if ((flags & MOBILE_AIR_UNIT) && !(flags & HOVERING)) {
                UnitsManager_SetNewOrderInt(this, ORDER_TAKE_OFF, ORDER_STATE_INIT);
                Ai_SetTasksPendingFlag("plane takeoff");

                result = false;

            } else {
                if (speed) {
                    Ai_SetTasksPendingFlag("moving");
                }

                result = path->Execute(this);
            }

        } else {
            if (flags & MISSILE_UNIT) {
                SmartPointer<UnitInfo> parent;

                parent = GetParent();
                parent->Attack(grid_x, grid_y);

                UnitsManager_DestroyUnit(this);

                result = false;

            } else {
                orders = ORDER_AWAIT;
                state = ORDER_STATE_EXECUTING_ORDER;
                result = false;
            }
        }
    }

    return result;
}

void UnitInfo::ClearBuildListAndPath() {
    path = nullptr;

    if (GetParent() != nullptr) {
        orders = ORDER_BUILD;
        state = ORDER_STATE_UNIT_READY;
    } else {
        orders = ORDER_AWAIT;
        state = ORDER_STATE_EXECUTING_ORDER;
    }

    build_list.Clear();
}

void UnitInfo::Build() {
    ResourceID build_unit_type;

    build_unit_type = *build_list[0];
    orders = ORDER_BUILD;

    if (path != nullptr) {
        if (UnitsManager_IssueBuildOrder(this, &grid_x, &grid_y, build_unit_type)) {
            move_to_grid_x = grid_x;
            move_to_grid_y = grid_y;

            build_time = BuildMenu_GetTurnsToBuild(build_unit_type, team);

            StartBuilding();

        } else {
            SetParent(nullptr);

            ClearBuildListAndPath();
            ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_IDLE);
        }

    } else {
        ClearBuildListAndPath();
    }

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
        GameManager_UpdateDrawBounds();
    }
}

void UnitInfo::Move() {
    SmartPointer<UnitInfo> unit;
    bool team_visibility;

    do {
        if (pin_count > 0) {
            return;
        }

        RefreshScreen();

        if (velocity < max_velocity) {
            velocity += GameManager_FastMovement ? 2 : 1;
        }

        if (velocity > max_velocity) {
            velocity = max_velocity;
        }

        if (GameManager_SelectedUnit == this) {
            if (speed > 0 && !path->IsEndStep()) {
                if (GetSfxType() != Unit::SFX_TYPE_DRIVE && GetSfxType() != Unit::SFX_TYPE_STOP) {
                    ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_DRIVE);
                }

            } else {
                ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_STOP);
            }
        }

        int32_t unit_velocity = velocity;

        if (unit_velocity + moved >= GFX_MAP_TILE_SIZE - (unit_velocity / 2)) {
            unit_velocity = GFX_MAP_TILE_SIZE - moved;

            if (unit_type != COMMANDO && unit_type != INFANTRY) {
                if (speed == 0 || path->IsEndStep()) {
                    unit_velocity /= 2;

                    if (unit_velocity == 0) {
                        unit_velocity = 1;
                    }

                    velocity = unit_velocity;
                    max_velocity = unit_velocity;
                }
            }
        }

        moved += unit_velocity;

        int32_t step_x = DIRECTION_OFFSETS[angle].x;
        int32_t step_y = DIRECTION_OFFSETS[angle].y;

        int32_t distance_x = step_x * unit_velocity;
        int32_t distance_y = step_y * unit_velocity;

        int32_t offset_x = ((x + distance_x - step_x) / GFX_MAP_TILE_SIZE) - grid_x;
        int32_t offset_y = ((y + distance_y - step_y) / GFX_MAP_TILE_SIZE) - grid_y;

        if (offset_x || offset_y) {
            unit = MakeCopy();

            Hash_MapHash.Remove(this);
            ClearInTransitFlag();
        }

        team_visibility = visible_to_team[GameManager_PlayerTeam];

        OffsetDrawZones(distance_x, distance_y);

        if (offset_x || offset_y) {
            grid_x = x / GFX_MAP_TILE_SIZE;
            grid_y = y / GFX_MAP_TILE_SIZE;

            if (path) {
                GroundPath* ground_path = dynamic_cast<GroundPath*>(&*path);
                SmartObjectArray<PathStep> path_steps = ground_path->GetSteps();
                uint32_t path_step_index = ground_path->GetStepIndex();

                path_steps[path_step_index]->x -= offset_x;
                path_steps[path_step_index]->y -= offset_y;
            }

            Access_UpdateMapStatus(this, true);
            Access_UpdateMapStatus(&*unit, false);
        }

        if (visible_to_team[GameManager_PlayerTeam]) {
            team_visibility = true;
        }

        if (team_visibility || GameManager_MaxSpy) {
            RefreshScreen();
        }

        if (unit_type == COMMANDO || unit_type == INFANTRY) {
            if (image_index + 8 > image_index_max) {
                DrawSpriteFrame(image_base + angle);

            } else {
                DrawSpriteFrame(image_index + 8);
            }
        }

        if (moved == GFX_MAP_TILE_SIZE) {
            path->UpdateUnitAngle(this);

            if (orders == ORDER_BUILD && build_list.GetCount() > 0 &&
                Access_IsAccessible(*build_list[0], team, grid_x, grid_y, AccessModifier_EnemySameClassBlocks)) {
                Build();

            } else {
                state = ORDER_STATE_IN_PROGRESS;
            }

            if (orders == ORDER_MOVE || orders == ORDER_MOVE_TO_UNIT || orders == ORDER_MOVE_TO_ATTACK) {
                FollowUnit();
            }

            if (GameManager_SelectedUnit == this) {
                ResourceManager_GetSoundManager().UpdateSfxPosition(this);
            }

            Ai_MarkMineMapPoint(Point(grid_x, grid_y), team);

            SmartPointer<UnitInfo> mine(Access_GetEnemyMineOnSentry(team, grid_x, grid_y));

            if (mine) {
                mine->SetOrder(ORDER_EXPLODE);
                mine->state = ORDER_STATE_EXPLODE;
                mine->visible_to_team[team] = true;

                Ai_UnitSpotted(&*mine, team);

                if (orders == ORDER_MOVE_TO_ATTACK) {
                    orders = ORDER_AWAIT;
                }

                BlockedOnPathRequest(true, true);

                return;
            }

            if (laying_state == 2) {
                PlaceMine();
            }

            if (laying_state == 1) {
                PickUpMine();
            }
        }

        if (Remote_IsNetworkGame) {
            return;
        }

        if (state == ORDER_STATE_IN_PROGRESS && !team_visibility) {
            ExecuteMove();
        }

    } while (state == ORDER_STATE_IN_TRANSITION && !team_visibility);
}

void UnitInfo::AllocateUnitList() { unit_list = new SmartList<UnitInfo>(); }

void UnitInfo::AssignUnitList(UnitInfo* unit) {
    unit_list = unit->GetUnitList();

    if (!unit_list) {
        unit->AllocateUnitList();
        unit->AssignUnitList(unit);
        unit_list = unit->GetUnitList();
    }

    unit_list->PushBack(*this);
}

void UnitInfo::ClearUnitList() {
    if (unit_list && unit_list->Remove(*this)) {
        if (unit_list->GetCount()) {
            if (unit_list->GetCount() == 1) {
                (*unit_list->Begin()).ClearUnitList();
            } else {
                Access_ProcessNewGroupOrder(this);
            }

        } else {
            delete unit_list;
        }

        unit_list = nullptr;
        group_speed = 0;
    }
}

void UnitInfo::MoveToFrontInUnitList() {
    unit_list->Remove(*this);
    unit_list->PushFront(*this);
}

UnitInfo* UnitInfo::GetConnectedBuilding(uint32_t connector) {
    int32_t grid_size;
    UnitInfo* result{nullptr};

    if (flags & BUILDING) {
        grid_size = 2;
    } else {
        grid_size = 1;
    }

    switch (connector) {
        case CONNECTOR_NORTH_LEFT: {
            result = Access_GetTeamBuilding(team, grid_x, grid_y - 1);
        } break;

        case CONNECTOR_NORTH_RIGHT: {
            result = Access_GetTeamBuilding(team, grid_x + 1, grid_y - 1);
        } break;

        case CONNECTOR_EAST_TOP: {
            result = Access_GetTeamBuilding(team, grid_x + grid_size, grid_y);
        } break;

        case CONNECTOR_EAST_BOTTOM: {
            result = Access_GetTeamBuilding(team, grid_x + grid_size, grid_y + 1);
        } break;

        case CONNECTOR_SOUTH_LEFT: {
            result = Access_GetTeamBuilding(team, grid_x, grid_y + grid_size);
        } break;

        case CONNECTOR_SOUTH_RIGHT: {
            result = Access_GetTeamBuilding(team, grid_x + 1, grid_y + grid_size);
        } break;

        case CONNECTOR_WEST_TOP: {
            result = Access_GetTeamBuilding(team, grid_x - 1, grid_y);
        } break;

        case CONNECTOR_WEST_BOTTOM: {
            result = Access_GetTeamBuilding(team, grid_x - 1, grid_y + 1);
        } break;
    }

    return result;
}

void UnitInfo::AttachComplex(Complex* complex) {
    if (this->complex != complex) {
        complex->Grow(*this);

        if (this->complex != nullptr) {
            this->complex->Shrink(*this);
        }

        this->complex = complex;

        if (connectors & CONNECTOR_NORTH_LEFT) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_NORTH_RIGHT) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_EAST_TOP) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_EAST_TOP);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_EAST_BOTTOM) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_SOUTH_RIGHT) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_SOUTH_LEFT) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_WEST_BOTTOM) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }

        if (connectors & CONNECTOR_WEST_TOP) {
            UnitInfo* unit = GetConnectedBuilding(CONNECTOR_WEST_TOP);

            if (unit) {
                unit->AttachComplex(complex);
            }
        }
    }
}

void UnitInfo::AttachToPrimaryComplex() {
    SmartPointer<Complex> unit_complex;

    if (complex) {
        complex->Shrink(*this);
        complex = nullptr;
    }

    if (connectors & CONNECTOR_NORTH_LEFT) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_NORTH_RIGHT) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_EAST_TOP) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_EAST_TOP);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_EAST_BOTTOM) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_SOUTH_RIGHT) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_SOUTH_LEFT) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_WEST_BOTTOM) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (connectors & CONNECTOR_WEST_TOP) {
        UnitInfo* building = GetConnectedBuilding(CONNECTOR_WEST_TOP);

        if (!unit_complex || building->complex->GetId() < unit_complex->GetId()) {
            unit_complex = building->complex;
        }
    }

    if (!unit_complex) {
        unit_complex = CreateComplex(team);
    }

    AttachComplex(&*unit_complex);
}

void UnitInfo::TestConnections() {
    if (!(connectors & CONNECTION_BEING_TESTED)) {
        UnitInfo* building = nullptr;

        connectors |= CONNECTION_BEING_TESTED;

        if (connectors & CONNECTOR_NORTH_LEFT) {
            building = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_NORTH_RIGHT) {
            building = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_EAST_TOP) {
            building = GetConnectedBuilding(CONNECTOR_EAST_TOP);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_EAST_BOTTOM) {
            building = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_SOUTH_RIGHT) {
            building = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_SOUTH_LEFT) {
            building = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_WEST_BOTTOM) {
            building = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

            if (building) {
                building->TestConnections();
            }
        }

        if (connectors & CONNECTOR_WEST_TOP) {
            building = GetConnectedBuilding(CONNECTOR_WEST_TOP);

            if (building) {
                building->TestConnections();
            }
        }
    }
}

UnitInfo* UnitInfo::GetFirstUntestedConnection() {
    UnitInfo* building;

    if (connectors & CONNECTOR_NORTH_LEFT) {
        building = GetConnectedBuilding(CONNECTOR_NORTH_LEFT);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_NORTH_RIGHT) {
        building = GetConnectedBuilding(CONNECTOR_NORTH_RIGHT);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_EAST_TOP) {
        building = GetConnectedBuilding(CONNECTOR_EAST_TOP);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_EAST_BOTTOM) {
        building = GetConnectedBuilding(CONNECTOR_EAST_BOTTOM);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_SOUTH_RIGHT) {
        building = GetConnectedBuilding(CONNECTOR_SOUTH_RIGHT);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_SOUTH_LEFT) {
        building = GetConnectedBuilding(CONNECTOR_SOUTH_LEFT);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_WEST_BOTTOM) {
        building = GetConnectedBuilding(CONNECTOR_WEST_BOTTOM);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    if (connectors & CONNECTOR_WEST_TOP) {
        building = GetConnectedBuilding(CONNECTOR_WEST_TOP);

        if (building && !(building->connectors & CONNECTION_BEING_TESTED)) {
            return building;
        }
    }

    return nullptr;
}

void UnitInfo::DetachComplex() {
    complex->Shrink(*this);
    complex = nullptr;

    UnitInfo* building = GetFirstUntestedConnection();

    connectors |= CONNECTION_BEING_TESTED;

    if (building) {
        SmartPointer<Complex> building_complex(building->GetComplex());

        building->TestConnections();

        do {
            building = GetFirstUntestedConnection();

            if (building) {
                SmartPointer<Complex> new_complex(CreateComplex(team));

                complex = new_complex;

                building->TestConnections();
                building->AttachComplex(&*new_complex);

                Access_UpdateResourcesTotal(&*new_complex);

                complex = nullptr;
            }

        } while (building);

        Access_UpdateResourcesTotal(&*building_complex);
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        (*it).connectors &= ~CONNECTION_BEING_TESTED;
    }
}

void UnitInfo::FileLoad(SmartFileReader& file) noexcept {
    file.Read(unit_type);
    file.Read(id);
    file.Read(flags);
    file.Read(x);
    file.Read(y);
    file.Read(grid_x);
    file.Read(grid_y);

    SDL_assert(grid_x >= 0 && grid_x < ResourceManager_MapSize.x);
    SDL_assert(grid_y >= 0 && grid_y < ResourceManager_MapSize.y);

    delete[] name;

    uint16_t name_length;

    file.Read(name_length);

    if (name_length) {
        name = new (std::nothrow) char[name_length + 1];

        file.Read(name, name_length);

        name[name_length] = '\0';

    } else {
        name = nullptr;
    }

    file.Read(shadow_offset);
    file.Read(team);
    file.Read(unit_id);
    file.Read(brightness);
    file.Read(angle);
    file.Read(visible_to_team);
    file.Read(spotted_by_team);
    file.Read(max_velocity);
    file.Read(velocity);
    file.Read(sound);
    file.Read(scaler_adjust);
    file.Read(sprite_bounds);
    file.Read(shadow_bounds);
    file.Read(turret_angle);
    file.Read(turret_offset_x);
    file.Read(turret_offset_y);
    file.Read(total_images);
    file.Read(image_base);
    file.Read(turret_image_base);
    file.Read(firing_image_base);
    file.Read(connector_image_base);
    file.Read(image_index);
    file.Read(turret_image_index);
    file.Read(image_index_max);
    file.Read(orders);
    file.Read(state);
    file.Read(prior_orders);
    file.Read(prior_state);
    file.Read(laying_state);

    if (file.GetFormat() == SmartFileFormat::V70) {
        int16_t target_grid_x_v70;
        int16_t target_grid_y_v70;

        file.Read(target_grid_x_v70);
        file.Read(target_grid_y_v70);

        move_to_grid_x = target_grid_x_v70;
        move_to_grid_y = target_grid_y_v70;
        fire_on_grid_x = target_grid_x_v70;
        fire_on_grid_y = target_grid_y_v70;

    } else {
        file.Read(move_to_grid_x);
        file.Read(move_to_grid_y);
        file.Read(fire_on_grid_x);
        file.Read(fire_on_grid_y);
    }

    file.Read(build_time);
    file.Read(total_mining);
    file.Read(raw_mining);
    file.Read(fuel_mining);
    file.Read(gold_mining);
    file.Read(raw_mining_max);
    file.Read(gold_mining_max);
    file.Read(fuel_mining_max);

    if (file.GetFormat() == SmartFileFormat::V70) {
        uint8_t hits_v70;
        file.Read(hits_v70);

        hits = hits_v70;

    } else {
        file.Read(hits);
    }

    if (file.GetFormat() == SmartFileFormat::V70) {
        uint8_t speed_v70;

        file.Read(speed_v70);

        speed = speed_v70;

    } else {
        file.Read(speed);
    }

    file.Read(shots);
    file.Read(move_and_fire);
    file.Read(storage);

    if (file.GetFormat() == SmartFileFormat::V70) {
        if ((flags & REGENERATING_UNIT) || unit_type == COMMANDO) {
            experience = storage;
            storage = 0;

        } else {
            experience = 0;
        }

        transfer_cargo = move_to_grid_x;
        stealth_dice_roll = static_cast<uint8_t>(move_to_grid_x);

    } else {
        file.Read(experience);
        file.Read(transfer_cargo);
        file.Read(stealth_dice_roll);
    }

    file.Read(ammo);
    file.Read(targeting_mode);
    file.Read(enter_mode);
    file.Read(cursor);

    if (file.GetFormat() == SmartFileFormat::V70) {
        int8_t recoil_delay;

        file.Read(recoil_delay);

        if (recoil_delay < 0) {
            recoil_delay = 0;
        }

        if (orders == ORDER_DISABLE) {
            disabled_turns_remaining = static_cast<uint8_t>(recoil_delay);
            firing_recoil_frames = 0;

        } else {
            firing_recoil_frames = static_cast<uint8_t>(recoil_delay);
            disabled_turns_remaining = 0;
        }

    } else {
        file.Read(firing_recoil_frames);
        file.Read(disabled_turns_remaining);
    }

    file.Read(delayed_reaction);
    file.Read(damaged_this_turn);
    file.Read(research_topic);
    file.Read(moved);
    file.Read(bobbed);
    file.Read(shake_effect_state);
    file.Read(engine);
    file.Read(weapon);

    if (file.GetFormat() == SmartFileFormat::V70) {
        uint8_t comm;
        uint8_t fuel_distance;
        bool energized;

        file.Read(comm);
        file.Read(fuel_distance);
        file.Read(move_fraction);
        file.Read(energized);

    } else {
        file.Read(move_fraction);
    }

    file.Read(repeat_build);
    file.Read(build_rate);
    file.Read(disabled_reaction_fire);
    file.Read(auto_survey);
    file.Read(ai_state_bits);

    ai_state_bits &= ~AI_STATE_MOVE_FINISHED_REMINDER;

    if (build_rate == 0) {
        build_rate = 1;
    }

    path = dynamic_cast<UnitPath*>(file.ReadObject());
    file.Read(connectors);
    base_values = dynamic_cast<UnitValues*>(file.ReadObject());
    complex = dynamic_cast<Complex*>(file.ReadObject());
    SetParent(dynamic_cast<UnitInfo*>(file.ReadObject()));
    enemy_unit = dynamic_cast<UnitInfo*>(file.ReadObject());

    UnitInfo_BuildList_FileLoad(&build_list, file);

    if (state == ORDER_STATE_NEW_ORDER || state == ORDER_STATE_MOVE_GETTING_PATH || state == ORDER_STATE_ISSUING_PATH ||
        state == ORDER_STATE_IN_TRANSITION || state == ORDER_STATE_IN_PROGRESS) {
        state = ORDER_STATE_EXECUTING_ORDER;
    }

    Init();

    UpdateUnitDrawZones();
}

void UnitInfo::FileSave(SmartFileWriter& file) noexcept {
    SDL_assert(grid_x >= 0 && grid_x < ResourceManager_MapSize.x);
    SDL_assert(grid_y >= 0 && grid_y < ResourceManager_MapSize.y);

    file.Write(unit_type);
    file.Write(id);
    file.Write(flags);
    file.Write(x);
    file.Write(y);
    file.Write(grid_x);
    file.Write(grid_y);

    if (name) {
        uint16_t name_length = strlen(name);

        file.Write(name_length);
        file.Write(name, name_length);

    } else {
        uint16_t name_length = 0;

        file.Write(name_length);
    }

    file.Write(shadow_offset);
    file.Write(team);
    file.Write(unit_id);
    file.Write(brightness);
    file.Write(angle);
    file.Write(visible_to_team);
    file.Write(spotted_by_team);
    file.Write(max_velocity);
    file.Write(velocity);
    file.Write(sound);
    file.Write(scaler_adjust);
    file.Write(sprite_bounds);
    file.Write(shadow_bounds);
    file.Write(turret_angle);
    file.Write(turret_offset_x);
    file.Write(turret_offset_y);
    file.Write(total_images);
    file.Write(image_base);
    file.Write(turret_image_base);
    file.Write(firing_image_base);
    file.Write(connector_image_base);
    file.Write(image_index);
    file.Write(turret_image_index);
    file.Write(image_index_max);
    file.Write(orders);
    file.Write(state);
    file.Write(prior_orders);
    file.Write(prior_state);
    file.Write(laying_state);
    file.Write(move_to_grid_x);
    file.Write(move_to_grid_y);
    file.Write(fire_on_grid_x);
    file.Write(fire_on_grid_y);
    file.Write(build_time);
    file.Write(total_mining);
    file.Write(raw_mining);
    file.Write(fuel_mining);
    file.Write(gold_mining);
    file.Write(raw_mining_max);
    file.Write(gold_mining_max);
    file.Write(fuel_mining_max);
    file.Write(hits);
    file.Write(speed);
    file.Write(shots);
    file.Write(move_and_fire);
    file.Write(storage);
    file.Write(experience);
    file.Write(transfer_cargo);
    file.Write(stealth_dice_roll);
    file.Write(ammo);
    file.Write(targeting_mode);
    file.Write(enter_mode);
    file.Write(cursor);
    file.Write(firing_recoil_frames);
    file.Write(disabled_turns_remaining);
    file.Write(delayed_reaction);
    file.Write(damaged_this_turn);
    file.Write(research_topic);
    file.Write(moved);
    file.Write(bobbed);
    file.Write(shake_effect_state);
    file.Write(engine);
    file.Write(weapon);
    file.Write(move_fraction);
    file.Write(repeat_build);
    file.Write(build_rate);
    file.Write(disabled_reaction_fire);
    file.Write(auto_survey);
    file.Write(ai_state_bits);
    file.WriteObject(&*path);
    file.Write(connectors);
    file.WriteObject(&*base_values);
    file.WriteObject(&*complex);
    file.WriteObject(GetParent());
    file.WriteObject(&*enemy_unit);

    UnitInfo_BuildList_FileSave(&build_list, file);
}

void UnitInfo::WritePacket(NetPacket& packet) {
    packet << team;
    packet << state;
    packet << repeat_build;
    packet << build_time;
    packet << build_rate;
    packet << move_to_grid_x;
    packet << move_to_grid_y;
    packet << fire_on_grid_x;
    packet << fire_on_grid_y;

    packet << build_list.GetCount();

    for (uint32_t i = 0; i < build_list.GetCount(); ++i) {
        packet << *build_list[i];
    }
}

void UnitInfo::ReadPacket(NetPacket& packet) {
    uint32_t unit_count;
    ResourceID list_item;

    packet >> team;
    packet >> state;
    packet >> repeat_build;
    packet >> build_time;
    packet >> build_rate;
    packet >> move_to_grid_x;
    packet >> move_to_grid_y;
    packet >> fire_on_grid_x;
    packet >> fire_on_grid_y;

    build_list.Clear();

    packet >> unit_count;

    for (uint32_t i = 0; i < unit_count; ++i) {
        packet >> list_item;

        build_list.PushBack(&list_item);
    }

    StartBuilding();
}

void UnitInfo::UpdateTurretAngle(int32_t turret_angle_, bool redraw) {
    const Unit& base_unit = ResourceManager_GetUnit(unit_type);
    const Point& offset = base_unit.GetFrameInfo().angle_offsets[angle];

    turret_angle = turret_angle_;

    turret_offset_x = offset.x;
    turret_offset_y = offset.y;

    if (redraw) {
        DrawSpriteTurretFrame(turret_image_base + turret_angle);
    }
}

void UnitInfo::Attack(int32_t grid_x, int32_t grid_y) {
    SmartPointer<UnitInfo> target(Access_GetAttackTarget(this, grid_x, grid_y));
    UnitInfo* enemy = nullptr;

    if (!target) {
        target = Access_GetAttackTarget2(this, grid_x, grid_y);

        if (target) {
            target->visible_to_team[team] = true;

        } else {
            enemy = Access_GetTeamUnit(grid_x, grid_y, team, SELECTABLE);

            if (!enemy && unit_type != ANTIAIR && unit_type != SP_FLAK && unit_type != FASTBOAT) {
                SmartPointer<UnitInfo> explosion =
                    UnitsManager_DeployUnit(HITEXPLD, team, nullptr, grid_x, grid_y, 0, true);

                if (Access_GetModifiedSurfaceType(grid_x, grid_y) == SURFACE_TYPE_LAND) {
                    ResourceManager_GetSoundManager().PlaySfx(&*explosion, Unit::SFX_TYPE_HIT);

                } else {
                    ResourceManager_GetSoundManager().PlaySfx(&*explosion, Unit::SFX_TYPE_EXPLOAD);
                }
            }
        }
    }

    int32_t target_angle;

    if (flags & MISSILE_UNIT) {
        target_angle = angle / 2;

    } else {
        target_angle = UnitsManager_GetTargetAngle(grid_x - this->grid_x, grid_y - this->grid_y);
    }

    if (GetBaseValues()->GetAttribute(ATTRIB_ATTACK_RADIUS)) {
        AttackAreaTargets(grid_x, grid_y);

    } else if (target) {
        target->AttackUnit(this, 0, target_angle);

    } else if (enemy && Access_IsValidAttackTarget(this, enemy, Point(grid_x, grid_y))) {
        enemy->AttackUnit(this, 0, target_angle);
    }

    if (shots == 0) {
        targeting_mode = 0;
    }
}

void UnitInfo::StartBuilding() {
    SDL_assert(build_list.GetCount() > 0);

    ResourceID unit_to_build = *build_list[0];

    if (flags & STATIONARY) {
        complex->material -= Cargo_GetRawConsumptionRate(unit_type, GetMaxAllowedBuildRate());
        complex->power -= Cargo_GetPowerConsumptionRate(unit_type);
        complex->workers -= Cargo_GetLifeConsumptionRate(unit_type);

        state = ORDER_STATE_EXECUTING_ORDER;

        if (GameManager_PlayerTeam == team) {
            GameManager_OptimizeProduction(team, &*complex, true, true);
        }

        DrawSpriteFrame(image_base + 1);

        if (GameManager_SelectedUnit == this) {
            GameManager_UpdateInfoDisplay(&*GameManager_SelectedUnit);

            ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_POWER_CONSUMPTION_START);
        }

    } else {
        DeployConstructionSiteUtilities(unit_to_build);
        Redraw();

        moved = 0;
        state = ORDER_STATE_BUILD_IN_PROGRESS;

        if (unit_type == CONSTRCT) {
            UnitsManager_SetNewOrderInt(this, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_INIT);

        } else {
            PrepareConstructionSite(unit_to_build);
            DrawSpriteFrame(image_index + 16);
        }

        ClearUnitList();
    }
}

void UnitInfo::InitStealthStatus() {
    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
        if (unit_type == LANDMINE || unit_type == SEAMINE || unit_type == COMMANDO || unit_type == SUBMARNE) {
            visible_to_team[i] = 0;
        } else {
            visible_to_team[i] = GameManager_AllVisible;
        }

        spotted_by_team[i] = 0;
    }

    visible_to_team[team] = 1;
}

void UnitInfo::SpotByTeam(uint16_t team) {
    if (this->team != team && orders != ORDER_IDLE && !visible_to_team[team]) {
        visible_to_team[team] = true;
        spotted_by_team[team] = true;

        if (UnitsManager_TeamInfo[this->team].team_type == TEAM_TYPE_COMPUTER) {
            Ai_AddUnitToTrackerList(this);
        }

        RefreshScreen();

        if (unit_type == COMMANDO && orders == ORDER_AWAIT) {
            DrawBustedCommando();
        }

        if (unit_type == SUBMARNE || unit_type == CLNTRANS) {
            image_base = 8;

            DrawSpriteFrame(image_base + angle);
        }

        Ai_UnitSpotted(this, team);

        if (team == GameManager_PlayerTeam) {
            RadarPing();

        } else if (unit_type == SUBMARNE && this->team == GameManager_PlayerTeam) {
            ResourceManager_GetSoundManager().PlayVoice(V_M201, V_F201);
        }
    }
}

void UnitInfo::Draw(uint16_t team) {
    if (this->team != team &&
        (visible_to_team[team] ||
         ((unit_type == COMMANDO || unit_type == SUBMARNE || unit_type == CLNTRANS) && image_base >= 8))) {
        visible_to_team[team] = false;

        if (unit_type == COMMANDO || unit_type == SUBMARNE || unit_type == CLNTRANS) {
            for (team = PLAYER_TEAM_RED;
                 (team < PLAYER_TEAM_MAX - 1) &&
                 (this->team == team || UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_NONE ||
                  !visible_to_team[team]);
                 ++team) {
            }
        }

        if (team == PLAYER_TEAM_ALIEN) {
            if (UnitsManager_IsUnitUnderWater(this)) {
                image_base = 0;

                DrawSpriteFrame(image_base + angle);

            } else if (unit_type == COMMANDO && orders == ORDER_AWAIT && image_base == 0) {
                DrawSpriteFrame(angle);
            }
        }

        RefreshScreen();
    }
}

void UnitInfo::DrawStealth(uint16_t team) {
    if (spotted_by_team[team] || (!UnitsManager_IsUnitUnderWater(this) && unit_type != COMMANDO &&
                                  unit_type != LANDMINE && unit_type != SEAMINE)) {
        SpotByTeam(team);

    } else if ((UnitsManager_IsUnitUnderWater(this) && UnitsManager_TeamInfo[team].heat_map &&
                !UnitsManager_TeamInfo[team].heat_map->GetStealthSea(grid_x, grid_y)) ||
               (unit_type == COMMANDO && UnitsManager_TeamInfo[team].heat_map &&
                !UnitsManager_TeamInfo[team].heat_map->GetStealthLand(grid_x, grid_y))) {
        Draw(team);
    }
}

void UnitInfo::Resupply() {
    if ((flags & STATIONARY) && base_values->GetAttribute(ATTRIB_ATTACK) > 0 &&
        base_values->GetAttribute(ATTRIB_ROUNDS) > ammo) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        if (materials.raw > 0) {
            ammo = base_values->GetAttribute(ATTRIB_AMMO);
            complex->Transfer(-1, 0, 0);
        }
    }

    Regenerate();
}

int32_t UnitInfo::GetRawConsumptionRate() { return Cargo_GetRawConsumptionRate(unit_type, GetMaxAllowedBuildRate()); }

void UnitInfo::UpdateProduction() {
    if (orders == ORDER_BUILD && state != ORDER_STATE_UNIT_READY && (flags & MOBILE_LAND_UNIT)) {
        int32_t maximum_build_rate = BuildMenu_GetMaxPossibleBuildRate(unit_type, build_time, storage);

        build_rate = std::min(static_cast<int32_t>(build_rate), maximum_build_rate);

        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER ||
            ResourceManager_GetSettings()->GetNumericValue("opponent") < OPPONENT_TYPE_MASTER ||
            UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], *build_list[0])
                    ->GetAttribute(ATTRIB_TURNS) > 1) {
            storage -= Cargo_GetRawConsumptionRate(unit_type, build_rate);
        }
    }

    if (unit_type == COMMTWR && orders == ORDER_POWER_ON) {
        TeamUnits* team_units = UnitsManager_TeamInfo[team].team_units;
        uint32_t gold_reserves = team_units->GetGold();

        team_units->SetGold(Cargo_GetGoldConsumptionRate(unit_type) + gold_reserves);

        if (!gold_reserves && GameManager_PlayerTeam == team && team_units->GetGold() > 0) {
            ResourceManager_GetSoundManager().PlayVoice(V_M276, V_F276);
        }
    }

    if (unit_type == GREENHSE && orders == ORDER_POWER_ON) {
        ++storage;
        ++UnitsManager_TeamInfo[team].team_points;

        if (GameManager_PlayerTeam != team && UnitsManager_TeamInfo[team].team_points == 1) {
            ResourceManager_GetSoundManager().PlayVoice(static_cast<ResourceID>(V_M098 + team * 2),
                                                        static_cast<ResourceID>(V_M098 + team * 2 + 1));
        }
    }

    if (base_values->GetAttribute(ATTRIB_ROUNDS) > 0) {
        if (base_values->GetAttribute(ATTRIB_ROUNDS) > ammo) {
            shots = ammo;

            if (GameManager_SelectedUnit == this && GameManager_PlayerTeam == team) {
                ResourceManager_GetSoundManager().PlayVoice(V_M270, V_F271);
            }

        } else {
            if (base_values->GetAttribute(ATTRIB_ROUNDS) != shots) {
                shots = base_values->GetAttribute(ATTRIB_ROUNDS);
            }
        }
    }

    speed = base_values->GetAttribute(ATTRIB_SPEED);

    if (speed > 0) {
        for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX; ++i) {
            spotted_by_team[i] = false;
        }
    }

    if (orders == ORDER_DISABLE || (orders == ORDER_IDLE && prior_orders == ORDER_DISABLE)) {
        if (team != PLAYER_TEAM_ALIEN) {
            --disabled_turns_remaining;

            if (disabled_turns_remaining == 0) {
                SmartPointer<UnitInfo> unit_copy = MakeCopy();

                if (orders == ORDER_IDLE) {
                    prior_orders = ORDER_AWAIT;
                    prior_state = ORDER_STATE_EXECUTING_ORDER;

                } else {
                    RestoreOrders();
                    Access_UpdateMapStatus(this, true);
                    Access_UpdateMapStatus(&*unit_copy, false);

                    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER) {
                        GameManager_RenderMinimapDisplay = true;
                    }
                }
            }
        }
    }

    if (state == ORDER_STATE_READY_TO_EXECUTE_ORDER) {
        state = ORDER_STATE_EXECUTING_ORDER;
    }

    if (complex) {
        switch (ResourceManager_GetUnit(unit_type).GetCargoType()) {
            case Unit::CargoType::CARGO_TYPE_RAW: {
                storage = std::min(static_cast<int32_t>(complex->material), base_values->GetAttribute(ATTRIB_STORAGE));
                complex->material -= storage;
            } break;

            case Unit::CargoType::CARGO_TYPE_FUEL: {
                storage = std::min(static_cast<int32_t>(complex->fuel), base_values->GetAttribute(ATTRIB_STORAGE));
                complex->fuel -= storage;
            } break;

            case Unit::CargoType::CARGO_TYPE_GOLD: {
                storage = std::min(static_cast<int32_t>(complex->gold), base_values->GetAttribute(ATTRIB_STORAGE));
                complex->gold -= storage;
            } break;
        }
    }
}

ResourceID UnitInfo::GetConstructedUnitType() const {
    SDL_assert(build_list.GetCount() > 0);

    return *build_list[0];
}

bool UnitInfo::IsBridgeElevated() const { return (unit_type == BRIDGE) && (image_index != image_base); }

bool UnitInfo::IsInGroupZone(UnitInfoGroup* group) {
    bool result;

    if (shadow_bounds.ulx < group->GetBounds1()->lrx && shadow_bounds.lrx >= group->GetBounds1()->ulx &&
        shadow_bounds.uly < group->GetBounds1()->lry && shadow_bounds.lry >= group->GetBounds1()->uly) {
        result = true;

    } else if (sprite_bounds.ulx < group->GetBounds1()->lrx && sprite_bounds.lrx >= group->GetBounds1()->ulx &&
               sprite_bounds.uly < group->GetBounds1()->lry && sprite_bounds.lry >= group->GetBounds1()->uly) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

void UnitInfo::RenderShadow(Point point, int32_t image_id, Rect* bounds) {
    uint8_t* shadow_data = ResourceManager_GetUnit(unit_type).GetShadowData();

    if (shadow_data) {
        uint32_t scaling_factor;
        uint32_t zoom_level;
        struct ImageMultiFrameHeader* frame;

        scaling_factor = 1 << (scaler_adjust + 1);

        zoom_level = (2 * Gfx_ZoomLevel) / scaling_factor;

        if (zoom_level >= 8) {
            Gfx_ResourceBuffer = shadow_data;

            frame = GetSpriteFrame(reinterpret_cast<struct ImageMultiHeader*>(Gfx_ResourceBuffer), image_id);

            point -= shadow_offset;

            if (ResourceManager_DisableEnhancedGraphics) {
                scaling_factor /= 2;
            }

            if (Gfx_DecodeSpriteSetup(point, reinterpret_cast<uint8_t*>(frame), scaling_factor, bounds)) {
                Gfx_SpriteRowAddresses = reinterpret_cast<uint32_t*>(&frame->rows);
                Gfx_ColorIndices = color_cycling_lut;

                Gfx_DecodeShadow();
            }
        }
    }
}

void UnitInfo::RenderAirShadow(Rect* bounds) { RenderShadow(Point(x, y), image_index, bounds); }

void UnitInfo::RenderSprite(Point point, int32_t image_base, Rect* bounds) {
    uint8_t* sprite_data = ResourceManager_GetUnit(unit_type).GetSpriteData();

    if (sprite_data) {
        uint32_t scaling_factor;
        uint32_t zoom_level;
        struct ImageMultiFrameHeader* frame;

        scaling_factor = 1 << (scaler_adjust + 1);

        zoom_level = (2 * Gfx_ZoomLevel) / scaling_factor;

        if (zoom_level >= 4) {
            Gfx_ResourceBuffer = sprite_data;

            frame = GetSpriteFrame(reinterpret_cast<struct ImageMultiHeader*>(Gfx_ResourceBuffer), image_base);

            if (ResourceManager_DisableEnhancedGraphics) {
                scaling_factor /= 2;
            }

            if (Gfx_DecodeSpriteSetup(point, reinterpret_cast<uint8_t*>(frame), scaling_factor, bounds)) {
                Gfx_SpriteRowAddresses = reinterpret_cast<uint32_t*>(&frame->rows);
                Gfx_ColorIndices = color_cycling_lut;
                Gfx_UnitBrightnessBase = brightness;

                if (zoom_level < 8) {
                    if (flags & HASH_TEAM_RED) {
                        Gfx_TeamColorIndexBase = COLOR_RED;

                    } else if (flags & HASH_TEAM_GREEN) {
                        Gfx_TeamColorIndexBase = COLOR_GREEN;

                    } else if (flags & HASH_TEAM_BLUE) {
                        Gfx_TeamColorIndexBase = COLOR_BLUE;

                    } else if (flags & HASH_TEAM_GRAY) {
                        Gfx_TeamColorIndexBase = 0xFF;

                    } else {
                        Gfx_TeamColorIndexBase = COLOR_YELLOW;
                    }

                } else {
                    Gfx_TeamColorIndexBase = COLOR_BLACK;
                }

                Gfx_DecodeSprite();
            }
        }
    }
}

void UnitInfo::Render(Rect* bounds) {
    Point position(x, y);

    RenderSprite(position, image_index, bounds);

    if (flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        position.x += turret_offset_x;
        position.y += turret_offset_y;

        RenderSprite(position, turret_image_index, bounds);
    }
}

void UnitInfo::RenderWithConnectors(Rect* bounds) {
    Point position(x, y);

    RenderShadow(position, image_index, bounds);
    RenderSprite(position, image_index, bounds);

    if (connectors) {
        if (connectors & CONNECTOR_NORTH_LEFT) {
            RenderShadow(position, connector_image_base, bounds);
            RenderSprite(position, connector_image_base, bounds);
        }

        if (connectors & CONNECTOR_NORTH_RIGHT) {
            RenderShadow(position, connector_image_base + 4, bounds);
            RenderSprite(position, connector_image_base + 4, bounds);
        }

        if (connectors & CONNECTOR_SOUTH_LEFT) {
            RenderShadow(position, connector_image_base + 2, bounds);
            RenderSprite(position, connector_image_base + 2, bounds);
        }

        if (connectors & CONNECTOR_SOUTH_RIGHT) {
            RenderShadow(position, connector_image_base + 6, bounds);
            RenderSprite(position, connector_image_base + 6, bounds);
        }

        if (connectors & CONNECTOR_EAST_TOP) {
            RenderShadow(position, connector_image_base + 1, bounds);
            RenderSprite(position, connector_image_base + 1, bounds);
        }

        if (connectors & CONNECTOR_EAST_BOTTOM) {
            RenderShadow(position, connector_image_base + 5, bounds);
            RenderSprite(position, connector_image_base + 5, bounds);
        }

        if (connectors & CONNECTOR_WEST_TOP) {
            RenderShadow(position, connector_image_base + 3, bounds);
            RenderSprite(position, connector_image_base + 3, bounds);
        }

        if (connectors & CONNECTOR_WEST_BOTTOM) {
            RenderShadow(position, connector_image_base + 7, bounds);
            RenderSprite(position, connector_image_base + 7, bounds);
        }
    }

    if (flags & (TURRET_SPRITE | SPINNING_TURRET)) {
        position.x += turret_offset_x;
        position.y += turret_offset_y;

        RenderShadow(position, turret_image_index, bounds);
        RenderSprite(position, turret_image_index, bounds);
    }
}

int32_t UnitInfo::GetMaxAllowedBuildRate() {
    int32_t result;

    if (flags & MOBILE_LAND_UNIT) {
        result = build_rate;

    } else {
        result = std::min(static_cast<int32_t>(build_rate),
                          BuildMenu_GetMaxPossibleBuildRate(unit_type, build_time, storage));
    }

    return result;
}

void UnitInfo::StopMovement() {
    AILOG(log, "{} at [{},{}]: Emergency Stop", ResourceManager_GetUnit(unit_type).GetSingularName().data(), grid_x + 1,
          grid_y + 1);

    if (orders == ORDER_MOVE && path != nullptr) {
        if (state == ORDER_STATE_IN_PROGRESS || state == ORDER_STATE_IN_TRANSITION) {
            path->CancelMovement(this);

        } else {
            AILOG_LOG(log, "Not in move / turn state.");
        }

    } else {
        AILOG_LOG(log, "Not moving.");
    }
}

int32_t UnitInfo::GetLayingState() const { return laying_state; }

void UnitInfo::SetLayingState(int32_t state) { laying_state = state; }

void UnitInfo::ClearPins() { pin_count = 0; }

bool UnitInfo::AttemptSideStep(int32_t grid_x, int32_t grid_y, int32_t angle) {
    bool result;

    if (orders == ORDER_AWAIT || orders == ORDER_SENTRY || orders == ORDER_MOVE) {
        if (speed > 0 || UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
            if (this->grid_x != grid_x || this->grid_y != grid_y) {
                int32_t backup_grid_x = this->grid_x;
                int32_t backup_grid_y = this->grid_y;

                this->grid_x = grid_x;
                this->grid_y = grid_y;

                Hash_MapHash.Remove(this);

                this->grid_x = backup_grid_x;
                this->grid_y = backup_grid_y;

                BlockedOnPathRequest(true);

                result = true;

            } else if (orders == ORDER_MOVE &&
                       (state == ORDER_STATE_IN_PROGRESS || state == ORDER_STATE_IN_TRANSITION)) {
                result = true;

            } else {
                Point position;
                Point best_site;
                int32_t step_cost;
                int32_t best_cost{0};
                int32_t unit_angle{8};
                int32_t best_angle{0};
                bool best_is_water{false};

                // CLNTRANS needs to stay on water tiles to remain hidden (underwater/stealth mode)
                const bool is_clntrans{unit_type == CLNTRANS};

                for (int32_t direction = 0; direction < 8; ++direction) {
                    for (int32_t scan_range = 0; scan_range < 2; ++scan_range) {
                        position.x = this->grid_x + DIRECTION_OFFSETS[direction].x;
                        position.y = this->grid_y + DIRECTION_OFFSETS[direction].y;

                        unit_angle = (direction - angle + 8) & 0x03;

                        if (unit_angle > 2) {
                            unit_angle = 4 - unit_angle;
                        }

                        if (best_cost == 0 || unit_angle > best_angle) {
                            step_cost = Access_IsAccessible(unit_type, team, position.x, position.y,
                                                            AccessModifier_SameClassBlocks);

                            if (direction & 0x01) {
                                step_cost = (step_cost * 3) / 2;
                            }

                            if (step_cost) {
                                if ((step_cost <= speed * 4 + move_fraction) ||
                                    UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_COMPUTER) {
                                    // For CLNTRANS, check if candidate position keeps unit underwater (on water)
                                    bool candidate_is_water{false};

                                    if (is_clntrans) {
                                        const uint8_t candidate_surface_type{
                                            Access_GetModifiedSurfaceType(position.x, position.y)};
                                        candidate_is_water = (candidate_surface_type == SURFACE_TYPE_WATER);
                                    }

                                    // Determine if this candidate is better than the current best
                                    bool is_better{false};

                                    if (is_clntrans) {
                                        // For CLNTRANS, prioritize staying underwater to remain hidden
                                        if (candidate_is_water && !best_is_water) {
                                            // Water tile beats non-water tile (stay hidden)
                                            is_better = true;

                                        } else if (candidate_is_water == best_is_water) {
                                            // Both water or both non-water: use original logic (turning angle, then
                                            // cost)
                                            if (unit_angle > best_angle) {
                                                is_better = true;

                                            } else if (unit_angle == best_angle && step_cost < best_cost) {
                                                is_better = true;
                                            }
                                        }
                                        // If candidate is non-water but best is water, don't replace

                                    } else {
                                        // For other stealth units (SUBMARNE, COMMANDO), use original logic
                                        is_better = (unit_angle > best_angle);
                                    }

                                    if (is_better) {
                                        best_angle = unit_angle;
                                        best_cost = step_cost;
                                        best_site = position;
                                        best_is_water = candidate_is_water;
                                    }
                                }
                            }
                        }
                    }
                }

                if (best_cost) {
                    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
                        speed += (best_cost + 3) / 4;
                    }

                    Redraw();
                    RemoveInTransitUnitFromMapHash();

                    if (IsVisibleToTeam(GameManager_PlayerTeam)) {
                        if (GameManager_SelectedUnit == this || GameManager_DisplayButtonRange ||
                            GameManager_DisplayButtonScan) {
                            GameManager_UpdateDrawBounds();
                        }
                    }

                    SmartPointer<GroundPath> ground_path(new (std::nothrow) GroundPath(best_site.x, best_site.y));

                    ground_path->AddStep(best_site.x - this->grid_x, best_site.y - this->grid_y);

                    path = &*ground_path;

                    orders = ORDER_MOVE;
                    state = ORDER_STATE_IN_PROGRESS;

                    result = true;

                } else {
                    result = false;
                }
            }

        } else {
            result = false;
        }

    } else {
        result = false;
    }

    return result;
}

int32_t UnitInfo::GetTurnsToBuild(ResourceID unit_type, int32_t build_speed_multiplier, int32_t* turns_to_build) {
    int32_t result;
    int32_t turns;
    int32_t local_storage;

    if (build_time > 0 && build_list.GetCount() > 0 && *build_list[0] == unit_type) {
        turns = build_time;

    } else {
        turns = UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type)->GetAttribute(ATTRIB_TURNS);
    }

    if (flags & MOBILE_LAND_UNIT) {
        local_storage = storage;

    } else {
        local_storage = INT16_MAX;
    }

    *turns_to_build = 0;
    result = 0;

    while (turns > 0) {
        int32_t build_speed_limit = BuildMenu_GetMaxPossibleBuildRate(this->unit_type, turns, local_storage);

        build_speed_multiplier = std::min(build_speed_multiplier, build_speed_limit);

        result += Cargo_GetRawConsumptionRate(this->unit_type, build_speed_multiplier);

        ++*turns_to_build;

        local_storage -= Cargo_GetRawConsumptionRate(this->unit_type, build_speed_multiplier);

        turns -= build_speed_multiplier;
    }

    return result;
}

void UnitInfo::SetBuildRate(int32_t value) { build_rate = value; }

int32_t UnitInfo::GetBuildRate() const { return build_rate; }

void UnitInfo::SetRepeatBuildState(bool value) { repeat_build = value; }

bool UnitInfo::GetRepeatBuildState() const { return repeat_build; }

void UnitInfo::SpawnNewUnit() {
    if (unit_type == BULLDOZR) {
        while (state == ORDER_STATE_IN_TRANSITION) {
            GameManager_ProcessTick(false);
        }

        const int32_t limit_x = GetParent()->unit_type == LRGRUBLE ? grid_x + 1 : grid_x;
        const int32_t limit_y = GetParent()->unit_type == LRGRUBLE ? grid_y + 1 : grid_y;

        for (int32_t pos_x{grid_x}; pos_x <= limit_x; ++pos_x) {
            for (int32_t pos_y{grid_y}; pos_y <= limit_y; ++pos_y) {
                const auto units = Hash_MapHash[Point(pos_x, pos_y)];

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if ((*it).unit_type == SMLRUBLE || (*it).unit_type == LRGRUBLE) {
                            storage += (*it).storage;

                            UnitsManager_DestroyUnit(it->Get());
                        }
                    }
                }
            }
        }

        Access_DestroyUtilities(grid_x, grid_y, false, false, false, false);
        storage = std::min<int32_t>(storage, GetBaseValues()->GetAttribute(ATTRIB_STORAGE));

        DrawSpriteFrame(image_index - 8);

        orders = ORDER_AWAIT;
        state = ORDER_STATE_EXECUTING_ORDER;

        if (GetParent()->flags & BUILDING) {
            move_to_grid_x = grid_x;
            move_to_grid_y = grid_y;

            UnitsManager_SetNewOrderInt(this, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_DEINIT);
        }

        SetParent(nullptr);

        if (GameManager_SelectedUnit == this) {
            ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_IDLE);
        }

    } else {
        if (flags & STATIONARY) {
            SmartPointer<UnitInfo> factory_unit;
            int32_t position_x = this->grid_x;
            int32_t position_y = this->grid_y;

            factory_unit = UnitsManager_DeployUnit(GetConstructedUnitType(), team, GetComplex(), position_x, position_y,
                                                   0, false, true);

            factory_unit->SetParent(this);
            factory_unit->SetOrder(ORDER_IDLE);
            factory_unit->SetOrderState(ORDER_STATE_STORE);
            factory_unit->scaler_adjust = 4;

            ++UnitsManager_TeamInfo[team].stats_units_built;

            build_list.Remove(0);

            if (repeat_build) {
                build_list.PushBack(&factory_unit->unit_type);
            }

            SetParent(factory_unit.Get());

            storage = 1;
            orders = ORDER_BUILD;
            state = ORDER_STATE_UNIT_READY;

            DrawSpriteFrame(image_base);
            ScheduleDelayedTasks(true);

        } else {
            SmartPointer<UnitInfo> utility_unit;
            int32_t position_x = this->grid_x;
            int32_t position_y = this->grid_y;

            if (GameManager_SelectedUnit == this) {
                ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_IDLE);
            }

            utility_unit = Access_GetConstructionUtility(team, position_x, position_y);

            SDL_assert(utility_unit != nullptr);

            utility_unit = UnitsManager_DeployUnit(GetConstructedUnitType(), team, nullptr, utility_unit->grid_x,
                                                   utility_unit->grid_y, 0, false, true);

            if (!path) {
                build_list.Clear();
            }

            this->SetParent(&*utility_unit);
            utility_unit->SetParent(this);

            UnitsManager_SetNewOrderInt(&*utility_unit, ORDER_IDLE, ORDER_STATE_BUILDING_READY);

            if (unit_type == ENGINEER) {
                DrawSpriteFrame(image_index - 16);

            } else {
                DrawSpriteFrame(angle);
            }

            if (utility_unit->unit_type == ROAD || utility_unit->unit_type == BRIDGE ||
                utility_unit->unit_type == WTRPLTFM || utility_unit->unit_type == CNCT_4W) {
                Access_DestroyUtilities(position_x, position_y, false, false, false, false);

                SetParent(nullptr);
                utility_unit->SetOrder(ORDER_AWAIT);
                utility_unit->SetOrderState(ORDER_STATE_EXECUTING_ORDER);

                UnitsManager_UpdateConnectors(&*utility_unit);
                Access_UpdateMapStatus(&*utility_unit, true);

                GameManager_RenderMinimapDisplay = true;

                Hash_MapHash.Remove(this);
                Hash_MapHash.Add(this);

                if (path) {
                    state = ORDER_STATE_UNIT_READY;

                } else {
                    orders = ORDER_AWAIT;
                    state = ORDER_STATE_EXECUTING_ORDER;
                }

                SetSpriteFrameForTerrain(grid_x, grid_y);

                if (GetTask()) {
                    GetTask()->AddUnit(*utility_unit);
                }

            } else {
                orders = ORDER_BUILD;
                state = ORDER_STATE_UNIT_READY;

                if (GetTask()) {
                    utility_unit->AddTask(GetTask());
                }
            }
        }
    }
}

void UnitInfo::FollowUnit() {
    if (ResourceManager_GetSettings()->GetNumericValue("follow_unit") && GameManager_SelectedUnit == this) {
        if (GameManager_PlayMode == PLAY_MODE_TURN_BASED || GameManager_PlayerTeam == team ||
            UnitsManager_TeamInfo[GameManager_PlayerTeam].finished_turn) {
            if (visible_to_team[GameManager_PlayerTeam] && !GameManager_IsInsideMapView(this)) {
                GameManager_UpdateMainMapView(MAP_VIEW_CENTER, grid_x, grid_y);
            }
        }
    }
}

int32_t UnitInfo::GetExperience() {
    int32_t result = std::sqrt(experience * 10) - 2.0;

    if (result < 0) {
        result = 0;
    }

    return result;
}

void UnitInfo::BlockedOnPathRequest(bool mode, bool skip_notification) {
    path = nullptr;

    if (orders != ORDER_MOVE_TO_ATTACK || state == ORDER_STATE_NEW_ORDER) {
        orders = ORDER_AWAIT;
    }

    state = ORDER_STATE_EXECUTING_ORDER;

    if (!ResourceManager_GetSettings()->GetNumericValue("disable_fire")) {
        delayed_reaction = true;
    }

    MoveFinished(mode);

    if (Remote_IsNetworkGame) {
        if (!skip_notification) {
            Remote_SendNetPacket_41(this, mode);
        }
    }
}

void UnitInfo::MoveFinished(bool mode) {
    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
        GameManager_AutoSelectNext(this);
        ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_IDLE);
    }

    velocity = 0;
    moved = 0;

    ScheduleDelayedTasks(true);

    if (unit_type == INFANTRY) {
        UpdateSpriteFrame(0, image_index_max);

    } else if (unit_type == COMMANDO) {
        UpdateSpriteFrame(0, image_index_max);
        TestBustedVisibility();
    }

    if (IsVisibleToTeam(GameManager_PlayerTeam)) {
        if (GameManager_SelectedUnit == this || GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) {
            GameManager_UpdateDrawBounds();
        }

        GameManager_RenderMinimapDisplay = true;
    }

    RemoveInTransitUnitFromMapHash();
    Redraw();

    if (unit_type == MINELAYR || unit_type == SEAMNLYR) {
        Ai_CheckMines(this);
    }

    if (mode && !ResourceManager_GetSettings()->GetNumericValue("disable_fire")) {
        delayed_reaction = true;

        UnitsManager_AddToDelayedReactionList(this);

        if (ammo > 0) {
            Ai_EvaluateAttackTargets(this);
        }
    }
}

void UnitInfo::RadarPing() {
    if (orders != ORDER_BUILD && orders != ORDER_CLEAR &&
        (!(flags & STATIONARY) || unit_type == LANDMINE || unit_type == SEAMINE) && (flags & SELECTABLE)) {
        ResourceManager_GetSoundManager().PlaySfx(RADRPING);
        GameManager_NotifyEvent(this, 0);
    }
}

int32_t UnitInfo::GetNormalRateBuildCost() const {
    int32_t result;

    if (flags & STATIONARY) {
        result = Cargo_GetRawConsumptionRate(CONSTRCT, 1) * GetBaseValues()->GetAttribute(ATTRIB_TURNS);

    } else if (unit_type == COMMANDO || unit_type == INFANTRY) {
        result = Cargo_GetRawConsumptionRate(TRAINHAL, 1) * GetBaseValues()->GetAttribute(ATTRIB_TURNS);

    } else {
        result = Cargo_GetRawConsumptionRate(LANDPLT, 1) * GetBaseValues()->GetAttribute(ATTRIB_TURNS);
    }

    return result;
}

SmartObjectArray<ResourceID> UnitInfo::GetBuildList() { return build_list; }

void UnitInfo::RemoveTask(Task* task, bool mode) {
    SmartPointer<Task> unit_task(GetTask());

    AILOG(log, "Removing task from {} {}: {}", ResourceManager_GetUnit(unit_type).GetSingularName().data(), unit_id,
          task->WriteStatusLog());

    if (unit_task) {
        SmartPointer<Task> reference_task;

        tasks.Remove(*task);

        reference_task = GetTask();

        if (unit_task == reference_task) {
            AILOG_LOG(log, "No change in top task ({})", unit_task->WriteStatusLog());

        } else if (tasks.GetCount() > 0) {
            AILOG_LOG(log, "New topmost task: {}", reference_task->WriteStatusLog());

            if (mode && Task_IsReadyToTakeOrders(this)) {
                Task_RemindMoveFinished(this);
            }
        }

    } else {
        SDL_assert(tasks.GetCount() == 0);

        AILOG_LOG(log, "Unit has no tasks!");
    }
}

bool UnitInfo::IsReadyForOrders(Task* task) {
    bool result;

    if (Task_IsReadyToTakeOrders(this) && GetTask() == task) {
        result = true;

    } else {
        result = false;
    }

    return result;
}

void UnitInfo::RestoreOrders() {
    if (prior_orders == orders && unit_type != COMMTWR && unit_type != MININGST && unit_type != HABITAT &&
        unit_type != POWGEN && unit_type != POWERSTN && unit_type != RESEARCH) {
        prior_orders = ORDER_AWAIT;
        prior_state = ORDER_STATE_EXECUTING_ORDER;
    }

    orders = prior_orders;
    state = prior_state;
}

void UnitInfo::AddDelayedTask(Task* task) { delayed_tasks.PushBack(*task); }

void UnitInfo::RemoveDelayedTask(Task* task) { delayed_tasks.Remove(*task); }

bool UnitInfo::AreTherePins() { return pin_count > 0; }

void UnitInfo::DeployConstructionSiteUtilities(ResourceID unit_type) {
    int32_t unit_angle = 0;
    ResourceID unit_type1;
    ResourceID unit_type2;

    if (ResourceManager_GetUnit(unit_type).GetFlags() & BUILDING) {
        unit_type1 = LRGTAPE;
        unit_type2 = LRGCONES;

    } else {
        unit_type1 = SMLTAPE;
        unit_type2 = SMLCONES;
    }

    if (Access_GetModifiedSurfaceType(move_to_grid_x, move_to_grid_y) & (SURFACE_TYPE_WATER | SURFACE_TYPE_COAST)) {
        ++unit_angle;

    } else {
        UnitsManager_DeployUnit(unit_type2, team, nullptr, move_to_grid_x, move_to_grid_y, 0);
    }

    SmartPointer<UnitInfo> unit =
        UnitsManager_DeployUnit(unit_type1, team, nullptr, move_to_grid_x, move_to_grid_y, unit_angle);

    unit->SetParent(this);
}

bool UnitInfo::Land() {
    bool result;

    if (moved == 0 && GameManager_SelectedUnit == this) {
        ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_LAND);
    }

    RefreshScreen();

    moved = (moved + 1) & 0x3F;

    if (moved > 8) {
        flags &= ~HOVERING;

        Redraw();

        result = true;

    } else {
        shadow_offset.x += 8;
        shadow_offset.y += 8;

        UpdateUnitDrawZones();
        RefreshScreen();

        result = false;
    }

    return result;
}

void UnitInfo::ClearInTransitFlag() { in_transit = false; }

bool UnitInfo::Take() {
    bool result;

    if (moved == 0 && GameManager_SelectedUnit == this) {
        ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_TAKE);
    }

    RefreshScreen();

    moved = (moved + 1) & 0x3F;

    if (moved > 4) {
        flags |= HOVERING;

        result = true;

    } else {
        shadow_offset.x -= 16;
        shadow_offset.y -= 16;

        UpdateUnitDrawZones();
        RefreshScreen();

        result = false;
    }

    return result;
}

void UnitInfo::SpinningTurretAdvanceAnimation() {
    if (turret_image_index + 1 > image_index_max) {
        DrawSpriteTurretFrame(turret_image_base);

    } else {
        DrawSpriteTurretFrame(turret_image_index + 1);
    }
}

int32_t UnitInfo::GetAttackRange() {
    int32_t result;

    if (base_values->GetAttribute(ATTRIB_ROUNDS) > 0) {
        if (base_values->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
            result = base_values->GetAttribute(ATTRIB_RANGE) + base_values->GetAttribute(ATTRIB_SPEED) / 2;

        } else {
            result = ((base_values->GetAttribute(ATTRIB_ROUNDS) - 1) * base_values->GetAttribute(ATTRIB_SPEED)) /
                         base_values->GetAttribute(ATTRIB_ROUNDS) +
                     base_values->GetAttribute(ATTRIB_RANGE);
        }

    } else {
        result = 0;
    }

    return result;
}

void UnitInfo::UpdatePinsFromLists(int32_t grid_x, int32_t grid_y, SmartList<UnitInfo>* units, int32_t pin_units) {
    int32_t attack_radius = base_values->GetAttribute(ATTRIB_ATTACK_RADIUS);

    for (SmartList<UnitInfo>::Iterator it = units->Begin(); it != units->End(); ++it) {
        if (((*it).flags & SELECTABLE) && Access_IsWithinAttackRange(&*it, grid_x, grid_y, attack_radius) &&
            Access_IsValidAttackTarget(this, &*it, Point(grid_x, grid_y))) {
            (*it).pin_count += pin_units;
        }
    }
}

void UnitInfo::AttackAreaTargets(int32_t grid_x, int32_t grid_y) {
    int32_t attack_radius = base_values->GetAttribute(ATTRIB_ATTACK_RADIUS);
    ZoneWalker walker(Point(grid_x, grid_y), attack_radius);

    // Iterate through all grid positions within the attack radius
    do {
        Point* location = walker.GetCurrentLocation();
        SmartList<UnitInfo>* units_at_position = Hash_MapHash[*location];

        if (units_at_position) {
            // Cache the end iterator in case Hash_MapHash.Remove() deletes the list during iteration
            for (auto it = units_at_position->Begin(), end = units_at_position->End(); it != end; ++it) {
                if (((*it).flags & SELECTABLE) && Access_IsValidAttackTarget(this, &*it, Point(grid_x, grid_y))) {
                    if (((*it).unit_type != BRIDGE && (*it).unit_type != WTRPLTFM) ||
                        !Access_IsUnitBusyAtLocation(&*it)) {
                        (*it).AttackUnit(this,
                                         Access_GetApproximateDistance((*it).grid_x - grid_x, (*it).grid_y - grid_y),
                                         UnitsManager_GetTargetAngle((*it).grid_x - grid_x, (*it).grid_y - grid_y));
                    }
                }
            }
        }
    } while (walker.FindNext());
}

void UnitInfo::UpgradeInt() {
    SmartPointer<UnitValues> team_values(UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], unit_type));
    SmartPointer<UnitValues> best_values(new (std::nothrow) UnitValues(*team_values));
    SmartPointer<UnitInfo> copy = MakeCopy();

    // Calculate best possible values by merging unit's current values with team values
    for (char attr = ATTRIB_ATTACK; attr < ATTRIB_COUNT; ++attr) {
        const int32_t current_value = base_values->GetAttribute(attr);
        const int32_t team_value = team_values->GetAttribute(attr);

        if (attr == ATTRIB_TURNS) {
            best_values->SetAttribute(attr, std::max(1, std::min(current_value, team_value)));

        } else {
            best_values->SetAttribute(attr, std::max(current_value, team_value));
        }
    }

    if (*best_values == *team_values) {
        // use team values instance
        best_values = team_values;

    } else {
        // update unique unit version
        best_values->SetVersion(base_values->GetVersion() + 1);
    }

    best_values->MarkAsInUse();

    // Update current hits proportionally based on max hits change
    const int32_t new_hits =
        static_cast<int32_t>(hits) + best_values->GetAttribute(ATTRIB_HITS) - base_values->GetAttribute(ATTRIB_HITS);

    SDL_assert(new_hits >= 0);

    hits = std::min<int32_t>(new_hits, UINT16_MAX);

    CheckIfDestroyed();

    base_values = best_values;

    Access_UpdateMapStatus(this, true);
    Access_UpdateMapStatus(&*copy, false);
}

void UnitInfo::Regenerate() {
    if (!damaged_this_turn && (flags & STATIONARY) && base_values->GetAttribute(ATTRIB_HITS) != hits) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        if (materials.raw == 0) {
            return;
        }

        if (materials.raw > 1) {
            materials.raw = 1;
        }

        int32_t turns_to_repair = Repair(materials.raw);

        complex->Transfer(-turns_to_repair, 0, 0);
    }

    if ((flags & REGENERATING_UNIT) && base_values->GetAttribute(ATTRIB_HITS) != hits) {
        Repair(1);
    }

    damaged_this_turn = false;
}

void UnitInfo::StepMoveUnit(Point position) {
    ++position.y;

    for (int32_t range_limit = 3;; range_limit += 2) {
        --position.x;
        ++position.y;

        for (int32_t direction = 0; direction < 8; direction += 2) {
            for (int32_t i = 0; i < range_limit; ++i) {
                position += DIRECTION_OFFSETS[direction];
                if (position.x >= 0 && position.x < ResourceManager_MapSize.x && position.y >= 0 &&
                    position.y < ResourceManager_MapSize.y &&
                    Access_IsAccessible(unit_type, team, position.x, position.y,
                                        AccessModifier_IgnoreVisibility | AccessModifier_SameClassBlocks)) {
                    SmartPointer<UnitInfo> unit_copy = MakeCopy();

                    RemoveInTransitUnitFromMapHash();

                    Hash_MapHash.Remove(this);

                    RefreshScreen();

                    grid_x = position.x;
                    grid_y = position.y;

                    Hash_MapHash.Add(this);

                    Redraw();

                    Access_UpdateMapStatus(this, true);
                    Access_UpdateMapStatus(&*unit_copy, false);

                    if (orders == ORDER_MOVE || orders == ORDER_MOVE_TO_UNIT || orders == ORDER_MOVE_TO_ATTACK) {
                        BlockedOnPathRequest();
                    }

                    return;
                }
            }
        }
    }
}

void UnitInfo::PrepareConstructionSite(ResourceID unit_type) {
    uint32_t unit_flags = ResourceManager_GetUnit(unit_type).GetFlags();
    SmartPointer<UnitInfo> utility_unit(Access_GetConstructionUtility(team, grid_x, grid_y));
    Point position(utility_unit->grid_x, utility_unit->grid_y);

    if (unit_flags & BUILDING) {
        Point site;

        for (site.x = position.x; site.x < position.x + 2; ++site.x) {
            for (site.y = position.y; site.y < position.y + 2; ++site.y) {
                const auto units = Hash_MapHash[site];

                if (units) {
                    // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                    for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                        if ((*it).unit_type == COMMANDO || (*it).unit_type == SUBMARNE || (*it).unit_type == CLNTRANS) {
                            (*it).StepMoveUnit(position);
                        }
                    }
                }
            }
        }
    }

    if (unit_flags & REQUIRES_SLAB) {
        ResourceID slab_type = (unit_flags & BUILDING) ? LRGSLAB : SMLSLAB;

        SmartPointer<UnitInfo> unit =
            UnitsManager_DeployUnit(slab_type, team, nullptr, position.x, position.y,
                                    Randomizer_Generate(ResourceManager_GetUnit(slab_type).GetFrameInfo().image_count));

        unit->scaler_adjust = 4;

        UnitsManager_ScaleUnit(&*unit, ORDER_STATE_EXPAND);
    }
}

int32_t UnitInfo::GetTargetUnitAngle() {
    int32_t result;

    if (state == ORDER_STATE_TAPE_POSITIONING_ENTER) {
        SmartPointer<UnitInfo> utility_unit(Access_GetConstructionUtility(team, grid_x, grid_y));

        if (grid_x == utility_unit->grid_x) {
            if (grid_y == utility_unit->grid_y) {
                result = UNIT_ANGLE_SE;

            } else {
                result = UNIT_ANGLE_NE;
            }

        } else {
            if (grid_y == utility_unit->grid_y) {
                result = UNIT_ANGLE_SW;

            } else {
                result = UNIT_ANGLE_NW;
            }
        }

    } else {
        int32_t target_x = move_to_grid_x * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;
        int32_t target_y = move_to_grid_y * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2;

        if (target_x > x) {
            if (target_y > y) {
                result = UNIT_ANGLE_SE;

            } else {
                result = UNIT_ANGLE_NE;
            }

        } else {
            if (target_y > y) {
                result = UNIT_ANGLE_SW;

            } else {
                result = UNIT_ANGLE_NW;
            }
        }
    }

    return result;
}

void UnitInfo::UpdateInfoDisplay() {
    int32_t base_speed = GetBaseValues()->GetAttribute(ATTRIB_SPEED);
    int32_t base_rounds = GetBaseValues()->GetAttribute(ATTRIB_ROUNDS);
    int32_t unit_speed = speed;

    unit_speed -= (((shots + 1) * base_speed) / base_rounds) - ((shots * base_speed) / base_rounds);

    if (unit_speed < 0) {
        unit_speed = 0;
    }

    speed = unit_speed;

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
    }
}

int32_t UnitInfo::Repair(int32_t materials) {
    int32_t hits_damage_level = base_values->GetAttribute(ATTRIB_HITS) - hits;
    int32_t repair_cost = GetTurnsToRepair();

    if (repair_cost > materials) {
        hits_damage_level = (base_values->GetAttribute(ATTRIB_HITS) * 4 * materials) / GetNormalRateBuildCost();
        repair_cost = (base_values->GetAttribute(ATTRIB_HITS) * 4 + GetNormalRateBuildCost() * hits_damage_level - 1) /
                      (base_values->GetAttribute(ATTRIB_HITS) * 4);
    }

    int32_t new_hits = static_cast<int32_t>(hits) + hits_damage_level;
    hits = std::min<int32_t>(new_hits, UINT16_MAX);

    CheckIfDestroyed();

    return repair_cost;
}

void UnitInfo::CancelBuilding() {
    if (flags & STATIONARY) {
        if (orders == ORDER_BUILD) {
            if (state == ORDER_STATE_BUILD_CANCEL) {
                orders = ORDER_HALT_BUILDING;

            } else {
                orders = ORDER_HALT_BUILDING_2;
            }

            state = ORDER_STATE_EXECUTING_ORDER;

            Cargo_UpdateResourceLevels(this, 1);

        } else {
            orders = ORDER_AWAIT;
            state = ORDER_STATE_EXECUTING_ORDER;

            build_list.Clear();

            build_time = 0;
        }

        if (GameManager_SelectedUnit == this) {
            ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_POWER_CONSUMPTION_END);

            GameManager_UpdateInfoDisplay(this);
        }

        DrawSpriteFrame(image_base);

    } else {
        build_list.Clear();

        build_time = 0;

        Access_DestroyUtilities(grid_x, grid_y, true, false, false, false);

        path = nullptr;

        moved = 0;

        orders = ORDER_AWAIT;
        state = ORDER_STATE_EXECUTING_ORDER;

        if (unit_type == CONSTRCT || (unit_type == BULLDOZR && (GetParent()->flags & BUILDING))) {
            move_to_grid_x = grid_x;
            move_to_grid_y = grid_y;

            UnitsManager_SetNewOrderInt(this, ORDER_AWAIT_TAPE_POSITIONING, ORDER_STATE_TAPE_POSITIONING_DEINIT);

            DrawSpriteFrame(angle);

        } else if (unit_type == ENGINEER && image_index >= 16) {
            DrawSpriteFrame(image_index - 16);

        } else if (unit_type == BULLDOZR && image_index >= 8) {
            DrawSpriteFrame(image_index - 8);
        }

        if (GameManager_SelectedUnit == this) {
            ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_IDLE);
        }
    }
}

void UnitInfo::Reload(UnitInfo* parent) {
    bool need_action = false;

    if (complex) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        if (materials.raw > 0) {
            need_action = true;

            complex->Transfer(-1, 0, 0);
        }

    } else {
        if (storage > 0) {
            need_action = true;

            --storage;
        }
    }

    if (need_action) {
        if (GameManager_SelectedUnit == this) {
            ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_POWER_CONSUMPTION_START, true);
        }

        parent->ammo = parent->GetBaseValues()->GetAttribute(ATTRIB_AMMO);
    }

    RestoreOrders();

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
    }
}

bool UnitInfo::Upgrade(UnitInfo* parent) {
    bool result{false};

    if (parent->GetBaseValues() != UnitsManager_GetCurrentUnitValues(&UnitsManager_TeamInfo[team], parent->unit_type) &&
        complex) {
        Cargo materials;
        Cargo capacity;

        complex->GetCargoInfo(materials, capacity);

        int32_t materials_cost = parent->GetNormalRateBuildCost() / 4;

        if (materials.raw >= materials_cost) {
            parent->UpgradeInt();

            complex->Transfer(-materials_cost, 0, 0);

            if (GameManager_PlayerTeam == team && state != ORDER_STATE_EXECUTING_ORDER) {
                char unit_mark[10];
                SmartString message;

                GetVersion(unit_mark, parent->GetBaseValues()->GetVersion());

                message.Sprintf(80, _(d23e), ResourceManager_GetUnit(parent->unit_type).GetSingularName().data(),
                                unit_mark, materials_cost);

                MessageManager_DrawMessage(message.GetCStr(), 0, parent, Point(parent->grid_x, parent->grid_y));
            }

            result = true;

        } else if (GameManager_PlayerTeam == team && state != ORDER_STATE_EXECUTING_ORDER) {
            SmartString message;

            message.Sprintf(80, _(e3e0), materials_cost);

            MessageManager_DrawMessage(message.GetCStr(), 2, 0);
        }
    }

    RestoreOrders();

    SetParent(nullptr);

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
    }

    if (GameManager_SelectedUnit == parent) {
        if (GameManager_DisplayButtonRange || GameManager_DisplayButtonScan) {
            GameManager_UpdateDrawBounds();
        }

        GameManager_MenuUnitSelect(parent);
    }

    parent->ScheduleDelayedTasks(result);

    return result;
}

void UnitInfo::BusyWaitOrder() {
    bool last_state = tasks_enabled;

    tasks_enabled = false;

    while (orders != ORDER_AWAIT) {
        GameManager_ProcessTick(false);
        MouseEvent::ProcessInput();
    }

    tasks_enabled = last_state;
}

void UnitInfo::PositionInTape() {
    Ai_SetTasksPendingFlag("Positioning in tape");

    if (state == ORDER_STATE_TAPE_POSITIONING_INIT) {
        moved = 0;
        state = ORDER_STATE_TAPE_POSITIONING_ENTER;

    } else if (state == ORDER_STATE_TAPE_POSITIONING_DEINIT) {
        moved = 0;
        state = ORDER_STATE_TAPE_POSITIONING_LEAVE;
    }

    RefreshScreen();

    if (moved >= 4) {
        uint8_t old_state = state;

        RestoreOrders();

        moved = 0;

        if (unit_type == MASTER) {
            PrepareConstructionSite(MININGST);
            UnitsManager_ScaleUnit(this, ORDER_STATE_SHRINK);

        } else if (unit_type == CONSTRCT && old_state == ORDER_STATE_TAPE_POSITIONING_ENTER) {
            PrepareConstructionSite(GetConstructedUnitType());
            Paths_UpdateAngle(this, (angle + 1) & 0x07);
            DrawSpriteFrame(image_index + 16);

        } else if (old_state != ORDER_STATE_TAPE_POSITIONING_ENTER) {
            SmartPointer<UnitInfo> copy = MakeCopy();

            Hash_MapHash.Remove(this);

            grid_x = x / GFX_MAP_TILE_SIZE;
            grid_y = y / GFX_MAP_TILE_SIZE;

            Hash_MapHash.Add(this);

            Access_UpdateMapStatus(this, true);
            Access_UpdateMapStatus(&*copy, false);
        }

    } else {
        int32_t target_angle = GetTargetUnitAngle();

        if (moved || !Paths_UpdateAngle(this, target_angle)) {
            ++moved;

            OffsetDrawZones(DIRECTION_OFFSETS[target_angle].x * 8, DIRECTION_OFFSETS[target_angle].y * 8);

            RefreshScreen();
        }
    }
}

void UnitInfo::PlaceMine() {
    if ((unit_type == MINELAYR || unit_type == SEAMNLYR) && storage > 0) {
        ResourceID mine_type{(unit_type == SEAMNLYR) ? SEAMINE : LANDMINE};
        bool is_found{false};

        {
            const auto units = Hash_MapHash[Point(grid_x, grid_y)];

            if (units) {
                // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                    if ((*it).unit_type == mine_type) {
                        is_found = true;
                        break;
                    }
                }
            }
        }

        if (!is_found) {
            SmartPointer<UnitInfo> new_unit(UnitsManager_DeployUnit(mine_type, team, nullptr, grid_x, grid_y, 0));
            Rect bounds;
            Point site;

            bounds.ulx = std::max(0, grid_x - 1);
            bounds.uly = std::max(0, grid_y - 1);
            bounds.lrx = std::min(static_cast<int32_t>(ResourceManager_MapSize.x), grid_x + 2);
            bounds.lry = std::min(static_cast<int32_t>(ResourceManager_MapSize.y), grid_y + 2);

            for (site.x = bounds.ulx; site.x < bounds.lrx; ++site.x) {
                for (site.y = bounds.uly; site.y < bounds.lry; ++site.y) {
                    const auto units = Hash_MapHash[site];

                    if (units) {
                        // the end node must be cached in case Hash_MapHash.Remove() deletes the list
                        for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                            if ((*it).unit_type == SURVEYOR) {
                                new_unit->SpotByTeam((*it).team);
                            }
                        }
                    }
                }
            }

            --storage;

            if (!storage) {
                laying_state = 0;
            }

            if (GameManager_SelectedUnit == this) {
                GameManager_UpdateInfoDisplay(this);

                ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_POWER_CONSUMPTION_START, true);
            }
        }
    }
}

void UnitInfo::PickUpMine() {
    if ((unit_type == MINELAYR || unit_type == SEAMNLYR) && storage < GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
        SmartPointer<UnitInfo> mine;
        ResourceID mine_type{(unit_type == SEAMNLYR) ? SEAMINE : LANDMINE};
        const auto units = Hash_MapHash[Point(grid_x, grid_y)];

        if (units) {
            // the end node must be cached in case Hash_MapHash.Remove() deletes the list
            for (auto it = units->Begin(), end = units->End(); it != end; ++it) {
                if ((*it).unit_type == mine_type) {
                    mine = *it;
                    break;
                }
            }
        }

        if (mine) {
            UnitsManager_DestroyUnit(mine.Get());
            ++storage;

            if (storage == GetBaseValues()->GetAttribute(ATTRIB_STORAGE)) {
                laying_state = 0;

                if (GameManager_SelectedUnit == this) {
                    GameManager_UpdateInfoDisplay(this);

                    ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_POWER_CONSUMPTION_END, true);
                }
            }
        }
    }
}

bool UnitInfo::ShakeWater() {
    if (UnitsManager_ResetBobState) {
        bobbed = false;
    }

    int32_t distance = moved / GFX_MAP_TILE_SIZE;
    int32_t offset_x = 0;
    int32_t offset_y = 0;

    if (moved >= distance * GFX_MAP_TILE_SIZE + GFX_MAP_TILE_SIZE / 2) {
        if ((moved & 0x1F) == 0) {
            offset_x = -DIRECTION_OFFSETS[distance].x;
            offset_y = -DIRECTION_OFFSETS[distance].y;
        }

    } else {
        if ((moved & 0x1F) == 0) {
            offset_x = DIRECTION_OFFSETS[distance].x;
            offset_y = DIRECTION_OFFSETS[distance].y;
        }
    }

    if (offset_x || offset_y) {
        if (bobbed || !UnitsManager_BobEffectQuota) {
            return false;
        }

        RefreshScreen();
        OffsetDrawZones(offset_x, offset_y);

        shadow_offset.x -= offset_x;
        shadow_offset.y -= offset_y;

        UpdateUnitDrawZones();
        RefreshScreen();

        bobbed = true;
    }

    ++moved;

    return offset_x || offset_y;
}

bool UnitInfo::ShakeAir() {
    if (UnitsManager_ResetBobState) {
        bobbed = false;
    }

    int32_t distance = moved / 8;
    int32_t offset_x = 0;
    int32_t offset_y = 0;

    if (moved >= distance * 8 + 4) {
        if ((moved & 0x01) == 0) {
            offset_x = -DIRECTION_OFFSETS[distance].x;
            offset_y = -DIRECTION_OFFSETS[distance].y;
        }

    } else {
        if ((moved & 0x01) == 0) {
            offset_x = DIRECTION_OFFSETS[distance].x;
            offset_y = DIRECTION_OFFSETS[distance].y;
        }
    }

    if (offset_x || offset_y) {
        if (bobbed || !UnitsManager_BobEffectQuota) {
            return false;
        }

        RefreshScreen();
        OffsetDrawZones(offset_x, offset_y);

        shadow_offset.x -= offset_x * 2;
        shadow_offset.y -= offset_y * 2;

        UpdateUnitDrawZones();
        RefreshScreen();

        bobbed = true;
    }

    moved = (moved + 1) & 0x3F;

    return offset_x || offset_y;
}

void UnitInfo::ShakeSabotage() {
    ++shake_effect_state;

    shake_effect_state &= 0x0F;

    if (!(flags & BUILDING)) {
        int32_t offset_x = DIRECTION_OFFSETS[shake_effect_state / 2].x;
        int32_t offset_y = DIRECTION_OFFSETS[shake_effect_state / 2].y;

        RefreshScreen();

        if (shake_effect_state & 0x01) {
            OffsetDrawZones(offset_x, offset_y);

        } else {
            OffsetDrawZones(-offset_x, -offset_y);
        }

        RefreshScreen();
    }
}

void UnitInfo::PrepareFire() {
    int32_t unit_angle = angle;
    bool team_visibility = IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy;

    firing_recoil_frames = 3;

    if (unit_type == ANTIAIR) {
        firing_recoil_frames *= 5;

    } else if (unit_type == SP_FLAK || unit_type == FASTBOAT) {
        firing_recoil_frames *= 2;
    }

    if (team_visibility ||
        (UnitsManager_TeamInfo[GameManager_PlayerTeam].heat_map &&
         UnitsManager_TeamInfo[GameManager_PlayerTeam].heat_map->GetComplete(fire_on_grid_x, fire_on_grid_y))) {
        ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_FIRE);
    }

    if (flags & HAS_FIRING_SPRITE) {
        if (flags & TURRET_SPRITE) {
            DrawSpriteTurretFrame(turret_angle + firing_image_base);

        } else if (unit_type == COMMANDO || unit_type == INFANTRY) {
            if (unit_type == COMMANDO) {
                firing_recoil_frames = 8;

                DrawSpriteFrame(unit_angle + 104);

            } else {
                firing_recoil_frames = 8;

                DrawSpriteFrame(unit_angle + 104);
            }

        } else {
            DrawSpriteFrame(unit_angle + firing_image_base);
        }
    }

    if (flags & FIRES_MISSILES) {
        SmartPointer<UnitInfo> new_unit;
        ResourceID particle_unit;

        switch (unit_type) {
            case SUBMARNE:
            case CORVETTE: {
                particle_unit = TORPEDO;
            } break;

            case ALNTANK: {
                particle_unit = ALNTBALL;
            } break;

            case ALNASGUN:
            case JUGGRNT: {
                particle_unit = ALNABALL;
            } break;

            case ALNPLANE: {
                particle_unit = ALNMISSL;
            } break;

            default: {
                particle_unit = ROCKET;
            } break;
        }

        if (particle_unit == ALNTBALL || particle_unit == ALNABALL) {
            unit_angle = 0;

        } else {
            unit_angle = UnitsManager_GetFiringAngle(fire_on_grid_x - grid_x, fire_on_grid_y - grid_y);

            if (particle_unit == ALNMISSL) {
                unit_angle *= 2;
            }
        }

        new_unit = UnitsManager_DeployUnit(particle_unit, team, nullptr, grid_x, grid_y, unit_angle, true);

        new_unit->fire_on_grid_x = fire_on_grid_x;
        new_unit->fire_on_grid_y = fire_on_grid_y;

        new_unit->SetParent(this);

        new_unit->SetOrder(ORDER_MOVE);
        new_unit->SetOrderState(ORDER_STATE_INIT);

        UnitsManager_AddToDelayedReactionList(this);
    }

    --ammo;

    if (GameManager_RealTime) {
        if (shots > ammo) {
            shots = ammo;
        }

    } else {
        --shots;

        if (!GetBaseValues()->GetAttribute(ATTRIB_MOVE_AND_FIRE)) {
            UpdateInfoDisplay();
        }
    }

    state = ORDER_STATE_FIRE_IN_PROGRESS;
}

void UnitInfo::ProgressFire() {
    --firing_recoil_frames;

    if (firing_recoil_frames) {
        if (unit_type == ANTIAIR || unit_type == SP_FLAK || unit_type == FASTBOAT) {
            DrawSpriteTurretFrame(turret_image_index + ((firing_recoil_frames & 1) ? -8 : 8));

        } else if (unit_type == COMMANDO || unit_type == INFANTRY) {
            DrawSpriteFrame(image_index + 8);
        }

    } else {
        if (flags & HAS_FIRING_SPRITE) {
            if (flags & TURRET_SPRITE) {
                DrawSpriteTurretFrame(turret_image_base + turret_angle);

            } else {
                DrawSpriteFrame(image_base + angle);
            }

            if (unit_type == COMMANDO) {
                TestBustedVisibility();
            }
        }

        RestoreOrders();

        if (GameManager_SelectedUnit == this) {
            GameManager_UpdateInfoDisplay(this);
        }

        if (!(flags & FIRES_MISSILES)) {
            Attack(fire_on_grid_x, fire_on_grid_y);

            UnitsManager_AddToDelayedReactionList(this);
        }

        if (speed > 0) {
            ScheduleDelayedTasks(true);
        }
    }
}

void UnitInfo::ChangeTeam(uint16_t target_team) {
    uint16_t old_team = team;

    if (target_team != old_team) {
        ResourceManager_GetPathsManager().RemoveRequest(this);
    }

    UnitsManager_DelayedAttackTargets[team].Remove(*this);

    ClearUnitList();
    SetParent(nullptr);
    SetEnemy(nullptr);
    path = nullptr;
    RemoveTasks();
    Ai_RemoveUnit(this);
    Access_UpdateMapStatus(this, true);

    if (UnitsManager_TeamInfo[team].heat_map) {
        visible_to_team[team] = UnitsManager_TeamInfo[team].heat_map->GetComplete(grid_x, grid_y) > 0;

    } else {
        visible_to_team[team] = false;
    }

    flags &= ~UnitsManager_TeamInfo[team].team_units->hash_team_id;
    team = target_team;
    auto_survey = false;
    flags |= UnitsManager_TeamInfo[target_team].team_units->hash_team_id;
    color_cycling_lut = UnitsManager_TeamInfo[target_team].team_units->color_index_table;

    if (orders == ORDER_DISABLE) {
        RestoreOrders();

        disabled_turns_remaining = 0;
    }

    Access_UpdateMapStatus(this, true);

    visible_to_team[target_team] = true;

    Ai_UpdateTerrainDistanceField(this);
    Ai_UnitSpotted(this, old_team);

    RefreshScreen();

    GameManager_RenderMinimapDisplay = true;
}

[[nodiscard]] UnitInfo* UnitInfo::GetParent() const noexcept { return parent_unit.Get(); }

void UnitInfo::SetParent(UnitInfo* const parent) noexcept { parent_unit = parent; }

[[nodiscard]] Unit::SfxType UnitInfo::GetSfxType() const noexcept { return sound; }

Unit::SfxType UnitInfo::SetSfxType(Unit::SfxType sound) noexcept {
    auto previous_sound{this->sound};

    SDL_assert(sound < Unit::SFX_TYPE_LIMIT);

    this->sound = sound;

    return previous_sound;
}

[[nodiscard]] ResourceID UnitInfo::GetUnitType() const noexcept { return unit_type; }

void UnitInfo::SetUnitType(const ResourceID unit_type) noexcept {
    SDL_assert(unit_type < UNIT_END);

    this->unit_type = unit_type;
}

[[nodiscard]] UnitOrderType UnitInfo::GetOrder() const noexcept { return orders; }

UnitOrderType UnitInfo::SetOrder(const UnitOrderType order) noexcept {
    auto previous_order{this->orders};

    this->orders = order;

    return previous_order;
}

[[nodiscard]] UnitOrderType UnitInfo::GetPriorOrder() const noexcept { return prior_orders; }

UnitOrderType UnitInfo::SetPriorOrder(const UnitOrderType order) noexcept {
    auto previous_order{this->prior_orders};

    this->prior_orders = order;

    return previous_order;
}

UnitOrderStateType UnitInfo::GetOrderState() const noexcept { return state; }

UnitOrderStateType UnitInfo::SetOrderState(const UnitOrderStateType order_state) noexcept {
    auto previous_order_state{this->state};

    this->state = order_state;

    return previous_order_state;
}

UnitOrderStateType UnitInfo::GetPriorOrderState() const noexcept { return prior_state; }

UnitOrderStateType UnitInfo::SetPriorOrderState(const UnitOrderStateType order_state) noexcept {
    auto previous_order_state{this->prior_state};

    this->prior_state = order_state;

    return previous_order_state;
}

void UnitInfo::CheckIfDestroyed() {
    if (hits == 0) {
        engine = 0;
        weapon = 0;
    }
}

void UnitInfo::UpdateGridPosition(int32_t new_grid_x, int32_t new_grid_y) {
    grid_x = new_grid_x;
    grid_y = new_grid_y;

    int32_t pixel_x = new_grid_x * 64 + 32;
    int32_t pixel_y = new_grid_y * 64 + 32;

    if (flags & BUILDING) {
        pixel_x += 31;
        pixel_y += 31;
    }

    pixel_x -= x;
    pixel_y -= y;

    OffsetDrawZones(pixel_x, pixel_y);
}

void UnitInfo::DrawBustedCommando() { DrawSpriteFrame(angle + 200); }

void UnitInfo::TestBustedVisibility() {
    for (int32_t team_index = PLAYER_TEAM_RED; team_index < PLAYER_TEAM_MAX - 1; ++team_index) {
        if (team != team_index) {
            if (IsVisibleToTeam(team_index)) {
                DrawBustedCommando();
            }
        }
    }
}

void UnitInfo::SetSpriteFrameForTerrain(int32_t target_grid_x, int32_t target_grid_y) {
    if ((flags & (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) == (MOBILE_SEA_UNIT | MOBILE_LAND_UNIT)) {
        const auto surface_type = Access_GetModifiedSurfaceType(target_grid_x, target_grid_y);

        if (GetUnitType() == CLNTRANS) {
            if (surface_type == SURFACE_TYPE_WATER) {
                image_base = 0;

            } else {
                image_base = 8;
            }

        } else {
            if (surface_type == SURFACE_TYPE_WATER) {
                image_base = 8;

            } else {
                image_base = 0;
            }

            firing_image_base = image_base + 16;
        }

        DrawSpriteFrame(image_base + angle);

    } else if (GetUnitType() == COMMANDO) {
        image_base = 0;
        DrawSpriteFrame(image_base + angle);
    }
}

void UnitInfo::Animate() {
    if (ResourceManager_GetSettings()->GetNumericValue("effects")) {
        bool is_unit_moved = false;

        if (flags & HOVERING) {
            is_unit_moved = ShakeAir();
        }

        if (((flags & MOBILE_SEA_UNIT) || GetUnitType() == SEAMINE) &&
            Access_GetModifiedSurfaceType(grid_x, grid_y) == SURFACE_TYPE_WATER) {
            is_unit_moved = ShakeWater();
        }

        if (GameManager_SelectedUnit == this || is_unit_moved) {
            int32_t unit_size = (flags & BUILDING) ? 63 : 31;
            Rect bounds;

            bounds.ulx = x - unit_size;
            bounds.uly = y - unit_size;
            bounds.lrx = bounds.ulx + 1 + unit_size * 2;
            bounds.lry = bounds.uly + 1 + unit_size * 2;

            if (is_unit_moved) {
                bounds.ulx -= 3;
                bounds.uly -= 3;
                bounds.lrx += 3;
                bounds.lry += 3;

                if (GameManager_IsInsideMapView(this)) {
                    --UnitsManager_BobEffectQuota;
                }
            }

            GameManager_AddDrawBounds(&bounds);
        }
    }
}

void UnitInfo::PowerUp(int32_t factor) {
    Cargo_UpdateResourceLevels(this, factor);

    SetOrderState(ORDER_STATE_EXECUTING_ORDER);

    if (UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_COMPUTER) {
        GameManager_OptimizeProduction(team, GetComplex(), false, true);

        if (IsVisibleToTeam(GameManager_PlayerTeam)) {
            RefreshScreen();
        }

        if (GameManager_SelectedUnit == this) {
            GameManager_UpdateInfoDisplay(this);
        }

    } else {
        if (GameManager_SelectedUnit == this) {
            GameManager_UpdateInfoDisplay(this);

            if (GameManager_PlayerTeam == team) {
                GameManager_OptimizeProduction(team, GetComplex(), true, true);
            }

            GameManager_AutoSelectNext(this);
        }
    }
}

void UnitInfo::PowerDown() {
    Ai_SetTasksPendingFlag("Power Down");

    if (GameManager_SelectedUnit == this) {
        ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_POWER_CONSUMPTION_END);
    }

    PowerUp(1);

    DrawSpriteFrame(image_base);

    if (GetUnitType() == RESEARCH) {
        ResearchMenu_UpdateResearchProgress(team, research_topic, -1);
    }
}

void UnitInfo::ProcessLanding() {
    if (Land()) {
        if (GetParent()) {
            SetOrder(ORDER_IDLE);
            SetOrderState(ORDER_STATE_STORE);

            UnitsManager_ScaleUnit(this, ORDER_STATE_SHRINK);

        } else {
            UnitsManager_RemoveUnitFromUnitLists(this);
            AddToDrawList();

            SetOrder(ORDER_AWAIT);
            SetOrderState(ORDER_STATE_EXECUTING_ORDER);
        }

        if (IsVisibleToTeam(GameManager_PlayerTeam) || GameManager_MaxSpy) {
            RefreshScreen();
        }
    }
}

void UnitInfo::ProcessLoading() {
    if (Take()) {
        SmartPointer<UnitInfo> parent = GetParent();
        const Unit& base_unit = ResourceManager_GetUnit(parent->GetUnitType());

        SetOrder(ORDER_AWAIT);
        SetOrderState(ORDER_STATE_EXECUTING_ORDER);
        SetParent(nullptr);

        if (GetTask()) {
            GetTask()->EventUnitLoaded(*this, *parent);
        }

        if (GameManager_SelectedUnit == this) {
            char message[400];

            sprintf(message, _(c15c), base_unit.GetSingularName().data(), parent->unit_id);

            MessageManager_DrawMessage(message, 0, 0);
        }
    }
}

void UnitInfo::ProcessUnloading() {
    if (Take()) {
        SmartPointer<UnitInfo> parent = GetParent();
        const Unit& base_unit = ResourceManager_GetUnit(parent->GetUnitType());

        SetOrder(ORDER_AWAIT);
        SetOrderState(ORDER_STATE_EXECUTING_ORDER);
        SetParent(nullptr);

        if (GetTask()) {
            GetTask()->EventUnitUnloaded(*this, *parent);
        }

        if (GameManager_SelectedUnit == this) {
            char message[400];

            sprintf(message, _(60f3), base_unit.GetSingularName().data(), parent->unit_id);

            MessageManager_DrawMessage(message, 0, 0);
        }
    }
}

bool UnitInfo::AimAtTarget() {
    bool result;
    int32_t unit_angle;

    if (flags & TURRET_SPRITE) {
        unit_angle = turret_angle;

    } else {
        unit_angle = angle;
    }

    int32_t distance_x = fire_on_grid_x - grid_x;
    int32_t distance_y = fire_on_grid_y - grid_y;
    int32_t target_angle = UnitsManager_GetTargetAngle(distance_x, distance_y);

    if (unit_angle == target_angle) {
        SetOrderState(ORDER_STATE_READY_TO_FIRE);

        for (int32_t team_index = PLAYER_TEAM_RED; team_index < PLAYER_TEAM_MAX - 1; ++team_index) {
            if (team != team_index && UnitsManager_TeamInfo[team_index].team_type != TEAM_TYPE_NONE &&
                UnitsManager_TeamInfo[team_index].heat_map &&
                UnitsManager_TeamInfo[team_index].heat_map->GetComplete(grid_x, grid_y)) {
                SpotByTeam(team_index);
            }
        }

        result = true;

    } else {
        if (((target_angle - unit_angle + 8) & 0x07) > 4) {
            target_angle = -1;

        } else {
            target_angle = 1;
        }

        unit_angle = ((unit_angle + target_angle) & 0x07);

        if (flags & TURRET_SPRITE) {
            UpdateTurretAngle(unit_angle, true);

        } else {
            UpdateAngle(unit_angle);
        }

        result = false;
    }

    return result;
}

void UnitInfo::ProcessRepair() {
    SmartPointer<UnitInfo> parent(GetParent());

    if (GameManager_SelectedUnit == this) {
        ResourceManager_GetSoundManager().PlaySfx(this, Unit::SFX_TYPE_POWER_CONSUMPTION_START, true);
    }

    if (GetComplex()) {
        Cargo materials;
        Cargo capacity;

        GetComplex()->GetCargoInfo(materials, capacity);

        int32_t repair_cost = parent->Repair(materials.raw);

        GetComplex()->Transfer(-repair_cost, 0, 0);

    } else {
        int32_t repair_cost = parent->Repair(storage);

        storage -= repair_cost;
    }

    if (parent->IsVisibleToTeam(GameManager_PlayerTeam)) {
        parent->RefreshScreen();
    }

    RestoreOrders();

    if (GameManager_SelectedUnit == this) {
        GameManager_UpdateInfoDisplay(this);
    }

    parent->ScheduleDelayedTasks(true);
}

void UnitInfo::ProcessTransfer() {
    SmartPointer<UnitInfo> source(this);
    SmartPointer<UnitInfo> target(GetParent());
    Unit::CargoType cargo_type = ResourceManager_GetUnit(GetUnitType()).GetCargoType();
    int32_t transfer_amount = transfer_cargo;

    if (transfer_amount < 0) {
        source = target;
        target = this;

        transfer_amount = labs(transfer_amount);
    }

    if (cargo_type == Unit::CargoType::CARGO_TYPE_RAW) {
        target->TransferRaw(transfer_amount);

        if (source->GetComplex()) {
            source->GetComplex()->material -= transfer_amount;
        }

        if (source->storage >= transfer_amount) {
            source->storage -= transfer_amount;

        } else {
            SDL_assert(source->GetComplex());

            transfer_amount -= source->storage;
            source->storage = 0;

            source->GetComplex()->Transfer(-transfer_amount, 0, 0);
            source->GetComplex()->material += transfer_amount;
        }

    } else if (cargo_type == Unit::CargoType::CARGO_TYPE_FUEL) {
        target->TransferFuel(transfer_amount);

        if (source->GetComplex()) {
            source->GetComplex()->fuel -= transfer_amount;
        }

        if (source->storage >= transfer_amount) {
            source->storage -= transfer_amount;

        } else {
            transfer_amount -= source->storage;
            source->storage = 0;

            source->GetComplex()->Transfer(0, -transfer_amount, 0);
            source->GetComplex()->fuel += transfer_amount;
        }

    } else {
        SDL_assert(cargo_type == Unit::CargoType::CARGO_TYPE_GOLD);

        target->TransferGold(transfer_amount);

        if (source->GetComplex()) {
            source->GetComplex()->gold -= transfer_amount;
        }

        if (source->storage >= transfer_amount) {
            source->storage -= transfer_amount;

        } else {
            transfer_amount -= source->storage;
            source->storage = 0;

            source->GetComplex()->Transfer(0, 0, -transfer_amount);
            source->GetComplex()->gold += transfer_amount;
        }
    }

    RestoreOrders();

    if (GameManager_SelectedUnit == source) {
        GameManager_UpdateInfoDisplay(&*source);
    }

    if (GameManager_SelectedUnit == target) {
        GameManager_UpdateInfoDisplay(&*target);
    }

    target = GetParent();

    if (target->GetTask()) {
        target->GetTask()->EventCargoTransfer(*target);
    }
}
