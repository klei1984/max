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

extern "C" {
#include "resrcmgr.h"
}

enum { LEFT_CHANNEL, RIGHT_CHANNEL, CENTER_CHANNEL, INTERLEAVED };
typedef enum { SOUND_TYPE_SFX0, SOUND_TYPE_SFX1, SOUND_TYPE_SFX2, SOUND_TYPE_VOICE, SOUND_TYPE_MUSIC } SOUND_TYPE;

class SoundMgr {
public:
    SoundMgr();
    ~SoundMgr();
    void Init();
    void Deinit();

    void FreeChunk(Mix_Chunk* chunk);
    void FreeAllChunks();

    void PlayMusic(GAME_RESOURCE id, bool shuffle);
    void DisableEnableMusic(bool disable);
    void FreeMusic();

    void PlaySfx(GAME_RESOURCE id);
    void DisableEnableSfx(bool disable);

    void PlayVoice(GAME_RESOURCE id1, GAME_RESOURCE id2, short priority);
    void DisableEnableVoice(bool disable);
    void FreeVoice(GAME_RESOURCE id1, GAME_RESOURCE id2);

    void SetVolume(int type, int volume);
    void BkProcess();

private:
    typedef struct {
        int volume;
        char flags;
    } SoundVolume;

    typedef struct {
        GAME_RESOURCE id;
        SOUND_TYPE type;
        unsigned int volume_1;
        unsigned int volume_2;
        unsigned short panning;
        int loop_count;
        short priority;
    } SoundSample;

    typedef struct {
        Mix_Music* chunk;
    } MusicChunk;

    typedef struct {
        Mix_Chunk* chunk;
    } VoiceChunk;

    bool is_audio_enabled;

    SoundVolume* volumes;

    GAME_RESOURCE current_music_played;
    GAME_RESOURCE last_music_played;

    bool shuffle_music;
    bool shuffle_music_playlist[BKG9_MSC - MAIN_MSC + 1];

    GAME_RESOURCE last_voice_played;

    std::list<SoundSample> samples;

    MusicChunk music_chunk;
    VoiceChunk voice_chunk;

    void AddSample(SoundSample& sample);
    void UpdateMusic(SoundSample& sample);
    void UpdateSfx(SoundSample& sample);
    void UpdateVoice(SoundSample& sample);
    bool IsVoiceGroupScheduled(GAME_RESOURCE id1, GAME_RESOURCE id2);
};

extern SoundMgr soundmgr;

#endif /* SOUNDMGR_HPP */
