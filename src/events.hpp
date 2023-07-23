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

#ifndef EVENTS_HPP
#define EVENTS_HPP

#include <cstdint>

class Event {
public:
    virtual uint16_t GetEventId() const = 0;
};

struct RegisterEvent {
    static uint16_t IdRegistry;
    RegisterEvent(uint16_t& id) { id = IdRegistry++; }
};

#define EVENTS_REGISTER_EVENT(event)  \
    uint16_t RegisterEventId_##event; \
    RegisterEvent RegisterEvent_##event(RegisterEventId_##event)

#define EVENTS_GET_EVENT_ID(event) RegisterEventId_##event

#define EVENTS_DECLARE_EVENT_ID(event) extern uint16_t RegisterEventId_##event

#endif /* EVENTS_HPP */
