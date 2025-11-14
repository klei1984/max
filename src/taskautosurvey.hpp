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

#ifndef TASKAUTOSURVEY_HPP
#define TASKAUTOSURVEY_HPP

#include "task.hpp"

class TaskAutoSurvey : public Task {
    Point central_site;
    SmartPointer<UnitInfo> unit;
    uint16_t radius;
    uint16_t direction;
    Point position;
    Point destination;
    uint16_t direction2;
    bool continue_search;
    bool location_found;
    uint16_t current_radius;

public:
    TaskAutoSurvey(UnitInfo* unit);
    ~TaskAutoSurvey();

    std::string WriteStatusLog() const;
    uint8_t GetType() const;
    void Init();
    bool Execute(UnitInfo& unit);
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);
};

#endif /* TASKAUTOSURVEY_HPP */
