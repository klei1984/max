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

extern "C" {
#include "ini.h"
}

class IniSettings {
public:
    IniSettings();
    ~IniSettings();
    void Init();
    void Save();
    int SetNumericValue(GAME_INI param, int value);
    int GetNumericValue(GAME_INI param);
    int SetStringValue(GAME_INI param, char *buffer);
    int GetStringValue(GAME_INI param, char *buffer, int buffer_size);
    void SaveSection(void *smartfile_handle, GAME_INI section);
    void LoadSection(void *smartfile_handle, GAME_INI section, char mode);

private:
    void Destroy();
    static const char *SeekToSection(GAME_INI param);

    Ini_descriptor ini;
    unsigned int item[118];
};

class IniClans {
public:
    IniClans();
    ~IniClans();
    void Init();
    int SeekUnit(int clan, int unit);
    int GetNextUnitUpgrade(short *attrib_id, short *value);
    void GetStringValue(char *buffer, int buffer_size);
    int GetClanGold(int clan);
    void GetClanText(int clan, char *buffer, int buffer_size);
    void GetClanName(int clan, char *buffer, int buffer_size);

private:
    Ini_descriptor ini;
    char buffer[20];
};

class IniSoundVolumes {
public:
    IniSoundVolumes();
    ~IniSoundVolumes();
    void Init();
    int GetUnitVolume(GAME_RESOURCE id);

private:
    Ini_descriptor ini;
};

extern IniSettings ini_config;
extern IniClans ini_clans;

#endif /* INIFILE_HPP */
