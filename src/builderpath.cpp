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

#include "builderpath.hpp"

#include "game_manager.hpp"
#include "paths.hpp"
#include "registerarray.hpp"
#include "remote.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "unitinfo.hpp"

static uint32_t Paths_BuilderPath_TypeIndex;
static RegisterClass Paths_BuilderPath_ClassRegister("BuilderPath", &Paths_BuilderPath_TypeIndex,
                                                     &BuilderPath::Allocate);

BuilderPath::BuilderPath() : UnitPath(0, 0), m_direction_x(1), m_direction_y(1) {}

BuilderPath::~BuilderPath() {}

FileObject* BuilderPath::Allocate() noexcept { return new (std::nothrow) BuilderPath(); }

uint32_t BuilderPath::GetTypeIndex() const { return Paths_BuilderPath_TypeIndex; }

void BuilderPath::FileLoad(SmartFileReader& file) noexcept {
    file.Read(m_direction_x);
    file.Read(m_direction_y);
}

void BuilderPath::FileSave(SmartFileWriter& file) noexcept {
    file.Write(m_direction_x);
    file.Write(m_direction_y);
}

int32_t BuilderPath::GetMovementCost(UnitInfo* unit) { return SHRT_MAX; }

bool BuilderPath::Execute(UnitInfo* unit) {
    bool result;
    int32_t direction;

    if (ResourceManager_GetSettings()->GetNumericValue("effects")) {
        unit->RefreshScreen();
    }

    if (Remote_IsNetworkGame || unit->IsVisibleToTeam(GameManager_PlayerTeam)) {
        m_direction_x = DIRECTION_OFFSETS[(unit->angle + 2) & 7].x;
        m_direction_y = DIRECTION_OFFSETS[(unit->angle + 2) & 7].y;

        direction = Paths_GetAngle(m_direction_x, m_direction_y);

        Paths_UpdateAngle(unit, direction);

        /// \todo Is this a defect? Paths_UpdateAngle() returns the correct status
        result = false;

    } else {
        result = false;
    }

    return result;
}

void BuilderPath::Draw(UnitInfo* unit, WindowInfo* window) {}
