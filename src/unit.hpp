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

#include "point.hpp"
#include "resource_manager.hpp"

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
    const uint32_t m_flags;
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

    uint8_t* m_sprite_data;
    uint8_t* m_shadow_data;
    const FrameInfo m_data_file;

public:
    Unit(uint32_t flags, ResourceID sprite, ResourceID shadow, ResourceID data, ResourceID flics_animation,
         ResourceID portrait, ResourceID icon, ResourceID armory_portrait, uint8_t land_type, CargoType cargo_type,
         Gender gender, uint32_t singular_name, uint32_t plural_name, uint32_t description,
         uint32_t tutorial_description);

    ~Unit();

    Unit(const Unit&) = delete;
    Unit& operator=(const Unit&) = delete;

    [[nodiscard]] uint32_t GetFlags() const;
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
    [[nodiscard]] uint32_t GetSingularName() const;
    [[nodiscard]] uint32_t GetPluralName() const;
    [[nodiscard]] uint32_t GetDescription() const;
    [[nodiscard]] uint32_t GetTutorialDescription() const;

    [[nodiscard]] uint8_t* GetSpriteData() const;
    [[nodiscard]] uint8_t* GetShadowData() const;
    [[nodiscard]] const FrameInfo& GetFrameInfo() const;

    void SetSpriteData(uint8_t* sprite_data);
    void SetShadowData(uint8_t* shadow_data);

private:
    void LoadDataFile();
};

#endif /* UNIT_HPP */
