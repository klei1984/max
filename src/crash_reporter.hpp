/* Copyright (c) 2026 M.A.X. Port Team
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

#ifndef CRASH_REPORTER_HPP
#define CRASH_REPORTER_HPP

#include <cstdint>
#include <filesystem>

struct CrashContext {
    char phase[48];
    char mission[96];
    char mission_kind[32];
    char mission_hash[48];
    char save_file[64];

    int32_t turn;
    int32_t game_state;
    int32_t play_mode;
    int32_t player_team;
    int32_t active_team;
    int32_t human_players;
    int32_t save_slot;
    int32_t victory_type;
    int32_t victory_limit;
    int32_t opponent;
    int32_t world_index;  // only index is saved as worlds are not externalized yet

    bool real_time;
    bool cheater;
    bool valid;
};

using CrashContextProvider = void (*)(CrashContext& context);

/**
 * \brief Install the crash handlers and prepare the report path.
 *
 * Everything that needs to allocate, format or touch the file system is done
 * here so the handler itself only has to write bytes it already holds. Call as
 * early as the preference path is known.
 *
 * \param pref_path Directory holding the user's settings and logs.
 * \return True when handlers are installed. False leaves the process running
 *         without crash reporting, which is not fatal.
 */
bool CrashReporter_Init(const std::filesystem::path& pref_path);

/**
 * \brief Register the single function allowed to read game globals.
 *
 * Keeping this a callback rather than a push API means game state stays where
 * it already lives and only one translation unit, crash_context.cpp, has to
 * know about it.
 */
void CrashReporter_SetContextProvider(CrashContextProvider provider);

/** \brief Restore the default handlers. Safe to call without a prior Init(). */
void CrashReporter_Shutdown();

/**
 * \brief Multi line build and system description, as it appears in a report.
 *
 * Built once during Init(). The session log prints the same block on startup
 * so a log and a report can always be matched to the same build.
 */
const char* CrashReporter_GetSystemInfo();

/** \brief Path a report would be written to, valid after Init(). */
const std::filesystem::path& CrashReporter_GetReportPath();

/** \brief Path of the DWARF source used for symbolisation, for diagnostics. */
const std::filesystem::path& CrashReporter_GetSymbolPath();

#endif /* CRASH_REPORTER_HPP */
