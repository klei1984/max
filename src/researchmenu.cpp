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

#include "researchmenu.hpp"

const char* const ResearchMenu_TopicLabels[] = {"Attack", "Shots", "Range", "Armor", "Hits", "Speed", "Scan", "Cost"};

const ResourceID ResearchMenu_TopicIcon[] = {I_HRDATK, I_SHOTS, I_RANGE, I_ARMOR, I_HITS, I_SPEED, I_SCAN, I_GOLD};

void ResearchMenu_Menu(UnitInfo* unit) {
    /// \todo
}

void ResearchMenu_CalculateResearchCost(unsigned short team, int research_topic, int allocation) {
    /// \todo
}

void ResearchMenu_NewTurn(unsigned short team) {
    /// \todo
}

int ResearchMenu_CalculateFactor(unsigned short team, int research_topic, ResourceID unit_type) {
    /// \todo
    return 0;
}
