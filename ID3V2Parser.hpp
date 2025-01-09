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


        class ID3V2Extractor : public Extractor {
        public:
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
            std::list<std::pair<Extractor::Data, size_t>> framesData(const std::string& frameName) override;
            std::vector<std::string> frameTitles() const override;
        private:
            void init(std::ifstream& fs);
            bool checkFile(std::ifstream& fs);
            int extractHeader(std::ifstream& fs);
            size_t extractSize(std::ifstream& fs);
            int extractFrames(std::ifstream& fs);
            int extractFramesFooter(std::ifstream& fs);
            int extractFrame(std::ifstream& fs);
            int extractFrameV22(std::ifstream& fs);
            void skipPadding(std::ifstream& fs);
            void syncLookup(std::ifstream& fs);
            Frames _frames;
            uint8_t _flags = 0;
            uint32_t _size = 0;
            uint8_t _version = 0;
            size_t fileSize = 0;
        };


        class ID3V2Parser : public Tag {
        public:
            ID3V2Parser(std::ifstream& fs);
            inline std::list<APICReader::ResultType> APIC() { return readFrames<APICReader>("APIC"); }
            inline TextualFrameReader::ResultType Textual(const std::string& frameName) { return readFrame<TextualFrameReader>(frameName); }
            inline std::list<TXXXReader::ResultType> TXXX() { return readFrames<TXXXReader>("TXXX"); }
            inline std::list<WUrlFrameReader::ResultType> WUrl(const std::string& frameName) { return readFrames<WUrlFrameReader>(frameName); }
            inline std::list<WXXXReader::ResultType> WXXX() { return readFrames<WXXXReader>("WXXX"); }
            inline std::list<COMMReader::ResultType> COMM() { return readFrames<COMMReader>("COMM"); }

            std::string songTitle() override;
            std::string album() override;
            std::string artist() override;
            size_t durationMs() override;

            template<typename ReaderType>
            typename ReaderType::ResultType readFrame(const std::string& frameName);
            template<typename ReaderType>
            std::list<typename ReaderType::ResultType> readFrames(const std::string& frameName);
            int64_t _durationMs = -1;
        };

        template<typename ReaderType>
        typename ReaderType::ResultType ID3V2Parser::readFrame(const std::string& frameName) {
            if (!extractor) {
                return {};
            }
            auto frames = extractor->framesData(frameName);
            if (frames.empty()) return {};
            auto [data, size] = frames.front();
            if (!data || !size) {
                return {};
            }
            return ReaderType().read(DataBlock(data.get(), size));
        }

        template<typename ReaderType>
        std::list<typename ReaderType::ResultType> ID3V2Parser::readFrames(const std::string& frameName) {
            if (!extractor) {
                return {};
            }
            auto frames = extractor->framesData(frameName);
            std::list<typename ReaderType::ResultType> res;
            for (auto& [data, size] : frames) {
                res.push_back(ReaderType().read(DataBlock(data.get(), size)));
            }
            return res;
        }
    }
}

#endif // ID3V2PARSER_HPP
