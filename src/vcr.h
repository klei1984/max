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

#ifndef VCR_H
#define VCR_H

#include <assert.h>
#include <stdint.h>

#include "db.h"
#include "input.h"
#include "kb.h"

typedef enum VCREventType_e {
    nop = 0x0,
    init = 0x1,
    key = 0x2,
    mouse = 0x3,
    joystick = 0x4,
} VCREventType;

typedef struct VCRInitData_s {
    int32_t mouse_x;
    int32_t mouse_y;
    kb_layout_t keyboard_layout;
} VCRInitData;

typedef struct VCRKeyData_s {
    uint16_t scan_code;
} VCRKeyData;

typedef struct VCRMouseData_s {
    int32_t delta_x;
    int32_t delta_y;
    int32_t buttons;
} VCRMouseData;

typedef struct VCRJoystickData_s {
    int32_t joy_x;
    int32_t joy_y;
    int32_t buttons;
} VCRJoystickData;

typedef union VCREventData_u {
    VCRInitData init_data;
    VCRKeyData key_data;
    VCRMouseData mouse_data;
    VCRJoystickData joy_data;
} VCREventData;

typedef struct VCREventRecord_s {
    VCREventType type;
    TOCKS time;
    uint32_t counter;
    VCREventData data;
} VCREventRecord;

static_assert(sizeof(struct VCREventRecord_s) == 24, "The structure needs to be packed.");

typedef void (*VCRNotifyCallback)(int32_t);

extern int32_t vcr_buffer_index;
extern VCREventRecord* vcr_buffer;
extern TOCKS vcr_time;
extern uint32_t vcr_counter;
extern int32_t vcr_state;
extern int32_t vcr_terminate_flags;
extern int32_t vcr_terminated_condition;

int32_t vcr_record(char* record_file);
int32_t vcr_play(char* playback_file, int32_t terminate_flags, VCRNotifyCallback notify_callback);
int32_t vcr_stop(void);
int32_t vcr_status(void);
VCREventType vcr_update(void);
int32_t vcr_dump_buffer(void);
int32_t vcr_save_record(VCREventRecord* record, DB_FILE fp);
int32_t vcr_load_record(VCREventRecord* record, DB_FILE fp);

#endif /* VCR_H */
