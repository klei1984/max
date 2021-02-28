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

#include "vcr.h"

#include "game.h"

static int vcr_create_buffer(void);
static int vcr_destroy_buffer(void);
static int vcr_clear_buffer(void);
static int vcr_load_buffer(void);

static int vcr_buffer_end;
static DB_FILE vcr_file;
static int vcr_temp_terminate_flags;
static VCRNotifyCallback vcr_notify_callback;
static TOCKS vcr_start_time;
static VCREventRecord vcr_last_play_event;
static kb_layout_t vcr_old_layout = english;
static int vcr_registered_atexit;

int vcr_buffer_index;
VCREventRecord *vcr_buffer;
TOCKS vcr_time;
unsigned int vcr_counter;
int vcr_state = 2;
int vcr_terminate_flags;
int vcr_terminated_condition;

int vcr_record(char *record_file) {
    if (vcr_state != 2 || !record_file || vcr_create_buffer() != 1) {
        return 0;
    }

    vcr_file = db_fopen(record_file, "wb");
    if (!vcr_file) {
        vcr_destroy_buffer();
        return 0;
    }

    if (!vcr_registered_atexit) {
        vcr_registered_atexit = atexit((void (*)(void))vcr_stop);
    }

    vcr_buffer[vcr_buffer_index].type = init;
    vcr_buffer[vcr_buffer_index].time = 0;
    vcr_buffer[vcr_buffer_index].counter = 0;
    vcr_buffer[vcr_buffer_index].data.init_data.keyboard_layout = kb_get_layout();

    while (mouse_get_buttons()) {
        mouse_info();
    }

    mouse_get_position(&vcr_buffer[vcr_buffer_index].data.init_data.mouse_x,
                       &vcr_buffer[vcr_buffer_index].data.init_data.mouse_y);

    vcr_buffer_index++;
    vcr_counter = 1;
    vcr_start_time = get_time();
    kb_clear();
    vcr_state = 0;

    return 1;
}

int vcr_play(char *playback_file, int terminate_flags, VCRNotifyCallback notify_callback) {
    if ((vcr_state != 2) || (!playback_file) || (vcr_create_buffer() != 1)) {
        return 0;
    }

    vcr_file = db_fopen(playback_file, "rb");

    if (!vcr_file) {
        vcr_destroy_buffer();
        return 0;
    }

    if (vcr_load_buffer() != 1) {
        db_fclose(vcr_file);
        vcr_destroy_buffer();
        return 0;
    }

    while (mouse_get_buttons()) {
        mouse_info();
    }

    kb_clear();

    vcr_temp_terminate_flags = terminate_flags;
    vcr_terminated_condition = 1;
    vcr_terminate_flags = 0;
    vcr_notify_callback = notify_callback;
    vcr_counter = 0;
    vcr_time = 0;
    vcr_start_time = get_time();
    vcr_state = 1;
    vcr_last_play_event.time = 0;
    vcr_last_play_event.counter = 0;

    return 1;
}

int vcr_stop(void) {
    int temp;

    temp = vcr_state;
    vcr_state = 2;

    switch (temp) {
        case 0:
            vcr_dump_buffer();

            db_fclose(vcr_file);
            vcr_file = NULL;

            vcr_destroy_buffer();
            break;
        case 1:
            db_fclose(vcr_file);
            vcr_file = NULL;

            vcr_destroy_buffer();

            kb_set_layout(vcr_old_layout);

            if (vcr_notify_callback) {
                vcr_notify_callback(vcr_terminated_condition);
            }
            break;
        default:
            break;
    }

    kb_clear();

    return 1;
}

int vcr_status(void) { return vcr_state; }

VCREventType vcr_update(void) {
    VCREventType event_type;
    TOCKS wait_until;

    event_type = nop;

    if (vcr_state == 0) {
        vcr_counter++;

        vcr_time = elapsed_time(vcr_start_time);

        if (vcr_buffer_index == 4095) {
            vcr_dump_buffer();
        }
    } else if (vcr_state == 1) {
        if (vcr_buffer_index >= vcr_buffer_end && !vcr_load_buffer()) {
            vcr_stop();

            return nop;
        }

        if (vcr_last_play_event.counter < vcr_buffer[vcr_buffer_index].counter) {
            wait_until =
                vcr_last_play_event.time + (vcr_buffer[vcr_buffer_index].time - vcr_last_play_event.time) *
                                               (vcr_counter - vcr_last_play_event.counter) /
                                               (vcr_buffer[vcr_buffer_index].counter - vcr_last_play_event.counter);

            while (elapsed_time(vcr_start_time) < wait_until) {
                ;
            }
        }

        vcr_counter++;

        while (vcr_counter >= vcr_buffer[vcr_buffer_index].counter) {
            TOCKS time_passed = elapsed_time(vcr_start_time);

            vcr_time = time_passed;

            if ((time_passed > (vcr_buffer[vcr_buffer_index].time + 5)) ||
                (time_passed < (vcr_buffer[vcr_buffer_index].time - 5))) {
                vcr_start_time += vcr_time - vcr_buffer[vcr_buffer_index].time;
            }

            switch (vcr_buffer[vcr_buffer_index].type) {
                case init:
                    vcr_state = 2;
                    vcr_old_layout = kb_get_layout();
                    kb_set_layout(vcr_buffer[vcr_buffer_index].data.init_data.keyboard_layout);

                    while (mouse_get_buttons()) {
                        mouse_info();
                    }

                    vcr_state = 1;

                    mouse_hide();
                    mouse_set_position(vcr_buffer[vcr_buffer_index].data.init_data.mouse_x,
                                       vcr_buffer[vcr_buffer_index].data.init_data.mouse_y);
                    mouse_show();

                    kb_clear();

                    vcr_terminate_flags = vcr_temp_terminate_flags;
                    vcr_start_time = get_time();
                    vcr_counter = 0;
                    break;
                case key:
                    kb_simulate_key(vcr_buffer[vcr_buffer_index].data.key_data.scan_code);
                    break;
                case mouse:
                    event_type = mouse;
                    mouse_simulate_input(vcr_buffer[vcr_buffer_index].data.mouse_data.delta_x,
                                         vcr_buffer[vcr_buffer_index].data.mouse_data.delta_y,
                                         vcr_buffer[vcr_buffer_index].data.mouse_data.buttons);
                    break;
                default:
                    break;
            }

            memcpy(&vcr_last_play_event, &vcr_buffer[vcr_buffer_index], sizeof(vcr_last_play_event));

            vcr_buffer_index++;
        }
    }

    return event_type;
}

int vcr_create_buffer(void) {
    int result;

    if (vcr_buffer) {
        result = 0;
    } else {
        vcr_buffer = (VCREventRecord *)malloc(sizeof(VCREventRecord) * 4096);
        result = vcr_clear_buffer();
    }

    return result;
}

int vcr_destroy_buffer(void) {
    int result;

    result = vcr_clear_buffer();
    if (result) {
        free(vcr_buffer);
        vcr_buffer = NULL;
    }

    return result;
}

int vcr_clear_buffer(void) {
    int result;

    if (vcr_buffer) {
        vcr_buffer_index = 0;
        result = 1;
    } else {
        result = 0;
    }

    return result;
}

int vcr_dump_buffer(void) {
    int result;

    if (!vcr_file || !vcr_buffer) {
        result = 0;
    } else {
        int i;

        for (i = 0; i < vcr_buffer_index && vcr_save_record(&vcr_buffer[i], vcr_file); i++) {
            ;
        }

        if (i == vcr_buffer_index && vcr_clear_buffer()) {
            result = 1;
        } else {
            result = 0;
        }
    }

    return result;
}

int vcr_load_buffer(void) {
    int result;

    if (!vcr_file || vcr_clear_buffer() != 1) {
        result = 0;
    } else {
        for (vcr_buffer_end = 0; vcr_buffer_end < 4096 && vcr_load_record(&vcr_buffer[vcr_buffer_end], vcr_file);
             vcr_buffer_end++) {
            ;
        }

        if (vcr_buffer_end) {
            result = 1;
        } else {
            result = 0;
        }
    }

    return result;
}

int vcr_save_record(VCREventRecord *record, DB_FILE fp) {
    int result = 0;

    if ((db_fwriteLong(fp, record->type) != -1) && (db_fwriteLong(fp, record->time) != -1) &&
        (db_fwriteLong(fp, record->counter) != -1)) {
        switch (record->type) {
            case init:
                if ((db_fwriteLong(fp, record->data.init_data.mouse_x) != -1) &&
                    (db_fwriteLong(fp, record->data.init_data.mouse_y) != -1) &&
                    (db_fwriteLong(fp, record->data.init_data.keyboard_layout) != -1)) {
                    result = 1;
                }
                break;
            case key:
                if (db_fwriteShort(fp, record->data.key_data.scan_code) != -1) {
                    result = 1;
                }
                break;
            case mouse:
                if ((db_fwriteLong(fp, record->data.mouse_data.delta_x) != -1) &&
                    (db_fwriteLong(fp, record->data.mouse_data.delta_y) != -1) &&
                    (db_fwriteLong(fp, record->data.mouse_data.buttons) != -1)) {
                    result = 1;
                }
                break;
            default:
                result = 0;
                break;
        }
    }

    return result;
}

int vcr_load_record(VCREventRecord *record, DB_FILE fp) {
    int result = 0;

    if ((db_freadInt(fp, (int *)&record->type) != -1) && (db_freadInt(fp, (int *)&record->time) != -1) &&
        (db_freadInt(fp, (int *)&record->counter) != -1)) {
        switch (record->type) {
            case init:
                if ((db_freadInt(fp, &record->data.init_data.mouse_x) != -1) &&
                    (db_freadInt(fp, &record->data.init_data.mouse_y) != -1) &&
                    (db_freadInt(fp, (int *)&record->data.init_data.keyboard_layout) != -1)) {
                    result = 1;
                }
                break;
            case key:
                if (db_freadShort(fp, &record->data.key_data.scan_code) != -1) {
                    result = 1;
                }
                break;
            case mouse:
                if ((db_freadInt(fp, &record->data.mouse_data.delta_x) != -1) &&
                    (db_freadInt(fp, &record->data.mouse_data.delta_y) != -1) &&
                    (db_freadInt(fp, &record->data.mouse_data.buttons) != -1)) {
                    result = 1;
                }
                break;
            default:
                result = 0;
                break;
        }
    }

    return result;
}
