/* Copyright (c) 2020 M.A.X. Port Team
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

#ifndef INIFILE_HPP
#define INIFILE_HPP

#include "enums.hpp"
#include "ini.hpp"
#include "smartfile.hpp"

enum : uint8_t {
    OPPONENT_TYPE_CLUELESS,
    OPPONENT_TYPE_APPRENTICE,
    OPPONENT_TYPE_AVERAGE,
    OPPONENT_TYPE_EXPERT,
    OPPONENT_TYPE_MASTER,
    OPPONENT_TYPE_GOD
};

class IniSettings {
public:
    IniSettings();
    ~IniSettings();
    void Init();
    void Save();
    int32_t SetNumericValue(IniParameter param, int32_t value);
    int32_t GetNumericValue(IniParameter param);
    int32_t SetStringValue(IniParameter param, const char *buffer);
    int32_t GetStringValue(IniParameter param, char *buffer, int32_t buffer_size);
    void SaveSection(SmartFileWriter &file, IniParameter section);
    void LoadSection(SmartFileReader &file, IniParameter section, char mode);

private:
    void Destroy();
    static const char *SeekToSection(IniParameter param);

    Ini_descriptor ini;
    uint32_t *items;
};

class IniClans {
public:
    IniClans();
    ~IniClans();
    void Init();
    int32_t SeekUnit(int32_t clan, int32_t unit);
    int32_t GetNextUnitUpgrade(int16_t *attrib_id, int16_t *value);
    void GetStringValue(char *buffer, int32_t buffer_size);
    int32_t GetClanGold(int32_t clan);
    void GetClanText(int32_t clan, char *buffer, int32_t buffer_size);
    void GetClanName(int32_t clan, char *buffer, int32_t buffer_size);

private:
    Ini_descriptor ini;
    char buffer[20];
};

class IniSoundVolumes {
    Ini_descriptor ini;

public:
    IniSoundVolumes();
    ~IniSoundVolumes();
    void Init();
    int32_t GetUnitVolume(ResourceID id);
};

extern IniSettings ini_config;
extern IniClans ini_clans;

extern int32_t ini_setting_victory_type;
extern int32_t ini_setting_victory_limit;

#endif /* INIFILE_HPP */
