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
    inline Frames& tags() { return _frames; }
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

#endif // ID3V2PARSER_HPP
