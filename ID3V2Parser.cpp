#include "ID3V2Parser.hpp"
#include "Mp3FrameParser.hpp"

using namespace util;
using namespace tag::id3v2;

uint32_t syncSafe(uint32_t n) {
    return (n & 0b01111111) | ((n & (0b01111111 << 8)) >> 1) | ((n & (0b01111111 << 16)) >> 2) | ((n & (0b01111111 << 24)) >> 3);
}

tag::id3v2::ID3V2Extractor::ID3V2Extractor(std::ifstream& fs) {
    if (!checkFile(fs)) {
        throw InvalidTagException{};
    }
    //while (true) {
        if (extractHeader(fs)) {
            throw InvalidTagException{};
        }
        /*if (auto iter = _frames.find("SEEK"); iter != _frames.end()) {
            if (iter->second.size != 4) {
                throw std::runtime_error("invalid file");
            }
            uint32_t nextOffset = *(uint32_t*)iter->second.data.get();
            nextOffset = swapBytes(nextOffset);

        }*/
    //}
    if (unsynchronisation()) {
        //throw std::runtime_error("unsupported file: unsynchronization");
        // ignoring - should not have influence on correct work
    }
    if (extendedHeader()) {
        //throw std::runtime_error("unsupported file: extended headers");
        // ignoring - should not have influence on correct work
    }
    bool error = extractFrames(fs);
    // leaving fs in state, convenient for later work (on actual audio data)
    fs.seekg((hasFooter() ? 20 : 10) + _size, std::ios_base::beg);
    if (error) {
        throw InvalidTagException{};
    }
    if ((_version == 4) && hasFooter()) {
        throw NotImplementedException{};
    }
    if ((_version == 4) && _frames.find("SEEK") != _frames.end()) {
        throw NotImplementedException{};
    }
}

std::list<std::pair<tag::Extractor::Data, size_t>> tag::id3v2::ID3V2Extractor::framesData(const std::string& frameName) {
    auto iter = _frames.find(frameName);
    if (iter == _frames.end()) {
        return {};
    }
    std::list<std::pair<tag::Extractor::Data, size_t>> res;
    for (const auto& item : iter->second) {
        res.push_back({item.data, item.size});
    }
    return res;
}

std::vector<std::string> tag::id3v2::ID3V2Extractor::frameTitles() const {
    std::vector<std::string> res;
    for (const auto& [title, frame] : _frames) {
        res.push_back(title);
    }
    return res;
}

bool tag::id3v2::ID3V2Extractor::checkFile(std::ifstream& fs) {
    char data[5];
    fs.read((char*)&data, sizeof(data));
    _version = data[3];

    // 0x49 0x44 0x33 - ID3 string
    // 0x03 0x00 - version
    bool valid =
        data[0] == 0x49 &&
        data[1] == 0x44 &&
        data[2] == 0x33 &&
        (data[3] == 0x02 || data[3] == 0x03 || data[3] == 0x04)  &&
        data[4] == 0x00;
    if (!valid) {
        if ((data[0] == 0xff && ((data[1] & 0b11100000) == 0b11100000)) || (data[0] == 0x00)) {
            // sync bytes or some padding
            throw NoTagException{};
        }
        else if (!(data[0] == 0x49 && data[1] == 0x44 && data[2] == 0x33)) {
            // not id3 tag at all, but may be tag of some other type
            throw UnknownTagException{};
        }
        if (data[3] != 0x02 && data[3] != 0x03 && data[3] != 0x04) {
            throw UnknownTagVersionException{};
        }
    }
    return valid;
}

int tag::id3v2::ID3V2Extractor::extractHeader(std::ifstream& fs) {
    fs.read((char*)&_flags, 1);
    fs.read((char*)&_size, 4);
    _size = swapBytes<uint32_t>(_size);
    _size = syncSafe(_size);
    return 0;
}

int tag::id3v2::ID3V2Extractor::extractFrames(std::ifstream& fs) {
    size_t offset = hasFooter() ? 20 : 10;
    while (offset < _size) {
        int nbytes = _version == 2 ? extractFrameV22(fs) : extractFrame(fs);
        if (nbytes <= 0) {
            // error or PADDING
            return nbytes;
        }
        offset += nbytes;
        if (offset > _size) {
            // invalid tag, but some already read data may be useful
            return -1;
        }
    }
    return 0;
}

int tag::id3v2::ID3V2Extractor::extractFramesFooter(std::ifstream&) {
    return 0;
}

int tag::id3v2::ID3V2Extractor::extractFrame(std::ifstream& fs) {
    Frame frame;
    char ID[4];
    fs.read((char*)&ID[0], sizeof(ID));
    if (ID[0] == 0) {
        // padding?
        return 0;
    }
    fs.read((char*)&frame.size, sizeof(frame.flags) + sizeof(frame.size));
    frame.size = swapBytes<uint32_t>(frame.size);
    frame.flags = swapBytes<uint16_t>(frame.flags);
    if (frame.size == 0) {
        return frame.size;
    }
    // in id3v2.4 size of frame is also SYNCSAFE, like header (but NOT like frame size in id3v2.3)
    if (_version == 4) {
        frame.size = syncSafe(frame.size);
    }
    frame.data = Data(new uint8_t[frame.size]);
    fs.read((char*)frame.data.get(), frame.size);
    _frames[std::string(&ID[0], sizeof(ID))].push_back(std::move(frame));
    return frame.size;
}

int tag::id3v2::ID3V2Extractor::extractFrameV22(std::ifstream& fs) {
    Frame frame;
    char ID[3];
    fs.read((char*)&ID[0], sizeof(ID));
    if (ID[0] == 0) {
        // padding?
        return 0;
    }
    fs.read((char*)&frame.size, 3);
    // swapping 3 bytes in place
    frame.size = ((frame.size & 0xff) << 16) | ((frame.size & 0xff00)) | ((frame.size & 0xff0000) >> 16);
    if (frame.size == 0) {
        return frame.size;
    }
    frame.data = Data(new uint8_t[frame.size]);
    fs.read((char*)frame.data.get(), frame.size);
    _frames[std::string(&ID[0], sizeof(ID))].push_back(std::move(frame));
    return frame.size;
}

tag::id3v2::ID3V2Parser::ID3V2Parser(std::ifstream& fs)
{
    extractor = std::shared_ptr<Extractor>(new ID3V2Extractor(fs));
    _durationMs = mp3::getMp3FileDuration(fs);
}

std::string tag::id3v2::ID3V2Parser::songTitle() {
    return std::get<1>(Textual("TIT2"));
}

std::string tag::id3v2::ID3V2Parser::album() {
    return std::get<1>(Textual("TALB"));
}

std::string tag::id3v2::ID3V2Parser::artist() {
    return std::get<1>(Textual("TPE1"));
}

size_t tag::id3v2::ID3V2Parser::durationMs() {
    return _durationMs;
}
