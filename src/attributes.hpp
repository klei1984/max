/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef ATTRIBUTES_HPP
#define ATTRIBUTES_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

struct UnitAttributes {
    uint32_t turns_to_build;
    uint32_t hit_points;
    uint32_t armor_rating;
    uint32_t attack_rating;
    uint32_t move_and_fire;
    uint32_t movement_points;
    uint32_t attack_range;
    uint32_t shots_per_turn;
    uint32_t scan_range;
    uint32_t storage_capacity;
    uint32_t ammunition;
    uint32_t blast_radius;
};

class Attributes {
    std::unique_ptr<std::unordered_map<std::string, UnitAttributes>> m_attributes;

    [[nodiscard]] bool LoadScript(const std::string& script);
    [[nodiscard]] std::string LoadSchema();

public:
    Attributes();
    ~Attributes();

    [[nodiscard]] bool LoadFile(const std::string& path);
    [[nodiscard]] bool LoadResource();
    [[nodiscard]] bool GetUnitAttributes(const std::string unit_id, UnitAttributes* const attributes);
};

#endif /* ATTRIBUTES_HPP */
