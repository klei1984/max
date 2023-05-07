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

static void Searcher_DrawMarker(int angle, int grid_x, int grid_y, int color);
static int Searcher_EvaluateCost(Point point1, Point point2, bool mode);

int Searcher::Searcher_MarkerColor = 1;

void Searcher_DrawMarker(int angle, int grid_x, int grid_y, int color) {
    WindowInfo* window;
    int pixel_x;
    int pixel_y;
    Rect bounds;

    window = WindowManager_GetWindow(WINDOW_MAIN_MAP);

    pixel_x = grid_x * 64 + 32;
    pixel_y = grid_y * 64 + 32;

    if (pixel_x < GameManager_MapWindowDrawBounds.lrx && pixel_x > GameManager_MapWindowDrawBounds.ulx &&
        pixel_y < GameManager_MapWindowDrawBounds.lry && pixel_y > GameManager_MapWindowDrawBounds.uly) {
    }

    grid_x = (pixel_x << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUlx;
    grid_y = (pixel_y << 16) / Gfx_MapScalingFactor - Gfx_MapWindowUly;

    Paths_DrawMarker(window, angle, grid_x, grid_y, color);

    bounds.ulx = window->window.ulx + grid_x - (Gfx_ZoomLevel / 2);
    bounds.uly = window->window.uly + grid_y - (Gfx_ZoomLevel / 2);
    bounds.lrx = bounds.ulx + Gfx_ZoomLevel;
    bounds.lry = bounds.uly + Gfx_ZoomLevel;

    win_draw_rect(window->id, &bounds);
}

int Searcher_EvaluateCost(Point point1, Point point2, bool mode) {
    unsigned char value1;
    unsigned char value2;
    int result;

    value2 = PathsManager_GetAccessMap()[point2.x][point2.y];

    ++Paths_EvaluatorCallCount;

    if (mode) {
        value1 = PathsManager_GetAccessMap()[point1.x][point1.y];

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

Searcher::Searcher(Point point1, Point point2, unsigned char mode) : mode(mode) {
    Point map_size;
    PathSquare square;

    costs_map = new (std::nothrow) unsigned short*[ResourceManager_MapSize.x];
    directions_map = new (std::nothrow) unsigned char*[ResourceManager_MapSize.x];

    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        costs_map[i] = new (std::nothrow) unsigned short[ResourceManager_MapSize.y];
        directions_map[i] = new (std::nothrow) unsigned char[ResourceManager_MapSize.y];

        memset(directions_map[i], 0xFF, ResourceManager_MapSize.y);

        for (int j = 0; j < ResourceManager_MapSize.y; ++j) {
            costs_map[i][j] = 0x3FFF;
        }
    }

    field_12 = 0;

    map_size.x = ResourceManager_MapSize.x - point1.x;
    map_size.y = ResourceManager_MapSize.y - point1.y;

    if (map_size.x < point1.x) {
        map_size.x = point1.x;
    }

    if (map_size.y < point1.y) {
        map_size.y = point1.y;
    }

    {
        int array_size;

        if (map_size.x <= map_size.y) {
            array_size = map_size.y * 2 + map_size.x;

        } else {
            array_size = map_size.x * 2 + map_size.y;
        }

        array = new (std::nothrow) unsigned short[array_size];

        for (int i = 0; i < array_size; ++i) {
            array[i] = 0x7FFF;
        }

        array[0] = 0;
        costs_map[point1.x][point1.y] = 0;

        square.point.x = point1.x;
        square.point.y = point1.y;
        square.cost = 0;

        squares.Append(&square);
        destination = point2;
    }
}

Searcher::~Searcher() {
    for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
        delete[] costs_map[i];
        delete[] directions_map[i];
    }

    delete[] costs_map;
    delete[] directions_map;
    delete[] array;
}

void Searcher::EvaluateSquare(Point point, int cost, int direction, Searcher* searcher) {
    Point distance;
    short line_distance;
    short array_value;

    ++Paths_EvaluatedSquareCount;

    if (costs_map[point.x][point.y] > cost) {
        costs_map[point.x][point.y] = cost;

        directions_map[point.x][point.y] = direction;

        distance.x = labs(destination.x - point.x);
        distance.y = labs(destination.y - point.y);

        if (distance.x > distance.y) {
            line_distance = distance.x * 2 + distance.y;

        } else {
            line_distance = distance.y * 2 + distance.x;
        }

        if (line_distance > searcher->field_12) {
            array_value = searcher->array[searcher->field_12] + ((line_distance - searcher->field_12) & (~1));

        } else {
            array_value = searcher->array[line_distance];
        }

        if (cost + array_value <= costs_map[destination.x][destination.y]) {
            int square_count;
            int loop_limit;
            int index;

            square_count = squares.GetCount();

            if (square_count + 1 > Paths_MaxDepth) {
                Paths_MaxDepth = square_count + 1;
            }

            if (searcher->costs_map[point.x][point.y] + cost < costs_map[destination.x][destination.y]) {
                costs_map[destination.x][destination.y] = searcher->costs_map[point.x][point.y] + cost;
            }

            ++Paths_SquareAdditionsCount;

            if (square_count > 0) {
                int array_index;

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

                path_square.point = point;
                path_square.cost = cost;

                squares.Insert(&path_square, square_count);

                if (Paths_DebugMode >= 2) {
                    Searcher_DrawMarker(direction, point.x, point.y, Searcher_MarkerColor);
                }
            }
        }

    } else if (costs_map[point.x][point.y] == cost) {
        directions_map[point.x][point.y] = direction;

        if (Paths_DebugMode >= 2) {
            Searcher_DrawMarker(direction, point.x, point.y, Searcher_MarkerColor);
        }
    }
}

void Searcher::UpdateCost(Point point1, Point point2, int cost) {
    Point distance;
    int line_distance;

    distance.x = labs(point2.x - point1.x);
    distance.y = labs(point2.y - point1.y);

    if (distance.x > distance.y) {
        line_distance = distance.x * 2 + distance.y;

    } else {
        line_distance = distance.y * 2 + distance.x;
    }

    if (line_distance > field_12) {
        field_12 = line_distance;
    }

    while (cost < array[line_distance]) {
        array[line_distance] = cost;
        --line_distance;
    }
}

void Searcher::Process(Point point, bool mode_flag) {
    Point point1;
    Point distance;
    Point point3;
    Point point4;
    Point point5;
    PathSquare path_square;
    int min_distance;
    int direction;
    int angle;
    int step_cost;

    distance.x = destination.x - point.x;
    distance.y = destination.y - point.y;

    if (distance.x <= 0) {
        distance.x = -distance.x;
        point3.x = -1;

    } else {
        point3.x = 1;
    }

    if (distance.y <= 0) {
        distance.y = -distance.y;
        point3.y = -1;

    } else {
        point3.y = 1;
    }

    if (distance.x < distance.y) {
        min_distance = distance.x;

        if (min_distance == 0) {
            min_distance = 1;
            ++distance.y;
            distance.x = 1;
        }

        direction = point3.y * 2 + 2;

        if (point3.x == point3.y) {
            if (direction) {
                point4.x = -1;

            } else {
                point4.x = 7;
            }

        } else {
            point4.x = 1;
        }

        point4.y = 0;

    } else {
        min_distance = distance.y;

        if (min_distance == 0) {
            min_distance = 1;
            ++distance.x;
            distance.y = 1;
        }

        direction = 4 - point3.x * 2;

        if (point3.x == point3.y) {
            point4.y = 1;

        } else {
            point4.y = -1;
        }

        point4.x = 0;
    }

    point5.x = 0;
    point5.y = 0;

    path_square.point = point;
    path_square.cost = 0;

    while (path_square.point != destination) {
        point5.x += min_distance;
        angle = direction;

        if (point5.x >= distance.y) {
            angle += point4.x;
            point5.x -= distance.y;
        }

        point5.y += min_distance;

        if (point5.y >= distance.x) {
            angle += point4.y;
            point5.y -= distance.x;
        }

        point1 = point;
        point1 += Paths_8DirPointsArray[angle];

        step_cost = Searcher_EvaluateCost(point, point1, mode);

        if (step_cost == 0) {
            return;
        }

        if (!mode_flag) {
            step_cost = Searcher_EvaluateCost(point1, point, mode);
        }

        if (angle & 1) {
            step_cost = (step_cost * 3) / 2;
        }

        point = point1;

        path_square.point = point1;
        path_square.cost += step_cost;

        costs_map[point1.x][point1.y] = path_square.cost;
        directions_map[point1.x][point1.y] = angle;

        squares.Insert(&path_square, 0);
    }
}

bool Searcher::ForwardSearch(Searcher* backward_searcher) {
    Point step;
    Point position;
    int position_cost;
    bool result;
    int cost;

    if (squares.GetCount()) {
        ++Paths_EvaluatedTileCount;

        position = squares[squares.GetCount() - 1]->point;
        squares.Remove(squares.GetCount() - 1);

        position_cost = costs_map[position.x][position.y];

        UpdateCost(position, backward_searcher->destination, position_cost);

        Searcher_MarkerColor = 1;

        for (int direction = 0; direction < 8; ++direction) {
            step = position;
            step += Paths_8DirPointsArray[direction];

            if (step.x >= 0 && step.x < ResourceManager_MapSize.x && step.y >= 0 &&
                step.y < ResourceManager_MapSize.y) {
                if (position_cost < costs_map[step.x][step.y]) {
                    cost = Searcher_EvaluateCost(position, step, mode);

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

bool Searcher::BackwardSearch(Searcher* forward_searcher) {
    Point step;
    Point position;
    int position_cost;
    bool result;
    int reference_cost;
    int cost;

    if (squares.GetCount()) {
        ++Paths_EvaluatedTileCount;

        position = squares[squares.GetCount() - 1]->point;
        squares.Remove(squares.GetCount() - 1);

        position_cost = costs_map[position.x][position.y];

        UpdateCost(position, forward_searcher->destination, position_cost);

        Searcher_MarkerColor = 3;

        reference_cost = Searcher_EvaluateCost(position, position, mode);

        for (int direction = 0; direction < 8; ++direction) {
            step = position;
            step += Paths_8DirPointsArray[direction];

            if (step.x >= 0 && step.x < ResourceManager_MapSize.x && step.y >= 0 &&
                step.y < ResourceManager_MapSize.y) {
                if (position_cost < costs_map[step.x][step.y]) {
                    if (Searcher_EvaluateCost(position, step, mode) > 0) {
                        cost = reference_cost;

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

SmartPointer<GroundPath> Searcher::DeterminePath(Point point, int max_cost) {
    SmartPointer<GroundPath> ground_path(new (std::nothrow) GroundPath(destination.x, destination.y));
    ObjectArray<Point> points;
    int destination_x;
    int destination_y;
    int direction;

    destination_x = destination.x;
    destination_y = destination.y;

    for (;;) {
        if (destination_x == point.x && destination_y == point.y) {
            if (points.GetCount()) {
                int cost = 0;
                int accessmap_cost;

                for (int steps_count = points.GetCount() - 1; steps_count >= 0 && cost < max_cost; --steps_count) {
                    destination_x += points[steps_count]->x;
                    destination_y += points[steps_count]->y;

                    accessmap_cost = PathsManager_GetAccessMap()[destination_x][destination_y] & 0x1F;

                    if (destination_x && destination_y) {
                        accessmap_cost = (accessmap_cost * 3) / 2;
                    }

                    cost += accessmap_cost;

                    ground_path->AddStep(points[steps_count]->x, points[steps_count]->y);
                }

                if (!ground_path->GetSteps()->GetCount()) {
                    ground_path = nullptr;
                }

                return ground_path;

            } else {
                AiLog log("Error: null path.");

                ground_path = nullptr;
                return ground_path;
            }

        } else {
            direction = directions_map[destination_x][destination_y];

            if (direction < 8) {
                points.Append(const_cast<Point*>(&Paths_8DirPointsArray[direction]));

                destination_x -= Paths_8DirPointsArray[direction].x;
                destination_y -= Paths_8DirPointsArray[direction].y;

                if (destination_x < 0 || destination_x >= ResourceManager_MapSize.x || destination_x < 0 ||
                    destination_y >= ResourceManager_MapSize.y) {
                    ground_path = nullptr;
                    return ground_path;
                }

            } else {
                AiLog log("Error in path transcription.");

                ground_path = nullptr;
                return ground_path;
            }
        }
    }
}
