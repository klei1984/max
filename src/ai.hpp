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

#ifndef AI_HPP
#define AI_HPP

#include "unitinfo.hpp"

enum AiReactionStateType : uint8_t {
    AI_REACTION_STATE_IDLE = 0,              // Ready to process reactions
    AI_REACTION_STATE_PROCESSING = 1,        // Waiting for movements/processing
    AI_REACTION_STATE_ANIMATIONS_ACTIVE = 2  // Particles/attacks in progress
};

int32_t Ai_GetNormalRateBuildCost(ResourceID unit_type, uint16_t team);
bool Ai_SetupStrategy(uint16_t team);
void Ai_SetInfoMapPoint(Point point, uint16_t team);
void Ai_MarkMineMapPoint(Point point, uint16_t team);
void Ai_UpdateMineMap(Point point, uint16_t team);
void Ai_SetTasksPendingFlag(const char* event);
void Ai_BeginTurn(uint16_t team);
void Ai_Init();
void Ai_CheckEndTurn();
void Ai_ClearTasksPendingFlags();
void Ai_FileLoad(SmartFileReader& file);
void Ai_FileSave(SmartFileWriter& file);
void Ai_SelectStartingPosition(uint16_t team);
void Ai_AddUnitToTrackerList(UnitInfo* unit);
void Ai_EnableAutoSurvey(UnitInfo* unit);
bool Ai_IsDangerousLocation(UnitInfo* unit, Point destination, int32_t caution_level, bool is_for_attacking);
void Ai_UpdateTerrainDistanceField(UnitInfo* unit);
int32_t Ai_DetermineCautionLevel(UnitInfo* unit);
void Ai_RemoveUnit(UnitInfo* unit);
void Ai_UnitSpotted(UnitInfo* unit, uint16_t team);
bool Ai_IsTargetTeam(UnitInfo* unit, UnitInfo* target);
void Ai_EvaluateAttackTargets(UnitInfo* unit);
void Ai_CheckComputerReactions();
void Ai_CheckMines(UnitInfo* unit);
void Ai_CheckReactions();
AiReactionStateType Ai_GetReactionState();
void Ai_SetReactionState(const AiReactionStateType state);

#endif /* AI_HPP */
