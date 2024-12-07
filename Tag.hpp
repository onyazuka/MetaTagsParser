#ifndef TAG_H
#define TAG_H
#include <string>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <cassert>
#include <concepts>
#include <string.h>
#include <list>
#include <vector>
#include <filesystem>
#include "util.hpp"

namespace tag {

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
        // to determine size of lists, strings etc
        size_t sizeOfData = 0;
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

    struct BigEndian {};
    struct LittleEndian {};

    template<typename T>
    concept Endianness = std::is_same_v<T, BigEndian> || std::is_same_v<T, LittleEndian>;

    template<std::unsigned_integral N, Endianness E>
    struct Bytes {
        using Data = N;
        Data read(DataBlock& data) {
            assert (data.size - data.offset);
            if constexpr (std::is_same_v<E, BigEndian>) {
                Data res;
                res = util::swapBytes<N>(*(N*)(data.data + data.offset));
                data.offset += sizeof(N);
                return res;
            }
            // LE
            else {
                Data res = *(N*)(data.data + data.offset);
                data.offset += sizeof(N);
                return res;
            }
        }
    };

    // consumes all remaining data as binary data
    struct BinaryData {
        using Data = std::pair<std::shared_ptr<uint8_t[]>, size_t>;
        Data read(DataBlock& data);
    };

    struct EncodingByte {
        using Data = Encoding;
        Data read(DataBlock& data);
    };

    template<Endianness E>
    struct SizeOfData {
        using Data = uint32_t;
        Data read(DataBlock& data);
    };

    template<Endianness E>
    SizeOfData<E>::Data SizeOfData<E>::read(DataBlock& data) {
        Data res = Bytes<Data, E>().read(data);
        data.sizeOfData = res;
        return res;
    }

    // endiannes for internal numbers (size of list)
    template<Endianness E>
    struct ListOfEncodedStrings {
        using Data = std::list<std::string>;
        Data read(DataBlock& data);
    };

    template<Endianness E>
    ListOfEncodedStrings<E>::Data ListOfEncodedStrings<E>::read(DataBlock& data) {
        assert (((int64_t)data.size - (int64_t)data.offset) >= 0);
        if ((data.size - data.offset) == 0) {
            return Data{};
        }
        Data res;
        for (size_t i = 0; i < data.sizeOfData; ++i) {
            SizeOfData<E> size;
            size.read(data);
            EncodedStr str;
            res.push_back(str.read(data));
        }
        return res;
    }

    using Byte = Bytes<uint8_t, LittleEndian>;

    template<typename... Args>
    struct FrameReader {
        using ResultType = std::tuple<typename Args::Data...>;
        std::tuple<typename Args::Data...> read(DataBlock data) {
            return ResultType{Args().read(data)...};
        }
    };

    class NoTagException : public std::exception {};
    class UnknownTagVersionException : public std::exception {};
    class InvalidTagException : public std::exception {};
    class UnknownTagException : public std::exception {};
    class NotImplementedException : public std::exception {};

    class Extractor {
    public:
        using Data = std::shared_ptr<uint8_t[]>;
        std::pair<Extractor::Data, size_t> frameData(const std::string& frameName);
        virtual std::list<std::pair<Extractor::Data, size_t>> framesData(const std::string& frameName) = 0;
        virtual std::vector<std::string> frameTitles() const = 0;
    };

    class Tag {
    public:
        virtual ~Tag() {}
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
        inline auto getExtractor() { return extractor; }
    protected:
        std::shared_ptr<Extractor> extractor;
    };

}


#endif // TAG_H
