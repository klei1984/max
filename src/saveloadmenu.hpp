/* Copyright (c) 2022 M.A.X. Port Team
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

#ifndef SAVELOADMENU_HPP
#define SAVELOADMENU_HPP

#include <cstdint>

#include "saveload.hpp"

extern const char* SaveLoadMenu_SaveFileTypes[];
extern const char* SaveLoadMenu_TutorialTitles[];
extern const char* SaveLoadMenu_ScenarioTitles[];
extern const char* SaveLoadMenu_CampaignTitles[];

extern int32_t SaveLoadMenu_SaveSlot;
extern uint8_t SaveLoadMenu_GameState;

void SaveLoadMenu_CreateBackup(const char* file_name);
int32_t SaveLoadMenu_MenuLoop(int32_t is_saving_allowed);
void SaveLoadMenu_Save(const char* file_name, const char* save_name, bool play_voice, bool backup = false);
bool SaveLoadMenu_Load(int32_t save_slot, int32_t game_file_type, bool ini_load_mode);
int32_t SaveLoadMenu_GetGameFileType();

#endif /* SAVELOADMENU_HPP */
