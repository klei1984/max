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
    int32_t research_level;
    int32_t turns_to_complete;
    int32_t allocation;
};

struct ScreenLocation {
    int8_t x;
    int8_t y;
};

struct CTInfo {
    CTInfo() : heat_map_complete(nullptr), heat_map_stealth_sea(nullptr), heat_map_stealth_land(nullptr) { Reset(); }

    void Reset() noexcept {
        for (auto& marker : markers) {
            marker = {-1, -1};
        }

        team_type = TEAM_TYPE_NONE;
        finished_turn = false;

        for (auto& unit_counter : unit_counters) {
            unit_counter = 1;
        }

        team_clan = TEAM_CLAN_RANDOM;

        for (auto& research_topic : research_topics) {
            research_topic = {0, 0, 0};
        }

        team_points = 0;
        number_of_objects_created = 0;

        for (auto& screen_location : screen_locations) {
            screen_location = {-1, -1};
        }

        team_units = nullptr;
        selected_unit = nullptr;

        zoom_level = 0;

        camera_position = {0, 0};

        display_button_range = 0;
        display_button_scan = 0;
        display_button_status = 0;
        display_button_colors = 0;
        display_button_hits = 0;
        display_button_ammo = 0;
        display_button_names = 0;
        display_button_minimap_2x = 0;
        display_button_minimap_tnt = 0;
        display_button_grid = 0;
        display_button_survey = 0;

        stats_factories_built = 0;
        stats_mines_built = 0;
        stats_buildings_built = 0;
        stats_units_built = 0;
        stats_gold_spent_on_upgrades = 0;

        for (auto& item : score_graph) {
            item = 0;
        }

        for (auto& casualty : casualties) {
            casualty = 0;
        }

        delete[] heat_map_complete;
        heat_map_complete = nullptr;

        delete[] heat_map_stealth_sea;
        heat_map_stealth_sea = nullptr;

        delete[] heat_map_stealth_land;
        heat_map_stealth_land = nullptr;
    }

    Point markers[10];
    uint8_t team_type;
    bool finished_turn;
    uint8_t unit_counters[UNIT_END];
    uint8_t team_clan;
    ResearchTopic research_topics[RESEARCH_TOPIC_COUNT];
    uint32_t team_points;
    uint16_t number_of_objects_created;
    ScreenLocation screen_locations[6];
    TeamUnits* team_units;
    SmartPointer<UnitInfo> selected_unit;
    uint16_t zoom_level;
    Point camera_position;
    int8_t display_button_range;
    int8_t display_button_scan;
    int8_t display_button_status;
    int8_t display_button_colors;
    int8_t display_button_hits;
    int8_t display_button_ammo;
    int8_t display_button_names;
    int8_t display_button_minimap_2x;
    int8_t display_button_minimap_tnt;
    int8_t display_button_grid;
    int8_t display_button_survey;
    int16_t stats_factories_built;
    int16_t stats_mines_built;
    int16_t stats_buildings_built;
    int16_t stats_units_built;
    int32_t stats_gold_spent_on_upgrades;
    int16_t score_graph[50];
    uint16_t casualties[UNIT_END];
    int8_t* heat_map_complete;
    int8_t* heat_map_stealth_sea;
    int8_t* heat_map_stealth_land;
};

#endif /* CTINFO_HPP */
