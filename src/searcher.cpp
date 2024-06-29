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

#include "searcher.hpp"

#include "ailog.hpp"
#include "game_manager.hpp"
#include "gfx.hpp"
#include "paths_manager.hpp"
#include "resource_manager.hpp"
#include "window_manager.hpp"

static void Searcher_DrawMarker(int32_t angle, int32_t grid_x, int32_t grid_y, int32_t color);
static int32_t Searcher_EvaluateCost(const Point position, const Point new_position, const bool air_support);

int32_t Searcher::Searcher_MarkerColor = COLOR_RED;

void Searcher_DrawMarker(int32_t angle, int32_t grid_x, int32_t grid_y, int32_t color) {
    WindowInfo* window;
    int32_t pixel_x;
    int32_t pixel_y;
    Rect bounds;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    grid_x = grid_x * 64 + 32;
    grid_y = grid_y * 64 + 32;

    if (grid_x < GameManager_MapWindowDrawBounds.lrx && grid_x > GameManager_MapWindowDrawBounds.ulx &&
        grid_y < GameManager_MapWindowDrawBounds.lry && grid_y > GameManager_MapWindowDrawBounds.uly) {
        pixel_x = (grid_x << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
        pixel_y = (grid_y << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

        Paths_DrawMarker(window, angle, pixel_x, pixel_y, color);

        bounds.ulx = window->window.ulx + pixel_x - (Gfx_ZoomLevel / 2);
        bounds.uly = window->window.uly + pixel_y - (Gfx_ZoomLevel / 2);
        bounds.lrx = bounds.ulx + Gfx_ZoomLevel;
        bounds.lry = bounds.uly + Gfx_ZoomLevel;

        win_draw_rect(window->id, &bounds);
    }
}

int32_t Searcher_EvaluateCost(const Point position, const Point new_position, const bool air_support) {
    uint8_t value1;
    uint8_t value2;
    int32_t result;

    value2 = PathsManager_GetAccessMap()[new_position.x][new_position.y];

    ++Paths_EvaluatorCallCount;

    if (air_support) {
        value1 = PathsManager_GetAccessMap()[position.x][position.y];

        if ((value2 & 0x40) && (value1 & 0x80)) {
            result = 0;

        } else if ((value1 & 0x40) && (value2 & 0x80)) {
            result = 0;

        } else {
            result = value2 & 0x1F;
        }

    } else {
        result = value2 & 0x1F;
    }

    return result;
}

Searcher::Searcher(const Point start_point, const Point end_point, const bool air_support)
    : use_air_support(air_support) {
    Point map_size;
    PathSquare square;

    costs_map = new (std::nothrow) uint16_t*[ResourceManager_MapSize.x];
    directions_map = new (std::nothrow) uint8_t*[ResourceManager_MapSize.x];

    for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
        costs_map[i] = new (std::nothrow) uint16_t[ResourceManager_MapSize.y];
        directions_map[i] = new (std::nothrow) uint8_t[ResourceManager_MapSize.y];

        memset(directions_map[i], 0xFF, ResourceManager_MapSize.y);

        for (int32_t j = 0; j < ResourceManager_MapSize.y; ++j) {
            costs_map[i][j] = 0x3FFF;
        }
    }

    line_distance_max = 0;

    map_size.x = ResourceManager_MapSize.x - start_point.x;
    map_size.y = ResourceManager_MapSize.y - start_point.y;

    if (map_size.x < start_point.x) {
        map_size.x = start_point.x;
    }

    if (map_size.y < start_point.y) {
        map_size.y = start_point.y;
    }

    {
        if (map_size.x <= map_size.y) {
            line_distance_limit = map_size.y * 2 + map_size.x + 1;

        } else {
            line_distance_limit = map_size.x * 2 + map_size.y + 1;
        }

        distance_vector = new (std::nothrow) uint16_t[line_distance_limit];

        for (int32_t i = 0; i < line_distance_limit; ++i) {
            distance_vector[i] = 0x7FFF;
        }

        distance_vector[0] = 0;
        costs_map[start_point.x][start_point.y] = 0;

        square.point.x = start_point.x;
        square.point.y = start_point.y;
        square.cost = 0;

        squares.Append(&square);
        destination = end_point;
    }
}

Searcher::~Searcher() {
    for (int32_t i = 0; i < ResourceManager_MapSize.x; ++i) {
        delete[] costs_map[i];
        delete[] directions_map[i];
    }

    delete[] costs_map;
    delete[] directions_map;
    delete[] distance_vector;
}

void Searcher::EvaluateSquare(const Point position, const int32_t cost, const int32_t direction,
                              Searcher* const searcher) {
    ++Paths_EvaluatedSquareCount;

    if (costs_map[position.x][position.y] > cost) {
        Point distance;
        int16_t line_distance;
        int16_t best_cost;

        costs_map[position.x][position.y] = cost;

        directions_map[position.x][position.y] = direction;

        distance.x = labs(destination.x - position.x);
        distance.y = labs(destination.y - position.y);

        if (distance.x > distance.y) {
            line_distance = distance.x * 2 + distance.y;

        } else {
            line_distance = distance.y * 2 + distance.x;
        }

        if (line_distance > searcher->line_distance_max) {
            best_cost = searcher->distance_vector[searcher->line_distance_max] +
                        ((line_distance - searcher->line_distance_max) & (~1));

        } else {
            best_cost = searcher->distance_vector[line_distance];
        }

        if (cost + best_cost <= costs_map[destination.x][destination.y]) {
            uint32_t square_count;
            int32_t loop_limit;
            int32_t index;

            square_count = squares.GetCount();

            if (square_count + 1 > Paths_MaxDepth) {
                Paths_MaxDepth = square_count + 1;
            }

            if (searcher->costs_map[position.x][position.y] + cost < costs_map[destination.x][destination.y]) {
                costs_map[destination.x][destination.y] = searcher->costs_map[position.x][position.y] + cost;
            }

            ++Paths_SquareAdditionsCount;

            if (square_count > 0) {
                int32_t array_index;

                for (index = 0, loop_limit = square_count - 1; index < loop_limit;) {
                    array_index = (index + loop_limit + 1) / 2;

                    if (squares[array_index]->cost >= cost) {
                        index = array_index;

                    } else {
                        loop_limit = array_index - 1;
                    }
                }

                square_count = index;
            }

            Paths_SquareInsertionsCount += squares.GetCount() - square_count;

            {
                PathSquare path_square;

                path_square.point = position;
                path_square.cost = cost;

                squares.Insert(&path_square, square_count);

                if (Paths_DebugMode >= 2) {
                    Searcher_DrawMarker(direction, position.x, position.y, Searcher_MarkerColor);
                }
            }
        }

    } else if (costs_map[position.x][position.y] == cost) {
        directions_map[position.x][position.y] = direction;

        if (Paths_DebugMode >= 2) {
            Searcher_DrawMarker(direction, position.x, position.y, Searcher_MarkerColor);
        }
    }
}

void Searcher::UpdateCost(const Point start_point, const Point end_point, const int32_t cost) {
    Point distance;
    int32_t line_distance;

    distance.x = labs(end_point.x - start_point.x);
    distance.y = labs(end_point.y - start_point.y);

    if (distance.x > distance.y) {
        line_distance = distance.x * 2 + distance.y;

    } else {
        line_distance = distance.y * 2 + distance.x;
    }

    if (line_distance > line_distance_max) {
        line_distance_max = line_distance;

        SDL_assert(line_distance_limit > line_distance_max);
    }

    while (cost < distance_vector[line_distance]) {
        distance_vector[line_distance] = cost;
        --line_distance;
    }
}

void Searcher::Process(Point position, const bool mode_flag) {
    Point new_position;
    Point distance;
    Point path_step;
    Point angle_offset;
    Point distance_limit;
    PathSquare path_square;
    int32_t min_distance;
    int32_t direction;
    int32_t unit_angle;
    int32_t step_cost;

    distance = destination - position;

    if (distance.x <= 0) {
        distance.x = -distance.x;
        path_step.x = -1;

    } else {
        path_step.x = 1;
    }

    if (distance.y <= 0) {
        distance.y = -distance.y;
        path_step.y = -1;

    } else {
        path_step.y = 1;
    }

    if (distance.x < distance.y) {
        min_distance = distance.x;

        if (min_distance == 0) {
            min_distance = 1;
            ++distance.y;
            distance.x = 1;
        }

        direction = path_step.y * 2 + 2;

        if (path_step.x == path_step.y) {
            if (direction) {
                angle_offset.x = -1;

            } else {
                angle_offset.x = 7;
            }

        } else {
            angle_offset.x = 1;
        }

        angle_offset.y = 0;

    } else {
        min_distance = distance.y;

        if (min_distance == 0) {
            min_distance = 1;
            ++distance.x;
            distance.y = 1;
        }

        direction = 4 - path_step.x * 2;

        if (path_step.x == path_step.y) {
            angle_offset.y = 1;

        } else {
            angle_offset.y = -1;
        }

        angle_offset.x = 0;
    }

    distance_limit.x = 0;
    distance_limit.y = 0;

    path_square.point = position;
    path_square.cost = 0;

    while (path_square.point != destination) {
        distance_limit.x += min_distance;
        unit_angle = direction;

        if (distance_limit.x >= distance.y) {
            unit_angle += angle_offset.x;
            distance_limit.x -= distance.y;
        }

        distance_limit.y += min_distance;

        if (distance_limit.y >= distance.x) {
            unit_angle += angle_offset.y;
            distance_limit.y -= distance.x;
        }

        SDL_assert(unit_angle < 8 && unit_angle >= 0);

        new_position = position + Paths_8DirPointsArray[unit_angle];

        step_cost = Searcher_EvaluateCost(position, new_position, use_air_support);

        if (step_cost == 0) {
            return;
        }

        if (!mode_flag) {
            step_cost = Searcher_EvaluateCost(new_position, position, use_air_support);
        }

        if (unit_angle & 1) {
            step_cost = (step_cost * 3) / 2;
        }

        position = new_position;

        path_square.point = new_position;
        path_square.cost += step_cost;

        costs_map[new_position.x][new_position.y] = path_square.cost;
        directions_map[new_position.x][new_position.y] = unit_angle;

        squares.Insert(&path_square, 0);
    }
}

bool Searcher::ForwardSearch(Searcher* const backward_searcher) {
    Point step;
    Point position;
    int32_t position_cost;
    bool result;
    int32_t cost;

    if (squares.GetCount()) {
        ++Paths_EvaluatedTileCount;

        position = squares[squares.GetCount() - 1]->point;
        squares.Remove(squares.GetCount() - 1);

        position_cost = costs_map[position.x][position.y];

        UpdateCost(position, backward_searcher->destination, position_cost);

        Searcher_MarkerColor = COLOR_RED;

        for (int32_t direction = 0; direction < 8; ++direction) {
            step = position;
            step += Paths_8DirPointsArray[direction];

            if (step.x >= 0 && step.x < ResourceManager_MapSize.x && step.y >= 0 &&
                step.y < ResourceManager_MapSize.y) {
                if (position_cost < costs_map[step.x][step.y]) {
                    cost = Searcher_EvaluateCost(position, step, use_air_support);

                    if (cost > 0) {
                        if (direction & 1) {
                            cost = (cost * 3) / 2;
                        }

                        EvaluateSquare(step, position_cost + cost, direction, backward_searcher);
                    }
                }
            }
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

bool Searcher::BackwardSearch(Searcher* const forward_searcher) {
    bool result;

    if (squares.GetCount()) {
        ++Paths_EvaluatedTileCount;

        Point position = squares[squares.GetCount() - 1]->point;
        squares.Remove(squares.GetCount() - 1);

        const int32_t position_cost = costs_map[position.x][position.y];

        UpdateCost(position, forward_searcher->destination, position_cost);

        Searcher_MarkerColor = COLOR_BLUE;

        const int32_t reference_cost = Searcher_EvaluateCost(position, position, use_air_support);

        for (int32_t direction = 0; direction < 8; ++direction) {
            const Point step = position + Paths_8DirPointsArray[direction];

            if (step.x >= 0 && step.x < ResourceManager_MapSize.x && step.y >= 0 &&
                step.y < ResourceManager_MapSize.y) {
                if (position_cost < costs_map[step.x][step.y]) {
                    if (Searcher_EvaluateCost(position, step, use_air_support) > 0) {
                        int32_t cost = reference_cost;

                        if (direction & 1) {
                            cost = (cost * 3) / 2;
                        }

                        EvaluateSquare(step, position_cost + cost, direction, forward_searcher);
                    }
                }
            }
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

SmartPointer<GroundPath> Searcher::DeterminePath(const Point position, const int32_t max_cost) {
    SmartPointer<GroundPath> ground_path;
    ObjectArray<Point> steps;
    int32_t destination_x;
    int32_t destination_y;
    int32_t direction;

    destination_x = destination.x;
    destination_y = destination.y;

    for (;;) {
        if (destination_x == position.x && destination_y == position.y) {
            if (steps.GetCount()) {
                ground_path = new (std::nothrow) GroundPath(destination.x, destination.y);

                for (int32_t steps_count = steps.GetCount() - 1, cost = 0; steps_count >= 0 && cost < max_cost;
                     --steps_count) {
                    destination_x += steps[steps_count]->x;
                    destination_y += steps[steps_count]->y;

                    int32_t accessmap_cost = PathsManager_GetAccessMap()[destination_x][destination_y] & 0x1F;

                    if (steps[steps_count]->x && steps[steps_count]->y) {
                        accessmap_cost = (accessmap_cost * 3) / 2;
                    }

                    cost += accessmap_cost;

                    ground_path->AddStep(steps[steps_count]->x, steps[steps_count]->y);
                }

                if (!ground_path->GetSteps()->GetCount()) {
                    ground_path = nullptr;
                }

                return ground_path;

            } else {
                AiLog log("Error: null path.");

                return ground_path;
            }

        } else {
            direction = directions_map[destination_x][destination_y];

            if (direction < 8) {
                steps.Append(const_cast<Point*>(&Paths_8DirPointsArray[direction]));

                destination_x -= Paths_8DirPointsArray[direction].x;
                destination_y -= Paths_8DirPointsArray[direction].y;

                SDL_assert(directions_map[destination_x][destination_y] != (direction + 4 % 8));

                if (destination_x < 0 || destination_x >= ResourceManager_MapSize.x || destination_y < 0 ||
                    destination_y >= ResourceManager_MapSize.y) {
                    return ground_path;
                }

            } else {
                AiLog log("Error in path transcription.");

                return ground_path;
            }
        }
    }
}
