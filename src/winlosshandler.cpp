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

bool WinLossHandler::LoadScripts(const Mission& mission) {
    if (m_interpreter) {
        Scripter::DestroyContext(m_interpreter);

        if (mission.HasVictoryConditions()) {
            m_win_script = mission.GetVictoryConditions();

        } else {
            m_win_script = "return false";
        }

        if (mission.HasDefeatConditions()) {
            m_loss_script = mission.GetDefeatConditions();

        } else {
            m_loss_script = "return false";
        }

        m_interpreter = Scripter::CreateContext(Scripter::WINLOSS_CONDITIONS);
    }

    return (m_interpreter != nullptr);
}

template <typename T>
static inline T WinLossHandler_GetElement(Scripter::ScriptParameters parameters, size_t index) {
    SDL_assert(std::holds_alternative<T>(parameters[index]));

    return std::get<T>(parameters[index]);
}

bool WinLossHandler::TestWinCriteria(const size_t team) {
    Scripter::ScriptParameters results;
    std::string error;
    bool result;

    results.push_back(static_cast<bool>(false));

    if (Scripter::RunScript(m_interpreter, m_win_script, {team}, results, &error)) {
        SDL_assert(results.size() == 1);

        result = WinLossHandler_GetElement<bool>(results, 0);

    } else {
        SDL_Log("%s", error.c_str());
        SDL_assert(0);

        result = false;
    }

    return result;
}

bool WinLossHandler::TestLossCriteria(const size_t team) {
    Scripter::ScriptParameters results;
    std::string error;
    bool result;

    results.push_back(static_cast<bool>(false));

    if (Scripter::RunScript(m_interpreter, m_win_script, {team}, results, &error)) {
        SDL_assert(results.size() == 1);

        result = WinLossHandler_GetElement<bool>(results, 0);

    } else {
        SDL_Log("%s", error.c_str());
        SDL_assert(0);

        result = false;
    }

    return result;
}
