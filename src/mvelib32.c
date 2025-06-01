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
#include "miniaudio.h"

#define MVE_SND_FORMAT_MONO 1
#define MVE_SND_FORMAT_STEREO 2

enum {
    MVE_OPCODE_END_OF_STREAM,
    MVE_OPCODE_END_OF_RECORD,
    MVE_OPCODE_CREATE_TIMER,
    MVE_OPCODE_ALLOC_AUDIO_BUFFERS,
    MVE_OPCODE_SYNCH_AUDIO,
    MVE_OPCODE_ALLOC_VIDEO_BUFFERS,
    MVE_OPCODE_DECOMP_VIDEO0,
    MVE_OPCODE_SHOW_VIDEO_FRAME,
    MVE_OPCODE_AUDIO_FRAME,
    MVE_OPCODE_SILENCE_FRAME,
    MVE_OPCODE_INIT_GFX_MODE,
    MVE_OPCODE_SYNTH_PALETTE,
    MVE_OPCODE_LOAD_PALETTE,
    MVE_OPCODE_LOAD_COMPRESSED_PALETTE,
    MVE_OPCODE_SET_DECODING_MAP1,
    MVE_OPCODE_SET_DECODING_MAP2,
    MVE_OPCODE_DECOMP_VIDEO1,
    MVE_OPCODE_DECOMP_VIDEO2
};

struct MveControlBlock {
    uint16_t size;
    uint8_t opcode;
    uint8_t version;
};

struct MveMemBlock {
    uint8_t *buffer;
    size_t length;
    int32_t is_allocated;
};

struct __attribute__((packed)) MveHeader {
    const char tag[20];
    uint16_t field_20;
    uint16_t field_22;
    uint16_t field_24;
    struct MveControlBlock next_header;
};

static_assert(sizeof(struct MveHeader) == 30, "The structure needs to be packed.");

struct mve_data_source {
    ma_data_source_base base;
    ma_data_source_vtable vtable;
    ma_format format;
    ma_uint32 channels;
    ma_uint32 sample_rate;
    ma_uint64 cursor;
};

struct mve_sound {
    struct ma_sound sound;
    struct mve_data_source data_source;
    ma_pcm_rb rb;
};

int16_t snd_8to16[256] = {
    0,      1,      2,      3,      4,      5,      6,      7,      8,      9,      10,     11,     12,     13,
    14,     15,     16,     17,     18,     19,     20,     21,     22,     23,     24,     25,     26,     27,
    28,     29,     30,     31,     32,     33,     34,     35,     36,     37,     38,     39,     40,     41,
    42,     43,     47,     51,     56,     61,     66,     72,     79,     86,     94,     102,    112,    122,
    133,    145,    158,    173,    189,    206,    225,    245,    267,    292,    318,    348,    379,    414,
    452,    493,    538,    587,    640,    699,    763,    832,    908,    991,    1081,   1180,   1288,   1405,
    1534,   1673,   1826,   1993,   2175,   2373,   2590,   2826,   3084,   3365,   3672,   4008,   4373,   4772,
    5208,   5683,   6202,   6767,   7385,   8059,   8794,   9597,   10472,  11428,  12471,  13609,  14851,  16206,
    17685,  19298,  21060,  22981,  25078,  27367,  29864,  32589,  -29973, -26728, -23186, -19322, -15105, -10503,
    -5481,  -1,     1,      1,      5481,   10503,  15105,  19322,  23186,  26728,  29973,  -32589, -29864, -27367,
    -25078, -22981, -21060, -19298, -17685, -16206, -14851, -13609, -12471, -11428, -10472, -9597,  -8794,  -8059,
    -7385,  -6767,  -6202,  -5683,  -5208,  -4772,  -4373,  -4008,  -3672,  -3365,  -3084,  -2826,  -2590,  -2373,
    -2175,  -1993,  -1826,  -1673,  -1534,  -1405,  -1288,  -1180,  -1081,  -991,   -908,   -832,   -763,   -699,
    -640,   -587,   -538,   -493,   -452,   -414,   -379,   -348,   -318,   -292,   -267,   -245,   -225,   -206,
    -189,   -173,   -158,   -145,   -133,   -122,   -112,   -102,   -94,    -86,    -79,    -72,    -66,    -61,
    -56,    -51,    -47,    -43,    -42,    -41,    -40,    -39,    -38,    -37,    -36,    -35,    -34,    -33,
    -32,    -31,    -30,    -29,    -28,    -27,    -26,    -25,    -24,    -23,    -22,    -21,    -20,    -19,
    -18,    -17,    -16,    -15,    -14,    -13,    -12,    -11,    -10,    -9,     -8,     -7,     -6,     -5,
    -4,     -3,     -2,     -1};

static void MemFree(struct MveMemBlock *ptr);
static void MemInit(struct MveMemBlock *ptr, size_t length, void *address);
static void *MemAlloc(struct MveMemBlock *ptr, size_t size);
static void MVE_memIO(void *buffer, size_t length);
static void MVE_ShowFrame(uint8_t *buffer, int bufw, int bufh, int sx, int sy, int w, int h, int dstx, int dsty);
static void MVE_SetPalette(uint8_t *p, int32_t start, int32_t count);
static int32_t MVE_gfxMode(int32_t mode);
static void MVE_gfxSetDoubleBuffer(int32_t v1, int32_t v2, int32_t v3);

static void syncRelease(void);
static int32_t syncInit(uint32_t rate, uint16_t divider);
static int32_t syncWait(void);
static int32_t syncWaitLevel(int32_t level);
static void syncReset(int32_t level);
static void syncSync(void);

static int32_t ioReset(FILE *handle);
static void *ioRead(size_t size);
static uint8_t *ioNextRecord(void);
static void ioRelease(void);

static ma_result mveDataSourceRead(ma_data_source *data_source, void *frames_out, ma_uint64 frame_count,
                                   ma_uint64 *frames_read);
static ma_result mveDataSourceSeek(ma_data_source *data_source, ma_uint64 frame_index);
static ma_result mveDataSourceGetDataFormat(ma_data_source *data_source, ma_format *format, ma_uint32 *channels,
                                            ma_uint32 *sample_rate, ma_channel *channel_map, size_t channel_map_cap);
static ma_result mveDataSourceGetCursor(ma_data_source *data_source, ma_uint64 *cursor);
static ma_result mveDataSourceGetLength(ma_data_source *data_source, ma_uint64 *length);
static ma_result mveDataSourceSetLooping(ma_data_source *data_source, ma_bool32 is_looping);

static ma_result sndDataSourceInit(struct mve_data_source *data_source);
static void sndDataSourceUninit(struct mve_data_source *data_source);
static void sndMveSoundInit(struct mve_sound **sound, int32_t format, int32_t channels, int32_t sample_rate,
                            int32_t minimum_buffer_size);
static void sndMveSoundUninit(struct mve_sound **sound);

static void sndReset(void);
static int32_t sndConfigure(int32_t unused, int32_t minimum_buffer_size, int32_t channels, int32_t sample_rate,
                            int32_t format, int32_t compression);
static void sndSync(void);
static void sndDecompS16(uint8_t *dst, uint8_t *src, int32_t length);
static void sndDecompM16(uint8_t *dst, uint8_t *src, int32_t length);
static void sndAdd(uint8_t *buffer, int32_t length);
static void sndRelease(void);
static void sndPause(void);
static void sndResume(void);

static void nfBlockCopy(uint8_t *dst, uint8_t *src);
static int32_t nfConfig(int32_t width, int32_t height, int32_t stride, int32_t hicolor);
static void nfPkConfig(void);
static void nfDecomp(uint8_t *buffer, int32_t pos_x, int32_t pos_y, int32_t width, int32_t height);
static void nfPkDecomp(uint8_t *decoding_map, uint8_t *buffer, int32_t pos_x, int32_t pos_y, int32_t width,
                       int32_t height);
static void nfRelease(void);
static void nfAdvance(void);

static void palMakePal15(void);
static void palSetPalette(int32_t start, int32_t count);
static void palClrPalette(int32_t start, int32_t count);
static void palLoadPalette(uint8_t *p, int32_t start, int32_t count);

static void sfShowFrame(int32_t dst_x, int32_t dst_y, int32_t flags);

static FILE *io_handle;
static struct MveControlBlock io_next_hdr;

static struct MveMemBlock snd_mem_buf;
static struct MveMemBlock io_mem_buf;
static struct MveMemBlock nf_mem_buf1;
static struct MveMemBlock nf_mem_buf2;
static int32_t nf_conf_tbl[256];
static uint8_t *nf_buf_cur;
static uint8_t *nf_buf_prv;
static int32_t nf_new_row0;
static int32_t nf_hicolor;
static int32_t nf_width;
static int32_t nf_height;
static int32_t nf_new_h;
static int32_t nf_new_w;
static int32_t nf_new_x;
static int32_t nf_new_y;
static int32_t nf_new_line;
static int32_t nf_back_right;
static uint8_t nf_fqty;
static uint8_t nf_hqty;
static uint8_t nf_wqty;

static uint16_t pal15_tbl[PALETTE_SIZE];
static uint8_t pal_tbl[PALETTE_STRIDE * PALETTE_SIZE];
static uint8_t pal_palette[PALETTE_STRIDE * PALETTE_SIZE];

static int32_t sf_auto = 1;
static int32_t sf_auto_dbl;
static int32_t sf_auto_mode;
static void (*sf_SetBank)(void);
static int32_t sf_hicolor;
static int32_t sf_LineWidth;
static int32_t sf_WinGranPerSize;
static int32_t sf_WinSize;
static int32_t sf_WinGran;
static uint8_t *sf_WriteWinPtr;
static uint8_t *sf_WriteWinLimit;
static int32_t sf_WriteWin;
static int32_t sf_ScreenWidth;
static int32_t sf_ResolutionWidth;
static int32_t sf_ScreenHeight;
static int32_t sf_ResolutionHeight;

static mve_cb_Alloc mem_alloc;
static mve_cb_Free mem_free;
static mve_cb_Read io_read;
static mve_cb_ShowFrame sf_ShowFrame = &MVE_ShowFrame;
static mve_cb_SetPalette pal_SetPalette = &MVE_SetPalette;
static mve_cb_Ctl rm_ctl;

static int32_t rm_hold;
static int32_t rm_active;
static uint8_t *rm_p;
static int32_t rm_len;
static int32_t rm_FrameCount;
static int32_t rm_FrameDropCount;
static int32_t rm_track_bit;
static int32_t mve_rm_dx;
static int32_t mve_rm_dy;

static int32_t opt_hscale_adj;
static int32_t opt_fastmode;
static int32_t opt_hscale_step = 4;

static int32_t gfx_curpage;
static uint32_t gfx_page_y[2];

static int32_t sync_active;
static int32_t sync_wait_quanta;
static int32_t sync_late;
static int32_t sync_FrameDropped;
static int32_t sync_time;

static void *snd_hDriver;
static struct mve_sound *snd_SampleHandle;
static int32_t snd_paused;
static float snd_vol = 1.f;
static int32_t snd_bits16;
static int32_t snd_stereo;
static uint8_t *snd_buf;
static int32_t snd_comp16;
static int32_t snd_done;

void MVE_memCallbacks(mve_cb_Alloc alloc, mve_cb_Free free) {
    mem_alloc = alloc;
    mem_free = free;
}

void MemFree(struct MveMemBlock *ptr) {
    if (ptr->is_allocated && mem_free) {
        mem_free(ptr->buffer);
        ptr->is_allocated = 0;
    }

    ptr->length = 0;
}

void MemInit(struct MveMemBlock *ptr, size_t length, void *buffer) {
    if (buffer) {
        SDL_assert(ptr);

        MemFree(ptr);

        ptr->is_allocated = 0;
        ptr->length = length;
        ptr->buffer = buffer;
    }
}

void *MemAlloc(struct MveMemBlock *ptr, size_t size) {
    void *buffer;

    if (ptr->length < size) {
        if (mem_alloc) {
            MemFree(ptr);

            buffer = mem_alloc(size + 100);

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

void MVE_sndInit(void *handle) { snd_hDriver = handle; }

void syncRelease(void) { sync_active = 0; }

int32_t syncInit(uint32_t rate, uint16_t divider) {
    const int32_t wait_quanta = -((divider / 2) + rate * divider);

    if (!sync_active || sync_wait_quanta != wait_quanta) {
        syncWait();
        sync_wait_quanta = wait_quanta;
        syncReset(wait_quanta);
    }

    return 1;
}

int32_t syncWait(void) { return syncWaitLevel(0); }

int32_t syncWaitLevel(int32_t level) {
    int32_t result = 0;

    if (sync_active) {
        result = sync_time + level + 1000 * timer_get();

        if (result < 0) {
            timer_wait(-result / 1000);
        }

        sync_time += sync_wait_quanta;
    }

    return result;
}

void syncReset(int32_t level) {
    sync_active = 1;
    sync_time = -1000 * timer_get() + level;
}

void syncSync(void) {
    if (sync_active) {
        const int32_t time_to_wait = sync_time + 1000 * timer_get();

        if (time_to_wait < 0) {
            timer_wait(-time_to_wait / 1000);
        }
    }
}

void MVE_memIO(void *buffer, size_t length) { MemInit(&io_mem_buf, length, buffer); }

void MVE_logDumpStats(void) {}

void MVE_ioCallbacks(mve_cb_Read read) { io_read = read; }

int32_t ioReset(FILE *handle) {
    struct MveHeader *header;
    int32_t result;

    io_handle = handle;
    header = ioRead(sizeof(struct MveHeader));

    if (header && !strcmp(header->tag, "Interplay MVE File\x1A") && header->field_24 == (~header->field_22 + 0x1234) &&
        header->field_22 == 0x100 && header->field_20 == 0x1A) {
        io_next_hdr = header->next_header;

        result = 1;

    } else {
        result = 0;
    }

    return result;
}

void *ioRead(size_t size) {
    void *buffer = MemAlloc(&io_mem_buf, size);

    if (buffer) {
        buffer = io_read(io_handle, buffer, size) ? buffer : NULL;
    }

    return buffer;
}

uint8_t *ioNextRecord(void) {
    uint8_t *buffer = ioRead(io_next_hdr.size + sizeof(struct MveControlBlock));

    if (buffer) {
        io_next_hdr = *(struct MveControlBlock *)&buffer[io_next_hdr.size];
    }

    return buffer;
}

void ioRelease(void) { MemFree(&io_mem_buf); }

void MVE_sndVolume(uint32_t volume) {
    if (volume > 32767) {
        volume = 32767;
    }

    snd_vol = volume / 32767.f;
}

void sndReset(void) {
    if (snd_hDriver) {
        if (snd_SampleHandle) {
            sndMveSoundUninit(&snd_SampleHandle);
        }
    }
}

ma_result mveDataSourceRead(ma_data_source *data_source, void *frames_out, ma_uint64 frame_count,
                            ma_uint64 *frames_read) {
    if (frame_count == 0 || frames_read == NULL || data_source == NULL) {
        return MA_INVALID_ARGS;
    }

    *frames_read = 0;

    struct mve_data_source *ds = (struct mve_data_source *)data_source;

    if (!snd_paused) {
        ma_result result;

        while (*frames_read < frame_count) {
            void *mapped_buffer;
            ma_uint32 frames_to_read = frame_count - *frames_read;

            result = ma_pcm_rb_acquire_read(&snd_SampleHandle->rb, &frames_to_read, &mapped_buffer);
            if (result != MA_SUCCESS) {
                break;
            }

            if (frames_to_read == 0) {
                break;
            }

            ma_copy_pcm_frames(frames_out, mapped_buffer, frames_to_read, snd_SampleHandle->data_source.format,
                               snd_SampleHandle->data_source.channels);

            result = ma_pcm_rb_commit_read(&snd_SampleHandle->rb, frames_to_read);
            if (result != MA_SUCCESS) {
                break;
            }

            *frames_read += frames_to_read;
        }
    }

    if (*frames_read == 0) {
        snd_done = 1;
    }

    return MA_SUCCESS;
}

ma_result mveDataSourceSeek(ma_data_source *data_source, ma_uint64 frame_index) { return MA_NOT_IMPLEMENTED; }

ma_result mveDataSourceGetDataFormat(ma_data_source *data_source, ma_format *format, ma_uint32 *channels,
                                     ma_uint32 *sample_rate, ma_channel *channel_map, size_t channel_map_cap) {
    if (data_source == NULL) {
        return MA_INVALID_ARGS;
    }

    struct mve_data_source *ds = (struct mve_data_source *)data_source;

    if (format != NULL) {
        *format = ds->format;
    }

    if (channels != NULL) {
        *channels = ds->channels;
    }

    if (sample_rate != NULL) {
        *sample_rate = ds->sample_rate;
    }

    if (channel_map != NULL) {
        memset(channel_map, 0, sizeof(*channel_map) * channel_map_cap);
    }

    return MA_SUCCESS;
}

ma_result mveDataSourceGetCursor(ma_data_source *data_source, ma_uint64 *cursor) {
    if (data_source == NULL || cursor == NULL) {
        return MA_INVALID_ARGS;
    }

    *cursor = 0;
    return MA_NOT_IMPLEMENTED;
}

ma_result mveDataSourceGetLength(ma_data_source *data_source, ma_uint64 *length) {
    if (length == NULL) {
        return MA_INVALID_ARGS;
    }

    length = 0;
    return MA_NOT_IMPLEMENTED;
}

ma_result mveDataSourceSetLooping(ma_data_source *data_source, ma_bool32 is_looping) { return MA_NOT_IMPLEMENTED; }

ma_result sndDataSourceInit(struct mve_data_source *data_source) {
    ma_result result;
    ma_data_source_config baseConfig;

    data_source->vtable.onRead = &mveDataSourceRead;
    data_source->vtable.onSeek = &mveDataSourceSeek;
    data_source->vtable.onGetDataFormat = &mveDataSourceGetDataFormat;
    data_source->vtable.onGetCursor = &mveDataSourceGetCursor;
    data_source->vtable.onGetLength = &mveDataSourceGetLength;
    data_source->vtable.onSetLooping = &mveDataSourceSetLooping;
    data_source->vtable.flags = 0;
    data_source->cursor = 0;

    baseConfig = ma_data_source_config_init();
    baseConfig.vtable = &data_source->vtable;

    result = ma_data_source_init(&baseConfig, &data_source->base);
    if (result != MA_SUCCESS) {
        return result;
    }

    return MA_SUCCESS;
}

void sndDataSourceUninit(struct mve_data_source *data_source) { ma_data_source_uninit(&data_source->base); }

void sndMveSoundInit(struct mve_sound **sound, int32_t format, int32_t channels, int32_t sample_rate,
                     int32_t minimum_buffer_size) {
    (*sound) = malloc(sizeof(struct mve_sound));

    if ((*sound)) {
        ma_result result;
        ma_sound_config soundConfig;
        int32_t minimum_pcm_frame_count;

        (*sound)->data_source.format = (format == 16) ? ma_format_s16 : ma_format_u8;
        (*sound)->data_source.channels = channels;
        (*sound)->data_source.sample_rate = sample_rate;

        result = sndDataSourceInit(&(*sound)->data_source);
        if (result != MA_SUCCESS) {
            free((*sound));
            (*sound) = NULL;
        }

        soundConfig = ma_sound_config_init();
        soundConfig.pFilePath = NULL;
        soundConfig.pDataSource = &(*sound)->data_source;
        soundConfig.flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION;

        result = ma_sound_init_ex(snd_hDriver, &soundConfig, &(*sound)->sound);
        if (result != MA_SUCCESS) {
            sndDataSourceUninit(&(*sound)->data_source);
            free((*sound));
            (*sound) = NULL;
        }

        minimum_pcm_frame_count =
            (10 * minimum_buffer_size) / ((*sound)->data_source.format * (*sound)->data_source.channels);

        result = ma_pcm_rb_init((*sound)->data_source.format, (*sound)->data_source.channels, minimum_pcm_frame_count,
                                NULL, NULL, &(*sound)->rb);
        if (result != MA_SUCCESS) {
            ma_sound_uninit(&(*sound)->sound);
            sndDataSourceUninit(&(*sound)->data_source);
            free((*sound));
            (*sound) = NULL;
        }

        ma_pcm_rb_set_sample_rate(&(*sound)->rb, (*sound)->data_source.sample_rate);
    }
}

void sndMveSoundUninit(struct mve_sound **sound) {
    if (sound && *sound) {
        ma_sound_uninit(&(*sound)->sound);
        sndDataSourceUninit(&(*sound)->data_source);
        ma_pcm_rb_uninit(&(*sound)->rb);
        free((*sound));
        (*sound) = NULL;
    }
}

int32_t sndConfigure(int32_t unused, int32_t minimum_buffer_size, int32_t channels, int32_t sample_rate, int32_t format,
                     int32_t compression) {
    if (snd_hDriver) {
        syncSync();

        snd_bits16 = format;
        snd_stereo = channels;
        snd_comp16 = compression;

        sndMveSoundUninit(&snd_SampleHandle);
        sndMveSoundInit(&snd_SampleHandle, format, channels, sample_rate, minimum_buffer_size);

        if (snd_SampleHandle) {
            ma_sound_set_volume(&snd_SampleHandle->sound, snd_vol);

            snd_paused = 0;
            snd_done = 1;

            snd_buf = MemAlloc(&snd_mem_buf, minimum_buffer_size * 3 / 2);

            if (!snd_buf) {
                sndMveSoundUninit(&snd_SampleHandle);
                snd_hDriver = NULL;
            }

        } else {
            snd_hDriver = NULL;
        }
    }

    return 1;
}

void sndSync(void) {
    int32_t wait_time;
    int32_t need_reset;

    wait_time = syncWaitLevel(sync_wait_quanta / 4);

    if (wait_time > ((-sync_wait_quanta) / 2)) {
        if (!sync_FrameDropped) {
            sync_late = 1;

        } else {
            sync_late = 0;
        }

    } else {
        sync_late = 0;
    }

    sync_FrameDropped = 0;

    if (snd_hDriver != NULL && snd_SampleHandle != NULL && snd_done) {
        if (ma_sound_start(&snd_SampleHandle->sound) == MA_SUCCESS) {
            snd_done = 0;
        }
    }
}

void sndDecompM16(uint8_t *dst, uint8_t *src, int32_t length) {
    int32_t count;
    int16_t left;

    count = length / 2;

    if (count) {
        left = (src[1] << 8) | src[0];
        ((int16_t *)dst)[0] = left;
        src += sizeof(int16_t);
        dst += sizeof(int16_t);

        --count;

        while (count > 0) {
            left += snd_8to16[src[0]];
            ((int16_t *)dst)[0] = left;

            src += sizeof(int8_t);
            dst += sizeof(int16_t);

            --count;
        }
    }
}

void sndDecompS16(uint8_t *dst, uint8_t *src, int32_t length) {
    int32_t count;
    int16_t left;
    int16_t right;

    count = length / 2;

    if (count) {
        left = (src[1] << 8) | src[0];
        ((int16_t *)dst)[0] = left;
        src += sizeof(int16_t);
        dst += sizeof(int16_t);

        --count;

        if (count) {
            right = (src[1] << 8) | src[0];
            ((int16_t *)dst)[0] = right;
            src += sizeof(int16_t);
            dst += sizeof(int16_t);

            --count;

            while (count > 0) {
                left += snd_8to16[src[0]];
                ((int16_t *)dst)[0] = left;

                src += sizeof(int8_t);
                dst += sizeof(int16_t);

                --count;

                if (count == 0) {
                    break;
                }

                right += snd_8to16[src[0]];
                ((int16_t *)dst)[0] = right;

                src += sizeof(int8_t);
                dst += sizeof(int16_t);

                --count;
            }
        }
    }
}

void sndAdd(uint8_t *buffer, int32_t length) {
    if (snd_hDriver && snd_SampleHandle) {
        snd_paused = 0;

        if (buffer) {
            if (snd_comp16) {
                if (snd_stereo == MVE_SND_FORMAT_STEREO) {
                    sndDecompS16(snd_buf, buffer, length);

                } else {
                    sndDecompM16(snd_buf, buffer, length);
                }

            } else {
                memcpy(snd_buf, buffer, length);
            }

        } else {
            memset(snd_buf, snd_bits16 ? 0 : 128, length);
        }

        {
            ma_result result;
            ma_uint32 frames_written = 0;
            ma_uint32 frame_count =
                length / (snd_SampleHandle->data_source.format * snd_SampleHandle->data_source.channels);

            while (frames_written < frame_count) {
                void *mapped_buffer;
                ma_uint32 frames_to_write = frame_count - frames_written;

                result = ma_pcm_rb_acquire_write(&snd_SampleHandle->rb, &frames_to_write, &mapped_buffer);
                if (result != MA_SUCCESS) {
                    break;
                }

                if (frames_to_write == 0) {
                    break;
                }

                ma_copy_pcm_frames(
                    mapped_buffer,
                    ma_offset_pcm_frames_const_ptr(snd_buf, frames_written, snd_SampleHandle->data_source.format,
                                                   snd_SampleHandle->data_source.channels),
                    frames_to_write, snd_SampleHandle->data_source.format, snd_SampleHandle->data_source.channels);

                result = ma_pcm_rb_commit_write(&snd_SampleHandle->rb, frames_to_write);
                if (result != MA_SUCCESS) {
                    break;
                }

                frames_written += frames_to_write;
            }
        }
    }
}

void sndRelease(void) { MemFree(&snd_mem_buf); }

void sndPause(void) {
    if (snd_hDriver) {
        snd_paused = 1;
    }
}

void sndResume(void) {
    if (snd_hDriver) {
        snd_paused = 0;
    }
}

int32_t nfConfig(int32_t width, int32_t height, int32_t stride, int32_t hicolor) {
    int32_t result;

    nf_wqty = width;
    nf_width = 8 * width;
    nf_fqty = stride;
    nf_hqty = height;
    nf_height = 8 * stride * height;

    if (opt_fastmode) {
        nf_height = (8 * stride * height) >> 1;
    }

    nf_hicolor = hicolor;
    nf_new_line = stride * nf_width - 8;

    if (hicolor) {
        nf_new_line = 2 * (stride * nf_width - 8);
        nf_width *= 2;
    }

    nf_back_right = nf_width * 7 * stride;
    nf_new_row0 = nf_width * 8 * stride;

    nf_buf_cur = MemAlloc(&nf_mem_buf1, nf_height * nf_width);
    nf_buf_prv = MemAlloc(&nf_mem_buf2, nf_height * nf_width);

    if (nf_buf_cur && nf_buf_prv) {
        nfPkConfig();
        result = 1;

    } else {
        result = 0;
    }

    return result;
}

void nfPkConfig(void) {
    for (int32_t i = 0; i < 128; ++i) {
        nf_conf_tbl[i] = i * nf_width;
    }

    for (int32_t i = 128; i < 256; ++i) {
        nf_conf_tbl[i] = (i - 256) * nf_width;
    }
}

void nfDecomp(uint8_t *buffer, int32_t pos_x, int32_t pos_y, int32_t width, int32_t height) {
    SDL_assert(NULL);
    /// \todo
}

void nfRelease(void) {
    MemFree(&nf_mem_buf1);
    MemFree(&nf_mem_buf2);
}

void nfAdvance(void) {
    uint8_t *ptr;

    ptr = nf_buf_prv;
    nf_buf_prv = nf_buf_cur;
    nf_buf_cur = ptr;
}

void MVE_sfSVGA(int width, int height, int bytes_per_scan_line, int write_window, uint8_t *write_win_ptr,
                int window_size, int window_granuality, void *window_function, int hicolor) {
    int32_t line_width;
    uint32_t size;

    sf_ScreenWidth = width;
    sf_ScreenHeight = height;
    sf_ResolutionWidth = width;
    sf_ResolutionHeight = height;

    line_width = bytes_per_scan_line;

    if (opt_fastmode & 4) {
        line_width = 2 * bytes_per_scan_line;
    }

    sf_WriteWin = write_window;
    sf_WriteWinPtr = write_win_ptr;
    sf_WinSize = window_size;
    sf_WriteWinLimit = &write_win_ptr[window_size];
    sf_WinGran = window_granuality;
    sf_SetBank = window_function;

    if (window_granuality) {
        size = window_size / window_granuality;

    } else {
        size = 1;
    }

    sf_auto = 0;
    sf_hicolor = hicolor;
    sf_LineWidth = line_width;
    sf_WinGranPerSize = size;
}

void sfShowFrame(int32_t dst_x, int32_t dst_y, int32_t flags) {
    uint32_t h_offset;
    int32_t v9;

    h_offset = ((4 * nf_width / opt_hscale_step - 12) & (~0xF)) + 12;
    opt_hscale_adj = nf_width - (h_offset / 4) * opt_hscale_step;

    if (dst_x < 0) {
        if (nf_hicolor) {
            dst_x = sf_ScreenWidth - (h_offset / 2);

        } else {
            dst_x = sf_ScreenWidth - h_offset;
        }

        dst_x /= 2;
    }

    if (nf_hicolor) {
        dst_x *= 2;
    }

    if (dst_y < 0) {
        if ((opt_fastmode & 4)) {
            dst_y = sf_ScreenHeight - 2 * nf_height;

        } else {
            dst_y = sf_ScreenHeight - nf_height;
        }

        dst_y /= 2;
    }

    gfx_curpage ^= 0x01;
    dst_x &= ~0x03;
    dst_y = gfx_page_y[gfx_curpage] + dst_y;

    if ((opt_fastmode & 4)) {
        dst_y >>= 1;
    }

    if (flags) {
        SDL_assert(NULL);
        /// \todo

        // mve_ShowFrameField(nf_buf_cur, nf_width, nf_height, nf_new_x, nf_new_y, nf_new_w, nf_new_h, dst_x, dst_y,
        // flags);
    }

    if (opt_hscale_step == 4) {
        sf_ShowFrame(nf_buf_cur, nf_width, nf_height, nf_new_x, nf_new_y, nf_new_w, nf_new_h, dst_x, dst_y);

    } else {
        sf_ShowFrame(nf_buf_cur, nf_width, nf_height, 0, nf_new_y, h_offset, nf_new_h, dst_x, dst_y);
    }
}

void MVE_ShowFrame(uint8_t *buffer, int bufw, int bufh, int sx, int sy, int w, int h, int dstx, int dsty) {
    SDL_assert(NULL);
    /// \todo
}

void MVE_SetPalette(uint8_t *p, int32_t start, int32_t count) {
    SDL_assert(NULL);
    /// \todo
}

int32_t MVE_gfxMode(int32_t mode) {
    SDL_assert(NULL);
    /// \todo

    return 1;
}

void MVE_sfCallbacks(mve_cb_ShowFrame showframe) { sf_ShowFrame = showframe; }

void MVE_palCallbacks(mve_cb_SetPalette setpalette) { pal_SetPalette = setpalette; }

void palMakePal15(void) {
    if (sf_hicolor) {
        for (int32_t i = 0; i < PALETTE_SIZE; ++i) {
            pal15_tbl[i] = (pal_tbl[i * PALETTE_STRIDE + 2] / 2) | ((pal_tbl[i * PALETTE_STRIDE + 1] / 2) << 5) |
                           ((pal_tbl[i * PALETTE_STRIDE + 0] / 2) << 10);
        }
    }
}

void palSetPalette(int32_t start, int32_t count) {
    if (!sf_hicolor) {
        pal_SetPalette(pal_tbl, start, count);
    }
}

void palClrPalette(int32_t start, int32_t count) {
    if (!sf_hicolor) {
        pal_SetPalette(pal_palette, start, count);
    }
}

void palLoadPalette(uint8_t *p, int32_t start, int32_t count) {
    memcpy(&pal_tbl[PALETTE_STRIDE * start], p, PALETTE_STRIDE * count);
}

void MVE_gfxSetDoubleBuffer(int32_t v1, int32_t v2, int32_t v3) {
    gfx_curpage = v3 & 1;
    gfx_page_y[0] = v1;
    gfx_page_y[1] = v2;
}

void MVE_rmCallbacks(mve_cb_Ctl ctl) { rm_ctl = ctl; }

void MVE_rmFastMode(int32_t fastmode) { opt_fastmode = fastmode; }

void MVE_rmHScale(int32_t hscale_step) { opt_hscale_step = (hscale_step != 3) ? 4 : 3; }

void MVE_rmFrameCounts(int32_t *frame_count, int32_t *drop_count) {
    *frame_count = rm_FrameCount;
    *drop_count = rm_FrameDropCount;
}

int32_t MVE_rmPrepMovie(FILE *handle, int32_t dx, int32_t dy, int32_t track) {
    int32_t result;

    mve_rm_dx = dx;
    mve_rm_dy = dy;
    rm_track_bit = 1 << track;

    if ((1 << track) == 0) {
        rm_track_bit = 1;
    }

    if (ioReset(handle)) {
        rm_p = ioNextRecord();
        rm_len = 0;

        if (rm_p) {
            rm_hold = 0;
            rm_FrameCount = 0;
            rm_FrameDropCount = 0;
            rm_active = 1;

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

int32_t MVE_rmHoldMovie(void) {
    if (!rm_hold) {
        sndPause();
        rm_hold = 1;
    }

    syncWait();

    return 0;
}

int32_t MVE_rmStepMovie(void) {
    uint8_t *buffer;
    uint8_t *decoding_map1;
    uint8_t *decoding_map2;
    int32_t buffer_length;

    buffer_length = rm_len;
    buffer = rm_p;

    if (!rm_active) {
        return -10;
    }

    if (rm_hold) {
        sndResume();
        rm_hold = 0;
    }

    decoding_map1 = NULL;
    decoding_map2 = NULL;

    if (!buffer) {
        MVE_rmEndMovie();

        return -2;
    }

    for (;;) {
        struct MveControlBlock cb = *(struct MveControlBlock *)&buffer[buffer_length];

        buffer = &buffer[buffer_length + sizeof(struct MveControlBlock)];
        buffer_length = cb.size;

        switch (cb.opcode) {
            case MVE_OPCODE_END_OF_STREAM: {
                return -1;
            } break;

            case MVE_OPCODE_END_OF_RECORD: {
                buffer = ioNextRecord();
                buffer_length = 0;

                decoding_map1 = NULL;
                decoding_map2 = NULL;

                if (!buffer) {
                    MVE_rmEndMovie();

                    return -2;
                }
            } break;

            case MVE_OPCODE_CREATE_TIMER: {
                struct __attribute__((packed)) MveOpcodeCreateTimer {
                    uint32_t timer_rate;
                    uint16_t divider;
                } *const opcode = (struct MveOpcodeCreateTimer *)buffer;

                if (!syncInit(opcode->timer_rate, opcode->divider)) {
                    MVE_rmEndMovie();

                    return -3;
                }
            } break;

            case MVE_OPCODE_ALLOC_AUDIO_BUFFERS: {
                int32_t opcode_result;

                if (cb.version < 1) {
                    struct __attribute__((packed)) MveOpcodeAllocAudioBuffersV0 {
                        uint16_t unused;
                        uint16_t flags;
                        uint16_t sample_rate;
                        uint16_t minimum_buffer_size;
                    } *const opcode = (struct MveOpcodeAllocAudioBuffersV0 *)buffer;

                    opcode_result =
                        sndConfigure(opcode->unused, opcode->minimum_buffer_size, (opcode->flags & 1) ? 2 : 1,
                                     opcode->sample_rate, (opcode->flags & 2) ? 16 : 8, 0);

                } else {
                    struct __attribute__((packed)) MveOpcodeAllocAudioBuffersV1 {
                        uint16_t unused;
                        uint16_t flags;
                        uint16_t sample_rate;
                        uint32_t minimum_buffer_size;
                    } *const opcode = (struct MveOpcodeAllocAudioBuffersV1 *)buffer;

                    opcode_result =
                        sndConfigure(opcode->unused, opcode->minimum_buffer_size,
                                     (opcode->flags & 1) ? MVE_SND_FORMAT_STEREO : MVE_SND_FORMAT_MONO,
                                     opcode->sample_rate, (opcode->flags & 2) ? 16 : 8, (opcode->flags & 4) ? 1 : 0);
                }

                if (!opcode_result) {
                    MVE_rmEndMovie();

                    return -4;
                }
            } break;

            case MVE_OPCODE_SYNCH_AUDIO: {
                sndSync();
            } break;

            case MVE_OPCODE_ALLOC_VIDEO_BUFFERS: {
                struct __attribute__((packed)) MveOpcodeAllocVideoBuffers {
                    uint16_t width;
                    uint16_t height;
                    uint16_t fqty;
                    uint16_t hicolor;
                } *const opcode = (struct MveOpcodeAllocVideoBuffers *)buffer;

                if (nfConfig(opcode->width, opcode->height, (cb.version < 1) ? 0 : opcode->fqty,
                             (cb.version < 2) ? 0 : opcode->hicolor)) {
                    uint32_t width = ((4 * nf_width) / opt_hscale_step) & (~0x0F);
                    int32_t rm_dx = (mve_rm_dx < 0) ? 0 : mve_rm_dx;
                    int32_t rm_dy = (mve_rm_dy < 0) ? 0 : mve_rm_dy;

                    if (nf_hicolor) {
                        width /= 2;
                    }

                    if ((width + rm_dx > sf_ScreenWidth) || (nf_height + rm_dy > sf_ScreenHeight) ||
                        (nf_hicolor && !sf_hicolor)) {
                        MVE_rmEndMovie();

                        return -6;
                    }

                } else {
                    MVE_rmEndMovie();

                    return -5;
                }
            } break;

            case MVE_OPCODE_DECOMP_VIDEO0: {
                struct __attribute__((packed)) MveOpcodeNfDecomp {
                    uint16_t unused_0;
                    uint16_t unused_1;
                    uint16_t x;
                    uint16_t y;
                    uint16_t width;
                    uint16_t height;
                    uint16_t flags;
                    uint8_t data;
                } *const opcode = (struct MveOpcodeNfDecomp *)buffer;

                if (opcode->flags & 0x01) {
                    nfAdvance();
                }

                nfDecomp(&opcode->data, opcode->x, opcode->y, opcode->width, opcode->height);
            } break;

            case MVE_OPCODE_SHOW_VIDEO_FRAME: {
                struct __attribute__((packed)) MveOpcodeShowVideoFrame {
                    uint16_t start;
                    uint16_t count;
                    uint16_t flags;
                } *const opcode = (struct MveOpcodeShowVideoFrame *)buffer;

                ++rm_FrameCount;

                if (gfx_page_y[1] || opcode->count == 0 || decoding_map1) {
                    if (!gfx_page_y[1]) {
                        palSetPalette(opcode->start, opcode->count);
                    }

                } else {
                    palClrPalette(opcode->start, opcode->count);
                }

                if (decoding_map1) {
                    SDL_assert(NULL);
                    /// \todo
                    // sfShowFrameChg(mve_rm_dx, mve_rm_dy, decoding_map1);

                } else if (!sync_late || opcode->count) {
                    sfShowFrame(mve_rm_dx, mve_rm_dy, (cb.version < 1) ? 0 : opcode->flags);

                } else {
                    sync_FrameDropped = 1;
                    rm_FrameDropCount += 1;
                }

                if (gfx_page_y[1] || opcode->count == 0 || decoding_map1) {
                    if (gfx_page_y[1]) {
                        SDL_assert(NULL);
                        /// \todo
                    }

                } else {
                    palSetPalette(opcode->start, opcode->count);
                }

                rm_p = buffer;
                rm_len = buffer_length;

                return 0;

            } break;

            case MVE_OPCODE_AUDIO_FRAME:
            case MVE_OPCODE_SILENCE_FRAME: {
                struct __attribute__((packed)) MveOpcodeAudioFrame {
                    uint16_t id;
                    uint16_t mask;
                    uint16_t length;
                    uint8_t data;
                } *const opcode = (struct MveOpcodeAudioFrame *)buffer;

                if (rm_track_bit & opcode->mask) {
                    sndAdd((cb.opcode == MVE_OPCODE_AUDIO_FRAME) ? &opcode->data : NULL, opcode->length);
                }
            } break;

            case MVE_OPCODE_INIT_GFX_MODE: {
                struct __attribute__((packed)) MveOpcodeInitGfx {
                    uint16_t window_width;
                    uint16_t window_height;
                    uint16_t flags;
                } *const opcode = (struct MveOpcodeInitGfx *)buffer;

                if (sf_auto) {
                    uint16_t flags = opcode->flags;

                    if (opt_fastmode && !(opt_fastmode & 4)) {
                        flags |= 0x8000;
                    }

                    if (flags == sf_auto_mode) {
                        sf_auto = 1;
                    }

                    if (MVE_gfxMode(flags)) {
                        if (sf_auto_dbl) {
                            MVE_gfxSetDoubleBuffer(0, sf_ResolutionHeight, 0);
                        }

                        sf_auto = 1;
                        sf_auto_mode = flags;

                    } else {
                        MVE_rmEndMovie();

                        return -7;
                    }
                }
            } break;

            case MVE_OPCODE_SYNTH_PALETTE: {
                struct __attribute__((packed)) MveOpcodeSynthPalette {
                    uint8_t base_rb;
                    uint8_t num_r_rb;
                    uint8_t num_b_rb;
                    uint8_t base_rg;
                    uint8_t num_r_rg;
                    uint8_t num_g_rg;
                } *const opcode = (struct MveOpcodeSynthPalette *)buffer;

                SDL_assert(NULL);
                /// \todo
                // palMakeSynthPalette(opcode->base_rb, opcode->num_r_rb, opcode->num_b_rb, opcode->base_rg,
                // opcode->num_r_rg, opcode->num_g_rg);
                palMakePal15();
            } break;

            case MVE_OPCODE_LOAD_PALETTE: {
                struct __attribute__((packed)) MveOpcodeLoadPalette {
                    uint16_t start;
                    uint16_t count;
                    uint8_t data;
                } *const opcode = (struct MveOpcodeLoadPalette *)buffer;

                palLoadPalette(&opcode->data, opcode->start, opcode->count);
                palMakePal15();
            } break;

            case MVE_OPCODE_LOAD_COMPRESSED_PALETTE: {
                SDL_assert(NULL);
                /// \todo
                // palLoadCompPalette(buffer);
                palMakePal15();
            } break;

            case MVE_OPCODE_SET_DECODING_MAP1: {
                decoding_map1 = buffer;
            } break;

            case MVE_OPCODE_SET_DECODING_MAP2: {
                decoding_map2 = buffer;
            } break;

            case MVE_OPCODE_DECOMP_VIDEO1: {
                struct __attribute__((packed)) MveOpcodeDecompVideo1 {
                    uint16_t unused_0;
                    uint16_t unused_1;
                    uint16_t x;
                    uint16_t y;
                    uint16_t width;
                    uint16_t height;
                    uint16_t flags;
                    uint8_t data;
                } *const opcode = (struct MveOpcodeDecompVideo1 *)buffer;

                if (opcode->flags & 0x01) {
                    nfAdvance();
                }

                SDL_assert(NULL);
                /// \todo
                // nfDecompChg(decoding_map1, decoding_map2, &opcode->data, opcode->x, opcode->y, opcode->width,
                // opcode->height);
            } break;

            case MVE_OPCODE_DECOMP_VIDEO2: {
                struct __attribute__((packed)) MveOpcodeDecompVideo2 {
                    uint16_t unused_0;
                    uint16_t unused_1;
                    uint16_t x;
                    uint16_t y;
                    uint16_t width;
                    uint16_t height;
                    uint16_t flags;
                    uint8_t data;
                } *const opcode = (struct MveOpcodeDecompVideo2 *)buffer;

                if (cb.version < 3) {
                    MVE_rmEndMovie();

                    return -8;
                }

                if (opcode->flags & 0x01) {
                    nfAdvance();
                }

                if (!nf_hicolor) {
                    if ((opt_fastmode & 3) == 1 || (opt_fastmode & 3) == 2) {
                        SDL_assert(NULL);
                        /// \todo
                        // nfPkDecompH(decoding_map2, &opcode->data, opcode->x, opcode->y, opcode->width);

                    } else {
                        nfPkDecomp(decoding_map2, &opcode->data, opcode->x, opcode->y, opcode->width, opcode->height);
                    }

                } else if (!opt_fastmode) {
                    SDL_assert(NULL);
                    /// \todo
                    // nfHPkDecomp(decoding_map2, &opcode->data, opcode->x, opcode->y, opcode->width, opcode->height);

                } else {
                    MVE_rmEndMovie();

                    return -8;
                }
            } break;
        }
    }
}

void MVE_rmEndMovie(void) {
    if (rm_active) {
        syncWait();
        syncRelease();
        sndReset();
        rm_active = 0;
    }
}

int32_t MVE_RunMovie(FILE *handle, int32_t dx, int32_t dy, int32_t track) {
    int32_t result = MVE_RunMovieContinue(handle, dx, dy, track);

    MVE_rmEndMovie();

    return result;
}

int32_t MVE_RunMovieContinue(FILE *handle, int32_t dx, int32_t dy, int32_t track) {
    int32_t result = MVE_rmPrepMovie(handle, dx, dy, track);

    while (!result) {
        if ((result = MVE_rmStepMovie()) == 0) {
            for (;;) {
                if ((result = rm_ctl()) != -1) {
                    break;
                }

                if (!rm_hold) {
                    sndPause();
                    rm_hold = 1;
                }

                syncWait();
            }
        }
    }

    if (result == -1) {
        result = 0;
    }

    return result;
}

void MVE_ReleaseMem(void) {
    MVE_rmEndMovie();
    ioRelease();
    sndRelease();
    nfRelease();
}

const char *MVE_strerror(int32_t error_code) {
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

uint16_t nf_pk_lut_2[256] = {
    0xF8F8, 0xF8F9, 0xF8FA, 0xF8FB, 0xF8FC, 0xF8FD, 0xF8FE, 0xF8FF, 0xF800, 0xF801, 0xF802, 0xF803, 0xF804, 0xF805,
    0xF806, 0xF807, 0xF9F8, 0xF9F9, 0xF9FA, 0xF9FB, 0xF9FC, 0xF9FD, 0xF9FE, 0xF9FF, 0xF900, 0xF901, 0xF902, 0xF903,
    0xF904, 0xF905, 0xF906, 0xF907, 0xFAF8, 0xFAF9, 0xFAFA, 0xFAFB, 0xFAFC, 0xFAFD, 0xFAFE, 0xFAFF, 0xFA00, 0xFA01,
    0xFA02, 0xFA03, 0xFA04, 0xFA05, 0xFA06, 0xFA07, 0xFBF8, 0xFBF9, 0xFBFA, 0xFBFB, 0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF,
    0xFB00, 0xFB01, 0xFB02, 0xFB03, 0xFB04, 0xFB05, 0xFB06, 0xFB07, 0xFCF8, 0xFCF9, 0xFCFA, 0xFCFB, 0xFCFC, 0xFCFD,
    0xFCFE, 0xFCFF, 0xFC00, 0xFC01, 0xFC02, 0xFC03, 0xFC04, 0xFC05, 0xFC06, 0xFC07, 0xFDF8, 0xFDF9, 0xFDFA, 0xFDFB,
    0xFDFC, 0xFDFD, 0xFDFE, 0xFDFF, 0xFD00, 0xFD01, 0xFD02, 0xFD03, 0xFD04, 0xFD05, 0xFD06, 0xFD07, 0xFEF8, 0xFEF9,
    0xFEFA, 0xFEFB, 0xFEFC, 0xFEFD, 0xFEFE, 0xFEFF, 0xFE00, 0xFE01, 0xFE02, 0xFE03, 0xFE04, 0xFE05, 0xFE06, 0xFE07,
    0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF, 0xFF00, 0xFF01, 0xFF02, 0xFF03, 0xFF04, 0xFF05,
    0xFF06, 0xFF07, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF, 0x0000, 0x0001, 0x0002, 0x0003,
    0x0004, 0x0005, 0x0006, 0x0007, 0x01F8, 0x01F9, 0x01FA, 0x01FB, 0x01FC, 0x01FD, 0x01FE, 0x01FF, 0x0100, 0x0101,
    0x0102, 0x0103, 0x0104, 0x0105, 0x0106, 0x0107, 0x02F8, 0x02F9, 0x02FA, 0x02FB, 0x02FC, 0x02FD, 0x02FE, 0x02FF,
    0x0200, 0x0201, 0x0202, 0x0203, 0x0204, 0x0205, 0x0206, 0x0207, 0x03F8, 0x03F9, 0x03FA, 0x03FB, 0x03FC, 0x03FD,
    0x03FE, 0x03FF, 0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307, 0x04F8, 0x04F9, 0x04FA, 0x04FB,
    0x04FC, 0x04FD, 0x04FE, 0x04FF, 0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x05F8, 0x05F9,
    0x05FA, 0x05FB, 0x05FC, 0x05FD, 0x05FE, 0x05FF, 0x0500, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506, 0x0507,
    0x06F8, 0x06F9, 0x06FA, 0x06FB, 0x06FC, 0x06FD, 0x06FE, 0x06FF, 0x0600, 0x0601, 0x0602, 0x0603, 0x0604, 0x0605,
    0x0606, 0x0607, 0x07F8, 0x07F9, 0x07FA, 0x07FB, 0x07FC, 0x07FD, 0x07FE, 0x07FF, 0x0700, 0x0701, 0x0702, 0x0703,
    0x0704, 0x0705, 0x0706, 0x0707,
};

uint16_t nf_pk_lut_3[256] = {
    0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x0108, 0x0109, 0x010A, 0x010B, 0x010C, 0x010D, 0x010E,
    0x0208, 0x0209, 0x020A, 0x020B, 0x020C, 0x020D, 0x020E, 0x0308, 0x0309, 0x030A, 0x030B, 0x030C, 0x030D, 0x030E,
    0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x040D, 0x040E, 0x0508, 0x0509, 0x050A, 0x050B, 0x050C, 0x050D, 0x050E,
    0x0608, 0x0609, 0x060A, 0x060B, 0x060C, 0x060D, 0x060E, 0x0708, 0x0709, 0x070A, 0x070B, 0x070C, 0x070D, 0x070E,
    0x08F2, 0x08F3, 0x08F4, 0x08F5, 0x08F6, 0x08F7, 0x08F8, 0x08F9, 0x08FA, 0x08FB, 0x08FC, 0x08FD, 0x08FE, 0x08FF,
    0x0800, 0x0801, 0x0802, 0x0803, 0x0804, 0x0805, 0x0806, 0x0807, 0x0808, 0x0809, 0x080A, 0x080B, 0x080C, 0x080D,
    0x080E, 0x09F2, 0x09F3, 0x09F4, 0x09F5, 0x09F6, 0x09F7, 0x09F8, 0x09F9, 0x09FA, 0x09FB, 0x09FC, 0x09FD, 0x09FE,
    0x09FF, 0x0900, 0x0901, 0x0902, 0x0903, 0x0904, 0x0905, 0x0906, 0x0907, 0x0908, 0x0909, 0x090A, 0x090B, 0x090C,
    0x090D, 0x090E, 0x0AF2, 0x0AF3, 0x0AF4, 0x0AF5, 0x0AF6, 0x0AF7, 0x0AF8, 0x0AF9, 0x0AFA, 0x0AFB, 0x0AFC, 0x0AFD,
    0x0AFE, 0x0AFF, 0x0A00, 0x0A01, 0x0A02, 0x0A03, 0x0A04, 0x0A05, 0x0A06, 0x0A07, 0x0A08, 0x0A09, 0x0A0A, 0x0A0B,
    0x0A0C, 0x0A0D, 0x0A0E, 0x0BF2, 0x0BF3, 0x0BF4, 0x0BF5, 0x0BF6, 0x0BF7, 0x0BF8, 0x0BF9, 0x0BFA, 0x0BFB, 0x0BFC,
    0x0BFD, 0x0BFE, 0x0BFF, 0x0B00, 0x0B01, 0x0B02, 0x0B03, 0x0B04, 0x0B05, 0x0B06, 0x0B07, 0x0B08, 0x0B09, 0x0B0A,
    0x0B0B, 0x0B0C, 0x0B0D, 0x0B0E, 0x0CF2, 0x0CF3, 0x0CF4, 0x0CF5, 0x0CF6, 0x0CF7, 0x0CF8, 0x0CF9, 0x0CFA, 0x0CFB,
    0x0CFC, 0x0CFD, 0x0CFE, 0x0CFF, 0x0C00, 0x0C01, 0x0C02, 0x0C03, 0x0C04, 0x0C05, 0x0C06, 0x0C07, 0x0C08, 0x0C09,
    0x0C0A, 0x0C0B, 0x0C0C, 0x0C0D, 0x0C0E, 0x0DF2, 0x0DF3, 0x0DF4, 0x0DF5, 0x0DF6, 0x0DF7, 0x0DF8, 0x0DF9, 0x0DFA,
    0x0DFB, 0x0DFC, 0x0DFD, 0x0DFE, 0x0DFF, 0x0D00, 0x0D01, 0x0D02, 0x0D03, 0x0D04, 0x0D05, 0x0D06, 0x0D07, 0x0D08,
    0x0D09, 0x0D0A, 0x0D0B, 0x0D0C, 0x0D0D, 0x0D0E, 0x0EF2, 0x0EF3, 0x0EF4, 0x0EF5, 0x0EF6, 0x0EF7, 0x0EF8, 0x0EF9,
    0x0EFA, 0x0EFB, 0x0EFC, 0x0EFD, 0x0EFE, 0x0EFF, 0x0E00, 0x0E01, 0x0E02, 0x0E03, 0x0E04, 0x0E05, 0x0E06, 0x0E07,
    0x0E08, 0x0E09, 0x0E0A, 0x0E0B,
};

uint32_t nf_pk_lut_4[16] = {
    0xC3C3C3C3, 0xC3C3C1C3, 0xC3C3C3C1, 0xC3C3C1C1, 0xC1C3C3C3, 0xC1C3C1C3, 0xC1C3C3C1, 0xC1C3C1C1,
    0xC3C1C3C3, 0xC3C1C1C3, 0xC3C1C3C1, 0xC3C1C1C1, 0xC1C1C3C3, 0xC1C1C1C3, 0xC1C1C3C1, 0xC1C1C1C1,
};

uint32_t nf_pk_lut_5[256] = {
    0xC3C3C3C3, 0xC3C3C2C3, 0xC3C3C1C3, 0xC3C3C5C3, 0xC3C3C3C2, 0xC3C3C2C2, 0xC3C3C1C2, 0xC3C3C5C2, 0xC3C3C3C1,
    0xC3C3C2C1, 0xC3C3C1C1, 0xC3C3C5C1, 0xC3C3C3C5, 0xC3C3C2C5, 0xC3C3C1C5, 0xC3C3C5C5, 0xC2C3C3C3, 0xC2C3C2C3,
    0xC2C3C1C3, 0xC2C3C5C3, 0xC2C3C3C2, 0xC2C3C2C2, 0xC2C3C1C2, 0xC2C3C5C2, 0xC2C3C3C1, 0xC2C3C2C1, 0xC2C3C1C1,
    0xC2C3C5C1, 0xC2C3C3C5, 0xC2C3C2C5, 0xC2C3C1C5, 0xC2C3C5C5, 0xC1C3C3C3, 0xC1C3C2C3, 0xC1C3C1C3, 0xC1C3C5C3,
    0xC1C3C3C2, 0xC1C3C2C2, 0xC1C3C1C2, 0xC1C3C5C2, 0xC1C3C3C1, 0xC1C3C2C1, 0xC1C3C1C1, 0xC1C3C5C1, 0xC1C3C3C5,
    0xC1C3C2C5, 0xC1C3C1C5, 0xC1C3C5C5, 0xC5C3C3C3, 0xC5C3C2C3, 0xC5C3C1C3, 0xC5C3C5C3, 0xC5C3C3C2, 0xC5C3C2C2,
    0xC5C3C1C2, 0xC5C3C5C2, 0xC5C3C3C1, 0xC5C3C2C1, 0xC5C3C1C1, 0xC5C3C5C1, 0xC5C3C3C5, 0xC5C3C2C5, 0xC5C3C1C5,
    0xC5C3C5C5, 0xC3C2C3C3, 0xC3C2C2C3, 0xC3C2C1C3, 0xC3C2C5C3, 0xC3C2C3C2, 0xC3C2C2C2, 0xC3C2C1C2, 0xC3C2C5C2,
    0xC3C2C3C1, 0xC3C2C2C1, 0xC3C2C1C1, 0xC3C2C5C1, 0xC3C2C3C5, 0xC3C2C2C5, 0xC3C2C1C5, 0xC3C2C5C5, 0xC2C2C3C3,
    0xC2C2C2C3, 0xC2C2C1C3, 0xC2C2C5C3, 0xC2C2C3C2, 0xC2C2C2C2, 0xC2C2C1C2, 0xC2C2C5C2, 0xC2C2C3C1, 0xC2C2C2C1,
    0xC2C2C1C1, 0xC2C2C5C1, 0xC2C2C3C5, 0xC2C2C2C5, 0xC2C2C1C5, 0xC2C2C5C5, 0xC1C2C3C3, 0xC1C2C2C3, 0xC1C2C1C3,
    0xC1C2C5C3, 0xC1C2C3C2, 0xC1C2C2C2, 0xC1C2C1C2, 0xC1C2C5C2, 0xC1C2C3C1, 0xC1C2C2C1, 0xC1C2C1C1, 0xC1C2C5C1,
    0xC1C2C3C5, 0xC1C2C2C5, 0xC1C2C1C5, 0xC1C2C5C5, 0xC5C2C3C3, 0xC5C2C2C3, 0xC5C2C1C3, 0xC5C2C5C3, 0xC5C2C3C2,
    0xC5C2C2C2, 0xC5C2C1C2, 0xC5C2C5C2, 0xC5C2C3C1, 0xC5C2C2C1, 0xC5C2C1C1, 0xC5C2C5C1, 0xC5C2C3C5, 0xC5C2C2C5,
    0xC5C2C1C5, 0xC5C2C5C5, 0xC3C1C3C3, 0xC3C1C2C3, 0xC3C1C1C3, 0xC3C1C5C3, 0xC3C1C3C2, 0xC3C1C2C2, 0xC3C1C1C2,
    0xC3C1C5C2, 0xC3C1C3C1, 0xC3C1C2C1, 0xC3C1C1C1, 0xC3C1C5C1, 0xC3C1C3C5, 0xC3C1C2C5, 0xC3C1C1C5, 0xC3C1C5C5,
    0xC2C1C3C3, 0xC2C1C2C3, 0xC2C1C1C3, 0xC2C1C5C3, 0xC2C1C3C2, 0xC2C1C2C2, 0xC2C1C1C2, 0xC2C1C5C2, 0xC2C1C3C1,
    0xC2C1C2C1, 0xC2C1C1C1, 0xC2C1C5C1, 0xC2C1C3C5, 0xC2C1C2C5, 0xC2C1C1C5, 0xC2C1C5C5, 0xC1C1C3C3, 0xC1C1C2C3,
    0xC1C1C1C3, 0xC1C1C5C3, 0xC1C1C3C2, 0xC1C1C2C2, 0xC1C1C1C2, 0xC1C1C5C2, 0xC1C1C3C1, 0xC1C1C2C1, 0xC1C1C1C1,
    0xC1C1C5C1, 0xC1C1C3C5, 0xC1C1C2C5, 0xC1C1C1C5, 0xC1C1C5C5, 0xC5C1C3C3, 0xC5C1C2C3, 0xC5C1C1C3, 0xC5C1C5C3,
    0xC5C1C3C2, 0xC5C1C2C2, 0xC5C1C1C2, 0xC5C1C5C2, 0xC5C1C3C1, 0xC5C1C2C1, 0xC5C1C1C1, 0xC5C1C5C1, 0xC5C1C3C5,
    0xC5C1C2C5, 0xC5C1C1C5, 0xC5C1C5C5, 0xC3C5C3C3, 0xC3C5C2C3, 0xC3C5C1C3, 0xC3C5C5C3, 0xC3C5C3C2, 0xC3C5C2C2,
    0xC3C5C1C2, 0xC3C5C5C2, 0xC3C5C3C1, 0xC3C5C2C1, 0xC3C5C1C1, 0xC3C5C5C1, 0xC3C5C3C5, 0xC3C5C2C5, 0xC3C5C1C5,
    0xC3C5C5C5, 0xC2C5C3C3, 0xC2C5C2C3, 0xC2C5C1C3, 0xC2C5C5C3, 0xC2C5C3C2, 0xC2C5C2C2, 0xC2C5C1C2, 0xC2C5C5C2,
    0xC2C5C3C1, 0xC2C5C2C1, 0xC2C5C1C1, 0xC2C5C5C1, 0xC2C5C3C5, 0xC2C5C2C5, 0xC2C5C1C5, 0xC2C5C5C5, 0xC1C5C3C3,
    0xC1C5C2C3, 0xC1C5C1C3, 0xC1C5C5C3, 0xC1C5C3C2, 0xC1C5C2C2, 0xC1C5C1C2, 0xC1C5C5C2, 0xC1C5C3C1, 0xC1C5C2C1,
    0xC1C5C1C1, 0xC1C5C5C1, 0xC1C5C3C5, 0xC1C5C2C5, 0xC1C5C1C5, 0xC1C5C5C5, 0xC5C5C3C3, 0xC5C5C2C3, 0xC5C5C1C3,
    0xC5C5C5C3, 0xC5C5C3C2, 0xC5C5C2C2, 0xC5C5C1C2, 0xC5C5C5C2, 0xC5C5C3C1, 0xC5C5C2C1, 0xC5C5C1C1, 0xC5C5C5C1,
    0xC5C5C3C5, 0xC5C5C2C5, 0xC5C5C1C5, 0xC5C5C5C5,
};

uint32_t nf_pk_lut_6[256] = {
    0xE3C3E3C3, 0xE3C7E3C3, 0xE3C1E3C3, 0xE3C5E3C3, 0xE7C3E3C3, 0xE7C7E3C3, 0xE7C1E3C3, 0xE7C5E3C3, 0xE1C3E3C3,
    0xE1C7E3C3, 0xE1C1E3C3, 0xE1C5E3C3, 0xE5C3E3C3, 0xE5C7E3C3, 0xE5C1E3C3, 0xE5C5E3C3, 0xE3C3E3C7, 0xE3C7E3C7,
    0xE3C1E3C7, 0xE3C5E3C7, 0xE7C3E3C7, 0xE7C7E3C7, 0xE7C1E3C7, 0xE7C5E3C7, 0xE1C3E3C7, 0xE1C7E3C7, 0xE1C1E3C7,
    0xE1C5E3C7, 0xE5C3E3C7, 0xE5C7E3C7, 0xE5C1E3C7, 0xE5C5E3C7, 0xE3C3E3C1, 0xE3C7E3C1, 0xE3C1E3C1, 0xE3C5E3C1,
    0xE7C3E3C1, 0xE7C7E3C1, 0xE7C1E3C1, 0xE7C5E3C1, 0xE1C3E3C1, 0xE1C7E3C1, 0xE1C1E3C1, 0xE1C5E3C1, 0xE5C3E3C1,
    0xE5C7E3C1, 0xE5C1E3C1, 0xE5C5E3C1, 0xE3C3E3C5, 0xE3C7E3C5, 0xE3C1E3C5, 0xE3C5E3C5, 0xE7C3E3C5, 0xE7C7E3C5,
    0xE7C1E3C5, 0xE7C5E3C5, 0xE1C3E3C5, 0xE1C7E3C5, 0xE1C1E3C5, 0xE1C5E3C5, 0xE5C3E3C5, 0xE5C7E3C5, 0xE5C1E3C5,
    0xE5C5E3C5, 0xE3C3E7C3, 0xE3C7E7C3, 0xE3C1E7C3, 0xE3C5E7C3, 0xE7C3E7C3, 0xE7C7E7C3, 0xE7C1E7C3, 0xE7C5E7C3,
    0xE1C3E7C3, 0xE1C7E7C3, 0xE1C1E7C3, 0xE1C5E7C3, 0xE5C3E7C3, 0xE5C7E7C3, 0xE5C1E7C3, 0xE5C5E7C3, 0xE3C3E7C7,
    0xE3C7E7C7, 0xE3C1E7C7, 0xE3C5E7C7, 0xE7C3E7C7, 0xE7C7E7C7, 0xE7C1E7C7, 0xE7C5E7C7, 0xE1C3E7C7, 0xE1C7E7C7,
    0xE1C1E7C7, 0xE1C5E7C7, 0xE5C3E7C7, 0xE5C7E7C7, 0xE5C1E7C7, 0xE5C5E7C7, 0xE3C3E7C1, 0xE3C7E7C1, 0xE3C1E7C1,
    0xE3C5E7C1, 0xE7C3E7C1, 0xE7C7E7C1, 0xE7C1E7C1, 0xE7C5E7C1, 0xE1C3E7C1, 0xE1C7E7C1, 0xE1C1E7C1, 0xE1C5E7C1,
    0xE5C3E7C1, 0xE5C7E7C1, 0xE5C1E7C1, 0xE5C5E7C1, 0xE3C3E7C5, 0xE3C7E7C5, 0xE3C1E7C5, 0xE3C5E7C5, 0xE7C3E7C5,
    0xE7C7E7C5, 0xE7C1E7C5, 0xE7C5E7C5, 0xE1C3E7C5, 0xE1C7E7C5, 0xE1C1E7C5, 0xE1C5E7C5, 0xE5C3E7C5, 0xE5C7E7C5,
    0xE5C1E7C5, 0xE5C5E7C5, 0xE3C3E1C3, 0xE3C7E1C3, 0xE3C1E1C3, 0xE3C5E1C3, 0xE7C3E1C3, 0xE7C7E1C3, 0xE7C1E1C3,
    0xE7C5E1C3, 0xE1C3E1C3, 0xE1C7E1C3, 0xE1C1E1C3, 0xE1C5E1C3, 0xE5C3E1C3, 0xE5C7E1C3, 0xE5C1E1C3, 0xE5C5E1C3,
    0xE3C3E1C7, 0xE3C7E1C7, 0xE3C1E1C7, 0xE3C5E1C7, 0xE7C3E1C7, 0xE7C7E1C7, 0xE7C1E1C7, 0xE7C5E1C7, 0xE1C3E1C7,
    0xE1C7E1C7, 0xE1C1E1C7, 0xE1C5E1C7, 0xE5C3E1C7, 0xE5C7E1C7, 0xE5C1E1C7, 0xE5C5E1C7, 0xE3C3E1C1, 0xE3C7E1C1,
    0xE3C1E1C1, 0xE3C5E1C1, 0xE7C3E1C1, 0xE7C7E1C1, 0xE7C1E1C1, 0xE7C5E1C1, 0xE1C3E1C1, 0xE1C7E1C1, 0xE1C1E1C1,
    0xE1C5E1C1, 0xE5C3E1C1, 0xE5C7E1C1, 0xE5C1E1C1, 0xE5C5E1C1, 0xE3C3E1C5, 0xE3C7E1C5, 0xE3C1E1C5, 0xE3C5E1C5,
    0xE7C3E1C5, 0xE7C7E1C5, 0xE7C1E1C5, 0xE7C5E1C5, 0xE1C3E1C5, 0xE1C7E1C5, 0xE1C1E1C5, 0xE1C5E1C5, 0xE5C3E1C5,
    0xE5C7E1C5, 0xE5C1E1C5, 0xE5C5E1C5, 0xE3C3E5C3, 0xE3C7E5C3, 0xE3C1E5C3, 0xE3C5E5C3, 0xE7C3E5C3, 0xE7C7E5C3,
    0xE7C1E5C3, 0xE7C5E5C3, 0xE1C3E5C3, 0xE1C7E5C3, 0xE1C1E5C3, 0xE1C5E5C3, 0xE5C3E5C3, 0xE5C7E5C3, 0xE5C1E5C3,
    0xE5C5E5C3, 0xE3C3E5C7, 0xE3C7E5C7, 0xE3C1E5C7, 0xE3C5E5C7, 0xE7C3E5C7, 0xE7C7E5C7, 0xE7C1E5C7, 0xE7C5E5C7,
    0xE1C3E5C7, 0xE1C7E5C7, 0xE1C1E5C7, 0xE1C5E5C7, 0xE5C3E5C7, 0xE5C7E5C7, 0xE5C1E5C7, 0xE5C5E5C7, 0xE3C3E5C1,
    0xE3C7E5C1, 0xE3C1E5C1, 0xE3C5E5C1, 0xE7C3E5C1, 0xE7C7E5C1, 0xE7C1E5C1, 0xE7C5E5C1, 0xE1C3E5C1, 0xE1C7E5C1,
    0xE1C1E5C1, 0xE1C5E5C1, 0xE5C3E5C1, 0xE5C7E5C1, 0xE5C1E5C1, 0xE5C5E5C1, 0xE3C3E5C5, 0xE3C7E5C5, 0xE3C1E5C5,
    0xE3C5E5C5, 0xE7C3E5C5, 0xE7C7E5C5, 0xE7C1E5C5, 0xE7C5E5C5, 0xE1C3E5C5, 0xE1C7E5C5, 0xE1C1E5C5, 0xE1C5E5C5,
    0xE5C3E5C5, 0xE5C7E5C5, 0xE5C1E5C5, 0xE5C5E5C5,
};

void nfBlockCopy(uint8_t *dst, uint8_t *src) {
    for (int32_t i = 0; i < 8; ++i) {
        memcpy(dst, src, 8);
        dst += nf_width;
        src += nf_width;
    }
}

void nfFillPattern(uint8_t *dst, uint8_t *src, uint8_t *map1, uint32_t *map2, uint8_t stride, uint8_t offset,
                   int32_t mode) {
    uint32_t value;

    for (int32_t i = 0; i < stride; ++i) {
        value = nf_pk_lut_5[src[2 + i]];
        map1[offset + i * 4 + 0] = (value >> 0);
        map1[offset + i * 4 + 1] = (value >> 8);
        map1[offset + i * 4 + 2] = (value >> 16);
        map1[offset + i * 4 + 3] = (value >> 24);
    }

    map2[0xC1] = (src[1] << 8) | src[0];
    map2[0xC2] = (src[0] << 8) | src[1];
    map2[0xC3] = (src[0] << 8) | src[0];
    map2[0xC5] = (src[1] << 8) | src[1];

    for (int32_t i = 0; i < stride; ++i) {
        if (mode == 2) {
            ((uint32_t *)dst)[0] = (map2[map1[offset + i * 4 + 0]] << 16) | map2[map1[offset + i * 4 + 1]];
            dst += nf_width;
            ((uint32_t *)dst)[0] = (map2[map1[offset + i * 4 + 2]] << 16) | map2[map1[offset + i * 4 + 3]];
            dst += nf_width;

        } else if (mode == 1) {
            ((uint32_t *)dst)[0] = (map2[map1[offset + i * 4 + 2]] << 16) | map2[map1[offset + i * 4 + 1]];
            dst += nf_width;
            ((uint32_t *)dst)[0] = (map2[map1[offset + i * 4 + 2]] << 16) | map2[map1[offset + i * 4 + 3]];
            dst += nf_width;

        } else if (mode == 0) {
            ((uint32_t *)dst)[0] = (map2[map1[offset + i * 4 + 0]] << 16) | map2[map1[offset + i * 4 + 1]];
            ((uint32_t *)dst)[1] = (map2[map1[offset + i * 4 + 2]] << 16) | map2[map1[offset + i * 4 + 3]];
            dst += nf_width;
        }
    }
}

void nfFillPattern11(uint8_t *src, uint8_t *map1, uint32_t *map2, uint8_t stride, int32_t mode) {
    uint32_t value;

    for (int32_t i = 0; i < stride; ++i) {
        value = nf_pk_lut_6[src[4 + i]];

        if (mode) {
            value = SDL_Swap32(value);
        }

        map1[i * 4 + 0] = (value >> 0);
        map1[i * 4 + 1] = (value >> 8);
        map1[i * 4 + 2] = (value >> 16);
        map1[i * 4 + 3] = (value >> 24);
    }

    map2[0xC1] = src[2];
    map2[0xC3] = src[0];
    map2[0xC5] = src[3];
    map2[0xC7] = src[1];
    map2[0xE1] = src[2];
    map2[0xE3] = src[0];
    map2[0xE5] = src[3];
    map2[0xE7] = src[1];
}

void nfFillPattern12(uint8_t *dst, uint8_t *map1, uint32_t *map2, int32_t index, int32_t mode) {
    switch (mode) {
        case 0: {
            ((uint32_t *)dst)[0] = (map2[map1[index + 0]] << 16) | (map2[map1[index + 1]] << 24) |
                                   (map2[map1[index + 2]] << 0) | (map2[map1[index + 3]] << 8);
            ((uint32_t *)dst)[1] = (map2[map1[index + 4]] << 16) | (map2[map1[index + 5]] << 24) |
                                   (map2[map1[index + 6]] << 0) | (map2[map1[index + 7]] << 8);
        } break;

        case 1: {
            ((uint32_t *)dst)[0] = (map2[map1[index + 0]] << 24) | (map2[map1[index + 0]] << 16) |
                                   (map2[map1[index + 1]] << 8) | (map2[map1[index + 1]]);
            ((uint32_t *)dst)[1] = (map2[map1[index + 2]] << 24) | (map2[map1[index + 2]] << 16) |
                                   (map2[map1[index + 3]] << 8) | (map2[map1[index + 3]]);
        } break;
    }
}

void nfPkDecomp(uint8_t *decoding_map, uint8_t *buffer, int32_t pos_x, int32_t pos_y, int32_t width, int32_t height) {
    uint8_t map1[512];
    uint32_t map2[256];
    uint32_t nibbles[2];
    uint8_t *dst;
    intptr_t offset;
    int32_t code_type;
    int32_t byte;
    uint32_t value1;
    uint32_t value2;
    int32_t value3;

    nf_new_x = 8 * pos_x;
    nf_new_y = 8 * pos_y * nf_fqty;

    nf_new_w = 8 * width;
    nf_new_h = 8 * height * nf_fqty;

    dst = nf_buf_cur;

    if (pos_x || pos_y) {
        dst = &nf_buf_cur[nf_new_x + nf_width * nf_new_y];
    }

    while (height--) {
        pos_x = width / 2;
        while (pos_x--) {
            value1 = *decoding_map++;
            nibbles[0] = value1 & 0xF;
            nibbles[1] = value1 >> 4;
            for (int32_t j = 0; j < 2; ++j) {
                code_type = nibbles[j];

                switch (code_type) {
                    case 0: {
                        nfBlockCopy(dst, dst + (nf_buf_prv - nf_buf_cur));
                        dst += 8;
                    } break;

                    case 1: {
                        dst += 8;
                    } break;

                    case 2: {
                        byte = *buffer++;
                        value3 = nf_pk_lut_3[byte];
                        offset = ((value3 << 24) >> 24) + nf_conf_tbl[value3 >> 8];

                        nfBlockCopy(dst, dst + offset);
                        dst += 8;
                    } break;

                    case 3: {
                        byte = *buffer++;
                        value3 = nf_pk_lut_3[byte];
                        value3 = ((-(value3 & 0xFF)) & 0xFF) | ((-(value3 >> 8) & 0xFF) << 8);
                        offset = ((value3 << 24) >> 24) + nf_conf_tbl[value3 >> 8];

                        nfBlockCopy(dst, dst + offset);
                        dst += 8;
                    } break;

                    case 4: {
                        byte = *buffer++;
                        value3 = nf_pk_lut_2[byte];
                        offset = ((value3 << 24) >> 24) + nf_conf_tbl[value3 >> 8] + (nf_buf_prv - nf_buf_cur);

                        nfBlockCopy(dst, dst + offset);
                        dst += 8;
                    } break;

                    case 5: {
                        value3 = *(uint16_t *)buffer;
                        buffer += 2;
                        offset = ((value3 << 24) >> 24) + nf_conf_tbl[value3 >> 8] + (nf_buf_prv - nf_buf_cur);

                        nfBlockCopy(dst, dst + offset);
                        dst += 8;
                    } break;

                    case 6: {
                        nibbles[0] += 2;
                        while (nibbles[0]--) {
                            dst += 16;

                            if (pos_x--) {
                                continue;
                            }

                            dst += nf_new_row0 - nf_new_w;
                            --height;
                            pos_x = (width / 2) - 1;
                        }
                    } break;

                    case 7: {
                        if (buffer[0] > buffer[1]) {
                            for (int32_t i = 0; i < 2; ++i) {
                                value1 = nf_pk_lut_4[buffer[2 + i] & 0xF];
                                map1[i * 8 + 0] = (value1 >> 0);
                                map1[i * 8 + 1] = (value1 >> 8);
                                map1[i * 8 + 2] = (value1 >> 16);
                                map1[i * 8 + 3] = (value1 >> 24);

                                value1 = nf_pk_lut_4[buffer[2 + i] >> 4];
                                map1[4 + i * 8 + 0] = (value1 >> 0);
                                map1[4 + i * 8 + 1] = (value1 >> 8);
                                map1[4 + i * 8 + 2] = (value1 >> 16);
                                map1[4 + i * 8 + 3] = (value1 >> 24);
                            }

                            map2[0xC1] = (buffer[1] << 8) | buffer[1];
                            map2[0xC3] = (buffer[0] << 8) | buffer[0];

                            for (int32_t i = 0; i < 4; ++i) {
                                ((uint32_t *)dst)[0] = (map2[map1[i * 4 + 0]] << 16) | (map2[map1[i * 4 + 1]]);
                                ((uint32_t *)dst)[1] = (map2[map1[i * 4 + 2]] << 16) | (map2[map1[i * 4 + 3]]);
                                dst += nf_width;

                                ((uint32_t *)dst)[0] = (map2[map1[i * 4 + 0]] << 16) | (map2[map1[i * 4 + 1]]);
                                ((uint32_t *)dst)[1] = (map2[map1[i * 4 + 2]] << 16) | (map2[map1[i * 4 + 3]]);
                                dst += nf_width;
                            }

                            dst -= nf_width;

                            buffer += 4;
                            dst -= nf_back_right - 8;

                        } else {
                            nfFillPattern(dst, buffer, map1, map2, 8, 0, 0);
                            buffer += 10;
                            dst += 8;
                        }
                    } break;

                    case 8: {
                        if (buffer[0] > buffer[1]) {
                            if (buffer[6] > buffer[7]) {
                                nfFillPattern(dst, buffer, map1, map2, 4, 0, 0);
                                buffer += 6;

                                nfFillPattern(&dst[4 * nf_width], buffer, map1, map2, 4, 16, 0);
                                buffer += 6;
                                dst += 8;

                            } else {
                                nfFillPattern(dst, buffer, map1, map2, 4, 0, 1);
                                buffer += 6;
                                dst += 4;

                                nfFillPattern(dst, buffer, map1, map2, 4, 16, 2);
                                buffer += 6;
                                dst += 4;
                            }

                        } else {
                            nfFillPattern(dst, buffer, map1, map2, 2, 0, 2);
                            buffer += 4;

                            nfFillPattern(&dst[4 * nf_width], buffer, map1, map2, 2, 8, 2);
                            buffer += 4;
                            dst += 4;

                            nfFillPattern(dst, buffer, map1, map2, 2, 16, 2);
                            buffer += 4;

                            nfFillPattern(&dst[4 * nf_width], buffer, map1, map2, 2, 24, 2);
                            buffer += 4;
                            dst += 4;
                        }
                    } break;

                    case 9: {
                        if (buffer[0] > buffer[1]) {
                            if (buffer[2] > buffer[3]) {
                                nfFillPattern11(buffer, map1, map2, 8, 0);

                                for (int32_t i = 0; i < 4; ++i) {
                                    nfFillPattern12(dst, map1, map2, i * 8, 0);
                                    dst += nf_width;
                                    nfFillPattern12(dst, map1, map2, i * 8, 0);
                                    dst += nf_width;
                                }

                                buffer += 12;
                                dst -= nf_width;
                                dst -= nf_back_right - 8;

                            } else {
                                nfFillPattern11(buffer, map1, map2, 8, 1);

                                for (int32_t i = 0; i < 8; ++i) {
                                    nfFillPattern12(dst, map1, map2, i * 4, 1);
                                    dst += nf_width;
                                }

                                dst -= nf_width;

                                buffer += 12;
                                dst -= nf_back_right - 8;
                            }
                        } else {
                            if (buffer[2] > buffer[3]) {
                                nfFillPattern11(buffer, map1, map2, 8, 1);

                                for (int32_t i = 0; i < 4; ++i) {
                                    nfFillPattern12(dst, map1, map2, i * 4, 1);
                                    dst += nf_width;
                                    nfFillPattern12(dst, map1, map2, i * 4, 1);
                                    dst += nf_width;
                                }

                                dst -= nf_width;

                                buffer += 8;
                                dst -= nf_back_right - 8;

                            } else {
                                nfFillPattern11(buffer, map1, map2, 16, 0);

                                for (int32_t i = 0; i < 8; ++i) {
                                    nfFillPattern12(dst, map1, map2, i * 8, 0);
                                    dst += nf_width;
                                }

                                dst -= nf_width;

                                buffer += 20;
                                dst -= nf_back_right - 8;
                            }
                        }
                    } break;

                    case 10: {
                        if (buffer[0] > buffer[1]) {
                            if (buffer[12] > buffer[13]) {
                                for (int32_t i = 0; i < 8; ++i) {
                                    value1 = nf_pk_lut_6[buffer[4 + i]];
                                    map1[i * 4 + 0] = (value1 >> 0);
                                    map1[i * 4 + 1] = (value1 >> 8);
                                    map1[i * 4 + 2] = (value1 >> 16);
                                    map1[i * 4 + 3] = (value1 >> 24);
                                }

                                for (int32_t i = 0; i < 8; ++i) {
                                    value1 = nf_pk_lut_6[buffer[16 + i]];
                                    map1[32 + i * 4 + 0] = (value1 >> 0);
                                    map1[32 + i * 4 + 1] = (value1 >> 8);
                                    map1[32 + i * 4 + 2] = (value1 >> 16);
                                    map1[32 + i * 4 + 3] = (value1 >> 24);
                                }

                                map2[0xC1] = buffer[2];
                                map2[0xC3] = buffer[0];
                                map2[0xC5] = buffer[3];
                                map2[0xC7] = buffer[1];
                                map2[0xE1] = buffer[2];
                                map2[0xE3] = buffer[0];
                                map2[0xE5] = buffer[3];
                                map2[0xE7] = buffer[1];

                                for (int32_t i = 0; i < 4; ++i) {
                                    ((uint32_t *)dst)[0] = (map2[map1[i * 8 + 0]] << 16) |
                                                           (map2[map1[i * 8 + 1]] << 24) | (map2[map1[i * 8 + 2]]) |
                                                           (map2[map1[i * 8 + 3]] << 8);
                                    ((uint32_t *)dst)[1] = (map2[map1[i * 8 + 4]] << 16) |
                                                           (map2[map1[i * 8 + 5]] << 24) | (map2[map1[i * 8 + 6]]) |
                                                           (map2[map1[i * 8 + 7]] << 8);
                                    dst += nf_width;
                                }

                                map2[0xC1] = buffer[12 + 2];
                                map2[0xC3] = buffer[12 + 0];
                                map2[0xC5] = buffer[12 + 3];
                                map2[0xC7] = buffer[12 + 1];
                                map2[0xE1] = buffer[12 + 2];
                                map2[0xE3] = buffer[12 + 0];
                                map2[0xE5] = buffer[12 + 3];
                                map2[0xE7] = buffer[12 + 1];

                                for (int32_t i = 0; i < 4; ++i) {
                                    ((uint32_t *)dst)[0] =
                                        (map2[map1[32 + i * 8 + 0]] << 16) | (map2[map1[32 + i * 8 + 1]] << 24) |
                                        (map2[map1[32 + i * 8 + 2]]) | (map2[map1[32 + i * 8 + 3]] << 8);
                                    ((uint32_t *)dst)[1] =
                                        (map2[map1[32 + i * 8 + 4]] << 16) | (map2[map1[32 + i * 8 + 5]] << 24) |
                                        (map2[map1[32 + i * 8 + 6]]) | (map2[map1[32 + i * 8 + 7]] << 8);
                                    dst += nf_width;
                                }

                                dst -= nf_width;

                                buffer += 24;
                                dst -= nf_back_right - 8;

                            } else {
                                for (int32_t i = 0; i < 8; ++i) {
                                    value1 = nf_pk_lut_6[buffer[4 + i]];
                                    map1[i * 4 + 0] = (value1 >> 0);
                                    map1[i * 4 + 1] = (value1 >> 8);
                                    map1[i * 4 + 2] = (value1 >> 16);
                                    map1[i * 4 + 3] = (value1 >> 24);
                                }

                                for (int32_t i = 0; i < 8; ++i) {
                                    value1 = nf_pk_lut_6[buffer[16 + i]];
                                    map1[32 + i * 4 + 0] = (value1 >> 0);
                                    map1[32 + i * 4 + 1] = (value1 >> 8);
                                    map1[32 + i * 4 + 2] = (value1 >> 16);
                                    map1[32 + i * 4 + 3] = (value1 >> 24);
                                }

                                map2[0xC1] = buffer[2];
                                map2[0xC3] = buffer[0];
                                map2[0xC5] = buffer[3];
                                map2[0xC7] = buffer[1];
                                map2[0xE1] = buffer[2];
                                map2[0xE3] = buffer[0];
                                map2[0xE5] = buffer[3];
                                map2[0xE7] = buffer[1];

                                for (int32_t i = 0; i < 4; ++i) {
                                    ((uint32_t *)dst)[0] = (map2[map1[i * 8 + 0]] << 16) |
                                                           (map2[map1[i * 8 + 1]] << 24) | (map2[map1[i * 8 + 2]]) |
                                                           (map2[map1[i * 8 + 3]] << 8);
                                    dst += nf_width;

                                    ((uint32_t *)dst)[0] = (map2[map1[i * 8 + 4]] << 16) |
                                                           (map2[map1[i * 8 + 5]] << 24) | (map2[map1[i * 8 + 6]]) |
                                                           (map2[map1[i * 8 + 7]] << 8);
                                    dst += nf_width;
                                }

                                dst -= nf_width * 8 - 4;

                                map2[0xC1] = buffer[12 + 2];
                                map2[0xC3] = buffer[12 + 0];
                                map2[0xC5] = buffer[12 + 3];
                                map2[0xC7] = buffer[12 + 1];
                                map2[0xE1] = buffer[12 + 2];
                                map2[0xE3] = buffer[12 + 0];
                                map2[0xE5] = buffer[12 + 3];
                                map2[0xE7] = buffer[12 + 1];

                                for (int32_t i = 0; i < 4; ++i) {
                                    ((uint32_t *)dst)[0] =
                                        (map2[map1[32 + i * 8 + 0]] << 16) | (map2[map1[32 + i * 8 + 1]] << 24) |
                                        (map2[map1[32 + i * 8 + 2]]) | (map2[map1[32 + i * 8 + 3]] << 8);
                                    dst += nf_width;

                                    ((uint32_t *)dst)[0] =
                                        (map2[map1[32 + i * 8 + 4]] << 16) | (map2[map1[32 + i * 8 + 5]] << 24) |
                                        (map2[map1[32 + i * 8 + 6]]) | (map2[map1[32 + i * 8 + 7]] << 8);
                                    dst += nf_width;
                                }

                                dst -= nf_width;
                                buffer += 24;
                                dst -= 4;
                                dst -= nf_back_right - 8;
                            }

                        } else {
                            for (int32_t i = 0; i < 4; ++i) {
                                value1 = nf_pk_lut_6[buffer[4 + i]];
                                map1[i * 4 + 0] = (value1 >> 0);
                                map1[i * 4 + 1] = (value1 >> 8);
                                map1[i * 4 + 2] = (value1 >> 16);
                                map1[i * 4 + 3] = (value1 >> 24);
                            }

                            for (int32_t i = 0; i < 4; ++i) {
                                value1 = nf_pk_lut_6[buffer[12 + i]];
                                map1[16 + i * 4 + 0] = (value1 >> 0);
                                map1[16 + i * 4 + 1] = (value1 >> 8);
                                map1[16 + i * 4 + 2] = (value1 >> 16);
                                map1[16 + i * 4 + 3] = (value1 >> 24);
                            }

                            for (int32_t i = 0; i < 4; ++i) {
                                value1 = nf_pk_lut_6[buffer[20 + i]];
                                map1[32 + i * 4 + 0] = (value1 >> 0);
                                map1[32 + i * 4 + 1] = (value1 >> 8);
                                map1[32 + i * 4 + 2] = (value1 >> 16);
                                map1[32 + i * 4 + 3] = (value1 >> 24);
                            }

                            for (int32_t i = 0; i < 4; ++i) {
                                value1 = nf_pk_lut_6[buffer[28 + i]];
                                map1[48 + i * 4 + 0] = (value1 >> 0);
                                map1[48 + i * 4 + 1] = (value1 >> 8);
                                map1[48 + i * 4 + 2] = (value1 >> 16);
                                map1[48 + i * 4 + 3] = (value1 >> 24);
                            }

                            map2[0xC1] = buffer[2];
                            map2[0xC3] = buffer[0];
                            map2[0xC5] = buffer[3];
                            map2[0xC7] = buffer[1];
                            map2[0xE1] = buffer[2];
                            map2[0xE3] = buffer[0];
                            map2[0xE5] = buffer[3];
                            map2[0xE7] = buffer[1];

                            for (int32_t i = 0; i < 2; ++i) {
                                ((uint32_t *)dst)[0] = (map2[map1[i * 8 + 0]] << 16) | (map2[map1[i * 8 + 1]] << 24) |
                                                       (map2[map1[i * 8 + 2]]) | (map2[map1[i * 8 + 3]] << 8);
                                dst += nf_width;

                                ((uint32_t *)dst)[0] = (map2[map1[i * 8 + 4]] << 16) | (map2[map1[i * 8 + 5]] << 24) |
                                                       (map2[map1[i * 8 + 6]]) | (map2[map1[i * 8 + 7]] << 8);
                                dst += nf_width;
                            }

                            map2[0xC1] = buffer[8 + 2];
                            map2[0xC3] = buffer[8 + 0];
                            map2[0xC5] = buffer[8 + 3];
                            map2[0xC7] = buffer[8 + 1];
                            map2[0xE1] = buffer[8 + 2];
                            map2[0xE3] = buffer[8 + 0];
                            map2[0xE5] = buffer[8 + 3];
                            map2[0xE7] = buffer[8 + 1];

                            for (int32_t i = 0; i < 2; ++i) {
                                ((uint32_t *)dst)[0] = (map2[map1[16 + i * 8 + 0]] << 16) |
                                                       (map2[map1[16 + i * 8 + 1]] << 24) |
                                                       (map2[map1[16 + i * 8 + 2]]) | (map2[map1[16 + i * 8 + 3]] << 8);
                                dst += nf_width;

                                ((uint32_t *)dst)[0] = (map2[map1[16 + i * 8 + 4]] << 16) |
                                                       (map2[map1[16 + i * 8 + 5]] << 24) |
                                                       (map2[map1[16 + i * 8 + 6]]) | (map2[map1[16 + i * 8 + 7]] << 8);
                                dst += nf_width;
                            }

                            dst -= nf_width * 8 - 4;

                            map2[0xC1] = buffer[16 + 2];
                            map2[0xC3] = buffer[16 + 0];
                            map2[0xC5] = buffer[16 + 3];
                            map2[0xC7] = buffer[16 + 1];
                            map2[0xE1] = buffer[16 + 2];
                            map2[0xE3] = buffer[16 + 0];
                            map2[0xE5] = buffer[16 + 3];
                            map2[0xE7] = buffer[16 + 1];

                            for (int32_t i = 0; i < 2; ++i) {
                                ((uint32_t *)dst)[0] = (map2[map1[32 + i * 8 + 0]] << 16) |
                                                       (map2[map1[32 + i * 8 + 1]] << 24) |
                                                       (map2[map1[32 + i * 8 + 2]]) | (map2[map1[32 + i * 8 + 3]] << 8);
                                dst += nf_width;

                                ((uint32_t *)dst)[0] = (map2[map1[32 + i * 8 + 4]] << 16) |
                                                       (map2[map1[32 + i * 8 + 5]] << 24) |
                                                       (map2[map1[32 + i * 8 + 6]]) | (map2[map1[32 + i * 8 + 7]] << 8);
                                dst += nf_width;
                            }

                            map2[0xC1] = buffer[24 + 2];
                            map2[0xC3] = buffer[24 + 0];
                            map2[0xC5] = buffer[24 + 3];
                            map2[0xC7] = buffer[24 + 1];
                            map2[0xE1] = buffer[24 + 2];
                            map2[0xE3] = buffer[24 + 0];
                            map2[0xE5] = buffer[24 + 3];
                            map2[0xE7] = buffer[24 + 1];

                            for (int32_t i = 0; i < 2; ++i) {
                                ((uint32_t *)dst)[0] = (map2[map1[48 + i * 8 + 0]] << 16) |
                                                       (map2[map1[48 + i * 8 + 1]] << 24) |
                                                       (map2[map1[48 + i * 8 + 2]]) | (map2[map1[48 + i * 8 + 3]] << 8);
                                dst += nf_width;

                                ((uint32_t *)dst)[0] = (map2[map1[48 + i * 8 + 4]] << 16) |
                                                       (map2[map1[48 + i * 8 + 5]] << 24) |
                                                       (map2[map1[48 + i * 8 + 6]]) | (map2[map1[48 + i * 8 + 7]] << 8);
                                dst += nf_width;
                            }

                            dst -= nf_width;

                            buffer += 32;
                            dst -= 4;
                            dst -= nf_back_right - 8;
                        }
                    } break;

                    case 11: {
                        for (int32_t i = 0; i < 8; ++i) {
                            ((uint32_t *)dst)[0] = ((uint32_t *)buffer)[i * 2];
                            ((uint32_t *)dst)[1] = ((uint32_t *)buffer)[i * 2 + 1];
                            dst += nf_width;
                        }

                        dst -= nf_width;

                        buffer += 64;
                        dst -= nf_back_right - 8;
                    } break;

                    case 12: {
                        value2 = nf_width;

                        for (int32_t i = 0; i < 4; ++i) {
                            byte = buffer[i * 4 + 0];
                            value1 = byte | (byte << 8);

                            byte = buffer[i * 4 + 1];
                            value1 |= (byte << 16) | (byte << 24);

                            byte = buffer[i * 4 + 2];
                            value2 = byte | (byte << 8);

                            byte = buffer[i * 4 + 3];
                            value2 |= (byte << 16) | (byte << 24);

                            ((uint32_t *)dst)[0] = value1;
                            ((uint32_t *)dst)[1] = value2;
                            dst += nf_width;

                            ((uint32_t *)dst)[0] = value1;
                            ((uint32_t *)dst)[1] = value2;
                            dst += nf_width;
                        }

                        dst -= nf_width;
                        buffer += 16;
                        dst -= nf_back_right - 8;
                    } break;

                    case 13:
                        byte = buffer[0];
                        value1 = byte | (byte << 8) | (byte << 16) | (byte << 24);

                        byte = buffer[1];
                        value2 = byte | (byte << 8) | (byte << 16) | (byte << 24);

                        for (int32_t i = 0; i < 2; ++i) {
                            ((uint32_t *)dst)[0] = value1;
                            ((uint32_t *)dst)[1] = value2;
                            dst += nf_width;

                            ((uint32_t *)dst)[0] = value1;
                            ((uint32_t *)dst)[1] = value2;
                            dst += nf_width;
                        }

                        byte = buffer[2];
                        value1 = byte | (byte << 8) | (byte << 16) | (byte << 24);

                        byte = buffer[3];
                        value2 = byte | (byte << 8) | (byte << 16) | (byte << 24);

                        for (int32_t i = 0; i < 2; ++i) {
                            ((uint32_t *)dst)[0] = value1;
                            ((uint32_t *)dst)[1] = value2;
                            dst += nf_width;

                            ((uint32_t *)dst)[0] = value1;
                            ((uint32_t *)dst)[1] = value2;
                            dst += nf_width;
                        }

                        dst -= nf_width;
                        buffer += 4;
                        dst -= nf_back_right - 8;
                        break;

                    case 14:
                    case 15: {
                        if (code_type == 14) {
                            byte = *buffer++;
                            value1 = byte | (byte << 8) | (byte << 16) | (byte << 24);
                            value2 = value1;

                        } else {
                            byte = *(uint16_t *)buffer;
                            buffer += 2;
                            value1 = byte | (byte << 16);
                            value2 = value1;
                            value2 = (value2 << 8) | (value2 >> (32 - 8));
                        }

                        for (int32_t i = 0; i < 4; ++i) {
                            ((uint32_t *)dst)[0] = value1;
                            ((uint32_t *)dst)[1] = value1;
                            dst += nf_width;

                            ((uint32_t *)dst)[0] = value2;
                            ((uint32_t *)dst)[1] = value2;
                            dst += nf_width;
                        }

                        dst -= nf_width;

                        dst -= nf_back_right - 8;
                    } break;
                }
            }
        }

        dst += nf_new_row0 - nf_new_w;
    }
}
