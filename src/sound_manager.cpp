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

#include <filesystem>
#include <iostream>
#include <new>

#include "enums.hpp"
#include "game_manager.hpp"
#include "gnw.h"
#include "inifile.hpp"
#include "localization.hpp"
#include "miniaudio.h"
#include "mvelib32.h"
#include "resource_manager.hpp"

#define SOUND_MANAGER_SAMPLE_RATE (48000)
#define SOUND_MANAGER_CHANNELS (2)

#define SOUND_MANAGER_MAX_VOLUME (1.f)
#define SOUND_MANAGER_PANNING_LEFT (-1.f)
#define SOUND_MANAGER_PANNING_CENTER (0.f)
#define SOUND_MANAGER_PANNING_RIGHT (1.f)

#define SOUND_MANAGER_SFX_FLAG_INVALID (0)
#define SOUND_MANAGER_SFX_FLAG_UNKNOWN_1 (1)
#define SOUND_MANAGER_SFX_FLAG_UNKNOWN_2 (2)
#define SOUND_MANAGER_SFX_FLAG_INFINITE_LOOPING (4)

enum JOB_TYPE { JOB_TYPE_INVALID, JOB_TYPE_SFX0, JOB_TYPE_SFX1, JOB_TYPE_SFX2, JOB_TYPE_VOICE, JOB_TYPE_MUSIC };

enum { SOUND_MANAGER_NO_FADING, SOUND_MANAGER_REQUEST_FADING, SOUND_MANAGER_FADING };

struct SoundVolume {
    float volume;
    int8_t flags;
};

class SoundJob : public SmartObject {
public:
    ResourceID id{INVALID_ID};
    JOB_TYPE type{JOB_TYPE_INVALID};
    float volume_1{0.f};
    float volume_2{0.f};
    float panning{0.f};
    int32_t loop_count{0};
    int16_t grid_x{-1};
    int16_t grid_y{-1};
    int16_t priority{0};
    int32_t sound{SFX_TYPE_INVALID};
    uint16_t unit_id{0xFFFF};
};

class SoundSample : public SmartObject {
public:
    virtual ~SoundSample() {
        if (initialized) {
            ma_sound_uninit(&sound);
            initialized = false;
        }
    }

    ma_sound sound;
    ResourceID id{INVALID_ID};
    JOB_TYPE type{JOB_TYPE_INVALID};
    float volume_1{0.f};
    float volume_2{0.f};
    int32_t loop_count{0};
    int16_t grid_x{-1};
    int16_t grid_y{-1};
    int16_t priority{0};
    uint32_t loop_point_start{0};
    int32_t loop_point_length{0};
    uint8_t fade_out{SOUND_MANAGER_NO_FADING};
    bool initialized{false};
};

class SoundGroup : public SmartObject {
    ma_sound_group* group{nullptr};
    SmartList<SoundSample> samples;

    inline void CleanUp() noexcept {
        for (auto it = samples.Begin(), it_end = samples.End(); it != it_end; ++it) {
            samples.Remove(*it);
        }
    }

public:
    SoundGroup() noexcept {}
    ~SoundGroup() noexcept { Deinit(); }

    inline void Init(ma_engine* const engine, const ma_uint32 flags) noexcept {
        group = new (std::nothrow) ma_sound_group;

        ma_sound_group_init(engine, flags, nullptr, group);
    }

    inline void Deinit() noexcept {
        CleanUp();

        if (group) {
            ma_sound_group_uninit(group);
            delete group;
            group = nullptr;
        }
    }

    [[nodiscard]] inline ma_sound_group* GetGroup() noexcept { return group; }

    [[nodiscard]] inline SmartList<SoundSample>* GetSamples() noexcept { return &samples; }

    [[nodiscard]] inline SmartPointer<SoundSample> GetSound(const ResourceID id,
                                                            const bool only_finished = false) noexcept {
        SmartPointer<SoundSample> result{nullptr};

        for (auto it = samples.Begin(), it_end = samples.End(); it != it_end; ++it) {
            if (it->Get()->id == id) {
                if (only_finished && ma_sound_at_end(&it->Get()->sound)) {
                    continue;
                }

                result = *it;
                break;
            }
        }

        return result;
    }
};

class SoundManager {
public:
    SoundManager() noexcept;
    ~SoundManager() noexcept;

    void Init() noexcept;
    void Deinit() noexcept;

    void InitVolumeTable() noexcept;
    void DeinitVolumeTable() noexcept;

    void SetVolume(const int32_t type, const float volume) noexcept;

    void PlayMusic(const ResourceID id, const bool shuffle) noexcept;
    void PlayVoice(const ResourceID id1, const ResourceID id2, const int16_t priority = 0) noexcept;
    void PlaySfx(const ResourceID id) noexcept;
    void PlaySfx(UnitInfo* const unit, const int32_t sound, const bool mode = false) noexcept;

    void HaltMusicPlayback(const bool disable) noexcept;
    void HaltVoicePlayback(const bool disable) noexcept;
    void HaltSfxPlayback(const bool disable) noexcept;

    void UpdateSfxPosition() noexcept;
    void UpdateSfxPosition(UnitInfo* const unit) noexcept;

    void FreeMusic() noexcept;
    void FreeAllSamples() noexcept;

    void ProcessJobs() noexcept;

private:
    SDL_AudioDeviceID device_id{0};
    ma_engine* engine{nullptr};
    SoundGroup* music_group{nullptr};
    SoundGroup* voice_group{nullptr};
    SoundGroup* sfx_group{nullptr};
    bool is_audio_enabled{false};
    SoundVolume* volumes{nullptr};

    ResourceID current_music_played{INVALID_ID};
    ResourceID last_music_played{INVALID_ID};
    ResourceID voice_played{INVALID_ID};

    bool shuffle_music{false};
    bool music_playlist[BKG9_MSC - MAIN_MSC + 1];

    SmartList<SoundJob> jobs;

    SmartPointer<SoundSample> music;
    SmartPointer<SoundSample> voice;
    SmartPointer<SoundSample> sfx;

    void AddJob(SoundJob& job) noexcept;
    int32_t ProcessJob(SoundJob& job) noexcept;
    void FreeSample(SmartPointer<SoundSample> sample) noexcept;
    void UpdateMusic() noexcept;
    void FreeSfx(UnitInfo* const unit) noexcept;
    void FreeVoice(const ResourceID id1, const ResourceID id2) noexcept;
    [[nodiscard]] bool IsVoiceGroupScheduled(const ResourceID id1, const ResourceID id2) noexcept;
    [[nodiscard]] static float GetPanning(int32_t distance, const bool reverse) noexcept;
    [[nodiscard]] bool PlayMusic(const ResourceID id) noexcept;
    int32_t LoadSound(SoundJob& job, SoundSample& sample) noexcept;
    void LoadLoopPoints(FILE* const fp, SoundSample& sample) noexcept;
};

static SoundManager SoundManager_Manager;

static const uint8_t SoundManager_SfxTypeFlags[SFX_TYPE_LIMIT] = {
    SOUND_MANAGER_SFX_FLAG_INVALID,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2 | SOUND_MANAGER_SFX_FLAG_INFINITE_LOOPING,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2 | SOUND_MANAGER_SFX_FLAG_INFINITE_LOOPING,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2 | SOUND_MANAGER_SFX_FLAG_INFINITE_LOOPING,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2 | SOUND_MANAGER_SFX_FLAG_INFINITE_LOOPING,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2 | SOUND_MANAGER_SFX_FLAG_INFINITE_LOOPING,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_1,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_1,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_1,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_1,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_1,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_1,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2,
    SOUND_MANAGER_SFX_FLAG_UNKNOWN_2};

static const int16_t SoundManager_VoicePriorities[V_END - V_START + 1] = {
    0,  10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 35, 35, 30, 30, 40, 40, 40, 40, 0,  0,  45, 45, 45, 45, 10,
    10, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 50,
    50, 50, 50, 0,  50, 50, 50, 50, 50, 50, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  50, 50, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  40, 40, 40, 40, 0,  0,  0,  0,  30, 30, 30, 30, 0,  5,  5,  5,  5,  60, 60, 60, 60, 60, 60, 60, 60,
    60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 0,  0,  5,  5,  5,  5,  5,  5,  0,  0,  0,  0,
    0,  0,  10, 10, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 5,  5,  5,  5,  5,  5,  5,  5,
    5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

static void SoundManager_BackgroundProcess(void) noexcept { SoundManager_Manager.ProcessJobs(); }

SoundManager::SoundManager() noexcept { SDL_zero(music_playlist); }

SoundManager::~SoundManager() noexcept { Deinit(); }

void SoundManager::Init() noexcept {
    ma_engine_config engineConfig = ma_engine_config_init();

    engineConfig.channels = SOUND_MANAGER_CHANNELS;
    engineConfig.sampleRate = SOUND_MANAGER_SAMPLE_RATE;

    engine = new (std::nothrow) ma_engine;

    if (ma_engine_init(&engineConfig, engine) != MA_SUCCESS) {
        delete engine;
        engine = nullptr;

        SDL_Log("%s", _(37d9));
        is_audio_enabled = false;

    } else {
        MVE_sndInit(engine);

        InitVolumeTable();

        music_group = new (std::nothrow) SoundGroup;
        voice_group = new (std::nothrow) SoundGroup;
        sfx_group = new (std::nothrow) SoundGroup;

        music_group->Init(engine, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_STREAM);
        voice_group->Init(engine, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE);
        sfx_group->Init(engine, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE);

        ma_sound_group_set_volume(music_group->GetGroup(),
                                  std::min<float>(ini_get_setting(INI_MUSIC_LEVEL), 100) / 100);

        ma_sound_group_set_volume(sfx_group->GetGroup(),
                                  std::min<float>(ini_get_setting(INI_FX_SOUND_LEVEL), 100) / 100);

        ma_sound_group_set_volume(voice_group->GetGroup(),
                                  std::min<float>(ini_get_setting(INI_VOICE_LEVEL), 100) / 100);

        is_audio_enabled = true;

        add_bk_process(SoundManager_BackgroundProcess);
        enable_bk();
    }
}

void SoundManager::Deinit() noexcept {
    if (is_audio_enabled) {
        music = nullptr;
        voice = nullptr;
        sfx = nullptr;

        delete music_group;
        music_group = nullptr;

        delete voice_group;
        voice_group = nullptr;

        delete sfx_group;
        sfx_group = nullptr;

        ma_engine_uninit(engine);
        delete engine;
        engine = nullptr;

        MVE_sndInit(nullptr);

        is_audio_enabled = false;
    }

    jobs.Clear();

    DeinitVolumeTable();
}

void SoundManager::InitVolumeTable() noexcept {
    IniSoundVolumes ini_soundvol;
    constexpr uint32_t table_size{FXS_END - FXS_STRT - 1};

    ini_soundvol.Init();

    volumes = new (std::nothrow) SoundVolume[table_size];

    for (uint32_t i = 0; i < table_size; ++i) {
        volumes[i].volume = static_cast<float>(ini_soundvol.GetUnitVolume((ResourceID)(FXS_STRT + i + 1))) / 0x7FFF;
        volumes[i].flags = -1;
    }
}

void SoundManager::DeinitVolumeTable() noexcept {
    if (volumes) {
        delete[] volumes;
        volumes = nullptr;
    }
}

void SoundManager::UpdateMusic() noexcept {
    if (music && !ma_sound_is_playing(&music->sound)) {
        if (shuffle_music) {
            ResourceID resource_id;
            uint32_t index;

            /* if all tracks were played from the list, reset list state */
            for (index = 0; (index < std::size(music_playlist)) && (music_playlist[index] != true); ++index) {
                ;
            }

            if (index == std::size(music_playlist)) {
                for (auto& item : music_playlist) {
                    item = true;
                }
            }

            for (;;) {
                do {
                    index = ((std::size(music_playlist) * dos_rand()) >> 15);
                    resource_id = (ResourceID)(index + MAIN_MSC);
                } while (!music_playlist[index]);

                music_playlist[index] = false;

                if (PlayMusic(resource_id)) {
                    return;
                }

                for (index = 0; (index < std::size(music_playlist)) && (music_playlist[index] != true); ++index) {
                    ;
                }

                if (index == std::size(music_playlist)) {
                    SmartPointer<SoundSample> sample(music_group->GetSamples()->Begin()->Get());

                    if (sample) {
                        ma_sound_start(&sample->sound);

                        current_music_played = resource_id;
                    }

                    return;
                }
            }

        } else {
            SmartPointer<SoundSample> sample(music_group->GetSamples()->Begin()->Get());

            if (sample) {
                ma_sound_start(&sample->sound);

                current_music_played = sample->id;
            }
        }
    }
}

void SoundManager::FreeSfx(UnitInfo* const unit) noexcept {
    if (sfx) {
        if (sfx->fade_out < SOUND_MANAGER_REQUEST_FADING) {
            sfx->fade_out = SOUND_MANAGER_REQUEST_FADING;
        }
    }

    sfx = nullptr;

    const uint16_t unit_id = unit->GetId();

    unit->SetSfxType(SFX_TYPE_INVALID);

    for (auto it = jobs.Begin(), it_end = jobs.End(); it != it_end; ++it) {
        if ((*it).unit_id == unit_id && (*it).type == JOB_TYPE_SFX0) {
            jobs.Remove(it);
        }
    }
}

void SoundManager::FreeSample(SmartPointer<SoundSample> sample) noexcept {
    if (is_audio_enabled) {
        if (ma_sound_is_playing(&sample->sound)) {
            ma_sound_stop(&sample->sound);
        }

        if (sample->type == JOB_TYPE_MUSIC) {
            music_group->GetSamples()->Remove(*sample);

            if (music == sample) {
                music = nullptr;
            }

            current_music_played = INVALID_ID;

        } else if (sample->type >= JOB_TYPE_SFX0 && sample->type <= JOB_TYPE_SFX2) {
            sfx_group->GetSamples()->Remove(*sample);

            if (sfx == sample) {
                sfx = nullptr;
            }

        } else if (sample->type == JOB_TYPE_VOICE) {
            voice_group->GetSamples()->Remove(*sample);

            if (voice == sample) {
                voice = nullptr;
            }

            voice_played = INVALID_ID;
        }
    }
}

void SoundManager::PlayMusic(const ResourceID id, const bool shuffle) noexcept {
    if ((id != INVALID_ID) && (id != current_music_played)) {
        if (ini_get_setting(INI_DISABLE_MUSIC)) {
            last_music_played = id;

        } else {
            FreeMusic();

            shuffle_music = shuffle;

            if ((shuffle_music) && ((id < MAIN_MSC) || (id > BKG9_MSC))) {
                for (auto& item : music_playlist) {
                    item = true;
                }
            }

            SmartPointer<SoundJob> job(new (std::nothrow) SoundJob);

            job->id = id;
            job->type = JOB_TYPE_MUSIC;
            job->volume_1 = SOUND_MANAGER_MAX_VOLUME;
            job->volume_2 = SOUND_MANAGER_MAX_VOLUME;
            job->panning = SOUND_MANAGER_PANNING_CENTER;
            job->loop_count = 0;
            job->grid_x = 0;
            job->grid_y = 0;
            job->priority = 0;
            job->sound = SFX_TYPE_INVALID;
            job->unit_id = -1;

            AddJob(*job);
        }
    }
}

void SoundManager::HaltMusicPlayback(const bool disable) noexcept {
    if (disable) {
        last_music_played = current_music_played;
        FreeMusic();

    } else {
        PlayMusic(last_music_played, shuffle_music);
    }
}

void SoundManager::FreeMusic() noexcept {
    if (music) {
        FreeSample(music);
    }
}

void SoundManager::PlaySfx(const ResourceID id) noexcept {
    if (!ini_get_setting(INI_DISABLE_FX)) {
        SmartPointer<SoundJob> job(new (std::nothrow) SoundJob);

        job->id = id;
        job->type = JOB_TYPE_SFX2;
        job->volume_1 = SOUND_MANAGER_MAX_VOLUME;
        job->volume_2 = SOUND_MANAGER_MAX_VOLUME;
        job->panning = SOUND_MANAGER_PANNING_CENTER;
        job->loop_count = 0;
        job->grid_x = 0;
        job->grid_y = 0;
        job->priority = 0;
        job->sound = SFX_TYPE_INVALID;
        job->unit_id = -1;

        AddJob(*job);
        ProcessJobs();
    }
}

void SoundManager::PlaySfx(UnitInfo* const unit, const int32_t sound, const bool mode) noexcept {
    uint8_t flags;

    SDL_assert(unit);
    SDL_assert(sound < SFX_TYPE_LIMIT);

    if (mode) {
        flags = SOUND_MANAGER_SFX_FLAG_UNKNOWN_1;

    } else {
        flags = SoundManager_SfxTypeFlags[sound];
    }

    if (!(flags & SOUND_MANAGER_SFX_FLAG_UNKNOWN_2) || unit->SetSfxType(sound) != sound) {
        if (SFX_TYPE_INVALID == sound) {
            FreeSfx(unit);
            return;
        }

        if (!ini_get_setting(INI_DISABLE_FX) && is_audio_enabled) {
            int32_t grid_center_x;
            int32_t grid_center_y;
            int32_t grid_offset_x;
            int32_t grid_offset_y;
            int32_t grid_distance_x;
            int32_t grid_distance_y;
            int32_t loop_count;
            uint32_t sound_index;
            int32_t volume_index;
            int32_t resource_id;

            for (sound_index = 0;
                 sound_index < unit->sound_table->size() && (*unit->sound_table)[sound_index].type != sound;
                 ++sound_index) {
                ;
            }

            if (sound_index == unit->sound_table->size()) {
                if (SoundManager_SfxTypeFlags[sound] != SOUND_MANAGER_SFX_FLAG_UNKNOWN_1) {
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
                ++resource_id;
            }

            volume_index = resource_id - GEN_IDLE;

            if (volumes[resource_id - GEN_IDLE].flags == -1) {
                volumes[volume_index].flags = 1;

                auto fp{ResourceManager_OpenFileResource(static_cast<ResourceID>(resource_id), ResourceType_Sfx)};

                if (fp) {
                    volumes[volume_index].flags = 0;
                    fclose(fp);
                }
            }

            if (volumes[volume_index].flags == 1) {
                resource_id = sound + FXS_STRT;
                volume_index = sound - 1;
            }

            SmartPointer<SoundJob> job(new (std::nothrow) SoundJob);

            job->type = static_cast<JOB_TYPE>(((flags & SOUND_MANAGER_SFX_FLAG_UNKNOWN_2) != 0) + 1);
            job->id = static_cast<ResourceID>(resource_id);

            job->grid_x = unit->grid_x;
            job->grid_y = unit->grid_y;

            if (flags & SOUND_MANAGER_SFX_FLAG_INFINITE_LOOPING) {
                loop_count = -1;

            } else {
                loop_count = 0;
            }

            job->loop_count = loop_count;
            job->sound = sound;

            grid_center_x = (GameManager_MapView.ulx + GameManager_MapView.lrx) / 2;
            grid_center_y = (GameManager_MapView.uly + GameManager_MapView.lry) / 2;

            grid_offset_x = job->grid_x - grid_center_x;
            grid_offset_y = job->grid_y - grid_center_y;

            grid_distance_x = labs(grid_offset_x);
            grid_distance_y = labs(grid_offset_y);

            job->volume_2 = volumes[volume_index].volume;
            job->volume_1 = job->volume_2 - job->volume_2 * std::max(grid_distance_x, grid_distance_y) /
                                                std::max(ResourceManager_MapSize.x, ResourceManager_MapSize.y);

            job->panning = GetPanning(grid_distance_x, (job->grid_x - grid_center_x) < 0);

            job->priority = 0;
            job->unit_id = unit->GetId();

            AddJob(*job);
        }
    }
}

void SoundManager::UpdateSfxPosition() noexcept {
    const int32_t grid_center_x = (GameManager_MapView.ulx + GameManager_MapView.lrx) / 2;
    const int32_t grid_center_y = (GameManager_MapView.uly + GameManager_MapView.lry) / 2;

    for (auto it = sfx_group->GetSamples()->Begin(), it_end = sfx_group->GetSamples()->End(); it != it_end; ++it) {
        if (ma_sound_is_playing(&(*it).sound) && (*it).type <= JOB_TYPE_SFX1) {
            const int32_t grid_offset_x = (*it).grid_x - grid_center_x;
            const int32_t grid_offset_y = (*it).grid_y - grid_center_y;

            const int32_t grid_distance_x = labs(grid_offset_x);
            const int32_t grid_distance_y = labs(grid_offset_y);

            ma_sound_set_pan(&(*it).sound, GetPanning(grid_distance_x, grid_offset_x < 0));

            float sound_level = (*it).volume_2 - (*it).volume_2 * std::max(grid_distance_x, grid_distance_y) /
                                                     std::max(ResourceManager_MapSize.x, ResourceManager_MapSize.y);

            ma_sound_set_volume(&(*it).sound, sound_level);
        }
    }
}

void SoundManager::UpdateSfxPosition(UnitInfo* const unit) noexcept {
    if (sfx && ma_sound_is_playing(&sfx->sound)) {
        sfx->grid_x = unit->grid_x;
        sfx->grid_y = unit->grid_y;

        const int32_t grid_center_x = (GameManager_MapView.ulx + GameManager_MapView.lrx) / 2;
        const int32_t grid_center_y = (GameManager_MapView.uly + GameManager_MapView.lry) / 2;

        const int32_t grid_offset_x = sfx->grid_x - grid_center_x;
        const int32_t grid_offset_y = sfx->grid_y - grid_center_y;

        const int32_t grid_distance_x = labs(grid_offset_x);
        const int32_t grid_distance_y = labs(grid_offset_y);

        ma_sound_set_pan(&sfx->sound, GetPanning(grid_distance_x, grid_offset_x < 0));

        float sound_level = sfx->volume_2 - sfx->volume_2 * std::max(grid_distance_x, grid_distance_y) /
                                                std::max(ResourceManager_MapSize.x, ResourceManager_MapSize.y);

        ma_sound_set_volume(&sfx->sound, sound_level);
    }
}

void SoundManager::HaltSfxPlayback(const bool disable) noexcept {
    if (disable) {
        for (auto it = sfx_group->GetSamples()->Begin(), it_end = sfx_group->GetSamples()->End(); it != it_end; ++it) {
            FreeSample(it->Get());
        }

    } else if (GameManager_SelectedUnit != nullptr) {
        PlaySfx(&*GameManager_SelectedUnit, SFX_TYPE_INVALID, false);
        PlaySfx(&*GameManager_SelectedUnit, GameManager_SelectedUnit->GetSfxType(), false);
    }
}

void SoundManager::PlayVoice(const ResourceID id1, const ResourceID id2, const int16_t priority) noexcept {
    if (priority >= 0) {
        if (!IsVoiceGroupScheduled(id1, id2) && !ini_get_setting(INI_DISABLE_VOICE)) {
            int16_t priority_value;
            uint16_t randomized_voice_id;

            if (priority > 0) {
                priority_value = priority;

            } else {
                priority_value = SoundManager_VoicePriorities[id1 - V_START];
            }

            randomized_voice_id = 2 * ((((id2 - id1) / 2 + 1) * dos_rand()) >> 15) + 1 + id1;
            SDL_assert(randomized_voice_id != id1 && randomized_voice_id <= id2);

            for (auto it = jobs.Begin(), it_end = jobs.End(); it != it_end; ++it) {
                if ((*it).type == JOB_TYPE_VOICE) {
                    if (priority_value >= (*it).priority) {
                        (*it).id = static_cast<ResourceID>(randomized_voice_id);
                        (*it).priority = priority_value;
                    }

                    return;
                }
            }

            SmartPointer<SoundJob> job(new (std::nothrow) SoundJob);

            job->id = (ResourceID)randomized_voice_id;
            job->type = JOB_TYPE_VOICE;
            job->volume_1 = SOUND_MANAGER_MAX_VOLUME;
            job->volume_2 = SOUND_MANAGER_MAX_VOLUME;
            job->panning = SOUND_MANAGER_PANNING_CENTER;
            job->loop_count = 0;
            job->grid_x = 0;
            job->grid_y = 0;
            job->priority = priority_value;
            job->sound = SFX_TYPE_INVALID;
            job->unit_id = -1;

            AddJob(*job);
        }

    } else {
        FreeVoice(id1, id2);
    }
}

void SoundManager::HaltVoicePlayback(const bool disable) noexcept {
    if (disable) {
        if (voice) {
            FreeSample(voice);
        }
    }
}

void SoundManager::FreeAllSamples() noexcept {
    if (is_audio_enabled) {
        for (auto it = music_group->GetSamples()->Begin(), it_end = music_group->GetSamples()->End(); it != it_end;
             ++it) {
            FreeSample(it->Get());
        }

        for (auto it = voice_group->GetSamples()->Begin(), it_end = voice_group->GetSamples()->End(); it != it_end;
             ++it) {
            FreeSample(it->Get());
        }

        for (auto it = sfx_group->GetSamples()->Begin(), it_end = sfx_group->GetSamples()->End(); it != it_end; ++it) {
            FreeSample(it->Get());
        }
    }
}

void SoundManager::SetVolume(const int32_t type, const float volume) noexcept {
    if (type == AUDIO_TYPE_MUSIC) {
        ma_sound_group_set_volume(music_group->GetGroup(), volume);

    } else if (type >= AUDIO_TYPE_SFX0 && type <= AUDIO_TYPE_SFX2) {
        ma_sound_group_set_volume(sfx_group->GetGroup(), volume);

    } else if (type == AUDIO_TYPE_VOICE) {
        ma_sound_group_set_volume(voice_group->GetGroup(), volume);
    }
}

void SoundManager::ProcessJobs() noexcept {
    if (is_audio_enabled) {
        UpdateMusic();

        for (auto it = jobs.Begin(), it_end = jobs.End(); it != it_end; ++it) {
            if (0 == ProcessJob(*it)) {
                jobs.Remove(it);
            }
        }

        for (auto it = sfx_group->GetSamples()->Begin(), it_end = sfx_group->GetSamples()->End(); it != it_end; ++it) {
            if (ma_sound_is_playing(&(*it).sound)) {
                if ((*it).fade_out == SOUND_MANAGER_REQUEST_FADING) {
                    float volume = ma_sound_group_get_volume(sfx_group->GetGroup()) * 100;
                    ma_uint64 time{0};

                    while (static_cast<int32_t>(volume) > 0) {
                        volume /= 2.f;
                        time += 125;
                    }

                    ma_sound_stop_with_fade_in_milliseconds(&(*it).sound, time);

                    (*it).fade_out = SOUND_MANAGER_FADING;
                }

            } else {
                FreeSample(*it);
            }
        }
    }
}

void SoundManager::AddJob(SoundJob& job) noexcept {
    if ((is_audio_enabled) && (job.id != INVALID_ID)) {
        if (job.type <= JOB_TYPE_SFX2) {
            for (auto it = jobs.Begin(), it_end = jobs.End(); it != it_end; ++it) {
                if ((*it).id == job.id) {
                    if (job.volume_1 > (*it).volume_1) {
                        (*it).volume_1 = job.volume_1;
                    }

                    return;
                }

                if (((*it).type == JOB_TYPE_SFX1) && (job.type == JOB_TYPE_SFX1)) {
                    *it = job;

                    return;
                }
            }

            if (job.type == JOB_TYPE_SFX2) {
                for (auto it = sfx_group->GetSamples()->Begin(), it_end = sfx_group->GetSamples()->End(); it != it_end;
                     ++it) {
                    if (job.id == (*it).id && ma_sound_is_playing(&(*it).sound)) {
                        return;
                    }
                }
            }
        }

        if (job.priority > 0) {
            for (auto it = jobs.Begin(), it_end = jobs.End(); it != it_end; ++it) {
                if (job.priority > (*it).priority) {
                    jobs.InsertBefore(it, job);

                    return;
                }
            }
        }

        jobs.PushBack(job);
    }
}

int32_t SoundManager::ProcessJob(SoundJob& job) noexcept {
    int32_t result;

    if (job.type == JOB_TYPE_VOICE && voice && ma_sound_is_playing(&voice->sound)) {
        result = 11;

    } else {
        for (auto it = sfx_group->GetSamples()->Begin(), it_end = sfx_group->GetSamples()->End(); it != it_end; ++it) {
            if (!ma_sound_is_playing(&(*it).sound)) {
                FreeSample(*it);
            }
        }

        SmartPointer<SoundSample> sample(new (std::nothrow) SoundSample);
        result = LoadSound(job, *sample);

        if (0 == result) {
            if (job.type >= JOB_TYPE_SFX0 && job.type <= JOB_TYPE_SFX2) {
                if (sample->loop_point_start != 0 || sample->loop_point_length != 0) {
                    ma_uint64 length;

                    ma_data_source_get_length_in_pcm_frames(ma_sound_get_data_source(&sample->sound), &length);

                    if (job.sound == SFX_TYPE_BUILDING) {
                        ma_data_source_set_range_in_pcm_frames(ma_sound_get_data_source(&sample->sound),
                                                               sample->loop_point_start,
                                                               sample->loop_point_start + sample->loop_point_length);

                        ma_data_source_set_loop_point_in_pcm_frames(ma_sound_get_data_source(&sample->sound), 0,
                                                                    sample->loop_point_length);

                    } else {
                        ma_data_source_set_loop_point_in_pcm_frames(
                            ma_sound_get_data_source(&sample->sound), sample->loop_point_start,
                            sample->loop_point_start + sample->loop_point_length);
                    }

                    job.loop_count = -1;
                }
            }

            sample->id = job.id;
            sample->type = job.type;
            sample->volume_1 = job.volume_1;
            sample->volume_2 = job.volume_2;
            sample->loop_count = job.loop_count;
            sample->grid_x = job.grid_x;
            sample->grid_y = job.grid_y;
            sample->priority = job.priority;
            sample->fade_out = SOUND_MANAGER_NO_FADING;

            ma_sound_set_pan(&sample->sound, job.panning);
            ma_sound_set_volume(&sample->sound, sample->volume_1);
            ma_sound_set_looping(&sample->sound, sample->loop_count);
            ma_sound_start(&sample->sound);

            if (job.type >= JOB_TYPE_SFX0 && job.type <= JOB_TYPE_SFX2) {
                sfx_group->GetSamples()->PushBack(*sample);

                if (job.type == JOB_TYPE_SFX1) {
                    if (sfx && (&sfx->sound != &sample->sound)) {
                        sfx->fade_out = SOUND_MANAGER_REQUEST_FADING;
                    }

                    sfx = sample;
                }

            } else if (job.type == JOB_TYPE_VOICE) {
                voice_group->GetSamples()->PushBack(*sample);
                voice = sample;
                voice_played = job.id;

            } else if (job.type == JOB_TYPE_MUSIC) {
                music_group->GetSamples()->PushBack(*sample);
                music = sample;
                current_music_played = job.id;
            }
        }
    }

    return result;
}

void SoundManager::FreeVoice(const ResourceID id1, const ResourceID id2) noexcept {
    if (voice && voice_played >= id1 && voice_played <= id2) {
        FreeSample(voice);
    }

    for (auto it = jobs.Begin(), it_end = jobs.End(); it != it_end; ++it) {
        if ((*it).type == JOB_TYPE_VOICE && (*it).id >= id1 && (*it).id <= id2) {
            jobs.Remove(it);
        }
    }
}

bool SoundManager::IsVoiceGroupScheduled(const ResourceID id1, const ResourceID id2) noexcept {
    if (voice_played >= id1 && voice_played <= id2 && voice && ma_sound_is_playing(&voice->sound)) {
        return true;
    }

    for (auto it = jobs.Begin(), it_end = jobs.End(); it != it_end; ++it) {
        if ((*it).type == JOB_TYPE_VOICE && (*it).id >= id1 && (*it).id <= id2) {
            return true;
        }
    }

    return false;
}

float SoundManager::GetPanning(int32_t distance, const bool reverse) noexcept {
    float panning;

    if (distance > 28) {
        distance = 28;
    }

    if (reverse) {
        distance = -distance;
    }

    panning = (SOUND_MANAGER_PANNING_RIGHT * (distance + 56)) / 112;

    if (ini_get_setting(INI_CHANNELS_REVERSED)) {
        panning = SOUND_MANAGER_PANNING_RIGHT - panning;
    }

    return panning;
}

bool SoundManager::PlayMusic(const ResourceID id) noexcept {
    bool result{false};

    if (music && music->id == id) {
        ma_sound_start(&music->sound);
        current_music_played = id;
        result = true;

    } else {
        SmartPointer<SoundSample> sample = music_group->GetSound(id, true);

        if (sample) {
            music = sample;
            ma_sound_start(&sample->sound);
            current_music_played = id;
            result = true;

        } else {
            char* const filename = reinterpret_cast<char*>(ResourceManager_ReadResource(id));

            if (filename) {
                ResourceManager_ToUpperCase(filename);
                const auto filepath =
                    (std::filesystem::path(ResourceManager_FilePathMusic) / filename).lexically_normal();
                delete[] filename;

                SmartPointer<SoundSample> new_sample(new (std::nothrow) SoundSample);

                if (ma_sound_init_from_file(
                        engine, filepath.string().c_str(),
                        MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_STREAM,
                        music_group->GetGroup(), nullptr, &new_sample->sound) == MA_SUCCESS) {
                    new_sample->initialized = true;
                    new_sample->id = id;
                    new_sample->type = JOB_TYPE_MUSIC;

                    music_group->GetSamples()->PushFront(*new_sample);
                    music = new_sample;
                    ma_sound_start(&music->sound);
                    current_music_played = id;
                    result = true;
                }
            }
        }
    }

    return result;
}

int32_t SoundManager::LoadSound(SoundJob& job, SoundSample& sample) noexcept {
    int32_t result;
    ma_sound_group* group;
    ma_uint32 flags;
    ResourceType type;
    std::filesystem::path filepath;

    SDL_memset(&sample.sound, 0, sizeof(sample.sound));

    if (JOB_TYPE_MUSIC == job.type) {
        type = ResourceType_Music;
        group = music_group->GetGroup();
        flags = MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_STREAM;

    } else if (JOB_TYPE_VOICE == job.type) {
        type = ResourceType_Voice;
        group = voice_group->GetGroup();
        flags = MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE;

    } else {
        type = ResourceType_Sfx;
        group = sfx_group->GetGroup();
        flags = MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE;
    }

    auto fp{ResourceManager_OpenFileResource(job.id, type, "rb", &filepath)};

    if (fp) {
        LoadLoopPoints(fp, sample);
        fclose(fp);

        if (ma_sound_init_from_file(engine, filepath.string().c_str(), flags, group, nullptr, &sample.sound) ==
            MA_SUCCESS) {
            sample.initialized = true;
            result = 0;

        } else {
            result = 4;
        }

    } else {
        result = 4;
    }

    return result;
}

void SoundManager::LoadLoopPoints(FILE* const fp, SoundSample& sample) noexcept {
    char chunk_id[4];
    uint32_t chunk_size;

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
                                    uint32_t manufacturer;
                                    uint32_t product;
                                    uint32_t sample_period;
                                    uint32_t midi_unity_note;
                                    uint32_t midi_pitch_fraction;
                                    uint32_t smpte_format;
                                    uint32_t smpte_offset;
                                    uint32_t num_sample_loops;
                                    uint32_t sampler_data;
                                } SamplerChunk;

                                SamplerChunk sampler_chunk;

                                if (fread(&sampler_chunk, sizeof(sampler_chunk), 1, fp)) {
                                    typedef struct {
                                        uint32_t cue_point_id;
                                        uint32_t type;
                                        uint32_t start;
                                        uint32_t end;
                                        uint32_t fraction;
                                        uint32_t play_count;
                                    } SampleLoop;

                                    SampleLoop sample_loop;

                                    for (uint32_t i = 0; i < sampler_chunk.num_sample_loops &&
                                                         fread(&sample_loop, sizeof(sample_loop), 1, fp);
                                         ++i) {
                                        sample.loop_point_start =
                                            (static_cast<uint64_t>(sample_loop.start) * SOUND_MANAGER_SAMPLE_RATE *
                                             sampler_chunk.sample_period) /
                                            1000000000LL;

                                        uint32_t loop_point_end =
                                            (static_cast<uint64_t>(sample_loop.end) * SOUND_MANAGER_SAMPLE_RATE *
                                             sampler_chunk.sample_period) /
                                            1000000000LL;

                                        sample.loop_point_length = loop_point_end - sample_loop.start;

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

void SoundManager_Init() noexcept { SoundManager_Manager.Init(); }

void SoundManager_Deinit() noexcept { SoundManager_Manager.Deinit(); }

void SoundManager_PlayMusic(const ResourceID id, const bool shuffle) noexcept {
    SoundManager_Manager.PlayMusic(id, shuffle);
}

void SoundManager_HaltMusicPlayback(const bool disable) noexcept { SoundManager_Manager.HaltMusicPlayback(disable); }

void SoundManager_FreeMusic() noexcept { SoundManager_Manager.FreeMusic(); }

void SoundManager_PlaySfx(const ResourceID id) noexcept { SoundManager_Manager.PlaySfx(id); }

void SoundManager_PlaySfx(UnitInfo* const unit, const int32_t sound, const bool mode) noexcept {
    SoundManager_Manager.PlaySfx(unit, sound, mode);
}

void SoundManager_UpdateSfxPosition() noexcept { SoundManager_Manager.UpdateSfxPosition(); }

void SoundManager_UpdateSfxPosition(UnitInfo* const unit) noexcept { SoundManager_Manager.UpdateSfxPosition(unit); }

void SoundManager_HaltSfxPlayback(const bool disable) noexcept { SoundManager_Manager.HaltSfxPlayback(disable); }

void SoundManager_PlayVoice(const ResourceID id1, const ResourceID id2, const int16_t priority) noexcept {
    SoundManager_Manager.PlayVoice(id1, id2, priority);
}

void SoundManager_HaltVoicePlayback(const bool disable) noexcept { SoundManager_Manager.HaltVoicePlayback(disable); }

void SoundManager_FreeAllSamples() noexcept { SoundManager_Manager.FreeAllSamples(); }

void SoundManager_SetVolume(const int32_t type, const float volume) noexcept {
    SoundManager_Manager.SetVolume(type, volume);
}
