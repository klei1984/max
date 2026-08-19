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

#include "crash_reporter.hpp"

#include <SDL3/SDL.h>
#include <unwind.h>

#include <algorithm>
#include <cerrno>
#include <cinttypes>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <system_error>
#include <vector>

#include "backtrace-supported.h"
#include "backtrace.h"

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#if defined(__linux__)
#include <link.h>
#endif
#endif

#include <cxxabi.h>

/* Constraints for the reporter:
 *   - no allocation, locking or C++ streams inside the handler,
 *   - raw write(2) only, because it is the one output call the standard
 *     guarantees from a signal handler,
 *   - everything expensive is precomputed during CrashReporter_Init().
 *
 * The two exceptions are the game context provider and the trace library that
 * are protected.
 */

namespace {

constexpr int32_t kMaxFrames = 64;
constexpr int32_t kMaxReportsKept = 10;
constexpr size_t kSystemInfoSize = 2048;
constexpr size_t kPathSize = 1024;

std::filesystem::path CrashReporter_ReportPath;
std::filesystem::path CrashReporter_SymbolPath;
char CrashReporter_SystemInfo[kSystemInfoSize];
char CrashReporter_ReportPathUtf8[kPathSize];
char CrashReporter_SymbolPathUtf8[kPathSize];
char CrashReporter_ModuleName[128];

uintptr_t CrashReporter_ModuleBase;
uintptr_t CrashReporter_ModuleSize;
uintptr_t CrashReporter_SymbolSlide;

uint64_t CrashReporter_StartCounter;

CrashContextProvider CrashReporter_Provider;
bool CrashReporter_Installed;

volatile std::sig_atomic_t CrashReporter_InHandler;

#if defined(_WIN32)
PVOID CrashReporter_VectoredHandle;
volatile std::sig_atomic_t CrashReporter_ExceptionCode;
uintptr_t CrashReporter_FaultAddress;
#else
void* CrashReporter_FaultAddress;
char CrashReporter_AltStack[64u * 1024u];
#endif

#if defined(_WIN32)
int CrashReporter_OpenReport(const char* path) {
    return _open(path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
}

void CrashReporter_WriteRaw(int fd, const char* data, size_t length) {
    if (fd >= 0 && length > 0) {
        (void)_write(fd, data, static_cast<unsigned int>(length));
    }
}

void CrashReporter_CloseReport(int fd) {
    if (fd >= 0) {
        (void)_close(fd);
    }
}
#else
int CrashReporter_OpenReport(const char* path) { return open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644); }

void CrashReporter_WriteRaw(int fd, const char* data, size_t length) {
    /* write(2) is allowed to do less than asked even for regular files, and a
     * signal arriving mid call turns into EINTR, so both have to be looped.
     */
    while (fd >= 0 && length > 0) {
        const ssize_t written = write(fd, data, length);

        if (written > 0) {
            data += written;
            length -= static_cast<size_t>(written);

        } else if (written < 0 && errno == EINTR) {
            continue;

        } else {
            break;
        }
    }
}

void CrashReporter_CloseReport(int fd) {
    if (fd >= 0) {
        (void)close(fd);
    }
}
#endif

void CrashReporter_Write(int fd, const char* text) {
    if (text) {
        CrashReporter_WriteRaw(fd, text, std::strlen(text));
    }
}

void CrashReporter_WriteSigned(int fd, int64_t value) {
    char buffer[24];
    size_t index = sizeof(buffer);
    const bool negative = value < 0;
    uint64_t magnitude = negative ? (~static_cast<uint64_t>(value) + 1u) : static_cast<uint64_t>(value);

    do {
        buffer[--index] = static_cast<char>('0' + (magnitude % 10u));
        magnitude /= 10u;
    } while (magnitude > 0u && index > 1u);

    if (negative) {
        buffer[--index] = '-';
    }

    CrashReporter_WriteRaw(fd, &buffer[index], sizeof(buffer) - index);
}

void CrashReporter_WriteHex(int fd, uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    char buffer[16];
    size_t index = sizeof(buffer);

    do {
        buffer[--index] = digits[value & 0xFu];
        value >>= 4u;
    } while (value > 0u && index > 0u);

    CrashReporter_Write(fd, "0x");
    CrashReporter_WriteRaw(fd, &buffer[index], sizeof(buffer) - index);
}

struct CrashReporter_Frames {
    uintptr_t address[kMaxFrames];
    int32_t count;
};

bool CrashReporter_IsOwnModule(uintptr_t address) {
    return CrashReporter_ModuleSize != 0u && address >= CrashReporter_ModuleBase &&
           address < (CrashReporter_ModuleBase + CrashReporter_ModuleSize);
}

_Unwind_Reason_Code CrashReporter_UnwindCallback(struct _Unwind_Context* context, void* argument) {
    auto* frames = static_cast<CrashReporter_Frames*>(argument);
    const uintptr_t pc = static_cast<uintptr_t>(_Unwind_GetIP(context));

    if (pc == 0u || frames->count >= kMaxFrames) {
        return _URC_END_OF_STACK;
    }

    frames->address[frames->count++] = pc;

    return _URC_NO_REASON;
}

void CrashReporter_CaptureFrames(CrashReporter_Frames& frames) {
    frames.count = 0;
    _Unwind_Backtrace(&CrashReporter_UnwindCallback, &frames);
}

struct CrashReporter_SymbolState {
    int fd;
    int32_t index;
    bool resolved;
};

void CrashReporter_SymbolError(void* data, const char* message, int errnum) {
    auto* state = static_cast<CrashReporter_SymbolState*>(data);

    CrashReporter_Write(state->fd, "    (symbol lookup failed: ");
    CrashReporter_Write(state->fd, message ? message : "unknown");

    if (errnum > 0) {
        CrashReporter_Write(state->fd, ", errno ");
        CrashReporter_WriteSigned(state->fd, errnum);
    }

    CrashReporter_Write(state->fd, ")\n");
}

void CrashReporter_WriteSymbolName(int fd, const char* name) {
    int status = -1;
    char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);

    CrashReporter_Write(fd, (status == 0 && demangled) ? demangled : name);

    if (demangled) {
        std::free(demangled);
    }
}

int CrashReporter_SymbolFound(void* data, uintptr_t /* pc */, const char* filename, int lineno, const char* function) {
    auto* state = static_cast<CrashReporter_SymbolState*>(data);

    if (!function && !filename) {
        return 0;
    }

    state->resolved = true;

    CrashReporter_Write(state->fd, "         ");

    if (function) {
        CrashReporter_WriteSymbolName(state->fd, function);

    } else {
        CrashReporter_Write(state->fd, "<unknown function>");
    }

    if (filename) {
        CrashReporter_Write(state->fd, " (");
        CrashReporter_Write(state->fd, filename);
        CrashReporter_Write(state->fd, ":");
        CrashReporter_WriteSigned(state->fd, lineno);
        CrashReporter_Write(state->fd, ")");
    }

    CrashReporter_Write(state->fd, "\n");

    return 0;
}

void CrashReporter_SymbolNameFound(void* data, uintptr_t pc, const char* symname, uintptr_t symval,
                                   uintptr_t /* symsize */) {
    auto* state = static_cast<CrashReporter_SymbolState*>(data);

    if (!symname) {
        return;
    }

    state->resolved = true;

    CrashReporter_Write(state->fd, "         ");
    CrashReporter_WriteSymbolName(state->fd, symname);

    if (symval != 0u && pc >= symval) {
        CrashReporter_Write(state->fd, " +");
        CrashReporter_WriteHex(state->fd, pc - symval);
    }

    CrashReporter_Write(state->fd, " (no line information)\n");
}

const char* CrashReporter_SignalName(int signal_number) {
    const char* name;

    switch (signal_number) {
        case SIGSEGV: {
            name = "SIGSEGV (invalid memory access)";
        } break;

        case SIGABRT: {
            name = "SIGABRT (abort, usually a failed assertion)";
        } break;

        case SIGFPE: {
            name = "SIGFPE (arithmetic exception)";
        } break;

        case SIGILL: {
            name = "SIGILL (illegal instruction)";
        } break;

#if defined(SIGBUS)
        case SIGBUS: {
            name = "SIGBUS (bus error)";
        } break;
#endif

        default: {
            name = "unknown signal";
        } break;
    }

    return name;
}

void CrashReporter_WriteHeader(int fd, const char* reason, int signal_number) {
    CrashReporter_Write(fd,
                        "================================================================\n"
                        "M.A.X. Port crashed.\n"
                        "\n"
                        "Please attach this whole file to a bug report at\n"
                        "https://github.com/klei1984/max/issues\n"
                        "================================================================\n\n");

    CrashReporter_Write(fd, "Reason          : ");
    CrashReporter_Write(fd, reason);

    if (signal_number != 0) {
        CrashReporter_Write(fd, " [signal ");
        CrashReporter_WriteSigned(fd, signal_number);
        CrashReporter_Write(fd, "]");
    }

    CrashReporter_Write(fd, "\n");

#if defined(_WIN32)
    if (CrashReporter_ExceptionCode != 0) {
        CrashReporter_Write(fd, "Exception code  : ");
        CrashReporter_WriteHex(fd, static_cast<uint32_t>(CrashReporter_ExceptionCode));
        CrashReporter_Write(fd, "\n");
    }

    if (CrashReporter_FaultAddress != 0u) {
        CrashReporter_Write(fd, "Fault address   : ");
        CrashReporter_WriteHex(fd, CrashReporter_FaultAddress);
        CrashReporter_Write(fd, "\n");
    }
#else
    if (CrashReporter_FaultAddress != nullptr) {
        CrashReporter_Write(fd, "Fault address   : ");
        CrashReporter_WriteHex(fd, reinterpret_cast<uintptr_t>(CrashReporter_FaultAddress));
        CrashReporter_Write(fd, "\n");
    }
#endif

    const uint64_t frequency = SDL_GetPerformanceFrequency();

    if (frequency > 0u) {
        CrashReporter_Write(fd, "Uptime          : ");
        CrashReporter_WriteSigned(
            fd, static_cast<int64_t>(((SDL_GetPerformanceCounter() - CrashReporter_StartCounter) * 1000u) / frequency));
        CrashReporter_Write(fd, " ms\n");
    }

    CrashReporter_Write(fd, "\n");

    CrashReporter_Write(fd, CrashReporter_SystemInfo);
    CrashReporter_Write(fd, "\n");
}

void CrashReporter_WriteRawFrames(int fd, const CrashReporter_Frames& frames) {
    CrashReporter_Write(fd,
                        "---- raw frames ------------------------------------------------\n"
                        "Offsets are relative to the module load address and stay valid\n"
                        "even when no debug information is installed. The first few frames\n"
                        "belong to the crash handler itself.\n\n");

    for (int32_t index = 0; index < frames.count; ++index) {
        const uintptr_t address = frames.address[index];

        CrashReporter_Write(fd, "  #");

        if (index < 10) {
            CrashReporter_Write(fd, "0");
        }

        CrashReporter_WriteSigned(fd, index);
        CrashReporter_Write(fd, "  ");
        CrashReporter_WriteHex(fd, address);

        if (CrashReporter_IsOwnModule(address)) {
            CrashReporter_Write(fd, "  ");
            CrashReporter_Write(fd, CrashReporter_ModuleName);
            CrashReporter_Write(fd, " [+");
            CrashReporter_WriteHex(fd, address - CrashReporter_ModuleBase);
            CrashReporter_Write(fd, "]");

        } else {
            CrashReporter_Write(fd, "  (system module)");
        }

        CrashReporter_Write(fd, "\n");
    }

    if (frames.count == 0) {
        CrashReporter_Write(fd, "  (the stack could not be unwound)\n");
    }

    CrashReporter_Write(fd, "\n");
}

void CrashReporter_WriteContext(int fd) {
    CrashReporter_Write(fd, "---- game state ------------------------------------------------\n");

    if (!CrashReporter_Provider) {
        CrashReporter_Write(fd, "  (no game state was registered)\n\n");

        return;
    }

    CrashContext context;
    std::memset(&context, 0, sizeof(context));

    CrashReporter_Provider(context);

    if (!context.valid) {
        CrashReporter_Write(fd, "  (the game had not started yet)\n\n");

        return;
    }

    CrashReporter_Write(fd, "Phase           : ");
    CrashReporter_Write(fd, context.phase);
    CrashReporter_Write(fd, "\n");

    if (context.mission_kind[0] != '\0') {
        CrashReporter_Write(fd, "Mission kind    : ");
        CrashReporter_Write(fd, context.mission_kind);
        CrashReporter_Write(fd, "\n");
    }

    if (context.mission[0] != '\0') {
        CrashReporter_Write(fd, "Mission         : ");
        CrashReporter_Write(fd, context.mission);
        CrashReporter_Write(fd, "\n");
    }

    if (context.mission_hash[0] != '\0') {
        CrashReporter_Write(fd, "Mission hash    : ");
        CrashReporter_Write(fd, context.mission_hash);
        CrashReporter_Write(fd, "\n");
    }

    if (context.save_file[0] != '\0') {
        CrashReporter_Write(fd, "Save file       : ");
        CrashReporter_Write(fd, context.save_file);
        CrashReporter_Write(fd, " [slot ");
        CrashReporter_WriteSigned(fd, context.save_slot);
        CrashReporter_Write(fd, "]\n");
    }

    CrashReporter_Write(fd, "Turn            : ");
    CrashReporter_WriteSigned(fd, context.turn);
    CrashReporter_Write(fd, "\n");

    CrashReporter_Write(fd, "Game state      : ");
    CrashReporter_WriteSigned(fd, context.game_state);
    CrashReporter_Write(fd, "\n");

    CrashReporter_Write(fd, "Play mode       : ");
    CrashReporter_WriteSigned(fd, context.play_mode);
    CrashReporter_Write(fd, context.real_time ? " (real time)\n" : " (turn based)\n");

    CrashReporter_Write(fd, "Player team     : ");
    CrashReporter_WriteSigned(fd, context.player_team);
    CrashReporter_Write(fd, "\n");

    CrashReporter_Write(fd, "Active team     : ");
    CrashReporter_WriteSigned(fd, context.active_team);
    CrashReporter_Write(fd, "\n");

    CrashReporter_Write(fd, "Human players   : ");
    CrashReporter_WriteSigned(fd, context.human_players);
    CrashReporter_Write(fd, "\n");

    CrashReporter_Write(fd, "Opponent level  : ");
    CrashReporter_WriteSigned(fd, context.opponent);
    CrashReporter_Write(fd, "\n");

    CrashReporter_Write(fd, "Victory         : type ");
    CrashReporter_WriteSigned(fd, context.victory_type);
    CrashReporter_Write(fd, ", limit ");
    CrashReporter_WriteSigned(fd, context.victory_limit);
    CrashReporter_Write(fd, "\n");

    if (context.cheater) {
        CrashReporter_Write(fd, "Cheats          : used in this session\n");
    }

    CrashReporter_Write(fd, "\n");
}

void CrashReporter_WriteBacktrace(int fd, const CrashReporter_Frames& frames) {
    CrashReporter_Write(fd, "---- backtrace -------------------------------------------------\n");

    CrashReporter_Write(fd, "Symbols from    : ");
    CrashReporter_Write(fd, CrashReporter_SymbolPathUtf8);
    CrashReporter_Write(fd, "\n\n");

    CrashReporter_SymbolState state;
    state.fd = fd;
    state.index = 0;
    state.resolved = false;

    backtrace_state* backtrace =
        backtrace_create_state(CrashReporter_SymbolPathUtf8, 0, &CrashReporter_SymbolError, &state);

    if (!backtrace) {
        CrashReporter_Write(fd, "  (symbol reader unavailable, use the raw frames above)\n\n");

        return;
    }

    for (int32_t index = 0; index < frames.count; ++index) {
        state.index = index;
        state.resolved = false;

        CrashReporter_Write(fd, "  #");

        if (index < 10) {
            CrashReporter_Write(fd, "0");
        }

        CrashReporter_WriteSigned(fd, index);
        CrashReporter_Write(fd, "  ");
        CrashReporter_WriteHex(fd, frames.address[index]);
        CrashReporter_Write(fd, "\n");

        backtrace_pcinfo(backtrace, frames.address[index], &CrashReporter_SymbolFound, &CrashReporter_SymbolError,
                         &state);

        if (!state.resolved && CrashReporter_IsOwnModule(frames.address[index])) {
            backtrace_syminfo(backtrace, frames.address[index] - CrashReporter_SymbolSlide,
                              &CrashReporter_SymbolNameFound, &CrashReporter_SymbolError, &state);
        }

        if (!state.resolved) {
            CrashReporter_Write(fd, "         <no debug information>\n");
        }
    }

    CrashReporter_Write(fd,
                        "\n"
                        "---- end of backtrace ------------------------------------------\n");
}

void CrashReporter_WriteReport(const char* reason, int signal_number) {
    CrashReporter_Frames frames;
    CrashReporter_CaptureFrames(frames);

    const int fd = CrashReporter_OpenReport(CrashReporter_ReportPathUtf8);

    if (fd >= 0) {
        CrashReporter_WriteHeader(fd, reason, signal_number);
        CrashReporter_WriteRawFrames(fd, frames);
        CrashReporter_WriteContext(fd);
        CrashReporter_WriteBacktrace(fd, frames);
        CrashReporter_CloseReport(fd);
    }

    CrashReporter_Write(2, "\nM.A.X. Port crashed. Crash report written to:\n  ");
    CrashReporter_Write(2, CrashReporter_ReportPathUtf8);
    CrashReporter_Write(2, "\n");
}

void CrashReporter_HandleCrash(int signal_number) {
    if (CrashReporter_InHandler) {
        std::signal(signal_number, SIG_DFL);
        std::raise(signal_number);

        return;
    }

    CrashReporter_InHandler = 1;

    CrashReporter_WriteReport(CrashReporter_SignalName(signal_number), signal_number);

    std::signal(signal_number, SIG_DFL);
    std::raise(signal_number);
}

#if !defined(_WIN32)
void CrashReporter_HandleCrashAction(int signal_number, siginfo_t* info, void* /* ucontext */) {
    CrashReporter_FaultAddress = info ? info->si_addr : nullptr;

    CrashReporter_HandleCrash(signal_number);
}
#endif

#if defined(_WIN32)
LONG WINAPI CrashReporter_VectoredHandler(EXCEPTION_POINTERS* pointers) {
    if (!pointers || !pointers->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const DWORD code = pointers->ExceptionRecord->ExceptionCode;
    const char* reason;

    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: {
            reason = "access violation";
        } break;

        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: {
            reason = "array bounds exceeded";
        } break;

        case EXCEPTION_DATATYPE_MISALIGNMENT: {
            reason = "misaligned access";
        } break;

        case EXCEPTION_FLT_DIVIDE_BY_ZERO: {
            reason = "floating point division by zero";
        } break;

        case EXCEPTION_FLT_INVALID_OPERATION: {
            reason = "invalid floating point operation";
        } break;

        case EXCEPTION_ILLEGAL_INSTRUCTION: {
            reason = "illegal instruction";
        } break;

        case EXCEPTION_INT_DIVIDE_BY_ZERO: {
            reason = "integer division by zero";
        } break;

        case EXCEPTION_IN_PAGE_ERROR: {
            reason = "in-page error (the file backing this memory is gone)";
        } break;

        case EXCEPTION_PRIV_INSTRUCTION: {
            reason = "privileged instruction";
        } break;

        case EXCEPTION_STACK_OVERFLOW: {
            reason = "stack overflow (runaway recursion)";
        } break;

        default: {
            reason = nullptr;
        } break;
    }

    if (!reason) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    CrashReporter_ExceptionCode = static_cast<std::sig_atomic_t>(code);

    if ((code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR) &&
        pointers->ExceptionRecord->NumberParameters >= 2) {
        CrashReporter_FaultAddress = static_cast<uintptr_t>(pointers->ExceptionRecord->ExceptionInformation[1]);

    } else {
        CrashReporter_FaultAddress = reinterpret_cast<uintptr_t>(pointers->ExceptionRecord->ExceptionAddress);
    }

    if (!CrashReporter_InHandler) {
        CrashReporter_InHandler = 1;

        CrashReporter_WriteReport(reason, 0);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

/* ------------------------------------------------------------------------ */
/* startup                                                                  */
/* ------------------------------------------------------------------------ */

#if defined(__linux__) && !defined(_WIN32)
int CrashReporter_VisitMainObject(struct dl_phdr_info* info, size_t /* size */, void* /* data */) {
    CrashReporter_ModuleBase = static_cast<uintptr_t>(info->dlpi_addr);

    for (int index = 0; index < static_cast<int>(info->dlpi_phnum); ++index) {
        const ElfW(Phdr) & header = info->dlpi_phdr[index];

        if (header.p_type == PT_LOAD) {
            const uintptr_t end = static_cast<uintptr_t>(header.p_vaddr) + static_cast<uintptr_t>(header.p_memsz);

            if (end > CrashReporter_ModuleSize) {
                CrashReporter_ModuleSize = end;
            }
        }
    }

    return 1;
}
#endif

#if defined(_WIN32)
uint64_t CrashReporter_ReadFileImageBase(const char* path) {
    std::FILE* file = std::fopen(path, "rb");

    if (!file) {
        return 0u;
    }

    uint64_t image_base = 0u;
    uint32_t pe_offset = 0u;
    uint32_t signature = 0u;
    uint16_t magic = 0u;

    if (std::fseek(file, 0x3c, SEEK_SET) == 0 && std::fread(&pe_offset, sizeof(pe_offset), 1, file) == 1 &&
        std::fseek(file, static_cast<long>(pe_offset), SEEK_SET) == 0 &&
        std::fread(&signature, sizeof(signature), 1, file) == 1 && signature == 0x00004550u) {
        const long optional = static_cast<long>(pe_offset) + 4 + 20;

        if (std::fseek(file, optional, SEEK_SET) == 0 && std::fread(&magic, sizeof(magic), 1, file) == 1) {
            if (magic == 0x10Bu) {
                uint32_t narrow = 0u;

                if (std::fseek(file, optional + 0x1c, SEEK_SET) == 0 &&
                    std::fread(&narrow, sizeof(narrow), 1, file) == 1) {
                    image_base = narrow;
                }

            } else if (magic == 0x20Bu) {
                uint64_t wide = 0u;

                if (std::fseek(file, optional + 0x18, SEEK_SET) == 0 && std::fread(&wide, sizeof(wide), 1, file) == 1) {
                    image_base = wide;
                }
            }
        }
    }

    std::fclose(file);

    return image_base;
}
#endif

void CrashReporter_ResolveModule() {
#if defined(_WIN32)
    const HMODULE module = GetModuleHandle(nullptr);

    if (!module) {
        return;
    }

    CrashReporter_ModuleBase = reinterpret_cast<uintptr_t>(module);

    const auto* dos_header = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);

    if (dos_header->e_magic == IMAGE_DOS_SIGNATURE) {
        const auto* nt_headers =
            reinterpret_cast<const IMAGE_NT_HEADERS*>(reinterpret_cast<const uint8_t*>(module) + dos_header->e_lfanew);

        if (nt_headers->Signature == IMAGE_NT_SIGNATURE) {
            CrashReporter_ModuleSize = nt_headers->OptionalHeader.SizeOfImage;
        }
    }

    const uint64_t file_image_base = CrashReporter_ReadFileImageBase(CrashReporter_SymbolPathUtf8);

    if (file_image_base != 0u) {
        CrashReporter_SymbolSlide = CrashReporter_ModuleBase - static_cast<uintptr_t>(file_image_base);
    }
#elif defined(__linux__)
    dl_iterate_phdr(&CrashReporter_VisitMainObject, nullptr);
#endif
}

std::filesystem::path CrashReporter_ResolveExecutable() {
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];

    if (GetModuleFileNameW(nullptr, buffer, MAX_PATH) > 0) {
        return std::filesystem::path(buffer);
    }
#else
    std::error_code error;
    const auto executable = std::filesystem::read_symlink("/proc/self/exe", error);

    if (!error) {
        return executable;
    }
#endif

    return std::filesystem::path();
}

#if defined(__linux__) && !defined(_WIN32)
std::filesystem::path CrashReporter_ResolveFlatpakSymbols(const std::filesystem::path& executable) {
    static constexpr char kAppPrefix[] = "/app/";

    std::error_code error;

    if (!std::filesystem::exists("/.flatpak-info", error) || error) {
        return std::filesystem::path();
    }

    const auto text = executable.string();
    const size_t prefix = std::strlen(kAppPrefix);

    if (text.compare(0, prefix, kAppPrefix) != 0) {
        return std::filesystem::path();
    }

    /* /app/bin/max-port becomes /app/lib/debug/bin/max-port.debug */
    std::filesystem::path candidate("/app/lib/debug");
    candidate /= text.substr(prefix);
    candidate += ".debug";

    if (std::filesystem::exists(candidate, error) && !error) {
        return candidate;
    }

    return std::filesystem::path();
}
#endif

std::filesystem::path CrashReporter_ResolveSymbolSource(const std::filesystem::path& executable) {
    std::error_code error;

    if (executable.empty()) {
        return executable;
    }

    std::filesystem::path candidate = executable;
    candidate += ".debug";

    if (std::filesystem::exists(candidate, error) && !error) {
        return candidate;
    }

    candidate = executable;
    candidate.replace_extension(".debug");

    if (std::filesystem::exists(candidate, error) && !error) {
        return candidate;
    }

#if defined(__linux__) && !defined(_WIN32)
    candidate = CrashReporter_ResolveFlatpakSymbols(executable);

    if (!candidate.empty()) {
        return candidate;
    }
#endif

    return executable;
}

const char* CrashReporter_BuildType() {
#if defined(NDEBUG)
    return "Release";
#else
    return "Debug";
#endif
}

const char* CrashReporter_Architecture() {
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__aarch64__)
    return "arm64";
#else
    return "unknown";
#endif
}

void CrashReporter_BuildSystemInfo() {
    const int sdl_version = SDL_GetVersion();

    SDL_snprintf(CrashReporter_SystemInfo, sizeof(CrashReporter_SystemInfo),
                 "Version         : M.A.X. Port %d.%d.%d\n"
                 "Revision        : %s\n"
                 "Build           : %s %s, %s, built %s %s\n"
                 "Compiler        : %s\n"
                 "SDL             : %d.%d.%d headers, %d.%d.%d runtime (%s)\n"
                 "Platform        : %s, %d logical cores, %d MB RAM\n"
                 "Executable      : %s\n"
                 "Symbols         : %s\n",
                 GAME_VERSION_MAJOR, GAME_VERSION_MINOR, GAME_VERSION_PATCH,
#if defined(GAME_VERSION_REVISION)
                 GAME_VERSION_REVISION,
#else
                 "unknown",
#endif
                 CrashReporter_BuildType(), CrashReporter_Architecture(),
#if defined(CROSS)
                 "cross compiled",
#else
                 "native",
#endif
                 __DATE__, __TIME__,
#if defined(__clang__)
                 "clang " __clang_version__,
#elif defined(__GNUC__)
                 "gcc " __VERSION__,
#else
                 "unknown",
#endif
                 SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION, SDL_VERSIONNUM_MAJOR(sdl_version),
                 SDL_VERSIONNUM_MINOR(sdl_version), SDL_VERSIONNUM_MICRO(sdl_version), SDL_GetRevision(),
                 SDL_GetPlatform(), SDL_GetNumLogicalCPUCores(), SDL_GetSystemRAM(), CrashReporter_ModuleName,
                 CrashReporter_SymbolPathUtf8);
}

void CrashReporter_PruneOldReports(const std::filesystem::path& directory) {
    std::error_code error;
    std::vector<std::filesystem::path> reports;

    for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
        if (error) {
            return;
        }

        if (entry.is_regular_file(error) && !error) {
            const auto name = entry.path().filename().string();

            if (name.rfind("crash-", 0) == 0) {
                reports.push_back(entry.path());
            }
        }
    }

    if (static_cast<int32_t>(reports.size()) < kMaxReportsKept) {
        return;
    }

    std::sort(reports.begin(), reports.end());

    const size_t excess = reports.size() - static_cast<size_t>(kMaxReportsKept) + 1u;

    for (size_t index = 0; index < excess; ++index) {
        std::filesystem::remove(reports[index], error);
    }
}

void CrashReporter_CopyUtf8(char* destination, size_t size, const std::filesystem::path& path) {
    const auto text = path.string();

    std::strncpy(destination, text.c_str(), size - 1u);
    destination[size - 1u] = '\0';
}

}  // namespace

bool CrashReporter_Init(const std::filesystem::path& pref_path) {
    std::error_code error;

    const auto directory = (pref_path / "crashes").lexically_normal();

    std::filesystem::create_directories(directory, error);

    if (error) {
        SDL_Log("CrashReporter: unable to create \"%s\", crash reports are disabled.\n", directory.string().c_str());

        return false;
    }

    CrashReporter_PruneOldReports(directory);

    const std::time_t now = std::time(nullptr);
    std::tm local{};

#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif

    char stamp[64];
    std::strftime(stamp, sizeof(stamp), "crash-%Y%m%d-%H%M%S.txt", &local);

    const auto executable = CrashReporter_ResolveExecutable();

    CrashReporter_ReportPath = (directory / stamp).lexically_normal();
    CrashReporter_SymbolPath = CrashReporter_ResolveSymbolSource(executable);

    CrashReporter_CopyUtf8(CrashReporter_ReportPathUtf8, sizeof(CrashReporter_ReportPathUtf8),
                           CrashReporter_ReportPath);
    CrashReporter_CopyUtf8(CrashReporter_SymbolPathUtf8, sizeof(CrashReporter_SymbolPathUtf8),
                           CrashReporter_SymbolPath);

    CrashReporter_CopyUtf8(CrashReporter_ModuleName, sizeof(CrashReporter_ModuleName), executable.filename());

    CrashReporter_StartCounter = SDL_GetPerformanceCounter();

    CrashReporter_ResolveModule();
    CrashReporter_BuildSystemInfo();

#if defined(_WIN32)
    ULONG guarantee = 64u * 1024u;
    SetThreadStackGuarantee(&guarantee);

    CrashReporter_VectoredHandle = AddVectoredExceptionHandler(1, &CrashReporter_VectoredHandler);

    std::signal(SIGSEGV, &CrashReporter_HandleCrash);
    std::signal(SIGABRT, &CrashReporter_HandleCrash);
    std::signal(SIGFPE, &CrashReporter_HandleCrash);
    std::signal(SIGILL, &CrashReporter_HandleCrash);
#else
    stack_t alternate{};
    alternate.ss_sp = CrashReporter_AltStack;
    alternate.ss_size = sizeof(CrashReporter_AltStack);
    alternate.ss_flags = 0;
    sigaltstack(&alternate, nullptr);

    struct sigaction action;

    std::memset(&action, 0, sizeof(action));

    action.sa_sigaction = &CrashReporter_HandleCrashAction;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGFPE, &action, nullptr);
    sigaction(SIGILL, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
#endif

    CrashReporter_Installed = true;

    return true;
}

void CrashReporter_SetContextProvider(CrashContextProvider provider) { CrashReporter_Provider = provider; }

void CrashReporter_Shutdown() {
    if (!CrashReporter_Installed) {
        return;
    }

    std::signal(SIGSEGV, SIG_DFL);
    std::signal(SIGABRT, SIG_DFL);
    std::signal(SIGFPE, SIG_DFL);
    std::signal(SIGILL, SIG_DFL);

#if defined(_WIN32)
    if (CrashReporter_VectoredHandle) {
        RemoveVectoredExceptionHandler(CrashReporter_VectoredHandle);
        CrashReporter_VectoredHandle = nullptr;
    }
#else
    std::signal(SIGBUS, SIG_DFL);
#endif

    CrashReporter_Provider = nullptr;
    CrashReporter_Installed = false;
}

const char* CrashReporter_GetSystemInfo() { return CrashReporter_SystemInfo; }

const std::filesystem::path& CrashReporter_GetReportPath() { return CrashReporter_ReportPath; }

const std::filesystem::path& CrashReporter_GetSymbolPath() { return CrashReporter_SymbolPath; }
