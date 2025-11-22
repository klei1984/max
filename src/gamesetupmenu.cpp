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
#include "menu.hpp"
#include "missionmanager.hpp"
#include "resource_manager.hpp"
#include "saveloadmenu.hpp"
#include "settings.hpp"
#include "text.hpp"
#include "window_manager.hpp"

#define GAME_SETUP_MENU_MISSION_COUNT 12

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

        mission_titles[index].clear();

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

    game_file_number = ResourceManager_GetSettings()->GetNumericValue("game_file_number");
    game_index_first_on_page =
        ((game_file_number - 1) / GAME_SETUP_MENU_MISSION_COUNT) * GAME_SETUP_MENU_MISSION_COUNT + 1;
    strings = nullptr;
    string_row_index = 0;

    mouse_hide();
    WindowManager_LoadBigImage(MAINPIC, window, window->width, palette_from_image, false, -1, -1, true);

    switch (m_mission_category) {
        case MISSION_CATEGORY_TRAINING: {
            draw_menu_title(window, _(9b46));
        } break;

        case MISSION_CATEGORY_SCENARIO: {
            draw_menu_title(window, _(5d49));
        } break;

        case MISSION_CATEGORY_CAMPAIGN: {
            draw_menu_title(window, _(385b));
        } break;

        case MISSION_CATEGORY_MULTI_PLAYER_SCENARIO: {
            if (m_is_single_player) {
                draw_menu_title(window, _(884f));

            } else {
                draw_menu_title(window, _(f27a));
            }
        } break;

        default: {
            draw_menu_title(window, _(f27a));
        } break;
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
    switch (m_mission_category) {
        case MISSION_CATEGORY_TRAINING: {
            HelpMenu_Menu("TRAINING_MENU_SETUP", WINDOW_MAIN_WINDOW);
        } break;

        case MISSION_CATEGORY_SCENARIO: {
            HelpMenu_Menu("STAND_ALONE_MENU_SETUP", WINDOW_MAIN_WINDOW);
        } break;

        case MISSION_CATEGORY_CAMPAIGN: {
            HelpMenu_Menu("CAMPAIGN_MENU", WINDOW_MAIN_WINDOW);
        } break;

        default: {
            HelpMenu_Menu("MULTI_SCENARIO_MENU", WINDOW_MAIN_WINDOW);
        } break;
    }
}

void GameSetupMenu::DrawMissionList() {
    const size_t ini_last_campaign = ResourceManager_GetSettings()->GetNumericValue("last_campaign");

    menu_draw_menu_portrait_frame(window);

    game_count = 0;

    const auto& missions = ResourceManager_GetMissionManager()->GetMissions(m_mission_category);

    for (int32_t i = 0; i < GAME_SETUP_MENU_MISSION_COUNT; ++i) {
        GameSetupMenuControlItem* control;
        const size_t mission_index = game_index_first_on_page + i - 1;

        control = &game_setup_menu_controls[i];

        if (i < GAME_SETUP_MENU_MISSION_COUNT - 1) {
            draw_line(window->buffer, window->width, WindowManager_ScaleUlx(window, control->bounds.ulx),
                      WindowManager_ScaleUly(window, control->bounds.lry),
                      WindowManager_ScaleLrx(window, control->bounds.ulx, control->bounds.lrx),
                      WindowManager_ScaleLry(window, control->bounds.uly, control->bounds.lry), COLOR_GREEN);
        }

        mission_titles[i].clear();

        if (m_mission_category == MISSION_CATEGORY_CAMPAIGN && (ini_last_campaign <= mission_index)) {
            buttons[i]->Disable();

        } else if (mission_index < missions.size()) {
            ++game_count;

            mission_titles[i] = missions[game_index_first_on_page + i - 1]->GetTitle();

            DrawSaveFileTitle(i, COLOR_GREEN);
            buttons[i]->Enable();

            process_bk();

        } else {
            buttons[i]->Disable();
        }
    }
}

void GameSetupMenu::LoadMissionDescription() {
    if (strings) {
        delete[] strings;
        strings = nullptr;

        string_row_index = 0;
    }

    if (game_count) {
        if (m_mission_category == MISSION_CATEGORY_CAMPAIGN && game_file_number > game_count) {
            game_file_number = game_count;
        }
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    const int32_t width = game_setup_menu_titles[1].bounds.lrx - game_setup_menu_titles[1].bounds.ulx;
    const int32_t height = game_setup_menu_titles[1].bounds.lry - game_setup_menu_titles[1].bounds.uly;

    rows_per_page = height / Text_GetHeight();

    const auto& missions = ResourceManager_GetMissionManager()->GetMissions(m_mission_category);
    const size_t mission_index = game_file_number - 1;

    if (mission_index < missions.size()) {
        const auto description = missions[mission_index]->GetDescription();

        if (description.length() > 0) {
            strings = Text_SplitText(description.c_str(), rows_per_page * 3, width, &string_row_count);
        }
    }
}

void GameSetupMenu::DrawMissionDescription() {
    GameSetupMenu_LoadSubtitleControl(window);
    GameSetupMenu_DrawBigInfoPanel(window);

    if (game_count) {
        const auto& missions = ResourceManager_GetMissionManager()->GetMissions(m_mission_category);

        if (game_file_number >= game_index_first_on_page &&
            game_file_number < (game_index_first_on_page + GAME_SETUP_MENU_MISSION_COUNT)) {
            DrawSaveFileTitle(game_file_number - game_index_first_on_page, COLOR_YELLOW);
        }

        Text_SetFont(GNW_TEXT_FONT_5);
        game_setup_menu_titles[0].title = missions[game_file_number - 1]->GetTitle();
        menu_draw_menu_title(window, &game_setup_menu_titles[0], COLOR_GREEN, true, true);
        DrawDescriptionPanel();
    }

    win_draw(window->id);
}

void GameSetupMenu::DrawSaveFileTitle(int32_t game_slot, int32_t color) {
    GameSetupMenuControlItem* control;
    char text[200];

    control = &game_setup_menu_controls[game_slot];

    sprintf(text, "%i. %s", game_index_first_on_page + game_slot, mission_titles[game_slot].c_str());

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

    if (m_mission_category == MISSION_CATEGORY_CAMPAIGN &&
        ResourceManager_GetSettings()->GetNumericValue("last_campaign") < game_slot) {
        result = false;

    } else {
        result = false;

        for (int32_t i = game_slot; i < (game_slot + GAME_SETUP_MENU_MISSION_COUNT); ++i) {
            const auto& missions = ResourceManager_GetMissionManager()->GetMissions(m_mission_category);

            if ((i - 1) > 0 && ((i - 1) < static_cast<int32_t>(missions.size()))) {
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

MissionCategory GameSetupMenu::GetMissionCategory() const { return m_mission_category; }

void GameSetupMenu::SetMissionCategory(const MissionCategory mission_category) {
    m_mission_category = mission_category;
}

void GameSetupMenu::SetSinglePlayerMode(const bool is_single_player) { m_is_single_player = is_single_player; }

bool GameSetupMenu::IsSinglePlayerMode() const { return m_is_single_player; }
