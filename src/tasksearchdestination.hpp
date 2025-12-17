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

#ifndef TASKSEARCHDESTINATION_HPP
#define TASKSEARCHDESTINATION_HPP

#include "taskabstractsearch.hpp"

class TaskSearchDestination : public Task {
    SmartPointer<UnitInfo> unit;
    Point initial_position;
    TaskAbstractSearch* search_task;
    uint16_t search_radius;
    int16_t index;
    Point spiral_cursors[2];
    int16_t directions[2];
    Point spiral_center;
    Point spiral_target;

    int16_t radius;
    int16_t previous_radius;
    int16_t spiral_direction;

    bool continue_search_at_radius;
    bool is_doomed;

    int16_t valid_sites;
    int16_t searched_sites;
    int16_t enterable_sites;

    void FinishSearch();
    void SearchNextCircle();
    bool Search();
    void SearchTrySite();
    bool sub_3DFCF(UnitInfo* unit, Point point);
    void ResumeSearch();

    static void CloseMoveFinishedCallback(Task* task, UnitInfo* unit, char result);
    static void FarMoveFinishedCallback(Task* task, UnitInfo* unit, char result);

public:
    TaskSearchDestination(Task* task, UnitInfo* unit, int32_t radius);
    ~TaskSearchDestination();

    std::string WriteStatusLog() const;
    uint8_t GetType() const;
    void Init();
    void EndTurn();
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);
};

#endif /* TASKSEARCHDESTINATION_HPP */
