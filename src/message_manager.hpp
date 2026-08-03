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

/**
 * \brief Selects the background tint of the in-game message box.
 *
 * The tint tables behind these styles are built per tileset, so the resulting hue is not fixed. Pick a style by the
 * meaning of the message, never by the color a particular tileset happens to produce.
 */
enum MessageBoxStyle : uint8_t {
    MESSAGE_BOX_INFO,     ///< Ordinary status text. Darkened background on every tileset.
    MESSAGE_BOX_NOTICE,   ///< Gameplay events worth attention: unit built, enemy spotted, chat.
    MESSAGE_BOX_WARNING,  ///< Failures and errors: network faults, invalid orders, save/load problems.
    MESSAGE_BOX_STYLE_COUNT
};

/**
 * \brief Selects how a message reaches the player.
 */
enum MessageBoxMode : uint8_t {
    MESSAGE_BOX_MODELESS,  ///< Paints the in-game message box and returns at once; the caller keeps running.
    MESSAGE_BOX_MODAL,     ///< Opens a WINDOW_MODAL dialog that runs its own event loop until dismissed.
};

void MessageManager_AddMessage(const char* text, ResourceID id);
void MessageManager_DrawMessage(const char* text, const MessageBoxStyle style, UnitInfo* unit, Point point);
void MessageManager_DrawMessage(const char* text, const MessageBoxStyle style, const MessageBoxMode mode,
                                bool center_align_text = false, bool save_to_log = false);
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
