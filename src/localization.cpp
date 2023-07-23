/* Copyright (c) 2022 M.A.X. Port Team
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

#include "localization.hpp"

#include <filesystem>
#include <iostream>
#include <memory>

#include "ini.hpp"
#include "lang_config.hpp"

#define LOCALIZATION_BUFFER_SIZE 2048

static std::unique_ptr<Localization> Localization_Locale;

enum IniKeyValueType_e {
    INI_SECTION = 0x1,
    INI_STRING = 0x4,
};

struct IniKey {
    const char* const name;
    const IniKeyValueType_e type;
};

static const IniKey Localization_IniKeysTable[] = {LOCALIZATION_INI_MAP};

Localization::Localization() {}

Localization::~Localization() {}

std::string Localization::GetLanguage() const {
    auto buffer = std::make_unique<char[]>(LOCALIZATION_BUFFER_SIZE);
    auto filepath = std::filesystem::current_path() / "settings.ini";
    Ini_descriptor ini;
    std::string result("english");

    if (inifile_init_ini_object_from_ini_file(&ini, filepath.string().c_str())) {
        if (inifile_ini_seek_section(&ini, "SETUP") && inifile_ini_seek_param(&ini, "language")) {
            if (inifile_ini_get_string(&ini, buffer.get(), LOCALIZATION_BUFFER_SIZE, 1)) {
                result = buffer.get();
            }
        }

        inifile_save_to_file_and_free_buffer(&ini, true);
    }

    return result;
}

int32_t Localization::Load() {
    auto filepath = std::filesystem::current_path() / (std::string("lang_") + GetLanguage() + ".ini");
    Ini_descriptor ini;

    if (!inifile_init_ini_object_from_ini_file(&ini, filepath.string().c_str())) {
        return 0;
    }

    strings.clear();

    auto buffer = std::make_unique<char[]>(LOCALIZATION_BUFFER_SIZE);
    const IniKey* section_entry = nullptr;

    for (const auto& entry : Localization_IniKeysTable) {
        if (entry.type & INI_SECTION) {
            section_entry = &entry;

        } else if (entry.type & INI_STRING) {
            if (inifile_ini_seek_section(&ini, section_entry->name) && inifile_ini_seek_param(&ini, entry.name)) {
                if (inifile_ini_get_string(&ini, buffer.get(), LOCALIZATION_BUFFER_SIZE, 1, false)) {
                    auto write_address = buffer.get();
                    for (auto read_address = write_address; *read_address; ++read_address, ++write_address) {
                        if (read_address[0] == '\\' and read_address[1] == 'n') {
                            ++read_address;
                            read_address[0] = '\n';
                        }

                        write_address[0] = read_address[0];
                    }

                    write_address[0] = '\0';

                    strings.insert({entry.name, buffer.get()});
                }
            }
        }
    }

    inifile_save_to_file_and_free_buffer(&ini, true);

    return 1;
}

const char* Localization::GetText(const char* key) {
    if (!Localization_Locale) {
        Localization_Locale = std::make_unique<Localization>(Localization());

        if (!Localization_Locale->Load()) {
            std::ios_base::Init init;  /// required to initialize standard streams in time
            std::cerr << "\nThe localization file configured in settings.ini is not found!\n\n";
            std::exit(1);
        }
    }

    if (Localization_Locale->strings.contains(key)) {
        return Localization_Locale->strings[key].c_str();

    } else {
        return key;
    }
}
