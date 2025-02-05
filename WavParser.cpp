#include "WavParser.hpp"

using namespace tag;
using namespace tag::wav;

WavExtractor::WavExtractor(std::ifstream& is) {
    if (!is) {
        throw NoTagException{};
    }
    size_t offset = is.tellg();
    size_t sz = 0;
    is.seekg(0, std::ios_base::end);
    sz = is.tellg();
    is.seekg(offset);
    if (sz < sizeof(WAVHeader)) {
        throw NoTagException{};
    }
    is.read((char*)&_header, sizeof(WAVHeader));
    if (!(
            (strncmp(_header.RIFF, "RIFF", 4) == 0) &&
            (strncmp(_header.WAVE, "WAVE", 4) == 0) &&
            (strncmp(_header.fmt, "fmt", 3) == 0) &&
            (strncmp(_header.data, "data", 4) == 0)
          ))
    {
        throw NoTagException{};
    }
}

std::list<std::pair<Extractor::Data, size_t>> WavExtractor::framesData(const std::string& frameName) {
    return {};
}

std::vector<std::string> WavExtractor::frameTitles() const {
    return {};
}

WavParser::WavParser(std::ifstream& fs) {
    try {
        extractor = std::shared_ptr<Extractor>(new WavExtractor(fs));
    }
    catch (NoTagException) {
        ;
    }
}

std::string WavParser::songTitle() {
    return {};
}

std::string WavParser::album() {
    return {};
}

std::string WavParser::artist() {
    return {};
}

std::string WavParser::year() {
    return {};
}

std::string WavParser::trackNumber()  {
    return {};
}

std::string WavParser::comment()  {
    return {};
}

std::vector<user::APICUserData> WavParser::image()  {
    return {};
}

size_t WavParser::durationMs() {
    auto header = std::dynamic_pointer_cast<WavExtractor>(extractor)->header();
    return header.dataSize / header.sampleRateMulBitsPerSampleMulChannelsBytes * 1000;
}
