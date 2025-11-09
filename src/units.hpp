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

#ifndef UNITS_HPP
#define UNITS_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "unit.hpp"

class Units {
    std::unique_ptr<std::unordered_map<std::string, Unit*>> m_units;

    [[nodiscard]] bool LoadScript(const std::string& script);
    [[nodiscard]] std::string LoadSchema();

public:
    Units();
    ~Units();

    [[nodiscard]] bool LoadFile(const std::string& path);
    [[nodiscard]] bool LoadResource();
    [[nodiscard]] const Unit* GetUnit(const std::string& unit_id) const;

    template <typename MapIterator>
    class IteratorWrapper {
        MapIterator m_iter;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<const std::string&, Unit&>;
        using pointer = value_type*;
        using reference = value_type;

        explicit IteratorWrapper(MapIterator iter) : m_iter(iter) {}

        reference operator*() const { return {m_iter->first, *m_iter->second}; }

        IteratorWrapper& operator++() {
            ++m_iter;
            return *this;
        }

        IteratorWrapper operator++(int) {
            IteratorWrapper tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const IteratorWrapper& other) const { return m_iter == other.m_iter; }
        bool operator!=(const IteratorWrapper& other) const { return m_iter != other.m_iter; }
    };

    using iterator = IteratorWrapper<std::unordered_map<std::string, Unit*>::iterator>;
    using const_iterator = IteratorWrapper<std::unordered_map<std::string, Unit*>::const_iterator>;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;
};

#endif /* UNITS_HPP */
