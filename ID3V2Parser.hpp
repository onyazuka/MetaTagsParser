#ifndef ID3V2PARSER_HPP
#define ID3V2PARSER_HPP
#include <string>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <exception>

class ID3V2Extractor {
public:
    using Data = std::shared_ptr<uint8_t[]>;
    struct Frame {
        uint32_t size;
        uint16_t flags;
        Data data;
    };
    using Frames = std::unordered_map<std::string, Frame>;

    ID3V2Extractor(std::ifstream& fs);
    inline Frames& frames() { return _frames; }
    inline uint32_t size() const { return _size; }
    inline bool unsynchronisation() const { return _flags & ((uint8_t)1<<7); }
    inline bool extendedHeader() const { return _flags & ((uint8_t)1<<6); }
    inline bool experimental() const { return _flags & ((uint8_t)1<<5); }
private:
    bool checkFile(std::ifstream& fs);
    int extractHeader(std::ifstream& fs);
    int extractFrames(std::ifstream& fs);
    int extractFrame(std::ifstream& fs);
    Frames _frames;
    uint8_t _flags = 0;
    uint32_t _size = 0;
};

class ID3V2Parser {
public:
    ID3V2Parser(ID3V2Extractor&& extractor);
    std::string asString(const std::string& title);
    std::string asNString(const std::string& title);
    std::basic_string<char16_t> asUtf16LEString(const std::string& title);
    std::wstring asUtf16LEWstring(const std::string& title);
    std::string asUtf8String(const std::string& title);
private:
    ID3V2Extractor extractor;
};

std::string utf16ToUtf8(char* str, size_t n);
std::string asciiToUtf8(char* str, size_t n);
std::basic_string<char16_t> utf8ToUtf16(char* str, size_t n);
std::string utf8ToAscii(char* str, size_t n);

#endif // ID3V2PARSER_HPP
