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

#include "db.h"
#include "kb.h"
#include "input.h"

typedef enum VCREventType_e {
    nop = 0x0,
    init = 0x1,
    key = 0x2,
    mouse = 0x3,
    joystick = 0x4,
} VCREventType;

typedef struct VCRInitData_s {
    int mouse_x;
    int mouse_y;
    kb_layout_t keyboard_layout;
} VCRInitData;

typedef struct VCRKeyData_s {
    unsigned short scan_code;
} VCRKeyData;

typedef struct VCRMouseData_s {
    int delta_x;
    int delta_y;
    int buttons;
} VCRMouseData;

typedef struct VCRJoystickData_s {
    int joy_x;
    int joy_y;
    int buttons;
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
    unsigned long counter;
    VCREventData data;
} VCREventRecord;

static_assert(sizeof(struct VCREventRecord_s) == 24, "The structure needs to be packed.");

typedef void (*VCRNotifyCallback)(int);

extern int vcr_buffer_index;
extern VCREventRecord* vcr_buffer;
extern TOCKS vcr_time;
extern unsigned int vcr_counter;
extern int vcr_state;
extern int vcr_terminate_flags;
extern int vcr_terminated_condition;

int vcr_record(char* record_file);
int vcr_play(char* playback_file, int terminate_flags, VCRNotifyCallback notify_callback);
int vcr_stop(void);
int vcr_status(void);
VCREventType vcr_update(void);
int vcr_dump_buffer(void);
int vcr_save_record(VCREventRecord* record, DB_FILE* fp);
int vcr_load_record(VCREventRecord* record, DB_FILE* fp);

#endif /* VCR_H */
