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

#ifndef TASKCLEARZONE_HPP
#define TASKCLEARZONE_HPP

#include "pathrequest.hpp"
#include "paths.hpp"
#include "task.hpp"

enum {
    CLEARZONE_STATE_WAITING,
    CLEARZONE_STATE_EXAMINING_ZONES,
    CLEARZONE_STATE_SEARCHING_MAP,
    CLEARZONE_STATE_WAITING_FOR_PATH,
    CLEARZONE_STATE_MOVING_UNIT,
};

class TaskClearZone : public Task {
    unsigned char state;
    SmartArray<Zone> zones;
    ObjectArray<ZoneSquare> zone_squares;
    ObjectArray<Point> points1;
    ObjectArray<Point> points2;
    SmartPointer<UnitInfo> moving_unit;
    unsigned int unit_flags;

    static void PathFindResultCallback(Task* task, PathRequest* request, Point point, GroundPath* path, char result);
    static void PathFindCancelCallback(Task* task, PathRequest* request);

    bool ExamineZones();
    bool IsNewSite(Point site);
    void EvaluateSite(ZoneSquare* zone_square, Point site);
    void SearchMap();

public:
    TaskClearZone(unsigned short team, unsigned int flags);
    ~TaskClearZone();

    int GetMemoryUse() const;
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    bool IsThinking();
    void Begin();
    void BeginTurn();
    void EndTurn();
    bool Task_vfunc17(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);

    void AddZone(Zone* zone);
};

#endif /* TASKCLEARZONE_HPP */
