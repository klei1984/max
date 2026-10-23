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

#ifndef ORDER_LOG_HPP
#define ORDER_LOG_HPP

/* Unit order diagnostics of debug builds, enabled by the order_log setting. Records order transitions and order
 * system anomalies into order_log.txt in the preferences folder. Release builds compile every hook to nothing.
 */

#if !defined(NDEBUG)

#include <cstdint>

class UnitInfo;

void OrderLog_GameStart() noexcept;
void OrderLog_GameEnd() noexcept;
void OrderLog_Shutdown() noexcept;
void OrderLog_NoteWrite(const UnitInfo* unit, const void* writer) noexcept;

/* the tick in progress, its records in order_log.txt carry the same number, safe on any thread */
uint32_t OrderLog_GetTick() noexcept;

class OrderLogTick {
public:
    OrderLogTick() noexcept;
    ~OrderLogTick() noexcept;
};

class OrderLogIssuer {
public:
    explicit OrderLogIssuer(const void* issuer) noexcept;
    ~OrderLogIssuer() noexcept;
};

#define ORDER_LOG_GAME_START() OrderLog_GameStart()
#define ORDER_LOG_GAME_END() OrderLog_GameEnd()
#define ORDER_LOG_SHUTDOWN() OrderLog_Shutdown()
#define ORDER_LOG_TICK() OrderLogTick order_log_tick
#define ORDER_LOG_ISSUER() OrderLogIssuer order_log_issuer(__builtin_return_address(0))
#define ORDER_LOG_WRITE(unit) OrderLog_NoteWrite((unit), __builtin_return_address(0))

/* attributes a direct write to the writing function itself instead of its caller */
#define ORDER_LOG_WRITE_HERE(unit)                    \
    do {                                              \
        __label__ order_log_here;                     \
    order_log_here:                                   \
        OrderLog_NoteWrite((unit), &&order_log_here); \
    } while (false)

#else

#define ORDER_LOG_GAME_START()
#define ORDER_LOG_GAME_END()
#define ORDER_LOG_SHUTDOWN()
#define ORDER_LOG_TICK()
#define ORDER_LOG_ISSUER()
#define ORDER_LOG_WRITE(unit)
#define ORDER_LOG_WRITE_HERE(unit)

#endif /* !defined(NDEBUG) */

#endif /* ORDER_LOG_HPP */
