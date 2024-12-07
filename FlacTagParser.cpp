#include "FlacTagParser.hpp"

using namespace tag::flac;
using namespace util;

const std::array<std::string, (size_t)tag::flac::FlacTagExtractor::BlockType::Count> tag::flac::FlacTagExtractor::BlockTypeStrMap = {
    "STREAMINFO",
    "PADDING",
    "APPLICATION",
    "SEEKTABLE",
    "VORBIS_COMMENT",
    "CUESHEET",
    "PICTURE"
};

tag::flac::FlacTagExtractor::FlacTagExtractor(std::ifstream& fs) {
    if (!checkFile(fs)) {
        throw InvalidTagException{};
    }
    if (extractFrames(fs)) {
        throw InvalidTagException{};
    }
}

bool tag::flac::FlacTagExtractor::checkFile(std::ifstream& fs) {
    char data[4];
    fs.read((char*)&data, sizeof(data));
    if (!(data[0] == 'f' && data[1] == 'L' && data[2] == 'a' && data[3] == 'C')) {
        return false;
    }
    return true;
}

int tag::flac::FlacTagExtractor::extractFrames(std::ifstream& fs) {
    Frame frame = extractFrame(fs);
    if ((BlockType)frame.header.blockType != BlockType::STREAMINFO) {
        // STREAMINFO is mandatory
        throw InvalidTagException{};
    }
    bool last = frame.header.lastMetadataBlockFlag;
    _frames[BlockTypeStrMap[frame.header.blockType]].push_back(std::move(frame));
    while (!last) {
        frame = extractFrame(fs);
        last = frame.header.lastMetadataBlockFlag;
        _frames[BlockTypeStrMap[frame.header.blockType]].push_back(std::move(frame));
    }
    return 0;
}

tag::flac::FlacTagExtractor::Frame tag::flac::FlacTagExtractor::extractFrame(std::ifstream& fs) {
    Frame frame;
    fs.read((char*)&frame.header, sizeof(FrameHeader));
    frame.header.size = ((frame.header.size & 0xff) << 16) | ((frame.header.size & 0xff00)) | ((frame.header.size & 0xff0000) >> 16);
    frame.data = Data(new uint8_t[frame.header.size]);
    fs.read((char*)frame.data.get(), frame.header.size);
    return frame;
}

FlacTagParser::FlacTagParser(FlacTagExtractor&& extractor)
    : extractor{std::move(extractor)}
{
    ;
}

VorbisCommentReader::ResultType tag::flac::FlacTagParser::VorbisComment() {
    const auto& frame = extractor.frames()["VORBIS_COMMENT"].front();
    DataBlock data(frame.data.get(), frame.header.size);
    data.encoding = Encoding::Utf8;
    return VorbisCommentReader().read(data);
}

std::unordered_map<std::string, std::string> tag::flac::FlacTagParser::VorbisCommentMap() {
    auto vorbisComment = VorbisComment();
    std::unordered_map<std::string, std::string> res;
    for (const auto& comment : std::get<3>(vorbisComment)) {
        auto pos = comment.find('=');
        if (pos == comment.npos) {
            continue;
        }
        std::string val;
        if (pos != (comment.size() - 1)) {
            val = comment.substr(pos + 1);
        }
        res[comment.substr(0, pos)] = std::move(val);
    }
    return res;
}

PictureReader::ResultType tag::flac::FlacTagParser::Picture() {
    const auto& frame = extractor.frames()["PICTURE"].front();
    DataBlock data(frame.data.get(), frame.header.size);
    data.encoding = Encoding::Utf8;
    return PictureReader().read(data);
}

tag::flac::FlacTagParser::StreamInfoDescr tag::flac::FlacTagParser::StreamInfo() {
    const auto& frame = extractor.frames()["STREAMINFO"].front();
    StreamInfoDescrRaw streamInfoRaw = *(StreamInfoDescrRaw*)(frame.data.get());
    StreamInfoDescr streamInfo;
    streamInfo.minimumBlockSizeInSamples = swapBytes(streamInfoRaw.minimumBlockSizeInSamples);
    streamInfo.maximumBlockSizeInSamples = swapBytes(streamInfoRaw.maximumBlockSizeInSamples);
    streamInfo.minimumFrameSizeInBytes = swapBytes<uint32_t, 3>(streamInfoRaw.minimumFrameSizeInBytes);
    streamInfo.maximumFrameSizeInBytes = swapBytes<uint32_t, 3>(streamInfoRaw.maximumFrameSizeInBytes);
    streamInfo.sampleRate = ((uint32_t)swapBytes(streamInfoRaw.sampleRate1) << 4) | ((uint32_t)streamInfoRaw.sampleRate2);
    streamInfo.channels = streamInfoRaw.channels;
    streamInfo.bitsPerSample = (streamInfoRaw.bitsPerSample1 << 4) | streamInfoRaw.bitsPerSample2;
    streamInfo.totalSamples = (streamInfoRaw.totalSamples1 << 4) | swapBytes(streamInfoRaw.totalSamples2);
    return streamInfo;
}
