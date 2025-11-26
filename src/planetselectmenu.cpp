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
#include "menu.hpp"
#include "randomizer.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "window_manager.hpp"

void PlanetSelectMenu::ButtonInit(int32_t index) {
    struct PlanetSelectMenuControlItem* control = &planet_select_menu_controls[index];

    Text_SetFont(GNW_TEXT_FONT_1);

    if (control->image_id == INVALID_ID) {
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

void PlanetSelectMenu::DrawMaps(int32_t draw_to_screen) {
    int32_t world_first;
    int32_t world_last;
    uint8_t* buffer_position;

    world_first = (world / 6) * 6;
    world_last = world_first + 6;

    menu_item_index = 0;

    for (int32_t i = world_first; i < world_last; ++i) {
        process_bk();

        minimap_bg_images[i % PLANET_SELECT_MENU_MAP_SLOT_COUNT]->Write(window);

        buffer_position =
            &window->buffer[WindowManager_ScaleOffset(window, planet_select_menu_controls[menu_item_index].bounds.ulx,
                                                      planet_select_menu_controls[menu_item_index].bounds.uly)];

        if (Menu_LoadPlanetMinimap(i, buffer_position, window->width)) {
            ++menu_item_index;

            if (draw_to_screen) {
                win_draw_rect(window->id, &planet_select_menu_controls[menu_item_index].bounds);
            }
        }
    }

    uint8_t* world_resource = ResourceManager_ReadResource(static_cast<ResourceID>(SNOW_PIC + world / 6));
    struct ImageBigHeader* world_image = reinterpret_cast<struct ImageBigHeader*>(world_resource);
    int32_t ulx = world_image->ulx;
    int32_t uly = world_image->uly;
    delete[] world_resource;

    WindowManager_LoadBigImage(static_cast<ResourceID>(SNOW_PIC + world / 6), window, window->width, false, false,
                               WindowManager_ScaleUlx(window, ulx), WindowManager_ScaleUly(window, uly), false);

    if (!image3) {
        image3 = new (std::nothrow)
            Image(WindowManager_ScaleUlx(window, planet_select_menu_planet_description.bounds.ulx),
                  WindowManager_ScaleUly(window, planet_select_menu_planet_description.bounds.uly),
                  planet_select_menu_planet_description.bounds.lrx - planet_select_menu_planet_description.bounds.ulx,
                  planet_select_menu_planet_description.bounds.lry - planet_select_menu_planet_description.bounds.uly);
        image3->Copy(window);
    }
}

void PlanetSelectMenu::DrawTexts() {
    Rect* bounds;

    const char* menu_planet_descriptions[] = {_(1471), _(c336), _(f40f), _(6c6c), _(8d9a), _(cb1d), _(5195), _(012c),
                                              _(323e), _(57a1), _(6da3), _(708f), _(6903), _(4d80), _(ae4e), _(d0a4),
                                              _(fdd7), _(0ac0), _(5f4b), _(8475), _(cffe), _(7842), _(dfdd), _(6517)};

    const char* menu_planet_names[] = {_(e43b), _(f588), _(c78b), _(895d), _(5f5f), _(e7b2), _(f3fe), _(8524),
                                       _(4bb8), _(f408), _(0935), _(7303), _(94ef), _(c46c), _(48ac), _(275a),
                                       _(ea47), _(fcf0), _(6426), _(7ea8), _(386d), _(41e5), _(bbcb), _(ba99)};

    Text_SetFont(GNW_TEXT_FONT_5);

    menu_draw_menu_title(window, &planet_select_menu_screen_title, COLOR_GREEN, true);

    if (!image1) {
        image1 = new (std::nothrow)
            Image(WindowManager_ScaleUlx(window, planet_select_menu_planet_name.bounds.ulx),
                  WindowManager_ScaleUly(window, planet_select_menu_planet_name.bounds.uly),
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

    for (int32_t i = 0; i < PLANET_SELECT_MENU_ITEM_COUNT; ++i) {
        buttons[i]->Enable();
    }

    if (image2) {
        image2->Write(window);

        delete image2;
        image2 = nullptr;
    }

    bounds = &planet_select_menu_controls[world % 6].bounds;

    image2 = new (std::nothrow)
        Image(WindowManager_ScaleUlx(window, bounds->ulx), WindowManager_ScaleUly(window, bounds->uly),
              bounds->lrx - bounds->ulx + 1, bounds->lry - bounds->uly + 1);
    image2->Copy(window);

    draw_box(window->buffer, window->width, WindowManager_ScaleUlx(window, bounds->ulx),
             WindowManager_ScaleUly(window, bounds->uly), WindowManager_ScaleUlx(window, bounds->lrx),
             WindowManager_ScaleUly(window, bounds->lry), COLOR_RED);
    draw_box(window->buffer, window->width, WindowManager_ScaleUlx(window, bounds->ulx + 1),
             WindowManager_ScaleUly(window, bounds->uly + 1), WindowManager_ScaleUlx(window, bounds->lrx - 1),
             WindowManager_ScaleUly(window, bounds->lry - 1), COLOR_RED);
    win_draw(window->id);
}

void PlanetSelectMenu::AnimateWorldChange(int32_t world1, int32_t world2, bool direction) {
    ResourceID resource;
    uint8_t* world1_image;
    uint8_t* world2_image;
    uint8_t* stars_image;
    uint8_t* window_buffer;

    resource = static_cast<ResourceID>(SNOW_PIC + (world1 / 6));
    world1_image = ResourceManager_ReadResource(resource);

    resource = static_cast<ResourceID>(SNOW_PIC + (world2 / 6));
    world2_image = ResourceManager_ReadResource(resource);

    stars_image = ResourceManager_ReadResource(STAR_PIC);

    if (world1_image && world2_image && stars_image) {
        struct ImageBigHeader* image1_header = reinterpret_cast<struct ImageBigHeader*>(world1_image);
        struct ImageBigHeader* image2_header = reinterpret_cast<struct ImageBigHeader*>(world2_image);
        struct ImageBigHeader* image3_header = reinterpret_cast<struct ImageBigHeader*>(stars_image);
        int32_t width;
        int32_t height;
        Rect bounds;
        uint8_t* buffer_position;
        uint64_t time_stamp;
        int32_t offset;

        width = image1_header->width;
        height = image1_header->height;

        window_buffer = new (std::nothrow) uint8_t[width * height * 3];

        if (direction) {
            WindowManager_DecodeBigImage(image1_header, window_buffer, false, false, width * 3);
            WindowManager_DecodeBigImage(image3_header, &window_buffer[width], false, false, width * 3);
            WindowManager_DecodeBigImage(image2_header, &window_buffer[width * 2], false, false, width * 3);

        } else {
            WindowManager_DecodeBigImage(image1_header, &window_buffer[width * 2], false, false, width * 3);
            WindowManager_DecodeBigImage(image3_header, &window_buffer[width], false, false, width * 3);
            WindowManager_DecodeBigImage(image2_header, window_buffer, false, false, width * 3);
        }

        bounds.ulx = WindowManager_ScaleUlx(window, image1_header->ulx);
        bounds.uly = WindowManager_ScaleUly(window, image1_header->uly);
        bounds.lrx = WindowManager_ScaleLrx(window, image1_header->ulx, image1_header->ulx + image1_header->width);
        bounds.lry = WindowManager_ScaleLry(window, image1_header->uly, image1_header->uly + image1_header->height);

        buffer_position = &window->buffer[bounds.uly * window->width + bounds.ulx];

        for (int32_t i = 1; i <= 16; ++i) {
            time_stamp = timer_get();

            if (direction) {
                offset = (width / 8) * i;

            } else {
                offset = (width / 8) * (16 - i);
            }

            buf_to_buf(&window_buffer[offset], width, height, width * 3, buffer_position, window->width);
            win_draw_rect(window->id, &bounds);
            process_bk();

            while (timer_get() - time_stamp < TIMER_FPS_TO_MS(24)) {
            }
        }

        delete[] window_buffer;
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
    world = ResourceManager_GetSettings()->GetNumericValue("world");

    mouse_hide();

    for (int32_t i = 0; i < PLANET_SELECT_MENU_ITEM_COUNT; ++i) {
        ButtonInit(i);
    }

    WindowManager_LoadBigImage(PLANETSE, window, window->width, false, false, -1, -1, true);

    for (int32_t i = 0; i < PLANET_SELECT_MENU_MAP_SLOT_COUNT; ++i) {
        const auto bounds = &planet_select_menu_controls[i].bounds;

        minimap_bg_images[i] = new (std::nothrow)
            Image(WindowManager_ScaleUlx(window, bounds->ulx), WindowManager_ScaleUly(window, bounds->uly),
                  bounds->lrx - bounds->ulx + 1, bounds->lry - bounds->uly + 1);
        minimap_bg_images[i]->Copy(window);
    }

    DrawMaps(false);
    DrawTexts();

    mouse_show();
}

void PlanetSelectMenu::Deinit() {
    for (int32_t i = 0; i < PLANET_SELECT_MENU_ITEM_COUNT; ++i) {
        delete buttons[i];
    }

    for (int32_t i = 0; i < PLANET_SELECT_MENU_MAP_SLOT_COUNT; ++i) {
        delete minimap_bg_images[i];
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
    int16_t old_world;
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
    world = Randomizer_Generate(24);
    event_click_done = true;
}

void PlanetSelectMenu::EventCancel() { event_click_cancel = true; }

void PlanetSelectMenu::EventHelp() { HelpMenu_Menu("PLANET_SETUP", WINDOW_MAIN_WINDOW); }

void PlanetSelectMenu::EventDone() { event_click_done = true; }
