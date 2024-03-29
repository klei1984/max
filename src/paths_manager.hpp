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

#ifndef PATHS_MANAGER_HPP
#define PATHS_MANAGER_HPP

#include "pathrequest.hpp"
#include "unitinfo.hpp"

int32_t PathsManager_GetRequestCount(uint16_t team);
void PathsManager_RemoveRequest(PathRequest* request);
void PathsManager_RemoveRequest(UnitInfo* unit);
void PathsManager_PushBack(PathRequest& object);
void PathsManager_PushFront(PathRequest& object);
void PathsManager_EvaluateTiles();
void PathsManager_Clear();
bool PathsManager_HasRequest(UnitInfo* unit);
void PathsManager_ProcessGroundCover(UnitInfo* unit, uint8_t** map, uint8_t flags, int32_t caution_level);
void PathsManager_InitAccessMap(UnitInfo* unit, uint8_t** map, uint8_t flags, int32_t caution_level);
uint8_t** PathsManager_GetAccessMap();
void PathsManager_ApplyCautionLevel(uint8_t** map, UnitInfo* unit, int32_t caution_level);
void PathsManager_SetPathDebugMode();

#endif /* PATHS_MANAGER_HPP */
