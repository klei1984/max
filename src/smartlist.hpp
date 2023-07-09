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

#ifndef SMARTLIST_HPP
#define SMARTLIST_HPP

#include "smartpointer.hpp"

template <class T>
class SmartList {
    template <class N>
    class ListNode : public SmartObject {
        SmartPointer<N> object;
        SmartPointer<ListNode<N>> next;
        SmartPointer<ListNode<N>> prev;

        friend class SmartList;

    public:
        explicit ListNode(N& object) : object(object) {}
        ~ListNode() = default;

        inline void InsertAfter(ListNode<N>& node) noexcept {
            node.next = this->next;
            node.prev = this;

            if (this->next != nullptr) {
                this->next->prev = node;
            }

            this->next = node;
        }

        inline void InsertBefore(ListNode<N>& node) noexcept {
            node.prev = this->prev;
            node.next = this;

            if (this->prev != nullptr) {
                this->prev->next = node;
            }

            this->prev = node;
        }

        inline void RemoveSelf() noexcept {
            if (this->prev != nullptr) {
                this->prev->next = this->next;
            }

            if (this->next != nullptr) {
                this->next->prev = this->prev;
            }
        }

        inline N* Get() const noexcept { return object.Get(); }

        inline ListNode<N>& operator=(const ListNode<N>* other) noexcept {
            if (this != other) {
                object = other->object;
            }

            return *this;
        }
    };

    uint16_t count{0};
    SmartPointer<ListNode<T>> first;
    SmartPointer<ListNode<T>> last;

    inline void Erase(ListNode<T>& position) noexcept {
        if (position.prev == nullptr) {
            if (first != position) {
                return;
            }

        } else {
            if (position.prev->next != position) {
                return;
            }
        }

        --count;

        position.RemoveSelf();

        if (first == position) {
            first = position.next;
        }

        if (last == position) {
            last = position.prev;
        }
    }

    inline ListNode<T>& Get(int32_t index) const noexcept {
        Iterator it;

        if (index >= (count / 2)) {
            index = count - index - 1;

            for (it = last.Get(); index != -1; --it) {
                --index;
            }

        } else {
            --index;

            for (it = first.Get(); index != -1; ++it) {
                --index;
            }
        }

        return it.GetNode();
    }

public:
    class Iterator : public SmartPointer<ListNode<T>> {
        friend class SmartList;

        ListNode<T>& GetNode() const noexcept { return *this->Get(); }

    public:
        Iterator() noexcept : SmartPointer<ListNode<T>>(nullptr) {}
        Iterator(ListNode<T>* object) noexcept : SmartPointer<ListNode<T>>(object) {}
        Iterator(ListNode<T>& object) noexcept : SmartPointer<ListNode<T>>(object) {}
        Iterator(const Iterator& other) noexcept : SmartPointer<ListNode<T>>(other.GetNode()) {}

        inline T& operator*() const noexcept { return *(this->Get()->Get()); }
        inline explicit operator T&() const noexcept { return *(this->Get()->Get()); }

        Iterator& operator++() noexcept {
            SmartPointer<ListNode<T>>::operator=(this->Get()->next);
            return *this;
        }

        Iterator& operator--() noexcept {
            SmartPointer<ListNode<T>>::operator=(this->Get()->prev);
            return *this;
        }
    };

    SmartList() noexcept = default;

    SmartList(const SmartList& other) noexcept {
        for (Iterator it = other.Begin(); it != other.End(); ++it) {
            PushBack(*it);
        }
    }

    ~SmartList() noexcept { Clear(); }

    inline Iterator Begin() const noexcept { return Iterator(first.Get()); }

    inline Iterator Last() const noexcept { return Iterator(last.Get()); }

    inline Iterator End() const {
        ListNode<T>* end = last.Get();

        if (end) {
            end = last->next.Get();
        }

        return Iterator(end);
    }

    inline void PushBack(T& object) noexcept {
        Iterator it = new (std::nothrow) ListNode<T>(object);

        ++count;

        if (last == nullptr) {
            first = it;

        } else {
            (*last).InsertAfter(it.GetNode());
        }

        last = it;
    }

    inline void PushFront(T& object) noexcept {
        Iterator it = new (std::nothrow) ListNode<T>(object);

        ++count;

        if (first == nullptr) {
            last = it;

        } else {
            (*first).InsertBefore(it.GetNode());
        }

        first = it;
    }

    inline void InsertAfter(Iterator& position, T& object) noexcept {
        if (position == nullptr) {
            PushBack(object);

        } else {
            Iterator it = new (std::nothrow) ListNode<T>(object);

            ++count;

            position.GetNode().InsertBefore(it.GetNode());

            if (it.GetNode().prev == nullptr) {
                first = it;
            }
        }
    }

    inline void InsertBefore(Iterator& position, T& object) noexcept {
        if (position == nullptr) {
            PushFront(object);

        } else {
            Iterator it = new (std::nothrow) ListNode<T>(object);

            ++count;

            position.GetNode().InsertAfter(it.GetNode());

            if (it.GetNode().prev == nullptr) {
                last = it;
            }
        }
    }

    inline ListNode<T>* Find(T& object) const noexcept {
        ListNode<T>* result = nullptr;

        for (Iterator it = Begin(); it != End(); ++it) {
            if (&*it == &object) {
                result = &it.GetNode();
                break;
            }
        }

        return result;
    }

    inline bool Remove(T& object) noexcept {
        bool result{false};

        Iterator it = Find(object);

        if (it != nullptr) {
            Erase(it.GetNode());
            result = true;

        } else {
            result = false;
        }

        return result;
    }

    inline void Remove(Iterator& position) noexcept { Erase(position.GetNode()); }

    inline void Clear() noexcept {
        while (first != nullptr) {
            last = first->next;
            first->prev = nullptr;
            first->next = nullptr;
            first = last;
        }

        last = nullptr;
        count = 0;
    }

    [[nodiscard]] inline uint16_t GetCount() const noexcept { return count; }

    [[nodiscard]] inline int32_t GetMemorySize() const noexcept { return count * sizeof(ListNode<T>) + sizeof(count); }

    inline SmartList<T>& operator=(const SmartList<T>& other) {
        if (&this == &other) {
            return *this;
        }

        Clear();

        for (Iterator it = other.Begin(); it != other.End(); ++it) {
            PushBack(*it);
        }

        return *this;
    }

    inline T& operator[](uint16_t index) const noexcept {
        SDL_assert(index >= 0 && index < count);

        return *Get(index).Get();
    }
};

#endif /* SMARTLIST_HPP */
