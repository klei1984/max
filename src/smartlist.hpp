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
        ListNode() noexcept : object(nullptr) {}
        explicit ListNode(N& object) noexcept : object(object) {}
        ~ListNode() noexcept override = default;

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

        [[nodiscard]] inline N* Get() const noexcept { return object.Get(); }
    };

    uint32_t count{0};
    SmartPointer<ListNode<T>> list_node;

    [[nodiscard]] inline ListNode<T>& Get(uint32_t index) const noexcept {
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

        [[nodiscard]] ListNode<T>& GetNode() const noexcept { return *this->Get(); }

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
    using Compare = bool (*)(const Iterator& lhs, const Iterator& rhs);

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

    [[nodiscard]] inline Iterator Begin() noexcept { return Iterator(list_node->next); }
    [[nodiscard]] inline Iterator Begin() const noexcept { return Iterator(list_node->next); }
    [[nodiscard]] inline Iterator End() noexcept { return Iterator(list_node); }
    [[nodiscard]] inline Iterator End() const noexcept { return Iterator(list_node); }
    [[nodiscard]] inline T& Front() const noexcept { return *Begin(); }
    [[nodiscard]] inline T& Back() const noexcept { return *(--End()); }

    /* compatibility interfaces */
    [[nodiscard]] inline Iterator begin() noexcept { return Iterator(list_node->next); }
    [[nodiscard]] inline Iterator begin() const noexcept { return Iterator(list_node->next); }
    [[nodiscard]] inline Iterator end() noexcept { return Iterator(list_node); }
    [[nodiscard]] inline Iterator end() const noexcept { return Iterator(list_node); }

    inline void PushBack(T& object) noexcept {
        End()->InsertBefore(*(new (std::nothrow) ListNode<T>(object)));
        ++count;
    }

    inline void PushFront(T& object) noexcept {
        Begin()->InsertBefore(*(new (std::nothrow) ListNode<T>(object)));
        ++count;
    }

    inline void InsertAfter(Iterator& position, T& object) noexcept {
        if (position == list_node || position.Get() == nullptr) {
            PushFront(object);

        } else {
            position->InsertAfter(*(new (std::nothrow) ListNode<T>(object)));
            ++count;
        }
    }

    inline void InsertBefore(Iterator& position, T& object) noexcept {
        if (position == list_node || position.Get() == nullptr) {
            PushBack(object);

        } else {
            position->InsertBefore(*(new (std::nothrow) ListNode<T>(object)));
            ++count;
        }
    }

    [[nodiscard]] inline Iterator Find(T& object) const noexcept {
        for (Iterator it = Begin(), end = End(); it != end; ++it) {
            if (&*it == &object) {
                return Iterator(it.GetNode());
            }
        }

        return End();
    }

    inline void Sort(Compare compare) noexcept {
        if (GetCount() > 1) {
            Sort(0, GetCount() - 1, compare);
        }
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

    inline void Remove(Iterator position) noexcept {
        Iterator it = Find(*position);

        if (it != End()) {
            Erase(it);
        }
    }

    inline void Clear() noexcept {
        while (Begin() != End()) {
            Erase(Begin());
        }

        SDL_assert(count == 0 && Begin() == End());
    }

    [[nodiscard]] inline uint32_t GetCount() const noexcept {
        SDL_assert(count == 0 ? Begin() == End() : true);

        return count;
    }

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

    [[nodiscard]] inline T& operator[](uint32_t index) const noexcept {
        SDL_assert(index < count);

        return *Get(index).Get();
    }

private:
    inline void Erase(Iterator position) noexcept {
        if (list_node != position.Get()) {
            position->RemoveSelf();
            --count;
        }

        SDL_assert(count == 0 ? Begin() == End() : true);
    }

    inline void Swap(uint32_t lhs, uint32_t rhs) noexcept {
        if (lhs == rhs) {
            return;
        }

        SmartPointer<T> object = Get(lhs).object;
        Get(lhs).object = Get(rhs).object;
        Get(rhs).object = object;
    }

    uint32_t Partition(uint32_t low, uint32_t high, Compare compare) noexcept {
        ListNode<T>& pivot = Get(high);
        uint32_t index{low - 1};

        for (uint32_t j{low}; j < high; ++j) {
            if (compare(Get(j), pivot)) {
                ++index;
                Swap(index, j);
            }
        }

        Swap(index + 1, high);

        return index + 1;
    }

    inline void Sort(uint32_t low, uint32_t high, Compare compare) noexcept {
        if (low < high) {
            const uint32_t pivot = Partition(low, high, compare);

            Sort(low, pivot - 1, compare);
            Sort(pivot + 1, high, compare);
        }
    }
};

#endif /* SMARTLIST_HPP */
