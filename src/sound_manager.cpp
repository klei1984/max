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

#include "sound_manager.hpp"

#include <cstring>
#include <new>

#include "enums.hpp"
#include "game_manager.hpp"
#include "gnw.h"
#include "inifile.hpp"
#include "resource_manager.hpp"

#define SOUNDMGR_MAX_SAMPLES 20
#define SOUNDMGR_CHUNK_SIZE 4096
#define SOUNDMGR_INVALID_CHANNEL -1
#define SOUNDMGR_MAX_VALUE 0x7FFFu
#define SOUNDMGR_PANNING_LEFT 0x0000u
#define SOUNDMGR_PANNING_CENTER 0x8000u
#define SOUNDMGR_PANNING_RIGHT 0xFFFFu

#define SOUNDMGR_SFX_FLAG_INVALID 0
#define SOUNDMGR_SFX_FLAG_UNKNOWN_1 1
#define SOUNDMGR_SFX_FLAG_UNKNOWN_2 2
#define SOUNDMGR_SFX_FLAG_INFINITE_LOOPING 4

#define SOUNDMGR_SCALE_VOLUME(volume) (((volume)*MIX_MAX_VOLUME) / SOUNDMGR_MAX_VALUE)
#define SOUNDMGR_SCALE_PANNING_RIGHT(panning) (((panning)*254u) / SOUNDMGR_PANNING_RIGHT)

CSoundManager SoundManager;

static const unsigned char soundmgr_sfx_type_flags[SFX_TYPE_LIMIT] = {
    SOUNDMGR_SFX_FLAG_INVALID,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2 | SOUNDMGR_SFX_FLAG_INFINITE_LOOPING,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2 | SOUNDMGR_SFX_FLAG_INFINITE_LOOPING,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2 | SOUNDMGR_SFX_FLAG_INFINITE_LOOPING,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2 | SOUNDMGR_SFX_FLAG_INFINITE_LOOPING,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2 | SOUNDMGR_SFX_FLAG_INFINITE_LOOPING,
    SOUNDMGR_SFX_FLAG_UNKNOWN_1,
    SOUNDMGR_SFX_FLAG_UNKNOWN_1,
    SOUNDMGR_SFX_FLAG_UNKNOWN_1,
    SOUNDMGR_SFX_FLAG_UNKNOWN_1,
    SOUNDMGR_SFX_FLAG_UNKNOWN_1,
    SOUNDMGR_SFX_FLAG_UNKNOWN_1,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2,
    SOUNDMGR_SFX_FLAG_UNKNOWN_2};

static const short soundmgr_voice_priority[V_END - V_START + 1] = {
    0,  10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 35, 35, 30, 30, 40, 40, 40, 40, 0,  0,  45, 45, 45, 45, 10,
    10, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 50,
    50, 50, 50, 0,  50, 50, 50, 50, 50, 50, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  50, 50, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  40, 40, 40, 40, 0,  0,  0,  0,  30, 30, 30, 30, 0,  5,  5,  5,  5,  60, 60, 60, 60, 60, 60, 60, 60,
    60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 0,  0,  5,  5,  5,  5,  5,  5,  0,  0,  0,  0,
    0,  0,  10, 10, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 5,  5,  5,  5,  5,  5,  5,  5,
    5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

static void Soundmgr_bk_process(void) { SoundManager.BkProcess(); }

CSoundManager::CSoundManager() {
    is_audio_enabled = false;

    mixer_channels_count = 0;

    volumes = nullptr;

    last_music_played = INVALID_ID;
    current_music_played = INVALID_ID;

    shuffle_music = false;
    SDL_zero(shuffle_music_playlist);

    voice_played = INVALID_ID;

    music = nullptr;
    voice = nullptr;
    sfx = nullptr;
}

CSoundManager::~CSoundManager() { Deinit(); }

void CSoundManager::Init() {
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, SOUNDMGR_CHUNK_SIZE) == -1) {
        SDL_Log("Unable to initialize SDL audio: %s\n", SDL_GetError());
        is_audio_enabled = false;
        /// \todo MVE_SOS_sndInit(-1);
    } else {
        IniSoundVolumes ini_soundvol;

        is_audio_enabled = true;

        /* setup sound effect volume table */
        volumes = new (std::nothrow) SoundVolume[FXS_END - FXS_STRT - 1];

        SDL_assert(volumes);

        for (int i = 0; i < (FXS_END - FXS_STRT - 1); i++) {
            volumes[i].volume = ini_soundvol.GetUnitVolume((ResourceID)(FXS_STRT + i + 1));
            volumes[i].flags = -1;
        }

        /* allocate SOUNDMGR_MAX_SAMPLES number of sound channels */
        mixer_channels_count = Mix_AllocateChannels(SOUNDMGR_MAX_SAMPLES);
        if (mixer_channels_count != SOUNDMGR_MAX_SAMPLES) {
            SDL_Log("Only allocated %i sound mixing channels instead of %i", mixer_channels_count,
                    SOUNDMGR_MAX_SAMPLES);
        }

        samples.clear();
        jobs.clear();

        /// \todo MVE_SOS_sndInit(HANDLE);
        add_bk_process(Soundmgr_bk_process);
        enable_bk();
    }
}

void CSoundManager::Deinit() {
    is_audio_enabled = false;

    samples.clear();
    jobs.clear();

    Mix_AllocateChannels(0);

    if (volumes) {
        delete[] volumes;
        volumes = nullptr;
    }

    while (Mix_QuerySpec(nullptr, nullptr, nullptr)) {
        Mix_CloseAudio();
    }
}

void CSoundManager::UpdateMusic() {
    if (music && !Mix_PlayingMusic()) {
        if (shuffle_music) {
            ResourceID resource_id;
            int index;

            /* if all tracks were played from the list, reset list state */
            for (index = 0; (index < (BKG9_MSC - MAIN_MSC + 1)) && (shuffle_music_playlist[index] != true); index++) {
                ;
            }

            if (index == (BKG9_MSC - MAIN_MSC + 1)) {
                for (index = 0; index < (BKG9_MSC - MAIN_MSC + 1); index++) {
                    shuffle_music_playlist[index] = true;
                }
            }

            for (;;) {
                do {
                    index = (((BKG9_MSC - MAIN_MSC + 1) * dos_rand()) >> 15);
                    resource_id = (ResourceID)(index + MAIN_MSC);
                } while (!shuffle_music_playlist[index]);

                shuffle_music_playlist[index] = false;

                if (LoadMusic(resource_id)) {
                    break;
                }

                for (index = 0; (index < (BKG9_MSC - MAIN_MSC + 1)) && (shuffle_music_playlist[index] != true);
                     index++) {
                    ;
                }

                if (index == (BKG9_MSC - MAIN_MSC + 1)) {
                    SDL_assert(music->music);

                    Mix_PlayMusic(music->music, 1);
                    return;
                }
            }
        } else {
            SDL_assert(music->music);

            Mix_PlayMusic(music->music, 1);
        }
    }
}

void CSoundManager::FreeSfx(UnitInfo* unit) {
    unsigned short unit_id;

    if (sfx) {
        sfx->time_stamp = timer_get_stamp32();
    }

    sfx = nullptr;
    unit->sound = SFX_TYPE_INVALID;
    unit_id = unit->GetId();

    for (auto it = jobs.begin(); it != jobs.end();) {
        if (it->unit_id == unit_id && it->type == JOB_TYPE_SFX0) {
            it = jobs.erase(it);
        } else {
            it++;
        }
    }
}

void CSoundManager::FreeSample(SoundSample* sample) {
    if (is_audio_enabled) {
        SDL_assert(sample);

        if (sample->mixer_channel != SOUNDMGR_INVALID_CHANNEL) {
            SDL_assert(sample->mixer_channel < mixer_channels_count);
            if (sample->type == JOB_TYPE_MUSIC) {
                if (music && music->music == sample->music) {
                    music = nullptr;
                }

                if (Mix_PlayingMusic()) {
                    Mix_HaltMusic();
                    current_music_played = INVALID_ID;
                }

                Mix_FreeMusic(sample->music);
                sample->music = nullptr;
                sample->mixer_channel = SOUNDMGR_INVALID_CHANNEL;

            } else {
                /// \todo SDL_assert(Mix_GetChunk(sample->mixer_channel) == sample->chunk);

                if (sfx && sfx->chunk == sample->chunk) {
                    sfx = nullptr;
                }

                if (voice && voice->chunk == sample->chunk) {
                    voice = nullptr;
                    voice_played = INVALID_ID;
                }

                Mix_HaltChannel(sample->mixer_channel);
                Mix_FreeChunk(sample->chunk);
                sample->chunk = nullptr;
                sample->mixer_channel = SOUNDMGR_INVALID_CHANNEL;
            }
        }
    }
}

void CSoundManager::PlayMusic(ResourceID id, bool shuffle) {
    if ((id != INVALID_ID) && (id != current_music_played)) {
        if (ini_get_setting(INI_DISABLE_MUSIC)) {
            last_music_played = id;
        } else {
            FreeMusic();

            shuffle_music = shuffle;

            if ((shuffle_music) && ((id < MAIN_MSC) || (id > BKG9_MSC))) {
                for (int i = 0; i < (BKG9_MSC - MAIN_MSC + 1); i++) {
                    shuffle_music_playlist[i] = true;
                }
            }

            SoundJob job;

            job.id = id;
            job.type = JOB_TYPE_MUSIC;
            job.volume_1 = SOUNDMGR_MAX_VALUE;
            job.volume_2 = SOUNDMGR_MAX_VALUE;
            job.panning = SOUNDMGR_PANNING_CENTER;
            job.loop_count = 0;
            job.grid_x = 0;
            job.grid_y = 0;
            job.priority = 0;
            job.sound = SFX_TYPE_INVALID;
            job.unit_id = -1;

            AddJob(job);
        }
    }
}

void CSoundManager::HaltMusicPlayback(bool disable) {
    if (disable) {
        last_music_played = current_music_played;
        FreeMusic();

    } else {
        PlayMusic(last_music_played, shuffle_music);
    }
}

void CSoundManager::FreeMusic() {
    if (music) {
        FreeSample(music);
        music = nullptr;
    }
}

void CSoundManager::PlaySfx(ResourceID id) {
    if (!ini_get_setting(INI_DISABLE_FX)) {
        SoundJob job;

        job.id = id;
        job.type = JOB_TYPE_SFX2;
        job.volume_1 = SOUNDMGR_MAX_VALUE;
        job.volume_2 = SOUNDMGR_MAX_VALUE;
        job.panning = SOUNDMGR_PANNING_CENTER;
        job.loop_count = 0;
        job.grid_x = 0;
        job.grid_y = 0;
        job.priority = 0;
        job.sound = SFX_TYPE_INVALID;
        job.unit_id = -1;

        AddJob(job);
        BkProcess();
    }
}

void CSoundManager::PlaySfx(UnitInfo* unit, int sound, bool mode) {
    unsigned char flags;
    int previous_sound;

    SDL_assert(unit);
    SDL_assert(sound < SFX_TYPE_LIMIT);

    if (mode) {
        flags = SOUNDMGR_SFX_FLAG_UNKNOWN_1;
    } else {
        flags = soundmgr_sfx_type_flags[sound];
    }

    if (!(flags & SOUNDMGR_SFX_FLAG_UNKNOWN_2) ||
        (previous_sound = unit->sound, unit->sound = sound, previous_sound != sound)) {
        if (SFX_TYPE_INVALID == sound) {
            FreeSfx(unit);
            return;
        }

        if (!ini_get_setting(INI_DISABLE_FX) && is_audio_enabled) {
            int grid_center_x;
            int grid_center_y;
            int grid_offset_x;
            int grid_offset_y;
            int grid_distance_x;
            int grid_distance_y;
            SoundJob job;
            int loop_count;
            int sound_index;
            int volume_index;
            int resource_id;

            for (sound_index = 0;
                 sound_index < unit->sound_table->size() && (*unit->sound_table)[sound_index].type != sound;
                 ++sound_index) {
                ;
            }

            if (sound_index == unit->sound_table->size()) {
                if (soundmgr_sfx_type_flags[sound] != SOUNDMGR_SFX_FLAG_UNKNOWN_1) {
                    FreeSfx(unit);
                    return;
                }

                resource_id = sound + FXS_STRT;
            } else {
                resource_id = (*unit->sound_table)[sound_index].resource_id;
            }

            if (sound >= SFX_TYPE_IDLE && sound <= SFX_TYPE_STOP &&
                (unit->flags & (MOBILE_LAND_UNIT | MOBILE_SEA_UNIT)) == (MOBILE_LAND_UNIT | MOBILE_SEA_UNIT) &&
                unit->image_base == 8) {
                resource_id++;
            }

            volume_index = resource_id - GEN_IDLE;

            if (volumes[resource_id - GEN_IDLE].flags == -1) {
                char* filename;
                volumes[volume_index].flags = 1;

                filename = reinterpret_cast<char*>(ResourceManager_ReadResource(static_cast<ResourceID>(resource_id)));

                if (filename) {
                    FILE* fp;

                    ResourceManager_ToUpperCase(filename);
                    fp = fopen(filename, "rb");

                    if (fp) {
                        volumes[volume_index].flags = 0;
                        fclose(fp);
                    }

                    delete[] filename;
                }
            }

            if (volumes[volume_index].flags == 1) {
                resource_id = sound + FXS_STRT;
                volume_index = sound - 1;
            }

            job.type = (JOB_TYPE)((flags & SOUNDMGR_SFX_FLAG_UNKNOWN_2) != 0);
            job.id = (ResourceID)resource_id;

            job.grid_x = unit->grid_x;
            job.grid_y = unit->grid_y;

            if (flags & SOUNDMGR_SFX_FLAG_INFINITE_LOOPING) {
                loop_count = -1;
            } else {
                loop_count = 0;
            }

            job.loop_count = loop_count;
            job.sound = sound;

            grid_center_x = (GameManager_GridPosition.ulx + GameManager_GridPosition.lrx) / 2;
            grid_center_y = (GameManager_GridPosition.uly + GameManager_GridPosition.lry) / 2;

            grid_offset_x = job.grid_x - grid_center_x;
            grid_offset_y = job.grid_y - grid_center_y;

            grid_distance_x = labs(grid_offset_x);
            grid_distance_y = labs(grid_offset_y);

            job.volume_2 = volumes[volume_index].volume;
            job.volume_1 = job.volume_2 - job.volume_2 * std::max(grid_distance_x, grid_distance_y) / 112;

            job.panning = GetPanning(grid_distance_x, (job.grid_x - grid_center_x) < 0);

            job.priority = 0;
            job.unit_id = unit->GetId();

            AddJob(job);
        }
    }
}

void CSoundManager::UpdateSfxPosition() {
    int grid_center_x;
    int grid_center_y;
    int grid_offset_x;
    int grid_offset_y;
    int grid_distance_x;
    int grid_distance_y;

    int pan_location;
    int sound_level;

    grid_center_x = (GameManager_GridPosition.ulx + GameManager_GridPosition.lrx) / 2;
    grid_center_y = (GameManager_GridPosition.uly + GameManager_GridPosition.lry) / 2;

    for (auto it = samples.begin(); it != samples.end(); it++) {
        if (it->mixer_channel != SOUNDMGR_INVALID_CHANNEL && it->type <= JOB_TYPE_SFX1) {
            grid_offset_x = it->grid_x - grid_center_x;
            grid_offset_y = it->grid_y - grid_center_y;

            grid_distance_x = labs(grid_offset_x);
            grid_distance_y = labs(grid_offset_y);

            if (grid_offset_x >= 0) {
                pan_location = GetPanning(grid_distance_x, false);
            } else {
                pan_location = GetPanning(grid_distance_x, true);
            }

            int pan_right = SOUNDMGR_SCALE_PANNING_RIGHT(pan_location);
            int pan_left = 254 - pan_right;

            if (!Mix_SetPanning(it->mixer_channel, pan_left, pan_right)) {
                SDL_Log("SDL_Mixer failed to set stereo pan position: %s\n", Mix_GetError());
            }

            sound_level = it->volume_2 - it->volume_2 * std::max(grid_distance_x, grid_distance_y) / 112;
            sound_level = (ini_get_setting(INI_FX_SOUND_LEVEL) * sound_level) / 100;
            Mix_Volume(it->mixer_channel, SOUNDMGR_SCALE_VOLUME(sound_level));
        }
    }
}

void CSoundManager::UpdateSfxPosition(UnitInfo* unit) {
    int grid_center_x;
    int grid_center_y;
    int grid_offset_x;
    int grid_offset_y;
    int grid_distance_x;
    int grid_distance_y;

    int pan_location;
    int sound_level;

    SDL_assert(unit);

    if (sfx && sfx->mixer_channel != SOUNDMGR_INVALID_CHANNEL) {
        sfx->grid_x = unit->grid_x;
        sfx->grid_y = unit->grid_y;

        grid_center_x = (GameManager_GridPosition.ulx + GameManager_GridPosition.lrx) / 2;
        grid_center_y = (GameManager_GridPosition.uly + GameManager_GridPosition.lry) / 2;

        grid_offset_x = sfx->grid_x - grid_center_x;
        grid_offset_y = sfx->grid_y - grid_center_y;

        grid_distance_x = labs(grid_offset_x);
        grid_distance_y = labs(grid_offset_y);

        if (grid_offset_x >= 0) {
            pan_location = GetPanning(grid_distance_x, false);
        } else {
            pan_location = GetPanning(grid_distance_x, true);
        }

        int pan_right = SOUNDMGR_SCALE_PANNING_RIGHT(pan_location);
        int pan_left = 254 - pan_right;

        if (!Mix_SetPanning(sfx->mixer_channel, pan_left, pan_right)) {
            SDL_Log("SDL_Mixer failed to set stereo pan position: %s\n", Mix_GetError());
        }

        sound_level = sfx->volume_2 - sfx->volume_2 * std::max(grid_distance_x, grid_distance_y) / 112;
        sound_level = (ini_get_setting(INI_FX_SOUND_LEVEL) * sound_level) / 100;
        Mix_Volume(sfx->mixer_channel, SOUNDMGR_SCALE_VOLUME(sound_level));
    }
}

void CSoundManager::UpdateAllSfxPositions() {}

void CSoundManager::HaltSfxPlayback(bool disable) {
    if (disable) {
        for (auto it = samples.begin(); it != samples.end();) {
            if (it->mixer_channel != SOUNDMGR_INVALID_CHANNEL && it->type <= JOB_TYPE_SFX2) {
                FreeSample(&(*it));
                it = samples.erase(it);
            } else {
                it++;
            }
        }

    } else if (GameManager_SelectedUnit != nullptr) {
        PlaySfx(&*GameManager_SelectedUnit, SFX_TYPE_INVALID, false);
        PlaySfx(&*GameManager_SelectedUnit, GameManager_SelectedUnit->sound, false);
    }
}

void CSoundManager::PlayVoice(ResourceID id1, ResourceID id2, short priority) {
    if (priority >= 0) {
        if (!IsVoiceGroupScheduled(id1, id2) && !ini_get_setting(INI_DISABLE_VOICE)) {
            short priority_value;
            unsigned short randomized_voice_id;

            if (priority > 0) {
                priority_value = priority;
            } else {
                priority_value = soundmgr_voice_priority[id1 - V_START];
            }

            randomized_voice_id = 2 * ((((id2 - id1) / 2 + 1) * dos_rand()) >> 15) + 1 + id1;
            SDL_assert(randomized_voice_id != id1 && randomized_voice_id <= id2);

            for (auto it = jobs.begin(); it != jobs.end(); it++) {
                if (it->type == JOB_TYPE_VOICE) {
                    if (priority_value >= it->priority) {
                        it->id = (ResourceID)randomized_voice_id;
                        it->priority = priority_value;
                    }

                    return;
                }
            }

            SoundJob job;

            job.id = (ResourceID)randomized_voice_id;
            job.type = JOB_TYPE_VOICE;
            job.volume_1 = SOUNDMGR_MAX_VALUE;
            job.volume_2 = SOUNDMGR_MAX_VALUE;
            job.panning = SOUNDMGR_PANNING_CENTER;
            job.loop_count = 0;
            job.grid_x = 0;
            job.grid_y = 0;
            job.priority = priority_value;
            job.sound = SFX_TYPE_INVALID;
            job.unit_id = -1;

            AddJob(job);
        }
    } else {
        FreeVoice(id1, id2);
    }
}

void CSoundManager::HaltVoicePlayback(bool disable) {
    if (disable) {
        if (voice) {
            FreeSample(voice);
            voice = nullptr;
        }
    }
}

void CSoundManager::FreeAllSamples() {
    FreeMusic();

    for (auto it = samples.begin(); it != samples.end();) {
        FreeSample(&(*it));

        it = samples.erase(it);
    }

    SDL_assert(samples.empty());
}

void CSoundManager::SetVolume(int type, int volume) {
    SDL_assert(type <= JOB_TYPE_MUSIC);
    SDL_assert(volume <= 100);

    for (auto it = samples.begin(); it != samples.end(); it++) {
        if (it->mixer_channel != SOUNDMGR_INVALID_CHANNEL &&
            (it->type == type || (type == JOB_TYPE_SFX2 && it->type <= JOB_TYPE_SFX2))) {
            int new_volume = (it->volume_1 * volume) / 100;

            if (it->type == JOB_TYPE_MUSIC) {
                Mix_VolumeMusic(SOUNDMGR_SCALE_VOLUME(new_volume));
            } else {
                Mix_Volume(it->mixer_channel, SOUNDMGR_SCALE_VOLUME(new_volume));
            }
        }
    }
}

void CSoundManager::BkProcess() {
    if (is_audio_enabled) {
        UpdateMusic();

        for (auto it = jobs.begin(); it != jobs.end();) {
            if (0 == ProcessJob(*it)) {
                it = jobs.erase(it);
            } else {
                it++;
            }
        }

        for (auto it = samples.begin(); it != samples.end();) {
            if (it->type <= JOB_TYPE_SFX2) {
                if (it->mixer_channel != SOUNDMGR_INVALID_CHANNEL && it->time_stamp &&
                    timer_elapsed_time_ms(it->time_stamp) > 125) {
                    it->volume_2 /= 2;

                    if (it->volume_2) {
                        int sfx_volume = ini_get_setting(INI_FX_SOUND_LEVEL);
                        SDL_assert(sfx_volume <= 100);

                        it->volume_1 /= 2;

                        Mix_Volume(it->mixer_channel, SOUNDMGR_SCALE_VOLUME(sfx_volume * it->volume_1 / 100));

                        it->time_stamp = timer_get_stamp32();
                    } else {
                        FreeSample(&(*it));
                        it = samples.erase(it);
                        continue;
                    }
                }
            }

            it++;
        }
    }
}

void CSoundManager::AddJob(SoundJob& job) {
    if ((is_audio_enabled) && (job.id != INVALID_ID) && (jobs.size() < SOUNDMGR_MAX_SAMPLES)) {
        if (job.type <= JOB_TYPE_SFX2) {
            for (auto it = jobs.begin(); it != jobs.end(); it++) {
                if (it->id == job.id) {
                    if (job.volume_1 > it->volume_1) {
                        it->volume_1 = job.volume_1;
                    }

                    return;
                }

                if ((it->type == JOB_TYPE_SFX1) && (job.type == JOB_TYPE_SFX1)) {
                    *it = job;

                    return;
                }
            }

            if (job.type == JOB_TYPE_SFX2) {
                for (auto it = samples.begin(); it != samples.end(); it++) {
                    if (job.id == it->id && it->mixer_channel != SOUNDMGR_INVALID_CHANNEL &&
                        Mix_Playing(it->mixer_channel)) {
                        return;
                    }
                }
            }
        }

        if (job.priority > 0) {
            for (auto it = jobs.begin(); it != jobs.end(); it++) {
                if (job.priority > it->priority) {
                    jobs.insert(it, job);

                    return;
                }
            }
        }

        jobs.push_back(job);
    }
}

int CSoundManager::ProcessJob(SoundJob& job) {
    int result;

    if (job.type == JOB_TYPE_VOICE && voice && voice->mixer_channel != SOUNDMGR_INVALID_CHANNEL &&
        Mix_Playing(voice->mixer_channel)) {
        result = 11;
    } else {
        for (auto it = samples.begin(); it != samples.end();) {
            if (it->mixer_channel == SOUNDMGR_INVALID_CHANNEL ||
                (it->type != JOB_TYPE_MUSIC && !Mix_Playing(it->mixer_channel)) ||
                (it->type == JOB_TYPE_MUSIC && !Mix_PlayingMusic())) {
                FreeSample(&(*it));
                it = samples.erase(it);
            } else {
                it++;
            }
        }

        if (samples.size() < mixer_channels_count) {
            SoundSample sample;
            SDL_zero(sample);

            result = LoadSound(job, sample);

            if (0 == result) {
                int sound_level;

                if (sample.loop_point_start != 0) {
                    if (job.sound == SFX_TYPE_BUILDING) {
                        ;  /// \todo Cut sample part before loop point start position and limit sample size to loop
                           /// point size in case of building type sound
                    } else {
                        ;  /// \todo SDL_mixer does not support loop points for Mix_Chunks
                    }

                    job.loop_count = -1;
                }

                if (job.type == JOB_TYPE_MUSIC) {
                    sound_level = ini_get_setting(INI_MUSIC_LEVEL);
                } else if (job.type == JOB_TYPE_VOICE) {
                    sound_level = ini_get_setting(INI_VOICE_LEVEL);
                } else {
                    sound_level = ini_get_setting(INI_FX_SOUND_LEVEL);
                }

                SDL_assert(sound_level <= 100);

                sample.id = job.id;
                sample.type = job.type;
                sample.volume_1 = job.volume_1;
                sample.volume_2 = job.volume_2;
                sample.loop_count = job.loop_count;
                sample.grid_x = job.grid_x;
                sample.grid_y = job.grid_y;
                sample.priority = job.priority;
                sample.time_stamp = 0;

                /// \todo Should we set chunk volume instead of channel volume?

                if (JOB_TYPE_MUSIC == sample.type) {
                    sample.mixer_channel = 0;
                    Mix_VolumeMusic(SOUNDMGR_SCALE_VOLUME(sample.volume_1 * sound_level / 100));
                    int local_result = Mix_PlayMusic(sample.music, sample.loop_count);
                    SDL_assert(local_result != -1);
                } else {
                    int pan_right = SOUNDMGR_SCALE_PANNING_RIGHT(job.panning);
                    int pan_left = 254 - pan_right;

                    if (!Mix_SetPanning(sample.mixer_channel, pan_left, pan_right)) {
                        SDL_Log("SDL_Mixer failed to set stereo pan position: %s\n", Mix_GetError());
                    }

                    {
                        int i = 0;

                        while (Mix_Playing(i)) {
                            i++;
                        }

                        SDL_assert(i < mixer_channels_count);
                        sample.mixer_channel = i;
                    }

                    Mix_Volume(sample.mixer_channel, SOUNDMGR_SCALE_VOLUME(sample.volume_1 * sound_level / 100));
                    int channel = Mix_PlayChannel(sample.mixer_channel, sample.chunk, sample.loop_count);
                    SDL_assert(channel == sample.mixer_channel);
                }

                samples.push_back(sample);

                if (job.type <= JOB_TYPE_SFX2) {
                    if (job.type == JOB_TYPE_SFX1) {
                        if (sfx && sfx->chunk != sample.chunk) {
                            sfx->time_stamp = timer_get_stamp32();
                        }
                        sfx = &samples.back();
                    }
                } else if (job.type == JOB_TYPE_VOICE) {
                    voice = &samples.back();
                    voice_played = job.id;
                } else if (job.type == JOB_TYPE_MUSIC) {
                    music = &samples.back();
                    current_music_played = job.id;
                }
            }
        } else {
            result = 11;
        }
    }

    return result;
}

void CSoundManager::FreeVoice(ResourceID id1, ResourceID id2) {
    if (voice && voice_played >= id1 && voice_played <= id2) {
        FreeSample(voice);
    }

    for (auto it = jobs.begin(); it != jobs.end();) {
        if (it->type == JOB_TYPE_VOICE && it->id >= id1 && it->id <= id2) {
            it = jobs.erase(it);
        } else {
            it++;
        }
    }
}

bool CSoundManager::IsVoiceGroupScheduled(ResourceID id1, ResourceID id2) {
    if (voice_played >= id1 && voice_played <= id2 && voice && Mix_Playing(voice->mixer_channel)) {
        return true;
    }

    for (auto it = jobs.begin(); it != jobs.end(); it++) {
        if (it->type == JOB_TYPE_VOICE && it->id >= id1 && it->id <= id2) {
            return true;
        }
    }

    return false;
}

int CSoundManager::GetPanning(int distance, bool reverse) {
    int panning;

    if (distance > 28) {
        distance = 28;
    }

    if (reverse) {
        distance = -distance;
    }

    panning = (SOUNDMGR_PANNING_RIGHT * (distance + 56)) / 112;

    if (ini_get_setting(INI_CHANNELS_REVERSED)) {
        panning = SOUNDMGR_PANNING_RIGHT - panning;
    }

    return panning;
}

bool CSoundManager::LoadMusic(ResourceID id) {
    Mix_Music* sample;
    char* file;
    char file_path[PATH_MAX];
    bool result = false;

    file = reinterpret_cast<char*>(ResourceManager_ReadResource(id));

    if (file) {
        strncpy(file_path, ResourceManager_FilePathMsc, PATH_MAX - 1);
        strncat(file_path, file, (PATH_MAX - 1) - strlen(file_path));
        delete[] file;

        file_path[PATH_MAX - 1] = '\0';

        sample = Mix_LoadMUS(file_path);
        if (sample) {
            if (music->music) {
                Mix_FreeMusic(music->music);
            }

            music->music = sample;
            current_music_played = id;

            result = true;
        }
    }

    return result;
}

int CSoundManager::LoadSound(SoundJob& job, SoundSample& sample) {
    char* file;
    char file_path[PATH_MAX];
    int result;

    file = reinterpret_cast<char*>(ResourceManager_ReadResource(job.id));

    if (file) {
        if (JOB_TYPE_VOICE == job.type) {
            strncpy(file_path, ResourceManager_FilePathVoiceSpw, PATH_MAX - 1);
        } else if (JOB_TYPE_MUSIC == job.type) {
            strncpy(file_path, ResourceManager_FilePathMsc, PATH_MAX - 1);
        } else {
            strncpy(file_path, ResourceManager_FilePathSfxSpw, PATH_MAX - 1);
        }

        ResourceManager_ToUpperCase(file);

        strncat(file_path, file, (PATH_MAX - 1) - strlen(file_path));
        delete[] file;

        file_path[PATH_MAX - 1] = '\0';

        FILE* fp = fopen(file_path, "rb");

        if (fp) {
            LoadLoopPoints(fp, sample);
            fclose(fp);

            if (JOB_TYPE_MUSIC == job.type) {
                sample.music = Mix_LoadMUS(file_path);
                if (sample.music) {
                    sample.chunk = nullptr;
                    current_music_played = job.id;
                    result = 0;
                } else {
                    sample.chunk = nullptr;
                    current_music_played = INVALID_ID;
                    result = 4;
                }
            } else {
                sample.music = nullptr;
                sample.chunk = Mix_LoadWAV(file_path);
                if (sample.chunk) {
                    result = 0;
                } else {
                    result = 4;
                }
            }
        } else {
            result = 6;
        }

    } else {
        result = 6;
    }

    return result;
}

void CSoundManager::LoadLoopPoints(FILE* fp, SoundSample& sample) {
    char chunk_id[4];
    unsigned int chunk_size;

    sample.loop_point_start = 0;
    sample.loop_point_length = 0;

    if (fread(chunk_id, sizeof(chunk_id), 1, fp)) {
        if (!strncmp(chunk_id, "RIFF", sizeof(chunk_id))) {
            if (fread(&chunk_size, sizeof(chunk_size), 1, fp)) {
                if (fread(chunk_id, sizeof(chunk_id), 1, fp)) {
                    if (!strncmp(chunk_id, "WAVE", sizeof(chunk_id))) {
                        for (; fread(chunk_id, sizeof(chunk_id), 1, fp);) {
                            if (!fread(&chunk_size, sizeof(chunk_size), 1, fp)) {
                                break; /* something is wrong */
                            }

                            if (!strncmp(chunk_id, "smpl", sizeof(chunk_id))) {
                                typedef struct {
                                    unsigned int manufacturer;
                                    unsigned int product;
                                    unsigned int sample_period;
                                    unsigned int midi_unity_note;
                                    unsigned int midi_pitch_fraction;
                                    unsigned int smpte_format;
                                    unsigned int smpte_offset;
                                    unsigned int num_sample_loops;
                                    unsigned int sampler_data;
                                } SamplerChunk;

                                SamplerChunk sampler_chunk;

                                if (fread(&sampler_chunk, sizeof(sampler_chunk), 1, fp)) {
                                    typedef struct {
                                        unsigned int cue_point_id;
                                        unsigned int type;
                                        unsigned int start;
                                        unsigned int end;
                                        unsigned int fraction;
                                        unsigned int play_count;
                                    } SampleLoop;

                                    SampleLoop sample_loop;

                                    for (int i = 0; i < sampler_chunk.num_sample_loops &&
                                                    fread(&sample_loop, sizeof(sample_loop), 1, fp);
                                         i++) {
                                        sample.loop_point_start = sample_loop.start;
                                        sample.loop_point_length = sample_loop.end - sample_loop.start;

                                        return; /* only one loop point is supported */
                                    }
                                }

                                break;
                            }

                            if (fseek(fp, chunk_size, SEEK_CUR)) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}
