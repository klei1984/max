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
    unsigned int research_level;
    unsigned int turns_to_complete;
    unsigned int allocation;
};

struct ScreenLocation {
    signed char x;
    signed char y;
};

struct CTInfo {
    char field_0[40];
    char team_type;
    char field_41;
    char unit_counters[93];
    char team_clan;
    ResearchTopic research_topics[8];
    unsigned int team_points;
    unsigned short number_of_objects_created;
    ScreenLocation screen_location[6];
    TeamUnits *team_units;
    SmartPointer<UnitInfo> selected_unit;
    unsigned short zoom_level;
    unsigned short camera_position_x;
    unsigned short camera_position_y;
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
    short stats_factories_built;
    short stats_mines_built;
    short stats_buildings_built;
    short stats_units_built;
    short stats_gold_spent_on_upgrades;
    short score_graph[50];
    unsigned short casulties[93];
    char *heat_map_complete;
    char *heat_map_stealth_sea;
    char *heat_map_stealth_land;
};

#endif /* CTINFO_HPP */
