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

#ifndef PLANETSELECTMENU_HPP
#define PLANETSELECTMENU_HPP

#include "button.hpp"
#include "menu.hpp"

#define PLANET_SELECT_MENU_ITEM_COUNT 12
#define PLANET_SELECT_MENU_MAP_SLOT_COUNT 6

enum PlanetType {
    PLANET_SNOWCRAB,
    PLANET_FRIGIA,
    PLANET_ICE_BERG,
    PLANET_THE_COOLER,
    PLANET_ULTIMA_THULE,
    PLANET_LONG_FLOES,
    PLANET_IRON_CROSS,
    PLANET_SPLATTERSCAPE,
    PLANET_PEAK_A_BOO,
    PLANET_VALENTINES_PLANET,
    PLANET_THREE_RINGS,
    PLANET_GREAT_DIVIDE,
    PLANET_NEW_LUZON,
    PLANET_MIDDLE_SEA,
    PLANET_HIGH_IMPACT,
    PLANET_SANCTUARY,
    PLANET_ISLANDIA,
    PLANET_HAMMERHEAD,
    PLANET_FRECKLES,
    PLANET_SANDSPIT,
    PLANET_GREAT_CIRCLE,
    PLANET_LONG_PASSAGE,
    PLANET_FLASH_POINT,
    PLANET_BOTTLENECK
};

class PlanetSelectMenu;

struct PlanetSelectMenuItem {
    uint32_t r_value;
    uint32_t event_code;
    void (PlanetSelectMenu::*event_handler)();
};

struct PlanetSelectMenuControlItem {
    Rect bounds;
    ResourceID image_id;
    const char *label;
    int32_t event_code;
    void (PlanetSelectMenu::*event_handler)();
    ResourceID sfx;
};

#define MENU_CONTROL_DEF(ulx, uly, lrx, lry, image_id, label, event_code, event_handler, sfx) \
    {{(ulx), (uly), (lrx), (lry)}, (image_id), (label), (event_code), (event_handler), (sfx)}

class PlanetSelectMenu {
    struct MenuTitleItem planet_select_menu_screen_title = {{230, 6, 410, 26}, _(f7c8)};
    struct MenuTitleItem planet_select_menu_planet_name = {{16, 61, 165, 80}, ""};
    struct MenuTitleItem planet_select_menu_planet_description = {{41, 350, 600, 409}, ""};

    struct PlanetSelectMenuControlItem planet_select_menu_controls[PLANET_SELECT_MENU_ITEM_COUNT] = {
        MENU_CONTROL_DEF(192, 51, 303, 162, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
        MENU_CONTROL_DEF(330, 51, 441, 162, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
        MENU_CONTROL_DEF(469, 51, 580, 162, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
        MENU_CONTROL_DEF(192, 189, 303, 300, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
        MENU_CONTROL_DEF(330, 189, 441, 300, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
        MENU_CONTROL_DEF(469, 189, 580, 300, INVALID_ID, nullptr, 0, &PlanetSelectMenu::EventPlanet, PSELM0),
        MENU_CONTROL_DEF(60, 248, 0, 0, MNULAROU, nullptr, 0, &PlanetSelectMenu::EventWorld, PSELW0),
        MENU_CONTROL_DEF(92, 248, 0, 0, MNURAROU, nullptr, 0, &PlanetSelectMenu::EventWorld, PSELW0),
        MENU_CONTROL_DEF(243, 438, 0, 0, MNUBTN4U, _(67bc), 0, &PlanetSelectMenu::EventRandom, PDONE0),
        MENU_CONTROL_DEF(354, 438, 0, 0, MNUBTN4U, _(9d35), GNW_KB_KEY_SHIFT_ESCAPE, &PlanetSelectMenu::EventCancel,
                         PCANC0),
        MENU_CONTROL_DEF(465, 438, 0, 0, MNUBTN5U, _(8a8d), GNW_KB_KEY_SHIFT_DIVIDE, &PlanetSelectMenu::EventHelp,
                         PHELP0),
        MENU_CONTROL_DEF(514, 438, 0, 0, MNUBTN6U, _(bf2c), GNW_KB_KEY_SHIFT_RETURN, &PlanetSelectMenu::EventDone,
                         PDONE0),
    };

    void ButtonInit(int32_t index);
    void DrawMaps(int32_t draw_to_screen);
    void DrawTexts();
    void AnimateWorldChange(int32_t world1, int32_t world2, bool direction);

public:
    WindowInfo *window;
    uint32_t event_click_done;
    uint32_t event_click_cancel;
    int16_t world;
    uint16_t menu_item_index;
    uint32_t key;
    Image *image1;
    Image *image2;
    Image *image3;
    Button *buttons[PLANET_SELECT_MENU_ITEM_COUNT];
    PlanetSelectMenuItem menu_item[PLANET_SELECT_MENU_ITEM_COUNT];
    Image *minimap_bg_images[PLANET_SELECT_MENU_MAP_SLOT_COUNT];

    void Init();
    void Deinit();
    void EventPlanet();
    void EventWorld();
    void EventRandom();
    void EventCancel();
    void EventHelp();
    void EventDone();
};

#endif /* PLANETSELECTMENU_HPP */
