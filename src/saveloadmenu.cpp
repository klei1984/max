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

#include "button.hpp"
#include "flicsmgr.hpp"
#include "game_manager.hpp"
#include "gui.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "message_manager.hpp"
#include "mouseevent.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "smartfile.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "textedit.hpp"
#include "textfile.hpp"
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
const char *SaveLoadMenu_SaveTypeTitles[] = {"Custom", "Learning", "Campaign", "Hot seat", "Multi",
                                             "Demo",   "Debug",    "Text",     "Scenario", "MPS"};

const char *SaveLoadMenu_TutorialTitles[] = {
    "Mining and Storage",
    "Factory & Structures",
    "Power & Construction",
    "Surveying",
    "Seek & Destroy",
    "Material Transfer",
    "Mining Allocation",
    "Mining for Gold",
    "Upgrade your Tanks",
    "Victory Points",
    "Infiltrator",
    "Mines",
    "Repairing",
    "Reloading",
    "Research",
};

const char *SaveLoadMenu_ScenarioTitles[] = {
    "Fast & Furious",       "Great Abandon",      "The Long Short Cut",
    "Planes from Hell",     "The Wall",           "Stuck in the Middle",
    "Golden Opportunity",   "Man versus Machine", "One Bullet",
    "Scout Horde",          "The Last Ditch",     "Sneaky Sub",
    "Battle at Sea",        "Their Finest Hour",  "Playing Catch Up",
    "Defense of the Realm", "The King Must Die",  "Covert",
    "To The Death",         "Alien Attack",       "Tank Horde",
    "Stealth Required",     "Beachhead",          "D-Day",
};

const char *SaveLoadMenu_CampaignTitles[] = {
    "Islands in the Sun", "Heart of the Matter",  "Element of Import", "Stone Cold Deadly", "Slaughter Shore",
    "Repel Boarders",     "Bastion of Rebellion", "Bright Hope",       "Price of Freedom",
};

const char *SaveLoadMenu_MultiScenarioTitles[] = {
    "Bottleneck",    "Iron Cross", "4 Player 1 Island", "Small Sea Battle", "Great Circle",
    "Splatterscape", "Middle Sea", "Around The Lake",   "Four Way",
};

static int SaveLoadMenu_FirstSaveSlotOnPage = 1;
static int SaveLoadMenu_SaveSlot;
static char SaveLoadMenu_SaveSlotTextEditBuffer[30];

static void SaveLoadMenu_UpdateSaveName(struct SaveFormatHeader &save_file_header, int save_slot, int game_file_type) {
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
            title = SaveLoadMenu_MultiScenarioTitles[save_slot];

            sprintf(buffer, "%s #%i", "Scenario", save_slot + 1);
            title = buffer;
        } break;
    }

    if (title) {
        strncpy(save_file_header.save_name, title, sizeof(save_file_header.save_name));
    }
}

static Button *SaveLoadMenu_CreateButton(WinID wid, ResourceID up, ResourceID down, int ulx, int uly,
                                         const char *caption, int r_value) {
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

static void SaveLoadMenu_DrawSaveSlotResource(unsigned char *image, int uly, ResourceID id, const char *title,
                                              int font_num) {
    struct ImageSimpleHeader *image_header;
    int buffer_position;

    text_font(font_num);

    image_header = reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_ReadResource(id));

    buffer_position = image_header->height * uly + image_header->width;

    buf_to_buf(image_header->data, image_header->ulx, image_header->uly, image_header->ulx, &image[buffer_position],
               uly);

    buffer_position += ((image_header->uly - text_height()) / 2) * uly;

    if (font_num == 2) {
        buffer_position += (image_header->ulx - text_width(title)) / 2;
    } else {
        buffer_position += 5;
    }

    text_to_buf(&image[buffer_position], title, image_header->ulx + uly + 2, uly, 0x2);

    delete[] image_header;
}

int SaveLoadMenu_GetSavedGameInfo(int save_slot, int game_file_type, struct SaveFormatHeader &save_file_header,
                                  bool load_ini_options) {
    SmartFileReader file;
    char filename[16];
    char filepath[100];
    bool result;

    sprintf(filename, "save%i.%s", save_slot, SaveLoadMenu_SaveFileTypes[game_file_type]);
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

static void SaveLoadMenu_PlaySfx(ResourceID id) { soundmgr.PlaySfx(id); }

static void SaveLoadMenu_Init(SaveSlot *slots, int num_buttons, Button *buttons[], Flic **flc, bool is_saving_allowed,
                              bool is_text_mode, int save_file_type, int first_slot_on_page, bool mode) {
    WindowInfo *window;
    unsigned char game_file_type;
    char filepath[PATH_MAX];
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

    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);

    if (mode) {
        WindowManager_LoadImage(LOADPIC, window, 640, true, false);
    }

    text_font(5);

    Text_TextBox(window->buffer, window->width, is_saving_allowed ? "Save/Load Menu" : "Load Menu", 229, 5, 181, 21,
                 0x2, true);

    for (int i = 0; i < num_buttons; ++i) {
        if (is_text_mode) {
            sprintf(slots[i].file_name, "scnrio%i.txt", first_slot_on_page + i);
        } else {
            sprintf(slots[i].file_name, "save%i.%s", first_slot_on_page + i,
                    SaveLoadMenu_SaveFileTypes[save_file_type]);
        }

        game_file_type = ini_get_setting(INI_GAME_FILE_TYPE);
        strcpy(filepath, ResourceManager_FilePathGameInstall);
        strcat(filepath, slots[i].file_name);
        fp = fopen(filepath, "rb");

        if (fp) {
            if (is_text_mode) {
                slots[i].in_use = true;
            } else {
                fread(&version, sizeof(version), 1, fp);
                slots[i].in_use = version == MAX_SAVE_FILE_FORMAT_VERSION;

                fread(&game_file_type, sizeof(game_file_type), 1, fp);
            }
        } else {
            slots[i].in_use = false;
        }

        slots[i].game_file_type = game_file_type;

        if (slots[i].in_use) {
            if (is_text_mode) {
                fclose(fp);
                fp = nullptr;

                SmartTextfileReader file;

                file.Open(filepath);
                SmartPointer<TextStructure> object = file.GetObject();
                SmartString description = object->ReadString("description");

                strcpy(slots[i].save_name, description.GetCStr());

                file.Close();

            } else {
                fread(slots[i].save_name, sizeof(char), 30, fp);
            }
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

        image_up_size = image_up->ulx * image_up->uly;
        image_down_size = image_down->ulx * image_down->uly;

        slots[i].image_up = new (std::nothrow) unsigned char[image_up_size];
        slots[i].image_down = new (std::nothrow) unsigned char[image_down_size];

        memcpy(slots[i].image_up, image_up->data, image_up_size);
        memcpy(slots[i].image_down, image_down->data, image_down_size);

        text_font(1);

        snprintf(text_slot_index, sizeof(text_slot_index), "%d", first_slot_on_page + i);

        slot_window.buffer = slots[i].image_up;
        slot_window.width = image_up->ulx;
        Text_TextBox(&slot_window, text_slot_index, 0, 0, 40, 70, true, true, FontColor(165, 177, 199));

        slot_window.buffer = slots[i].image_down;
        slot_window.width = image_down->ulx;
        Text_TextBox(&slot_window, text_slot_index, 0, 0, 40, 70, true, true, FontColor(5, 58, 199));

        text_font(5);

        slots[i].ulx = 402 * (i / 5) + 16;
        slots[i].uly = 76 * (i % 5) + 44;
        slots[i].width = image_up->ulx;
        slots[i].height = image_up->uly;

        slots[i].DrawSaveSlot(slots[i].game_file_type);

        slots[i].bid = win_register_button(window->id, slots[i].ulx, slots[i].uly, image_up->ulx, image_up->uly, -1, -1,
                                           1001 + i, 1011 + i, slots[i].image_up, slots[i].image_down, nullptr, 0x1);

        delete[] image_up;
        delete[] image_down;

        slots[i].InitTextEdit(window->id);
        button_list[i] = slots[i].bid;
    }

    win_group_radio_buttons(num_buttons, button_list);

    buttons[0] = SaveLoadMenu_CreateButton(window->id, MNUUAROU, MNUUAROD, 33, 438, nullptr, 329);
    buttons[1] = SaveLoadMenu_CreateButton(window->id, MNUDAROU, MNUDAROD, 63, 438, nullptr, 337);

    text_font(1);
    buttons[2] = SaveLoadMenu_CreateButton(window->id, MNUBTN6U, MNUBTN6D, 514, 438, "Load", 1023);
    buttons[3] = SaveLoadMenu_CreateButton(window->id, MNUBTN5U, MNUBTN5D, 465, 438, "?", 1021);
    buttons[4] = SaveLoadMenu_CreateButton(window->id, MNUBTN4U, MNUBTN4D, 354, 438,
                                           is_saving_allowed ? "Return" : "Cancel", 1000);

    if (is_saving_allowed) {
        buttons[5] = SaveLoadMenu_CreateButton(window->id, MNUBTN3U, MNUBTN3D, 243, 438, "Quit", 1024);
        buttons[6] = SaveLoadMenu_CreateButton(window->id, MNUBTN2U, MNUBTN2D, 132, 438, "Save", 1022);
    }

    win_draw(window->id);

    if (mode) {
        *flc = flicsmgr_construct(FILESFLC, window, 640, 256, 104, 2, false);
    }

    mouse_show();
}

static int SaveLoadMenu_GetGameFileType() {
    int game_file_type;
    int result;

    game_file_type = ini_get_setting(INI_GAME_FILE_TYPE);

    switch (game_file_type) {
        case GAME_TYPE_TRAINING:
        case GAME_TYPE_CAMPAIGN:
        case GAME_TYPE_SCENARIO: {
            result = 0; /* dta */
        } break;

        case GAME_TYPE_CUSTOM:
        case GAME_TYPE_HOT_SEAT:
        case GAME_TYPE_MULTI:
        case GAME_TYPE_DEMO:
        case GAME_TYPE_DEBUG:
        case GAME_TYPE_TEXT:
        case GAME_TYPE_MULTI_PLAYER_SCENARIO: {
            result = game_file_type;
        } break;

        default: {
            if (Remote_IsNetworkGame) {
                result = 4; /* mlt */
            } else if (GameManager_HumanPlayerCount) {
                result = 3; /* hot */
            } else {
                result = 0; /* dta */
            }
        } break;
    }

    return result;
}

static void SaveLoadMenu_EventLoadSlotClick(SaveSlot *slots, int *save_slot_index, int key, int is_saving_allowed) {
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

static void SaveLoadMenu_EventSaveLoadSlotClick(SaveSlot *slots, int save_slot_index, int is_saving_allowed) {
    SaveLoadMenu_PlaySfx(KCARG0);
    win_set_button_rest_state(slots[save_slot_index].bid, 1, 0);

    if (is_saving_allowed) {
        slots[save_slot_index].EnterTextEditField();
    }
}

int SaveLoadMenu_MenuLoop(int is_saving_allowed, int is_text_mode) {
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

    /// \todo Implement missing stuff
    //    if (!byte_1770F8) {
    //        SaveLoadMenu_SaveSlotTextEditBuffer[0] = '\0';
    //    }

    save_file_type = SaveLoadMenu_GetGameFileType();

    SaveLoadMenu_Init(slots, slot_count, buttons, &flc, is_saving_allowed, is_text_mode, save_file_type,
                      SaveLoadMenu_FirstSaveSlotOnPage, true);

    time_stamp = timer_get_stamp32();
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

                        SaveLoadMenu_Init(slots, slot_count, buttons, &flc, is_saving_allowed, is_text_mode,
                                          save_file_type, SaveLoadMenu_FirstSaveSlotOnPage, false);
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
                    is_text_mode = save_file_type == GAME_TYPE_TEXT;
                    SaveLoadMenu_FirstSaveSlotOnPage = 1;

                    SaveLoadMenu_Init(slots, slot_count, buttons, flc, is_saving_allowed, is_text_mode, save_file_type,
                                      SaveLoadMenu_FirstSaveSlotOnPage, false);
                    save_slot_index = -1;
                } break;
#endif /* defined(MAX_SAVE_TYPE_CONVERSION) */

                case 1022: {
                    SaveLoadMenu_PlaySfx(FSAVE);

                    if (save_slot_index >= 0) {
                        slots[save_slot_index].text_edit->ProcessKeyPress(GNW_KB_KEY_RETURN);
                        slots[save_slot_index].AcceptEditedText();

                        if (Remote_IsNetworkGame && !is_text_mode) {
                            Remote_SendNetPacket_16(slots[save_slot_index].file_name, slots[save_slot_index].save_name);
                        }

                        if (save_file_type == GAME_TYPE_TRAINING || save_file_type == GAME_TYPE_SCENARIO ||
                            save_file_type == GAME_TYPE_CAMPAIGN || save_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                            GameManager_TurnCounter = 1;
                            GameMamager_GameFileNumber = SaveLoadMenu_FirstSaveSlotOnPage + save_slot_index;

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

                        game_state = GUI_GameState;
                        GUI_GameState = GAME_STATE_10;
                        MessageManager_DrawMessage("Unable to load a saved game while remote play in progress.", 2, 1,
                                                   true);
                        GUI_GameState = game_state;
                    } else if (save_slot_index != -1 && slots[save_slot_index].in_use) {
                        result = SaveLoadMenu_FirstSaveSlotOnPage + save_slot_index;
                        exit_loop = true;
                    }
                } break;

                case 1024: {
                    SaveLoadMenu_PlaySfx(FQUIT);
                    GUI_GameState = GAME_STATE_3_MAIN_MENU;
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

        if (flc && timer_elapsed_time_ms(time_stamp) >= 80) {
            time_stamp = timer_get_stamp32();
            flicsmgr_advance_animation(flc);
        }

        if (Remote_IsNetworkGame) {
            GameManager_ProcessState(false);

        } else if (GUI_GameState != GAME_STATE_3_MAIN_MENU && GUI_GameState != GAME_STATE_6 &&
                   GUI_GameState != GAME_STATE_10) {
            /// \todo draw_turn_timer(menu_turn_timer_value, true);
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
        Remote_PauseTimeStamp = timer_get_stamp32();
    }

    return result;
}

void SaveLoadMenu_Save(char *file_name, char *save_name, bool play_voice) {}

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

    delete image;
    delete image_up;
    delete image_down;
    delete text_edit;
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

    text_edit = new (std::nothrow) TextEdit(&window, save_name, sizeof(save_name), 45, 39, 143, 17, 0x01, 5);
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

    SaveLoadMenu_DrawSaveSlotResource(image_up, width, FNAME_UP, filename, 5);
    SaveLoadMenu_DrawSaveSlotResource(image_up, width, FTYPE_UP, filetype, 2);

    SaveLoadMenu_DrawSaveSlotResource(image_down, width, FNAME_DN, file_name, 5);
    SaveLoadMenu_DrawSaveSlotResource(image_down, width, FTYPE_DN, SaveLoadMenu_SaveTypeTitles[game_file_type], 2);
}
