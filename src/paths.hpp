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

#include "gnw.h"
#include "point.hpp"
#include "smartobjectarray.hpp"

class UnitInfo;
class NetPacket;

class UnitPath : public FileObject {
protected:
    int16_t x_end;
    int16_t y_end;
    int32_t distance_x;
    int32_t distance_y;
    int16_t euclidean_distance;

public:
    UnitPath();
    UnitPath(int32_t target_x, int32_t target_y);
    UnitPath(int32_t distance_x, int32_t distance_y, int32_t euclidean_distance, int32_t target_x, int32_t target_y);
    virtual ~UnitPath();

    virtual uint16_t GetTypeIndex() const = 0;
    virtual void FileLoad(SmartFileReader& file) noexcept = 0;
    virtual void FileSave(SmartFileWriter& file) noexcept = 0;
    virtual Point GetPosition(UnitInfo* unit) const;
    virtual bool IsInPath(int32_t grid_x, int32_t grid_y) const;
    virtual void CancelMovement(UnitInfo* unit);
    virtual int32_t GetMovementCost(UnitInfo* unit) = 0;
    virtual bool Path_vfunc10(UnitInfo* unit) = 0;
    virtual void UpdateUnitAngle(UnitInfo* unit);
    virtual void Path_vfunc12(int32_t unknown) = 0;
    virtual void Draw(UnitInfo* unit, WindowInfo* window) = 0;
    virtual bool IsEndStep() const;
    virtual void WritePacket(NetPacket& packet);
    virtual void ReadPacket(NetPacket& packet, int32_t steps_count);
    virtual void Path_vfunc17(int32_t distance_x, int32_t distance_y);

    int16_t GetEndX() const;
    int16_t GetEndY() const;
    int32_t GetDistanceX() const;
    int32_t GetDistanceY() const;
    int16_t GetEuclideanDistance() const;
    void SetEndXY(int32_t target_x, int32_t target_y);
};

class GroundPath : public UnitPath {
    uint16_t index;
    SmartObjectArray<PathStep> steps;

public:
    GroundPath();
    GroundPath(int32_t target_x, int32_t target_y);
    ~GroundPath();

    static FileObject* Allocate() noexcept;

    uint16_t GetTypeIndex() const;
    void FileLoad(SmartFileReader& file) noexcept;
    void FileSave(SmartFileWriter& file) noexcept;
    Point GetPosition(UnitInfo* unit) const;
    bool IsInPath(int32_t grid_x, int32_t grid_y) const;
    void CancelMovement(UnitInfo* unit);
    int32_t GetMovementCost(UnitInfo* unit);
    bool Path_vfunc10(UnitInfo* unit);
    void UpdateUnitAngle(UnitInfo* unit);
    void Path_vfunc12(int32_t unknown);
    void Draw(UnitInfo* unit, WindowInfo* window);
    bool IsEndStep() const;
    void WritePacket(NetPacket& packet);
    void ReadPacket(NetPacket& packet, int32_t steps_count);
    void Path_vfunc17(int32_t distance_x, int32_t distance_y);

    void AddStep(int32_t step_x, int32_t step_y);
    SmartObjectArray<PathStep> GetSteps();
    uint16_t GetPathStepIndex() const;
};

class AirPath : public UnitPath {
    int16_t length;
    char angle;
    int16_t pixel_x_start;
    int16_t pixel_y_start;
    int32_t x_step;
    int32_t y_step;
    int32_t delta_x;
    int32_t delta_y;

public:
    AirPath();
    AirPath(UnitInfo* unit, int32_t distance_x, int32_t distance_y, int32_t euclidean_distance, int32_t target_x,
            int32_t target_y);
    ~AirPath();

    static FileObject* Allocate() noexcept;

    uint16_t GetTypeIndex() const;
    void FileLoad(SmartFileReader& file) noexcept;
    void FileSave(SmartFileWriter& file) noexcept;
    Point GetPosition(UnitInfo* unit) const;
    void CancelMovement(UnitInfo* unit);
    int32_t GetMovementCost(UnitInfo* unit);
    bool Path_vfunc10(UnitInfo* unit);
    void Path_vfunc12(int32_t unknown);
    void Draw(UnitInfo* unit, WindowInfo* window);
};

class BuilderPath : public UnitPath {
    int16_t x;
    int16_t y;

public:
    BuilderPath();
    ~BuilderPath();

    static FileObject* Allocate() noexcept;

    uint16_t GetTypeIndex() const;
    void FileLoad(SmartFileReader& file) noexcept;
    void FileSave(SmartFileWriter& file) noexcept;
    int32_t GetMovementCost(UnitInfo* unit);
    bool Path_vfunc10(UnitInfo* unit);
    void Path_vfunc12(int32_t unknown);
    void Draw(UnitInfo* unit, WindowInfo* window);
};

bool Paths_RequestPath(UnitInfo* unit, int32_t mode);
AirPath* Paths_GetAirPath(UnitInfo* unit);
bool Paths_UpdateAngle(UnitInfo* unit, int32_t angle);
void Paths_DrawMarker(WindowInfo* window, int32_t angle, int32_t grid_x, int32_t grid_y, int32_t color);
void Paths_DrawShots(WindowInfo* window, int32_t grid_x, int32_t grid_y, int32_t shots);
bool Paths_IsOccupied(int32_t grid_x, int32_t grid_y, int32_t angle, int32_t team);

extern const Point Paths_8DirPointsArray[8];
extern const int16_t Paths_8DirPointsArrayX[8];
extern const int16_t Paths_8DirPointsArrayY[8];
extern uint32_t TickTimer_LastTimeStamp;
extern uint32_t Paths_DebugMode;
extern bool Paths_TimeBenchmarkDisable;
extern uint32_t Paths_TimeLimit;
extern uint32_t Paths_EvaluatedTileCount;
extern uint32_t Paths_EvaluatorCallCount;
extern uint32_t Paths_SquareAdditionsCount;
extern uint32_t Paths_SquareInsertionsCount;
extern uint32_t Paths_EvaluatedSquareCount;
extern uint32_t Paths_MaxDepth;

#endif /* PATHS_HPP */
