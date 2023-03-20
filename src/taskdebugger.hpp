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
    unsigned short team;
    SmartArray<Task> tasks;
    short row_index;
    short task_count;
    short row_count;
    Image *image;
    Button *button_up;
    Button *button_down;
    unsigned short first_row_r_value;
    unsigned short button_up_r_value;
    unsigned short button_down_r_value;
    ButtonManager button_manager;

    void InitTaskList(Task *task);
    void DrawRow(int uly, const char *caption, Task *task, int color);
    void SetLimits(int limit);

public:
    TaskDebugger(WindowInfo *win, Task *task, int button_up_value, int button_down_value, int first_row_value);
    ~TaskDebugger();

    bool ProcessKeyPress(int key);
    void DrawRows();
};

int TaskDebugger_GetDebugMode();
void TaskDebugger_SetDebugMode();

#endif /* TASKDEBUGGER_HPP */
