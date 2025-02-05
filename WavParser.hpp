#ifndef WAVPARSER_HPP
#define WAVPARSER_HPP
#include "Tag.hpp"

namespace tag {
    namespace wav {

        struct WAVHeader {
            char RIFF[4];
            uint32_t fileSize;
            char WAVE[4];
            char fmt[4];
            uint32_t fmtLen;
            uint16_t typeOfFormat;
            uint16_t nChannels;
            uint32_t sampleRate;
            uint32_t sampleRateMulBitsPerSampleMulChannelsBytes;
            uint16_t bitsPerSampleMulChannelsBytes;
            uint16_t bitsPerSample;
            char data[4];
            uint32_t dataSize;
        };

        class WavExtractor : public Extractor {
        public:
            WavExtractor(std::ifstream& is);
            inline const WAVHeader& header() const { return _header; }
            std::list<std::pair<Extractor::Data, size_t>> framesData(const std::string& frameName) override;
            std::vector<std::string> frameTitles() const override;
        private:
            WAVHeader _header;
        };

        class WavParser : public Tag {
        public:
            WavParser(std::ifstream& ifs);
            std::string songTitle() override;
            std::string album() override;
            std::string artist() override;
            std::string year() override;
            std::string trackNumber() override;
            std::string comment() override;
            std::vector<user::APICUserData> image() override;
            size_t durationMs() override;
        };
    }
}

#endif // WAVPARSER_HPP
