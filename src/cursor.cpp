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

#include "cursor.hpp"

extern "C" {
#include "gnw.h"
}

#include "enums.hpp"
#include "resource_manager.hpp"
#include "units_manager.hpp"

#define CURSOR_CURSOR_COUNT 30

struct Cursor_Descriptor {
    int16_t width;
    int16_t height;
    int16_t ulx;
    int16_t uly;
    uint16_t frame_count;
    uint8_t* data;
};

static ResourceID Cursor_ResourceLut[CURSOR_CURSOR_COUNT] = {
    HIDDNPTR, HANDPTR,  ARROW_N,  ARROW_NE, ARROW_E,  ARROW_SE, ARROW_S,  ARROW_SW, ARROW_W,  ARROW_NW,
    MAP_PTR,  MINI_PTR, FRND_FIX, FRND_XFR, FRND_FUE, PTR_RLD,  FRND_LOD, FRND_PTR, ENMY_PTR, PTR_FTRG,
    UNIT_GO,  UNIT_NGO, WAY_PTR,  GROUPPTR, ACTVTPTR, MAP_PTR,  STEALPTR, DISBLPTR, PTR_PATH, PTR_HELP};

static Cursor_Descriptor Cursor_CursorDescriptorLut[CURSOR_CURSOR_COUNT];

static uint8_t Cursor_DefaultWindowCursorLut[] = {
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_MAP,      CURSOR_ARROW_N, CURSOR_ARROW_NE, CURSOR_ARROW_NE,
    CURSOR_ARROW_E, CURSOR_ARROW_SE, CURSOR_ARROW_SE, CURSOR_ARROW_S, CURSOR_ARROW_SW, CURSOR_ARROW_SW,
    CURSOR_ARROW_W, CURSOR_ARROW_NW, CURSOR_ARROW_NW, CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND};

static uint8_t Cursor_ActiveCursorIndex = CURSOR_HIDDEN;

static void Cursor_DrawAttackPowerCursorHelper(int32_t target_current_hits, int32_t attacker_damage,
                                               int32_t target_base_hits, uint8_t cursor_index) {
    Cursor_Descriptor* cursor = &Cursor_CursorDescriptorLut[cursor_index];
    int32_t lrx = cursor->ulx - 18;
    int32_t lry = cursor->uly + 15;
    uint8_t* data = &cursor->data[lrx + cursor->width * lry];

    if (target_base_hits) {
        if (attacker_damage > target_current_hits) {
            attacker_damage = target_current_hits;
        }

        target_current_hits -= attacker_damage;
        attacker_damage = (attacker_damage + target_current_hits) * 35 / target_base_hits;

        target_current_hits = target_current_hits * 35 / target_base_hits;
        attacker_damage -= target_current_hits;

        draw_box(data, cursor->width, 0, 0, 36, 4, COLOR_BLACK);

        if (target_current_hits) {
            buf_fill(&data[cursor->width + 1], target_current_hits, 3, cursor->width, 2);
        }

        if (attacker_damage) {
            buf_fill(&data[cursor->width + 1 + target_current_hits], attacker_damage, 3, cursor->width, 1);
        }

        if ((attacker_damage + target_current_hits) < 35) {
            buf_fill(&data[cursor->width + 1 + target_current_hits + attacker_damage],
                     35 - target_current_hits - attacker_damage, 3, cursor->width, cursor->data[0]);
        }
    } else {
        buf_fill(data, 37, 5, cursor->width, cursor->data[0]);
    }
}

void Cursor_Init() {
    for (int32_t cursor_index = 0; cursor_index < CURSOR_CURSOR_COUNT; ++cursor_index) {
        struct ImageSimpleHeader* const sprite =
            reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(Cursor_ResourceLut[cursor_index]));

        if (sprite) {
            Cursor_Descriptor* cursor = &Cursor_CursorDescriptorLut[cursor_index];

            cursor->data = &sprite->transparent_color;
            cursor->width = sprite->width;
            cursor->height = sprite->height;
            cursor->ulx = sprite->ulx;
            cursor->uly = sprite->uly;
            cursor->frame_count = 1;

            if (cursor->width < cursor->height) {
                cursor->frame_count = cursor->height / cursor->width;
                cursor->height = cursor->height / cursor->frame_count;
            }
        }
    }

    Cursor_ActiveCursorIndex = CURSOR_CURSOR_COUNT;
}

uint8_t Cursor_GetCursor() { return Cursor_ActiveCursorIndex; }

uint8_t Cursor_GetDefaultWindowCursor(uint8_t window_index) { return Cursor_DefaultWindowCursorLut[window_index]; }

void Cursor_SetCursor(uint8_t cursor_index) {
    if (cursor_index != Cursor_ActiveCursorIndex) {
        Cursor_Descriptor* cursor = &Cursor_CursorDescriptorLut[cursor_index];

        if (cursor->data) {
            if (cursor->frame_count <= 1) {
                mouse_set_shape(cursor->data, cursor->width, cursor->height, cursor->width, cursor->ulx, cursor->uly,
                                cursor->data[0]);
            } else {
                mouse_set_anim_frames(cursor->data, cursor->frame_count, 0, cursor->height, cursor->width, cursor->ulx,
                                      cursor->uly, cursor->data[0], 200);
            }
        }

        Cursor_ActiveCursorIndex = cursor_index;
    }
}

void Cursor_DrawAttackPowerCursor(UnitInfo* selected_unit, UnitInfo* target_unit, uint8_t cursor_index) {
    if (target_unit) {
        Cursor_DrawAttackPowerCursorHelper(target_unit->hits,
                                           UnitsManager_GetAttackDamage(selected_unit, target_unit, 0),
                                           target_unit->GetBaseValues()->GetAttribute(ATTRIB_HITS), cursor_index);
    } else {
        Cursor_DrawAttackPowerCursorHelper(0, 0, 0, cursor_index);
    }
}

void Cursor_DrawStealthActionChanceCursor(int32_t experience_level, uint8_t cursor_index) {
    Cursor_Descriptor* cursor = &Cursor_CursorDescriptorLut[cursor_index];
    int32_t lrx = cursor->ulx - 18;
    int32_t lry = cursor->uly + 15;
    uint8_t* data = &cursor->data[lrx + cursor->width * lry];
    int32_t chance = experience_level * 35 / 100;
    int32_t reminder = 35 - experience_level * 35 / 100;

    draw_box(data, cursor->width, 0, 0, 36, 4, COLOR_BLACK);

    if (chance) {
        buf_fill(&data[cursor->width + 1], chance, 3, cursor->width, 1);
    }

    if (reminder) {
        buf_fill(&data[cursor->width + 1 + chance], reminder, 3, cursor->width, data[0]);
    }
}
