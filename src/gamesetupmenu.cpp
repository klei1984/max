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

#include "gamesetupmenu.hpp"

#include <filesystem>

#include "helpmenu.hpp"
#include "inifile.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "resource_manager.hpp"
#include "saveloadmenu.hpp"
#include "text.hpp"
#include "window_manager.hpp"

struct GameSetupMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char* label;
    int32_t event_code;
    void (GameSetupMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    {{(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx)}

#define GAME_SETUP_MENU_MISSION_COUNT 12

static struct MenuTitleItem game_setup_menu_titles[] = {
    MENU_TITLE_ITEM_DEF(400, 174, 580, 195, nullptr),
    MENU_TITLE_ITEM_DEF(354, 215, 620, 416, nullptr),
};

static struct GameSetupMenuControlItem game_setup_menu_controls[] = {
    MENU_CONTROL_DEF(18, 184, 312, 203, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 204, 312, 223, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 224, 312, 243, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 244, 312, 263, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 264, 312, 283, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 284, 312, 303, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 304, 312, 323, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 324, 312, 343, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 344, 312, 363, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 364, 312, 383, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 384, 312, 403, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(18, 404, 312, 423, INVALID_ID, nullptr, 0, &GameSetupMenu::EventSelectItem, MBUTT0),
    MENU_CONTROL_DEF(16, 438, 0, 0, MNUUAROU, nullptr, 0, &GameSetupMenu::EventScrollButton, MBUTT0),
    MENU_CONTROL_DEF(48, 438, 0, 0, MNUDAROU, nullptr, 0, &GameSetupMenu::EventScrollButton, MBUTT0),
    MENU_CONTROL_DEF(563, 438, 0, 0, MNUUAROU, nullptr, 0, &GameSetupMenu::EventBriefingButton, MBUTT0),
    MENU_CONTROL_DEF(596, 438, 0, 0, MNUDAROU, nullptr, 0, &GameSetupMenu::EventBriefingButton, MBUTT0),
    MENU_CONTROL_DEF(200, 438, 0, 0, MNUBTN4U, _(639d), GNW_KB_KEY_ESCAPE, &GameSetupMenu::EventCancel, NCANC0),
    MENU_CONTROL_DEF(312, 438, 0, 0, MNUBTN5U, _(da8e), GNW_KB_KEY_SHIFT_DIVIDE, &GameSetupMenu::EventHelp, NHELP0),
    MENU_CONTROL_DEF(361, 438, 0, 0, MNUBTN6U, _(f0a3), GNW_KB_KEY_RETURN, &GameSetupMenu::EventStart, NDONE0),
};

static char* menu_setup_menu_mission_titles[GAME_SETUP_MENU_MISSION_COUNT];

static void GameSetupMenu_LoadSubtitleControl(WindowInfo* window) {
    WindowManager_LoadSimpleImage(SUBTITLE, WindowManager_ScaleUlx(window, 400), WindowManager_ScaleUly(window, 174),
                                  false, window);
}

static void GameSetupMenu_DrawBigInfoPanel(WindowInfo* window) {
    WindowManager_LoadSimpleImage(BIGSCRNL, WindowManager_ScaleUlx(window, 342), WindowManager_ScaleUly(window, 202),
                                  false, window);
    WindowManager_LoadSimpleImage(BIGSCRNR, WindowManager_ScaleUlx(window, 482), WindowManager_ScaleUly(window, 202),
                                  false, window);
}

void GameSetupMenu::ButtonInit(int32_t index) {
    GameSetupMenuControlItem* control;

    control = &game_setup_menu_controls[index];

    Text_SetFont((index < 16) ? GNW_TEXT_FONT_5 : GNW_TEXT_FONT_1);

    if (control->image_id == INVALID_ID) {
        if (index >= GAME_SETUP_MENU_MISSION_COUNT) {
            return;
        }

        menu_setup_menu_mission_titles[index] = nullptr;

        buttons[index] = new (std::nothrow) Button(
            WindowManager_ScaleUlx(window, control->bounds.ulx), WindowManager_ScaleUly(window, control->bounds.uly),
            control->bounds.lrx - control->bounds.ulx, control->bounds.lry - control->bounds.uly);
    } else {
        buttons[index] = new (std::nothrow) Button(control->image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   WindowManager_ScaleUlx(window, control->bounds.ulx),
                                                   WindowManager_ScaleUly(window, control->bounds.uly));

        if (control->label) {
            buttons[index]->SetCaption(control->label);
        }
    }

    buttons[index]->SetFlags(0);
    buttons[index]->SetRValue(1000 + index);
    buttons[index]->SetPValue(GNW_INPUT_PRESS + index);
    buttons[index]->SetSfx(control->sfx);
    buttons[index]->RegisterButton(window->id);

    menu_item[index].r_value = 1000 + index;
    menu_item[index].event_code = control->event_code;
    menu_item[index].event_handler = control->event_handler;
}

void GameSetupMenu::Init(int32_t palette_from_image) {
    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);

    event_click_done = false;
    event_click_cancel = false;

    game_file_number = ini_get_setting(INI_GAME_FILE_NUMBER);
    game_index_first_on_page =
        ((game_file_number - 1) / GAME_SETUP_MENU_MISSION_COUNT) * GAME_SETUP_MENU_MISSION_COUNT + 1;
    strings = nullptr;
    string_row_index = 0;

    mouse_hide();
    WindowManager_LoadBigImage(MAINPIC, window, window->width, palette_from_image, false, -1, -1, true);

    switch (game_file_type) {
        case GAME_TYPE_TRAINING:
            draw_menu_title(window, _(9b46));
            break;

        case GAME_TYPE_SCENARIO:
            draw_menu_title(window, _(5d49));
            break;

        case GAME_TYPE_CAMPAIGN:
            draw_menu_title(window, _(385b));
            break;

        default:
            draw_menu_title(window, _(f27a));
            break;
    }

    for (int32_t i = 0; i < GAME_SETUP_MENU_ITEM_COUNT; ++i) {
        buttons[i] = nullptr;
        ButtonInit(i);
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    DrawMissionList();
    LoadMissionDescription();
    DrawMissionDescription();

    mouse_show();
}

void GameSetupMenu::Deinit() {
    for (int32_t i = 0; i < GAME_SETUP_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }

    for (int32_t i = 0; i < GAME_SETUP_MENU_MISSION_COUNT; ++i) {
        delete[] menu_setup_menu_mission_titles[i];
    }

    delete[] strings;
}

void GameSetupMenu::EventSelectItem() {
    int32_t game_slot;

    game_slot = game_index_first_on_page + key;

    if (game_file_number == game_slot) {
        event_click_done = true;
    } else {
        if (game_file_number >= game_index_first_on_page &&
            game_file_number < game_index_first_on_page + GAME_SETUP_MENU_MISSION_COUNT) {
            DrawSaveFileTitle(game_file_number - game_index_first_on_page, 0x2);
        }

        game_file_number = game_slot;
        LoadMissionDescription();
        DrawMissionDescription();
    }
}

void GameSetupMenu::EventScrollButton() {
    int32_t game_slot;

    if (key == 12) {
        game_slot = game_index_first_on_page - GAME_SETUP_MENU_MISSION_COUNT;
    } else {
        game_slot = game_index_first_on_page + GAME_SETUP_MENU_MISSION_COUNT;
    }

    if (FindNextValidFile(game_slot)) {
        game_index_first_on_page = game_slot;
        DrawMissionList();
        DrawMissionDescription();
    }
}

void GameSetupMenu::EventBriefingButton() {
    if (key == 14) {
        if (string_row_index) {
            int32_t row_index;
            uint32_t time_stamp;

            row_index = string_row_index - rows_per_page;

            if (row_index < 0) {
                row_index = 0;
            }

            do {
                time_stamp = timer_get();

                --string_row_index;
                DrawDescriptionPanel();

                while (timer_get() - time_stamp < TIMER_FPS_TO_MS(96)) {
                }

            } while (string_row_index != row_index);
        }

    } else {
        if (string_row_index + rows_per_page < string_row_count) {
            int32_t row_index;
            uint32_t time_stamp;

            row_index = string_row_index + rows_per_page;

            do {
                time_stamp = timer_get();

                ++string_row_index;
                DrawDescriptionPanel();

                while (timer_get() - time_stamp < TIMER_FPS_TO_MS(96)) {
                }

            } while (string_row_index != row_index);
        }
    }
}

void GameSetupMenu::EventCancel() { event_click_cancel = true; }

void GameSetupMenu::EventHelp() {
    switch (game_file_type) {
        case GAME_TYPE_TRAINING: {
            HelpMenu_Menu(HELPMENU_TRAINING_MENU_SETUP, WINDOW_MAIN_WINDOW);
        } break;

        case GAME_TYPE_SCENARIO: {
            HelpMenu_Menu(HELPMENU_STAND_ALONE_MENU_SETUP, WINDOW_MAIN_WINDOW);
        } break;

        case GAME_TYPE_CAMPAIGN: {
            HelpMenu_Menu(HELPMENU_CAMPAIGN_MENU, WINDOW_MAIN_WINDOW);
        } break;

        default: {
            HelpMenu_Menu(HELPMENU_MULTI_SCENARIO_MENU, WINDOW_MAIN_WINDOW);
        } break;
    }
}

void GameSetupMenu::DrawMissionList() {
    menu_draw_menu_portrait_frame(window);

    game_count = 0;

    for (int32_t i = 0; i < GAME_SETUP_MENU_MISSION_COUNT; ++i) {
        GameSetupMenuControlItem* control;
        SaveFileInfo save_file_header;

        control = &game_setup_menu_controls[i];

        if (i < GAME_SETUP_MENU_MISSION_COUNT - 1) {
            draw_line(window->buffer, window->width, WindowManager_ScaleUlx(window, control->bounds.ulx),
                      WindowManager_ScaleUly(window, control->bounds.lry),
                      WindowManager_ScaleLrx(window, control->bounds.ulx, control->bounds.lrx),
                      WindowManager_ScaleLry(window, control->bounds.uly, control->bounds.lry), COLOR_GREEN);
        }

        if (menu_setup_menu_mission_titles[i]) {
            delete[] menu_setup_menu_mission_titles[i];
            menu_setup_menu_mission_titles[i] = nullptr;
        }

        if (game_file_type == GAME_TYPE_CAMPAIGN &&
            ini_get_setting(INI_LAST_CAMPAIGN) < (game_index_first_on_page + i)) {
            buttons[i]->Disable();

        } else if (SaveLoad_GetSaveFileInfo(game_index_first_on_page + i, game_file_type, save_file_header)) {
            ++game_count;

            menu_setup_menu_mission_titles[i] = new (std::nothrow) char[30];
            SDL_utf8strlcpy(menu_setup_menu_mission_titles[i], save_file_header.save_name.c_str(), 30);

            DrawSaveFileTitle(i, COLOR_GREEN);
            buttons[i]->Enable();

            process_bk();

        } else {
            buttons[i]->Disable();
        }
    }
}

void GameSetupMenu::LoadMissionDescription() {
    SmartString string;
    SmartString filename;
    int32_t width;
    int32_t height;

    if (strings) {
        delete[] strings;
        strings = nullptr;

        string_row_index = 0;
    }

    if (game_count) {
        if (game_file_type == GAME_TYPE_CAMPAIGN && game_file_number > game_count) {
            game_file_number = game_count;
        }
    }

    filename.Sprintf(20, "descr%i.%s", game_file_number, SaveLoadMenu_SaveFileTypes[game_file_type]);

    auto fp{ResourceManager_OpenFileResource(filename.GetCStr(), ResourceType_Text)};

    if (fp) {
        fseek(fp, 0, SEEK_END);
        int32_t text_size = ftell(fp);

        auto text = std::make_unique<char[]>(text_size + 1);

        text.get()[text_size] = '\0';

        fseek(fp, 0, SEEK_SET);
        fread(text.get(), sizeof(char), text_size, fp);

        fclose(fp);

        string = text.get();
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    width = game_setup_menu_titles[1].bounds.lrx - game_setup_menu_titles[1].bounds.ulx;
    height = game_setup_menu_titles[1].bounds.lry - game_setup_menu_titles[1].bounds.uly;

    rows_per_page = height / Text_GetHeight();

    if (string.GetLength()) {
        strings = Text_SplitText(string.GetCStr(), rows_per_page * 3, width, &string_row_count);
    }
}

void GameSetupMenu::DrawMissionDescription() {
    static char text[40];

    GameSetupMenu_LoadSubtitleControl(window);
    GameSetupMenu_DrawBigInfoPanel(window);

    if (game_count) {
        int32_t game_slot;

        game_slot = game_file_number - game_index_first_on_page;

        if (game_file_number >= game_index_first_on_page &&
            game_file_number < (game_index_first_on_page + GAME_SETUP_MENU_MISSION_COUNT)) {
            DrawSaveFileTitle(game_slot, 0x4);
            strcpy(text, menu_setup_menu_mission_titles[game_slot]);
        }

        Text_SetFont(GNW_TEXT_FONT_5);
        game_setup_menu_titles[0].title = text;
        menu_draw_menu_title(window, &game_setup_menu_titles[0], COLOR_GREEN, true, true);
        DrawDescriptionPanel();
    }

    win_draw(window->id);
}

void GameSetupMenu::DrawSaveFileTitle(int32_t game_slot, int32_t color) {
    GameSetupMenuControlItem* control;
    char text[40];

    control = &game_setup_menu_controls[game_slot];

    sprintf(text, "%i. %s", game_index_first_on_page + game_slot, menu_setup_menu_mission_titles[game_slot]);

    Text_TextBox(window->buffer, window->width, text, WindowManager_ScaleUlx(window, control->bounds.ulx),
                 WindowManager_ScaleUly(window, control->bounds.uly), control->bounds.lrx - control->bounds.ulx,
                 control->bounds.lry - control->bounds.uly, color, false, true);
}

void GameSetupMenu::DrawDescriptionPanel() {
    MenuTitleItem* menu_item;
    int32_t width;
    uint8_t* buffer_position;
    int32_t row_index_max;
    Rect bounds;

    GameSetupMenu_DrawBigInfoPanel(window);

    menu_item = &game_setup_menu_titles[1];

    bounds.ulx = WindowManager_ScaleUlx(window, menu_item->bounds.ulx);
    bounds.uly = WindowManager_ScaleUly(window, menu_item->bounds.uly);
    bounds.lrx = WindowManager_ScaleLrx(window, menu_item->bounds.ulx, menu_item->bounds.lrx);
    bounds.lry = WindowManager_ScaleLrx(window, menu_item->bounds.uly, menu_item->bounds.lry);

    width = bounds.lrx - bounds.ulx;
    buffer_position = &window->buffer[window->width * bounds.uly + bounds.ulx];

    row_index_max = string_row_index + rows_per_page;

    if (row_index_max > string_row_count) {
        row_index_max = string_row_count;
    }

    if (strings) {
        for (int32_t row_index = string_row_index; row_index < row_index_max; ++row_index) {
            Text_Blit(&buffer_position[window->width * (row_index - string_row_index) * Text_GetHeight()],
                      strings[row_index].GetCStr(), width, window->width, 0xA2);
        }
    }

    win_draw_rect(window->id, &bounds);
}

int32_t GameSetupMenu::FindNextValidFile(int32_t game_slot) {
    int32_t result;

    if (game_file_type == GAME_TYPE_CAMPAIGN && ini_get_setting(INI_LAST_CAMPAIGN) < game_slot) {
        result = false;
    } else {
        SaveFileInfo save_file_header;

        result = false;

        for (int32_t i = game_slot; i < game_slot + GAME_SETUP_MENU_MISSION_COUNT; ++i) {
            if (SaveLoad_GetSaveFileInfo(i, game_file_type, save_file_header)) {
                result = true;
                break;
            }
        }
    }

    return result;
}

void GameSetupMenu::EventStepButton() {
    int32_t game_slot;
    int32_t game_slot_first;

    game_slot = game_file_number;
    game_slot_first = game_index_first_on_page;

    if (key == GNW_KB_KEY_UP) {
        if (game_slot == 1) {
            return;
        }

        --game_slot;
    } else {
        ++game_slot;
    }

    while (game_slot < game_slot_first) {
        game_slot_first -= GAME_SETUP_MENU_MISSION_COUNT;
    }

    while (game_slot >= game_slot_first + GAME_SETUP_MENU_MISSION_COUNT) {
        game_slot_first += GAME_SETUP_MENU_MISSION_COUNT;
    }

    if (game_slot_first != game_index_first_on_page && FindNextValidFile(game_slot_first)) {
        game_index_first_on_page = game_slot_first;
        DrawMissionList();
    }

    if (game_slot >= game_count + game_index_first_on_page) {
        game_slot = game_count + game_index_first_on_page - 1;
    }

    if (game_slot != game_file_number) {
        key = game_slot - game_index_first_on_page;
        EventSelectItem();
    }
}

void GameSetupMenu::EventStart() {
    if (game_count) {
        event_click_done = true;
    } else {
        event_click_cancel = true;
    }
}
