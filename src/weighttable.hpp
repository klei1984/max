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

#ifndef UNITWEIGHT_HPP
#define UNITWEIGHT_HPP

#include <vector>

#include "resource_manager.hpp"

/**
 * \struct UnitWeight
 * \brief Entry in a WeightTable representing a unit type and its selection probability weight.
 */
struct UnitWeight {
    ResourceID unit_type{INVALID_ID};
    uint16_t weight{0};

    UnitWeight() = default;
    UnitWeight(ResourceID unit_type_, uint16_t weight_) : unit_type(unit_type_), weight(weight_) {}
};

/**
 * \class WeightTable
 * \brief A table of unit types with associated probability weights for AI unit selection.
 *
 * Used by AI systems to randomly select unit types based on weighted probabilities. The weight value determines the
 * relative likelihood of a unit type being selected - a unit with weight 3 is three times more likely to be chosen
 * than a unit with weight 1.
 *
 * The table maintains a team association for buildability validation. When adding entries via Add(), the table
 * automatically validates that the unit type is buildable by the associated team using the current game rules.
 *
 * Entries with weight 0 are excluded from random selection via RollUnitType() but remain in the table until explicitly
 * removed via Compact() or Disable().
 */
class WeightTable {
    std::vector<UnitWeight> m_entries;
    uint16_t m_team{PLAYER_TEAM_RED};

public:
    WeightTable() = default;

    /**
     * \brief Constructs a WeightTable with the specified team association.
     *
     * \param team The team ID used for buildability validation in Add().
     */
    explicit WeightTable(uint16_t team) : m_team(team) {}

    /**
     * \brief Copy constructor.
     *
     * \param other The source WeightTable to copy from.
     */
    WeightTable(const WeightTable& other) = default;

    ~WeightTable() = default;

    WeightTable& operator=(WeightTable const& other);
    WeightTable& operator+=(WeightTable const& other);

    /**
     * \brief Accesses entry at the specified index.
     *
     * \param position Zero-based index of the entry to access.
     * \return Reference to the UnitWeight entry at the specified position.
     */
    UnitWeight& operator[](uint32_t position) { return m_entries[position]; }

    /**
     * \brief Accesses entry at the specified index (const version).
     *
     * \param position Zero-based index of the entry to access.
     * \return Const reference to the UnitWeight entry at the specified position.
     */
    const UnitWeight& operator[](uint32_t position) const { return m_entries[position]; }

    /**
     * \brief Sets the team for buildability validation in Add().
     *
     * \param team The team ID to associate with this table.
     */
    void SetTeam(uint16_t team) { m_team = team; }

    /**
     * \brief Gets the team associated with this table.
     *
     * \return The team ID used for buildability validation.
     */
    [[nodiscard]] uint16_t GetTeam() const { return m_team; }

    /**
     * \brief Adds a unit type with weight if buildable by the associated team.
     *
     * Validates that the unit type is buildable using the current game rules before adding.
     *
     * \param unit_type The resource ID of the unit type to add.
     * \param weight The selection probability weight for this unit type.
     * \return True if the entry was added, false if the unit type is not buildable.
     */
    bool Add(ResourceID unit_type, uint16_t weight);

    /**
     * \brief Adds a pre-validated entry without buildability checking.
     *
     * Use this method when the entry has already been validated or when copying from another table.
     *
     * \param entry The UnitWeight entry to add.
     */
    void PushBack(const UnitWeight& entry);

    /**
     * \brief Removes entry at index using swap-and-pop for O(1) removal.
     *
     * This method swaps the target entry with the last entry and removes the last entry, which changes the order of
     * remaining entries but provides constant-time removal.
     *
     * \param index Zero-based index of the entry to remove.
     */
    void Disable(uint32_t index);

    /**
     * \brief Removes all entries with weight == 0 or unit_type == INVALID_ID.
     *
     * Call this method to clean up disabled entries and reduce memory usage.
     */
    void Compact();

    /**
     * \brief Removes all entries from the table.
     *
     * The team association is preserved.
     */
    void Clear();

    /**
     * \brief Returns the number of entries in the table.
     *
     * \return The count of entries, including those with weight 0.
     */
    [[nodiscard]] uint32_t GetCount() const { return static_cast<uint32_t>(m_entries.size()); }

    /**
     * \brief Randomly selects a unit type based on weights.
     *
     * Performs weighted random selection where each entry's probability is proportional to its weight relative to the
     * total weight of all entries. Entries with weight 0 are never selected.
     *
     * \return The selected unit type, or INVALID_ID if no valid entries exist.
     */
    [[nodiscard]] ResourceID RollUnitType() const;

    /**
     * \brief Gets the weight for a specific unit type.
     *
     * \param unit_type The resource ID of the unit type to look up.
     * \return The weight of the specified unit type, or 0 if not found.
     */
    [[nodiscard]] int32_t GetWeight(ResourceID unit_type) const;
};

#endif /* UNITWEIGHT_HPP */
