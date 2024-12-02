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
#include "util.hpp"

namespace tag {
    namespace id3v2 {

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
                res = util::swapBytes<N>(*(N*)(data.data + data.offset));
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
        using TXXXReader = FrameReader<EncodingByte, EncodedStrNullTerminated, EncodedStrNullTerminated>;
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
            using Frames = std::unordered_map<std::string, std::list<Frame>>;

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

            inline std::list<APICReader::ResultType> APIC() { return readFrames<APICReader>("APIC"); }
            inline TextualFrameReader::ResultType Textual(const std::string& frameName) { return readFrame<TextualFrameReader>(frameName); }
            inline std::list<TXXXReader::ResultType> TXXX() { return readFrames<TXXXReader>("TXXX"); }
            inline std::list<WUrlFrameReader::ResultType> WUrl(const std::string& frameName) { return readFrames<WUrlFrameReader>(frameName); }
            inline std::list<WXXXReader::ResultType> WXXX() { return readFrames<WXXXReader>("WXXX"); }
            inline std::list<COMMReader::ResultType> COMM() { return readFrames<COMMReader>("COMM"); }

            template<typename ReaderType>
            typename ReaderType::ResultType readFrame(const std::string& frameName);
            template<typename ReaderType>
            std::list<typename ReaderType::ResultType> readFrames(const std::string& frameName);

        protected:
            std::pair<uint8_t*, size_t> getFrameData(const std::string& title);
            std::list<std::pair<uint8_t*, size_t>> getFramesData(const std::string& title);
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

        template<typename ReaderType>
        std::list<typename ReaderType::ResultType> ID3V2Parser::readFrames(const std::string& frameName) {
            auto frames = getFramesData(frameName);
            std::list<typename ReaderType::ResultType> res;
            for (auto& [data, size] : frames) {
                res.push_back(ReaderType().read(DataBlock(data, size)));
            }
            return res;
        }
    }
}

#endif // ID3V2PARSER_HPP
