#include "ID3V2Parser.hpp"

uint32_t swapU32(uint32_t number) {
    return ((number & 0xff) << 24) | ((number & 0xff00) << 8) | ((number & 0xff0000) >> 8) | ((number & 0xff000000) >> 24);
}

uint16_t swapU16(uint16_t number) {
    return ((number & 0xff) << 8) | ((number & 0xff00) >> 8);
}

std::string utf16ToUtf8(char* str, size_t n) {
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

std::string asciiToUtf8(char* str, size_t n) {
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

std::basic_string<char16_t> utf8ToUtf16(char* str, size_t n) {
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

std::string utf8ToAscii(char* str, size_t n) {
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

ID3V2Extractor::ID3V2Extractor(std::ifstream& fs) {
    if (!checkFile(fs) || extractHeader(fs) || extractFrames(fs)) {
        throw std::runtime_error("invalid file");
    }
    if (unsynchronisation()) {
        throw std::runtime_error("unsupported file: unsynchronization");
    }
    if (extendedHeader()) {
        throw std::runtime_error("unsupported file: extended headers");
    }
    if (extractFrames(fs)) {
        throw std::runtime_error("unsupported file: invalid tags");
    }
}

bool ID3V2Extractor::checkFile(std::ifstream& fs) {
    std::unique_ptr<uint8_t[]> data(new uint8_t[5]);
    fs.read((char*)data.get(), 5);
    // 0x49 0x44 0x33 - ID3 string
    // 0x03 0x00 - version
    return
        data[0] == 0x49 &&
        data[1] == 0x44 &&
        data[2] == 0x33 &&
        data[3] == 0x03  &&
        data[4] == 0x00;

}

int ID3V2Extractor::extractHeader(std::ifstream& fs) {
    fs.read((char*)&_flags, 1);
    fs.read((char*)&_size, 4);
    _size = swapU32(_size);
    _size = (_size & 0b01111111) | ((_size & (0b01111111 << 8)) >> 1) | ((_size & (0b01111111 << 16)) >> 2) | ((_size & (0b01111111 << 24)) >> 3);
    return 0;
}

int ID3V2Extractor::extractFrames(std::ifstream& fs) {
    size_t offset = 0;
    while (offset < _size) {
        int nbytes = extractFrame(fs);
        if (nbytes <= 0) {
            // error or PADDING
            return nbytes;
        }
        offset += nbytes;
        if (offset > _size) {
            return -1;
        }
    }
    return 0;
}

int ID3V2Extractor::extractFrame(std::ifstream& fs) {
    Frame frame;
    char ID[4];
    fs.read((char*)&ID[0], sizeof(ID));
    fs.read((char*)&frame, sizeof(frame.flags) + sizeof(frame.size));
    frame.size = swapU32(frame.size);
    frame.flags = swapU16(frame.flags);
    if (frame.size == 0) {
        return frame.size;
    }
    frame.data = Data(new uint8_t[frame.size]);
    fs.read((char*)frame.data.get(), frame.size);
    _frames[ID] = std::move(frame);
    return frame.size;
}

ID3V2Parser::ID3V2Parser(ID3V2Extractor&& _extractor)
    : extractor{std::move(_extractor)}
{
    ;
}

// returns utf8 string
std::string ID3V2Parser::asString(const std::string& title) {
    return asUtf8String(title);
}

std::string ID3V2Parser::asNString(const std::string& title) {
    if (title.size() != 4 || title[0] != 'T') {
        return "";
    }
    const auto& frames = extractor.frames();
    auto iter = frames.find(title);
    if (iter == frames.end()) {
        return "";
    }
    auto frame = iter->second;
    if (frame.data[0] != 0) {
        return "";
    }
    std::string res((char*)&frame.data[1], frame.size - 1);
    return res;
}

std::basic_string<char16_t> ID3V2Parser::asUtf16LEString(const std::string& title) {
    if (title.size() != 4 || title[0] != 'T') {
        return u"";
    }
    const auto& frames = extractor.frames();
    auto iter = frames.find(title);
    if (iter == frames.end()) {
        return u"";
    }
    auto frame = iter->second;
    if (frame.data[0] != 1) {
        return u"";
    }
    // bin endian - swapping bytes
    if (frame.data[1] == 0xfe && frame.data[2] == 0xff) {
        for (size_t i = 3; i < frame.size; i+=2) {
            std::swap(frame.data[i], frame.data[i+1]);
        }
    }
    // little endian - ok
    else if (frame.data[1] == 0xff && frame.data[2] == 0xfe) {
        ;
    }
    // error - unknown encoding
    else {
        return u"";
    }
    return std::basic_string<char16_t>((char16_t*)&frame.data[3], (frame.size - 3) / 2);
}

std::string ID3V2Parser::asUtf8String(const std::string& title) {
    if (title.size() != 4 || title[0] != 'T') {
        return "";
    }
    const auto& frames = extractor.frames();
    auto iter = frames.find(title);
    if (iter == frames.end()) {
        return "";
    }
    auto frame = iter->second;
    // 0 - ascii
    if (frame.data[0] == 0) {
        return asciiToUtf8((char*)&frame.data[1], frame.size - 1);
    }
    // 1 - utf16 with BOM
    if (frame.data[0] == 1) {
        // bin endian - swapping bytes
        if (frame.data[1] == 0xfe && frame.data[2] == 0xff) {
            for (size_t i = 3; i < frame.size; i+=2) {
                std::swap(frame.data[i], frame.data[i+1]);
            }
        }
        // little endian - ok
        else if (frame.data[1] == 0xff && frame.data[2] == 0xfe) {
            ;
        }
        // error - unknown encoding
        else {
            return "";
        }
        return utf16ToUtf8((char*)&frame.data[3], frame.size - 3);
    }
    // 2 - utf16BE without BOM
    else if (frame.data[0] == 2) {
        for (size_t i = 1; i < frame.size; i+=2) {
            std::swap(frame.data[i], frame.data[i+1]);
        }
        return utf16ToUtf8((char*)&frame.data[1], frame.size - 1);
    }
    // 3 - utf8
    else if (frame.data[0] == 3) {
        return std::string((char*)&frame.data[1], frame.size - 1);
    }
    else {
        return "";
    }
}
