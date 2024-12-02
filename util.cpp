#include "util.hpp"

using namespace util;

std::string util::utf16ToUtf8(char* str, size_t n) {
    if (n % 2) {
        return "";
    }
    std::string res;
    for (size_t i = 0; i < n; i += 2, str += 2) {
        uint32_t codePoint = *(uint16_t*)(str);
        if (codePoint <= 0x7f) {
            res += codePoint;
        }
        else if (codePoint <= 0x7ff) {
            uint16_t ch = 0b1000000011000000;
            ch |= (codePoint & 0b11111000000) >> 6;
            ch |= (codePoint & 0b111111) << 8;
            res.append((char*)&ch, 2);
        }
        else if ((codePoint >= 0x800 && codePoint <= 0xd7ff) || (codePoint >= 0xe000 && codePoint <= 0xffff)) {
            uint32_t ch = 0b00000000100000001000000011100000;
            ch |= (codePoint & 0b1111000000000000) >> 12;
            ch |= (codePoint & 0b111111000000) << 2;
            ch |= (codePoint & 0b00111111) << 16;
            res.append(((char*)&ch), 3);
        }
        // surrogates - decoding 4 bytes
        else if (codePoint >= 0xd800 && codePoint <= 0xdfff) {
            if ((n - i) < 4) {
                return "";
            }
            uint16_t codePointP1 = codePoint - 0xd800;
            str += 2;
            i += 2;
            uint16_t codePointP2 = *(uint16_t*)(str);
            if (codePointP2 < 0xdc00) {
                return "";
            }
            codePointP2 -= 0xdc00;
            codePoint = ((uint32_t)0x10000 | (codePointP1 << 10)) | codePointP2;
            uint32_t ch = 0b10000000100000001000000011110000;
            ch |= (codePoint & 0b111000000000000000000) >> 18;
            ch |= (codePoint & 0b111111000000000000) >> 4;
            ch |= (codePoint & 0b111111000000) << 10;
            ch |= (codePoint & 0b111111) << 24;
            res.append((char*)&ch, 4);
        }
        else {
            return "";
        }
    }
    return res;
}

std::string util::asciiToUtf8(char* str, size_t n) {
    std::string res;
    for (size_t i = 0; i < n; ++i, ++str) {
        uint16_t codePoint = *(uint8_t*)(str);
        if (codePoint <= 0x7f) {
            res += codePoint;
        }
        else if (codePoint <= 0xff) {
            uint16_t ch = 0b1000000011000000;
            ch |= (codePoint & 0b11111000000) >> 6;
            ch |= (codePoint & 0b111111) << 8;
            res.append((char*)&ch, 2);
        }
        else {
            return "";
        }
    }
    return res;
}

std::basic_string<char16_t> util::utf8ToUtf16(char* str, size_t n) {
    std::basic_string<char16_t> res;
    uint8_t bytes[4];
    for (size_t i = 0; i < n; ++i, ++str) {
        bytes[0] = *str;
        // 1 byte
        if (((~(bytes[0] ^ 0b00000000)) & 0b10000000) == 0b10000000) {
            char16_t ch = 0;
            ch |= bytes[0];
            res += ch;
        }
        // 2 bytes
        else if (((~(bytes[0] ^ 0b11000000)) & 0b11100000) == 0b11100000) {
            ++str;
            ++i;
            bytes[1] = *str;
            if (!((~(bytes[1] ^ 0b10000000)) & 0b11000000)) {
                return u"";
            }
            char16_t ch = (bytes[1] & 0b00111111) | ((bytes[0] & 0b00011111) << 6);
            res += ch;
        }
        // 3 bytes
        else if (((~(bytes[0] ^ 0b11100000)) & 0b11110000) == 0b11110000) {
            for (size_t j = 1; j < 3; ++j) {
                ++str;
                ++i;
                bytes[j] = *str;
                if (!((~(bytes[j] ^ 0b10000000)) & 0b11000000)) {
                    return u"";
                }
            }
            char16_t ch = (bytes[2] & 0b00111111) | ((bytes[1] & 0b00111111) << 6) | ((bytes[0] & 0b00001111) << 12);
            res += ch;
        }
        // 4 bytes
        else if (((~(bytes[0] ^ 0b11110000)) & 0b11111000) == 0b11111000) {
            for (size_t j = 1; j < 4; ++j) {
                ++str;
                ++i;
                bytes[j] = *str;
                if (!((~(bytes[j] ^ 0b10000000)) & 0b11000000)) {
                    return u"";
                }
            }
            char16_t ch = (bytes[3] & 0b00111111) | ((bytes[2] & 0b00111111) << 6) | ((bytes[1] & 0b00001111) << 12);
            char16_t ch2 = ((bytes[1] & 0b00110000) >> 4) | ((bytes[0] & 0b00000111) << 2);
            ch2 -= 1;
            res += (((ch & 0b1111110000000000) >> 10) | ((ch2 & 0b1111) << 6)) + 0xd800;
            res += (ch & 0b1111111111) + 0xdc00;
        }
    }
    return res;
}

std::string util::utf8ToAscii(char* str, size_t n) {
    std::string res;
    for (size_t i = 0; i < n; ++i, ++str) {
        uint8_t byte = *str;
        if (((~(byte ^ 0b00000000)) & 0b10000000) == 0b10000000) {
            char16_t ch = 0;
            ch |= byte;
            res += ch;
        }
        else if (((~(byte ^ 0b11000000)) & 0b11100000) == 0b11100000) {
            ++str;
            ++i;
            uint8_t byte2 = *str;
            if (!((~(byte2 ^ 0b10000000)) & 0b11000000)) {
                return "";
            }
            char16_t ch = (byte2 & 0b00111111) | ((byte & 0b00011111) << 6);
            res += ch;
        }
    }
    return res;
}
