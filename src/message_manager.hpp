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

#ifndef MESSAGE_MANAGER_HPP
#define MESSAGE_MANAGER_HPP

#include "smartfile.hpp"
#include "unitinfo.hpp"

class World;

void MessageManager_AddMessage(const char* text, ResourceID id);
void MessageManager_DrawMessage(const char* text, uint8_t type, UnitInfo* unit, Point point);
void MessageManager_DrawMessage(const char* text, uint8_t type, int32_t mode, bool flag1 = false,
                                bool save_to_log = false);
void MessageManager_DrawMessageBox();
void MessageManager_ClearMessageBox();
void MessageManager_DrawTextMessage(WindowInfo* window, uint8_t* buffer, int32_t width, int32_t left_margin,
                                    int32_t top_margin, char* text, int32_t color, bool screen_refresh);
void MessageManager_LoadMessageLogs(SmartFileReader& file);
void MessageManager_SaveMessageLogs(SmartFileWriter& file);
void MessageManager_ClearMessageLogs();

/**
 * \brief Generates color lookup tables for semi-transparent message box backgrounds using tileset-specific RGB weights
 * from World instance.
 *
 * \param world Pointer to fully loaded World instance.
 */
void MessageManager_BuildMessageBoxColorTables(const World* world);

class MessageLogEntry : public SmartObject {
    ResourceID id;
    char* text;
    SmartPointer<UnitInfo> unit;
    Point point;
    bool is_alert_message;

public:
    MessageLogEntry(SmartFileReader& file);
    MessageLogEntry(const char* text, ResourceID id);
    MessageLogEntry(const char* text, UnitInfo* unit, Point point);

    virtual ~MessageLogEntry();

    void FileSave(SmartFileWriter& file);

    UnitInfo* GetUnit() const;
    Point GetPosition() const;
    char* GetCStr() const;
    ResourceID GetIcon() const;
    void Select();
};

extern bool MessageManager_MessageBox_IsActive;
extern SmartList<MessageLogEntry> MessageManager_TeamMessageLog[PLAYER_TEAM_MAX - 1];

#endif /* MESSAGE_MANAGER_HPP */
