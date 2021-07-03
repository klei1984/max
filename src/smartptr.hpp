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

#ifndef SMARTPTR_HPP
#define SMARTPTR_HPP

class SmartObject {
    friend class SmartPointer;
    unsigned short reference_count;

public:
    SmartObject();
    SmartObject(const SmartObject& other);
    virtual ~SmartObject();
};

class SmartPointer {
    SmartObject* object_pointer;

public:
    SmartPointer();
    SmartPointer(SmartObject* object);
    SmartPointer(const SmartPointer& other);
    ~SmartPointer();

    void Increment() const;
    void Decrement();

    SmartObject* operator->() const;
    SmartObject& operator*() const;
    SmartPointer& operator=(const SmartPointer& other);
    SmartPointer& operator=(SmartObject& other);
    bool check(bool test_is_null_mode) const;
    bool check_inverse(bool test_is_not_null_mode) const;
};

#endif /* SMARTPTR_HPP */
