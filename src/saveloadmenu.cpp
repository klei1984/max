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

#include "saveloadmenu.hpp"

#include <ctime>
#include <filesystem>

#include "access.hpp"
#include "ai.hpp"
#include "button.hpp"
#include "flicsmgr.hpp"
#include "game_manager.hpp"
#include "hash.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "missionmanager.hpp"
#include "mouseevent.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "saveloadchecks.hpp"
#include "sound_manager.hpp"
#include "teamunits.hpp"
#include "text.hpp"
#include "textedit.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

#define SAVESLOT_MENU_TITLE_COUNT 8

class SaveSlot {
    const char *SaveLoadMenu_SaveTypeTitles[SAVESLOT_MENU_TITLE_COUNT] = {_(494c), _(8b45), _(2682), _(4bf8),
                                                                          _(dea8), _(8a2c), _(2fcb), _(3bc3)};

public:
    WinID wid;
    int32_t ulx;
    int32_t uly;
    int32_t width;
    int32_t height;
    uint8_t *image_up;
    uint8_t *image_down;
    ButtonID bid;
    char file_name[15];
    char save_name[30];
    char in_use;
    uint32_t save_file_type;
    TextEdit *text_edit;

    SaveSlot();
    ~SaveSlot();

    void Deinit();
    void InitTextEdit(WinID wid);
    void UpdateTextEdit(uint8_t *image);
    void SetImageDownForTextEdit();
    void SetImageUpForTextEdit();
    void EnterTextEditField();
    void LeaveTextEditField();
    void AcceptEditedText();
    void DrawSaveSlot(int32_t game_file_type);
};

static constexpr int32_t SaveLoadMenu_SaveSlotLimit = 991L;

static int32_t SaveLoadMenu_FirstSaveSlotOnPage = 1;
int32_t SaveLoadMenu_SaveSlot;
uint8_t SaveLoadMenu_GameState;
uint16_t SaveLoadMenu_TurnTimer;

static char SaveLoadMenu_SaveSlotTextEditBuffer[30];

static void SaveLoadMenu_DrawSaveSlotResource(uint8_t *image, int32_t width, ResourceID id, const char *title,
                                              int32_t font_num);
static Button *SaveLoadMenu_CreateButton(WinID wid, ResourceID up, ResourceID down, int32_t ulx, int32_t uly,
                                         const char *caption, int32_t r_value);
static void SaveLoadMenu_Init(const MissionCategory mission_category, SaveSlot *slots, int32_t num_buttons,
                              Button *buttons[], Flic **flc, bool is_saving_allowed, int32_t first_slot_on_page,
                              bool mode);
static void SaveLoadMenu_PlaySfx(ResourceID id);
static void SaveLoadMenu_EventLoadSlotClick(SaveSlot *slots, int32_t *save_slot_index, int32_t key,
                                            int32_t is_saving_allowed);
static void SaveLoadMenu_EventSaveLoadSlotClick(SaveSlot *slots, int32_t save_slot_index, int32_t is_saving_allowed);
static bool SaveLoadMenu_RunPlausibilityTests();

Button *SaveLoadMenu_CreateButton(WinID wid, ResourceID up, ResourceID down, int32_t ulx, int32_t uly,
                                  const char *caption, int32_t r_value) {
    Button *button;

    button = new (std::nothrow) Button(up, down, ulx, uly);

    if (caption) {
        button->SetCaption(caption);
    }

    button->SetFlags(0x0);
    button->SetRValue(r_value);
    button->RegisterButton(wid);
    button->Enable();

    return button;
}

void SaveLoadMenu_DrawSaveSlotResource(uint8_t *image, int32_t width, ResourceID id, const char *title,
                                       int32_t font_num) {
    struct ImageSimpleHeader *image_header;
    int32_t buffer_position;

    Text_SetFont(font_num);

    image_header = reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_ReadResource(id));

    buffer_position = image_header->uly * width + image_header->ulx;

    buf_to_buf(&image_header->transparent_color, image_header->width, image_header->height, image_header->width,
               &image[buffer_position], width);

    buffer_position += ((image_header->height - Text_GetHeight()) / 2) * width;

    if (font_num == GNW_TEXT_FONT_2) {
        int32_t offset = (image_header->width - Text_GetWidth(title)) / 2;
        if (offset > 5) {
            buffer_position += offset;
        } else {
            buffer_position += 5;
        }
    } else {
        buffer_position += 5;
    }

    Text_Blit(&image[buffer_position], title, image_header->width - 10, width, COLOR_GREEN);

    delete[] image_header;
}

void SaveLoadMenu_PlaySfx(ResourceID id) { SoundManager_PlaySfx(id); }

void SaveLoadMenu_Init(const MissionCategory mission_category, SaveSlot *slots, int32_t num_buttons, Button *buttons[],
                       Flic **flc, bool is_saving_allowed, int32_t first_slot_on_page, bool mode) {
    WindowInfo *window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    uint8_t save_game_type;
    ImageSimpleHeader *image_up;
    ImageSimpleHeader *image_down;
    int32_t image_up_size;
    int32_t image_down_size;
    char text_slot_index[8];
    WindowInfo slot_window;
    ButtonID button_list[num_buttons];

    mouse_hide();

    if (mode) {
        WindowManager_LoadBigImage(LOADPIC, window, window->width, true, false, -1, -1, true);
    }

    Text_SetFont(GNW_TEXT_FONT_5);

    Text_TextBox(window->buffer, window->width, is_saving_allowed ? _(5426) : _(63e7),
                 WindowManager_ScaleUlx(window, 229), WindowManager_ScaleUly(window, 5), 181, 21, COLOR_GREEN, true);

    for (int32_t i = 0; i < num_buttons; ++i) {
        struct SaveFileInfo save_file_info;

        const auto save_file_category = SaveLoad_GetSaveFileCategory(mission_category);

        if (SaveLoad_GetSaveFileInfo(save_file_category, first_slot_on_page + i, save_file_info) &&
            SaveLoad_IsSaveFileFormatSupported(save_file_info.version)) {
            slots[i].in_use = true;
            SDL_utf8strlcpy(slots[i].file_name, save_file_info.file_name.c_str(), sizeof(slots[i].file_name));
            SDL_utf8strlcpy(slots[i].save_name, save_file_info.save_name.c_str(), sizeof(slots[i].save_name));
            slots[i].save_file_type = save_file_info.save_file_category;

        } else {
            slots[i].in_use = false;
            slots[i].file_name[0] = '\0';
            slots[i].save_name[0] = '\0';
            slots[i].save_file_type = mission_category;
        }

        image_up =
            reinterpret_cast<ImageSimpleHeader *>(ResourceManager_ReadResource(static_cast<ResourceID>(FILE1_UP + i)));
        image_down =
            reinterpret_cast<ImageSimpleHeader *>(ResourceManager_ReadResource(static_cast<ResourceID>(FILE1_DN + i)));

        image_up_size = image_up->width * image_up->height;
        image_down_size = image_down->width * image_down->height;

        slots[i].image_up = new (std::nothrow) uint8_t[image_up_size];
        slots[i].image_down = new (std::nothrow) uint8_t[image_down_size];

        memcpy(slots[i].image_up, &image_up->transparent_color, image_up_size);
        memcpy(slots[i].image_down, &image_down->transparent_color, image_down_size);

        Text_SetFont(GNW_TEXT_FONT_1);

        snprintf(text_slot_index, sizeof(text_slot_index), "%d", first_slot_on_page + i);

        slot_window.buffer = slots[i].image_up;
        slot_window.width = image_up->width;
        Text_TextBox(&slot_window, text_slot_index, 0, 0, 40, 70, true, true, FontColor(165, 177, 199));

        slot_window.buffer = slots[i].image_down;
        slot_window.width = image_down->width;
        Text_TextBox(&slot_window, text_slot_index, 0, 0, 40, 70, true, true, FontColor(5, 58, 199));

        Text_SetFont(GNW_TEXT_FONT_5);

        slots[i].ulx = WindowManager_ScaleUlx(window, 402 * (i / 5) + 16);
        slots[i].uly = WindowManager_ScaleUly(window, 76 * (i % 5) + 44);
        slots[i].width = image_up->width;
        slots[i].height = image_up->height;

        slots[i].DrawSaveSlot(slots[i].save_file_type);

        slots[i].bid =
            win_register_button(window->id, slots[i].ulx, slots[i].uly, image_up->width, image_up->height, -1, -1,
                                1001 + i, 1011 + i, slots[i].image_up, slots[i].image_down, nullptr, 0x1);

        delete[] image_up;
        delete[] image_down;

        slots[i].InitTextEdit(window->id);
        button_list[i] = slots[i].bid;
    }

    win_group_radio_buttons(num_buttons, button_list);

    buttons[0] = SaveLoadMenu_CreateButton(window->id, MNUUAROU, MNUUAROD, WindowManager_ScaleUlx(window, 33),
                                           WindowManager_ScaleUly(window, 438), nullptr, 329);
    buttons[1] = SaveLoadMenu_CreateButton(window->id, MNUDAROU, MNUDAROD, WindowManager_ScaleUlx(window, 63),
                                           WindowManager_ScaleUly(window, 438), nullptr, 337);

    Text_SetFont(GNW_TEXT_FONT_1);
    buttons[2] = SaveLoadMenu_CreateButton(window->id, MNUBTN6U, MNUBTN6D, WindowManager_ScaleUlx(window, 514),
                                           WindowManager_ScaleUly(window, 438), _(f846), 1023);
    buttons[3] = SaveLoadMenu_CreateButton(window->id, MNUBTN5U, MNUBTN5D, WindowManager_ScaleUlx(window, 465),
                                           WindowManager_ScaleUly(window, 438), _(278f), 1021);
    buttons[4] =
        SaveLoadMenu_CreateButton(window->id, MNUBTN4U, MNUBTN4D, WindowManager_ScaleUlx(window, 354),
                                  WindowManager_ScaleUly(window, 438), is_saving_allowed ? _(6b01) : _(f752), 1000);

    if (is_saving_allowed) {
        buttons[5] = SaveLoadMenu_CreateButton(window->id, MNUBTN3U, MNUBTN3D, WindowManager_ScaleUlx(window, 243),
                                               WindowManager_ScaleUly(window, 438), _(d4ac), 1024);
        buttons[6] = SaveLoadMenu_CreateButton(window->id, MNUBTN2U, MNUBTN2D, WindowManager_ScaleUlx(window, 132),
                                               WindowManager_ScaleUly(window, 438), _(1509), 1022);
    }

    win_draw(window->id);

    if (mode) {
        *flc = flicsmgr_construct(FILESFLC, window, window->width, WindowManager_ScaleUlx(window, 256),
                                  WindowManager_ScaleUly(window, 104), 2, false);
    }

    mouse_show();
}

void SaveLoadMenu_EventLoadSlotClick(SaveSlot *slots, int32_t *save_slot_index, int32_t key,
                                     int32_t is_saving_allowed) {
    SaveLoadMenu_PlaySfx(KCARG0);

    if (*save_slot_index >= 0) {
        slots[*save_slot_index].SetImageUpForTextEdit();
        slots[*save_slot_index].LeaveTextEditField();
    }

    *save_slot_index = key;

    slots[*save_slot_index].SetImageDownForTextEdit();

    if (is_saving_allowed) {
        if (!strlen(slots[*save_slot_index].save_name)) {
            slots[*save_slot_index].text_edit->SetEditedText(SaveLoadMenu_SaveSlotTextEditBuffer);
        }

        slots[*save_slot_index].text_edit->EnterTextEditField();
    }
}

void SaveLoadMenu_EventSaveLoadSlotClick(SaveSlot *slots, int32_t save_slot_index, int32_t is_saving_allowed) {
    SaveLoadMenu_PlaySfx(KCARG0);
    win_set_button_rest_state(slots[save_slot_index].bid, 1, 0);

    if (is_saving_allowed) {
        slots[save_slot_index].EnterTextEditField();
    }
}

int32_t SaveLoadMenu_MenuLoop(const MissionCategory mission_category, const bool is_saving_allowed) {
    const char *menu_team_names[] = {_(f394), _(a8a6), _(a3ee), _(319d), ""};
    SaveSlot slots[10];
    Button *buttons[7];
    Flic *flc;
    int32_t result;
    int32_t save_slot_index;
    uint32_t time_stamp;
    bool exit_loop;
    int32_t key;
    const int32_t slot_count = sizeof(slots) / sizeof(SaveSlot);

    result = 0;
    flc = nullptr;
    save_slot_index = -1;

    SaveLoadMenu_SaveSlotTextEditBuffer[0] = '\0';

    SaveLoadMenu_Init(mission_category, slots, slot_count, buttons, &flc, is_saving_allowed,
                      SaveLoadMenu_FirstSaveSlotOnPage, true);

    time_stamp = timer_get();
    exit_loop = false;

    do {
        if (Remote_GameState == 1) {
            Remote_UiProcessNetPackets();
        }

        key = get_input();

        if (save_slot_index >= 0) {
            switch (key) {
                case GNW_KB_KEY_PAGEUP:
                case GNW_KB_KEY_PAGEDOWN: {
                    slots[save_slot_index].text_edit->ProcessKeyPress(GNW_KB_KEY_ESCAPE);
                } break;
                default: {
                    if (slots[save_slot_index].text_edit->ProcessKeyPress(key)) {
                        if (key == GNW_KB_KEY_RALT_RETURN && is_saving_allowed) {
                            key = 1022;
                        } else {
                            key = -1;
                        }
                    }
                } break;
            }
        }

        if (key >= 0) {
            if (key >= 1001 && key <= 1010) {
                SaveLoadMenu_EventLoadSlotClick(slots, &save_slot_index, key - 1001, is_saving_allowed);
            } else if (key >= 1011 && key <= 1020) {
                if (is_saving_allowed) {
                    SaveLoadMenu_EventSaveLoadSlotClick(slots, save_slot_index, is_saving_allowed);
                } else {
                    key = 1023;
                }
            }

            switch (key) {
                case GNW_KB_KEY_PAGEUP:
                case GNW_KB_KEY_PAGEDOWN: {
                    SaveLoadMenu_PlaySfx(KCARG0);

                    if ((key != GNW_KB_KEY_PAGEUP || SaveLoadMenu_FirstSaveSlotOnPage != 1) &&
                        (key != GNW_KB_KEY_PAGEDOWN ||
                         SaveLoadMenu_FirstSaveSlotOnPage != (SaveLoadMenu_SaveSlotLimit))) {
                        int32_t button_max_index;

                        if (is_saving_allowed) {
                            button_max_index = 7;
                        } else {
                            button_max_index = 5;
                        }

                        for (int32_t i = 0; i < button_max_index; ++i) {
                            delete buttons[i];
                        }

                        for (int32_t i = 0; i < slot_count; ++i) {
                            slots[i].Deinit();
                        }

                        if (key == GNW_KB_KEY_PAGEUP) {
                            if (SaveLoadMenu_FirstSaveSlotOnPage > slot_count) {
                                SaveLoadMenu_FirstSaveSlotOnPage -= slot_count;
                            }
                        } else {
                            SaveLoadMenu_FirstSaveSlotOnPage += slot_count;
                        }

                        SaveLoadMenu_Init(mission_category, slots, slot_count, buttons, &flc, is_saving_allowed,
                                          SaveLoadMenu_FirstSaveSlotOnPage, false);
                        save_slot_index = -1;
                    }
                } break;

                case 1022: {
                    SaveLoadMenu_PlaySfx(FSAVE);

                    if (save_slot_index >= 0) {
                        slots[save_slot_index].text_edit->ProcessKeyPress(GNW_KB_KEY_RETURN);
                        slots[save_slot_index].AcceptEditedText();

                        if (Remote_IsNetworkGame) {
                            Remote_SendNetPacket_16(slots[save_slot_index].file_name, slots[save_slot_index].save_name);
                        }

                        if (mission_category == MISSION_CATEGORY_TRAINING ||
                            mission_category == MISSION_CATEGORY_SCENARIO ||
                            mission_category == MISSION_CATEGORY_CAMPAIGN ||
                            mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
                            GameManager_TurnCounter = 1;

                            if (mission_category == MISSION_CATEGORY_MULTI_PLAYER_SCENARIO) {
                                for (int32_t i = 0; i < 4; ++i) {
                                    if (UnitsManager_TeamInfo[i].team_type) {
                                        ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + i),
                                                                  menu_team_names[i]);
                                    }
                                }
                            }
                        }

                        SaveLoadMenu_Save(slots[save_slot_index].file_name, slots[save_slot_index].save_name, true);
                        slots[save_slot_index].in_use = true;
                        slots[save_slot_index].DrawSaveSlot(slots[save_slot_index].save_file_type);
                        win_draw(WindowManager_GetWindow(WINDOW_MAIN_WINDOW)->id);
                        SaveLoadMenu_SaveSlot = SaveLoadMenu_FirstSaveSlotOnPage + save_slot_index;
                    }

                } break;

                case 1000: {
                    SaveLoadMenu_PlaySfx(FCANC);
                    exit_loop = true;
                } break;

                case 1021: {
                    SaveLoadMenu_PlaySfx(FHELP);
                    if (is_saving_allowed) {
                        HelpMenu_Menu("SAVELOAD_SETUP", WINDOW_MAIN_WINDOW, Remote_IsNetworkGame == false);
                    } else {
                        HelpMenu_Menu("LOAD_SETUP", WINDOW_MAIN_WINDOW);
                    }
                } break;

                case 1023: {
                    SaveLoadMenu_PlaySfx(FLOAD);
                    if (is_saving_allowed && Remote_IsNetworkGame) {
                        int32_t game_state;

                        game_state = GameManager_GameState;
                        GameManager_GameState = GAME_STATE_10;
                        MessageManager_DrawMessage(_(1fc6), 2, 1, true);
                        GameManager_GameState = game_state;
                    } else if (save_slot_index != -1 && slots[save_slot_index].in_use) {
                        struct SaveFileInfo save_file_info;
                        const int32_t save_slot = SaveLoadMenu_FirstSaveSlotOnPage + save_slot_index;
                        const auto save_file_category = SaveLoad_GetSaveFileCategory(mission_category);

                        if (SaveLoad_GetSaveFileInfo(save_file_category, save_slot, save_file_info)) {
                            switch (save_file_info.version) {
                                case static_cast<uint32_t>(SmartFileFormat::V70): {
                                    std::string hash;

                                    if (save_file_info.save_file_category == MISSION_CATEGORY_CUSTOM) {
                                        hash = "MISSION_CATEGORY_CUSTOM";

                                    } else if (save_file_info.save_file_category == MISSION_CATEGORY_HOT_SEAT) {
                                        hash = "MISSION_CATEGORY_HOT_SEAT";

                                    } else if (save_file_info.save_file_category == MISSION_CATEGORY_MULTI) {
                                        hash = "MISSION_CATEGORY_MULTI";

                                    } else {
                                        hash = save_file_info.mission;
                                    }

                                    if (ResourceManager_GetMissionManager()->LoadMission(
                                            static_cast<MissionCategory>(save_file_info.save_file_category), hash)) {
                                        ResourceManager_GetMissionManager()->GetMission()->SetMission(
                                            save_file_info.file_path);

                                        result = save_slot;
                                    }
                                } break;
                                case static_cast<uint32_t>(SmartFileFormat::V71): {
                                    /// \todo language
                                    auto mission = std::make_shared<Mission>("en-US");

                                    if (mission && mission->LoadBinaryBuffer(save_file_info.script)) {
                                        if (ResourceManager_GetMissionManager()->LoadMission(mission)) {
                                            ResourceManager_GetMissionManager()->GetMission()->SetMission(
                                                save_file_info.file_path);

                                            result = save_slot;
                                        }
                                    }
                                } break;
                            }

                            const auto mission = ResourceManager_GetMissionManager()->GetMission();

                            if (mission && mission->GetMission().empty()) {
                                mission->SetMission(save_file_info.file_path);
                            }
                        }

                        exit_loop = true;
                    }
                } break;

                case 1024: {
                    SaveLoadMenu_PlaySfx(FQUIT);
                    GameManager_GameState = GAME_STATE_3_MAIN_MENU;
                    exit_loop = true;
                } break;

                case GNW_KB_KEY_SHIFT_DIVIDE: {
                    if (is_saving_allowed) {
                        HelpMenu_Menu("SAVELOAD_SETUP", WINDOW_MAIN_WINDOW, Remote_IsNetworkGame == false);
                    } else {
                        HelpMenu_Menu("LOAD_SETUP", WINDOW_MAIN_WINDOW);
                    }
                } break;

                case GNW_KB_KEY_ESCAPE: {
                    exit_loop = true;
                } break;

                case GNW_KB_KEY_LALT_P: {
                    PauseMenu_Menu();
                } break;
            }
        }

        if (flc && timer_elapsed_time(time_stamp) >= TIMER_FPS_TO_MS(12.5)) {
            time_stamp = timer_get();
            flicsmgr_advance_animation(flc);
        }

        if (Remote_IsNetworkGame) {
            GameManager_ProcessState(false);

        } else if (GameManager_GameState != GAME_STATE_3_MAIN_MENU && GameManager_GameState != GAME_STATE_6 &&
                   GameManager_GameState != GAME_STATE_10) {
            GameManager_DrawTurnTimer(GameManager_TurnTimerValue, true);
        }

    } while (!exit_loop);

    {
        int32_t button_max_index;

        if (is_saving_allowed) {
            button_max_index = 7;
        } else {
            button_max_index = 5;
        }

        for (int32_t i = 0; i < button_max_index; ++i) {
            delete buttons[i];
        }
    }

    if (flc) {
        flicsmgr_delete(flc);
        free(flc);
    }

    MouseEvent::Clear();

    if (!Remote_IsNetworkGame) {
        Remote_PauseTimeStamp = timer_get();
    }

    return result;
}

void SaveLoadMenu_Save(const char *file_name, const char *save_name, bool play_voice, bool backup) {
    SmartString filename{file_name};
    std::filesystem::path filepath;
    char team_types[PLAYER_TEAM_MAX - 1];
    uint32_t rng_seed;

    filepath = (ResourceManager_FilePathGamePref / filename.Toupper().GetCStr()).lexically_normal();

    if (backup) {
        SaveLoadMenu_CreateBackup(filepath.string().c_str());
    }

    GameManager_GuiSwitchTeam(GameManager_PlayerTeam);

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        team_types[team] = UnitsManager_TeamInfo[team].team_type;
    }

    if (Remote_IsNetworkGame) {
        rng_seed = Remote_RngSeed;
    } else {
        rng_seed = time(nullptr);
    }

    for (int32_t i = 0, limit = strlen(file_name); i < limit; ++i) {
        rng_seed += file_name[i];
    }

    if (SaveLoad_Save(filepath, save_name, rng_seed)) {
        if (play_voice) {
            SoundManager_PlayVoice(V_M013, V_F013);
        }
    }

    for (int32_t team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        UnitsManager_TeamInfo[team].team_type = team_types[team];
    }
}

SaveSlot::SaveSlot() {
    wid = -1;
    ulx = -1;
    uly = -1;
    width = -1;
    height = -1;
    image_up = nullptr;
    image_down = nullptr;
    bid = -1;
    file_name[0] = '\0';
    save_name[0] = '\0';
    in_use = false;
    save_file_type = 0;
    text_edit = nullptr;
}

SaveSlot::~SaveSlot() { Deinit(); }

void SaveSlot::Deinit() {
    WindowInfo window;
    Image *image = new (std::nothrow) Image(ulx, uly, width, height);

    window.id = wid;
    window.buffer = win_get_buf(wid);
    window.width = win_width(wid);

    image->Copy(&window);

    win_delete_button(bid);

    image->Write(&window);

    delete text_edit;
    delete image;
    delete[] image_up;
    delete[] image_down;
}

void SaveSlot::InitTextEdit(WinID wid) {
    WindowInfo window;

    this->wid = wid;

    window.id = wid;
    window.buffer = image_up;
    window.width = width;
    window.window.ulx = ulx;
    window.window.uly = uly;
    window.window.lrx = ulx + width;
    window.window.lry = uly + height;

    text_edit =
        new (std::nothrow) TextEdit(&window, save_name, sizeof(save_name), 45, 39, 143, 17, 0x01, GNW_TEXT_FONT_5);
    text_edit->LoadBgImage();
    text_edit->DrawFullText(false);
}

void SaveSlot::UpdateTextEdit(uint8_t *image) {
    WindowInfo window;

    window.id = wid;
    window.buffer = image;
    window.width = width;
    window.window.ulx = ulx;
    window.window.uly = uly;
    window.window.lrx = ulx + width;
    window.window.lry = uly + height;

    text_edit->UpdateWindow(&window);
    text_edit->DrawFullText();
}

void SaveSlot::SetImageDownForTextEdit() { UpdateTextEdit(image_down); }

void SaveSlot::SetImageUpForTextEdit() { UpdateTextEdit(image_up); }

void SaveSlot::EnterTextEditField() { text_edit->EnterTextEditField(); }

void SaveSlot::LeaveTextEditField() { text_edit->LeaveTextEditField(); }

void SaveSlot::AcceptEditedText() { text_edit->AcceptEditedText(); }

void SaveSlot::DrawSaveSlot(int32_t game_file_type) {
    const char *filename;
    const char *filetype;

    if (in_use) {
        filename = file_name;
        filetype = SaveLoadMenu_SaveTypeTitles[game_file_type];
    } else {
        filename = "";
        filetype = "";
    }

    SaveLoadMenu_DrawSaveSlotResource(image_up, width, FNAME_UP, filename, GNW_TEXT_FONT_5);
    SaveLoadMenu_DrawSaveSlotResource(image_up, width, FTYPE_UP, filetype, GNW_TEXT_FONT_2);

    SaveLoadMenu_DrawSaveSlotResource(image_down, width, FNAME_DN, file_name, GNW_TEXT_FONT_5);
    SaveLoadMenu_DrawSaveSlotResource(image_down, width, FTYPE_DN, SaveLoadMenu_SaveTypeTitles[game_file_type],
                                      GNW_TEXT_FONT_2);
}

bool SaveLoadMenu_RunPlausibilityTests() {
    bool result = false;

    // too slow    result |= SaveLoadChecks_OrderedLists();
    result |= SaveLoadChecks_Defect11();
    result |= SaveLoadChecks_Defect183();
    result |= SaveLoadChecks_Defect151();

    return result;
}

void SaveLoadMenu_CreateBackup(const char *file_name) {
    const auto save_path = std::filesystem::path(file_name).lexically_normal();
    std::error_code ec;

    if (std::filesystem::exists(save_path, ec)) {
        const auto backup_path = std::filesystem::path(file_name).replace_extension(".BAK");

        std::filesystem::rename(save_path, backup_path, ec);

        if (ec) {
            SDL_Log(_(3889), ec.message().c_str());
        }
    }
}
