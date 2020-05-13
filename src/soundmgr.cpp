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

#include "soundmgr.hpp"

#include "resrcmgr.h"

extern "C" {

typedef void (*BackgroundProcess)(void);

typedef unsigned int HANDLE;

struct __attribute__((packed)) SoundFxMeta_s {
    void* ptr;
    signed char flag;
};

struct __attribute__((packed)) SoundMgr_s {
    HANDLE music_sample;
    HANDLE hSample_2;
    FILE* file_handle;
    unsigned short resource_id_1;
    unsigned short resource_id_2;
    unsigned short resource_id_3;
    unsigned int unknown_1;
    unsigned char unknown_2;
    unsigned char unknown_3;
    unsigned int unknown_4;
    HANDLE hDigiDriverHandle;
    unsigned char unknown_5;
    unsigned int unknown_6;
    unsigned char unknown_7[556];
    unsigned char buffer[20][145];
    unsigned int unknown_8;
    HANDLE hTimerEventHandle;
    SoundFxMeta_s* unknown_9;
    unsigned int unknown_10;
    unsigned int unknown_11;
    unsigned int unknown_12;
    unsigned int unknown_13;
    unsigned int unknown_14;
    unsigned int unknown_15;
    unsigned char unknown_16;
    unsigned int unknown_17;
    unsigned int unknown_18;
    unsigned char timer_system_initialized;
};

void digi_init(SoundMgrPtr sound_mgr);
void sub_DE49E(void);
void* sub_A68D0(void* buffer);
void sub_A6944(void* buffer);
void* sub_A697A(void* buffer, unsigned short resource_id);
void* sub_A690A(void* buffer, unsigned int flag);

static inline void add_bk_process(BackgroundProcess f) {
    __asm__ __volatile__("	call	add_bk_process_\n"
                         : /* out  */
                         : /* in   */ "a"(f)
                         : /* clob */);
}

static inline void* wrap_sub_A68D0(void* buffer) {
    void* result;

    __asm__ __volatile__("	call	sub_A68D0\n"
                         : /* out  */ "=a"(result) /* return value is EAX (%0) */
                         : /* in   */ "a"(buffer)
                         : /* clob */);

    return result;
}

static inline void wrap_sub_A6944(void* buffer) {
    __asm__ __volatile__("	call	sub_A6944\n"
                         : /* out  */
                         : /* in   */ "a"(buffer)
                         : /* clob */);
}

static inline void* wrap_sub_A697A(void* buffer, unsigned short resource_id) {
    void* result;

    __asm__ __volatile__("	call	sub_A697A\n"
                         : /* out  */ "=a"(result) /* return value is EAX (%0) */
                         : /* in   */ "a"(buffer), "d"(resource_id)
                         : /* clob */);

    return result;
}

static inline void* wrap_sub_A690A(void* buffer, unsigned int flag) {
    void* result;

    __asm__ __volatile__("	call	sub_A690A\n"
                         : /* out  */ "=a"(result) /* return value is EAX (%0) */
                         : /* in   */ "a"(buffer), "d"(flag)
                         : /* clob */);

    return result;
}

void digi_init(SoundMgrPtr sound_mgr) {
    unsigned char buffer[172];

    // sound_mgr->hDigiDriverHandle = 1;
    // _MVE_SOS_sndInit(handle);

    /* register sound manager background process */
    add_bk_process(&sub_DE49E);

    /* enable background process service */
    asm volatile("call enable_bk\n");

    /*  */
    sound_mgr->unknown_9 = new struct SoundFxMeta_s[FXS_END - FXS_STRT];

    wrap_sub_A68D0(&buffer);
    wrap_sub_A6944(&buffer);

    for (size_t i = 0; i < (FXS_END - FXS_STRT); i++) {
        sound_mgr->unknown_9[i].ptr = wrap_sub_A697A(&buffer, FXS_STRT + 1 + i);
        sound_mgr->unknown_9[i].flag = -1;
    }

    wrap_sub_A690A(&buffer, 0);
}

} /* extern "C" */
