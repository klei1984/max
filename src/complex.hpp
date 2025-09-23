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
#include "smartfile.hpp"

class UnitInfo;
class NetPacket;

class Complex : public FileObject {
    int16_t buildings;
    int16_t id;

    static void TransferCargo(UnitInfo* unit, int32_t* cargo);

public:
    Complex(int16_t id);
    ~Complex();

    static FileObject* Allocate() noexcept;

    int16_t GetId() const;

    uint32_t GetTypeIndex() const;
    void FileLoad(SmartFileReader& file) noexcept;
    void FileSave(SmartFileWriter& file) noexcept;

    void WritePacket(NetPacket& packet);
    void ReadPacket(NetPacket& packet);

    void Grow(UnitInfo& unit);
    void Shrink(UnitInfo& unit);

    void GetCargoMinable(Cargo& capacity);
    void GetCargoMining(Cargo& materials, Cargo& capacity);
    void GetCargoInfo(Cargo& materials, Cargo& capacity);

    void Transfer(int32_t raw, int32_t fuel, int32_t gold);

    int16_t GetBuildings() const;

    int16_t material;
    int16_t fuel;
    int16_t gold;
    int16_t power;
    int16_t workers;
};

#endif /* COMPLEX_HPP */
