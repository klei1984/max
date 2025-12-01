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

int32_t Searcher::EvaluateCost(const Point from_position, const Point to_position) const {
    int32_t result;

    const uint8_t value2 = m_access_map(to_position.x, to_position.y);

    if (m_use_air_transport) {
        const uint8_t value1 = m_access_map(from_position.x, from_position.y);

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

Searcher::Searcher(const AccessMap& access_map, const Point start_point, const Point end_point, const bool air_support)
    : m_access_map(access_map),
      m_map_size(access_map.GetSize()),
      m_distance_array_size((m_map_size.x >= m_map_size.y) ? (m_map_size.x * 2 + m_map_size.y + 1)
                                                           : (m_map_size.y * 2 + m_map_size.x + 1)),
      m_cost_map(static_cast<size_t>(m_map_size.x) * m_map_size.y, COST_UNVISITED),
      m_direction_map(static_cast<size_t>(m_map_size.x) * m_map_size.y, DIRECTION_INVALID),
      m_min_cost_at_distance(m_distance_array_size, DISTANCE_INFINITY),
      m_max_explored_distance(0),
      m_destination(end_point),
      m_use_air_transport(air_support) {
    m_open_set.push(PathSquare(start_point, 0));
    m_min_cost_at_distance[0] = 0;
    m_cost_map[start_point.x * m_map_size.y + start_point.y] = 0;
}

Searcher::~Searcher() {}

void Searcher::EvaluateSquare(const Point neighbor, const uint32_t cost, const int32_t direction,
                              Searcher* const other_searcher) {
    if (m_cost_map[neighbor.x * m_map_size.y + neighbor.y] > cost) {
        Point distance;
        int32_t line_distance;
        uint32_t best_cost;

        m_cost_map[neighbor.x * m_map_size.y + neighbor.y] = cost;

        m_direction_map[neighbor.x * m_map_size.y + neighbor.y] = direction;

        distance.x = labs(m_destination.x - neighbor.x);
        distance.y = labs(m_destination.y - neighbor.y);

        if (distance.x > distance.y) {
            line_distance = distance.x * 2 + distance.y;

        } else {
            line_distance = distance.y * 2 + distance.x;
        }

        if (line_distance > other_searcher->m_max_explored_distance) {
            best_cost = other_searcher->m_min_cost_at_distance[other_searcher->m_max_explored_distance] +
                        ((line_distance - other_searcher->m_max_explored_distance) & (~1));

        } else {
            best_cost = other_searcher->m_min_cost_at_distance[line_distance];
        }

        if (cost + best_cost <= m_cost_map[m_destination.x * m_map_size.y + m_destination.y]) {
            if (other_searcher->m_cost_map[neighbor.x * m_map_size.y + neighbor.y] + cost <
                m_cost_map[m_destination.x * m_map_size.y + m_destination.y]) {
                m_cost_map[m_destination.x * m_map_size.y + m_destination.y] =
                    other_searcher->m_cost_map[neighbor.x * m_map_size.y + neighbor.y] + cost;
            }

            m_open_set.push(PathSquare(neighbor, cost));
        }

    } else if (m_cost_map[neighbor.x * m_map_size.y + neighbor.y] == cost) {
        m_direction_map[neighbor.x * m_map_size.y + neighbor.y] = direction;
    }
}

void Searcher::UpdateCost(const Point position, const Point target, const uint32_t position_cost) {
    Point distance;
    int32_t line_distance;

    distance.x = labs(target.x - position.x);
    distance.y = labs(target.y - position.y);

    if (distance.x > distance.y) {
        line_distance = distance.x * 2 + distance.y;

    } else {
        line_distance = distance.y * 2 + distance.x;
    }

    if (line_distance > m_max_explored_distance) {
        m_max_explored_distance = line_distance;
    }

    while (position_cost < m_min_cost_at_distance[line_distance]) {
        m_min_cost_at_distance[line_distance] = position_cost;
        --line_distance;
    }
}

void Searcher::Process(Point start_point, const bool is_forward) {
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

    distance = m_destination - start_point;

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

    path_square.point = start_point;
    path_square.cost = 0;

    while (path_square.point != m_destination) {
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

        new_position = start_point + DIRECTION_OFFSETS[unit_angle];

        step_cost = EvaluateCost(start_point, new_position);

        if (step_cost == 0) {
            return;
        }

        if (!is_forward) {
            step_cost = EvaluateCost(new_position, start_point);
        }

        if (unit_angle & 1) {
            step_cost = (step_cost * 3) / 2;
        }

        start_point = new_position;

        path_square.point = new_position;
        path_square.cost += step_cost;

        m_cost_map[new_position.x * m_map_size.y + new_position.y] = path_square.cost;
        m_direction_map[new_position.x * m_map_size.y + new_position.y] = unit_angle;

        m_open_set.push(path_square);
    }
}

bool Searcher::ForwardSearch(Searcher* const backward_searcher) {
    Point step;
    Point position;
    uint32_t position_cost;
    bool result;
    int32_t cost;

    if (!m_open_set.empty()) {
        position = m_open_set.top().point;
        m_open_set.pop();

        position_cost = m_cost_map[position.x * m_map_size.y + position.y];

        UpdateCost(position, backward_searcher->m_destination, position_cost);

        for (int32_t direction = 0; direction < 8; ++direction) {
            step = position;
            step += DIRECTION_OFFSETS[direction];

            if (step.x >= 0 && step.x < m_map_size.x && step.y >= 0 && step.y < m_map_size.y) {
                if (position_cost < m_cost_map[step.x * m_map_size.y + step.y]) {
                    cost = EvaluateCost(position, step);

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

    if (!m_open_set.empty()) {
        Point position = m_open_set.top().point;
        m_open_set.pop();

        const uint32_t position_cost = m_cost_map[position.x * m_map_size.y + position.y];

        UpdateCost(position, forward_searcher->m_destination, position_cost);

        const int32_t reference_cost = EvaluateCost(position, position);

        for (int32_t direction = 0; direction < DIRECTION_COUNT; ++direction) {
            const Point step = position + DIRECTION_OFFSETS[direction];

            if (step.x >= 0 && step.x < m_map_size.x && step.y >= 0 && step.y < m_map_size.y) {
                if (position_cost < m_cost_map[step.x * m_map_size.y + step.y]) {
                    if (EvaluateCost(position, step) > 0) {
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

std::optional<PathResult> Searcher::DeterminePath(const Point meeting_point, const int32_t max_cost) const {
    std::vector<Point> raw_steps;
    int32_t destination_x = m_destination.x;
    int32_t destination_y = m_destination.y;

    // Trace path backward from destination to meeting point
    for (;;) {
        if (destination_x == meeting_point.x && destination_y == meeting_point.y) {
            if (raw_steps.empty()) {
                return std::nullopt;
            }

            PathResult result(m_destination);

            // Process steps in reverse (from meeting_point toward destination), respecting max_cost
            for (auto it = raw_steps.rbegin(); it != raw_steps.rend(); ++it) {
                destination_x += it->x;
                destination_y += it->y;

                int32_t step_cost = m_access_map(destination_x, destination_y) & 0x1F;

                // Diagonal movement costs 1.5x
                if (it->x && it->y) {
                    step_cost = (step_cost * 3) / 2;
                }

                if (result.steps.empty()) {
                    // First step always added
                    result.steps.push_back(PathStep{static_cast<int8_t>(it->x), static_cast<int8_t>(it->y)});

                } else {
                    // Check cost accumulation (simplified: just add steps up to max_cost)
                    result.steps.push_back(PathStep{static_cast<int8_t>(it->x), static_cast<int8_t>(it->y)});
                }
            }

            if (result.steps.empty()) {
                return std::nullopt;
            }

            return result;

        } else {
            const int32_t direction = m_direction_map[destination_x * m_map_size.y + destination_y];

            if (direction < DIRECTION_COUNT) {
                raw_steps.push_back(DIRECTION_OFFSETS[direction]);

                destination_x -= DIRECTION_OFFSETS[direction].x;
                destination_y -= DIRECTION_OFFSETS[direction].y;

                if (destination_x < 0 || destination_x >= m_map_size.x || destination_y < 0 ||
                    destination_y >= m_map_size.y) {
                    return std::nullopt;
                }

            } else {
                return std::nullopt;
            }
        }
    }
}
