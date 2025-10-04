/* Copyright (c) 2023 M.A.X. Port Team
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

#ifndef TASKDEBUGGER_HPP
#define TASKDEBUGGER_HPP

#include "button.hpp"
#include "buttonmanager.hpp"
#include "task.hpp"

class TaskDebugger {
    WindowInfo window;
    uint16_t team;
    SmartArray<Task> tasks;
    uint32_t row_index;
    uint32_t task_count;
    uint32_t row_count;
    Image* image;
    Button* button_up;
    Button* button_down;
    uint16_t first_row_r_value;
    uint16_t button_up_r_value;
    uint16_t button_down_r_value;
    ButtonManager button_manager;

    void InitTaskList(Task* task);
    void DrawRow(int32_t uly, const char* caption, Task* task, int32_t color);
    void SetLimits(int32_t limit);

public:
    TaskDebugger(WindowInfo* win, Task* task, int32_t button_up_value, int32_t button_down_value,
                 int32_t first_row_value);
    ~TaskDebugger();

    bool ProcessKeyPress(uint32_t key);
    void DrawRows();
};

int32_t TaskDebugger_GetDebugMode();
void TaskDebugger_SetDebugMode();
void TaskDebugger_DebugBreak(int32_t task_id);

#endif /* TASKDEBUGGER_HPP */
