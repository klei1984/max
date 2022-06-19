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
        ListNode(N& object) : object(object) {}
        ~ListNode() {}

        void InsertAfter(ListNode<N>& node) {
            node.next = this->next;
            node.prev = this;

            if (this->next != nullptr) {
                this->next->prev = node;
            }

            this->next = node;
        }

        void InsertBefore(ListNode<N>& node) {
            node.prev = this->prev;
            node.next = this;

            if (this->prev != nullptr) {
                this->prev->next = node;
            }

            this->prev = node;
        }

        void RemoveSelf() {
            if (this->prev != nullptr) {
                this->prev->next = this->next;
            }

            if (this->next != nullptr) {
                this->next->prev = this->prev;
            }
        }

        N* GetObject() { return &*object; }

        ListNode<N>& operator=(const ListNode<N>* other) {
            object = other->object;

            return *this;
        }
    };

    unsigned short count;
    SmartPointer<ListNode<T>> first;
    SmartPointer<ListNode<T>> last;

    void Erase(ListNode<T>& position) {
        if (position.prev == nullptr) {
            if (first != position) {
                return;
            }
        } else {
            if (position.prev->next != &position) {
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

    ListNode<T>& GetByIndex(int index) const {
        Iterator it;

        SDL_assert(index >= 0 && index < count);

        if (index > (count / 2)) {
            index = count - index - 1;

            for (it = &*last; index != -1; --it) {
                --index;
            }
        } else {
            --index;
            for (it = &*first; index != -1; ++it) {
                --index;
            }
        }

        return it.GetNode();
    }

public:
    class Iterator : public SmartPointer<ListNode<T>> {
        friend class SmartList;

        ListNode<T>& GetNode() const { return *this->object_pointer; }

    public:
        Iterator() : SmartPointer<ListNode<T>>(nullptr) {}
        Iterator(ListNode<T>* object) : SmartPointer<ListNode<T>>(object) {}

        T& operator*() const { return *(this->object_pointer->GetObject()); }

        Iterator& operator++() {
            SmartPointer<ListNode<T>>::operator=(this->object_pointer->next);
            return *this;
        }

        Iterator& operator--() {
            SmartPointer<ListNode<T>>::operator=(this->object_pointer->prev);
            return *this;
        }
    };

    SmartList() : count(0) {}

    SmartList(const SmartList& other) : count(0) {
        for (Iterator it = other.Begin(); it != other.End(); ++it) {
            PushBack(*it);
        }
    }

    ~SmartList() { Clear(); }

    Iterator Begin() const { return Iterator(&*first); }

    Iterator Last() const { return Iterator(&*last); }

    Iterator End() const {
        ListNode<T>* end = &*last;

        if (end) {
            end = &*last->next;
        }

        return Iterator(end);
    }

    void PushBack(T& object) {
        Iterator it = new (std::nothrow) ListNode<T>(object);
        ++count;
        if (last == nullptr) {
            first = it;
        } else {
            (*last).InsertAfter(it.GetNode());
        }

        last = it;
    }

    void PushFront(T& object) {
        Iterator it = new (std::nothrow) ListNode<T>(object);
        ++count;
        if (first == nullptr) {
            last = it;
        } else {
            (*first).InsertBefore(it.GetNode());
        }

        first = it;
    }

    void InsertAfter(Iterator& position, T& object) {
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

    void InsertBefore(Iterator& position, T& object) {
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

    ListNode<T>* Find(T& object) {
        ListNode<T>* result = nullptr;

        for (Iterator it = Begin(); it != End(); ++it) {
            if (&*it == &object) {
                result = &it.GetNode();
                break;
            }
        }

        return result;
    }

    bool Remove(T& object) {
        bool result;

        Iterator it = Find(object);

        if (it != nullptr) {
            Erase(it.GetNode());
            result = true;

        } else {
            result = false;
        }

        return result;
    }

    void Remove(Iterator& position) { Erase(position.GetNode()); }

    void Clear() {
        while (first != nullptr) {
            last = first->next;
            first->prev = nullptr;
            first->next = nullptr;
            first = last;
        }

        last = nullptr;
        count = 0;
    }

    unsigned short GetCount() const { return count; }

    int GetMemorySize() const { return count * sizeof(ListNode<T>) + sizeof(count); }

    SmartList<T>& operator=(const SmartList<T>& other) {
        Clear();

        for (Iterator it = other.Begin(); it != other.End(); ++it) {
            PushBack(*it);
        }

        return *this;
    }

    T& operator[](unsigned short index) const { return *GetByIndex(index).GetObject(); }
};

#endif /* SMARTLIST_HPP */
