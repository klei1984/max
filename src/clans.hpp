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

#ifndef CLANS_HPP
#define CLANS_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "resourcetable.hpp"

struct ClanResource {
    std::filesystem::path file;
    ResourceID id;
};

struct ClanMedia {
    ClanResource resource;
    std::string copyright;
    std::string license;
};

struct ClanScriptBlock {
    std::string script;
};

using ClanTextBlock = std::unordered_map<std::string, std::string>;

struct UnitTradedoffs {
    int32_t turns_to_build;
    int32_t hit_points;
    int32_t armor_rating;
    int32_t attack_rating;
    int32_t move_and_fire;
    int32_t movement_points;
    int32_t attack_range;
    int32_t shots_per_turn;
    int32_t scan_range;
    int32_t storage_capacity;
    int32_t ammunition;
    int32_t blast_radius;
    int32_t experience;
};

struct ClanObject {
    std::string script;
    std::string author;
    std::string copyright;
    std::string license;
    ClanTextBlock name;
    ClanTextBlock description;
    ClanMedia logo;
    ClanScriptBlock loadout_rules;
    int32_t credits;
    std::unordered_map<ResourceID, UnitTradedoffs> tradeoffs;
};

class Clans {
    std::unique_ptr<std::unordered_map<std::string, ClanObject>> m_clans;
    std::string& m_language;

    [[nodiscard]] bool LoadScript(const std::string& script);
    [[nodiscard]] std::string LoadSchema();

public:
    using ResourceType = std::variant<ResourceID, std::filesystem::path>;

    enum AttributeID : uint8_t {
        ATTRIB_TURNS_TO_BUILD,
        ATTRIB_HIT_POINTS,
        ATTRIB_ARMOR_RATING,
        ATTRIB_ATTACK_RATING,
        ATTRIB_MOVE_AND_FIRE,
        ATTRIB_MOVEMENT_POINTS,
        ATTRIB_ATTACK_RANGE,
        ATTRIB_SHOTS_PER_TURN,
        ATTRIB_SCAN_RANGE,
        ATTRIB_STORAGE_CAPACITY,
        ATTRIB_AMMUNITION,
        ATTRIB_BLAST_RADIUS,
        ATTRIB_EXPERIENCE,
        ATTRIB_COUNT,
    };

    Clans();
    ~Clans();

    [[nodiscard]] bool LoadFile(const std::string& path);
    [[nodiscard]] bool LoadResource();

    void SetLanguage(const std::string& language);

    [[nodiscard]] bool HasLoadoutRules(const std::string& clan_id) const;
    [[nodiscard]] std::string GetLoadoutRules(const std::string& clan_id) const;
    [[nodiscard]] std::string GetName(const std::string& clan_id) const;
    [[nodiscard]] std::string GetDescription(const std::string& clan_id) const;
    [[nodiscard]] int32_t GetCredits(const std::string& clan_id) const;
    [[nodiscard]] int32_t GetUnitTradeoff(const std::string& clan_id, const ResourceID resource_id,
                                          const AttributeID attribute) const;
    [[nodiscard]] ResourceType GetLogo(const std::string& clan_id) const;

    class KeyIterator {
        using MapIterator = std::unordered_map<std::string, ClanObject>::const_iterator;
        MapIterator m_iter;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::string;
        using difference_type = std::ptrdiff_t;
        using pointer = const std::string*;
        using reference = const std::string&;

        explicit KeyIterator(MapIterator iter);

        reference operator*() const;
        pointer operator->() const;

        KeyIterator& operator++();
        KeyIterator operator++(int);

        friend bool operator==(const KeyIterator& a, const KeyIterator& b);
        friend bool operator!=(const KeyIterator& a, const KeyIterator& b);
    };

    class KeyRange {
        const std::unordered_map<std::string, ClanObject>* m_map;

    public:
        explicit KeyRange(const std::unordered_map<std::string, ClanObject>* map);

        KeyIterator begin() const;
        KeyIterator end() const;
    };

    [[nodiscard]] KeyRange GetClanKeys() const { return KeyRange(m_clans.get()); }
};

#endif /* CLANS_HPP */
