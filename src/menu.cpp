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

#include "menu.hpp"

#include <ctime>

#include "button.hpp"
#include "chooseplayermenu.hpp"
#include "clanselectmenu.hpp"
#include "cursor.hpp"
#include "game_manager.hpp"
#include "gameconfigmenu.hpp"
#include "gamesetupmenu.hpp"
#include "ginit.h"
#include "gui.hpp"
#include "gwindow.hpp"
#include "inifile.hpp"
#include "movie.h"
#include "optionsmenu.hpp"
#include "planetselectmenu.hpp"
#include "remote.hpp"
#include "saveloadmenu.hpp"
#include "smartstring.hpp"
#include "soundmgr.hpp"
#include "text.hpp"
#include "units_manager.hpp"
#include "version.hpp"

struct MenuButton {
    Button* button;
    char big_size;
    short ulx;
    short uly;
    const char* label;
    int r_value;
    ResourceID sfx;
};

#define MENU_BUTTON_DEF(is_big, ulx, uly, label, r_value, sfx) \
    { nullptr, (is_big), (ulx), (uly), (label), (r_value), (sfx) }

struct CreditsLine {
    unsigned short color;
    const char* text;
};

#define CREDITS_SEPARATOR \
    { (0x00), ("") }

#define CREDITS_TITLE(text) \
    { (0xFF), (text) }

#define CREDITS_TEXT(text) \
    { (0x04), (text) }

static ResourceID menu_portrait_id;

static struct MenuButton* menu_button_items;
static int menu_button_items_count;

static int menu_game_file_number;

static struct MenuButton main_menu_buttons[] = {
    MENU_BUTTON_DEF(true, 385, 175, "New Game", GNW_KB_KEY_SHIFT_N, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 210, "Load Game", GNW_KB_KEY_SHIFT_L, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 245, "Multiplayer Game", GNW_KB_KEY_SHIFT_M, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 280, "Setup", GNW_KB_KEY_SHIFT_S, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 315, "Introduction", GNW_KB_KEY_SHIFT_I, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 350, "Credits", GNW_KB_KEY_SHIFT_C, MBUTT0),
    MENU_BUTTON_DEF(false, 435, 420, "Exit", GNW_KB_KEY_SHIFT_E, NDONE0),
    MENU_BUTTON_DEF(false, 16, 182, nullptr, GNW_KB_KEY_UP, KCARG0),
};

static struct MenuButton new_game_menu_buttons[] = {
    MENU_BUTTON_DEF(true, 385, 175, "Training Missions", GNW_KB_KEY_SHIFT_T, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 210, "Stand Alone Missions", GNW_KB_KEY_SHIFT_S, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 245, "Custom Game", GNW_KB_KEY_SHIFT_U, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 280, "Custom Scenarios", GNW_KB_KEY_SHIFT_M, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 315, "Campaign Game", GNW_KB_KEY_SHIFT_A, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 350, "Tips", GNW_KB_KEY_SHIFT_I, MBUTT0),
    MENU_BUTTON_DEF(false, 435, 420, "Cancel", GNW_KB_KEY_SHIFT_C, NCANC0),
    MENU_BUTTON_DEF(false, 16, 182, nullptr, GNW_KB_KEY_UP, KCARG0),
};

static struct MenuButton network_game_menu_buttons[] = {
    MENU_BUTTON_DEF(true, 385, 175, "Host Network Game", GNW_KB_KEY_SHIFT_H, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 210, "Join Network Game", GNW_KB_KEY_SHIFT_J, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 280, "Hot Seat Game", GNW_KB_KEY_SHIFT_O, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 315, "Hot Seat Scenarios", GNW_KB_KEY_SHIFT_T, MBUTT0),
    MENU_BUTTON_DEF(true, 385, 350, "Load Hot Seat Game", GNW_KB_KEY_SHIFT_L, MBUTT0),
    MENU_BUTTON_DEF(false, 435, 420, "Cancel", GNW_KB_KEY_SHIFT_C, NCANC0),
    MENU_BUTTON_DEF(false, 16, 182, nullptr, GNW_KB_KEY_UP, KCARG0),
};

static const char* menu_tips_data;
static SmartString* menu_tips_strings;
static Button* menu_tips_button_up;
static Button* menu_tips_button_down;
static int menu_tips_max_row_count_per_page;
static int menu_tips_row_count;
static int menu_tips_current_row_index;

static struct CreditsLine menu_credits_lines[] = {
    CREDITS_TITLE("Project Manager:"),
    CREDITS_TEXT("Ali Atabek"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Producers:"),
    CREDITS_TEXT("Ali Atabek"),
    CREDITS_TEXT("Paul Kellner"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Game Design:"),
    CREDITS_TEXT("Ali Atabek"),
    CREDITS_TEXT("Paul Kellner"),
    CREDITS_TEXT("Gus Smedstad"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Lead Programmer:"),
    CREDITS_TEXT("Dave Boulanger"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Artificial Intelligence:"),
    CREDITS_TITLE("Design & Programming:"),
    CREDITS_TEXT("Gus Smedstad"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Network & Modem Programming:"),
    CREDITS_TEXT("Dave Boulanger"),
    CREDITS_TEXT("Bernie Weir"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("DOS and Windows Install Programmer:"),
    CREDITS_TEXT("Darren Monahan"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("2D Artists:"),
    CREDITS_TEXT("Tony Postma"),
    CREDITS_TEXT("Arlene C. Somers"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("3D Artists:"),
    CREDITS_TEXT("Chris Regalado"),
    CREDITS_TEXT("Mark Bergo"),
    CREDITS_TEXT("Arlene C. Somers"),
    CREDITS_TEXT("Mike Dean"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Visual Concept & Storyboards:"),
    CREDITS_TEXT("Tony Postma"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Original World Design:"),
    CREDITS_TEXT("Cheryl Austin"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("World Creation:"),
    CREDITS_TEXT("Arlene C. Somers"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Map Design:"),
    CREDITS_TEXT("Ali Atabek"),
    CREDITS_TEXT("Paul Kellner"),
    CREDITS_TEXT("Steve Perrin"),
    CREDITS_TEXT("Gus Smedstad"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Audio Director:"),
    CREDITS_TEXT("Charles Deenen"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Head Writer:"),
    CREDITS_TEXT("Steve Perrin"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Writers:"),
    CREDITS_TEXT("Paul Kellner"),
    CREDITS_TEXT("Tony Postma"),
    CREDITS_TEXT("Amy Mitchell"),
    CREDITS_TEXT("Laura Mitchell"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Editor:"),
    CREDITS_TEXT("Kelly Newcomb"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Box Cover Design:"),
    CREDITS_TEXT("Tony Postma"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Director Of Quality Assurance:"),
    CREDITS_TEXT("Chad Allison"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Assistant Director Of Quality Assurance:"),
    CREDITS_TEXT("Colin Totman"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Lead Tester:"),
    CREDITS_TEXT("Cory Nelson"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Testers:"),
    CREDITS_TEXT("Amy Mitchell"),
    CREDITS_TEXT("Chris Peak"),
    CREDITS_TEXT("Quinn Summers"),
    CREDITS_TEXT("Doug Avery"),
    CREDITS_TEXT("Steve McLafferty"),
    CREDITS_TEXT("Erick Lujan"),
    CREDITS_TEXT("Steve Victory"),
    CREDITS_TEXT("James Dunn"),
    CREDITS_TEXT("Henry Kahng"),
    CREDITS_TEXT("Bill Field"),
    CREDITS_TEXT("Steve Reed"),
    CREDITS_TEXT("John Stavros"),
    CREDITS_TEXT("Steve Baldoni"),
    CREDITS_TEXT("Richard Barker"),
    CREDITS_TEXT("Tony Bland"),
    CREDITS_TEXT("Larry Smith"),
    CREDITS_TEXT("Daniel Huffman"),
    CREDITS_TEXT("Arlen Nydam"),
    CREDITS_TEXT("Charles Crail"),
    CREDITS_TEXT("Rene Hakiki"),
    CREDITS_TEXT("Jon Atabek"),
    CREDITS_TEXT("Robert Wood"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("I.S. Technicians:"),
    CREDITS_TEXT("Bill Delk"),
    CREDITS_TEXT("Aaron J. Meyers"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Director Of Compatibility:"),
    CREDITS_TEXT("John Werner"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Compatibility Technicians:"),
    CREDITS_TEXT("Dan Forsyth"),
    CREDITS_TEXT("John Parker"),
    CREDITS_TEXT("Aaron Olaiz"),
    CREDITS_TEXT("Derek Gibbs"),
    CREDITS_TEXT("Phuong Nguyen"),
    CREDITS_TEXT("Marc Duran"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Video Compression and Playback Technology:"),
    CREDITS_TEXT("Paul Edelstein"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Video Compression:"),
    CREDITS_TEXT("Bill Stoudt"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Marketing Manager:"),
    CREDITS_TEXT("Jim Veevaert"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Associate Marketing Manager:"),
    CREDITS_TEXT("Dean Schulte"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("PR Manager:"),
    CREDITS_TEXT("Julie Roether"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Voices:"),
    CREDITS_TEXT("Nicole Pelerine"),
    CREDITS_TEXT("Brian Cummings"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("VO Director:"),
    CREDITS_TEXT("Charles Deenen"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("VO Supervision:"),
    CREDITS_TEXT("Chris Borders"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("VO Mastering:"),
    CREDITS_TEXT("Craig Duman"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("VO Editing:"),
    CREDITS_TEXT("Sergio Bustamante"),
    CREDITS_TEXT("Chris Borders"),
    CREDITS_TEXT("Craig Duman"),
    CREDITS_TEXT("Doug Rappaport"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Gameplay Music:"),
    CREDITS_TEXT("Brian Luzietti"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Additional Gameplay Music:"),
    CREDITS_TEXT("Rick Jackson"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Music Mastering:"),
    CREDITS_TEXT("Digital Brothers"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Game Soundeffects:"),
    CREDITS_TEXT("Gregory G. Allen"),
    CREDITS_TEXT("Larry Peacock"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Soundeffects Mastering:"),
    CREDITS_TEXT("Craig Duman"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Intro Movie:"),
    CREDITS_TEXT("James Doyle"),
    CREDITS_TEXT("Apple's Animation"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Cinematics Music:"),
    CREDITS_TEXT("Albert Olsen for Four Bars Intertainment"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Cinematics Sound Design:"),
    CREDITS_TEXT("David Farmer"),
    CREDITS_TEXT("Harry Cohen"),
    CREDITS_TEXT("Jeffrey R. Whitcher"),
    CREDITS_TEXT("Elisabeth Flaum"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Cinematics Mixers:"),
    CREDITS_TEXT("Ken Teaney"),
    CREDITS_TEXT("Mashall Garlington"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Cinematics Sound Supervision:"),
    CREDITS_TEXT("Charles Deenen"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Cinematics Ambient Voices:"),
    CREDITS_TEXT("Doug Rappaport"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Cinematics Voice Processing:"),
    CREDITS_TEXT("Doug Rappaport"),
    CREDITS_TEXT("Sergio Bustamante"),
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Mixed in Dolby Surround at"),
    CREDITS_TITLE("EFX Systems, Burbank"),
    CREDITS_TITLE("Dolby and the double-D are trademarks of"),
    CREDITS_TITLE("Dolby Laboratories Licensing Corporation."),
    CREDITS_SEPARATOR,
    CREDITS_SEPARATOR,
    CREDITS_SEPARATOR,
    CREDITS_SEPARATOR,
    CREDITS_TITLE("Thanks to Tim Cain and Chris Jones for GNW,"),
    CREDITS_TITLE("Jay Patel and Paul Edelstein"),
    CREDITS_TITLE("for Technical Assistance,"),
    CREDITS_TITLE("Sanjay Bala-Krishnan for all his support,"),
    CREDITS_TITLE("James Thomas for Story Elements"),
    CREDITS_TITLE("and assorted inspirations,"),
    CREDITS_TITLE("Newtek, Inc. for Lightwave 3D,"),
    CREDITS_TITLE("and Parallax Software for the use of"),
    CREDITS_TITLE("their Installation and Setup Programs."),
    CREDITS_SEPARATOR,
    CREDITS_SEPARATOR,
    CREDITS_SEPARATOR,
};

const char* menu_planet_descriptions[] = {
    "This world is named for the shape of its only landmass. If participants on this world are placed too close "
    "together, a production strategy is unlikely to work.\nNumber of Participants: Two to three participants is "
    "preferable.\nGame Time: Games on this land mass can be very short unless the combatants are well separated.",

    "This area is likely to need a full deployment of land, sea, and air forces. A force starting on the same land "
    "mass as another might abandon their base camp and striking out for another land mass to start over.\nNumber of "
    "Participants: two to four can be easily accommodated.\nGame Time: Games in this area are likely to be long.",

    "These chunks of ice poking up out of the frozen sea are widely enough separated that participants should have "
    "plenty of time to build up naval and air forces to pursue their goals.\nNumber of Participants: two to four can "
    "be easily accommodated.\nGame Time: Games in this area are likely to be long.",

    "This single land mass with surrounding islands is so small that two participants starting on the mass makes "
    "immediate contact with the enemy very likely. Start on one of the outlying islands to gain plenty of time for "
    "force increase.\nNumber of Participants: Two to three participants is preferable.\nGame Time: Games in this area "
    "can be very short.",

    "Participants who land on one of the smaller land masses should have plenty of time to build production before "
    "encountering an opponent. Landing two on the same land mass will cause instant combat.\nNumber of Participants: "
    "two to four can be easily accommodated.\nGame Time: Games in this area are likely to be long.",

    "The nature of this land mass makes construction of water platforms mandatory for a successful colonization. The "
    "chance of a preemptive strike by an opponent is high, though not as high as from other single-land areas.\nNumber "
    "of Participants: Two to three participants is preferable.\nGame Time: Games in this area should be of medium to "
    "long length.",

    "A starting position on one of the outlying islands almost ensures a long time for buildup before contact is made "
    "with an enemy.\nNumber of Participants: two to four can be easily accommodated.\nGame Time: A long game unless "
    "the participants are all started in close proximity on the main continent.",

    "This archipelago of closely placed islands could be the scene of an entirely land and bridge based campaign, "
    "though sea and air assets would be useful.\nNumber of Participants: two to four can be easily accommodated.\nGame "
    "Time: Likely to be long unless there are only two participants and they both land on the same island.",

    "Only these few islands peak up from the omnipresent muddy seas of this world. The islands are sufficiently "
    "scattered that sea and air elements are necessary for any successful endeavor.\nNumber of Participants: two to "
    "four can be easily accommodated.\nGame Time: This is likely to be a long campaign.",

    "Named for the heart-shaped great lake in the center of the land mass, this world can be won by land and air "
    "forces alone, but the use of sea forces in the central lake could be crucial.\nNumber of Participants: Two to "
    "four can be easily accommodated.\nGame Time: This is likely to be a long campaign.",

    "The name of this world refers to the three lakes encompassed by the otherwise solid land mass of the world. There "
    "is a chance that a production-first strategy can run afoul of a preemptory strike by another participant.\nNumber "
    "of Participants: Two to three participants is preferable.\nGame Time: Probably short.",

    "This world has a lot of space and virtually every land mass has plenty of room for large armies. Depending on "
    "initial placements, the islands might see no activity at all. If every place is populated, expect intense "
    "seaborne combats.\nNumber of Participants: two to four can be easily accommodated.\nGame Time: Most games in this "
    "location will be long.",

    "This archipelago is tightly grouped in the middle of the planetary sea. There are opportunities for extensive air "
    "and sea resources, though an entirely land-and-air-based campaign could be successful.\nNumber of Participants: "
    "Two to three participants is preferable.\nGame Time: varies greatly depending on the landing points of the "
    "participants.",

    "The useful parts of this world are split apart by a central sea. Control of the central sea with seaborne assets "
    "will be essential unless the participants all land on the same continent.\nNumber of Participants: two to four "
    "can be easily accommodated.\nGame Time: will be long unless two participants land close together.",

    "This world is generally resourceless except for the region around the impact crater from a major asteroid strike. "
    "A combatant can win using just land and air forces. However, control of the inner sea could be important.\nNumber "
    "of Participants: two to four can be easily accommodated.\nGame Time: the game can be very long.",

    "This archipelago with a slice of mainland is suited to a balanced combined arm force with extensive resource "
    "building. Taking sanctuary from the incessant naval action on the slice of mainland is a legitimate tactic, of "
    "course.\nNumber of Participants: Two to three participants is preferable.\nGame Time: The game will probably be "
    "long.",

    "This archipelago is best suited to a concentration on sea borne forces. Extensive engineering will be necessary "
    "to create areas for proper exploitation of the planetary resources.\nNumber of Participants: two to four can be "
    "easily accommodated.\nGame Time: The necessary construction makes this a long game.",

    "The extensive land area on this planet makes it ideal for extensive land-based warfare. Sea borne assets may play "
    "a necessary but secondary role, depending on initial placement.\nNumber of Participants: two to four can be "
    "easily accommodated.\nGame Time: this is likely to be a long game.",

    "This planet gets its name from its appearance when seen from space. The mountains jutting out of the desert give "
    "it a decidedly freckled appearance. There might be extensive use of naval assets, or none.\nNumber of "
    "Participants: two to four can be easily accommodated.\nGame Time: Most games in this location will be long games.",

    "Very few lumps of sand poke out of the ocean on this planet. Its situation demands heavy use of water platforms "
    "and naval assets. Two participants are very unlikely to end up on the same land mass.\nNumber of Participants: "
    "Two to three participants is preferable.\nGame Time: Most games in this location will be long games.",

    "Like any other world where all the land is connected, this can be very risky for a high-production strategy. "
    "However, its size does give some protection against a preemptive strike by another participant.\nNumber of "
    "Participants: two to four can be easily accommodated.\nGame Time: medium to long unless participants' landing "
    "points are close.",

    "The sea passage that separates the two main land masses makes naval action likely here. Those who wish to follow "
    "a production strategy might land on the island to the north east.\nNumber of Participants: two to four can be "
    "easily accommodated.\nGame Time: this is likely to be a long game.",

    "The central island of this planet makes a natural battleground for all participants, whether they start there or "
    "not. Start on one of the corner lands t gain a reasonable amount of time for a strategic buildup.\nNumber of "
    "Participants: two to four can be easily accommodated.\nGame Time: Games on this world are likely to be very long.",

    "This world gets its name from the land bottleneck controllable by sea forces and the sea bottleneck controllable "
    "by land forces. Both are in the center of the map. Extensive use of all forces is likely in this "
    "location.\nNumber of Participants: two to four can be easily accommodated.\nGame Time: Most games on this world "
    "are likely to be very long."};

const char* menu_planet_names[] = {"Snowcrab",     "Frigia",       "Ice Berg",      "The Cooler", "Ultima Thule",
                                   "Long Floes",   "Iron Cross",   "Splatterscape", "Peak-a-boo", "Valentine's Planet",
                                   "Three Rings",  "Great divide", "New Luzon",     "Middle Sea", "High Impact",
                                   "Sanctuary",    "Islandia",     "Hammerhead",    "Freckles",   "Sandspit",
                                   "Great Circle", "Long Passage", "Flash Point",   "Bottleneck"};

static const char* save_file_extensions[] = {"dmo", "dbg", "txt", "sce", "mps"};

static const char* menu_team_names[] = {"Red Team", "Green Team", "Blue Team", "Gray Team"};

void draw_menu_title(WindowInfo* window, const char* caption) {
    text_font(5);
    Text_TextBox(window->buffer, window->width, caption, 236, 145, 172, 15, 2, true, true);
}

void menu_draw_menu_portrait_frame(WindowInfo* window) {
    gwin_load_image2(BGRSCRNL, 6, 173, 0, window);
    gwin_load_image2(BGRSCRNR, 166, 173, 0, window);
}

void menu_draw_menu_portrait(WindowInfo* window, ResourceID portrait, bool draw_to_screen) {
    menu_draw_menu_portrait_frame(window);

    if (portrait != menu_portrait_id || portrait == INVALID_ID) {
        while (portrait == INVALID_ID || portrait == menu_portrait_id || portrait == P_SHIELD || portrait == P_LIFESP ||
               portrait == P_RECCTR || portrait == P_MASTER || portrait == P_JUGGER || portrait == P_ALNTAN ||
               portrait == P_ALNASG || portrait == P_ALNPLA) {
            portrait = static_cast<ResourceID>(P_TRANSP + ((67 * dos_rand()) >> 15));
        }

        menu_portrait_id = portrait;
    }

    WindowInfo wininfo;

    wininfo.buffer = &window->buffer[window->width * 182 + 16];
    wininfo.width = window->width;
    wininfo.id = window->id;
    wininfo.window.ulx = 16;
    wininfo.window.uly = 182;
    wininfo.window.lrx = 316;
    wininfo.window.lry = 422;

    gwin_load_image(menu_portrait_id, &wininfo, wininfo.width, false, false);

    if (draw_to_screen) {
        win_draw_rect(wininfo.id, &wininfo.window);
    }
}

void menu_draw_menu_title(WindowInfo* window, MenuTitleItem* menu_item, int color, bool horizontal_align = false,
                          bool vertical_align = true) {
    if (menu_item->title && strlen(menu_item->title)) {
        Text_TextBox(window->buffer, window->width, menu_item->title, menu_item->bounds.ulx, menu_item->bounds.uly,
                     menu_item->bounds.lrx - menu_item->bounds.ulx, menu_item->bounds.lry - menu_item->bounds.uly,
                     color, horizontal_align, vertical_align);
    }
}

void menu_draw_logo(ResourceID resource_id, int time_limit) {
    WindowInfo* window;
    unsigned int time_stamp;

    window = gwin_get_window(GWINDOW_MAIN_WINDOW);

    if (gwin_load_image(resource_id, window, 640, true, false)) {
        Cursor_SetCursor(CURSOR_HIDDEN);
        mouse_show();
        setSystemPalette(system_palette_ptr);
        win_draw(window->id);
        gwin_fade_out(500);

        time_stamp = timer_get_stamp32();
        while (timer_elapsed_time_ms(time_stamp) < time_limit && get_input() != -1) {
        }

        gwin_clear_window();
    }
}

template <typename T>
void ReadFile(FILE* fp, T& buffer) {
    if (fread(&buffer, sizeof(char), sizeof(T), fp) != (sizeof(T))) {
        gexit(7);
    }
}

template <typename T>
void ReadFile(FILE* fp, T* buffer, int size) {
    if (fread(buffer, sizeof(char), size, fp) != size) {
        gexit(7);
    }
}

int Menu_LoadPlanetMinimap(int planet_index, char* buffer, int width) {
    char* filename;
    int result;

    filename = ResourceManager_LoadResource(static_cast<ResourceID>(SNOW_1 + planet_index));

    if (filename) {
        FILE* fp;

        fp = fopen(filename, "rb");
        free(filename);

        if (fp) {
            char map_type[5];
            Point map_dimensions;
            short map_tile_count;
            char palette[256 * 3];
            int offset;

            ReadFile(fp, map_type);
            ReadFile(fp, map_dimensions);

            for (int i = 0; i < map_dimensions.x; ++i) {
                ReadFile(fp, &buffer[width * i], map_dimensions.y);
            }

            fseek(fp, map_dimensions.x * map_dimensions.y * 2, SEEK_CUR);

            ReadFile(fp, map_tile_count);
            fseek(fp, map_tile_count * 64 * 64, SEEK_CUR);

            ReadFile(fp, palette);

            for (int i = 0; i < sizeof(palette); ++i) {
                palette[i] /= 4;
            }

            color_palette = cmap;

            for (int i = 0; i < sizeof(palette); i + 3) {
                palette[i / 3] = Resrcmgr_sub_D3A88(palette[i], palette[i + 1], palette[i + 2], 1);
            }

            for (int i = 0, offset = 0; i < map_dimensions.x; ++i) {
                for (int j = 0; j < map_dimensions.y; ++j) {
                    buffer[i][j + offset] = palette[buffer[i][j + offset]];
                }

                offset += width - map_dimensions.x;
            }

            fclose(fp);
            result = true;

        } else {
            result = false;
        }
    } else {
        result = false;
    }

    return result;
}

void menu_update_resource_levels() {
    int resource_min;
    int resource_max;
    int resource_raw;
    int resource_fuel;
    int resource_gold;
    int resource_derelicts;

    resource_raw = ini_get_setting(INI_RAW_RESOURCE);
    resource_fuel = ini_get_setting(INI_FUEL_RESOURCE);
    resource_gold = ini_get_setting(INI_GOLD_RESOURCE);
    resource_derelicts = ini_get_setting(INI_ALIEN_DERELICTS);

    resource_min = 1;
    resource_max = 15;

    switch (resource_raw) {
        case 0: {
            ini_set_setting(INI_RAW_NORMAL_LOW, 0);
            ini_set_setting(INI_RAW_NORMAL_HIGH, 3);
            ini_set_setting(INI_RAW_CONCENTRATE_LOW, 8);
            ini_set_setting(INI_RAW_CONCENTRATE_HIGH, 12);
            ini_set_setting(INI_RAW_CONCENTRATE_SEPERATION, 28);
            ini_set_setting(INI_RAW_CONCENTRATE_DIFFUSION, 6);

            resource_min = 4;
            resource_max = 16;
        } break;

        case 1: {
            ini_set_setting(INI_RAW_NORMAL_LOW, 0);
            ini_set_setting(INI_RAW_NORMAL_HIGH, 5);
            ini_set_setting(INI_RAW_CONCENTRATE_LOW, 13);
            ini_set_setting(INI_RAW_CONCENTRATE_HIGH, 16);
            ini_set_setting(INI_RAW_CONCENTRATE_SEPERATION, 23);
            ini_set_setting(INI_RAW_CONCENTRATE_DIFFUSION, 5);

            resource_min = 6;
            resource_max = 17;
        } break;

        case 2: {
            ini_set_setting(INI_RAW_NORMAL_LOW, 1);
            ini_set_setting(INI_RAW_NORMAL_HIGH, 5);
            ini_set_setting(INI_RAW_CONCENTRATE_LOW, 16);
            ini_set_setting(INI_RAW_CONCENTRATE_HIGH, 16);
            ini_set_setting(INI_RAW_CONCENTRATE_SEPERATION, 19);
            ini_set_setting(INI_RAW_CONCENTRATE_DIFFUSION, 4);

            resource_min = 7;
            resource_max = 18;
        } break;
    }

    switch (resource_fuel) {
        case 0: {
            ini_set_setting(INI_FUEL_NORMAL_LOW, 1);
            ini_set_setting(INI_FUEL_NORMAL_HIGH, 2);
            ini_set_setting(INI_FUEL_CONCENTRATE_LOW, 8);
            ini_set_setting(INI_FUEL_CONCENTRATE_HIGH, 12);
            ini_set_setting(INI_FUEL_CONCENTRATE_SEPERATION, 29);
            ini_set_setting(INI_FUEL_CONCENTRATE_DIFFUSION, 6);

            resource_min += 2;
            resource_max += 1;
        } break;

        case 1: {
            ini_set_setting(INI_FUEL_NORMAL_LOW, 2);
            ini_set_setting(INI_FUEL_NORMAL_HIGH, 3);
            ini_set_setting(INI_FUEL_CONCENTRATE_LOW, 12);
            ini_set_setting(INI_FUEL_CONCENTRATE_HIGH, 16);
            ini_set_setting(INI_FUEL_CONCENTRATE_SEPERATION, 24);
            ini_set_setting(INI_FUEL_CONCENTRATE_DIFFUSION, 5);

            resource_min += 3;
            resource_max += 2;
        } break;

        case 2: {
            ini_set_setting(INI_FUEL_NORMAL_LOW, 2);
            ini_set_setting(INI_FUEL_NORMAL_HIGH, 4);
            ini_set_setting(INI_FUEL_CONCENTRATE_LOW, 16);
            ini_set_setting(INI_FUEL_CONCENTRATE_HIGH, 16);
            ini_set_setting(INI_FUEL_CONCENTRATE_SEPERATION, 20);
            ini_set_setting(INI_FUEL_CONCENTRATE_DIFFUSION, 4);

            resource_min += 4;
            resource_max += 3;
        } break;
    }

    switch (resource_raw + resource_fuel) {
        case 0: {
            ini_set_setting(INI_MIXED_RESOURCE_SEPERATION, 49);
        } break;

        case 1: {
            ini_set_setting(INI_MIXED_RESOURCE_SEPERATION, 44);
        } break;

        case 2: {
            ini_set_setting(INI_MIXED_RESOURCE_SEPERATION, 40);
        } break;

        case 3: {
            ini_set_setting(INI_MIXED_RESOURCE_SEPERATION, 36);
        } break;

        case 4: {
            ini_set_setting(INI_MIXED_RESOURCE_SEPERATION, 33);
        } break;
    }

    switch (resource_gold) {
        case 0: {
            ini_set_setting(INI_GOLD_NORMAL_LOW, 0);
            ini_set_setting(INI_GOLD_NORMAL_HIGH, 0);
            ini_set_setting(INI_GOLD_CONCENTRATE_LOW, 5);
            ini_set_setting(INI_GOLD_CONCENTRATE_HIGH, 9);
            ini_set_setting(INI_GOLD_CONCENTRATE_SEPERATION, 39);
            ini_set_setting(INI_GOLD_CONCENTRATE_DIFFUSION, 6);

            resource_min += 0;
            resource_max += 0;
        } break;

        case 1: {
            ini_set_setting(INI_GOLD_NORMAL_LOW, 0);
            ini_set_setting(INI_GOLD_NORMAL_HIGH, 0);
            ini_set_setting(INI_GOLD_CONCENTRATE_LOW, 8);
            ini_set_setting(INI_GOLD_CONCENTRATE_HIGH, 12);
            ini_set_setting(INI_GOLD_CONCENTRATE_SEPERATION, 32);
            ini_set_setting(INI_GOLD_CONCENTRATE_DIFFUSION, 5);

            resource_min += 0;
            resource_max += 1;
        } break;

        case 2: {
            ini_set_setting(INI_GOLD_NORMAL_LOW, 0);
            ini_set_setting(INI_GOLD_NORMAL_HIGH, 1);
            ini_set_setting(INI_GOLD_CONCENTRATE_LOW, 12);
            ini_set_setting(INI_GOLD_CONCENTRATE_HIGH, 16);
            ini_set_setting(INI_GOLD_CONCENTRATE_SEPERATION, 26);
            ini_set_setting(INI_GOLD_CONCENTRATE_DIFFUSION, 4);

            resource_min += 1;
            resource_max += 2;
        } break;
    }

    ini_set_setting(INI_MIN_RESOURCES, resource_min);
    ini_set_setting(INI_MAX_RESOURCES, resource_max);

    switch (resource_derelicts) {
        case 0: {
            ini_set_setting(INI_ALIEN_SEPERATION, 0);
            ini_set_setting(INI_ALIEN_UNIT_VALUE, 0);
        } break;

        case 1: {
            ini_set_setting(INI_ALIEN_SEPERATION, 65);
            ini_set_setting(INI_ALIEN_UNIT_VALUE, 20);
        } break;

        case 2: {
            ini_set_setting(INI_ALIEN_SEPERATION, 50);
            ini_set_setting(INI_ALIEN_UNIT_VALUE, 30);
        } break;
    }
}

void draw_copyright_label(WindowInfo* window) {
    Rect bounds;

    bounds.ulx = 10;
    bounds.uly = 469;
    bounds.lrx = 630;
    bounds.lry = 479;

    text_font(5);
    Text_TextBox(window->buffer, window->width,
                 "Copyright 1996 Interplay Productions. v1.04"
                 "  (M.A.X. Port " GAME_VERSION ")",
                 bounds.ulx, bounds.uly, bounds.lrx - bounds.ulx, bounds.lry - bounds.uly, 0, true, false);
}

void menu_draw_main_menu_buttons(MenuButton* button_items, int button_count, int special_button_id = 0) {
    WindowInfo* window;

    window = gwin_get_window(GWINDOW_MAIN_WINDOW);

    menu_button_items = button_items;
    menu_button_items_count = button_count;

    text_font(1);

    for (int i = 0; i < button_count; ++i) {
        ResourceID up;
        ResourceID down;

        if (button_items[i].big_size) {
            up = BLANK_UP;
            down = BLANK_DN;
        } else {
            up = SBLNK_UP;
            down = SBLNK_DN;
        }

        if (button_items[i].label) {
            button_items[i].button = new (std::nothrow) Button(up, down, button_items[i].ulx, button_items[i].uly);
            button_items[i].button->SetCaption(button_items[i].label);
        } else {
            button_items[i].button = new (std::nothrow) Button(button_items[i].ulx, button_items[i].uly, 300, 240);
        }

        button_items[i].button->SetRValue(button_items[i].r_value);
        button_items[i].button->SetPValue(GNW_INPUT_PRESS + i);
        button_items[i].button->SetSfx(button_items[i].sfx);

        if (special_button_id) {
            if (special_button_id == i) {
                button_items[i].button->SetFlags(1);
                button_items[i].button->SetPValue(button_items[i].r_value);
            }
        }

        button_items[i].button->RegisterButton(window->id);
        button_items[i].button->Enable();
    }

    win_draw(window->id);
    mouse_show();
}

void menu_draw_tips_frame(WindowInfo* window) {
    if (menu_tips_strings) {
        Rect bounds;
        char* buffer_position;
        int row_index_limit;

        menu_draw_menu_portrait_frame(window);

        bounds.ulx = 19;
        bounds.uly = 185;
        bounds.lrx = 315;
        bounds.lry = 421;

        buffer_position = window->buffer[window->width * bounds.uly + bounds.ulx];
        row_index_limit = menu_tips_current_row_index + menu_tips_max_row_count_per_page;

        if (row_index_limit > menu_tips_row_count) {
            row_index_limit = menu_tips_row_count;
        }

        text_font(5);

        for (int i = menu_tips_current_row_index; i < row_index_limit; ++i) {
            text_to_buf(&buffer_position[(i - menu_tips_current_row_index) * text_height() * window->width],
                        menu_tips_strings[i].GetCStr(), 296, window->width, 162);
        }

        win_draw_rect(window->id, &bounds);
    }
}

void menu_draw_tips(WindowInfo* window) {
    text_font(5);
    menu_tips_data = ResourceManager_LoadResource(TIPS);

    menu_tips_max_row_count_per_page = 236 / text_height();

    menu_tips_strings =
        Text_SplitText(menu_tips_data, menu_tips_max_row_count_per_page * 100, 296, &menu_tips_row_count);

    menu_tips_current_row_index = 0;

    menu_tips_button_up = new (std::nothrow) Button(MNUUAROU, MNUUAROD, 16, 438);
    menu_tips_button_up->SetRValue(1000);
    menu_tips_button_up->SetPValue(GNW_INPUT_PRESS + 1000);
    menu_tips_button_up->SetSfx(KCARG0);
    menu_tips_button_up->RegisterButton(window->id);

    menu_tips_button_down = new (std::nothrow) Button(MNUDAROU, MNUDAROD, 48, 438);
    menu_tips_button_down->SetRValue(1001);
    menu_tips_button_down->SetPValue(GNW_INPUT_PRESS + 1001);
    menu_tips_button_down->RegisterButton(window->id);

    menu_draw_tips_frame(window);
    win_draw(window->id);
}

void menu_delete_tips() {
    delete menu_tips_data;
    menu_tips_data = nullptr;

    delete[] menu_tips_strings;
    menu_tips_strings = nullptr;

    delete menu_tips_button_up;
    menu_tips_button_up = nullptr;

    delete menu_tips_button_down;
    menu_tips_button_down = nullptr;
}

void tips_on_click_up(WindowInfo* window) {
    if (menu_tips_current_row_index) {
        int page_offset;
        unsigned int time_stamp;

        page_offset = menu_tips_current_row_index - menu_tips_max_row_count_per_page;

        if (page_offset < 0) {
            page_offset = 0;
        }

        do {
            time_stamp = timer_get_stamp32();
            --menu_tips_current_row_index;
            menu_draw_tips_frame(window);

            while ((timer_get_stamp32() - time_stamp) < 12428) {
            }
        } while (menu_tips_current_row_index != page_offset);
    }
}

void tips_on_click_down(WindowInfo* window) {
    int page_offset;
    unsigned int time_stamp;

    page_offset = menu_tips_current_row_index + menu_tips_max_row_count_per_page;

    if (page_offset < menu_tips_row_count) {
        do {
            time_stamp = timer_get_stamp32();
            ++menu_tips_current_row_index;
            menu_draw_tips_frame(window);

            while ((timer_get_stamp32() - time_stamp) < 12428) {
            }
        } while (menu_tips_current_row_index != page_offset);
    }
}

void menu_delete_menu_buttons() {
    if (menu_button_items) {
        for (int i = 0; i < menu_button_items_count; ++i) {
            if (menu_button_items[i].button) {
                delete menu_button_items[i].button;
            }
        }

        menu_button_items_count = -1;
        menu_button_items = nullptr;
        menu_delete_tips();
    }
}

int play_attract_demo(int save_slot) {
    char filename[20];
    FILE* fp;
    int result;

    sprintf(filename, "save%i.%s", save_slot, save_file_extensions[0]);
    fp = fopen(filename, "rb");

    if (fp) {
        int backup_opponent;
        int backup_timer;
        int backup_endturn;
        int backup_play_mode;
        int backup_start_gold;
        int backup_raw_resource;
        int backup_fuel_resource;
        int backup_gold_resource;
        int backup_alien_derelicts;

        char backup_player_name[30];
        char backup_red_team_name[30];
        char backup_green_team_name[30];
        char backup_blue_team_name[30];
        char backup_gray_team_name[30];

        fclose(fp);

        backup_opponent = ini_get_setting(INI_OPPONENT);
        backup_timer = ini_get_setting(INI_TIMER);
        backup_endturn = ini_get_setting(INI_ENDTURN);
        backup_play_mode = ini_get_setting(INI_PLAY_MODE);
        backup_start_gold = ini_get_setting(INI_START_GOLD);
        backup_raw_resource = ini_get_setting(INI_RAW_RESOURCE);
        backup_fuel_resource = ini_get_setting(INI_FUEL_RESOURCE);
        backup_gold_resource = ini_get_setting(INI_GOLD_RESOURCE);
        backup_alien_derelicts = ini_get_setting(INI_ALIEN_DERELICTS);

        ini_config.GetStringValue(INI_PLAYER_NAME, backup_player_name, 30);
        ini_config.GetStringValue(INI_RED_TEAM_NAME, backup_red_team_name, 30);
        ini_config.GetStringValue(INI_GREEN_TEAM_NAME, backup_green_team_name, 30);
        ini_config.GetStringValue(INI_BLUE_TEAM_NAME, backup_blue_team_name, 30);
        ini_config.GetStringValue(INI_GRAY_TEAM_NAME, backup_gray_team_name, 30);

        ini_set_setting(INI_GAME_FILE_NUMBER, save_slot);
        ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_DEMO);

        GameManager_GameLoop(GAME_STATE_10);

        ini_set_setting(INI_OPPONENT, backup_opponent);
        ini_set_setting(INI_TIMER, backup_timer);
        ini_set_setting(INI_ENDTURN, backup_endturn);
        ini_set_setting(INI_PLAY_MODE, backup_play_mode);
        ini_set_setting(INI_START_GOLD, backup_start_gold);
        ini_set_setting(INI_RAW_RESOURCE, backup_raw_resource);
        ini_set_setting(INI_FUEL_RESOURCE, backup_fuel_resource);
        ini_set_setting(INI_GOLD_RESOURCE, backup_gold_resource);
        ini_set_setting(INI_ALIEN_DERELICTS, backup_alien_derelicts);

        ini_config.SetStringValue(INI_PLAYER_NAME, backup_player_name);
        ini_config.SetStringValue(INI_RED_TEAM_NAME, backup_red_team_name);
        ini_config.SetStringValue(INI_GREEN_TEAM_NAME, backup_green_team_name);
        ini_config.SetStringValue(INI_BLUE_TEAM_NAME, backup_blue_team_name);
        ini_config.SetStringValue(INI_GRAY_TEAM_NAME, backup_gray_team_name);

        result = 1;

    } else {
        result = 0;
    }

    return result;
}

void menu_disable_menu_buttons() {
    if (menu_button_items) {
        for (int i = 0; i < menu_button_items_count; ++i) {
            menu_button_items[i].button->Disable();
        }
    }
}

void menu_enable_menu_buttons() {
    if (menu_button_items) {
        for (int i = 0; i < menu_button_items_count; ++i) {
            menu_button_items[i].button->Enable();
        }
    }
}

void menu_credits_menu_loop() {
    WindowInfo* window;
    Image* image1;
    Image* image2;
    Rect bounds;
    int window_width;
    int window_height;
    bool play;
    int height_offset;
    int line_index;
    char* image_buffer_position;
    char* window_buffer_position;
    unsigned int time_stamp;

    window = gwin_get_window(GWINDOW_MAIN_WINDOW);
    Cursor_SetCursor(CURSOR_HIDDEN);
    gwin_load_image(CREDITS, window, 640, true, false);
    win_draw(window->id);

    window_width = 450;
    window_height = 230;

    bounds.ulx = (640 - window_width) / 2;
    bounds.uly = (480 - window_height) / 2;
    bounds.lrx = bounds.ulx + window_width;
    bounds.lry = bounds.uly + window_height;

    image1 = new (std::nothrow) Image(bounds.ulx, bounds.uly, bounds.lrx, bounds.lry);
    image2 = new (std::nothrow) Image(bounds.ulx, bounds.uly, window_width, window_height + 18);

    image1->Copy(window);
    memset(image2->GetData(), 0x80, (window_height + 18) * window_width);

    text_font(1);

    play = true;
    line_index = 0;
    height_offset = bounds.lry;

    while (play) {
        image1->Write(window);
        memmove(image2->GetData(), &image2->GetData()[window_width], (window_height + 17) * window_width);
        memset(&image2->GetData()[(window_height + 17) * window_width], 0x80, window_width);

        if (line_index < (sizeof(menu_credits_lines) / sizeof(struct CreditsLine)) && height_offset <= bounds.lry) {
            Text_TextBox(image2->GetData(), window_width, menu_credits_lines[line_index].text, 0, window_height,
                         window_width, 0x12, menu_credits_lines[line_index].color | 0x10000, true, true);
            ++line_index;
            height_offset = bounds.lry + 20;
        }

        image_buffer_position = image2->GetData();
        window_buffer_position = &window->buffer[window->width * bounds.uly + bounds.ulx];

        for (int i = 0; i < window_height; ++i) {
            for (int j = 0; j < window_width; ++j) {
                if (image_buffer_position[j] != 0x80) {
                    window_buffer_position[j] = image_buffer_position[j];
                }
            }

            image_buffer_position += window_width;
            window_buffer_position += window->width;
        }

        win_draw_rect(window->id, &bounds);

        time_stamp = timer_get_stamp32();
        while (timer_elapsed_time_ms(time_stamp) < 10) {
            if (get_input() != -1) {
                play = 0;
            }
        }

        if (height_offset <= bounds.uly) {
            play = 0;
        }
    }

    delete image1;
    delete image2;
}

int menu_clan_select_menu_loop(int team) {
    ClanSelectMenu clan_select_menu;
    int result;
    bool event_release;

    event_release = false;
    clan_select_menu.Init(team);

    do {
        if (Remote_GameState) {
            Remote_sub_CAC94();
        }

        if (Remote_GameState == 2) {
            break;
        }

        clan_select_menu.key = get_input();

        if (clan_select_menu.key > 0 && clan_select_menu.key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        for (int i = 0; i < CLAN_SELECT_MENU_ITEM_COUNT; ++i) {
            if (clan_select_menu.buttons[i]) {
                if (clan_select_menu.key == GNW_INPUT_PRESS + i) {
                    if (!event_release) {
                        clan_select_menu.buttons[i]->PlaySound();
                    }

                    event_release = true;
                    break;
                }

                if (clan_select_menu.key == clan_select_menu.menu_item[i].event_code) {
                    clan_select_menu.key = clan_select_menu.menu_item[i].r_value;
                }

                if (clan_select_menu.key == clan_select_menu.menu_item[i].r_value) {
                    if (i < 8) {
                        clan_select_menu.buttons[i]->PlaySound();
                    }

                    clan_select_menu.key -= 1000;
                    clan_select_menu.menu_item[i].event_handler();
                    break;
                }
            }
        }

    } while (!clan_select_menu.event_click_done_cancel_random);

    clan_select_menu.Deinit();

    UnitsManager_TeamInfo[team].team_clan = clan_select_menu.team_clan;
    result = clan_select_menu.team_clan;

    return result;
}

int menu_planet_select_menu_loop() {
    PlanetSelectMenu planet_select_menu;
    int result;
    bool event_release;

    event_release = false;
    planet_select_menu.Init();

    do {
        if (Remote_GameState == 1) {
            Remote_sub_CAC94();
        }

        planet_select_menu.key = get_input();

        if (planet_select_menu.key > 0 && planet_select_menu.key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        for (int i = 0; i < PLANET_SELECT_MENU_ITEM_COUNT; ++i) {
            if (planet_select_menu.buttons[i]) {
                if (planet_select_menu.key == GNW_INPUT_PRESS + i) {
                    if (!event_release) {
                        planet_select_menu.buttons[i]->PlaySound();
                    }

                    event_release = true;
                    break;
                }

                if (planet_select_menu.key == planet_select_menu.menu_item[i].event_code) {
                    planet_select_menu.key = planet_select_menu.menu_item[i].r_value;
                }

                if (planet_select_menu.key == planet_select_menu.menu_item[i].r_value) {
                    planet_select_menu.key -= 1000;
                    planet_select_menu.menu_item[i].event_handler();
                    break;
                }
            }
        }
    } while (!planet_select_menu.event_click_done && !planet_select_menu.event_click_cancel);

    planet_select_menu.Deinit();
    if (!planet_select_menu.event_click_cancel) {
        ini_set_setting(INI_WORLD, planet_select_menu.world);

        result = 1;
    } else {
        result = 0;
    }

    return result;
}

int menu_options_menu_loop(int game_mode) {
    GameConfigMenu game_config_menu;
    int result;
    bool event_release;

    event_release = false;
    game_config_menu.game_mode = game_mode;
    game_config_menu.Init();

    do {
        if (Remote_GameState == 1) {
            Remote_sub_CAC94();
        }

        game_config_menu.key = get_input();

        if (game_config_menu.key > 0 && game_config_menu.key < GNW_INPUT_PRESS) {
            event_release = false;
        }

        if (game_config_menu.text_edit) {
            if (game_config_menu.text_edit->ProcessKeyPress(game_config_menu.key)) {
                if (game_config_menu.key == GNW_KB_KEY_ESCAPE || game_config_menu.key == GNW_KB_KEY_RETURN) {
                    game_config_menu.UpdateTextEdit(game_config_menu.key == GNW_KB_KEY_RETURN);
                }

                game_config_menu.key = 0;
                continue;
            }

            if (game_config_menu.key > 0) {
                game_config_menu.text_edit->ProcessKeyPress(GNW_KB_KEY_RETURN);
                game_config_menu.UpdateTextEdit(1);
            }
        }

        for (int i = 0; i < GAME_CONFIG_MENU_ITEM_COUNT; ++i) {
            if (game_config_menu.buttons[i]) {
                if (game_config_menu.key == game_config_menu.menu_item[i].event_code) {
                    game_config_menu.key = game_config_menu.menu_item[i].r_value;
                }

                if (game_config_menu.key == game_config_menu.menu_item[i].r_value) {
                    if (!event_release) {
                        game_config_menu.buttons[i]->PlaySound();
                    }

                    event_release = true;
                    game_config_menu.key -= 1000;
                    game_config_menu.menu_item[i].event_handler();
                    break;
                }
            }
        }

    } while (!game_config_menu.event_click_cancel && !game_config_menu.event_click_done);

    game_config_menu.Deinit();
    result = game_config_menu.event_click_done;

    return result;
}

int menu_choose_player_menu_loop(bool game_type) {
    ChoosePlayerMenu choose_player_menu;
    bool event_release;

    if (game_type) {
        ini_set_setting(INI_RED_TEAM_PLAYER, TEAM_TYPE_PLAYER);
        ini_set_setting(INI_GREEN_TEAM_PLAYER, TEAM_TYPE_COMPUTER);
    } else {
        ini_set_setting(INI_RED_TEAM_PLAYER, TEAM_TYPE_PLAYER);
        ini_set_setting(INI_GREEN_TEAM_PLAYER, TEAM_TYPE_PLAYER);
    }

    ini_set_setting(INI_BLUE_TEAM_PLAYER, TEAM_TYPE_NONE);
    ini_set_setting(INI_GRAY_TEAM_PLAYER, TEAM_TYPE_NONE);

    event_release = false;

    choose_player_menu.game_type = game_type;
    choose_player_menu.Init();
    choose_player_menu.event_click_done = false;

    do {
        choose_player_menu.key_press = get_input();

        if (choose_player_menu.key_press > 0 && choose_player_menu.key_press < GNW_INPUT_PRESS) {
            event_release = false;
        }

        for (int i = 0; i < CHOOSE_PLAYER_MENU_ITEM_COUNT; ++i) {
            if (choose_player_menu.buttons[i]) {
                if (choose_player_menu.key_press == i + GNW_INPUT_PRESS) {
                    if (!event_release) {
                        choose_player_menu.buttons[i]->PlaySound();
                    }

                    event_release = true;
                    break;
                }

                if (choose_player_menu.key_press == choose_player_menu.menu_item[i].event_code) {
                    choose_player_menu.key_press = choose_player_menu.menu_item[i].r_value;
                }

                if (choose_player_menu.key_press == choose_player_menu.menu_item[i].r_value) {
                    if (i < 12) {
                        choose_player_menu.buttons[i]->PlaySound();
                    }

                    choose_player_menu.key_press -= 1000;
                    choose_player_menu.menu_item[i].event_handler();
                    break;
                }
            }
        }

        if (choose_player_menu.event_click_cancel) {
            choose_player_menu.Deinit();

            return false;
        }

    } while (!choose_player_menu.event_click_done);

    choose_player_menu.Deinit();

    for (int i = 0; i < PLAYER_TEAM_ALIEN; ++i) {
        int team_type;
        char buffer[30];

        team_type = ini_get_setting(INI_RED_TEAM_PLAYER + i);

        if (game_type && team_type == TEAM_TYPE_PLAYER) {
            ini_config.GetStringValue(INI_PLAYER_NAME, buffer, sizeof(buffer));
            ini_config.SetStringValue(INI_RED_TEAM_NAME + i, buffer);
            game_type = 0;
        } else {
            ini_config.SetStringValue(INI_RED_TEAM_NAME + i, menu_team_names[i]);
        }
    }

    return true;
}

int GameSetupMenu_menu_loop(int game_file_type, bool flag1, bool flag2) {
    GameSetupMenu game_setup_menu;
    SaveFormatHeader save_file_header;
    bool palette_from_image;
    bool event_release;
    int result;

    palette_from_image = false;
    game_setup_menu.game_file_type = game_file_type;

    if (game_file_type == GAME_TYPE_CAMPAIGN) {
        ini_set_setting(INI_GAME_FILE_NUMBER, ini_get_setting(INI_LAST_CAMPAIGN));
    } else {
        ini_set_setting(INI_GAME_FILE_NUMBER, 1);
    }

    for (;;) {
        event_release = false;
        game_setup_menu.Init(palette_from_image);

        do {
            if (Remote_GameState == 1) {
                Remote_sub_CAC94();
            }

            game_setup_menu.key = get_input();

            if (game_setup_menu.key > 0 && game_setup_menu.key < GNW_INPUT_PRESS) {
                event_release = false;
            }

            switch (key) {
                case GNW_KB_KEY_PAGEUP: {
                    game_setup_menu.key = 12;
                    game_setup_menu.EventScrollButton();
                } break;

                case GNW_KB_KEY_PAGEDOWN: {
                    game_setup_menu.key = 13;
                    game_setup_menu.EventScrollButton();
                } break;

                case GNW_KB_KEY_UP:
                case GNW_KB_KEY_DOWN: {
                    game_setup_menu.EventStepButton();
                } break;

                default: {
                    for (int i = 0; i < GAME_CONFIG_MENU_ITEM_COUNT; ++i) {
                        if (game_setup_menu.buttons[i]) {
                            if (game_setup_menu.key == GNW_INPUT_PRESS + i) {
                                if (!event_release) {
                                    game_setup_menu.buttons[i]->PlaySound();
                                }

                                event_release = true;
                                break;
                            }

                            if (game_setup_menu.key == game_setup_menu.menu_item[i].event_code) {
                                game_setup_menu.key = game_setup_menu.menu_item[i].r_value;
                            }

                            if (game_setup_menu.key == game_setup_menu.menu_item[i].r_value) {
                                game_setup_menu.key -= 1000;
                                game_setup_menu.menu_item[i].event_handler();
                                break;
                            }
                        }
                    }

                } break;
            }

        } while (!game_setup_menu.event_click_done && !game_setup_menu.event_click_cancel);

        game_setup_menu.Deinit();

        if (game_setup_menu.event_click_cancel) {
            result = false;
            break;
        }

        menu_game_file_number = game_setup_menu.game_file_number;

        ini_set_setting(INI_GAME_FILE_NUMBER, menu_game_file_number);
        ini_set_setting(INI_GAME_FILE_TYPE, game_setup_menu.game_file_type);

        if (game_setup_menu.game_file_type != GAME_TYPE_MULTI_PLAYER_SCENARIO || flag1) {
            if (menu_options_menu_loop(2)) {
                if (game_setup_menu.game_file_type == GAME_TYPE_CAMPAIGN) {
                    menu_draw_campaign_mission_briefing_screen();
                }

                if (menu_game_file_number == 1 && game_setup_menu.game_file_type == GAME_TYPE_CAMPAIGN) {
                    movie_play(DEMO2FLC);
                }

                if (game_setup_menu.game_file_type == GAME_TYPE_MULTI_PLAYER_SCENARIO) {
                    SaveLoadMenu_GetSavedGameInfo(menu_game_file_number, ini_get_setting(INI_GAME_FILE_TYPE),
                                                  save_file_header, false);

                    ini_set_setting(INI_RED_TEAM_CLAN, save_file_header.team_clan[PLAYER_TEAM_RED]);
                    ini_set_setting(INI_GREEN_TEAM_CLAN, save_file_header.team_clan[PLAYER_TEAM_GREEN]);
                    ini_set_setting(INI_BLUE_TEAM_CLAN, save_file_header.team_clan[PLAYER_TEAM_BLUE]);
                    ini_set_setting(INI_GRAY_TEAM_CLAN, save_file_header.team_clan[PLAYER_TEAM_GRAY]);

                    if (menu_choose_player_menu_loop(flag2)) {
                        game_loop(GAME_STATE_10);
                        palette_from_image = true;
                    } else {
                        palette_from_image = false;
                    }

                } else {
                    game_loop(GAME_STATE_10);
                    palette_from_image = true;
                }

            } else {
                palette_from_image = false;
            }

            continue;
        }

        result = true;
        break;
    };

    return result;
}

int menu_custom_game_menu(bool game_type) {
    int result;
    bool enter_options_menu;
    bool enter_planet_select_menu;
    bool player_menu_passed;

    enter_options_menu = true;
    enter_planet_select_menu = true;
    player_menu_passed = false;

    do {
        if (enter_options_menu) {
            if (!menu_options_menu_loop(1)) {
                return 0;
            }

            enter_options_menu = false;
        }

        if (enter_planet_select_menu) {
            if (!menu_planet_select_menu_loop()) {
                enter_options_menu = 1;
                continue;
            }

            enter_planet_select_menu = false;
        }

        for (int i = PLAYER_TEAM_RED; i < PLAYER_TEAM_ALIEN; ++i) {
            ini_set_setting(static_cast<IniParameter>(INI_RED_TEAM_CLAN + i), ini_get_setting(INI_PLAYER_CLAN));
        }

        player_menu_passed = menu_choose_player_menu_loop(game_type);

        if (!player_menu_passed) {
            enter_planet_select_menu = true;
        }

    } while (!player_menu_passed);

    mouse_hide();
    gwin_clear_window();
    mouse_show();

    GameManager_GameLoop(GAME_STATE_6);

    return 1;
}

int menu_new_game_menu_loop() {
    WindowInfo* window;
    int key;
    bool exit_loop;
    bool event_release;
    bool is_tips_window_active;

    window = gwin_get_window(GWINDOW_MAIN_WINDOW);
    key = 0;

    while (key != GNW_KB_KEY_CTRL_ESCAPE && key != 9000) {
        event_release = false;

        mouse_hide();
        gwin_load_image(MAINPIC, window, 640, false);
        draw_menu_title(window, "New Game Menu");
        menu_draw_menu_portrait(window, menu_portrait_id, false);
        menu_draw_main_menu_buttons(new_game_menu_buttons, sizeof(new_game_menu_buttons) / sizeof(MenuButton), 5);
        mouse_show();

        exit_loop = false;
        is_tips_window_active = false;

        while (!exit_loop) {
            key = toupper(get_input());

            if (key > 0 && key < GNW_INPUT_PRESS) {
                event_release = false;
            }

            switch (key) {
                case GNW_KB_KEY_SHIFT_T: {
                    menu_delete_menu_buttons();

                    if (GameSetupMenu_menu_loop(GAME_TYPE_TRAINING)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_SHIFT_S: {
                    menu_delete_menu_buttons();

                    if (GameSetupMenu_menu_loop(GAME_TYPE_SCENARIO)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_SHIFT_U: {
                    menu_delete_menu_buttons();
                    ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_CUSTOM);
                    if (menu_custom_game_menu(true)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_SHIFT_M: {
                    menu_delete_menu_buttons();
                    ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_CUSTOM);
                    if (GameSetupMenu_menu_loop(GAME_TYPE_MULTI_PLAYER_SCENARIO)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_SHIFT_A: {
                    menu_delete_menu_buttons();
                    if (GameSetupMenu_menu_loop(GAME_TYPE_CAMPAIGN)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_SHIFT_I: {
                    is_tips_window_active = !is_tips_window_active;

                    SDL_assert(menu_button_items_count >= 5);

                    menu_button_items[5].button->PlaySound();

                    if (is_tips_window_active) {
                        menu_draw_tips(window);
                    } else {
                        menu_delete_menu_buttons();
                        exit_loop = true;
                    }

                } break;

                case GNW_KB_KEY_SHIFT_C:
                case GNW_KB_KEY_ESCAPE: {
                    menu_delete_menu_buttons();
                    key = GNW_KB_KEY_ESCAPE;
                    exit_loop = true;
                } break;

                case GNW_KB_KEY_UP: {
                    if (!is_tips_window_active) {
                        menu_draw_menu_portrait(window, INVALID_ID, true);
                    }
                } break;

                case 1000: {
                    tips_on_click_up(window);
                } break;

                case 1001: {
                    tips_on_click_down(window);
                } break;

                default: {
                    if (key >= GNW_INPUT_PRESS && !event_release) {
                        key -= GNW_INPUT_PRESS;

                        if (key == 1000 || key == 1001) {
                            menu_tips_button_up->PlaySound();
                        } else if (key < menu_button_items_count) {
                            menu_button_items[key].button->PlaySound();
                        }

                        event_release = true;
                    }

                } break;
            }
        }
    }

    return key == 9000;
}

void menu_preferences_window(unsigned short team) {
    OptionsMenu* options_menu = new (std::nothrow) OptionsMenu(team, PREFSPIC);

    options_menu->Run();
}

void menu_setup_window() {
    OptionsMenu* options_menu = new (std::nothrow) OptionsMenu(PLAYER_TEAM_RED, SETUPPIC);

    options_menu->Run();
}

int menu_network_game_menu(bool is_host_mode) {
    if (Remote_Lobby(is_host_mode)) {
        if (is_host_mode) {
            menu_update_resource_levels();
            Remote_SendNetPacket_17();
        }

        GameManager_GameLoop(GUI_GameState);
    }

    Remote_Deinit();

    return false;
}

int menu_multiplayer_menu_loop() {
    WindowInfo* window;
    short palette_from_image;
    int key;
    bool exit_loop;
    bool event_release;

    window = gwin_get_window(GWINDOW_MAIN_WINDOW);
    palette_from_image = 0;
    key = 0;

    while (key != GNW_KB_KEY_CTRL_ESCAPE && key != 9000) {
        event_release = false;

        mouse_hide();
        gwin_load_image(MAINPIC, window, 640, palette_from_image, false);
        draw_menu_title(window, "Multiplayer Menu");
        menu_draw_menu_portrait(window, menu_portrait_id, false);
        menu_draw_main_menu_buttons(network_game_menu_buttons,
                                    sizeof(network_game_menu_buttons) / sizeof(struct MenuButton));
        ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_MULTI);
        palette_from_image = 0;
        mouse_show();

        exit_loop = false;

        while (!exit_loop) {
            key = toupper(get_input());

            if (key > 0 && key < GNW_INPUT_PRESS) {
                event_release = false;
            }

            switch (key) {
                case GNW_KB_KEY_SHIFT_L: {
                    int game_file_number;
                    menu_delete_menu_buttons();
                    ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_HOT_SEAT);
                    game_file_number = SaveLoadMenu_menu_loop(false, false);

                    if (game_file_number) {
                        ini_set_setting(INI_GAME_FILE_NUMBER, game_file_number);
                        GameManager_GameLoop(GAME_STATE_10);

                        key = 9000;
                    }

                    exit_loop = true;
                    palette_from_image = true;

                } break;

                case GNW_KB_KEY_SHIFT_T: {
                    menu_delete_menu_buttons();

                    if (GameSetupMenu_menu_loop(GAME_TYPE_MULTI_PLAYER_SCENARIO, true, false)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_UP: {
                    menu_draw_menu_portrait(window, INVALID_ID, true);
                } break;

                case GNW_KB_KEY_SHIFT_O: {
                    menu_delete_menu_buttons();
                    ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_HOT_SEAT);

                    if (menu_custom_game_menu(false)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_SHIFT_H: {
                    menu_delete_menu_buttons();

                    if (menu_network_game_menu(true)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_SHIFT_J: {
                    menu_delete_menu_buttons();

                    if (menu_network_game_menu(false)) {
                        key = 9000;
                    }

                    exit_loop = true;

                } break;

                case GNW_KB_KEY_SHIFT_C:
                case GNW_KB_KEY_ESCAPE: {
                    menu_delete_menu_buttons();
                    key = GNW_KB_KEY_ESCAPE;
                    exit_loop = true;
                } break;

                default: {
                    if (key >= GNW_INPUT_PRESS) {
                        key -= GNW_INPUT_PRESS;

                        if (!event_release && key < menu_button_items_count) {
                            menu_button_items[key].button->PlaySound();
                        }

                        event_release = true;
                    }

                } break;
            }
        }

        Remote_Deinit();
    }

    return key == 9000;
}

void main_menu() {
    WindowInfo* window;
    unsigned int time_stamp;
    short palette_from_image;
    bool exit_loop;
    bool event_release;
    int save_slot;
    int old_save_slot;

    window = gwin_get_window(GWINDOW_MAIN_WINDOW);
    palette_from_image = 1;
    save_slot = 1;
    mouse_set_position(320, 240);
    menu_portrait_id = INVALID_ID;
    dos_srand(time(nullptr));
    ini_setting_victory_type = ini_get_setting(INI_VICTORY_TYPE);
    ini_setting_victory_limit = ini_get_setting(INI_VICTORY_LIMIT);

    for (;;) {
        event_release = false;
        time_stamp = timer_get_stamp32();
        GUI_GameState = GAME_STATE_3_MAIN_MENU;
        mouse_hide();
        gwin_load_image(MAINPIC, window, 640, palette_from_image, false);
        draw_menu_title(window, "Main Menu");
        menu_draw_menu_portrait(window, menu_portrait_id, false);
        draw_copyright_label(window);
        menu_draw_main_menu_buttons(main_menu_buttons, sizeof(main_menu_buttons) / sizeof(struct MenuButton));
        mouse_show();
        soundmgr.PlayMusic(MAIN_MSC, false);
        Cursor_SetCursor(CURSOR_HAND);
        mouse_show();
        palette_from_image = 0;
        ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_CUSTOM);

        exit_loop = false;

        while (!exit_loop) {
            if (timer_elapsed_time_ms(time_stamp) > 60000) {
                old_save_slot = save_slot;
                menu_delete_menu_buttons();

                do {
                    palette_from_image = play_attract_demo(save_slot);
                    ++save_slot;
                    if (save_slot == 10) {
                        save_slot = 1;
                    }
                } while (!palette_from_image && old_save_slot != save_slot);
                break;
            } else {
                int key;
                int mouse_x;
                int mouse_y;
                int last_mouse_x;
                int last_mouse_y;

                key = get_input();
                mouse_get_position(&mouse_x, &mouse_y);

                if (key != -1 || mouse_x != 0 || mouse_y != 0) {
                    time_stamp = timer_get_stamp32();
                    last_mouse_x = mouse_x;
                    last_mouse_y = mouse_y;
                }

                key = toupper(key);

                if (key > 0 && key < GNW_INPUT_PRESS) {
                    event_release = false;
                }

                switch (key) {
                    case GNW_KB_KEY_SHIFT_L: {
                        int game_file_number;

                        menu_delete_menu_buttons();
                        game_file_number = SaveLoadMenu_menu_loop(false, false);

                        if (game_file_number) {
                            ini_set_setting(INI_GAME_FILE_NUMBER, game_file_number);
                        }

                        GameManager_GameLoop(GAME_STATE_10);

                        menu_portrait_id = INVALID_ID;
                        palette_from_image = 1;
                        exit_loop = true;

                    } break;

                    case GNW_KB_KEY_CTRL_L: {
                        int game_file_number;

                        menu_delete_menu_buttons();
                        game_file_number = SaveLoadMenu_menu_loop(false, true);

                        if (game_file_number) {
                            ini_set_setting(INI_GAME_FILE_TYPE, GAME_TYPE_TEXT);
                            ini_set_setting(INI_GAME_FILE_NUMBER, game_file_number);
                        }

                        GameManager_GameLoop(GAME_STATE_10);

                        menu_portrait_id = INVALID_ID;
                        palette_from_image = 1;
                        exit_loop = true;
                    } break;

                    case GNW_KB_KEY_SHIFT_N: {
                        menu_delete_menu_buttons();
                        palette_from_image = menu_new_game_menu_loop();

                        if (palette_from_image) {
                            menu_portrait_id = INVALID_ID;
                        }

                        exit_loop = true;

                    } break;

                    case GNW_KB_KEY_SHIFT_S: {
                        menu_disable_menu_buttons();
                        menu_setup_window();
                        menu_enable_menu_buttons();
                        time_stamp = timer_get_stamp32();
                    } break;

                    case GNW_KB_KEY_UP: {
                        menu_draw_menu_portrait(window, INVALID_ID, true);
                    } break;

                    case GNW_KB_KEY_SHIFT_M: {
                        menu_delete_menu_buttons();
                        palette_from_image = menu_multiplayer_menu_loop();

                        if (palette_from_image) {
                            menu_portrait_id = INVALID_ID;
                        }

                        exit_loop = true;

                    } break;

                    case GNW_KB_KEY_SHIFT_E: {
                        menu_delete_menu_buttons();
                        gexit(1);

                    } break;

                    case GNW_KB_KEY_SHIFT_I: {
                        mouse_hide();
                        menu_delete_menu_buttons();

                        movie_play_intro();

                        palette_from_image = 1;
                        exit_loop = true;

                    } break;

                    case GNW_KB_KEY_SHIFT_C: {
                        menu_delete_menu_buttons();

                        menu_credits_menu_loop();

                        palette_from_image = 1;
                        exit_loop = true;

                    } break;

                    default: {
                        if (key >= GNW_INPUT_PRESS) {
                            key -= GNW_INPUT_PRESS;

                            if (!event_release && key < menu_button_items_count) {
                                menu_button_items[key].button->PlaySound();
                            }

                            event_release = true;
                        }
                    } break;
                }
            }
        }
    }
}
