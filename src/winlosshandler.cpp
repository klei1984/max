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

#include "winlosshandler.hpp"

#include "scripter.hpp"

WinLossHandler::WinLossHandler() {}

WinLossHandler::~WinLossHandler() {}

bool WinLossHandler::LoadScript(const Mission& mission) {
    bool result;

    if (m_interpreter) {
        m_script.clear();
        Scripter::DestroyContext(m_interpreter);
    }

    m_interpreter = Scripter::CreateContext(Scripter::WINLOSS_CONDITIONS);

    if (m_interpreter) {
        (void)Scripter::SetTimeBudget(m_interpreter);

        if (mission.HasWinLossConditions()) {
            m_script = mission.GetWinLossConditions();

        } else {
            m_script = Scripter::LoadDefaultWinLossConditions();
        }

        result = true;

    } else {
        result = false;
    }

    return result;
}

template <typename T>
static inline T WinLossHandler_GetElement(Scripter::ScriptParameters parameters, int64_t index) {
    SDL_assert(std::holds_alternative<T>(parameters[index]));

    return std::get<T>(parameters[index]);
}

bool WinLossHandler::TestWinLossConditions(const int64_t team, WinLossState& state) {
    Scripter::ScriptParameters results;
    std::string error;
    bool result;

    SDL_assert(m_interpreter);

    if (m_interpreter) {
        results.push_back(static_cast<int64_t>(VICTORY_STATE_GENERIC));

        if (Scripter::RunScript(m_interpreter, m_script, {team}, results, &error)) {
            SDL_assert(results.size() == 1);

            state = static_cast<WinLossState>(WinLossHandler_GetElement<int64_t>(results, 0));

            result = true;

        } else {
            SDL_Log("\n%s\n", error.c_str());
            SDL_assert(0);

            result = false;
        }

    } else {
        result = false;
    }

    return result;
}
