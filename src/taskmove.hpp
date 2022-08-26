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

#ifndef TASKMOVE_HPP
#define TASKMOVE_HPP

#include "pathrequest.hpp"
#include "paths.hpp"
#include "task.hpp"

class TaskMove : public Task {
    SmartPointer<UnitInfo> passenger;
    unsigned short minimum_distance;
    unsigned char caution_level;
    Point point1;
    Point point2;
    Point point3;
    ResourceID transporter_unit_type;
    Point destination;
    ObjectArray<Point> planned_path;
    SmartPointer<Zone> zone;
    void (*result_callback)(Task* task, UnitInfo* unit, char result);
    unsigned char field_68;
    unsigned char field_69;
    unsigned char field_70;
    unsigned char field_71;
    unsigned char field_72;

    void AttemptTransport();
    void AttemptTransportType(ResourceID unit_type);
    void TaskMove_sub_4C6BA();
    void Search(bool mode);
    void Finished(int result);
    void ProcessPath(Point site, GroundPath* path);
    void RetryMove();
    void FindTransport(ResourceID unit_type);
    void TranscribeTransportPath(Point site, GroundPath* path);
    bool CheckAirPath();
    void MoveAirUnit();
    bool IsPathClear();
    void FindCurrentLocation();
    bool FindWaypoint();
    void MoveUnit(GroundPath* path);

    static UnitInfo* FindUnit(SmartList<UnitInfo>* units, unsigned short team, ResourceID unit_type);
    static void PathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                   char result);
    static void FullPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                       char result);
    static void DirectPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                         char result);
    static void BlockedPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                          char result);
    static void ActualPathResultCallback(Task* task, PathRequest* path_request, Point destination, GroundPath* path,
                                         char result);
    static void PathCancelCallback(Task* task, PathRequest* path_request);

public:
    TaskMove(UnitInfo* unit, Task* task, unsigned short minimum_distance, unsigned char caution_level,
             Point destination, void (*result_callback)(Task* task, UnitInfo* unit, char result));
    TaskMove(UnitInfo* unit, void (*result_callback)(Task* task, UnitInfo* unit, char result));
    ~TaskMove();

    int GetCautionLevel(UnitInfo& unit);
    int GetMemoryUse() const;
    char* WriteStatusLog(char* buffer) const;
    unsigned char GetType() const;
    void Task_vfunc11(UnitInfo& unit);
    void AddReminder();
    void BeginTurn();
    void EndTurn();
    bool Task_vfunc17(UnitInfo& unit);
    void RemoveSelf();
    void Remove(UnitInfo& unit);
    void Task_vfunc27(Zone* zone, char mode);

    void SetField68(bool value);
    UnitInfo* GetPassenger();
    Point GetDestination();
    void SetDestination(Point site);
    ResourceID GetTransporterType() const;
    Point GetPoint2() const;
    void TaskMove_sub_4C66B(bool mode);
    bool TaskMove_sub_4D247();
};

#endif /* TASKMOVE_HPP */
