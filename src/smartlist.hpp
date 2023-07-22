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
        ListNode() : object(nullptr) {}
        explicit ListNode(N& object) : object(object) {}
        ~ListNode() = default;

        inline void InsertAfter(ListNode<N>& node) noexcept {
            node.next = this->next;
            node.prev = this;
            this->next->prev = node;
            this->next = node;
        }

        inline void InsertBefore(ListNode<N>& node) noexcept {
            node.prev = this->prev;
            node.next = this;
            this->prev->next = node;
            this->prev = node;
        }

        inline void RemoveSelf() noexcept {
            SmartPointer<ListNode<T>> backup(this);

            this->prev->next = this->next;
            this->next->prev = this->prev;
        }

        inline N* Get() const noexcept { return object.Get(); }
    };

    uint16_t count{0};
    SmartPointer<ListNode<T>> list_node;

    inline ListNode<T>& Get(int32_t index) const noexcept {
        Iterator it;

        if (index >= (count / 2)) {
            index = count - index - 1;

            for (it = --End(); index > 0; --it) {
                --index;
            }

        } else {
            for (it = Begin(); index > 0; ++it) {
                --index;
            }
        }

        return it.GetNode();
    }

public:
    class ListIterator : public SmartPointer<ListNode<T>> {
        friend class SmartList;

        ListNode<T>& GetNode() const noexcept { return *this->Get(); }

    protected:
        ListIterator(const ListNode<T>* object) noexcept : SmartPointer<ListNode<T>>(object) {}
        ListIterator(ListNode<T>& object) noexcept : SmartPointer<ListNode<T>>(object) {}
        ListIterator(const ListNode<T>& object) noexcept : SmartPointer<ListNode<T>>(object) {}
        ListIterator(SmartPointer<ListNode<T>>& object) noexcept : SmartPointer<ListNode<T>>(object) {}
        ListIterator(const SmartPointer<ListNode<T>>& object) noexcept : SmartPointer<ListNode<T>>(object) {}

    public:
        ListIterator() noexcept : SmartPointer<ListNode<T>>() {}
        ListIterator(ListIterator& other) noexcept : SmartPointer<ListNode<T>>(other.GetNode()) {}
        ListIterator(const ListIterator& other) noexcept : SmartPointer<ListNode<T>>(other.GetNode()) {}

        inline T& operator*() const noexcept { return *(this->Get()->Get()); }

        inline ListIterator& operator++() noexcept {
            // must not use operator=(ConstReference other) as the node could be deleted
            SmartPointer<ListNode<T>>::operator=(*this->Get()->next);
            return *this;
        }

        inline ListIterator& operator--() noexcept {
            // must not use operator=(ConstReference other) as the node could be deleted
            SmartPointer<ListNode<T>>::operator=(*this->Get()->prev);
            return *this;
        }

        inline bool operator==(std::nullptr_t) = delete;
        inline bool operator!=(std::nullptr_t) = delete;
        inline operator bool() = delete;
    };

    using Iterator = SmartList<T>::ListIterator;

    SmartList() noexcept : list_node(new(std::nothrow) ListNode<T>()) {
        list_node->next = list_node;
        list_node->prev = list_node;
    }

    SmartList(const SmartList<T>& other) noexcept : list_node(new(std::nothrow) ListNode<T>()) {
        list_node->next = list_node;
        list_node->prev = list_node;

        for (Iterator it = other.Begin(); it != other.End(); ++it) {
            PushBack(*it);
        }
    }

    ~SmartList() noexcept { Clear(); }

    inline Iterator Begin() noexcept { return Iterator(list_node->next); }
    inline Iterator Begin() const noexcept { return Iterator(list_node->next); }
    inline Iterator End() noexcept { return Iterator(list_node); }
    inline Iterator End() const noexcept { return Iterator(list_node); }
    inline T& Front() const noexcept { return *Begin(); }
    inline T& Back() const noexcept { return *(--End()); }

    /* compatibility interfaces */
    inline Iterator begin() noexcept { return Iterator(list_node->next); }
    inline Iterator begin() const noexcept { return Iterator(list_node->next); }
    inline Iterator end() noexcept { return Iterator(list_node); }
    inline Iterator end() const noexcept { return Iterator(list_node); }

    inline void PushBack(T& object) noexcept {
        End()->InsertBefore(*(new (std::nothrow) ListNode<T>(object)));
        ++count;
    }

    inline void PushFront(T& object) noexcept {
        Begin()->InsertBefore(*(new (std::nothrow) ListNode<T>(object)));
        ++count;
    }

    inline void InsertAfter(Iterator& position, T& object) noexcept {
        position.GetNode().InsertAfter(*(new (std::nothrow) ListNode<T>(object)));
        ++count;
    }

    inline void InsertBefore(Iterator& position, T& object) noexcept {
        position.GetNode().InsertBefore(*(new (std::nothrow) ListNode<T>(object)));
        ++count;
    }

    inline Iterator Find(T& object) const noexcept {
        for (Iterator it = Begin(), end = End(); it != end; ++it) {
            if (&*it == &object) {
                return Iterator(it.GetNode());
            }
        }

        return End();
    }

    inline bool Remove(T& object) noexcept {
        bool result{false};
        Iterator it = Find(object);

        if (it != End()) {
            Erase(it);
            result = true;

        } else {
            result = false;
        }

        return result;
    }

    inline void Remove(Iterator position) noexcept { Erase(position); }

    inline void Clear() noexcept {
        while (Begin() != End()) {
            Erase(Begin());
        }

        SDL_assert(count == 0);
    }

    inline uint16_t GetCount() const noexcept { return count; }

    inline int32_t GetMemorySize() const noexcept { return count * sizeof(ListNode<T>) + sizeof(count); }

    inline SmartList<T>& operator=(const SmartList<T>& other) {
        if (this == &other) {
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

private:
    inline void Erase(Iterator position) noexcept {
        if (list_node != position.Get()) {
            position->RemoveSelf();
            --count;
        }
    }
};

#endif /* SMARTLIST_HPP */
