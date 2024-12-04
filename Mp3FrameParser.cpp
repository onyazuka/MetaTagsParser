#include "Mp3FrameParser.hpp"
#include <string.h>
#include "util.hpp"

using namespace mp3;
using namespace util;

const int mp3::Mp3FrameParser::BitrateIndexV1Map[4][16]   = {
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},                  // reserved
    {-1,32,40,48,56,64,80,96,112,128,160,192,224,256,320,-2},           // layer 3
    {-1,32,48,56,64,80,96,112,128,160,192,224,256,320,384,-2},          // l2
    {-1,32,64,96,128,160,192,224,256,288,320,352,384,416,448,-2}        // l1
};

const int mp3::Mp3FrameParser::BitrateIndexV2Map[4][16]   = {
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},                  // reserved
    {-1,8,16,24,32,40,48,56,64,80,96,112,128,144,160,-2},               // l3
    {-1,8,16,24,32,40,48,56,64,80,96,112,128,144,160,-2},               // l2 = l3
    {-1,32,48,56,64,80,96,112,128,144,160,176,192,224,256,-2}           // l1
};

const int mp3::Mp3FrameParser::SamplingRateFreqIndexMap[4][4] = {
    {11025,12000,8000,-1},      // v2.5
    {-1,-1,-1,-1},              // reserved
    {22050,24000,16000,-1},     // v2
    {44100,48000,32000,-1}      // v1
};

mp3::Mp3FrameParser::Mp3FrameParser(std::ifstream& ifs, const Options& options)
    : options{options}
{
    memset((void*)&headerRaw, 0, sizeof(headerRaw));
    if (parse(ifs)) {
        throw std::runtime_error("invalid mp3 header");
    }
    if (!options.assumeSameHeaders) {
        throw std::runtime_error("not implemented");
    }
}

double mp3::Mp3FrameParser::durationMs() const {
    auto d1 = frameLenMs();
    auto d2 = frameLenBytes();
    auto d3 = headerLenBytes();
    return frameLenMs() * (options.fileSize / frameLenBytes());
}

double mp3::Mp3FrameParser::frameLenBytes() const {
    //
    //For Layer I files us this formula:
    //    FrameLengthInBytes = (12 * BitRate / SampleRate + Padding) * 4
    //For Layer II & III files use this formula:
    //    FrameLengthInBytes = 144 * BitRate / SampleRate + Padding
    // Result size is floor()'ed
    if (header.layer == Layer::LayerI) {
        return (12.0 * (double)header.bitrate / (double)header.sampleRate + headerRaw.paddingBit) * 4.0;
    }
    // layers II and III
    else {
        return (144.0 * (double)header.bitrate / (double)header.sampleRate) + headerRaw.paddingBit;
    }
}

double mp3::Mp3FrameParser::frameLenMs() const {
    // each L1 frame has 384 samples
    // each L2 or L3 frame has 1152 samples
    if (header.layer == Layer::LayerI) {
        return 384.0 / (double)header.sampleRate * 1000;
    }
    // layers II and III
    else {
        return 1152.0 / (double)header.sampleRate * 1000;
    }
}

size_t mp3::Mp3FrameParser::headerLenBytes() const  {
    if (headerRaw.protectionBit) {
        // 4 bytes of header + crc 16 bit
        return 6;
    }
    else {
        return 4;
    }
}

int mp3::Mp3FrameParser::parse(std::ifstream& ifs) {
    if (!ifs) {
        return -1;
    }

    ifs.read((char*)&headerRaw, sizeof(headerRaw));
    if (!(headerRaw.sync1 == 0xff && headerRaw.sync2 == 0x7)) {
        return -1;
    }
    if (headerRaw.bitrateIndex == 0b1111) {
        // bad bitrate index
        return -1;
    }
    if (headerRaw.bitrateIndex == 0b0000) {
        throw std::runtime_error("not implemented");
    }
    if (headerRaw.samplingRateFreqIndex == 0b11) {
        throw std::runtime_error("not implemented");
    }
    header.layer = (Layer)headerRaw.layerDescr;
    header.version = (MPEGAudioVersion)headerRaw.mpegAudioVersionID;
    header.sampleRate = SamplingRateFreqIndexMap[headerRaw.mpegAudioVersionID][headerRaw.samplingRateFreqIndex];
    header.bitrate = (header.version == MPEGAudioVersion::MPEGVersion1 ?
        BitrateIndexV1Map[headerRaw.layerDescr][headerRaw.bitrateIndex] :
        BitrateIndexV2Map[headerRaw.layerDescr][headerRaw.bitrateIndex])
        * 1000;

    ifs.seekg((size_t)ifs.tellg() + frameLenBytes() - 4);

    return 0;
}
