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
#include "Tag.hpp"

namespace tag {
    namespace id3v2 {

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


        class ID3V2Parser : public Tag {
        public:
            ID3V2Parser(ID3V2Extractor&& extractor);
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
            std::pair<uint8_t*, size_t> getFrameData(const std::string& title) override;
            std::list<std::pair<uint8_t*, size_t>> getFramesData(const std::string& title) override;
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
