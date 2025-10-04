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

#include "ini.hpp"

#include <cctype>
#include <cstdlib>

#include "resource_manager.hpp"

static const char* hex_to_dec_lut1 = "0123456789ABCDEF";
static uint32_t hex_to_dec_lut2[] = {0x00, 0x01, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000};

static int32_t inifile_hex_to_dec_digit(char c);

Ini_descriptor::Ini_descriptor() noexcept
    : flags(0),
      buffer(nullptr),
      file_size(0),
      buffer_size(0),
      current_address(nullptr),
      section_start_address(nullptr),
      key_start_address(nullptr),
      value_start_address(nullptr),
      next_value_address(nullptr),
      param_start_address(nullptr) {}

void inifile_load_from_resource(Ini_descriptor* const pini, ResourceID resource_id) {
    pini->ini_file_path.clear();
    pini->file_size = ResourceManager_GetResourceSize(resource_id);
    pini->buffer_size = pini->file_size;
    pini->buffer = reinterpret_cast<char*>(ResourceManager_ReadResource(resource_id));

    if (pini->buffer) {
        pini->current_address = pini->buffer;
        pini->value_start_address = nullptr;
        pini->next_value_address = nullptr;
        pini->key_start_address = nullptr;
        pini->param_start_address = nullptr;
        pini->flags &= ~0x80u;
    }
}

int32_t inifile_init_ini_object_from_ini_file(Ini_descriptor* const pini, const char* const inifile_path) {
    int32_t result;
    FILE* fp;

    pini->ini_file_path = std::filesystem::path(inifile_path).lexically_normal();
    fp = fopen(pini->ini_file_path.string().c_str(), "rb");

    if (fp == nullptr) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    pini->file_size = ftell(fp);
    pini->buffer_size = pini->file_size + 1024;
    fseek(fp, 0, SEEK_SET);
    pini->buffer = new (std::nothrow) char[pini->buffer_size + 2];

    if (pini->buffer) {
        if (fread(pini->buffer, sizeof(char), pini->file_size, fp) == pini->file_size) {
            fclose(fp);
            pini->buffer[pini->file_size] = '\r';
            pini->buffer[pini->file_size + 1] = '\n';
            pini->current_address = pini->buffer;
            pini->value_start_address = nullptr;
            pini->next_value_address = nullptr;
            pini->key_start_address = nullptr;
            pini->param_start_address = nullptr;
            pini->flags &= ~0x80u;
            result = 1;
        } else {
            fclose(fp);
            delete[] pini->buffer;
            pini->buffer = nullptr;
            result = 0;
        }
    } else {
        fclose(fp);
        result = 0;
    }

    return result;
}

int32_t inifile_save_to_file(Ini_descriptor* const pini) {
    FILE* fp;

    if (pini->buffer) {
        if (pini->flags & 0x80u) {
            fp = fopen(pini->ini_file_path.string().c_str(), "wb");

            if (fp == nullptr) {
                return 0;
            }

            fwrite(pini->buffer, sizeof(char), pini->file_size, fp);
            fclose(fp);
        }

        return 1;
    }

    return 1;
}

int32_t inifile_save_to_file_and_free_buffer(Ini_descriptor* const pini, bool free_only) {
    int32_t result = 1;

    if (!free_only) {
        if (!inifile_save_to_file(pini)) {
            result = 0;
        }
    }

    delete[] pini->buffer;
    pini->buffer = nullptr;

    return result;
}

int32_t inifile_ini_seek_section(Ini_descriptor* const pini, const char* const ini_section_name) {
    char* address;
    char* section_start;
    char* end_address;
    int32_t result;

    result = 0;

    if (!pini->buffer) {
        return 0;
    }

    address = pini->buffer;
    end_address = &address[pini->file_size];

    do {
        /* seek to section start */
        if (*address == '[') {
            int32_t offset = 0;

            section_start = address++;

            /* seek through section name */
            while (*address == ini_section_name[offset] && address < end_address) {
                address++;
                offset++;
            }

            /* seek section end */
            if (*address == ']' && ini_section_name[offset] == '\0') {
                result = 1;

                /* seek to start of key within section */
                while (address < end_address && *address != '\n') {
                    address++;
                }

                /* skip forward new line character */
                if (address < end_address) {
                    address++;
                }

                pini->param_start_address = address;
                pini->current_address = address;
                pini->section_start_address = section_start;
            }
        }

        /* skip forward */
        if (address < end_address) {
            address++;
        }
    } while (!result && address < end_address);

    return result;
}

int32_t inifile_ini_seek_param(Ini_descriptor* const pini, const char* const ini_param_name) {
    char* address;
    char* key_start;
    char* end_address;
    int32_t result;

    result = 0;

    address = pini->current_address;
    end_address = &pini->buffer[pini->file_size];

    do {
        int32_t offset = 0;

        if (*address == ini_param_name[offset]) {
            key_start = address;
            address++;
            offset++;

            while (*address == ini_param_name[offset] && address < end_address) {
                address++;
                offset++;
            }

            if (ini_param_name[offset] == '\0') {
                /* seek key end */
                while (address < end_address && *address != '=' && *address != '\r') {
                    address++;
                }

                if (address < end_address && *address == '=') {
                    address++;

                    pini->value_start_address = address;
                } else {
                    pini->value_start_address = nullptr;
                }

                pini->key_start_address = key_start;
                pini->next_value_address = nullptr;

                result = 1;
            }
        }

        if (!result) {
            /* skip current key value pair */
            while (address < end_address && *address != '\n') {
                address++;
            }
        }

        /* skip forward new line character */
        if (address < end_address) {
            address++;
        }
    } while (!result && address < end_address && *address != '[');

    return result;
}

int32_t inifile_ini_process_numeric_value(Ini_descriptor* const pini, int32_t* const value) {
    char number[30];
    char* address;
    char* end_address;
    uint32_t offset;

    if (pini->next_value_address) {
        address = pini->next_value_address;
    } else {
        address = pini->value_start_address;
    }

    end_address = &pini->buffer[pini->file_size];

    if (!address) {
        return 0;
    }

    /* seek through leading space */
    while (*address == ' ') {
        address++;
    }

    if (*address == '\r') {
        return 0;
    }

    offset = 0;

    /* copy number to local buffer */
    while (address < end_address && *address != '\r' && *address != ',' && *address != ' ') {
        number[offset] = *address;
        address++;
        offset++;
    }

    number[offset] = '\0';
    SDL_assert(offset < sizeof(number));

    if (!offset) {
        return 0;
    }

    /* seek through trailing space */
    while (address < end_address && *address == ' ') {
        address++;
    }

    if (*address == ',') {
        address++;
    }

    pini->next_value_address = address;

    if (number[1] == 'x') {
        *value = inifile_hex_to_dec(&number[2]);
    } else {
        *value = strtol(number, nullptr, 10);
    }

    return 1;
}

int32_t inifile_ini_process_string_value(Ini_descriptor* const pini, char* const buffer, const uint32_t buffer_size) {
    char* address;
    uint32_t offset;

    if (pini->next_value_address) {
        address = pini->next_value_address;
    } else {
        address = pini->value_start_address;
    }

    if (!address) {
        return 0;
    }

    offset = 0;

    /* seek through leading space */
    while (*address == ' ') {
        address++;
    }

    while (*address != '\r' && *address != ',' && (buffer_size - 1) > offset) {
        buffer[offset] = *address;
        address++;
        offset++;
    }

    buffer[offset] = '\0';

    if ((buffer_size - 1) == offset) {
        while (*address != '\r' && *address != ',') {
            address++;
        }
    }

    if (*address == ',') {
        address++;
    }

    pini->next_value_address = address;

    return 1;
}

int32_t inifile_ini_get_string(Ini_descriptor* const pini, char* const buffer, const uint32_t buffer_size,
                               const int32_t mode, bool skip_leading_white_space) {
    char* address;
    char* end_address;
    uint32_t offset;

    if (mode) {
        address = pini->value_start_address;
    } else {
        address = pini->param_start_address;
    }

    *buffer = '\0';

    if (!address || *address == '[' || *address == '\r') {
        return 0;
    }

    end_address = &pini->buffer[pini->file_size];
    offset = 0;

    if (skip_leading_white_space) {
        /* seek through leading space */
        while (*address == ' ') {
            address++;
        }
    }

    while (*address != '\r' && (buffer_size - 1) > offset) {
        buffer[offset] = *address;
        address++;
        offset++;
    }

    buffer[offset] = '\0';
    address += 2;

    if (address < end_address) {
        pini->param_start_address = address;
    } else {
        pini->param_start_address = nullptr;
    }

    return 1;
}

int32_t inifile_ini_set_numeric_value(Ini_descriptor* const pini, const int32_t value) {
    char buffer[30];
    char* source_address;
    char* address;
    uint32_t offset;
    uint32_t length;
    size_t size;

    if (!pini->value_start_address) {
        return 0;
    }

    address = pini->value_start_address;

    /* seek through leading space */
    while (*address == ' ') {
        address++;
    }

    source_address = address;
    offset = 0;

    /* copy old value */
    while (*address != '\r') {
        buffer[offset] = *address;
        address++;
        offset++;
    }

    buffer[offset] = '\0';

    if (offset > 1 && buffer[1] == 'x') {
        snprintf(buffer, sizeof(buffer), "%#x", value);
    } else {
        snprintf(buffer, sizeof(buffer), "%d", value);
    }

    length = strlen(buffer);

    if (length >= offset) {
        if (length > offset) {
            if ((pini->file_size + length - offset) > pini->buffer_size) {
                return 0;
            }

            size = &pini->buffer[pini->file_size + length - offset] - source_address;
            memmove(&source_address[length - offset], source_address, size);
            pini->file_size += length - offset;
        }
    } else {
        size = &pini->buffer[pini->file_size + length - offset] - source_address;
        memmove(source_address, &source_address[offset - length], size);
        pini->file_size -= offset - length;
    }

    offset = 0;

    while (buffer[offset]) {
        *source_address = buffer[offset];
        offset++;
        source_address++;
    }

    pini->flags |= 0x80u;

    return 1;
}

int32_t inifile_ini_set_string_value(Ini_descriptor* const pini, const char* value) {
    char* address;
    char* source_address;
    uint32_t offset;
    uint32_t length;
    size_t size;

    address = pini->value_start_address;

    if (!address) {
        return 0;
    }

    /* seek through leading space */
    while (*address == ' ') {
        address++;
    }

    source_address = address;
    offset = 0;

    while (*address != '\r') {
        address++;
        offset++;
    }

    address++;

    length = strlen(value);

    if (length >= offset) {
        if (length > offset) {
            if ((pini->file_size + length - offset) > pini->buffer_size) {
                return 0;
            }

            size = &pini->buffer[pini->file_size + length - offset] - source_address;
            memmove(&source_address[length - offset], source_address, size);
            pini->file_size += length - offset;
        }
    } else {
        size = &pini->buffer[pini->file_size + length - offset] - source_address;
        memmove(source_address, &source_address[offset - length], size);
        pini->file_size -= offset - length;
    }

    while (*value) {
        *source_address = *value;
        value++;
        source_address++;
    }

    pini->flags |= 0x80u;

    return 1;
}

int32_t inifile_get_boolean_value(Ini_descriptor* const pini, const char* const ini_param_name) {
    char buffer[30];

    if (!inifile_ini_seek_param(pini, ini_param_name)) {
        return 0;
    }

    if (inifile_ini_process_string_value(pini, buffer, sizeof(buffer))) {
        return SDL_strcasecmp(buffer, "Yes") == 0;
    }

    return 0;
}

int32_t inifile_ini_get_numeric_value(Ini_descriptor* const pini, const char* const ini_param_name,
                                      int32_t* const value) {
    int32_t result;

    if (inifile_ini_seek_param(pini, ini_param_name)) {
        result = inifile_ini_process_numeric_value(pini, value) != 0;
    } else {
        result = 0;
    }

    return result;
}

int32_t inifile_ini_get_string_value(Ini_descriptor* const pini, const char* const ini_param_name, char* const buffer,
                                     const int32_t buffer_size) {
    int32_t result;

    if (inifile_ini_seek_param(pini, ini_param_name)) {
        result = inifile_ini_process_string_value(pini, buffer, buffer_size) != 0;
    } else {
        result = 0;
    }

    return result;
}

int32_t inifile_set_boolean_value(Ini_descriptor* const pini, const char* const ini_param_name, const int32_t value) {
    if (inifile_ini_seek_param(pini, ini_param_name)) {
        if (value) {
            if (!inifile_ini_set_string_value(pini, "Yes")) {
                return 0;
            }
        } else if (!inifile_ini_set_string_value(pini, "No")) {
            return 0;
        }

        return 1;
    }

    return 0;
}

int32_t inifile_delete_param(Ini_descriptor* const pini, const char* const ini_param_name) {
    char* address;
    uint32_t offset;

    offset = 0;

    if (!inifile_ini_seek_param(pini, ini_param_name)) {
        return 0;
    }

    address = pini->key_start_address;

    while (address[offset] != '\n') {
        offset++;
    }

    offset++;

    memmove(address, &address[offset], &pini->buffer[pini->file_size - offset] - address);
    pini->file_size -= offset;
    pini->flags |= 0x80u;

    return 1;
}

int32_t inifile_delete_section(Ini_descriptor* const pini, char* param_name) {
    char* end_address;
    char* address;
    uint32_t offset;

    if (!inifile_ini_seek_section(pini, param_name)) {
        return 0;
    }

    address = pini->section_start_address;
    end_address = &pini->buffer[pini->file_size];

    offset = 1;

    while ((address[offset] != '[') && (&address[offset] < end_address)) {
        offset++;
    }

    memmove(address, &address[offset], end_address - &address[offset]);
    pini->file_size -= offset;
    pini->flags |= 0x80u;

    return 1;
}

int32_t inifile_add_section(Ini_descriptor* const pini, char* ini_section_name) {
    char* address;
    uint32_t length;
    uint32_t offset;

    if (inifile_ini_seek_section(pini, ini_section_name)) {
        return 0;
    }

    address = &pini->buffer[pini->file_size];
    length = strlen(ini_section_name) + 6;

    if ((length + pini->file_size) > pini->buffer_size) {
        return 0;
    }

    offset = 0;
    address[offset] = '\r';
    offset++;

    address[offset] = '\n';
    offset++;

    address[offset] = '[';
    pini->section_start_address = &address[offset];
    offset++;

    while (*ini_section_name) {
        address[offset] = *ini_section_name;
        offset++;
        ini_section_name++;
    }

    address[offset] = ']';
    offset++;

    address[offset] = '\r';
    offset++;

    address[offset] = '\n';
    offset++;

    pini->param_start_address = &address[offset];
    pini->current_address = &address[offset];

    pini->file_size += length;
    pini->flags |= 0x80u;

    return 1;
}

int32_t inifile_add_string_param(Ini_descriptor* const pini, char* ini_param_name, char* buffer, int32_t buffer_size) {
    int32_t result;
    char* src;
    int32_t length;

    if (inifile_ini_seek_param(pini, ini_param_name)) {
        inifile_ini_set_string_value(pini, buffer);
        result = 1;
    } else {
        src = pini->current_address;
        length = strlen(buffer) + buffer_size + 4;

        if ((pini->file_size + length) <= pini->buffer_size) {
            memmove(&src[length], src, &pini->buffer[pini->file_size] - src);

            while (*ini_param_name) {
                *src = *ini_param_name;
                src++;
                ini_param_name++;
                --buffer_size;
            }

            while (--buffer_size != -1) {
                *src = ' ';
                src++;
            }

            *src = '=';
            src++;
            *src = ' ';
            src++;

            while (*buffer) {
                *src = *buffer;
                buffer++;
                src++;
            }

            *src = '\r';
            src++;
            *src = '\n';

            pini->file_size += length;
            pini->flags |= 0x80u;

            result = 1;
        } else {
            result = 0;
        }
    }

    return result;
}

int32_t inifile_add_numeric_param(Ini_descriptor* const pini, char* ini_param_name, int32_t value, int32_t size,
                                  int32_t radix) {
    char buffer[30];
    char* src;
    int32_t length;
    int32_t offset;
    int32_t result;

    if (inifile_ini_seek_param(pini, ini_param_name)) {
        inifile_ini_set_numeric_value(pini, value);
        result = 1;
    } else {
        if (radix == 16) {
            snprintf(buffer, sizeof(buffer), "%#x", value);
        } else {
            snprintf(buffer, sizeof(buffer), "%d", value);
        }

        src = pini->current_address;
        length = strlen(buffer) + size + 4;

        if ((pini->file_size + length) <= pini->buffer_size) {
            memmove(&src[length], src, &pini->buffer[pini->file_size] - src);

            while (*ini_param_name) {
                *src = *ini_param_name;
                src++;
                ini_param_name++;
                --size;
            }

            while (--size != -1) {
                *src = ' ';
                src++;
            }

            *src = '=';
            src++;
            *src = ' ';
            src++;

            offset = 0;

            while (buffer[offset]) {
                *src = buffer[offset];
                offset++;
                src++;
            }

            *src = '\r';
            src++;
            *src = '\n';
            src++;

            pini->file_size += length;
            pini->flags |= 0x80u;

            result = 1;
        } else {
            result = 0;
        }
    }

    return result;
}

uint32_t inifile_hex_to_dec(const char* const hex) {
    uint32_t sum;
    int32_t length;
    int32_t offset;

    sum = 0;
    offset = 0;

    length = strlen(hex);

    SDL_assert(length > 0);

    do {
        sum += hex_to_dec_lut2[length] * inifile_hex_to_dec_digit(hex[offset]);
        offset++;
        length--;
    } while (length);

    return sum;
}

static int32_t inifile_hex_to_dec_digit(const char c) {
    for (uint32_t index = 0; index < 16; index++) {
        if (hex_to_dec_lut1[index] == toupper(c)) {
            return index;
        }
    }

    return -1;
}
