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

#include "message_manager.hpp"

#include <algorithm>

#include "dialogmenu.hpp"
#include "game_manager.hpp"
#include "resource_manager.hpp"
#include "text.hpp"
#include "window_manager.hpp"

static_assert(sizeof(Point) == 4, "It is expected that Point is exactly 2+2 bytes int32_t.");
static_assert(sizeof(bool) == 1, "It is expected that bool is exactly 1 byte int8_t.");

#define MESSAGE_MANAGER_MAX_COUNT 50
#define MESSAGE_MANAGER_MESSAGE_BUFFER_SIZE 800
#define MESSAGE_MANAGER_TEXT_BUFFER_SIZE 300

char MessageManager_MessageBuffer[MESSAGE_MANAGER_MESSAGE_BUFFER_SIZE];
char MessageManager_TextBuffer[MESSAGE_MANAGER_TEXT_BUFFER_SIZE];

uint16_t MessageManager_MessageBuffer_Length;
int16_t MessageManager_MessageBox_Width;
int16_t MessageManager_MessageBox_Height;
ColorIndex* MessageManager_MessageBox_BgColor;
bool MessageManager_MessageBox_IsActive;

ColorIndex** MessageManager_MessageBox_BgColorArray[] = {
    &ResourceManager_ColorIndexTable12, &ResourceManager_ColorIndexTable10, &ResourceManager_ColorIndexTable06};

SmartList<MessageLogEntry> MessageManager_TeamMessageLog[PLAYER_TEAM_MAX - 1];

void MessageManager_WrapText(const char* text, int16_t width) {
    int32_t row_count{0};
    auto strings = Text_SplitText(text, 10, width, &row_count);

    for (int32_t i = 0; i < row_count; ++i) {
        auto stride{strings[i].GetLength() + 1};

        SDL_memcpy(&MessageManager_MessageBuffer[MessageManager_MessageBuffer_Length], strings[i].GetCStr(), stride);
        MessageManager_MessageBuffer_Length += stride;
        MessageManager_MessageBox_Width =
            std::max<int16_t>(Text_GetWidth(strings[i].GetCStr()), MessageManager_MessageBox_Width);
        MessageManager_MessageBox_Height += Text_GetHeight();
    }

    MessageManager_MessageBuffer[MessageManager_MessageBuffer_Length++] = '\0';

    delete[] strings;

    SDL_assert(MessageManager_MessageBuffer_Length < sizeof(MessageManager_MessageBuffer));
}

void MessageManager_DrawMessageBoxText(uint8_t* buffer, int32_t width, int32_t left_margin, int32_t top_margin,
                                       char* text, int32_t color, bool monospace) {
    int32_t flags;
    int32_t offset;

    offset = 0;
    Text_SetFont(GNW_TEXT_FONT_5);

    top_margin *= width;

    if (monospace) {
        flags = GNW_TEXT_MONOSPACE;
    } else {
        flags = 0;
    }

    color += flags + GNW_TEXT_OUTLINE;
    do {
        Text_Blit(&buffer[left_margin + top_margin], &text[offset], width, width, color);
        top_margin += 10 * width;
        offset += strlen(&text[offset]) + 1;
    } while (text[offset] != '\0');
}

void MessageManager_AddMessage(const char* text, ResourceID id) {
    MessageManager_TeamMessageLog[GameManager_PlayerTeam].PushBack(
        *dynamic_cast<MessageLogEntry*>(new (std::nothrow) MessageLogEntry(text, id)));

    if (MessageManager_TeamMessageLog[GameManager_PlayerTeam].GetCount() > MESSAGE_MANAGER_MAX_COUNT) {
        MessageManager_TeamMessageLog[GameManager_PlayerTeam].Remove(
            *MessageManager_TeamMessageLog[GameManager_PlayerTeam].Begin());
    }
}

void MessageManager_DrawMessage(const char* text, uint8_t type, UnitInfo* unit, Point point) {
    if (text[0] != '\0') {
        MessageManager_TeamMessageLog[GameManager_PlayerTeam].PushBack(
            *dynamic_cast<MessageLogEntry*>(new (std::nothrow) MessageLogEntry(text, unit, point)));

        if (MessageManager_TeamMessageLog[GameManager_PlayerTeam].GetCount() > MESSAGE_MANAGER_MAX_COUNT) {
            MessageManager_TeamMessageLog[GameManager_PlayerTeam].Remove(
                *MessageManager_TeamMessageLog[GameManager_PlayerTeam].Begin());
        }

        MessageManager_DrawMessage(text, type, 0);
    }
}

void MessageManager_DrawMessage(const char* text, uint8_t type, int32_t mode, bool flag1, bool save_to_log) {
    if (text && *text != '\0') {
        if (mode) {
            DialogMenu dialog(text, flag1);
            dialog.Run();
        } else {
            WindowInfo* window_message_box;
            WindowInfo* window_main_map;
            int32_t width;
            int32_t offset_x;
            int32_t offset_y;
            Rect bounds;

            if (MessageManager_MessageBox_IsActive) {
                MessageManager_ClearMessageBox();
            }

            if (save_to_log) {
                MessageManager_TeamMessageLog[GameManager_PlayerTeam].PushBack(
                    *dynamic_cast<MessageLogEntry*>(new (std::nothrow) MessageLogEntry(text, I_CMPLX)));

                if (MessageManager_TeamMessageLog[GameManager_PlayerTeam].GetCount() > MESSAGE_MANAGER_MAX_COUNT) {
                    MessageManager_TeamMessageLog[GameManager_PlayerTeam].Remove(
                        *MessageManager_TeamMessageLog[GameManager_PlayerTeam].Begin());
                }
            }

            Text_SetFont(GNW_TEXT_FONT_5);
            window_main_map = WindowManager_GetWindow(WINDOW_MAIN_MAP);
            width = window_main_map->window.lrx - window_main_map->window.ulx;

            MessageManager_MessageBox_Width = 0;
            MessageManager_MessageBuffer_Length = 0;
            MessageManager_MessageBox_Height = 20;

            MessageManager_WrapText(text, width - 20);

            MessageManager_MessageBuffer[MessageManager_MessageBuffer_Length] = '\0';
            MessageManager_MessageBox_Width += 20;

            offset_x = 0;
            offset_y = 10;

            window_message_box = WindowManager_GetWindow(WINDOW_MESSAGE_BOX);

            window_message_box->window.ulx = window_main_map->window.ulx + offset_x;
            window_message_box->window.uly = window_main_map->window.uly + offset_y;
            window_message_box->window.lrx = window_message_box->window.ulx + MessageManager_MessageBox_Width;
            window_message_box->window.lry = window_message_box->window.uly + MessageManager_MessageBox_Height;

            window_message_box->buffer = &window_main_map->buffer[offset_x + window_message_box->width * offset_y];

            MessageManager_MessageBox_BgColor = *MessageManager_MessageBox_BgColorArray[type];
            MessageManager_MessageBox_IsActive = true;

            GameManager_GetScaledMessageBoxBounds(&bounds);
            GameManager_AddDrawBounds(&bounds);
        }
    }
}

void MessageManager_DrawMessageBox() {
    WindowInfo* window;
    int32_t height;
    int32_t fullw;
    int32_t row;

    window = WindowManager_GetWindow(WINDOW_MESSAGE_BOX);

    for (height = MessageManager_MessageBox_Height, fullw = 0; height > 0; --height, fullw += window->width) {
        for (row = 0; row < MessageManager_MessageBox_Width; ++row) {
            window->buffer[row + fullw] = MessageManager_MessageBox_BgColor[window->buffer[row + fullw]];
        }
    }

    MessageManager_DrawMessageBoxText(window->buffer, window->width, 10, 10, MessageManager_MessageBuffer, 0xFF, false);
}

void MessageManager_ClearMessageBox() {
    WindowInfo* window;
    Rect bounds;

    GameManager_GetScaledMessageBoxBounds(&bounds);
    GameManager_AddDrawBounds(&bounds);

    window = WindowManager_GetWindow(WINDOW_MESSAGE_BOX);

    window->window.ulx = -1;
    window->window.uly = -1;
    window->window.lrx = -1;
    window->window.lry = -1;

    MessageManager_MessageBox_IsActive = false;
}

void MessageManager_DrawTextMessage(WindowInfo* window, uint8_t* buffer, int32_t width, int32_t left_margin,
                                    int32_t top_margin, char* text, int32_t color, bool screen_refresh) {
    int32_t text_position = 0;
    int32_t buffer_position = 0;

    do {
        if (text[text_position] == '\n') {
            MessageManager_TextBuffer[buffer_position] = ' ';
            ++buffer_position;
            MessageManager_TextBuffer[buffer_position] = '\0';
            ++buffer_position;
        } else {
            MessageManager_TextBuffer[buffer_position] = text[text_position];
            ++buffer_position;
        }
    } while (text[text_position++] != '\0');

    MessageManager_TextBuffer[buffer_position] = '\0';

    MessageManager_DrawMessageBoxText(buffer, width, left_margin, top_margin, MessageManager_TextBuffer, color, false);

    if (screen_refresh) {
        win_draw_rect(window->id, &window->window);
    }
}

void MessageManager_LoadMessageLogs(SmartFileReader& file) {
    for (int32_t i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
        MessageManager_TeamMessageLog[i].Clear();

        for (int32_t count = file.ReadObjectCount(); count > 0; --count) {
            MessageManager_TeamMessageLog[i].PushBack(
                *dynamic_cast<MessageLogEntry*>(new (std::nothrow) MessageLogEntry(file)));
        }
    }
}

void MessageManager_SaveMessageLogs(SmartFileWriter& file) {
    for (int32_t i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
        file.WriteObjectCount(MessageManager_TeamMessageLog[i].GetCount());

        for (SmartList<MessageLogEntry>::Iterator it = MessageManager_TeamMessageLog[i].Begin();
             it != MessageManager_TeamMessageLog[i].End(); ++it) {
            (*it).FileSave(file);
        }
    }
}

void MessageManager_ClearMessageLogs() {
    for (int32_t i = 0; i < PLAYER_TEAM_MAX - 1; ++i) {
        MessageManager_TeamMessageLog[i].Clear();
    }
}

MessageLogEntry::MessageLogEntry(SmartFileReader& file) {
    uint16_t length;

    file.Read(length);
    text = new (std::nothrow) char[length];
    file.Read(text, length);
    unit = dynamic_cast<UnitInfo*>(file.ReadObject());
    file.Read(point);
    file.Read(is_alert_message);
    file.Read(id);
}

MessageLogEntry::MessageLogEntry(const char* text, ResourceID id)
    : id(id), text(strdup(text)), is_alert_message(false) {}

MessageLogEntry::MessageLogEntry(const char* text, UnitInfo* unit, Point point)
    : text(strdup(text)), unit(unit), point(point), is_alert_message(true), id(INVALID_ID) {}

MessageLogEntry::~MessageLogEntry() { delete text; }

void MessageLogEntry::FileSave(SmartFileWriter& file) {
    uint16_t length = strlen(text) + 1;

    file.Write(length);
    file.Write(text, length);
    file.WriteObject(&*unit);
    file.Write(point);
    file.Write(is_alert_message);
    file.Write(id);
}

char* MessageLogEntry::GetCStr() const { return text; }

void MessageLogEntry::Select() {
    MessageManager_DrawMessage(text, 0, 0);

    if (is_alert_message) {
        if (unit != nullptr && unit->hits > 0 && unit->IsVisibleToTeam(GameManager_PlayerTeam)) {
            GameManager_MenuUnitSelect(&*unit);
        } else {
            GameManager_UpdateMainMapView(MAP_VIEW_CENTER, point.x, point.y);
        }
    }
}

UnitInfo* MessageLogEntry::GetUnit() const { return &*unit; }

Point MessageLogEntry::GetPosition() const { return point; }

ResourceID MessageLogEntry::GetIcon() const { return id; }
