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

#ifndef TEAMUNITS_HPP
#define TEAMUNITS_HPP

#include "complex.hpp"
#include "resource_manager.hpp"
#include "smartlist.hpp"
#include "unitvalues.hpp"

class TeamUnits {
    unsigned short color_field_0;
    unsigned short color_field_2;
    unsigned short gold;
    SmartPointer<UnitValues> base_values[UNIT_END];
    SmartPointer<UnitValues> current_values[UNIT_END];
    SmartList<Complex> complexes;

public:
    TeamUnits();
    ~TeamUnits();

    void FileLoad(SmartFileReader& file);
    void FileSave(SmartFileWriter& file);
    void TextLoad(TextStructure& object);
    void TextSave(SmartTextfileWriter& file);

    int WriteComplexPacket(unsigned short complex_id, void* buffer);
    void ReadComplexPacket(void* buffer);

    unsigned short GetGold();
    void SetGold(unsigned short value);

    Complex* CreateComplex();
    Complex* GetComplex(unsigned short complex_id);
    void sub_7F3DC(unsigned short team);
    void RemoveComplex(Complex& object);

    UnitValues* GetBaseUnitValues(unsigned short id);

    UnitValues* GetCurrentUnitValues(unsigned short id);
    void SetCurrentUnitValues(unsigned short id, UnitValues& object);
};

#endif /* TEAMUNITS_HPP */
