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

#include "unit.hpp"

#include <SDL.h>

#include <cstring>

Unit::Unit(uint32_t flags, ResourceID sprite, ResourceID shadow, ResourceID data, ResourceID flics_animation,
           ResourceID portrait, ResourceID icon, ResourceID armory_portrait, uint8_t land_type, CargoType cargo_type,
           Gender gender, uint32_t singular_name, uint32_t plural_name, uint32_t description,
           uint32_t tutorial_description)
    : m_flags(flags),
      m_sprite(sprite),
      m_shadow(shadow),
      m_data(data),
      m_flics_animation(flics_animation),
      m_portrait(portrait),
      m_icon(icon),
      m_armory_portrait(armory_portrait),
      m_land_type(land_type),
      m_cargo_type(cargo_type),
      m_gender(gender),
      m_singular_name(singular_name),
      m_plural_name(plural_name),
      m_description(description),
      m_tutorial_description(tutorial_description),
      m_sprite_data(nullptr),
      m_shadow_data(nullptr),
      m_data_file([data, flags]() {
          FrameInfo file{};
          if (data != INVALID_ID) {
              uint8_t* data_buffer = ResourceManager_LoadResource(data);

              if (data_buffer) {
                  file.image_base = static_cast<int8_t>(data_buffer[0]);
                  file.image_count = static_cast<int8_t>(data_buffer[1]);
                  file.turret_image_base = static_cast<int8_t>(data_buffer[2]);
                  file.turret_image_count = static_cast<int8_t>(data_buffer[3]);
                  file.firing_image_base = static_cast<int8_t>(data_buffer[4]);
                  file.firing_image_count = static_cast<int8_t>(data_buffer[5]);
                  file.connector_image_base = static_cast<int8_t>(data_buffer[6]);
                  file.connector_image_count = static_cast<int8_t>(data_buffer[7]);

                  if (flags & (TURRET_SPRITE | SPINNING_TURRET)) {
                      for (int32_t i = 0; i < 8; ++i) {
                          file.angle_offsets[i].x = static_cast<int8_t>(data_buffer[8 + i * 2]);
                          file.angle_offsets[i].y = static_cast<int8_t>(data_buffer[8 + i * 2 + 1]);
                      }
                  }
              }
          }

          return file;
      }()) {}

Unit::~Unit() {}

uint32_t Unit::GetFlags() const { return m_flags; }

ResourceID Unit::GetSprite() const { return m_sprite; }

ResourceID Unit::GetShadow() const { return m_shadow; }

ResourceID Unit::GetData() const { return m_data; }

ResourceID Unit::GetFlicsAnimation() const { return m_flics_animation; }

ResourceID Unit::GetPortrait() const { return m_portrait; }

ResourceID Unit::GetIcon() const { return m_icon; }

ResourceID Unit::GetArmoryPortrait() const { return m_armory_portrait; }

uint8_t Unit::GetLandType() const { return m_land_type; }

Unit::CargoType Unit::GetCargoType() const { return m_cargo_type; }

Unit::Gender Unit::GetGender() const { return m_gender; }

uint32_t Unit::GetSingularName() const { return m_singular_name; }

uint32_t Unit::GetPluralName() const { return m_plural_name; }

uint32_t Unit::GetDescription() const { return m_description; }

uint32_t Unit::GetTutorialDescription() const { return m_tutorial_description; }

uint8_t* Unit::GetSpriteData() const { return m_sprite_data; }

uint8_t* Unit::GetShadowData() const { return m_shadow_data; }

const FrameInfo& Unit::GetFrameInfo() const { return m_data_file; }

void Unit::SetSpriteData(uint8_t* sprite_data) { m_sprite_data = sprite_data; }

void Unit::SetShadowData(uint8_t* shadow_data) { m_shadow_data = shadow_data; }
