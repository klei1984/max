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

#include "gameruleshandler.hpp"

#include "resource_manager.hpp"
#include "scripter.hpp"
#include "units_manager.hpp"

GameRulesHandler::GameRulesHandler() {}

GameRulesHandler::~GameRulesHandler() {}

bool GameRulesHandler::LoadScript(const Mission& mission) {
    bool result;

    if (m_interpreter) {
        m_game_rules_script.clear();
        Scripter::DestroyContext(m_interpreter);
    }

    m_interpreter = Scripter::CreateContext(Scripter::GAME_RULES);

    if (m_interpreter) {
        Scripter::ScriptParameters arguments;
        Scripter::ScriptParameters results;
        std::string error;
        std::string script = Scripter::LoadDefaultGameRules();

        if (Scripter::RunScript(static_cast<void*>(m_interpreter), script, arguments, results, &error)) {
            if (mission.HasGameRules()) {
                arguments.clear();
                results.clear();
                error.clear();

                (void)Scripter::SetTimeBudget(m_interpreter);

                m_game_rules_script = mission.GetGameRules();

                if (Scripter::RunScript(m_interpreter, m_game_rules_script, arguments, results, &error)) {
                    result = true;

                } else {
                    SDL_Log("\n%s\n", error.c_str());
                    SDL_assert(0);

                    result = false;
                }

            } else {
                result = true;
            }

            if (result == true) {
                if (!InitClanLoadoutRules()) {
                    result = false;
                }
            }

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

ResourceID GameRulesHandler::GetBuilderType(const uint16_t team, const ResourceID unit_type) {
    Scripter::ScriptParameters args{static_cast<int64_t>(team), static_cast<int64_t>(unit_type)};
    Scripter::ScriptParameters results{static_cast<int64_t>(0)};
    std::string error;
    ResourceID result{INVALID_ID};

    SDL_assert(m_interpreter);

    if (m_interpreter) {
        if (Scripter::RunScript(m_interpreter, "return max_get_builder_type(...)", args, results, &error)) {
            if (results.size() == 1) {
                auto builder_type = static_cast<ResourceID>(std::get<int64_t>(results[0]));

                switch (builder_type) {
                    case CONSTRCT:
                    case ENGINEER:
                    case LIGHTPLT:
                    case LANDPLT:
                    case AIRPLT:
                    case SHIPYARD:
                    case TRAINHAL: {
                        result = builder_type;
                    } break;
                }
            }

        } else {
            SDL_Log("\n%s\n", error.c_str());
        }
    }

    return result;
}

bool GameRulesHandler::IsBuildable(const uint16_t team, const ResourceID unit_type) {
    Scripter::ScriptParameters args{static_cast<int64_t>(team), static_cast<int64_t>(unit_type)};
    Scripter::ScriptParameters results{false};
    std::string error;
    ResourceID result{false};

    SDL_assert(m_interpreter);

    if (m_interpreter) {
        if (Scripter::RunScript(m_interpreter, "return max_is_buildable(...)", args, results, &error)) {
            if (results.size() == 1) {
                result = static_cast<ResourceID>(std::get<bool>(results[0]));
            }

        } else {
            SDL_Log("\n%s\n", error.c_str());
        }
    }

    return result;
}

std::vector<ResourceID> GameRulesHandler::GetBuildableUnits(const uint16_t team, const ResourceID unit_type) {
    std::vector<ResourceID> result;

    Scripter::ScriptParameters args{static_cast<int64_t>(team), static_cast<int64_t>(unit_type)};
    Scripter::ScriptParameters results{Scripter::ScriptTable{}};
    std::string error;

    SDL_assert(m_interpreter);

    if (m_interpreter) {
        if (Scripter::RunScript(m_interpreter, "return max_get_buildable_units(...)", args, results, &error)) {
            if (results.size() == 1) {
                const auto table = std::get<Scripter::ScriptTable>(results[0]);

                for (const auto& value : table) {
                    const ResourceID buildable_unit_type = static_cast<ResourceID>(std::get<int64_t>(value));

                    result.push_back(buildable_unit_type);
                }
            }

        } else {
            SDL_Log("\n%s\n", error.c_str());
        }
    }

    return result;
}

bool GameRulesHandler::InitClanLoadoutRules() {
    auto clans = ResourceManager_GetClans();

    if (clans) {
        for (const auto& clan_id : clans->GetClanKeys()) {
            if (clans->HasLoadoutRules(clan_id)) {
                const std::string clan_script = clans->GetLoadoutRules(clan_id);
                Scripter::ScriptParameters arguments;
                Scripter::ScriptParameters results;
                std::string error;

                if (!Scripter::RunScript(m_interpreter, clan_script, arguments, results, &error)) {
                    SDL_Log("\n%s\n", error.c_str());

                    return false;
                }
            }
        }
    }

    return true;
}

bool GameRulesHandler::GetMissionLoadout(const uint16_t team, const std::string& clan_id,
                                         TeamMissionSupplies& supplies) {
    bool result = false;

    SDL_assert(m_interpreter);

    if (m_interpreter) {
        Scripter::ScriptParameters args{static_cast<int64_t>(team), static_cast<std::string>(clan_id),
                                        static_cast<int64_t>(supplies.team_gold)};

        Scripter::ScriptParameters script_results{Scripter::ScriptTable{}, Scripter::ScriptTable{}};
        std::string error;

        if (Scripter::RunScript(m_interpreter, "return max_get_mission_loadout(...)", args, script_results, &error)) {
            if (script_results.size() == 2) {
                const auto units_table = std::get<Scripter::ScriptTable>(script_results[0]);
                const auto cargos_table = std::get<Scripter::ScriptTable>(script_results[1]);

                for (const auto& value : units_table) {
                    const ResourceID unit_type = static_cast<ResourceID>(std::get<int64_t>(value));
                    supplies.units.PushBack(&unit_type);
                }

                for (const auto& value : cargos_table) {
                    const uint16_t cargo = static_cast<uint16_t>(std::get<int64_t>(value));
                    supplies.cargos.PushBack(&cargo);
                }

                result = true;
            }

        } else {
            SDL_Log("\n%s\n", error.c_str());
        }
    }

    return result;
}
