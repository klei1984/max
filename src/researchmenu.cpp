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

#include "researchmenu.hpp"

#include "cursor.hpp"
#include "game_manager.hpp"
#include "helpmenu.hpp"
#include "inifile.hpp"
#include "remote.hpp"
#include "reportstats.hpp"
#include "resource_manager.hpp"
#include "sound_manager.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "window.hpp"
#include "window_manager.hpp"

static void ResearchMenu_OnClick_Left(ButtonID bid, intptr_t value);
static void ResearchMenu_OnClick_Right(ButtonID bid, intptr_t value);
static void ResearchMenu_OnClick_Slider(ButtonID bid, intptr_t value);

static void ResearchMenu_OnClick_Done(ButtonID bid, intptr_t value);
static void ResearchMenu_OnClick_Help(ButtonID bid, intptr_t value);
static void ResearchMenu_OnClick_Cancel(ButtonID bid, intptr_t value);

static void ResearchMenu_ApplyUpgrades(unsigned short team, unsigned char research_topic);

class ResearchMenu;

class ResearchControl {
    ResearchMenu *research_menu;
    unsigned char topic_index;
    unsigned short topic_value;
    Button *button_upgrade_right;
    Button *button_upgrade_left;
    Button *button_slider;
    Image *image_labs_count_bg;
    Image *image_turns_bg;
    Image *image_labs_slider_bg;

    friend void ResearchMenu_OnClick_Left(ButtonID bid, intptr_t value);
    friend void ResearchMenu_OnClick_Right(ButtonID bid, intptr_t value);
    friend void ResearchMenu_OnClick_Slider(ButtonID bid, intptr_t value);

public:
    ResearchControl();
    ~ResearchControl();

    void Init(ResearchMenu *menu, unsigned char research_topic_index, unsigned short research_topic_value);
    void RefreshScreen(bool redraw);
    void Update(int value);
    void UpdateButtons();
    unsigned short GetValue() const;
};

class ResearchMenu : public Window {
    unsigned short team;
    unsigned short free_capacity;
    unsigned short active_research_centers;
    bool exit_loop;

    Button *button_cancel;
    Button *button_done;
    Button *button_help;

    ResearchControl topics[RESEARCH_TOPIC_COUNT];

    friend void ResearchMenu_OnClick_Done(ButtonID bid, intptr_t value);
    friend void ResearchMenu_OnClick_Help(ButtonID bid, intptr_t value);
    friend void ResearchMenu_OnClick_Cancel(ButtonID bid, intptr_t value);

public:
    ResearchMenu(unsigned short team);
    ~ResearchMenu();

    unsigned short GetFreeCapacity() const;
    unsigned short GetTeam() const;
    unsigned short GetActiveResearchersCount() const;
    void UpdateTopics(int difference);
    void Run();
};

const char *const ResearchMenu_TopicLabels[] = {"Attack", "Shots", "Range", "Armor", "Hits", "Speed", "Scan", "Cost"};

const ResourceID ResearchMenu_TopicIcons[] = {I_HRDATK, I_SHOTS, I_RANGE, I_ARMOR, I_HITS, I_SPEED, I_SCAN, I_GOLD};

void ResearchMenu_Menu(UnitInfo *unit) { ResearchMenu(unit->team).Run(); }

void ResearchMenu_UpdateResearchProgress(unsigned short team, int research_topic, int allocation) {
    ResearchTopic *topic;
    int topic_factor;
    int turns_to_complete;
    double base;

    topic = &UnitsManager_TeamInfo[team].research_topics[research_topic];

    topic_factor = ini_get_setting(static_cast<IniParameter>(INI_ATTACK_FACTOR + research_topic));

    base = (static_cast<double>(topic->research_level) / 10.0) + 1.0;

    turns_to_complete = ((pow(base + 0.1, 7.5) * 256.0) / static_cast<double>(topic_factor)) -
                        ((pow(base, 7.5) * 256.0) / static_cast<double>(topic_factor));

    topic->allocation += allocation;

    if (topic->allocation && topic->turns_to_complete == 0) {
        topic->turns_to_complete = turns_to_complete;
    }

    if (topic->allocation == 0 && topic->turns_to_complete == turns_to_complete) {
        topic->turns_to_complete = 0;
    }
}

void ResearchMenu_ApplyUpgrades(unsigned short team, unsigned char research_topic) {
    unsigned int research_level;
    TeamUnits *team_units;

    research_level = UnitsManager_TeamInfo[team].research_topics[research_topic].research_level;

    team_units = UnitsManager_TeamInfo[team].team_units;

    for (int unit_type = 0; unit_type < UNIT_END; ++unit_type) {
        SmartPointer<UnitValues> unit_values1 = team_units->GetBaseUnitValues(static_cast<ResourceID>(unit_type));
        SmartPointer<UnitValues> unit_values2 =
            new (std::nothrow) UnitValues(*team_units->GetCurrentUnitValues(static_cast<ResourceID>(unit_type)));
        unsigned short value1;
        unsigned short *value2;
        int old_value;
        int new_value;

        switch (research_topic) {
            case RESEARCH_TOPIC_ATTACK: {
                value1 = unit_values1->GetAttribute(ATTRIB_ATTACK);
                value2 = unit_values2->GetAttributeAddress(ATTRIB_ATTACK);
            } break;

            case RESEARCH_TOPIC_SHOTS: {
                value1 = unit_values1->GetAttribute(ATTRIB_ROUNDS);
                value2 = unit_values2->GetAttributeAddress(ATTRIB_ROUNDS);
            } break;

            case RESEARCH_TOPIC_RANGE: {
                value1 = unit_values1->GetAttribute(ATTRIB_RANGE);
                value2 = unit_values2->GetAttributeAddress(ATTRIB_RANGE);
            } break;

            case RESEARCH_TOPIC_ARMOR: {
                value1 = unit_values1->GetAttribute(ATTRIB_ARMOR);
                value2 = unit_values2->GetAttributeAddress(ATTRIB_ARMOR);
            } break;

            case RESEARCH_TOPIC_HITS: {
                value1 = unit_values1->GetAttribute(ATTRIB_HITS);
                value2 = unit_values2->GetAttributeAddress(ATTRIB_HITS);
            } break;

            case RESEARCH_TOPIC_SPEED: {
                value1 = unit_values1->GetAttribute(ATTRIB_SPEED);
                value2 = unit_values2->GetAttributeAddress(ATTRIB_SPEED);
            } break;

            case RESEARCH_TOPIC_SCAN: {
                value1 = unit_values1->GetAttribute(ATTRIB_SCAN);
                value2 = unit_values2->GetAttributeAddress(ATTRIB_SCAN);
            } break;

            case RESEARCH_TOPIC_COST: {
                value1 = unit_values1->GetAttribute(ATTRIB_TURNS);
                value2 = unit_values2->GetAttributeAddress(ATTRIB_TURNS);
            } break;
        }

        if (research_topic == RESEARCH_TOPIC_COST) {
            old_value = (((research_level + 9) / 2) + value1 * 10) / (research_level + 9);
            new_value = (((research_level + 10) / 2) + value1 * 10) / (research_level + 10);

            old_value = std::max(old_value, 1);
            new_value = std::max(new_value, 1);

        } else {
            old_value = ((research_level - 1) * value1) / 10;
            new_value = (research_level * value1) / 10;
        }

        if (new_value != old_value) {
            *value2 += new_value - old_value;

            unit_values2->UpdateVersion();
            unit_values2->SetUnitsBuilt(0);

            UnitsManager_TeamInfo[team].team_units->SetCurrentUnitValues(unit_type, *unit_values2);
        }
    }
}

void ResearchMenu_NewTurn(unsigned short team) {
    ResearchTopic *topic;

    for (int i = 0; i < RESEARCH_TOPIC_COUNT; ++i) {
        topic = &UnitsManager_TeamInfo[team].research_topics[i];

        if (topic->allocation) {
            topic->turns_to_complete -= topic->allocation;

            if (topic->turns_to_complete <= 0) {
                topic->turns_to_complete = 0;

                ++topic->research_level;

                ResearchMenu_ApplyUpgrades(team, i);

                if (team == GameManager_PlayerTeam) {
                    SoundManager.PlayVoice(V_M093, V_F093);
                }
            }
        }
    }
}

int ResearchMenu_CalculateFactor(unsigned short team, int research_topic, ResourceID unit_type) {
    ResearchTopic *topic;
    int result;

    topic = &UnitsManager_TeamInfo[team].research_topics[research_topic];

    if (topic->research_level) {
        SmartPointer<UnitValues> unit_values = UnitsManager_TeamInfo[team].team_units->GetBaseUnitValues(unit_type);
        int value;

        switch (research_topic) {
            case RESEARCH_TOPIC_ATTACK: {
                value = unit_values->GetAttribute(ATTRIB_ATTACK);
            } break;

            case RESEARCH_TOPIC_SHOTS: {
                value = unit_values->GetAttribute(ATTRIB_ROUNDS);
            } break;

            case RESEARCH_TOPIC_RANGE: {
                value = unit_values->GetAttribute(ATTRIB_RANGE);
            } break;

            case RESEARCH_TOPIC_ARMOR: {
                value = unit_values->GetAttribute(ATTRIB_ARMOR);
            } break;

            case RESEARCH_TOPIC_HITS: {
                value = unit_values->GetAttribute(ATTRIB_HITS);
            } break;

            case RESEARCH_TOPIC_SPEED: {
                value = unit_values->GetAttribute(ATTRIB_SPEED);
            } break;

            case RESEARCH_TOPIC_SCAN: {
                value = unit_values->GetAttribute(ATTRIB_SCAN);
            } break;

            default: {
                SDL_assert(0);
            } break;
        }

        result = (value * topic->research_level) / 10;

    } else {
        result = 0;
    }

    return result;
}

ResearchControl::ResearchControl()
    : research_menu(nullptr),
      topic_index(-1),
      topic_value(0),
      button_upgrade_right(nullptr),
      button_upgrade_left(nullptr),
      button_slider(nullptr),
      image_labs_count_bg(nullptr),
      image_turns_bg(nullptr),
      image_labs_slider_bg(nullptr) {}

ResearchControl::~ResearchControl() {
    delete button_upgrade_right;
    delete button_upgrade_left;
    delete button_slider;
    delete image_labs_count_bg;
    delete image_turns_bg;
    delete image_labs_slider_bg;
}

void ResearchControl::Update(int value) {
    if (value < 0) {
        value = 0;
    }

    if ((value - topic_value) > research_menu->GetFreeCapacity()) {
        value = topic_value + research_menu->GetFreeCapacity();
    }

    if (value != topic_value) {
        int difference;

        difference = topic_value - value;

        topic_value = value;

        RefreshScreen(true);

        research_menu->UpdateTopics(difference);
    }
}

void ResearchControl::UpdateButtons() {
    button_upgrade_right->Enable(research_menu->GetFreeCapacity() > 0);
    button_upgrade_left->Enable(topic_value > 0);
}

unsigned short ResearchControl::GetValue() const { return topic_value; }

void ResearchControl::Init(ResearchMenu *menu, unsigned char research_topic_index,
                           unsigned short research_topic_value) {
    WindowInfo window;
    SmartString string;
    int uly_slider;
    unsigned char *icon;
    struct ImageSimpleHeader *image;

    research_menu = menu;
    topic_index = research_topic_index;
    topic_value = research_topic_value;

    uly_slider = topic_index * 28 + 70;

    research_menu->FillWindowInfo(&window);

    button_upgrade_right = new (std::nothrow) Button(UPGRGT_U, UPGRGT_D, 143, uly_slider);
    button_upgrade_right->SetRFunc(&ResearchMenu_OnClick_Right, reinterpret_cast<intptr_t>(this));
    button_upgrade_right->CopyUpDisabled(UPGRGT_X);
    button_upgrade_right->Copy(window.id);
    button_upgrade_right->Enable(research_menu->GetFreeCapacity());
    button_upgrade_right->RegisterButton(window.id);

    button_upgrade_left = new (std::nothrow) Button(UPGLFT_U, UPGLFT_D, 72, uly_slider);
    button_upgrade_left->SetRFunc(&ResearchMenu_OnClick_Left, reinterpret_cast<intptr_t>(this));
    button_upgrade_left->CopyUpDisabled(UPGLFT_X);
    button_upgrade_left->Copy(window.id);
    button_upgrade_left->Enable(topic_value);
    button_upgrade_left->RegisterButton(window.id);

    button_slider = new (std::nothrow) Button(87, uly_slider, 56, 17);
    button_slider->SetPFunc(&ResearchMenu_OnClick_Slider, reinterpret_cast<intptr_t>(this));
    button_slider->RegisterButton(window.id);

    image_labs_count_bg = new (std::nothrow) Image(38, uly_slider + 3, 21, 10);
    image_labs_count_bg->Copy(&window);

    image_turns_bg = new (std::nothrow) Image(296, uly_slider + 3, 35, 10);
    image_turns_bg->Copy(&window);

    image_labs_slider_bg = new (std::nothrow) Image(91, uly_slider, 48, 17);
    image_labs_slider_bg->Copy(&window);

    text_font(GNW_TEXT_FONT_5);

    Text_TextBox(&window, ResearchMenu_TopicLabels[topic_index], 185, uly_slider, 49, 15, false, true);

    icon = ResourceManager_LoadResource(ResearchMenu_TopicIcons[topic_index]);
    image = reinterpret_cast<struct ImageSimpleHeader *>(icon);

    WindowManager_DecodeImage2(image, 170 - (image->width / 2), (uly_slider + 7) - (image->height / 2), true, &window);

    string.Sprintf(10, "+%i%%",
                   UnitsManager_TeamInfo[research_menu->GetTeam()].research_topics[topic_index].research_level * 10);

    text_to_buf(&window.buffer[(276 + (uly_slider + 3) * window.width) - text_width(string.GetCStr())],
                string.GetCStr(), 35, window.width, 0xA2);

    RefreshScreen(false);
}

void ResearchControl::RefreshScreen(bool redraw) {
    ResearchTopic *topic;
    WindowInfo window;
    int offset;
    struct ImageSimpleHeader *sprite;

    topic = &UnitsManager_TeamInfo[research_menu->GetTeam()].research_topics[topic_index];

    text_font(GNW_TEXT_FONT_5);

    research_menu->FillWindowInfo(&window);

    image_labs_count_bg->Write(&window);

    ReportStats_DrawNumber(&window.buffer[window.width * image_labs_count_bg->GetULY() + image_labs_count_bg->GetULX() +
                                          image_labs_count_bg->GetWidth() - 1],
                           topic_value, image_labs_count_bg->GetWidth(), window.width, 0xA2);

    image_turns_bg->Write(&window);

    if (topic_value > 0) {
        int turns;

        ResearchMenu_UpdateResearchProgress(research_menu->GetTeam(), topic_index, 1);

        turns = (topic_value + topic->turns_to_complete - 1) / topic_value;

        ResearchMenu_UpdateResearchProgress(research_menu->GetTeam(), topic_index, -1);

        ReportStats_DrawNumber(&window.buffer[window.width * image_turns_bg->GetULY() + image_turns_bg->GetULX() +
                                              image_turns_bg->GetWidth() - 1],
                               turns, image_turns_bg->GetWidth(), window.width, 0xA2);
    }

    sprite = reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(PRFSLIDE));

    offset = image_labs_slider_bg->GetWidth() - sprite->width;

    image_labs_slider_bg->Write(&window);

    if (research_menu->GetActiveResearchersCount()) {
        offset = (offset * topic_value) / research_menu->GetActiveResearchersCount();
    } else {
        offset = 0;
    }

    WindowManager_DecodeImage2(sprite, image_labs_slider_bg->GetULX() + offset, image_labs_slider_bg->GetULY(), true,
                               &window);

    if (redraw) {
        image_labs_count_bg->Draw(window.id);
        image_turns_bg->Draw(window.id);
        image_labs_slider_bg->Draw(window.id);
    }
}

void ResearchMenu_OnClick_Left(ButtonID bid, intptr_t value) {
    ResearchControl *control;

    control = reinterpret_cast<ResearchControl *>(value);

    control->Update(control->topic_value - 1);
}

void ResearchMenu_OnClick_Right(ButtonID bid, intptr_t value) {
    ResearchControl *control;

    control = reinterpret_cast<ResearchControl *>(value);

    control->Update(control->topic_value + 1);
}

void ResearchMenu_OnClick_Slider(ButtonID bid, intptr_t value) {
    ResearchControl *control;

    control = reinterpret_cast<ResearchControl *>(value);

    if (control->research_menu->GetActiveResearchersCount() > 0) {
        int width;
        int mouse_x;
        int mouse_y;

        width = control->image_labs_slider_bg->GetWidth();

        control->research_menu->GetCursorPosition(mouse_x, mouse_y);

        mouse_x =
            ((mouse_x - control->image_labs_slider_bg->GetULX()) * control->research_menu->GetActiveResearchersCount() +
             width / 2) /
            width;

        control->Update(mouse_x);
    }
}

void ResearchMenu_OnClick_Done(ButtonID bid, intptr_t value) {
    unsigned short research_topics[RESEARCH_TOPIC_COUNT];
    ResearchMenu *control;

    control = reinterpret_cast<ResearchMenu *>(value);

    control->exit_loop = true;

    for (int i = 0; i < RESEARCH_TOPIC_COUNT; ++i) {
        research_topics[i] = control->topics[i].GetValue();
    }

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == control->team && (*it).unit_type == RESEARCH && (*it).orders == ORDER_POWER_ON) {
            if (research_topics[(*it).research_topic] > 0) {
                --research_topics[(*it).research_topic];

            } else {
                int index;

                for (index = 0; index < RESEARCH_TOPIC_COUNT && !research_topics[index]; ++index) {
                }

                if (index == RESEARCH_TOPIC_COUNT) {
                    UnitsManager_SetNewOrder(&*it, ORDER_POWER_OFF, ORDER_STATE_0);

                } else {
                    ResearchMenu_UpdateResearchProgress(control->team, (*it).research_topic, -1);

                    (*it).research_topic = index;

                    ResearchMenu_UpdateResearchProgress(control->team, (*it).research_topic, 1);

                    --research_topics[index];
                }
            }
        }
    }

    if (Remote_IsNetworkGame) {
        Remote_SendNetPacket_09(control->team);
    }
}

void ResearchMenu_OnClick_Help(ButtonID bid, intptr_t value) {
    HelpMenu_Menu(HELPMENU_RESEARCH_SETUP, WINDOW_MAIN_MAP);
}

void ResearchMenu_OnClick_Cancel(ButtonID bid, intptr_t value) {
    ResearchMenu *control;

    control = reinterpret_cast<ResearchMenu *>(value);

    control->exit_loop = true;
}

ResearchMenu::ResearchMenu(unsigned short team) : Window(RSRCHPIC, WINDOW_MAIN_MAP), team(team) {
    WindowInfo window;
    unsigned short research_topics[RESEARCH_TOPIC_COUNT];

    Cursor_SetCursor(CURSOR_HAND);

    GameManager_DisableMainMenu();

    Add();
    FillWindowInfo(&window);

    text_font(GNW_TEXT_FONT_5);

    Text_TextBox(window.buffer, window.width, "Research Topics", 95, 19, 170, 13, COLOR_GREEN, true);

    text_font(GNW_TEXT_FONT_1);

    Text_TextLine(&window, "Labs", 23, 52, 125, true);
    Text_TextLine(&window, "Topics", 184, 52, 90, true);
    Text_TextLine(&window, "Turns", 282, 52, 60, true);

    text_font(GNW_TEXT_FONT_5);

    button_cancel = new (std::nothrow) Button(DPTBAYUP, DPTBAYDN, 92, 294);
    button_cancel->Copy(window.id);
    button_cancel->SetCaption("Cancel");
    button_cancel->SetRFunc(&ResearchMenu_OnClick_Cancel, reinterpret_cast<intptr_t>(this));
    button_cancel->RegisterButton(window.id);

    button_help = new (std::nothrow) Button(DPTHP_UP, DPTHP_DN, 167, 294);
    button_help->SetRFunc(&ResearchMenu_OnClick_Help, reinterpret_cast<intptr_t>(this));
    button_help->RegisterButton(window.id);

    button_done = new (std::nothrow) Button(DPTBAYUP, DPTBAYDN, 194, 294);
    button_done->Copy(window.id);
    button_done->SetCaption("Done");
    button_done->SetRFunc(&ResearchMenu_OnClick_Done, reinterpret_cast<intptr_t>(this));
    button_done->RegisterButton(window.id);

    free_capacity = 0;
    active_research_centers = 0;
    exit_loop = false;

    memset(research_topics, 0, sizeof(research_topics));

    for (SmartList<UnitInfo>::Iterator it = UnitsManager_StationaryUnits.Begin();
         it != UnitsManager_StationaryUnits.End(); ++it) {
        if ((*it).team == team && (*it).unit_type == RESEARCH && (*it).orders == ORDER_POWER_ON) {
            ++active_research_centers;
            ++research_topics[(*it).research_topic];
        }
    }

    for (int i = 0; i < RESEARCH_TOPIC_COUNT; ++i) {
        topics[i].Init(this, i, research_topics[i]);
    }

    win_draw(window.id);
}

ResearchMenu::~ResearchMenu() {
    delete button_cancel;
    delete button_help;
    delete button_done;

    GameManager_EnableMainMenu(&*GameManager_SelectedUnit);
    GameManager_ProcessTick(true);
}

unsigned short ResearchMenu::GetFreeCapacity() const { return free_capacity; }

unsigned short ResearchMenu::GetTeam() const { return team; }

unsigned short ResearchMenu::GetActiveResearchersCount() const { return active_research_centers; }

void ResearchMenu::UpdateTopics(int difference) {
    free_capacity += difference;

    for (int i = 0; i < RESEARCH_TOPIC_COUNT; ++i) {
        topics[i].UpdateButtons();
    }
}

void ResearchMenu::Run() {
    exit_loop = false;
    int key;

    while (!exit_loop) {
        key = get_input();

        if (GameManager_RequestMenuExit || key == GNW_KB_KEY_ESCAPE) {
            exit_loop = true;
        }

        GameManager_ProcessState(true);
    }
}
