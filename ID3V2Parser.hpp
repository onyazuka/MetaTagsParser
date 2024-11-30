#ifndef ID3V2PARSER_HPP
#define ID3V2PARSER_HPP
#include <string>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <cassert>
#include <concepts>
#include <string.h>
#include <list>

template<std::unsigned_integral T>
T swapBytes(T n) {
    if constexpr(std::is_same_v<T, uint8_t>) {
        return n;
    }
    else if (std::is_same_v<T, uint16_t>) {
        return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
    }
    else if (std::is_same_v<T, uint32_t>) {
        return ((n & 0xff) << 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24);
    }
    else if (std::is_same_v<T, uint64_t>) {
        return ((n & 0xff) << 56) | ((n & 0xff00) << 40) | ((n & 0xff0000) << 24) | ((n & 0xff000000) << 8) | ((n & 0xff00000000) >> 8) | ((n & 0xff0000000000) >> 24) | ((n & 0xff000000000000) >> 40) | ((n & 0xff00000000000000) >> 56);
    }
}

enum class Encoding : uint8_t{
    Ascii,
    Utf16BOM,
    Utf16BE,
    Utf8
};

struct DataBlock {
    DataBlock(uint8_t* data, size_t size);
    uint8_t* data = 0;
    size_t size = 0;
    size_t offset = 0;
    Encoding encoding = Encoding::Ascii;
};

struct AsciiStrNullTerminated {
    using Data = std::string;
    Data read(DataBlock& data);
};

template<size_t N>
struct AsciiStrSized {
    using Data = std::string;
    Data read(DataBlock& data) {
        assert ((data.size - data.offset) >= N);
        std::string res((char*)data.data + data.offset, N);
        data.offset += N;
        return res;
    }
};

std::string decodeStr(uint8_t* data, size_t sz, Encoding encoding);

// Encoded string with null terminator of unknown length
class EncodedStrNullTerminated {
public:
    using Data = std::string;
    Data read(DataBlock& data);
};

// Encoded string of KNOWN length (data.size)
class EncodedStr {
public:
    using Data = std::string;
    Data read(DataBlock& data);
};

template<std::unsigned_integral N>
struct Bytes {
    using Data = N;
    Data read(DataBlock& data) {
        assert (data.size - data.offset);
        Data res;
        res = swapBytes<N>(*(N*)(data.data + data.offset));
        data.offset += sizeof(N);
        return res;
    }
};

// consumes all remaining data as binary data
struct BinaryData {
    using Data = std::pair<std::shared_ptr<uint8_t[]>, size_t>;
    Data read(DataBlock& data);
};

using Byte = Bytes<uint8_t>;
using Byte2 = Bytes<uint16_t>;
using Byte4 = Bytes<uint32_t>;
using Byte8 = Bytes<uint64_t>;

struct EncodingByte {
    using Data = Encoding;
    Data read(DataBlock& data);
};


template<typename... Args>
struct FrameReader {
    using ResultType = std::tuple<typename Args::Data...>;
    std::tuple<typename Args::Data...> read(DataBlock data) {
        return ResultType{Args().read(data)...};
    }
};

using APICReader = FrameReader<EncodingByte, AsciiStrNullTerminated, Byte, EncodedStrNullTerminated, BinaryData>;
using TextualFrameReader = FrameReader<EncodingByte, EncodedStrNullTerminated>;
using WUrlFrameReader = FrameReader<AsciiStrNullTerminated>;
using WXXXReader = FrameReader<EncodingByte, EncodedStrNullTerminated, AsciiStrNullTerminated>;
using COMMReader = FrameReader<EncodingByte, AsciiStrSized<3>, EncodedStrNullTerminated, EncodedStrNullTerminated>;


class ID3V2Extractor {
public:
    using Data = std::shared_ptr<uint8_t[]>;
    struct Frame {
        uint32_t size = 0;
        uint16_t flags = 0;
        Data data;
    };
    using Frames = std::unordered_map<std::string, Frame>;
    using FramesMultiple = std::unordered_map<std::string, std::list<Frame>>;

    class NoTagException : public std::exception {};
    class UnknownTagVersionException : public std::exception {};
    class InvalidTagException : public std::exception {};
    class UnknownTagException : public std::exception {};
    class NotImplementedException : public std::exception {};

    ID3V2Extractor(std::ifstream& fs);
    inline Frames& frames() { return _frames; }
    inline uint32_t size() const { return _size; }
    inline bool unsynchronisation() const { return _flags & ((uint8_t)1<<7); }
    inline bool extendedHeader() const { return _flags & ((uint8_t)1<<6); }
    inline bool experimental() const { return _flags & ((uint8_t)1<<5); }
    inline bool hasFooter() const { return (_version == 4) && (_flags & ((uint8_t)1 << 4) ); }
    inline uint8_t version() const { return _version; }
private:
    bool checkFile(std::ifstream& fs);
    int extractHeader(std::ifstream& fs);
    int extractFrames(std::ifstream& fs);
    int extractFramesFooter(std::ifstream& fs);
    int extractFrame(std::ifstream& fs);
    int extractFrameV22(std::ifstream& fs);
    Frames _frames;
    FramesMultiple _framesMultiple;
    uint8_t _flags = 0;
    uint32_t _size = 0;
    uint8_t _version = 0;
};


class ID3V2Parser {
public:
    ID3V2Parser(ID3V2Extractor&& extractor);

    std::string asString(const std::string& title);
    std::string asNString(const std::string& title);
    static std::string asNString(uint8_t* data, size_t n);
    std::basic_string<char16_t> asUtf16LEString(const std::string& title);
    static std::basic_string<char16_t> asUtf16LEString(uint8_t* data, size_t n);
    std::wstring asUtf16LEWstring(const std::string& title);
    static std::wstring asUtf16LEWstring(uint8_t* data, size_t n);
    std::string asUtf8String(const std::string& title);
    static std::string asUtf8String(uint8_t* data, size_t n);
    static std::string asUtf8String(uint8_t* data, size_t n, uint8_t encoding);
    static std::string asUtf8String_ascii(uint8_t* data, size_t n);
    static std::string asUtf8String_utf16BOM(uint8_t* data, size_t n);
    static std::string asUtf8String_utf16BE(uint8_t* data, size_t n);
    static std::string asUtf8String_utf8(uint8_t* data, size_t n);

    inline APICReader::ResultType APIC() { return readFrame<APICReader>("APIC"); }
    inline TextualFrameReader::ResultType Textual(const std::string& frameName) { return readFrame<TextualFrameReader>(frameName); }
    inline WUrlFrameReader::ResultType WUrl(const std::string& frameName) { return readFrame<WUrlFrameReader>(frameName); }
    inline WXXXReader::ResultType WXXX() { return readFrame<WXXXReader>("WXXX"); }
    inline COMMReader::ResultType COMM() { return readFrame<COMMReader>("COMM"); }

    template<typename ReaderType>
    typename ReaderType::ResultType readFrame(const std::string& frameName);

protected:
    std::pair<uint8_t*, size_t> getFrameData(const std::string& title);
    ID3V2Extractor extractor;
};

template<typename ReaderType>
typename ReaderType::ResultType ID3V2Parser::readFrame(const std::string& frameName) {
    auto [data, size] = getFrameData(frameName);
    if (!data || !size) {
        return {};
    }
    return ReaderType().read(DataBlock(data, size));
}

std::string utf16ToUtf8(char* str, size_t n);
std::string asciiToUtf8(char* str, size_t n);
std::basic_string<char16_t> utf8ToUtf16(char* str, size_t n);
std::string utf8ToAscii(char* str, size_t n);

#endif // ID3V2PARSER_HPP
