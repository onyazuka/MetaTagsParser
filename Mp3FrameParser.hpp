#ifndef Mp3FrameParser_HPP
#define Mp3FrameParser_HPP
#include <string>
#include <cstdint>
#include <fstream>
#include <unordered_map>

namespace mp3 {

    enum class MPEGAudioVersion : uint8_t {
        MPEGVersion2_5,
        Reserved,
        MPEGVersion2,
        MPEGVersion1
    };

    enum class Layer : uint8_t {
        Reserved,
        LayerIII,
        LayerII,
        LayerI
    };

    enum class ChannelMode : uint8_t {
        Stereo,
        JointStereo,
        DualChannelStereo,
        SingleChannelMono
    };

    struct Mp3FrameHeaderRaw {
        uint8_t sync1 : 8;

        uint8_t protectionBit : 1;
        uint8_t layerDescr : 2;
        uint8_t mpegAudioVersionID : 2;
        uint8_t sync2 : 3;

        uint8_t privateBit : 1;
        uint8_t paddingBit : 1;
        uint8_t samplingRateFreqIndex : 2;
        uint8_t bitrateIndex : 4;

        uint8_t emphasis : 2;
        uint8_t original : 1;
        uint8_t copyright : 1;
        uint8_t modeExtension : 2;
        uint8_t channelMode : 2;
    };

    struct Mp3FrameHeader {
        Layer layer;
        MPEGAudioVersion version;
        size_t sampleRate;
        size_t bitrate;
        ChannelMode channelMode;
    };

    class Mp3FrameParser {
    public:

        class NotImplementedException : public std::exception {};
        class EOFException : public std::exception{};
        class NoFrameException : public std::exception{};
        class InvalidFrameHeaderException : public std::exception{};

        Mp3FrameParser(std::ifstream& ifs);
        void next();
        inline const Mp3FrameHeader& getHeader() const { return header; }
        inline bool isVBR() const { return VBR; }
        double frameLenBytes() const;
        double frameLenMs() const;
        size_t headerLenBytes() const;
    private:
        void parse(std::ifstream& ifs);
        Mp3FrameHeaderRaw headerRaw;
        Mp3FrameHeader header;
        std::ifstream& ifs;
        bool VBR = false;
        static const int BitrateIndexV1Map[4][16];
        static const int BitrateIndexV2Map[4][16];
        static const int SamplingRateFreqIndexMap[4][4];
    };

    size_t getMp3FileDuration(std::ifstream& ifs);

}


#endif // Mp3FrameParser_HPP
