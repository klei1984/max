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

#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <cstdio>

#include "unitinfo.hpp"

enum AudioType {
    AUDIO_TYPE_SFX0,
    AUDIO_TYPE_SFX1,
    AUDIO_TYPE_SFX2,
    AUDIO_TYPE_VOICE,
    AUDIO_TYPE_MUSIC,
};

class SoundJob;
class SoundSample;
class SoundGroup;
struct ma_engine;

/**
 * \class SoundManager
 * \brief Central manager for all audio playback including music, sound effects, and voice.
 *
 * The SoundManager orchestrates audio output through a job-based processing system that handles asynchronous loading
 * and playback of audio assets. It manages three distinct audio channels (music, voice, sound effects) with independent
 * volume controls and playback states.
 *
 * Core Architecture:
 * - **Sound Groups**: Separate audio groups for music, voice, and SFX with independent volume and state management.
 * - **Job Queue**: Asynchronous audio loading via SoundJob objects processed each frame.
 * - **Spatial Audio**: Position-based panning and attenuation for in-game sound effects.
 * - **Music Shuffle**: Playlist management with optional shuffle playback for background music.
 *
 * Audio Types:
 * - **Music**: Background tracks with loop point support and shuffle capability.
 * - **Voice**: Priority-based voice clips with queueing (e.g., unit acknowledgments).
 * - **SFX**: Positional sound effects tied to game units with automatic spatial updates.
 *
 * Main Responsibilities:
 * - Initialize and manage the audio engine (miniaudio backend)
 * - Load and decode audio resources from game assets
 * - Control volume levels per audio type
 * - Handle spatial positioning for unit-attached sounds
 * - Process background music playlists with shuffle support
 */
class SoundManager {
public:
    /**
     * \brief Constructs a new SoundManager instance.
     *
     * Initializes the miniaudio engine, creates audio groups for music, voice, and SFX,
     * and prepares the job processing system. Audio is disabled if initialization fails.
     */
    SoundManager() noexcept;

    /**
     * \brief Destructs the SoundManager instance.
     *
     * Frees all audio samples, stops playback, and releases the audio engine resources.
     */
    ~SoundManager() noexcept;

    /**
     * \brief Sets the volume level for a specific audio type.
     *
     * Adjusts the volume for music, voice, or sound effects independently.
     * Volume changes take effect immediately on currently playing audio.
     *
     * \param type The audio type to adjust (AUDIO_TYPE_SFX0-2, AUDIO_TYPE_VOICE, AUDIO_TYPE_MUSIC).
     * \param volume The volume level from 0.0 (silent) to 1.0 (full volume).
     */
    void SetVolume(const int32_t type, const float volume) noexcept;

    /**
     * \brief Starts playback of a background music track.
     *
     * Loads and plays the specified music resource. If shuffle mode is enabled, the manager will
     * automatically advance through the music playlist in random order when tracks complete.
     *
     * \param id The resource ID of the music track to play.
     * \param shuffle If true, enables shuffle mode for automatic playlist advancement.
     */
    void PlayMusic(const ResourceID id, const bool shuffle) noexcept;

    /**
     * \brief Plays a voice clip with optional priority-based queueing.
     *
     * Voice clips are identified by a pair of resource IDs representing a voice group.
     * Higher priority voices can interrupt lower priority ones. Duplicate voice groups
     * already scheduled will not be queued again.
     *
     * \param id1 The first resource ID identifying the voice group.
     * \param id2 The second resource ID identifying the voice group.
     * \param priority The playback priority; higher values interrupt lower priority voices. Default is 0.
     */
    void PlayVoice(const ResourceID id1, const ResourceID id2, const int16_t priority = 0) noexcept;

    /**
     * \brief Plays a sound effect by resource ID.
     *
     * Plays a non-positional sound effect that is not attached to any game unit.
     *
     * \param id The resource ID of the sound effect to play.
     */
    void PlaySfx(const ResourceID id) noexcept;

    /**
     * \brief Plays a positional sound effect attached to a game unit.
     *
     * The sound effect will be spatially positioned based on the unit's location relative to
     * the current viewport, with automatic panning and volume attenuation.
     *
     * \param unit Pointer to the unit that is the source of the sound.
     * \param sound The type of unit sound effect to play (e.g., fire, move, build).
     * \param mode If true, uses alternate playback behavior. Default is false.
     */
    void PlaySfx(UnitInfo* const unit, const Unit::SfxType sound, const bool mode = false) noexcept;

    /**
     * \brief Halts or resumes music playback.
     *
     * When disabled, stops any currently playing music and prevents new music from starting.
     * When re-enabled, allows music playback to resume.
     *
     * \param disable If true, halts music playback; if false, allows playback.
     */
    void HaltMusicPlayback(const bool disable) noexcept;

    /**
     * \brief Halts or resumes voice playback.
     *
     * When disabled, stops any currently playing voice and prevents new voice clips from starting.
     * When re-enabled, allows voice playback to resume.
     *
     * \param disable If true, halts voice playback; if false, allows playback.
     */
    void HaltVoicePlayback(const bool disable) noexcept;

    /**
     * \brief Halts or resumes sound effect playback.
     *
     * When disabled, stops all currently playing sound effects and prevents new ones from starting.
     * When re-enabled, allows sound effect playback to resume.
     *
     * \param disable If true, halts SFX playback; if false, allows playback.
     */
    void HaltSfxPlayback(const bool disable) noexcept;

    /**
     * \brief Updates spatial positioning for all active sound effects.
     *
     * Recalculates panning and attenuation for all unit-attached sounds based on their
     * current positions relative to the viewport. Should be called when the camera moves.
     */
    void UpdateSfxPosition() noexcept;

    /**
     * \brief Updates spatial positioning for a specific unit's sound effect.
     *
     * Recalculates panning and attenuation for the sound attached to the specified unit
     * based on its current position relative to the viewport.
     *
     * \param unit Pointer to the unit whose sound positioning should be updated.
     */
    void UpdateSfxPosition(UnitInfo* const unit) noexcept;

    /**
     * \brief Stops and releases the currently playing music track.
     *
     * Immediately stops music playback and frees the associated audio resources.
     */
    void FreeMusic() noexcept;

    /**
     * \brief Stops and releases all audio samples.
     *
     * Stops all music, voice, and sound effect playback and frees all associated resources.
     * Typically called during game state transitions or shutdown.
     */
    void FreeAllSamples() noexcept;

    /**
     * \brief Processes the audio job queue.
     *
     * Executes pending audio loading and playback jobs. Should be called once per frame
     * to ensure audio assets are loaded and playback is initiated in a timely manner.
     * Also handles music playlist advancement when tracks complete.
     */
    void ProcessJobs() noexcept;

private:
    static constexpr size_t MUSIC_PLAYLIST_SIZE = 10;  // BKG9_MSC - MAIN_MSC + 1

    uint32_t m_device_id{0};
    ma_engine* m_engine{nullptr};
    SoundGroup* m_music_group{nullptr};
    SoundGroup* m_voice_group{nullptr};
    SoundGroup* m_sfx_group{nullptr};
    bool m_is_audio_enabled{false};

    ResourceID m_current_music_played{INVALID_ID};
    ResourceID m_last_music_played{INVALID_ID};
    ResourceID m_voice_played{INVALID_ID};

    bool m_shuffle_music{false};
    bool m_music_playlist[MUSIC_PLAYLIST_SIZE];

    SmartList<SoundJob> m_jobs;

    SmartPointer<SoundSample> m_music;
    SmartPointer<SoundSample> m_voice;
    SmartPointer<SoundSample> m_sfx;

    void AddJob(SoundJob& job) noexcept;
    int32_t ProcessJob(SoundJob& job) noexcept;
    void FreeSample(SmartPointer<SoundSample> sample) noexcept;
    void UpdateMusic() noexcept;
    void FreeSfx(UnitInfo* const unit) noexcept;
    void FreeVoice(const ResourceID id1, const ResourceID id2) noexcept;
    [[nodiscard]] bool IsVoiceGroupScheduled(const ResourceID id1, const ResourceID id2) noexcept;
    [[nodiscard]] static float GetPanning(int32_t distance, const bool reverse) noexcept;
    [[nodiscard]] static float GetZoomAttenuation() noexcept;
    [[nodiscard]] bool PlayMusic(const ResourceID id) noexcept;
    int32_t LoadSound(SoundJob& job, SoundSample& sample) noexcept;
    void LoadLoopPoints(FILE* const fp, SoundSample& sample) noexcept;
};

#endif /* SOUND_MANAGER_HPP */
