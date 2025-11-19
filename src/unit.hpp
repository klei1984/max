/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef UNIT_HPP
#define UNIT_HPP

#include <cstdint>
#include <string_view>
#include <unordered_map>

#include "point.hpp"
#include "resourcetable.hpp"

struct FrameInfo {
    int16_t image_base;
    int16_t image_count;
    int16_t turret_image_base;
    int16_t turret_image_count;
    int16_t firing_image_base;
    int16_t firing_image_count;
    int16_t connector_image_base;
    int16_t connector_image_count;
    Point angle_offsets[8];
};

class Unit {
public:
    enum SfxType : uint8_t {
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

    struct SoundEffectInfo {
        ResourceID resource_id;
        uint32_t volume;
        bool is_default;
        bool persistent;
        bool looping;
    };

    enum Flags : uint32_t {
        GROUND_COVER = 0x1,
        EXPLODING = 0x2,
        ANIMATED = 0x4,
        CONNECTOR_UNIT = 0x8,
        BUILDING = 0x10,
        MISSILE_UNIT = 0x20,
        MOBILE_AIR_UNIT = 0x40,
        MOBILE_SEA_UNIT = 0x80,
        MOBILE_LAND_UNIT = 0x100,
        STATIONARY = 0x200,
        UPGRADABLE = 0x4000,
        HOVERING = 0x10000,
        HAS_FIRING_SPRITE = 0x20000,
        FIRES_MISSILES = 0x40000,
        CONSTRUCTOR_UNIT = 0x80000,
        ELECTRONIC_UNIT = 0x200000,
        SELECTABLE = 0x400000,
        STANDALONE = 0x800000,
        REQUIRES_SLAB = 0x1000000,
        TURRET_SPRITE = 0x2000000,
        SENTRY_UNIT = 0x4000000,
        SPINNING_TURRET = 0x8000000,
        REGENERATING_UNIT = 0x10000000
    };

    enum SurfaceType : uint8_t {
        SURFACE_TYPE_NONE = 0x0,
        SURFACE_TYPE_LAND = 0x1,
        SURFACE_TYPE_WATER = 0x2,
        SURFACE_TYPE_COAST = 0x4,
        SURFACE_TYPE_AIR = 0x8,
        SURFACE_TYPE_ALL = 0xF
    };

    enum CargoType : uint8_t {
        CARGO_TYPE_NONE,
        CARGO_TYPE_RAW,
        CARGO_TYPE_FUEL,
        CARGO_TYPE_GOLD,
        CARGO_TYPE_LAND,
        CARGO_TYPE_SEA,
        CARGO_TYPE_AIR,
    };

    enum Gender : uint8_t {
        GENDER_MASCULINE,
        GENDER_FEMININE,
        GENDER_NEUTER,
    };

private:
    const ResourceID m_sprite;
    const ResourceID m_shadow;
    const ResourceID m_data;
    const ResourceID m_flics_animation;
    const ResourceID m_portrait;
    const ResourceID m_icon;
    const ResourceID m_armory_portrait;
    const uint8_t m_land_type;
    const CargoType m_cargo_type;
    const Gender m_gender;
    const uint32_t m_singular_name;
    const uint32_t m_plural_name;
    const uint32_t m_description;
    const uint32_t m_tutorial_description;
    const FrameInfo m_frame_info;
    const std::unordered_map<SfxType, SoundEffectInfo> m_sound_effects;

    uint32_t m_flags;
    uint8_t* m_sprite_data;
    uint8_t* m_shadow_data;

public:
    Unit(uint32_t flags, ResourceID sprite, ResourceID shadow, ResourceID data, ResourceID flics_animation,
         ResourceID portrait, ResourceID icon, ResourceID armory_portrait, uint8_t land_type, CargoType cargo_type,
         Gender gender, uint32_t singular_name, uint32_t plural_name, uint32_t description,
         uint32_t tutorial_description, std::unordered_map<SfxType, SoundEffectInfo>&& sound_effects);

    ~Unit();

    Unit(const Unit&) = delete;
    Unit& operator=(const Unit&) = delete;

    [[nodiscard]] uint32_t GetFlags() const;
    void SetFlags(const uint32_t flags);

    [[nodiscard]] ResourceID GetSprite() const;
    [[nodiscard]] ResourceID GetShadow() const;
    [[nodiscard]] ResourceID GetData() const;
    [[nodiscard]] ResourceID GetFlicsAnimation() const;
    [[nodiscard]] ResourceID GetPortrait() const;
    [[nodiscard]] ResourceID GetIcon() const;
    [[nodiscard]] ResourceID GetArmoryPortrait() const;
    [[nodiscard]] uint8_t GetLandType() const;
    [[nodiscard]] CargoType GetCargoType() const;
    [[nodiscard]] Gender GetGender() const;
    [[nodiscard]] std::string_view GetSingularName() const;
    [[nodiscard]] std::string_view GetPluralName() const;
    [[nodiscard]] std::string_view GetDescription() const;
    [[nodiscard]] std::string_view GetTutorialDescription() const;

    [[nodiscard]] uint8_t* GetSpriteData() const;
    [[nodiscard]] uint8_t* GetShadowData() const;
    [[nodiscard]] const FrameInfo& GetFrameInfo() const;
    [[nodiscard]] const SoundEffectInfo& GetSoundEffect(SfxType sfx_type) const;

    void SetSpriteData(uint8_t* sprite_data);
    void SetShadowData(uint8_t* shadow_data);

private:
    void LoadDataFile();
};

#endif /* UNIT_HPP */
