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

#ifndef CTINFO_HPP
#define CTINFO_HPP

#include "teamunits.hpp"
#include "unitinfo.hpp"

struct ResearchTopic {
    uint32_t research_level;
    int32_t turns_to_complete;
    uint32_t allocation;
};

struct ScreenLocation {
    int8_t x;
    int8_t y;
};

struct CTInfo {
    Point markers[10];
    char team_type;
    bool finished_turn;
    uint8_t unit_counters[UNIT_END];
    char team_clan;
    ResearchTopic research_topics[RESEARCH_TOPIC_COUNT];
    uint32_t team_points;
    uint16_t number_of_objects_created;
    ScreenLocation screen_location[6];
    TeamUnits *team_units;
    SmartPointer<UnitInfo> selected_unit;
    uint16_t zoom_level;
    Point camera_position;
    char display_button_range;
    char display_button_scan;
    char display_button_status;
    char display_button_colors;
    char display_button_hits;
    char display_button_ammo;
    char display_button_names;
    char display_button_minimap_2x;
    char display_button_minimap_tnt;
    char display_button_grid;
    char display_button_survey;
    int16_t stats_factories_built;
    int16_t stats_mines_built;
    int16_t stats_buildings_built;
    int16_t stats_units_built;
    int16_t stats_gold_spent_on_upgrades;
    int16_t score_graph[50];
    uint16_t casualties[UNIT_END];
    char *heat_map_complete;
    char *heat_map_stealth_sea;
    char *heat_map_stealth_land;
};

#endif /* CTINFO_HPP */
