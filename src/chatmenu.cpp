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

#include "chatmenu.hpp"

#include "button.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "remote.hpp"
#include "text.hpp"
#include "textedit.hpp"
#include "units_manager.hpp"
#include "window.hpp"
#include "window_manager.hpp"

class ChatMenu : public Window {
    WindowInfo window;
    Button *button_cancel;
    Button *button_send;
    Button *button_help;
    Button *button_team[PLAYER_TEAM_MAX - 1];
    TextEdit *text_edit;
    uint16_t team;
    bool event_click_done_cancel;
    bool event_release;
    char text[60];

    bool ProcessKey(int32_t key);
    void DrawMessage();

public:
    ChatMenu(uint16_t team);
    ~ChatMenu();

    void Run();
};

ChatMenu::ChatMenu(uint16_t team) : Window(CHATWNDO, WINDOW_MAIN_MAP) {
    Cursor_SetCursor(CURSOR_HAND);
    Text_SetFont(GNW_TEXT_FONT_5);

    this->team = team;
    event_release = false;

    Add();
    FillWindowInfo(&window);

    button_cancel = new (std::nothrow) Button(CHTCAN_U, CHTCAN_D, 139, 125);
    button_send = new (std::nothrow) Button(CHTSND_U, CHTSND_D, 229, 125);
    button_help = new (std::nothrow) Button(CHTHLP_U, CHTHLP_D, 202, 125);

    button_team[PLAYER_TEAM_RED] = new (std::nothrow) Button(CHTRED_U, CHTRED_D, 25, 14);
    button_team[PLAYER_TEAM_GREEN] = new (std::nothrow) Button(CHTGRN_U, CHTGRN_D, 222, 14);
    button_team[PLAYER_TEAM_BLUE] = new (std::nothrow) Button(CHTBLU_U, CHTBLU_D, 25, 49);
    button_team[PLAYER_TEAM_GRAY] = new (std::nothrow) Button(CHTGRY_U, CHTGRY_D, 222, 49);

    button_send->SetCaption(_(f6f4));
    button_send->SetRValue(GNW_KB_KEY_RETURN);
    button_send->SetPValue(GNW_KB_KEY_RETURN + GNW_INPUT_PRESS);
    button_send->SetSfx(NDONE0);
    button_send->RegisterButton(window.id);

    button_cancel->SetCaption(_(0de3));
    button_cancel->SetRValue(GNW_KB_KEY_ESCAPE);
    button_cancel->SetPValue(GNW_KB_KEY_ESCAPE + GNW_INPUT_PRESS);
    button_cancel->SetSfx(NCANC0);
    button_cancel->RegisterButton(window.id);

    button_help->SetRValue(1000);
    button_help->SetPValue(1000 + GNW_INPUT_PRESS);
    button_help->SetSfx(NHELP0);
    button_help->RegisterButton(window.id);

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_REMOTE) {
            button_team[i]->SetPValue(i + 1001);
            button_team[i]->SetRValue(i + 1001);
            button_team[i]->SetFlags(0x01);
            button_team[i]->SetSfx(KCARG0);
            button_team[i]->SetRestState(GameManager_MultiChatTargets[i]);
        }

        button_team[i]->RegisterButton(window.id);
    }

    text[0] = '\0';
    text_edit = new (std::nothrow) TextEdit(&window, text, sizeof(text), 25, 87, 380, 25, 0xA2, GNW_TEXT_FONT_5);
    text_edit->SetMode(0);
    text_edit->LoadBgImage();
    text_edit->SetEditedText(text);
    text_edit->DrawFullText();
    text_edit->EnterTextEditField();

    DrawMessage();
}

ChatMenu::~ChatMenu() {
    delete button_cancel;
    delete button_send;
    delete button_help;

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        delete button_team[i];
    }

    delete text_edit;

    GameManager_ProcessTick(true);
}

bool ChatMenu::ProcessKey(int32_t key) {
    if (key > 0 && key < GNW_INPUT_PRESS) {
        event_click_done_cancel = false;
    }

    switch (key) {
        case GNW_KB_KEY_LALT_P: {
            PauseMenu_Menu();
        } break;

        case GNW_KB_KEY_RETURN: {
            text_edit->AcceptEditedText();

            for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
                GameManager_MultiChatTargets[i] = win_button_down(button_team[i]->GetId());

                if (GameManager_MultiChatTargets[i]) {
                    Remote_SendNetPacket_18(team, i, text);
                }
            }

            MessageManager_AddMessage(text, LIPS);
            text_edit->LeaveTextEditField();
            event_click_done_cancel = true;

        } break;

        case GNW_KB_KEY_ESCAPE: {
            text_edit->LeaveTextEditField();
            event_click_done_cancel = true;

        } break;

        case 1000: {
            HelpMenu_Menu(HELPMENU_CHAT_MENU_SETUP, WINDOW_MAIN_MAP);
        } break;

        case 1001 + PLAYER_TEAM_RED:
        case 1001 + PLAYER_TEAM_GREEN:
        case 1001 + PLAYER_TEAM_BLUE:
        case 1001 + PLAYER_TEAM_GRAY: {
            button_team[key - 1001]->PlaySound();
            DrawMessage();
        } break;

        default: {
            if (key < GNW_INPUT_PRESS) {
                text_edit->ProcessKeyPress(key);

            } else if (!event_release) {
                event_release = true;

                switch (key) {
                    case GNW_INPUT_PRESS + GNW_KB_KEY_RETURN: {
                        button_send->PlaySound();
                    } break;

                    case GNW_INPUT_PRESS + 1000: {
                        button_help->PlaySound();
                    } break;

                    case GNW_INPUT_PRESS + GNW_KB_KEY_ESCAPE: {
                        button_cancel->PlaySound();
                    } break;
                }
            }
        } break;
    }

    return true;
}

void ChatMenu::DrawMessage() {
    char buffer[30];

    for (int32_t i = PLAYER_TEAM_RED; i < PLAYER_TEAM_MAX - 1; ++i) {
        if (UnitsManager_TeamInfo[i].team_type == TEAM_TYPE_REMOTE) {
            int32_t team_ulx;
            int32_t team_uly;

            ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + i), buffer, sizeof(buffer));

            switch (i) {
                case PLAYER_TEAM_RED: {
                    team_ulx = 66;
                    team_uly = 22;
                } break;

                case PLAYER_TEAM_GREEN: {
                    team_ulx = 262;
                    team_uly = 22;
                } break;

                case PLAYER_TEAM_BLUE: {
                    team_ulx = 262;
                    team_uly = 57;
                } break;

                case PLAYER_TEAM_GRAY: {
                    team_ulx = 66;
                    team_uly = 57;
                } break;
            }

            Text_TextBox(window.buffer, window.width, buffer, team_ulx, team_uly, 130, 10,
                         win_button_down(button_team[i]->GetId()) ? 0xFF : COLOR_RED, true);
        }
    }

    win_draw_rect(window.id, &window.window);
}

void ChatMenu::Run() {
    int32_t key;

    event_click_done_cancel = false;

    while (!event_click_done_cancel) {
        key = get_input();

        if (GameManager_RequestMenuExit) {
            key = GNW_KB_KEY_ESCAPE;
        }

        ProcessKey(key);
        GameManager_ProcessState(true);
    }
}

void ChatMenu_Menu(uint16_t team) { ChatMenu(team).Run(); }
