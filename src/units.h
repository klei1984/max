/* Copyright (c) 2020 M.A.X. Port Team
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

#ifndef UNITS_H
#define UNITS_H

#include <assert.h>

struct __attribute__((packed)) UnitInfo_s {
    unsigned int flags;
    unsigned short data;
    unsigned int unknown_1;
    unsigned short animation;
    unsigned short portrait;
    unsigned short interface_icon;
    unsigned short stored_portrait;
    unsigned short unknown_2;
    unsigned char cargo_type;
    unsigned char land_type;
    unsigned char gender;
    char* singular_name;
    char* plural_name;
    char* description;
    char* alt_description;
    unsigned short sprite;
    unsigned short shadows;
};

typedef struct UnitInfo_s* UnitInfoPtr;

static_assert(sizeof(struct UnitInfo_s) == 43, "The structure needs to be packed.");

struct __attribute__((packed)) UnitInfo2_s {
    unsigned int flags;
    unsigned short data;
    void* resource_buffer;
    unsigned short animation;
    unsigned short portrait;
    unsigned short interface_icon;
    unsigned short stored_portrait;
    unsigned short unknown_2;
    unsigned char cargo_type;
    unsigned char land_type;
    unsigned char gender;
    char* singular_name;
    char* plural_name;
    char* description;
    char* alt_description;
    void* sprite_ptr;
    void* shadow_ptr;
    void* unknown_5;
};

static_assert(sizeof(struct UnitInfo2_s) == 51, "The structure needs to be packed.");

typedef struct UnitInfo2_s* UnitInfo2Ptr;

struct __attribute__((packed)) Attribs_s {
    unsigned short unknown1;
    void* function_pointer;
    unsigned short unknow2;
    unsigned short turns_to_build;
    unsigned short hit_points;
    unsigned short armor_rating;
    unsigned short attack_power;
    unsigned short movement_speed;
    unsigned short attack_range;
    unsigned short rounds_per_turn;
    unsigned char fire_speed;
    unsigned short scan_range;
    unsigned short storage_size;
    unsigned short ammunition;
    unsigned short area_attack_size;
    unsigned short unknow3;
    unsigned short unknow4;
    unsigned char unknow5;
};

static_assert(sizeof(struct Attribs_s) == 36, "The structure needs to be packed.");

typedef struct Attribs_s* AttribsPtr;

struct PlayerInfo_s {
    unsigned char unknown[583];
};

typedef struct PlayerInfo_s PlayerInfo;

#endif /* UNITS_H */
