/* Copyright (c) 2021 M.A.X. Port Team
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

#ifndef PATHS_HPP
#define PATHS_HPP

#include "airpath.hpp"
#include "builderpath.hpp"
#include "groundpath.hpp"
#include "path.hpp"
#include "unitpath.hpp"

class UnitInfo;

bool Paths_RequestPath(UnitInfo* unit, int32_t mode);
AirPath* Paths_GetAirPath(UnitInfo* unit);
bool Paths_UpdateAngle(UnitInfo* unit, int32_t angle);
int32_t Paths_GetAngle(int32_t x, int32_t y);
bool Paths_CalculateStep(UnitInfo* unit, int32_t cost, bool is_diagonal_step);
void Paths_DrawMarker(WindowInfo* window, int32_t angle, int32_t grid_x, int32_t grid_y, int32_t color);
void Paths_DrawShots(WindowInfo* window, int32_t grid_x, int32_t grid_y, int32_t shots);
bool Paths_IsOccupied(int32_t grid_x, int32_t grid_y, int32_t angle, int32_t team);

void Paths_ReserveSite(const Point site) noexcept;
void Paths_RemoveSiteReservation(const Point site) noexcept;
void Paths_ClearSiteReservations() noexcept;
[[nodiscard]] bool Paths_IsSiteReserved(const Point site) noexcept;

#endif /* PATHS_HPP */
