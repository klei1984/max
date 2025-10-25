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

#ifndef SCRIPTER_HPP
#define SCRIPTER_HPP

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace Scripter {
enum ScriptType : uint8_t {
    WINLOSS_CONDITIONS,
    GAME_RULES,
    GAME_EVENTS,
    GAME_MUSIC,
};

using ScriptTable = std::vector<std::variant<bool, int64_t, double, std::string>>;
using ScriptParameters = std::vector<std::variant<bool, int64_t, double, std::string, ScriptTable>>;

void Init();

bool TestScript(const std::string script, std::string* error = nullptr);
bool RunScript(void* const handle, const std::string script, const ScriptParameters& args, ScriptParameters& results,
               std::string* error = nullptr);
bool SetTimeBudget(void* const handle, const uint64_t time_budget = 0uLL);
std::string LoadDefaultGameRules();
std::string LoadDefaultWinLossConditions();
void* CreateContext(const ScriptType type);
void DestroyContext(void* handle);

using MaxRegistryFunctionType = std::variant<bool, int64_t, double, std::string> (*)();

void MaxRegistryRegister(const std::string key, MaxRegistryFunctionType fn);
void MaxRegistryRemove(const std::string key);
void MaxRegistryReset();
void MaxRegistryUpdate();

}  // namespace Scripter

#endif /* SCRIPTER_HPP */
