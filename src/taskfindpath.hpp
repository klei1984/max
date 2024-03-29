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

#ifndef TASKFINDPATH_HPP
#define TASKFINDPATH_HPP

#include "pathrequest.hpp"
#include "task.hpp"

class TaskFindPath : public Task {
    SmartPointer<PathRequest> path_request;
    void (*result_callback)(Task* task, PathRequest* path_request, Point destination_, GroundPath* path,
                            uint8_t result);
    void (*cancel_callback)(Task* task, PathRequest* path_request);

public:
    TaskFindPath(Task* parent, PathRequest* request,
                 void (*result_callback)(Task* task, PathRequest* path_request, Point destination_, GroundPath* path,
                                         uint8_t result),
                 void (*cancel_callback)(Task* task, PathRequest* path_request));
    ~TaskFindPath();

    char* WriteStatusLog(char* buffer) const;
    uint8_t GetType() const;
    bool IsThinking();
    void Begin();
    void EndTurn();
    void RemoveSelf();
    void RemoveUnit(UnitInfo& unit);

    void CancelRequest();
    void Finish(Point position, GroundPath* path, bool result);
};

#endif /* TASKFINDPATH_HPP */
