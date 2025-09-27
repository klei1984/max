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

#include "taskdebugger.hpp"

#include "ailog.hpp"
#include "game_manager.hpp"
#include "message_manager.hpp"
#include "task_manager.hpp"
#include "text.hpp"

#define TASK_DEBUGGER_INVALID_ID (-1)

static int32_t TaskDebugger_DebugMode;
static int32_t TaskDebugger_DebugTask = TASK_DEBUGGER_INVALID_ID;

TaskDebugger::TaskDebugger(WindowInfo *win, Task *task, int32_t button_up_value, int32_t button_down_value,
                           int32_t first_row_value) {
    TaskDebugger_DebugTask = TASK_DEBUGGER_INVALID_ID;

    Text_SetFont(GNW_TEXT_FONT_5);

    window = *win;

    button_up_r_value = button_up_value;
    button_down_r_value = button_down_value;
    first_row_r_value = first_row_value;

    team = task->GetTeam();

    row_count = (window.window.lry - window.window.uly) / (Text_GetHeight() * 2);

    int32_t width = window.window.lrx - window.window.ulx;
    int32_t height = Text_GetHeight() * row_count * 2;

    image = new (std::nothrow) Image(0, 0, width, height);
    image->Copy(&window);

    button_up = new (std::nothrow) Button(BLDUP__U, BLDUP__D, 336, 21);
    button_up->CopyUpDisabled(BLDUP__X);
    button_up->SetRValue(button_up_r_value);
    button_up->SetPValue(GNW_INPUT_PRESS);
    button_up->RegisterButton(window.id);

    button_down = new (std::nothrow) Button(BLDDWN_U, BLDDWN_D, 336, 233);
    button_down->CopyUpDisabled(BLDDWN_X);
    button_down->SetRValue(button_down_r_value);
    button_down->SetPValue(GNW_INPUT_PRESS + 1);
    button_down->RegisterButton(window.id);

    for (uint32_t i = 0; i < row_count; ++i) {
        button_manager.Add(win_register_button(
            window.id, window.window.ulx, window.window.uly + Text_GetHeight() * i * 2, width, Text_GetHeight() * 2, -1,
            -1, -1, first_row_value + i, nullptr, nullptr, nullptr, 0x00));
    }

    InitTaskList(task);
}

TaskDebugger::~TaskDebugger() {
    delete image;
    delete button_up;
    delete button_down;
}

void TaskDebugger::InitTaskList(Task *task) {
    tasks.Release();

    tasks.Insert(nullptr);

    if (task) {
        SmartPointer<Task> parent(task->GetParent());

        while (parent) {
            tasks.Insert(&*parent, 1);
            parent = parent->GetParent();
        }

        task_count = tasks.GetCount();

        tasks.Insert(task);

    } else {
        task_count = 0;
    }

    for (SmartList<Task>::Iterator it = TaskManager.GetTaskList().Begin(); it != TaskManager.GetTaskList().End();
         ++it) {
        if ((*it).GetTeam() == team && (*it).GetParent() == task) {
            tasks.Insert(&*it);
        }
    }

    SetLimits(task_count);
}

void TaskDebugger::DrawRow(int32_t uly, const char *caption, Task *task, int32_t color) {
    char text[100];
    int32_t full_width = Text_GetWidth(caption);

    Text_Blit(&window.buffer[window.width * (Text_GetHeight() / 2 + uly)], caption, image->GetWidth(), window.width,
              color);

    if (task) {
        task->WriteStatusLog(text);

    } else {
        sprintf(text, "Task Manager (%i reminders queued)", TaskManager.GetRemindersCount());
    }

    Text_TextBox(window.buffer, window.width, text, full_width, uly, image->GetWidth() - full_width,
                 Text_GetHeight() * 2, color, false);
}

void TaskDebugger::DrawRows() {
    Text_SetFont(GNW_TEXT_FONT_5);

    image->Write(&window);

    uint64_t limit = row_index + row_count;

    if (tasks.GetCount() > limit) {
        button_down->Enable();

    } else {
        limit = tasks.GetCount();

        button_down->Disable();
    }

    if (row_index) {
        button_up->Enable();

    } else {
        button_up->Disable();
    }

    int32_t uly = 0;

    for (uint64_t i = row_index; i < limit; ++i) {
        if (i < task_count) {
            DrawRow(uly, "Parent: ", &tasks[i], COLOR_GREEN);

        } else if (i == task_count) {
            DrawRow(uly, "Task: ", &tasks[i], 48);

        } else {
            DrawRow(uly, "Child: ", &tasks[i], COLOR_YELLOW);
        }

        uly += Text_GetHeight() * 2;
    }

    win_draw(window.id);
}

void TaskDebugger::SetLimits(int32_t limit) {
    int64_t index = row_index;

    if (index > limit) {
        index = limit;
    }

    if (index + row_count <= limit) {
        index = limit - static_cast<int64_t>(row_count) + 1;
    }

    if (index + row_count > tasks.GetCount()) {
        index = static_cast<int64_t>(tasks.GetCount()) - row_count;
    }

    if (index < 0) {
        index = 0;
    }

    row_index = index;
}

int32_t TaskDebugger_GetDebugMode() { return TaskDebugger_DebugMode; }

void TaskDebugger_SetDebugMode() {
    TaskDebugger_DebugMode = (TaskDebugger_DebugMode + 1) % 3;

    switch (TaskDebugger_DebugMode) {
        case 0: {
            MessageManager_DrawMessage("Task Debug info OFF.", 0, 0);
            AiLog_Close();
        } break;

        case 1: {
            MessageManager_DrawMessage("Task Debug info ON.", 0, 0);
        } break;

        case 2: {
            MessageManager_DrawMessage("Task Log ON.", 0, 0);
            AiLog_Open();
        } break;
    }
}

void TaskDebugger_DebugBreak(int32_t task_id) {
    if (task_id == TaskDebugger_DebugTask) {
        TaskDebugger_DebugTask = TASK_DEBUGGER_INVALID_ID;
        SDL_TriggerBreakpoint();
    }
}

bool TaskDebugger::ProcessKeyPress(uint32_t key) {
    bool result;

    if (first_row_r_value <= key && first_row_r_value + row_count > key) {
        int64_t index = key + static_cast<int64_t>(row_index) - first_row_r_value;

        if (tasks.GetCount() > index) {
            if (index == task_count) {
                GameManager_RequestMenuExit = true;
                TaskDebugger_DebugTask = tasks[task_count].GetId();

            } else {
                task_count = index;

                InitTaskList(&tasks[task_count]);
                DrawRows();
            }
        }

        result = true;

    } else if (button_down_r_value == key) {
        int64_t index = row_index;

        index += static_cast<int64_t>(row_count) - 1;

        if (index + row_count > tasks.GetCount()) {
            index = static_cast<int64_t>(tasks.GetCount()) - row_count;

            if (index < 0) {
                index = 0;
            }
        }

        row_index = index;

        DrawRows();

        result = true;

    } else if (key == button_up_r_value) {
        int64_t index = row_index;

        index -= static_cast<int64_t>(row_count) - 1;

        if (index < 0) {
            index = 0;
        }

        row_index = index;

        DrawRows();

        result = true;

    } else {
        result = false;
    }

    return result;
}
