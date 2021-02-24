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

#include <new>

extern "C" {
#include "game.h"
}

#include "inifile.hpp"

#define SOUNDMGR_MAX_SAMPLES 20
#define SOUNDMGR_CHUNK_SIZE 8192

SoundMgr soundmgr;

static const short soundmgr_voice_priority[V_END - V_START + 1] = {
    0,  10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 35, 35, 30, 30, 40, 40, 40, 40, 0,  0,  45, 45, 45, 45, 10,
    10, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 50,
    50, 50, 50, 0,  50, 50, 50, 50, 50, 50, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  50, 50, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  40, 40, 40, 40, 0,  0,  0,  0,  30, 30, 30, 30, 0,  5,  5,  5,  5,  60, 60, 60, 60, 60, 60, 60, 60,
    60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 0,  0,  5,  5,  5,  5,  5,  5,  0,  0,  0,  0,
    0,  0,  10, 10, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 5,  5,  5,  5,  5,  5,  5,  5,
    5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

static void Soundmgr_bk_process(void) { soundmgr.BkProcess(); }

SoundMgr::SoundMgr() {
    is_audio_enabled = false;

    volumes = NULL;

    last_music_played = INVALID_ID;
    current_music_played = INVALID_ID;

    shuffle_music = false;
    SDL_zero(shuffle_music_playlist);

    last_voice_played = INVALID_ID;

    music_chunk.chunk = NULL;
    voice_chunk.chunk = NULL;
}

SoundMgr::~SoundMgr() { Deinit(); }

void SoundMgr::Init() {
    /// \todo MVE_SOS_sndInit(-1);

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, SOUNDMGR_CHUNK_SIZE) == -1) {
        SDL_Log("Unable to initialize SDL audio: %s\n", SDL_GetError());
        exit(1);
    }

    IniSoundVolumes ini_soundvol;

    volumes = new (std::nothrow) SoundVolume[FXS_END - FXS_STRT - 1];

    SDL_assert(volumes);

    for (int i = 0; i < (FXS_END - FXS_STRT - 1); i++) {
        volumes[i].volume = ini_soundvol.GetUnitVolume((GAME_RESOURCE)(FXS_STRT + i + 1));
        volumes[i].flags = -1;
    }

    int channel_count = Mix_AllocateChannels(SOUNDMGR_MAX_SAMPLES);
    if (channel_count != SOUNDMGR_MAX_SAMPLES) {
        SDL_Log("Only allocated %i sound mixing channels instead of %i", channel_count, SOUNDMGR_MAX_SAMPLES);
    }

    samples.clear();

    add_bk_process(Soundmgr_bk_process);
    enable_bk();

    is_audio_enabled = true;
}

void SoundMgr::Deinit() {
    is_audio_enabled = false;

    remove_bk_process(Soundmgr_bk_process);

    samples.clear();

    Mix_AllocateChannels(0);

    if (volumes) {
        delete[] volumes;
        volumes = NULL;
    }

    while (Mix_QuerySpec(NULL, NULL, NULL)) {
        Mix_CloseAudio();
    }
}

void SoundMgr::BkProcess() {
    if (is_audio_enabled) {
        for (auto it = samples.begin(); it != samples.end(); it++) {
            switch (it->type) {
                case SOUND_TYPE_SFX0:
                case SOUND_TYPE_SFX1:
                case SOUND_TYPE_SFX2:
                    UpdateSfx(*it);
                    break;
                case SOUND_TYPE_VOICE:
                    UpdateVoice(*it);
                    break;
                case SOUND_TYPE_MUSIC:
                    UpdateMusic(*it);
                    break;
                default:
                    SDL_assert(0);
                    break;
            }
        }
    }
}

void SoundMgr::UpdateMusic(SoundSample& sample) {
    Mix_Music* music;
    char* file;

    if (sample.type == SOUND_TYPE_MUSIC && current_music_played == INVALID_ID) {
        file = (char*)read_game_resource(sample.id);
        music = Mix_LoadMUS(file);
        free(file);
        Mix_PlayMusic(music, 1);
        current_music_played = sample.id;
    }
}

void SoundMgr::UpdateSfx(SoundSample& sample) {}

void SoundMgr::UpdateVoice(SoundSample& sample) {}

void SoundMgr::FreeChunk(Mix_Chunk* chunk) {
    if (chunk) {
        Mix_FreeChunk(chunk);
    }
}

void SoundMgr::FreeAllChunks() {
    FreeMusic();

    for (int i = 0; i < SOUNDMGR_MAX_SAMPLES; i++) {
        /// \todo free
    }
}

void SoundMgr::PlayMusic(GAME_RESOURCE id, bool shuffle) {
    if ((id != INVALID_ID) && (id != current_music_played)) {
        if (ini_get_setting(ini_disable_music)) {
            last_music_played = id;
        } else {
            FreeMusic();

            shuffle_music = shuffle;

            if ((shuffle_music) && ((id < MAIN_MSC) || (id > BKG9_MSC))) {
                for (int i = 0; i < (BKG9_MSC - MAIN_MSC + 1); i++) {
                    shuffle_music_playlist[i] = true;
                }
            }

            SoundSample sample;
            SDL_zero(sample);

            sample.type = SOUND_TYPE_MUSIC;
            sample.id = id;
            sample.priority = 0;
            sample.volume_1 = 0x7FFF;
            sample.volume_2 = 0x7FFF;
            sample.panning = 0x8000;
            sample.loop_count = 0;
            /// \todo Additional parameters

            AddSample(sample);
        }
    }
}

void SoundMgr::PlaySfx(GAME_RESOURCE id) {
    if (!ini_get_setting(ini_disable_fx)) {
        SoundSample sample;
        SDL_zero(sample);

        sample.id = id;
        sample.type = SOUND_TYPE_SFX2;

        AddSample(sample);
        BkProcess();
    }
}

void SoundMgr::DisableEnableMusic(bool disable) {
    if (disable) {
        last_music_played = current_music_played;
        FreeMusic();
    } else {
        PlayMusic(last_music_played, shuffle_music);
    }
}

void SoundMgr::DisableEnableSfx(bool disable) {
    if (disable) {
        for (;;) {
        }
    } else {
        /// \todo
    }
}

void SoundMgr::PlayVoice(GAME_RESOURCE id1, GAME_RESOURCE id2, short priority) {
    if (priority >= 0) {
        if (IsVoiceGroupScheduled(id1, id2) && !ini_get_setting(ini_disable_voice)) {
            short priority_value;
            int randomized_voice_id;

            if (priority > 0) {
                priority_value = priority;
            } else {
                priority_value = soundmgr_voice_priority[id1 - V_START];
            }

            randomized_voice_id = 2 * ((((id2 - id1) >> 1 + 1) * rand()) >> 15) + 1 + id1;
            SDL_assert(randomized_voice_id != id1 && randomized_voice_id <= id2);

            for (auto it = samples.begin(); it != samples.end(); it++) {
                if ((it->type == SOUND_TYPE_VOICE) && (priority_value >= it->priority)) {
                    it->id = (GAME_RESOURCE)randomized_voice_id;
                    it->priority = priority_value;

                    return;
                }
            }

            SoundSample sample;
            SDL_zero(sample);

            sample.type = SOUND_TYPE_VOICE;
            sample.id = (GAME_RESOURCE)randomized_voice_id;
            sample.priority = priority_value;
            sample.volume_1 = 0x7FFF;
            sample.volume_2 = 0x7FFF;
            sample.panning = 0x8000;
            sample.loop_count = 0;
            /// \todo Additional parameters

            AddSample(sample);
        }
    } else {
        FreeVoice(id1, id2);
    }
}

void SoundMgr::DisableEnableVoice(bool disable) {
    if ((disable) && (voice_chunk.chunk)) {
        FreeChunk(voice_chunk.chunk);
        voice_chunk.chunk = NULL;
        last_voice_played = INVALID_ID;
    }
}

void SoundMgr::FreeVoice(GAME_RESOURCE id1, GAME_RESOURCE id2) {}

void SoundMgr::FreeMusic() {
    if (music_chunk.chunk) {
        current_music_played = INVALID_ID;
        Mix_FreeMusic(music_chunk.chunk);
        music_chunk.chunk = NULL;
    }
}

void SoundMgr::SetVolume(int type, int volume) {}

void SoundMgr::AddSample(SoundSample& sample) {
    if ((is_audio_enabled) && (sample.id != INVALID_ID) && (samples.size() < SOUNDMGR_MAX_SAMPLES)) {
        if (sample.type <= SOUND_TYPE_SFX2) {
            for (auto it = samples.begin(); it != samples.end(); it++) {
                if (it->id == sample.id) {
                    if (sample.volume_1 > it->volume_1) {
                        it->volume_1 = sample.volume_1;
                    }

                    return;
                }

                if ((it->type == SOUND_TYPE_SFX1) && (sample.type == SOUND_TYPE_SFX1)) {
                    *it = sample;

                    return;
                }
            }

            if (sample.type == SOUND_TYPE_SFX2) {
                /// \todo return if sample is being played
            }
        }

        if (sample.priority > 0) {
            for (auto it = samples.begin(); it != samples.end(); it++) {
                if (sample.priority > it->priority) {
                    samples.insert(it, sample);

                    return;
                }
            }
        }

        samples.push_back(sample);
    }
}

bool SoundMgr::IsVoiceGroupScheduled(GAME_RESOURCE id1, GAME_RESOURCE id2) { return false; }
