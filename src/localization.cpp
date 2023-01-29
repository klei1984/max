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

#define LOCALIZATION_BUFFER_SIZE 1000

static std::unique_ptr<Localization> Localization_Locale;

enum IniKeyValueType_e {
    INI_SECTION = 0x1,
    INI_STRING = 0x4,
};

struct IniKey {
    const char* const name;
    const IniKeyValueType_e type;
};

static const IniKey Localization_IniKeysTable[] = {
    {"main_menu", INI_SECTION},
    {"35f9", INI_STRING},
    {"28f9", INI_STRING},
    {"dc41", INI_STRING},
    {"8339", INI_STRING},
    {"7013", INI_STRING},
    {"b605", INI_STRING},
    {"2a6b", INI_STRING},
    {"d17d", INI_STRING},
    {"new_game_menu", INI_SECTION},
    {"9b46", INI_STRING},
    {"5d49", INI_STRING},
    {"3d29", INI_STRING},
    {"884f", INI_STRING},
    {"385b", INI_STRING},
    {"3838", INI_STRING},
    {"1976", INI_STRING},
    {"network_game_menu", INI_SECTION},
    {"c7a9", INI_STRING},
    {"7064", INI_STRING},
    {"f731", INI_STRING},
    {"bf00", INI_STRING},
    {"847e", INI_STRING},
    {"0de3", INI_STRING},
    {"credits_menu", INI_SECTION},
    {"aabe", INI_STRING},
    {"84ee", INI_STRING},
    {"8984", INI_STRING},
    {"aa55", INI_STRING},
    {"3c7e", INI_STRING},
    {"24ef", INI_STRING},
    {"5fe6", INI_STRING},
    {"3215", INI_STRING},
    {"38c1", INI_STRING},
    {"363f", INI_STRING},
    {"477a", INI_STRING},
    {"fff7", INI_STRING},
    {"47c6", INI_STRING},
    {"9a25", INI_STRING},
    {"7ed6", INI_STRING},
    {"0822", INI_STRING},
    {"c0bb", INI_STRING},
    {"4a00", INI_STRING},
    {"d200", INI_STRING},
    {"88c0", INI_STRING},
    {"f823", INI_STRING},
    {"796e", INI_STRING},
    {"bf4a", INI_STRING},
    {"79e5", INI_STRING},
    {"e3e3", INI_STRING},
    {"8433", INI_STRING},
    {"d5e4", INI_STRING},
    {"25b0", INI_STRING},
    {"1d8d", INI_STRING},
    {"11dd", INI_STRING},
    {"3a89", INI_STRING},
    {"e13c", INI_STRING},
    {"f7fe", INI_STRING},
    {"b3c3", INI_STRING},
    {"65d0", INI_STRING},
    {"85e2", INI_STRING},
    {"3c03", INI_STRING},
    {"afad", INI_STRING},
    {"181c", INI_STRING},
    {"02ff", INI_STRING},
    {"5dab", INI_STRING},
    {"bec3", INI_STRING},
    {"e725", INI_STRING},
    {"5c49", INI_STRING},
    {"6a6b", INI_STRING},
    {"9b23", INI_STRING},
    {"f7ea", INI_STRING},
    {"3a41", INI_STRING},
    {"f813", INI_STRING},
    {"11ae", INI_STRING},
    {"06d1", INI_STRING},
    {"b2a8", INI_STRING},
    {"7a22", INI_STRING},
    {"efb3", INI_STRING},
    {"fda7", INI_STRING},
    {"ab0a", INI_STRING},
    {"163c", INI_STRING},
    {"3ce2", INI_STRING},
    {"d126", INI_STRING},
    {"00a5", INI_STRING},
    {"7bd6", INI_STRING},
    {"planet_descriptions", INI_SECTION},
    {"1471", INI_STRING},
    {"c336", INI_STRING},
    {"f40f", INI_STRING},
    {"6c6c", INI_STRING},
    {"8d9a", INI_STRING},
    {"cb1d", INI_STRING},
    {"5195", INI_STRING},
    {"012c", INI_STRING},
    {"323e", INI_STRING},
    {"57a1", INI_STRING},
    {"6f14", INI_STRING},
    {"708f", INI_STRING},
    {"6903", INI_STRING},
    {"4d80", INI_STRING},
    {"ae4e", INI_STRING},
    {"d0a4", INI_STRING},
    {"fdd7", INI_STRING},
    {"0ac0", INI_STRING},
    {"5f4b", INI_STRING},
    {"8475", INI_STRING},
    {"cffe", INI_STRING},
    {"7842", INI_STRING},
    {"da7e", INI_STRING},
    {"6517", INI_STRING},
    {"planet_titles", INI_SECTION},
    {"e43b", INI_STRING},
    {"f588", INI_STRING},
    {"c78b", INI_STRING},
    {"895d", INI_STRING},
    {"5f5f", INI_STRING},
    {"e7b2", INI_STRING},
    {"f3fe", INI_STRING},
    {"8524", INI_STRING},
    {"4bb8", INI_STRING},
    {"f408", INI_STRING},
    {"0935", INI_STRING},
    {"7303", INI_STRING},
    {"94ef", INI_STRING},
    {"c46c", INI_STRING},
    {"48ac", INI_STRING},
    {"275a", INI_STRING},
    {"ea47", INI_STRING},
    {"fcf0", INI_STRING},
    {"6426", INI_STRING},
    {"7ea8", INI_STRING},
    {"386d", INI_STRING},
    {"41e5", INI_STRING},
    {"bbcb", INI_STRING},
    {"ba99", INI_STRING},
    {"team_names", INI_SECTION},
    {"3bda", INI_STRING},
    {"3bb7", INI_STRING},
    {"11a8", INI_STRING},
    {"75a0", INI_STRING},
    {"game_over_screen", INI_SECTION},
    {"153d", INI_STRING},
    {"6b25", INI_STRING},
    {"aaea", INI_STRING},
    {"bb8a", INI_STRING},
    {"cdd0", INI_STRING},
    {"71f1", INI_STRING},
    {"77de", INI_STRING},
    {"bb4a", INI_STRING},
    {"fc74", INI_STRING},
    {"7564", INI_STRING},
    {"c9c5", INI_STRING},
    {"7b17", INI_STRING},
    {"51e1", INI_STRING},
    {"3d2c", INI_STRING},
    {"5c73", INI_STRING},
    {"e02a", INI_STRING},
    {"6249", INI_STRING},
};

Localization::Localization() {}

Localization::~Localization() {}

std::string Localization::GetLanguage() const {
    auto buffer = std::make_unique<char[]>(LOCALIZATION_BUFFER_SIZE);
    auto filepath = std::filesystem::current_path() / "max.ini";
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

int Localization::Load() {
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
                if (inifile_ini_get_string(&ini, buffer.get(), LOCALIZATION_BUFFER_SIZE, 1)) {
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
            std::cerr << "\nThe localization file configured in max.ini is not found!\n\n";
            std::exit(1);
        }
    }

    if (Localization_Locale->strings.contains(key)) {
        return Localization_Locale->strings[key].c_str();

    } else {
        return key;
    }
}
