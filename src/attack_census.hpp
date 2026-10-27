/* Copyright (c) 2026 M.A.X. Port Team
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

#ifndef ATTACK_CENSUS_HPP
#define ATTACK_CENSUS_HPP

/* Attack census of debug builds, written into the AI log at each turn start of a computer team: what its armed mobile
 * units do and what its attacks hold. Release builds compile every hook to nothing.
 */

#if !defined(NDEBUG)

#include <cstdint>

void AttackCensus_Log(uint16_t team);
void AttackCensus_CountAssault(uint16_t team);
void AttackCensus_CountKillTaskEnd(uint16_t team);

#define ATTACK_CENSUS_LOG(team) AttackCensus_Log(team)
#define ATTACK_CENSUS_COUNT_ASSAULT(team) AttackCensus_CountAssault(team)
#define ATTACK_CENSUS_COUNT_KILL_TASK_END(team) AttackCensus_CountKillTaskEnd(team)

#else

#define ATTACK_CENSUS_LOG(team)
#define ATTACK_CENSUS_COUNT_ASSAULT(team)
#define ATTACK_CENSUS_COUNT_KILL_TASK_END(team)

#endif /* !defined(NDEBUG) */

#endif /* ATTACK_CENSUS_HPP */
