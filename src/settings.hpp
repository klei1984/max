/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

class Settings {
public:
    using SettingValue = std::variant<int32_t, std::string>;

    struct SettingDefinition {
        SettingValue default_value;
        std::string section;
    };

    Settings();
    ~Settings();

    [[nodiscard]] bool Load();
    [[nodiscard]] bool Save();

    [[nodiscard]] int32_t GetNumericValue(const std::string& key, int32_t default_value = 0) const;
    [[nodiscard]] std::string GetStringValue(const std::string& key, const std::string& default_value = "") const;

    void SetNumericValue(const std::string& key, int32_t value);
    void SetStringValue(const std::string& key, const std::string& value);

private:
    [[nodiscard]] std::string LoadSchema();
    [[nodiscard]] bool LoadScript(const std::string& script);

    std::filesystem::path m_filepath;
    std::unique_ptr<std::unordered_map<std::string, SettingValue>> m_loaded_settings;
    std::unique_ptr<std::vector<std::pair<std::string, SettingDefinition>>> m_setting_definitions;
};

#endif /* SETTINGS_HPP */
