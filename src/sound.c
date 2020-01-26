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

#include "sound.h"

enum { SOS_SAMPLE_PROCESSED, SOS_SAMPLE_LOOPING, SOS_SAMPLE_DONE };

struct SosHardware_s {
    unsigned int wPort;
    unsigned int wIRQ;
    unsigned int wDMA;
    unsigned int wParam;
};

struct SosInitDriver_s {
    unsigned int wBufferSize;
    unsigned int lpBuffer;
    unsigned int farPointerPlaceholder1;
    unsigned int wAllocateBuffer;
    unsigned int wSampleRate;
    unsigned int wParam;
    unsigned int dwParam;
    void (*lpFillHandler)(void);
    /* further stuff */
};

struct SosSoundSample_s {
    unsigned char* lpSamplePtr;
    unsigned int farPointerPlaceholder1;
    unsigned int dwSampleSize;
    unsigned int wLoopCount;
    unsigned int wChannel;
    unsigned int wVolume;
    unsigned int wSampleID;
    void (*lpCallback)(unsigned int, unsigned int, unsigned int);
    unsigned int farPointerPlaceholder2;
    unsigned int wSamplePort;
    unsigned int wSampleFlags;
    unsigned int dwSampleByteLength;
    unsigned int dwSampleLoopPoint;
    unsigned int dwSampleLoopLength;
    unsigned int dwSamplePitchAdd;
    unsigned int wSamplePitchFraction;
    unsigned int wSamplePanLocation;
    unsigned int wSamplePanSpeed;
    unsigned int wSamplePanDirection;
    unsigned int wSamplePanStart;
    unsigned int wSamplePanEnd;
    unsigned int wSampleDelayBytes;
    unsigned int wSampleDelayRepeat;
    unsigned int dwSampleADPCMPredicted;
    unsigned int wSampleADPCMIndex;
    unsigned int wSampleRootNoteMIDI;
    unsigned int dwSampleTemp1;
    unsigned int dwSampleTemp2;
    unsigned int dwSampleTemp3;
};

unsigned int sosTIMERInitSystem(unsigned int wTimerRate, unsigned int wDebug);
unsigned int sosTIMERRegisterEvent(unsigned int wCallRate, void (*lpTimerEvent)(void), unsigned int* lpTimerHandle);
unsigned int sosTIMERRemoveEvent(unsigned int hEvent);
unsigned int sosTIMERUnInitSystem(unsigned int wTimerRate);

unsigned int sosDIGIDetectInit(char* sDriverPath);
unsigned int sosDIGIDetectFindHardware(unsigned int wDriverId, void /*_SOS_CAPABILITIES*/* pDriver,
                                       unsigned int* pPort);
unsigned int sosDIGIDetectUnInit(void);

unsigned int sosDIGIInitSystem(char* sPath, unsigned int wDebugFlags);
unsigned int sosDIGIUnInitSystem(void);

unsigned int sosDIGIInitDriver(unsigned int driver_id, struct SosHardware_s* sos_hardware_settings,
                               struct SosInitDriver_s* sos_init_driver, unsigned int* hDriver);
unsigned int sosDIGIUnInitDriver(unsigned int hDriver);

unsigned int sosDIGIStartSample(unsigned int hDriver, struct SosSoundSample_s* sSample);
unsigned int sosDIGIStopSample(unsigned int hDriver, unsigned int hSample);
unsigned int sosDIGIContinueSample(unsigned int hDriver, unsigned int hSample, struct SosSoundSample_s* sSample);
unsigned int sosDIGISampleDone(unsigned int hDriver, unsigned int hSample);

unsigned int sosDIGISetSampleVolume(unsigned int hDriver, unsigned int hSample, unsigned int wVolume);
unsigned int sosDIGIGetPanLocation(unsigned int hDriver, unsigned int hSample);
unsigned int sosDIGISetPanLocation(unsigned int hDriver, unsigned int hSample, unsigned int wLocation);
static void sound_callback(void* userdata, Uint8* stream, int len);

static struct sound_s { SDL_AudioDeviceID audio_device_id; } sound_object;

unsigned int sosTIMERInitSystem(unsigned int wTimerRate, unsigned int wDebug) { return 0; }
unsigned int sosTIMERRegisterEvent(unsigned int wCallRate, void (*lpTimerEvent)(void), unsigned int* lpTimerHandle) {
    return 0;
}
unsigned int sosTIMERRemoveEvent(unsigned int hEvent) { return 0; }
unsigned int sosTIMERUnInitSystem(unsigned int wTimerRate) { return 0; }
unsigned int sosDIGIDetectInit(char* sDriverPath) { return 0; }
unsigned int sosDIGIDetectFindHardware(unsigned int wDriverId, void /*_SOS_CAPABILITIES*/* pDriver,
                                       unsigned int* pPort) {
    return 0;
}
unsigned int sosDIGIDetectUnInit(void) { return 0; }
unsigned int sosDIGIInitSystem(char* sPath, unsigned int wDebugFlags) { return 0; }
unsigned int sosDIGIUnInitSystem(void) { return 0; }

unsigned int sosDIGIInitDriver(unsigned int driver_id, struct SosHardware_s* sos_hardware_settings,
                               struct SosInitDriver_s* sos_init_driver, unsigned int* hDriver) {
    SDL_assert(hDriver != NULL);
    SDL_assert(sos_hardware_settings != NULL);
    SDL_assert(sos_init_driver != NULL);

    {
        SDL_AudioSpec audio_spec;

        SDL_zero(audio_spec);

        audio_spec.freq = sos_init_driver->wSampleRate;
        audio_spec.format = AUDIO_S16SYS;
        audio_spec.userdata = &audio_spec;
        audio_spec.channels = 1;
        // audio_spec.callback = &sound_callback;

        if ((sound_object.audio_device_id = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0)) == 0) {
            SDL_Log("SDL_CreateWindow: %s\n", SDL_GetError());
        }

        *hDriver = sound_object.audio_device_id;
    }

    return 0;
}

unsigned int sosDIGIUnInitDriver(unsigned int hDriver) {
    SDL_assert(sound_object.audio_device_id == hDriver);

    /// \todo : this hangs the game on quit    SDL_CloseAudioDevice(sound_object.audio_device_id);

    return 0;
}

unsigned int sosDIGIStartSample(unsigned int hDriver, struct SosSoundSample_s* sSample) {
    SDL_assert(sSample);

    if (SDL_QueueAudio(hDriver, sSample->lpSamplePtr, sSample->dwSampleSize) != 0) {
        SDL_Log("sosDIGIStartSample: %s\n", SDL_GetError());
    }

    SDL_PauseAudioDevice(hDriver, 0);

    return 0;
}

unsigned int sosDIGIStopSample(unsigned int hDriver, unsigned int hSample) {
    SDL_PauseAudioDevice(hDriver, 1);

    return 0;
}

unsigned int sosDIGIContinueSample(unsigned int hDriver, unsigned int hSample, struct SosSoundSample_s* sSample) {
    return 0;
}

unsigned int sosDIGISampleDone(unsigned int hDriver, unsigned int hSample) { return 1; }

unsigned int sosDIGISetSampleVolume(unsigned int hDriver, unsigned int hSample, unsigned int wVolume) { return 0; }

unsigned int sosDIGIGetPanLocation(unsigned int hDriver, unsigned int hSample) { return 0x8000; }

unsigned int sosDIGISetPanLocation(unsigned int hDriver, unsigned int hSample, unsigned int wLocation) {
    return 0x8000;
}

static void sound_callback(void* userdata, Uint8* stream, int len) {
    SDL_assert(stream);
    SDL_assert(len);
    SDL_assert(userdata);

    struct sound_s* s = userdata;

    //    void TSoundStreamCallback( WORD wDriverHandle, WORD wAction, WORD wSampleHndl );
}
