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
#include "localization.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "saveloadchecks.hpp"
#include "smartfile.hpp"
#include "sound_manager.hpp"
#include "teamunits.hpp"
#include "text.hpp"
#include "textedit.hpp"
#include "units_manager.hpp"
#include "window_manager.hpp"

static_assert(sizeof(struct SaveFormatHeader) == 176,
              "The structure needs to be packed due to save file format compatibility.");

class SaveSlot {
public:
    WinID wid;
    int ulx;
    int uly;
    int width;
    int height;
    unsigned char *image_up;
    unsigned char *image_down;
    ButtonID bid;
    char file_name[15];
    char save_name[30];
    char in_use;
    char game_file_type;
    TextEdit *text_edit;

    SaveSlot();
    ~SaveSlot();

    void Deinit();
    void InitTextEdit(WinID wid);
    void UpdateTextEdit(unsigned char *image);
    void SetImageDownForTextEdit();
    void SetImageUpForTextEdit();
    void EnterTextEditField();
    void LeaveTextEditField();
    void AcceptEditedText();
    void DrawSaveSlot(int game_file_type);
};

const char *SaveLoadMenu_SaveFileTypes[] = {"dta", "tra", "cam", "hot", "mlt", "dmo", "dbg", "txt", "sce", "mps"};
const char *SaveLoadMenu_SaveTypeTitles[] = {_(494c), _(8b45), _(2682), _(4bf8), _(dea8),
                                             _(8a2c), _(1a03), _(7198), _(2fcb), _(3bc3)};

const char *SaveLoadMenu_TutorialTitles[] = {
    _(a03c), _(1ec8), _(357f), _(be30), _(6e67), _(de7c), _(6e4e), _(768e),
    _(5f44), _(1e2d), _(38df), _(dd4a), _(e7f7), _(a7ec), _(979d),
};

const char *SaveLoadMenu_ScenarioTitles[] = {
    _(244f), _(5006), _(8974), _(1b58), _(e302), _(b452), _(abbf), _(e356), _(54fa), _(f88f), _(353a), _(b7dd),
    _(adcf), _(f4d5), _(ab3a), _(625d), _(513e), _(7e10), _(4be1), _(803e), _(5eec), _(da78), _(54eb), _(563d),
};

const char *SaveLoadMenu_CampaignTitles[] = {
    _(78f1), _(db51), _(c13a), _(00b7), _(963a), _(c14a), _(ecc9), _(9d98), _(d033),
};

static int SaveLoadMenu_FirstSaveSlotOnPage = 1;
int SaveLoadMenu_SaveSlot;
unsigned char SaveLoadMenu_GameState;
unsigned short SaveLoadMenu_TurnTimer;
static bool SaveLoadMenu_Flag;
static char SaveLoadMenu_SaveSlotTextEditBuffer[30];

static void SaveLoadMenu_DrawSaveSlotResource(unsigned char *image, int width, ResourceID id, const char *title,
                                              int font_num);
static Button *SaveLoadMenu_CreateButton(WinID wid, ResourceID up, ResourceID down, int ulx, int uly,
                                         const char *caption, int r_value);
static void SaveLoadMenu_Init(SaveSlot *slots, int num_buttons, Button *buttons[], Flic **flc, bool is_saving_allowed,
                              int save_file_type, int first_slot_on_page, bool mode);
static void SaveLoadMenu_PlaySfx(ResourceID id);
static void SaveLoadMenu_EventLoadSlotClick(SaveSlot *slots, int *save_slot_index, int key, int is_saving_allowed);
static void SaveLoadMenu_EventSaveLoadSlotClick(SaveSlot *slots, int save_slot_index, int is_saving_allowed);
static void SaveLoadMenu_UpdateSaveName(struct SaveFormatHeader &save_file_header, int save_slot, int game_file_type);
static void SaveLoadMenu_TeamClearUnitList(SmartList<UnitInfo> &units, unsigned short team);
static bool SaveLoadMenu_RunPlausibilityTests();

void SaveLoadMenu_UpdateSaveName(struct SaveFormatHeader &save_file_header, int save_slot, int game_file_type) {
    const char *title;
    char buffer[50];

    title = nullptr;
    --save_slot;

    switch (game_file_type) {
        case GAME_TYPE_TRAINING: {
            title = SaveLoadMenu_TutorialTitles[save_slot];
        } break;

        case GAME_TYPE_SCENARIO: {
            title = SaveLoadMenu_ScenarioTitles[save_slot];
        } break;

        case GAME_TYPE_CAMPAIGN: {
            title = SaveLoadMenu_CampaignTitles[save_slot];
        } break;

        case GAME_TYPE_MULTI_PLAYER_SCENARIO: {
            sprintf(buffer, "%s #%i", _(2954), save_slot + 1);
            title = buffer;
        } break;
    }

    if (title) {
        strncpy(save_file_header.save_name, title, sizeof(save_file_header.save_name));
    }
}

void SaveLoadMenu_TeamClearUnitList(SmartList<UnitInfo> &units, unsigned short team) {
    for (SmartList<UnitInfo>::Iterator it = units.Begin(); it != units.End(); ++it) {
        if ((*it).team == team) {
            UnitsManager_DestroyUnit(&*it);
        }
    }
}

Button *SaveLoadMenu_CreateButton(WinID wid, ResourceID up, ResourceID down, int ulx, int uly, const char *caption,
                                  int r_value) {
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

void SaveLoadMenu_DrawSaveSlotResource(unsigned char *image, int width, ResourceID id, const char *title,
                                       int font_num) {
    struct ImageSimpleHeader *image_header;
    int buffer_position;

    Text_SetFont(font_num);

    image_header = reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_ReadResource(id));

    buffer_position = image_header->uly * width + image_header->ulx;

    buf_to_buf(&image_header->transparent_color, image_header->width, image_header->height, image_header->width,
               &image[buffer_position], width);

    buffer_position += ((image_header->height - Text_GetHeight()) / 2) * width;

    if (font_num == GNW_TEXT_FONT_2) {
        int offset = (image_header->width - Text_GetWidth(title)) / 2;
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

int SaveLoadMenu_GetSavedGameInfo(int save_slot, int game_file_type, struct SaveFormatHeader &save_file_header,
                                  bool load_ini_options) {
    SmartFileReader file;
    char filename[16];
    char filepath[100];
    bool result;

    sprintf(filename, "save%i.%s", save_slot, SaveLoadMenu_SaveFileTypes[game_file_type]);
    ResourceManager_ToUpperCase(filename);
    filepath[0] = '\0';

    if (game_file_type == GAME_TYPE_CUSTOM || game_file_type == GAME_TYPE_HOT_SEAT ||
        game_file_type == GAME_TYPE_MULTI) {
        strcpy(filepath, ResourceManager_FilePathGameInstall);
    }

    strcat(filepath, filename);

    if (file.Open(filepath)) {
        file.Read(save_file_header);
        SaveLoadMenu_UpdateSaveName(save_file_header, save_slot, game_file_type);

        if (load_ini_options) {
            ini_config.LoadSection(file, INI_OPTIONS, true);
        }

        file.Close();

        result = true;

    } else {
        result = false;
    }

    return result;
}

void SaveLoadMenu_PlaySfx(ResourceID id) { SoundManager.PlaySfx(id); }

void SaveLoadMenu_Init(SaveSlot *slots, int num_buttons, Button *buttons[], Flic **flc, bool is_saving_allowed,
                       int save_file_type, int first_slot_on_page, bool mode) {
    WindowInfo *window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    unsigned char game_file_type;
    char filepath[PATH_MAX];
    char file_name[20];
    FILE *fp;
    unsigned short version;
    ImageSimpleHeader *image_up;
    ImageSimpleHeader *image_down;
    int image_up_size;
    int image_down_size;
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

    for (int i = 0; i < num_buttons; ++i) {
        sprintf(slots[i].file_name, "save%i.%s", first_slot_on_page + i, SaveLoadMenu_SaveFileTypes[save_file_type]);

        strcpy(file_name, slots[i].file_name);
        ResourceManager_ToUpperCase(file_name);

        game_file_type = ini_get_setting(INI_GAME_FILE_TYPE);
        strcpy(filepath, ResourceManager_FilePathGameInstall);
        strcat(filepath, file_name);
        fp = fopen(filepath, "rb");

        if (fp) {
            fread(&version, sizeof(version), 1, fp);
            slots[i].in_use = version == MAX_SAVE_FILE_FORMAT_VERSION;

            fread(&game_file_type, sizeof(game_file_type), 1, fp);
        } else {
            slots[i].in_use = false;
        }

        slots[i].game_file_type = game_file_type;

        if (slots[i].in_use) {
            fread(slots[i].save_name, sizeof(char), 30, fp);
        } else {
            slots[i].save_name[0] = '\0';
        }

        if (fp) {
            fclose(fp);
        }

        image_up =
            reinterpret_cast<ImageSimpleHeader *>(ResourceManager_ReadResource(static_cast<ResourceID>(FILE1_UP + i)));
        image_down =
            reinterpret_cast<ImageSimpleHeader *>(ResourceManager_ReadResource(static_cast<ResourceID>(FILE1_DN + i)));

        image_up_size = image_up->width * image_up->height;
        image_down_size = image_down->width * image_down->height;

        slots[i].image_up = new (std::nothrow) unsigned char[image_up_size];
        slots[i].image_down = new (std::nothrow) unsigned char[image_down_size];

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

        slots[i].DrawSaveSlot(slots[i].game_file_type);

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

int SaveLoadMenu_GetGameFileType() {
    int game_file_type;
    int result;

    game_file_type = ini_get_setting(INI_GAME_FILE_TYPE);

    switch (game_file_type) {
        case GAME_TYPE_TRAINING:
        case GAME_TYPE_CAMPAIGN:
        case GAME_TYPE_SCENARIO: {
            result = 0; /* dta */
        } break;

        case GAME_TYPE_MULTI_PLAYER_SCENARIO: {
            if (Remote_IsNetworkGame) {
                result = 4; /* mlt */
            } else if (GameManager_HumanPlayerCount) {
                result = 3; /* hot */
            } else {
                result = 0; /* dta */
            }
        } break;

        default: {
            result = game_file_type;
        } break;
    }

    return result;
}

void SaveLoadMenu_EventLoadSlotClick(SaveSlot *slots, int *save_slot_index, int key, int is_saving_allowed) {
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

void SaveLoadMenu_EventSaveLoadSlotClick(SaveSlot *slots, int save_slot_index, int is_saving_allowed) {
    SaveLoadMenu_PlaySfx(KCARG0);
    win_set_button_rest_state(slots[save_slot_index].bid, 1, 0);

    if (is_saving_allowed) {
        slots[save_slot_index].EnterTextEditField();
    }
}

int SaveLoadMenu_MenuLoop(int is_saving_allowed) {
    SaveSlot slots[10];
    Button *buttons[7];
    Flic *flc;
    int result;
    int save_slot_index;
    int save_file_type;
    unsigned int time_stamp;
    bool exit_loop;
    int key;
    const int slot_count = sizeof(slots) / sizeof(SaveSlot);

    result = 0;
    flc = nullptr;
    save_slot_index = -1;

    if (!SaveLoadMenu_Flag) {
        SaveLoadMenu_SaveSlotTextEditBuffer[0] = '\0';
    }

    save_file_type = SaveLoadMenu_GetGameFileType();

    SaveLoadMenu_Init(slots, slot_count, buttons, &flc, is_saving_allowed, save_file_type,
                      SaveLoadMenu_FirstSaveSlotOnPage, true);

    time_stamp = timer_get();
    exit_loop = false;

    do {
        if (Remote_GameState == 1) {
            Remote_NetSync();
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
                        (key != GNW_KB_KEY_PAGEDOWN || SaveLoadMenu_FirstSaveSlotOnPage != 91)) {
                        int button_max_index;

                        if (is_saving_allowed) {
                            button_max_index = 7;
                        } else {
                            button_max_index = 5;
                        }

                        for (int i = 0; i < button_max_index; ++i) {
                            delete buttons[i];
                        }

                        for (int i = 0; i < slot_count; ++i) {
                            slots[i].Deinit();
                        }

                        if (key == GNW_KB_KEY_PAGEUP) {
                            if (SaveLoadMenu_FirstSaveSlotOnPage > slot_count) {
                                SaveLoadMenu_FirstSaveSlotOnPage -= slot_count;
                            }
                        } else {
                            SaveLoadMenu_FirstSaveSlotOnPage += slot_count;
                        }

                        SaveLoadMenu_Init(slots, slot_count, buttons, &flc, is_saving_allowed, save_file_type,
                                          SaveLoadMenu_FirstSaveSlotOnPage, false);
                        save_slot_index = -1;
                    }
                } break;

#if defined(MAX_SAVE_TYPE_CONVERSION)
                case GNW_KB_KEY_F1:
                case GNW_KB_KEY_F2: {
                    save_file_type += key == GNW_KB_KEY_F1 ? 1 : -1;

                    if (save_file_type > GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                        save_file_type = GAME_TYPE_CUSTOM;
                    }

                    if (save_file_type < GAME_TYPE_CUSTOM) {
                        save_file_type = GAME_TYPE_MULTI_PLAYER_SCENARIO;
                    }

                    ini_set_setting(INI_GAME_FILE_TYPE, save_file_type);
                    SaveLoadMenu_FirstSaveSlotOnPage = 1;

                    SaveLoadMenu_Init(slots, slot_count, buttons, &flc, is_saving_allowed, save_file_type,
                                      SaveLoadMenu_FirstSaveSlotOnPage, false);
                    save_slot_index = -1;
                } break;
#endif /* defined(MAX_SAVE_TYPE_CONVERSION) */

                case 1022: {
                    SaveLoadMenu_PlaySfx(FSAVE);

                    if (save_slot_index >= 0) {
                        slots[save_slot_index].text_edit->ProcessKeyPress(GNW_KB_KEY_RETURN);
                        slots[save_slot_index].AcceptEditedText();

                        if (Remote_IsNetworkGame) {
                            Remote_SendNetPacket_16(slots[save_slot_index].file_name, slots[save_slot_index].save_name);
                        }

                        if (save_file_type == GAME_TYPE_TRAINING || save_file_type == GAME_TYPE_SCENARIO ||
                            save_file_type == GAME_TYPE_CAMPAIGN || save_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                            GameManager_TurnCounter = 1;
                            GameManager_GameFileNumber = SaveLoadMenu_FirstSaveSlotOnPage + save_slot_index;

                            if (save_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                                for (int i = 0; i < 4; ++i) {
                                    if (UnitsManager_TeamInfo[i].team_type) {
                                        ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + i),
                                                                  menu_team_names[i]);
                                    }
                                }
                            }
                        }

                        SaveLoadMenu_Save(slots[save_slot_index].file_name, slots[save_slot_index].save_name, true);
                        slots[save_slot_index].in_use = true;
                        slots[save_slot_index].DrawSaveSlot(slots[save_slot_index].game_file_type);
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
                        HelpMenu_Menu(HELPMENU_SAVELOAD_SETUP, WINDOW_MAIN_WINDOW, Remote_IsNetworkGame == false);
                    } else {
                        HelpMenu_Menu(HELPMENU_LOAD_SETUP, WINDOW_MAIN_WINDOW);
                    }
                } break;

                case 1023: {
                    SaveLoadMenu_PlaySfx(FLOAD);
                    if (is_saving_allowed && Remote_IsNetworkGame) {
                        int game_state;

                        game_state = GameManager_GameState;
                        GameManager_GameState = GAME_STATE_10;
                        MessageManager_DrawMessage(_(1fc6), 2, 1, true);
                        GameManager_GameState = game_state;
                    } else if (save_slot_index != -1 && slots[save_slot_index].in_use) {
                        result = SaveLoadMenu_FirstSaveSlotOnPage + save_slot_index;
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
                        HelpMenu_Menu(HELPMENU_SAVELOAD_SETUP, WINDOW_MAIN_WINDOW, Remote_IsNetworkGame == false);
                    } else {
                        HelpMenu_Menu(HELPMENU_LOAD_SETUP, WINDOW_MAIN_WINDOW);
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
        int button_max_index;

        if (is_saving_allowed) {
            button_max_index = 7;
        } else {
            button_max_index = 5;
        }

        for (int i = 0; i < button_max_index; ++i) {
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
    SmartFileWriter file;
    SmartString filepath;
    SmartString filename(file_name);
    char team_types[PLAYER_TEAM_MAX - 1];
    struct SaveFormatHeader file_header;
    unsigned short game_state;

    if (!play_voice) {
        bool corruption_detected = SaveLoadMenu_RunPlausibilityTests();

        SDL_assert(!corruption_detected);

        if (corruption_detected) {
            return;
        }
    }

    filepath = ResourceManager_FilePathGameInstall;
    filepath += filename.Toupper();

    if (backup) {
        SaveLoadMenu_CreateBackup(filepath.GetCStr());
    }

    if (file.Open(filepath.GetCStr())) {
        GameManager_GuiSwitchTeam(GameManager_PlayerTeam);
    }

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        team_types[team] = UnitsManager_TeamInfo[team].team_type;
    }

    memset(&file_header, 0, sizeof(file_header));

    file_header.version = MAX_SAVE_FILE_FORMAT_VERSION;
    file_header.save_game_type = ini_get_setting(INI_GAME_FILE_TYPE);

    strcpy(file_header.save_name, save_name);

    file_header.world = ini_get_setting(INI_WORLD);
    file_header.mission_index = GameManager_GameFileNumber;
    file_header.opponent = ini_get_setting(INI_OPPONENT);
    file_header.turn_timer_time = ini_get_setting(INI_TIMER);
    file_header.endturn_time = ini_get_setting(INI_ENDTURN);
    file_header.play_mode = ini_get_setting(INI_PLAY_MODE);

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if ((file_header.save_game_type == GAME_TYPE_TRAINING || file_header.save_game_type == GAME_TYPE_SCENARIO ||
             file_header.save_game_type == GAME_TYPE_CAMPAIGN) &&
            team != PLAYER_TEAM_RED && UnitsManager_TeamInfo[team].team_type == TEAM_TYPE_PLAYER) {
            UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_COMPUTER;
        }

        if (file_header.save_game_type == GAME_TYPE_DEMO && UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            UnitsManager_TeamInfo[team].team_type = TEAM_TYPE_COMPUTER;
        }

        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), file_header.team_name[team],
                                      sizeof(file_header.team_name[team]));
        }

        file_header.team_type[team] = UnitsManager_TeamInfo[team].team_type;
        file_header.team_clan[team] = UnitsManager_TeamInfo[team].team_clan;
    }

    if (Remote_IsNetworkGame) {
        file_header.rng_seed = Remote_RngSeed;
    } else {
        file_header.rng_seed = time(nullptr);
    }

    for (int i = 0, limit = strlen(file_name); i < limit; ++i) {
        file_header.rng_seed += file_name[i];
    }

    file.Write(file_header);

    ini_config.SaveSection(file, INI_OPTIONS);

    file.Write(ResourceManager_MapSurfaceMap,
               ResourceManager_MapSize.x * ResourceManager_MapSize.y * sizeof(unsigned char));

    file.Write(ResourceManager_CargoMap,
               ResourceManager_MapSize.x * ResourceManager_MapSize.y * sizeof(unsigned short));

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        CTInfo *team_info;
        unsigned short unit_id;

        team_info = &UnitsManager_TeamInfo[team];

        file.Write(team_info->markers);
        file.Write(team_info->team_type);
        file.Write(team_info->finished_turn);
        file.Write(team_info->team_clan);
        file.Write(team_info->research_topics);
        file.Write(team_info->team_points);
        file.Write(team_info->number_of_objects_created);
        file.Write(team_info->unit_counters);
        file.Write(team_info->screen_location);
        file.Write(team_info->score_graph, sizeof(team_info->score_graph));

        if (team_info->selected_unit != nullptr) {
            unit_id = team_info->selected_unit->GetId();
        } else {
            unit_id = 0xFFFF;
        }

        file.Write(unit_id);

        file.Write(team_info->zoom_level);
        file.Write(team_info->camera_position.x);
        file.Write(team_info->camera_position.y);
        file.Write(team_info->display_button_range);
        file.Write(team_info->display_button_scan);
        file.Write(team_info->display_button_status);
        file.Write(team_info->display_button_colors);
        file.Write(team_info->display_button_hits);
        file.Write(team_info->display_button_ammo);
        file.Write(team_info->display_button_names);
        file.Write(team_info->display_button_minimap_2x);
        file.Write(team_info->display_button_minimap_tnt);
        file.Write(team_info->display_button_grid);
        file.Write(team_info->display_button_survey);
        file.Write(team_info->stats_factories_built);
        file.Write(team_info->stats_mines_built);
        file.Write(team_info->stats_buildings_built);
        file.Write(team_info->stats_units_built);
        file.Write(team_info->casualties);
        file.Write(team_info->stats_gold_spent_on_upgrades);
    }

    file.Write(GameManager_ActiveTurnTeam);
    file.Write(GameManager_PlayerTeam);
    file.Write(GameManager_TurnCounter);

    game_state = GameManager_GameState;
    file.Write(game_state);

    unsigned short timer_value = GameManager_TurnTimerValue;
    file.Write(timer_value);

    ini_config.SaveSection(file, INI_PREFERENCES);

    ResourceManager_TeamUnitsRed.FileSave(file);
    ResourceManager_TeamUnitsGreen.FileSave(file);
    ResourceManager_TeamUnitsBlue.FileSave(file);
    ResourceManager_TeamUnitsGray.FileSave(file);

    SmartList_UnitInfo_FileSave(UnitsManager_GroundCoverUnits, file);
    SmartList_UnitInfo_FileSave(UnitsManager_MobileLandSeaUnits, file);
    SmartList_UnitInfo_FileSave(UnitsManager_StationaryUnits, file);
    SmartList_UnitInfo_FileSave(UnitsManager_MobileAirUnits, file);
    SmartList_UnitInfo_FileSave(UnitsManager_ParticleUnits, file);

    Hash_UnitHash.FileSave(file);
    Hash_MapHash.FileSave(file);

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        if (UnitsManager_TeamInfo[team].team_type != TEAM_TYPE_NONE) {
            file.Write(UnitsManager_TeamInfo[team].heat_map_complete, 112 * 112);
            file.Write(UnitsManager_TeamInfo[team].heat_map_stealth_sea, 112 * 112);
            file.Write(UnitsManager_TeamInfo[team].heat_map_stealth_land, 112 * 112);
        }
    }

    MessageManager_SaveMessageLogs(file);
    Ai_FileSave(file);
    file.Close();

    for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
        UnitsManager_TeamInfo[team].team_type = team_types[team];
    }

    if (play_voice) {
        SoundManager.PlayVoice(V_M013, V_F013);
    }
}

bool SaveLoadMenu_Load(int save_slot, int game_file_type, bool ini_load_mode) {
    SmartFileReader file;
    char file_name[16];
    char file_path[PATH_MAX];
    struct SaveFormatHeader file_header;
    bool result;
    bool save_load_flag;

    file_path[0] = '\0';

    sprintf(file_name, "save%i.%s", save_slot, SaveLoadMenu_SaveFileTypes[game_file_type]);
    ResourceManager_ToUpperCase(file_name);

    if (game_file_type != GAME_TYPE_TRAINING && game_file_type != GAME_TYPE_CAMPAIGN &&
        game_file_type != GAME_TYPE_SCENARIO && game_file_type != GAME_TYPE_MULTI_PLAYER_SCENARIO &&
        game_file_type != GAME_TYPE_DEMO) {
        strcpy(file_path, ResourceManager_FilePathGameInstall);
    }

    strcat(file_path, file_name);

    if (file.Open(file_path)) {
        file.Read(file_header);
        SaveLoadMenu_UpdateSaveName(file_header, save_slot, game_file_type);

        if (file_header.version == MAX_SAVE_FILE_FORMAT_VERSION) {
            int backup_start_gold;
            int backup_raw_resource;
            int backup_fuel_resource;
            int backup_gold_resource;
            int backup_alien_derelicts;
            unsigned short selected_unit_ids[PLAYER_TEAM_MAX - 1];
            unsigned short game_state;

            if (!Remote_IsNetworkGame) {
                for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                    ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team),
                                              file_header.team_name[team]);
                }
            }

            GameManager_GameFileNumber = file_header.mission_index;

            ResourceManager_InitInGameAssets(file_header.world);

            ini_set_setting(INI_GAME_FILE_TYPE, file_header.save_game_type);

            file_header.opponent = ini_get_setting(INI_OPPONENT);
            file_header.turn_timer_time = ini_get_setting(INI_TIMER);
            file_header.endturn_time = ini_get_setting(INI_ENDTURN);
            file_header.play_mode = ini_get_setting(INI_PLAY_MODE);

            backup_start_gold = ini_get_setting(INI_START_GOLD);
            backup_raw_resource = ini_get_setting(INI_RAW_RESOURCE);
            backup_fuel_resource = ini_get_setting(INI_FUEL_RESOURCE);
            backup_gold_resource = ini_get_setting(INI_GOLD_RESOURCE);
            backup_alien_derelicts = ini_get_setting(INI_ALIEN_DERELICTS);

            ini_config.LoadSection(file, INI_OPTIONS, ini_load_mode);

            if (game_file_type == GAME_TYPE_TRAINING || game_file_type == GAME_TYPE_CAMPAIGN ||
                game_file_type == GAME_TYPE_SCENARIO || game_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                ini_set_setting(INI_OPPONENT, file_header.opponent);
                ini_set_setting(INI_TIMER, file_header.turn_timer_time);
                ini_set_setting(INI_ENDTURN, file_header.endturn_time);
                ini_set_setting(INI_PLAY_MODE, file_header.play_mode);

                ini_set_setting(INI_START_GOLD, backup_start_gold);
                ini_set_setting(INI_RAW_RESOURCE, backup_raw_resource);
                ini_set_setting(INI_FUEL_RESOURCE, backup_fuel_resource);
                ini_set_setting(INI_GOLD_RESOURCE, backup_gold_resource);
                ini_set_setting(INI_ALIEN_DERELICTS, backup_alien_derelicts);
            }

            GameManager_PlayMode = ini_get_setting(INI_PLAY_MODE);

            file.Read(ResourceManager_MapSurfaceMap,
                      ResourceManager_MapSize.x * ResourceManager_MapSize.y * sizeof(unsigned char));

            file.Read(ResourceManager_CargoMap,
                      ResourceManager_MapSize.x * ResourceManager_MapSize.y * sizeof(unsigned short));

            ResourceManager_InitTeamInfo();

            for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                CTInfo *team_info;

                team_info = &UnitsManager_TeamInfo[team];

                file.Read(team_info->markers);
                file.Read(team_info->team_type);
                file.Read(team_info->finished_turn);
                file.Read(team_info->team_clan);
                file.Read(team_info->research_topics);
                file.Read(team_info->team_points);
                file.Read(team_info->number_of_objects_created);
                file.Read(team_info->unit_counters);
                file.Read(team_info->screen_location);
                file.Read(team_info->score_graph, sizeof(team_info->score_graph));
                file.Read(selected_unit_ids[team]);
                file.Read(team_info->zoom_level);
                file.Read(team_info->camera_position.x);
                file.Read(team_info->camera_position.y);
                file.Read(team_info->display_button_range);
                file.Read(team_info->display_button_scan);
                file.Read(team_info->display_button_status);
                file.Read(team_info->display_button_colors);
                file.Read(team_info->display_button_hits);
                file.Read(team_info->display_button_ammo);
                file.Read(team_info->display_button_names);
                file.Read(team_info->display_button_minimap_2x);
                file.Read(team_info->display_button_minimap_tnt);
                file.Read(team_info->display_button_grid);
                file.Read(team_info->display_button_survey);
                file.Read(team_info->stats_factories_built);
                file.Read(team_info->stats_mines_built);
                file.Read(team_info->stats_buildings_built);
                file.Read(team_info->stats_units_built);
                file.Read(team_info->casualties);
                file.Read(team_info->stats_gold_spent_on_upgrades);
            }

            file.Read(GameManager_ActiveTurnTeam);
            file.Read(GameManager_PlayerTeam);
            file.Read(GameManager_TurnCounter);
            file.Read(game_state);

            SaveLoadMenu_GameState = game_state;

            file.Read(SaveLoadMenu_TurnTimer);

            ini_config.LoadSection(file, INI_PREFERENCES, true);

            ResourceManager_TeamUnitsRed.FileLoad(file);
            ResourceManager_TeamUnitsGreen.FileLoad(file);
            ResourceManager_TeamUnitsBlue.FileLoad(file);
            ResourceManager_TeamUnitsGray.FileLoad(file);

            UnitsManager_InitPopupMenus();

            SmartList_UnitInfo_FileLoad(UnitsManager_GroundCoverUnits, file);
            SmartList_UnitInfo_FileLoad(UnitsManager_MobileLandSeaUnits, file);
            SmartList_UnitInfo_FileLoad(UnitsManager_StationaryUnits, file);
            SmartList_UnitInfo_FileLoad(UnitsManager_MobileAirUnits, file);
            SmartList_UnitInfo_FileLoad(UnitsManager_ParticleUnits, file);

            Hash_UnitHash.FileLoad(file);
            Hash_MapHash.FileLoad(file);

            UnitsManager_TeamInfo[PLAYER_TEAM_RED].team_units = &ResourceManager_TeamUnitsRed;
            UnitsManager_TeamInfo[PLAYER_TEAM_GREEN].team_units = &ResourceManager_TeamUnitsGreen;
            UnitsManager_TeamInfo[PLAYER_TEAM_BLUE].team_units = &ResourceManager_TeamUnitsBlue;
            UnitsManager_TeamInfo[PLAYER_TEAM_GRAY].team_units = &ResourceManager_TeamUnitsGray;

            save_load_flag = true;

            for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                CTInfo *team_info;
                char team_name[30];

                team_info = &UnitsManager_TeamInfo[team];

                if (team_info->team_type != TEAM_TYPE_NONE) {
                    if (Remote_IsNetworkGame) {
                        team_info->team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team));

                        if (team_info->team_type == TEAM_TYPE_PLAYER) {
                            GameManager_PlayerTeam = team;
                            ini_config.GetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team), team_name,
                                                      sizeof(team_name));
                            ini_config.SetStringValue(INI_PLAYER_NAME, team_name);
                        }

                    } else if (game_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                        team_info->team_type = ini_get_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team));

                        if (save_load_flag && team_info->team_type == TEAM_TYPE_PLAYER) {
                            GameManager_PlayerTeam = team;
                            save_load_flag = false;
                        }

                    } else {
                        switch (team_info->team_type) {
                            case TEAM_TYPE_PLAYER: {
                                if (game_file_type != GAME_TYPE_HOT_SEAT) {
                                    ini_config.GetStringValue(INI_PLAYER_NAME, team_name, sizeof(team_name));
                                    ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team),
                                                              team_name);
                                }
                            } break;

                            case TEAM_TYPE_COMPUTER: {
                                ini_config.SetStringValue(static_cast<IniParameter>(INI_RED_TEAM_NAME + team),
                                                          menu_team_names[team]);
                            } break;

                            case TEAM_TYPE_REMOTE: {
                                team_info->team_type = TEAM_TYPE_PLAYER;
                            } break;
                        }
                    }

                    ResourceManager_InitHeatMaps(team);

                    if (team_info->team_type != TEAM_TYPE_NONE) {
                        file.Read(team_info->heat_map_complete, 112 * 112);
                        file.Read(team_info->heat_map_stealth_sea, 112 * 112);
                        file.Read(team_info->heat_map_stealth_land, 112 * 112);

                    } else {
                        char *temp_buffer;

                        temp_buffer = new (std::nothrow) char[112 * 112];

                        file.Read(temp_buffer, 112 * 112);
                        file.Read(temp_buffer, 112 * 112);
                        file.Read(temp_buffer, 112 * 112);

                        delete[] temp_buffer;

                        SaveLoadMenu_TeamClearUnitList(UnitsManager_GroundCoverUnits, team);
                        SaveLoadMenu_TeamClearUnitList(UnitsManager_MobileLandSeaUnits, team);
                        SaveLoadMenu_TeamClearUnitList(UnitsManager_StationaryUnits, team);
                        SaveLoadMenu_TeamClearUnitList(UnitsManager_MobileAirUnits, team);
                        SaveLoadMenu_TeamClearUnitList(UnitsManager_ParticleUnits, team);

                        if (team == UnitsManager_Team) {
                            ++UnitsManager_Team;
                        }
                    }

                } else {
                    ResourceManager_InitHeatMaps(team);
                }

                ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_PLAYER + team), team_info->team_type);
            }

            ResourceManager_InitHeatMaps(PLAYER_TEAM_ALIEN);
            MessageManager_LoadMessageLogs(file);

            if (game_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                Ai_Init();

            } else {
                Ai_FileLoad(file);
            }

            file.Close();

            GameManager_UpdateDrawBounds();
            Access_UpdateVisibilityStatus(GameManager_AllVisible);
            GameManager_SelectedUnit = nullptr;

            for (int team = PLAYER_TEAM_RED; team < PLAYER_TEAM_MAX - 1; ++team) {
                if (selected_unit_ids[team] != 0xFFFF) {
                    UnitsManager_TeamInfo[team].selected_unit = Hash_UnitHash[selected_unit_ids[team]];

                } else {
                    UnitsManager_TeamInfo[team].selected_unit = nullptr;
                }
            }

            if (file_header.save_game_type != GAME_TYPE_CAMPAIGN) {
                SaveLoadMenu_SaveSlot = save_slot;
            }

            SaveLoadMenu_Flag = false;

            SaveLoadMenu_RunPlausibilityTests();

            result = true;

        } else {
            MessageManager_DrawMessage(_(61ef), 2, 1, true);
            result = false;
        }

    } else {
        result = false;
    }

    return result;
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
    game_file_type = 0;
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

void SaveSlot::UpdateTextEdit(unsigned char *image) {
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

void SaveSlot::DrawSaveSlot(int game_file_type) {
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

    result |= SaveLoadChecks_Defect11();
    result |= SaveLoadChecks_Defect183();
    result |= SaveLoadChecks_Defect151();

    return result;
}

void SaveLoadMenu_CreateBackup(const char *fil_path) {
    const auto save_path = std::filesystem::path(fil_path).lexically_normal();
    std::error_code ec;

    if (std::filesystem::exists(save_path, ec)) {
        const auto backup_path = std::filesystem::path(fil_path).replace_extension(".BAK");

        std::filesystem::rename(save_path, backup_path, ec);

        if (ec) {
            SDL_Log("Cannot create saved game backup: '%s'.", ec.message().c_str());
        }
    }
}
