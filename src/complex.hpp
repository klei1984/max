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

#ifndef COMPLEX_HPP
#define COMPLEX_HPP

#include "cargo.hpp"
#include "textfile.hpp"

class UnitInfo;
class NetPacket;

class Complex : public TextFileObject {
    short buildings;
    short id;

    static void TransferCargo(UnitInfo* unit, int* cargo);

public:
    Complex(short id);
    ~Complex();

    static TextFileObject* Allocate();

    short GetId() const;

    unsigned short GetTypeIndex() const;
    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);

    void WritePacket(NetPacket& packet);
    void ReadPacket(NetPacket& packet);

    void Grow(UnitInfo& unit);
    void Shrink(UnitInfo& unit);

    void GetCargoMinable(Cargo& capacity);
    void GetCargoMining(Cargo& materials, Cargo& capacity);
    void GetCargoInfo(Cargo& materials, Cargo& capacity);

    void Transfer(int raw, int fuel, int gold);

    short material;
    short fuel;
    short gold;
    short power;
    short workers;
};

#endif /* COMPLEX_HPP */
