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

#include "planetselectmenu.hpp"

#include "helpmenu.hpp"
#include "inifile.hpp"
#include "menu.hpp"
#include "resource_manager.hpp"
#include "window_manager.hpp"

struct PlanetSelectMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char* label;
    int event_code;
    void (PlanetSelectMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    { {(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx) }

static struct MenuTitleItem planet_select_menu_screen_title = {{230, 6, 410, 26}, "Planet Select Menu"};
static struct MenuTitleItem planet_select_menu_planet_name = {{16, 61, 165, 80}, nullptr};
static struct MenuTitleItem planet_select_menu_planet_description = {{41, 350, 600, 409}, nullptr};

static struct PlanetSelectMenuControlItem planet_select_menu_controls[] = {
    MENU_CONTROL_DEF(192, 51, 303, 162, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
    MENU_CONTROL_DEF(330, 51, 441, 162, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
    MENU_CONTROL_DEF(469, 51, 580, 162, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
    MENU_CONTROL_DEF(192, 189, 303, 300, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
    MENU_CONTROL_DEF(330, 189, 441, 300, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
    MENU_CONTROL_DEF(469, 189, 580, 300, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
    MENU_CONTROL_DEF(60, 248, 0, 0, MNULAROU, nullptr, 0, &PlanetSelectMenu::EventWorld, PSELW0),
    MENU_CONTROL_DEF(92, 248, 0, 0, MNURAROU, nullptr, 0, &PlanetSelectMenu::EventWorld, PSELW0),
    MENU_CONTROL_DEF(243, 438, 0, 0, MNUBTN4U, "Random", 0, &PlanetSelectMenu::EventRandom, PDONE0),
    MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, "Cancel", GNW_KB_KEY_SHIFT_ESCAPE, &PlanetSelectMenu::EventCancel,
                     PCANC0),
    MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, "?", GNW_KB_KEY_SHIFT_DIVIDE, &PlanetSelectMenu::EventHelp, PHELP0),
    MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, "Done", GNW_KB_KEY_SHIFT_RETURN, &PlanetSelectMenu::EventDone, PDONE0),
};

static_assert(PLANET_SELECT_MENU_ITEM_COUNT ==
              sizeof(planet_select_menu_controls) / sizeof(PlanetSelectMenuControlItem));

void PlanetSelectMenu::ButtonInit(int index) {
    struct PlanetSelectMenuControlItem* control = &planet_select_menu_controls[index];

    text_font(GNW_TEXT_FONT_1);

    if (control->image_id == INVALID_ID) {
        buttons[index] = new (std::nothrow)
            Button(control->bounds.ulx, control->bounds.uly, control->bounds.lrx - control->bounds.ulx,
                   control->bounds.lry - control->bounds.uly);
    } else {
        buttons[index] = new (std::nothrow) Button(control->image_id, static_cast<ResourceID>(control->image_id + 1),
                                                   control->bounds.ulx, control->bounds.uly);

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

void PlanetSelectMenu::DrawMaps(int draw_to_screen) {
    int world_first;
    int world_last;
    unsigned char* buffer_position;

    world_first = (world / 6) * 6;
    world_last = world_first + 6;

    menu_item_index = 0;

    for (int i = world_first; i < world_last; ++i) {
        process_bk();

        buffer_position = &window->buffer[planet_select_menu_controls[menu_item_index].bounds.uly * window->width +
                                          planet_select_menu_controls[menu_item_index].bounds.ulx];

        if (Menu_LoadPlanetMinimap(i, buffer_position, window->width)) {
            ++menu_item_index;

            if (draw_to_screen) {
                win_draw_rect(window->id, &planet_select_menu_controls[menu_item_index].bounds);
            }
        }
    }

    WindowManager_LoadImage(static_cast<ResourceID>(SNOW_PIC + world / 6), window, window->width, false, false);

    if (!image3) {
        image3 = new (std::nothrow)
            Image(planet_select_menu_planet_description.bounds.ulx, planet_select_menu_planet_description.bounds.uly,
                  planet_select_menu_planet_description.bounds.lrx - planet_select_menu_planet_description.bounds.ulx,
                  planet_select_menu_planet_description.bounds.lry - planet_select_menu_planet_description.bounds.uly);
        image3->Copy(window);
    }
}

void PlanetSelectMenu::DrawTexts() {
    Rect* bounds;

    text_font(GNW_TEXT_FONT_5);

    menu_draw_menu_title(window, &planet_select_menu_screen_title, COLOR_GREEN, true);

    if (!image1) {
        image1 = new (std::nothrow)
            Image(planet_select_menu_planet_name.bounds.ulx, planet_select_menu_planet_name.bounds.uly,
                  planet_select_menu_planet_name.bounds.lrx - planet_select_menu_planet_name.bounds.ulx + 1,
                  planet_select_menu_planet_name.bounds.lry - planet_select_menu_planet_name.bounds.uly + 1);
        image1->Copy(window);
    }

    image1->Write(window);
    planet_select_menu_planet_name.title = menu_planet_names[world];
    menu_draw_menu_title(window, &planet_select_menu_planet_name, COLOR_GREEN, true);

    image3->Write(window);
    planet_select_menu_planet_description.title = menu_planet_descriptions[world];
    menu_draw_menu_title(window, &planet_select_menu_planet_description, COLOR_GREEN);

    for (int i = 0; i < PLANET_SELECT_MENU_ITEM_COUNT; ++i) {
        buttons[i]->Enable();
    }

    if (image2) {
        image2->Write(window);

        delete image2;
        image2 = nullptr;
    }

    bounds = &planet_select_menu_controls[world % 6].bounds;

    image2 = new (std::nothrow)
        Image(bounds->ulx, bounds->uly, bounds->lrx - bounds->ulx + 1, bounds->lry - bounds->uly + 1);
    image2->Copy(window);

    draw_box(window->buffer, window->width, bounds->ulx, bounds->uly, bounds->lrx, bounds->lry, COLOR_RED);
    draw_box(window->buffer, window->width, bounds->ulx + 1, bounds->uly + 1, bounds->lrx - 1, bounds->lry - 1,
             COLOR_RED);
    win_draw(window->id);
}

void PlanetSelectMenu::AnimateWorldChange(int world1, int world2, bool direction) {
    ResourceID resource;
    unsigned char* world1_image;
    unsigned char* world2_image;
    unsigned char* stars_image;
    unsigned char* window_buffer;

    resource = static_cast<ResourceID>(SNOW_PIC + (world1 / 6));
    world1_image = ResourceManager_ReadResource(resource);

    resource = static_cast<ResourceID>(SNOW_PIC + (world2 / 6));
    world2_image = ResourceManager_ReadResource(resource);

    stars_image = ResourceManager_ReadResource(STAR_PIC);

    if (world1_image && world2_image && stars_image) {
        struct ImageBigHeader* image1_header = reinterpret_cast<struct ImageBigHeader*>(world1_image);
        struct ImageBigHeader* image2_header = reinterpret_cast<struct ImageBigHeader*>(world2_image);
        struct ImageBigHeader* image3_header = reinterpret_cast<struct ImageBigHeader*>(stars_image);
        int width;
        int height;
        Rect bounds;
        unsigned char* buffer_position;
        unsigned int time_stamp;
        int offset;

        width = image1_header->width;
        height = image1_header->height;

        window_buffer = new (std::nothrow) unsigned char[width * height * 3];

        if (direction) {
            WindowManager_DecodeImage(image1_header, window_buffer, false, false, width * 3);
            WindowManager_DecodeImage(image3_header, &window_buffer[width], false, false, width * 3);
            WindowManager_DecodeImage(image2_header, &window_buffer[width * 2], false, false, width * 3);
        } else {
            WindowManager_DecodeImage(image1_header, &window_buffer[width * 2], false, false, width * 3);
            WindowManager_DecodeImage(image3_header, &window_buffer[width], false, false, width * 3);
            WindowManager_DecodeImage(image2_header, window_buffer, false, false, width * 3);
        }

        bounds.ulx = image1_header->ulx;
        bounds.uly = image1_header->uly;
        bounds.lrx = image1_header->ulx + image1_header->width;
        bounds.lry = image1_header->uly + image1_header->height;

        buffer_position = &window->buffer[bounds.uly * window->width + bounds.ulx];

        for (int i = 1; i <= 16; ++i) {
            time_stamp = timer_get_stamp32();

            if (direction) {
                offset = (width / 8) * i;
            } else {
                offset = (width / 8) * (16 - i);
            }

            buf_to_buf(&window_buffer[offset], width, height, width * 3, buffer_position, window->width);
            win_draw_rect(window->id, &bounds);
            process_bk();

            while (timer_get_stamp32() - time_stamp < TIMER_FPS_TO_TICKS(24)) {
            }
        }

        delete window_buffer;
    }

    delete[] world1_image;
    delete[] world2_image;
    delete[] stars_image;
}

void PlanetSelectMenu::Init() {
    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);

    event_click_done = 0;
    event_click_cancel = 0;
    image1 = nullptr;
    image2 = nullptr;
    image3 = nullptr;
    world = ini_get_setting(INI_WORLD);

    mouse_hide();

    for (int i = 0; i < PLANET_SELECT_MENU_ITEM_COUNT; ++i) {
        ButtonInit(i);
    }

    WindowManager_LoadImage(PLANETSE, window, window->width, false, false);

    DrawMaps(false);
    DrawTexts();

    mouse_show();
}

void PlanetSelectMenu::Deinit() {
    for (int i = 0; i < PLANET_SELECT_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }

    delete image1;
    delete image2;
    delete image3;
}

void PlanetSelectMenu::EventPlanet() {
    if (key < menu_item_index) {
        world = ((world / 6) * 6) + key;

        DrawTexts();
    }
}

void PlanetSelectMenu::EventWorld() {
    short old_world;
    bool direction;

    if (image2) {
        image2->Write(window);
        delete image2;
        image2 = nullptr;
    }

    old_world = world;
    direction = key == 7;

    if (direction) {
        world = ((world / 6) * 6) + 6;
    } else {
        world = ((world / 6) * 6) - 6;
    }

    if (world < 0) {
        world = 18;
    }

    if (world > 23) {
        world = 0;
    }

    mouse_hide();
    AnimateWorldChange(old_world, world, direction);
    DrawMaps(true);
    DrawTexts();
    mouse_show();
}

void PlanetSelectMenu::EventRandom() {
    world = (dos_rand() * 24) >> 15;
    event_click_done = true;
}

void PlanetSelectMenu::EventCancel() { event_click_cancel = true; }

void PlanetSelectMenu::EventHelp() { HelpMenu_Menu(HELPMENU_PLANET_SETUP, WINDOW_MAIN_WINDOW); }

void PlanetSelectMenu::EventDone() { event_click_done = true; }
