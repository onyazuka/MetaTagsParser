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

std::list<std::pair<tag::Extractor::Data, size_t>> tag::flac::FlacTagExtractor::framesData(const std::string& frameName) {
    auto iter = _frames.find(frameName);
    if (iter == _frames.end()) {
        return {};
    }
    std::list<std::pair<tag::Extractor::Data, size_t>> res;
    for (const auto& item : iter->second) {
        res.push_back({item.data, item.header.size});
    }
    return res;
}

std::vector<std::string> tag::flac::FlacTagExtractor::frameTitles() const {
    std::vector<std::string> res;
    for (const auto& [title, frame] : _frames) {
        res.push_back(title);
    }
    return res;
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
    // can't use sizeof(Header) and must use hardcode, because of MSVC and it's alignment pervercies even with pragma pack
    fs.read((char*)&frame.header, 4);
    frame.header.size = ((frame.header.size & 0xff) << 16) | ((frame.header.size & 0xff00)) | ((frame.header.size & 0xff0000) >> 16);
    frame.data = Data(new uint8_t[frame.header.size]);
    fs.read((char*)frame.data.get(), frame.header.size);
    return frame;
}

FlacTagParser::FlacTagParser(std::ifstream& fs)
{
    extractor = std::shared_ptr<Extractor>(new FlacTagExtractor(fs));
    vorbis = VorbisCommentMap();
}

VorbisCommentReader::ResultType tag::flac::FlacTagParser::VorbisComment() {
    auto [frameData, frameSize] = extractor->frameData("VORBIS_COMMENT");
    if (!frameData || !frameSize) {
        return {};
    }
    DataBlock data(frameData.get(), frameSize);
    data.encoding = Encoding::Utf8;
    return VorbisCommentReader().read(data);
}

std::unordered_map<std::string, std::string> tag::flac::FlacTagParser::VorbisCommentMap() {
    auto vorbisComment = VorbisComment();
    if (std::get<2>(vorbisComment) == 0) {
        return {};
    }
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

std::list<PictureReader::ResultType> tag::flac::FlacTagParser::Picture() {
    auto pictures = extractor->framesData("PICTURE");
    std::list<PictureReader::ResultType> res;
    for (auto [frameData, frameSize] : pictures) {
        if (!frameData || !frameSize) {
            continue;
        }
        DataBlock data(frameData.get(), frameSize);
        data.encoding = Encoding::Utf8;
        res.push_back(PictureReader().read(data));
    }
    return res;
}

tag::flac::FlacTagParser::StreamInfoDescr tag::flac::FlacTagParser::StreamInfo() {
    auto [frameData, frameSize] = extractor->frameData("STREAMINFO");
    if (!frameData || !frameSize) {
        return {};
    }
    StreamInfoDescrRaw streamInfoRaw = *(StreamInfoDescrRaw*)(frameData.get());
    StreamInfoDescr streamInfo;
    streamInfo.minimumBlockSizeInSamples = swapBytes(streamInfoRaw.minimumBlockSizeInSamples);
    streamInfo.maximumBlockSizeInSamples = swapBytes(streamInfoRaw.maximumBlockSizeInSamples);
    streamInfo.minimumFrameSizeInBytes = swapBytes<uint32_t, 3>(streamInfoRaw.minimumFrameSizeInBytes);
    // FIX THIS ON LINUX!!!
    uint32_t val = (streamInfoRaw.maximumFrameSizeInBytes1 << 16) | streamInfoRaw.maximumFrameSizeInBytes2;
    streamInfo.maximumFrameSizeInBytes = swapBytes<uint32_t, 3>(val);
    streamInfo.sampleRate = ((uint32_t)swapBytes(streamInfoRaw.sampleRate1) << 4) | ((uint32_t)streamInfoRaw.sampleRate2);
    streamInfo.channels = streamInfoRaw.channels;
    streamInfo.bitsPerSample = (streamInfoRaw.bitsPerSample1 << 4) | streamInfoRaw.bitsPerSample2;
    streamInfo.totalSamples = (streamInfoRaw.totalSamples1 << 4) | swapBytes(streamInfoRaw.totalSamples2);
    return streamInfo;
}

std::string tag::flac::FlacTagParser::songTitle() {
    return textual("TITLE");
}

std::string tag::flac::FlacTagParser::album() {
    return textual("ALBUM");
}

std::string tag::flac::FlacTagParser::artist() {
    return textual("ARTIST");
}

std::string tag::flac::FlacTagParser::year() {
    return textual("YEAR");
}

std::string tag::flac::FlacTagParser::trackNumber() {
    return textual("TRACKNUMBER");
}

std::string tag::flac::FlacTagParser::comment() {
    return textual("DESCRIPTION");
}

std::string tag::flac::FlacTagParser::textual(const std::string& name) {
    if (auto iter = vorbis.find(name); iter != vorbis.end()) {
        return iter->second;
    }
    else {
        return "";
    }
}

std::vector<tag::user::APICUserData> tag::flac::FlacTagParser::image() {
    auto images = Picture();
    std::vector<tag::user::APICUserData> res;
    for (auto& image : images) {
        res.push_back({(user::ImageType)std::get<0>(image), std::get<2>(image), std::get<10>(image)});
    }
    return res;
}

size_t tag::flac::FlacTagParser::durationMs() {
    auto streamInfo = StreamInfo();
    if (!streamInfo.sampleRate) {
        return 0;
    }
    return streamInfo.totalSamples / streamInfo.sampleRate * 1000;
}
