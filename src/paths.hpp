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

class UnitPath : public TextFileObject {
protected:
    short x_end;
    short y_end;
    int distance_x;
    int distance_y;
    short euclidean_distance;

public:
    UnitPath();
    UnitPath(int target_x, int target_y);
    UnitPath(int distance_x, int distance_y, int euclidean_distance, int target_x, int target_y);
    virtual ~UnitPath();

    virtual unsigned short GetTypeIndex() const = 0;
    virtual void FileLoad(SmartFileReader& file) = 0;
    virtual void FileSave(SmartFileWriter& file) = 0;
    virtual void TextLoad(TextStructure& object) = 0;
    virtual void TextSave(SmartTextfileWriter& file) = 0;
    virtual Point GetPosition(UnitInfo* unit) const;
    virtual bool IsInPath(int grid_x, int grid_y) const;
    virtual void Path_vfunc8(UnitInfo* unit);
    virtual int GetMovementCost(UnitInfo* unit) = 0;
    virtual bool Path_vfunc10(UnitInfo* unit) = 0;
    virtual void UpdateUnitAngle(UnitInfo* unit);
    virtual void Path_vfunc12(int unknown) = 0;
    virtual void Draw(UnitInfo* unit, WindowInfo* window) = 0;
    virtual bool IsEndStep() const;
    virtual void WritePacket(NetPacket& packet);
    virtual void ReadPacket(NetPacket& packet, int steps_count);
    virtual void Path_vfunc17(int distance_x, int distance_y);

    short GetEndX() const;
    short GetEndY() const;
    int GetDistanceX() const;
    int GetDistanceY() const;
    short GetEuclideanDistance() const;
    void SetEndXY(int target_x, int target_y);
};

class GroundPath : public UnitPath {
    unsigned short index;
    SmartObjectArray<PathStep> steps;

public:
    GroundPath();
    GroundPath(int target_x, int target_y);
    ~GroundPath();

    static TextFileObject* Allocate();

    unsigned short GetTypeIndex() const;
    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);
    Point GetPosition(UnitInfo* unit) const;
    bool IsInPath(int grid_x, int grid_y) const;
    void Path_vfunc8(UnitInfo* unit);
    int GetMovementCost(UnitInfo* unit);
    bool Path_vfunc10(UnitInfo* unit);
    void UpdateUnitAngle(UnitInfo* unit);
    void Path_vfunc12(int unknown);
    void Draw(UnitInfo* unit, WindowInfo* window);
    bool IsEndStep() const;
    void WritePacket(NetPacket& packet);
    void ReadPacket(NetPacket& packet, int steps_count);
    void Path_vfunc17(int distance_x, int distance_y);

    void AddStep(int step_x, int step_y);
    SmartObjectArray<PathStep> GetSteps();
    unsigned short GetPathStepIndex() const;
};

class AirPath : public UnitPath {
    short length;
    char angle;
    short pixel_x_start;
    short pixel_y_start;
    int x_step;
    int y_step;
    int delta_x;
    int delta_y;

public:
    AirPath();
    AirPath(UnitInfo* unit, int distance_x, int distance_y, int euclidean_distance, int target_x, int target_y);
    ~AirPath();

    static TextFileObject* Allocate();

    unsigned short GetTypeIndex() const;
    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);
    Point GetPosition(UnitInfo* unit) const;
    void Path_vfunc8(UnitInfo* unit);
    int GetMovementCost(UnitInfo* unit);
    bool Path_vfunc10(UnitInfo* unit);
    void Path_vfunc12(int unknown);
    void Draw(UnitInfo* unit, WindowInfo* window);
};

class BuilderPath : public UnitPath {
    short x;
    short y;

public:
    BuilderPath();
    ~BuilderPath();

    static TextFileObject* Allocate();

    unsigned short GetTypeIndex() const;
    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);
    int GetMovementCost(UnitInfo* unit);
    bool Path_vfunc10(UnitInfo* unit);
    void Path_vfunc12(int unknown);
    void Draw(UnitInfo* unit, WindowInfo* window);
};

bool Paths_RequestPath(UnitInfo* unit, int mode);
AirPath* Paths_GetAirPath(UnitInfo* unit);
bool Paths_UpdateAngle(UnitInfo* unit, int angle);
void Paths_DrawMarker(WindowInfo* window, int angle, int grid_x, int grid_y, int color);
void Paths_DrawShots(WindowInfo* window, int grid_x, int grid_y, int shots);
bool Paths_IsOccupied(int grid_x, int grid_y, int angle, int team);

extern const Point Paths_8DirPointsArray[8];
extern const short Paths_8DirPointsArrayX[8];
extern const short Paths_8DirPointsArrayY[8];
extern unsigned int Paths_LastTimeStamp;
extern unsigned int Paths_DebugMode;
extern bool Paths_TimeBenchmarkDisable;
extern unsigned int Paths_TimeLimit;
extern unsigned int Paths_EvaluatedTileCount;
extern unsigned int Paths_EvaluatorCallCount;
extern unsigned int Paths_SquareAdditionsCount;
extern unsigned int Paths_SquareInsertionsCount;
extern unsigned int Paths_EvaluatedSquareCount;
extern unsigned int Paths_MaxDepth;

#endif /* PATHS_HPP */
