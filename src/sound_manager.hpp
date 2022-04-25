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

#ifndef SOUNDMGR_HPP
#define SOUNDMGR_HPP

#include <SDL_mixer.h>

#include <list>

#include "unitinfo.hpp"

enum AudioType {
    AUDIO_TYPE_SFX0,
    AUDIO_TYPE_SFX1,
    AUDIO_TYPE_SFX2,
    AUDIO_TYPE_VOICE,
    AUDIO_TYPE_MUSIC,
};

class SoundMgr {
public:
    typedef enum {
        SFX_TYPE_INVALID,
        SFX_TYPE_IDLE,
        SFX_TYPE_WATER_IDLE,
        SFX_TYPE_DRIVE,
        SFX_TYPE_WATER_DRIVE,
        SFX_TYPE_STOP,
        SFX_TYPE_WATER_STOP,
        SFX_TYPE_TRANSFORM,
        SFX_TYPE_BUILDING,
        SFX_TYPE_SHRINK,
        SFX_TYPE_EXPAND,
        SFX_TYPE_TURRET,
        SFX_TYPE_FIRE,
        SFX_TYPE_HIT,
        SFX_TYPE_EXPLOAD,
        SFX_TYPE_POWER_CONSUMPTION_START,
        SFX_TYPE_POWER_CONSUMPTION_END,
        SFX_TYPE_LAND,
        SFX_TYPE_TAKE,
        SFX_TYPE_LIMIT
    } SFX_TYPE;

    SoundMgr();
    ~SoundMgr();
    void Init();
    void Deinit();

    void PlayMusic(ResourceID id, bool shuffle);
    void HaltMusicPlayback(bool disable);
    void FreeMusic();

    void PlaySfx(ResourceID id);
    void PlaySfx(UnitInfo* unit, SFX_TYPE sound, bool mode = false);
    void UpdateSfxPosition();
    void UpdateSfxPosition(UnitInfo* unit);
    void UpdateAllSfxPositions();
    void HaltSfxPlayback(bool disable);

    void PlayVoice(ResourceID id1, ResourceID id2, short priority = 0);
    void HaltVoicePlayback(bool disable);

    void FreeAllSamples();
    void SetVolume(int type, int volume);

    void BkProcess();

private:
    typedef enum { JOB_TYPE_SFX0, JOB_TYPE_SFX1, JOB_TYPE_SFX2, JOB_TYPE_VOICE, JOB_TYPE_MUSIC } JOB_TYPE;

    typedef struct {
        int volume;
        char flags;
    } SoundVolume;

    typedef struct {
        ResourceID id;
        JOB_TYPE type;
        unsigned int volume_1;
        unsigned int volume_2;
        unsigned short panning;
        int loop_count;
        short grid_x;
        short grid_y;
        short priority;
        SFX_TYPE sound;
        unsigned short unit_id;
    } SoundJob;

    typedef struct {
        ResourceID id;
        JOB_TYPE type;
        unsigned int volume_1;
        unsigned int volume_2;
        int loop_count;
        short grid_x;
        short grid_y;
        short priority;
        unsigned int time_stamp;
        unsigned int loop_point_start;
        int loop_point_length;

        int mixer_channel;
        Mix_Chunk* chunk;
        Mix_Music* music;
    } SoundSample;

    bool is_audio_enabled;

    SoundVolume* volumes;

    ResourceID current_music_played;
    ResourceID last_music_played;

    bool shuffle_music;
    bool shuffle_music_playlist[BKG9_MSC - MAIN_MSC + 1];

    ResourceID voice_played;

    std::list<SoundJob> jobs;

    std::list<SoundSample> samples;
    int mixer_channels_count;
    SoundSample* music;
    SoundSample* voice;
    SoundSample* sfx;

    void AddJob(SoundJob& job);
    int ProcessJob(SoundJob& job);
    void FreeSample(SoundSample* sample);
    void UpdateMusic();
    void FreeSfx(UnitInfo* unit);
    void FreeVoice(ResourceID id1, ResourceID id2);
    bool IsVoiceGroupScheduled(ResourceID id1, ResourceID id2);
    static int GetPanning(int distance, bool reverse);
    bool LoadMusic(ResourceID id);
    int LoadSound(SoundJob& job, SoundSample& sample);
    void LoadLoopPoints(FILE* fp, SoundSample& sample);
};

extern SoundMgr soundmgr;

#endif /* SOUNDMGR_HPP */
