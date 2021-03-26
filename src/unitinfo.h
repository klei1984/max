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

#ifndef UNITINFO_H
#define UNITINFO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
    unsigned char type;
    unsigned short resource_id;
} SoundElement;

static_assert(sizeof(SoundElement) == 3, "The structure needs to be packed.");

typedef struct __attribute__((packed)) {
    unsigned char count;
    SoundElement item[];
} SoundTable;

enum {
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
};

typedef struct __attribute__((packed)) {
    unsigned short smartptr_ref_count;
    unsigned int *smartptr_dtor;
    unsigned short field_6;
    unsigned short unit_type;
    unsigned int field_10;
    SoundTable *sound_table;
    unsigned int flags;
    unsigned short x;
    unsigned short y;
    unsigned short grid_x;
    unsigned short grid_y;
    unsigned char field_30[4];
    unsigned int field_34;
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
    unsigned char field_79;
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
    unsigned char field_91;
    unsigned char auto_survey;
    unsigned char research_topic;
    unsigned char moved;
    unsigned char bobbed;
    unsigned char engine;
    unsigned char weapon;
    unsigned char comm;
    unsigned char fuel_distance;
    unsigned char move_fraction;
    void *path;
    unsigned short connectors;
    unsigned char field_107;
    void *base_values;
    void *complex;
    void *build_list;
    unsigned short build_rate;
    unsigned char repeat_build;
    unsigned char energized;
    unsigned short id;
    unsigned int field_126;
    void *parent_unit;
    unsigned int *field_134;
    unsigned char field_138[10];
    unsigned char field_148[10];
    unsigned char field_158;
    unsigned int *field_159;
    unsigned short field_163;
    unsigned char field_165;
    unsigned char laying_state;
    char *name;
    unsigned char visible_to_red;
    unsigned char visible_to_green;
    unsigned char visible_to_blue;
    unsigned char visible_to_gray;
    unsigned char visible_to_alien;
    unsigned char spotted_by_red;
    unsigned char spotted_by_green;
    unsigned char spotted_by_blue;
    unsigned char spotted_by_gray;
    unsigned char spotted_by_alien;
    unsigned int field_181;
    unsigned int field_185;
    unsigned int field_189;
    unsigned int field_193;
    unsigned int field_197;
    unsigned int field_201;
    unsigned int field_205;
    unsigned int field_209;
    unsigned short field_213;
    unsigned short field_215;
    unsigned short shadow_offset_x;
    unsigned short shadow_offset_y;
    unsigned int field_221;
} UnitInfo;

static_assert(sizeof(UnitInfo) == 225, "The structure needs to be packed.");

#ifdef __cplusplus
}
#endif

#endif /* UNITINFO_H */
