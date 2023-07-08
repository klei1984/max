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

#include "mvelib32.h"

#include "gnw.h"

#if defined(__unix__)
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <memoryapi.h>
#include <windows.h>
#else
#error "Platform is not supported"
#endif /* defined(platform) */

enum {
    MVE_OPCODE_END_OF_STREAM,
    MVE_OPCODE_END_OF_RECORD,
    MVE_OPCODE_CREATE_TIMER,
    MVE_OPCODE_ALLOC_AUDIO_BUFFERS,
    MVE_OPCODE_SYNCH_AUDIO,
    MVE_OPCODE_ALLOC_VIDEO_BUFFERS,
    MVE_OPCODE_NFDECOMP,
    MVE_OPCODE_SHOW_VIDEO_FRAME,
    MVE_OPCODE_AUDIO_FRAME,
    MVE_OPCODE_SILENCE_FRAME,
    MVE_OPCODE_INIT_GFX_MODE,
    MVE_OPCODE_SYNTH_PALETTE,
    MVE_OPCODE_LOAD_PALETTE,
    MVE_OPCODE_LOAD_COMPRESSED_PALETTE,
    MVE_OPCODE_UNKNOWN_0E,
    MVE_OPCODE_SET_DECODING_MAP,
    MVE_OPCODE_UNKNOWN_10,
    MVE_OPCODE_DECOMP_VIDEO
};

struct MveControlBlock_s {
    unsigned short size;
    unsigned char opcode;
    unsigned char version;
};

typedef struct MveControlBlock_s MveControlBlock;

struct MveMemBlock_s {
    unsigned char *buffer;
    size_t length;
    int is_allocated;
};

typedef struct MveMemBlock_s MveMemBlock;

struct __attribute__((packed)) MveHeader_s {
    const char tag[20];
    unsigned short field_20;
    unsigned short field_22;
    unsigned short field_24;
    int next_header;
};

static_assert(sizeof(struct MveHeader_s) == 30, "The structure needs to be packed.");

typedef struct MveHeader_s MveHeader;

void mveliba_start(void);
void mveliba_end(void);

void MemFree(MveMemBlock *ptr);
static void MemInit(MveMemBlock *ptr, size_t length, void *address);
void *MemAlloc(MveMemBlock *ptr, size_t size);
static void syncRelease(void);
static int syncInit(unsigned int rate, unsigned short divider);
static int syncWait(void);
int ioReset(FILE *handle);
void *ioRead(size_t size);
unsigned char *ioNextRecord(void);
static void ioRelease(void);
static void sndReset(void);
static void nfRelease(void);
static void nfAdvance(void);

static FILE *mve_io_handle;
static int mve_io_next_hdr;

static MveMemBlock mve_io_mem_buf;

static MveMemBlock nf_mem_buf1;
static MveMemBlock nf_mem_buf2;
static unsigned char *nf_buf_cur;
static unsigned char *nf_buf_prv;

static mve_cb_Alloc mve_mem_alloc;
static mve_cb_Free mve_mem_free;
static mve_cb_Read mve_io_read;
static mve_cb_ShowFrame mve_sf_ShowFrame;
static mve_cb_SetPalette mve_pal_SetPalette;
static mve_cb_Ctl mve_rm_ctl;

static int mve_rm_hold;
static int mve_rm_active;

static unsigned char *mve_rm_p;
static int mve_rm_len;
static int mve_rm_FrameCount;
static int mve_rm_FrameDropCount;
static int mve_rm_track_bit;
static int mve_rm_dx;
static int mve_rm_dy;

int mve_opt_fastmode;
int mve_opt_hscale_step = 4;

unsigned int mve_snd_vol = 32767;

void MVE_memCallbacks(mve_cb_Alloc alloc, mve_cb_Free free) {
    mve_mem_alloc = alloc;
    mve_mem_free = free;
}

void MemFree(MveMemBlock *ptr) {
    if (ptr->is_allocated && mve_mem_free) {
        mve_mem_free(ptr->buffer);
        ptr->is_allocated = 0;
    }

    ptr->length = 0;
}

void MemInit(MveMemBlock *ptr, size_t length, void *buffer) {
    if (buffer) {
        SDL_assert(ptr);

        MemFree(ptr);

        ptr->is_allocated = 0;
        ptr->length = length;
        ptr->buffer = buffer;
    }
}

void *MemAlloc(MveMemBlock *ptr, size_t size) {
    void *buffer;

    SDL_assert(ptr);

    if (ptr->length < size) {
        if (mve_mem_alloc) {
            MemFree(ptr);

            buffer = mve_mem_alloc(size + 100);
            if (buffer) {
                MemInit(ptr, size + 100, buffer);
                ptr->is_allocated = 1;

            } else {
                buffer = NULL;
            }
        } else {
            buffer = NULL;
        }
    } else {
        buffer = ptr->buffer;
    }

    return buffer;
}

void syncRelease(void) {}

int syncInit(unsigned int rate, unsigned short divider) { return 0; }

int syncWait(void) {}

void MVE_logDumpStats(void) { SDL_Log("Logging support disabled.\n"); }

void MVE_ioCallbacks(mve_cb_Read read) { mve_io_read = read; }

int ioReset(FILE *handle) {
    MveHeader *header;
    int result;

    mve_io_handle = handle;
    header = ioRead(30);

    if (header && !strcmp(header->tag, "Interplay MVE File\x1A") && header->field_24 == (~header->field_22 + 0x1234) &&
        header->field_22 == 0x100 && header->field_20 == 0x1A) {
        mve_io_next_hdr = header->next_header;

        result = 1;
    } else {
        result = 0;
    }

    return result;
}

void *ioRead(size_t size) {
    void *buffer = MemAlloc(&mve_io_mem_buf, size);

    if (!buffer || !mve_io_read(mve_io_handle, buffer, size)) {
        buffer = NULL;
    }

    return buffer;
}

unsigned char *ioNextRecord(void) {
    unsigned char *buffer;

    buffer = ioRead(mve_io_next_hdr + sizeof(int));
    if (buffer) {
        mve_io_next_hdr = *(int *)&buffer[mve_io_next_hdr];
    }

    return buffer;
}

void ioRelease(void) { MemFree(&mve_io_mem_buf); }

void MVE_sndVolume(unsigned int volume) {
    if (volume > 32767) {
        volume = 32767;
    }

    mve_snd_vol = volume;
}

void sndReset(void) {}
int MVE_sndConfigure() {}

void MVE_sndPause(void) {}
void MVE_sndResume(void) {}

void nfRelease(void) {
    MemFree(&nf_mem_buf1);
    MemFree(&nf_mem_buf2);
}

void nfAdvance(void) {
    unsigned char *ptr;

    ptr = nf_buf_prv;
    nf_buf_prv = nf_buf_cur;
    nf_buf_cur = ptr;
}

void MVE_sfSVGA(int width, int height, int bytes_per_scan_line, int write_window, void *write_win_ptr, int window_size,
                int window_granuality, void *window_function, int hicolor) {
    //    int line_width;            // ecx@1
    //    unsigned int v10;  // ebx@4
    //
    //    sf_ScreenWidth = width;
    //    sf_ScreenHeight = height;
    //    sf_ResolutionWidth = width;
    //    sf_ResolutionHeight = height;
    //    line_width = bytes_per_scan_line;
    //    if (mve_opt_fastmode & 4) {
    //        line_width = 2 * bytes_per_scan_line;
    //    }
    //    sf_WriteWin = write_window;
    //    sf_WriteWinPtr = (int)write_win_ptr;
    //    sf_WinSize = window_size;
    //    sf_WriteWinLimit = (int)write_win_ptr + window_size;
    //    sf_WinGran = window_granuality;
    //    sf_SetBank = (int)window_function;
    //    if (window_granuality)
    //        v10 = window_size / (unsigned int)window_granuality;
    //    else
    //        v10 = 1;
    //    sf_auto = 0;
    //    sf_hicolor = hicolor;
    //    sf_LineWidth = line_width;
    //    sf_WinGranPerSize = v10;
}

void MVE_sfCallbacks(mve_cb_ShowFrame showframe) { mve_sf_ShowFrame = showframe; }

void MVE_palCallbacks(mve_cb_SetPalette setpalette) { mve_pal_SetPalette = setpalette; }

void MVE_rmCallbacks(mve_cb_Ctl ctl) { mve_rm_ctl = ctl; }

void MVE_rmFastMode(int fastmode) { mve_opt_fastmode = fastmode; }

void MVE_rmHScale(int hscale_step) { mve_opt_hscale_step = (hscale_step != 3) + 3; }

void MVE_rmFrameCounts(int *frame_count, int *drop_count) {
    *frame_count = mve_rm_FrameCount;
    *drop_count = mve_rm_FrameDropCount;
}

int MVE_rmUnprotect(void) {
    static int is_unprotected = 0;
    unsigned long flOldProtect;
    int result;

    if (!is_unprotected) {
#if defined(__unix__)
//        size_t page_size = sysconf(_SC_PAGESIZE);
//
//        uintptr_t page_start = (uintptr_t)mveliba_start & -page_size;
//        if (mprotect((void *)page_start, (uintptr_t)mveliba_end - page_start, PROT_READ | PROT_WRITE | PROT_EXEC)) {
//            SDL_Log("VirtualProtect failed: %i\n", errno);
//            return -1;
//        }
#elif defined(_WIN32)
//        if (!VirtualProtect(mveliba_start, mveliba_end - mveliba_start, PAGE_EXECUTE_READWRITE, &flOldProtect)) {
//            SDL_Log("VirtualProtect failed: %i\n", GetLastError());
//            return -1;
//        }
#else
#error "Platform is not supported"
#endif /* defined(platform) */

        is_unprotected = 1;
    }

    return 0;
}

int MVE_rmPrepMovie(FILE *handle, int dx, int dy, int track) {
    int result;

    mve_rm_dx = dx;
    mve_rm_dy = dy;
    mve_rm_track_bit = 1 << track;

    if (!(1 << track)) {
        mve_rm_track_bit = 1;
    }

    if (ioReset(handle)) {
        mve_rm_p = ioNextRecord();
        mve_rm_len = 0;

        if (mve_rm_p) {
            mve_rm_hold = 0;
            mve_rm_FrameCount = 0;
            mve_rm_FrameDropCount = 0;
            mve_rm_active = 1;

            result = 0;

        } else {
            MVE_rmEndMovie();
            result = -2;
        }
    } else {
        MVE_rmEndMovie();
        result = -8;
    }

    return result;
}

int MVE_rmStepMovie(void) {
    unsigned char *buffer;
    unsigned char *decoding_map;
    int buffer_length;

    buffer_length = mve_rm_len;
    buffer = mve_rm_p;

    if (!mve_rm_active) {
        return -10;
    }

    if (mve_rm_hold) {
        MVE_sndResume();
        mve_rm_hold = 0;
    }

    for (;;) {
        MveControlBlock cb;
        decoding_map = NULL;

        if (!buffer) {
            MVE_rmEndMovie();

            return -2;
        }

        cb = *(MveControlBlock *)&buffer[buffer_length];

        buffer = &buffer[buffer_length + sizeof(int)];
        buffer_length = cb.size;

        switch (cb.opcode) {
            case MVE_OPCODE_END_OF_STREAM: {
                return -1;
            } break;

            case MVE_OPCODE_END_OF_RECORD: {
                buffer = ioNextRecord();
                buffer_length = 0;
            } break;

            case MVE_OPCODE_CREATE_TIMER: {
                unsigned int rate = *(unsigned int *)buffer;
                unsigned short divider = *(unsigned short *)(buffer + sizeof(unsigned int));

                if (!syncInit(rate, divider)) {
                    MVE_rmEndMovie();

                    return -3;
                }
            } break;

            case MVE_OPCODE_ALLOC_AUDIO_BUFFERS: {
                if (!MVE_sndConfigure()) {
                    MVE_rmEndMovie();

                    return -4;
                }
            } break;

            case MVE_OPCODE_SYNCH_AUDIO: {
                // MVE_sndSync();
            } break;

            case MVE_OPCODE_ALLOC_VIDEO_BUFFERS: {
                //                if (!nfConfig()) {
                //                    MVE_rmEndMovie();
                //
                //                    return -5;
                //                }
            } break;
            case MVE_OPCODE_NFDECOMP: {
                if (0) {
                    nfAdvance();
                }

                // nfDecomp();
            } break;
            case MVE_OPCODE_SHOW_VIDEO_FRAME: {
                mve_rm_FrameCount++;
                // palette
                // show frame
            } break;
            case MVE_OPCODE_AUDIO_FRAME:
            case MVE_OPCODE_SILENCE_FRAME: {
                // sndAdd
            } break;
            case MVE_OPCODE_INIT_GFX_MODE: {
                //
            } break;
            case MVE_OPCODE_SYNTH_PALETTE: {
                // palMakeSynthPalette
                // palMakePal15
            } break;
            case MVE_OPCODE_LOAD_PALETTE: {
                // palMakePal15
            } break;
            case MVE_OPCODE_LOAD_COMPRESSED_PALETTE: {
                // palLoadCompPalette
                // palMakePal15
            } break;
            case MVE_OPCODE_UNKNOWN_0E: {
            } break;
            case MVE_OPCODE_SET_DECODING_MAP: {
                decoding_map = buffer;
            } break;
            case MVE_OPCODE_UNKNOWN_10: {
            } break;
            case MVE_OPCODE_DECOMP_VIDEO: {
                // nfPkDecomp
            } break;
        }
    }
}

int MVE_rmHoldMovie(void) {
    if (!mve_rm_hold) {
        MVE_sndPause();
        mve_rm_hold = 1;
    }

    syncWait();

    return 0;
}

void MVE_rmEndMovie(void) {
    if (mve_rm_active) {
        syncWait();
        syncRelease();
        sndReset();
        mve_rm_active = 0;
    }
}

int MVE_RunMovie(FILE *handle, int dx, int dy, int track) {
    int result;
    int aborted = 0;

    result = MVE_rmPrepMovie(handle, dx, dy, track);

    for (; (result = MVE_rmStepMovie()) == 0 && !aborted;) {
        if (mve_rm_ctl()) {
            aborted = 1;
            result = 1;
        }

        SDL_Delay(33);
    }

    MVE_rmEndMovie();

    if (result == -1) {
        result = 0;
    }

    return result;
}

void MVE_ReleaseMem(void) {
    MVE_rmEndMovie();
    ioRelease();
    nfRelease();
}

const char *MVE_strerror(int error_code) {
    const char *str_error;

    switch (error_code) {
        case 2:
            str_error = "Movie aborted with special code";
            break;
        case 1:
            str_error = "Movie aborted";
            break;
        case 0:
        case -1:
            str_error = "Movie completed normally";
            break;
        case -2:
            str_error = "File I/O error or Unable to allocate I/O buffers";
            break;
        case -3:
            str_error = "Unable to create timer";
            break;
        case -4:
            str_error = "Unable to allocate sound buffers";
            break;
        case -5:
            str_error = "Unable to allocate video buffers";
            break;
        case -6:
            str_error = "Insufficient screen resolution for movie";
            break;
        case -7:
            str_error = "Unable to setup graphics mode used by movie";
            break;
        case -8:
            str_error = "Invalid movie file";
            break;
        case -9:
            str_error = "Incorrect screen color mode";
            break;
        case -10:
            str_error = "StepMovie() without PrepMovie()";
            break;
        default:
            str_error = "Unknown movie error code";
            break;
    }

    return str_error;
}
