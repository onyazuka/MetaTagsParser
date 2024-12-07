#ifndef FLACTAGPARSER_HPP
#define FLACTAGPARSER_HPP
#include <string>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <cassert>
#include <concepts>
#include <string.h>
#include <list>
#include <array>
#include "Tag.hpp"

namespace tag {
    namespace flac {

        using VorbisCommentReader = FrameReader<SizeOfData<LittleEndian>, EncodedStr, SizeOfData<LittleEndian>, ListOfEncodedStrings<LittleEndian>>;
        using PictureReader = FrameReader<Bytes<uint32_t, BigEndian>, SizeOfData<BigEndian>, EncodedStr, SizeOfData<BigEndian>, EncodedStr, Bytes<uint32_t, BigEndian>, Bytes<uint32_t, BigEndian>, Bytes<uint32_t, BigEndian>, Bytes<uint32_t, BigEndian>, SizeOfData<BigEndian>, BinaryData>;

        class FlacTagExtractor {
        public:
            using Data = std::shared_ptr<uint8_t[]>;
            enum class BlockType {
                STREAMINFO,
                PADDING,
                APPLICATION,
                SEEKTABLE,
                VORBIS_COMMENT,
                CUESHEET,
                PICTURE,
                Count
            };
            static const std::array<std::string, (size_t)BlockType::Count> BlockTypeStrMap;
            struct FrameHeader {
                uint8_t blockType : 7;
                uint8_t lastMetadataBlockFlag : 1;
                uint32_t size : 24;
            };
            struct Frame {
                FrameHeader header;
                Data data;
            };

            using Frames = std::unordered_map<std::string, std::list<Frame>>;
            FlacTagExtractor(std::ifstream& ifs);
            inline Frames& frames() { return _frames; }
        private:
            bool checkFile(std::ifstream& fs);
            int extractFrames(std::ifstream& fs);
            Frame extractFrame(std::ifstream& fs);

            Frames _frames;
        };


        class FlacTagParser : public Tag {
        public:
            struct_packed_begin(StreamInfoDescrRaw)
                uint16_t minimumBlockSizeInSamples;
                uint16_t maximumBlockSizeInSamples;
                uint32_t minimumFrameSizeInBytes : 24;
                uint32_t maximumFrameSizeInBytes : 24;
                uint16_t sampleRate1;
                uint8_t bitsPerSample1 : 1;
                uint8_t channels : 3;
                uint8_t sampleRate2 : 4;
                uint8_t totalSamples1 : 4;
                uint8_t bitsPerSample2 : 4;
                uint32_t totalSamples2;
            struct_packed_end;

            struct StreamInfoDescr {
                uint16_t minimumBlockSizeInSamples;
                uint16_t maximumBlockSizeInSamples;
                uint32_t minimumFrameSizeInBytes;
                uint32_t maximumFrameSizeInBytes;
                uint32_t sampleRate;
                uint8_t channels;
                uint8_t bitsPerSample;
                uint64_t totalSamples;
            };

            FlacTagParser(FlacTagExtractor&& extractor);
            VorbisCommentReader::ResultType VorbisComment();
            std::unordered_map<std::string, std::string> VorbisCommentMap();
            PictureReader::ResultType Picture();
            StreamInfoDescr StreamInfo();

            inline std::pair<uint8_t*, size_t> getFrameData(const std::string&) override {return {};};
            inline std::list<std::pair<uint8_t*, size_t>> getFramesData(const std::string&) override {return {};};
        private:
            FlacTagExtractor extractor;
        };

    }
}

#endif // FLACTAGPARSER_HPP
