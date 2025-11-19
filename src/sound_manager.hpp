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

#include "unitinfo.hpp"

enum AudioType {
    AUDIO_TYPE_SFX0,
    AUDIO_TYPE_SFX1,
    AUDIO_TYPE_SFX2,
    AUDIO_TYPE_VOICE,
    AUDIO_TYPE_MUSIC,
};

void SoundManager_Init() noexcept;
void SoundManager_Deinit() noexcept;
void SoundManager_PlayMusic(const ResourceID id, const bool shuffle) noexcept;
void SoundManager_HaltMusicPlayback(const bool disable) noexcept;
void SoundManager_FreeMusic() noexcept;
void SoundManager_PlaySfx(const ResourceID id) noexcept;
void SoundManager_PlaySfx(UnitInfo* const unit, const Unit::SfxType sound, const bool mode = false) noexcept;
void SoundManager_UpdateSfxPosition() noexcept;
void SoundManager_UpdateSfxPosition(UnitInfo* const unit) noexcept;
void SoundManager_HaltSfxPlayback(const bool disable) noexcept;
void SoundManager_PlayVoice(const ResourceID id1, const ResourceID id2, const int16_t priority = 0) noexcept;
void SoundManager_HaltVoicePlayback(const bool disable) noexcept;
void SoundManager_FreeAllSamples() noexcept;
void SoundManager_SetVolume(const int32_t type, const float volume) noexcept;

#endif /* SOUND_MANAGER_HPP */
