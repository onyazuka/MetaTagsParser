#include "ID3V2Parser.hpp"
#include "Mp3FrameParser.hpp"

using namespace util;
using namespace tag::id3v2;

uint32_t syncSafe(uint32_t n) {
    return (n & 0b01111111) | ((n & (0b01111111 << 8)) >> 1) | ((n & (0b01111111 << 16)) >> 2) | ((n & (0b01111111 << 24)) >> 3);
}

tag::id3v2::ID3V2Extractor::ID3V2Extractor(std::ifstream& fs) {
    size_t offset = fs.tellg();
    fs.seekg(0, std::ios_base::end);
    fileSize = fs.tellg();
    fs.seekg(offset, std::ios_base::beg);
    init(fs);
}

void tag::id3v2::ID3V2Extractor::init(std::ifstream& fs) {
    if (!checkFile(fs)) {
        // leaving fs in state, convenient for later work (on actual audio data)
        skipPadding(fs);
        syncLookup(fs);
        throw InvalidTagException{};
    }
    //while (true) {
    if (extractHeader(fs)) {
        //throw InvalidTagException{};
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
    skipPadding(fs);
    syncLookup(fs);
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
    uint8_t data[5];
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
            fs.seekg(0);
            skipPadding(fs);
            throw NoTagException{};
        }
        else if (!(data[0] == 0x49 && data[1] == 0x44 && data[2] == 0x33)) {
            // not id3 tag at all, but may be tag of some other type
            fs.seekg(0);
            skipPadding(fs);
            syncLookup(fs);
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
    _size = extractSize(fs);
    return 0;
}

size_t tag::id3v2::ID3V2Extractor::extractSize(std::ifstream& fs) {
    size_t size = 0;
    fs.read((char*)&size, 4);
    size = swapBytes<uint32_t>(size);
    size = syncSafe(size);
    return size;
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
    if (frame.size > (fileSize - fs.tellg())) {
        return 0;
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
    if (frame.size > (fileSize - fs.tellg())) {
        return 0;
    }
    frame.data = Data(new uint8_t[frame.size]);
    fs.read((char*)frame.data.get(), frame.size);
    _frames[std::string(&ID[0], sizeof(ID))].push_back(std::move(frame));
    return frame.size;
}

void tag::id3v2::ID3V2Extractor::skipPadding(std::ifstream& fs) {
    uint8_t byte = 0;
    while (!byte && fs) {
        fs.read((char*)&byte, 1);
    }
    if (byte) {
        fs.seekg((size_t)fs.tellg() - 1);
    }
}

/*
    looking for sync seq (13 or more sequential 1 bytes)
    handling known kinds of data (not equal to sync seq) (RIFF, another ID3 tag...)
    returns true if something was skipped
*/
void tag::id3v2::ID3V2Extractor::syncLookup(std::ifstream& fs) {
    static constexpr size_t SyncLookupSize = 4096;
    size_t remainFsize = 0;
    size_t initOffset = fs.tellg();
    remainFsize = fileSize - initOffset;
    uint8_t buf[4];
    // sync seq not found - giving up
    if (!(remainFsize > 2)) {
        return;
    }
    fs.read((char*)&buf[0], 2);
    // sync seq is next - ok
    if (buf[0] == 0xff && (buf[1] & 0b11100000)) {
        fs.seekg(initOffset);
        return;
    }
    // suspect one more ID3 tag - JUST SKIPPING IT for now
    if (buf[0] == 'I' && buf[1] == 'D' && remainFsize >= 3) {
        fs.read((char*)&buf[2], 1);
        if (buf[2] == '3') {
            // ID3 tag
            // skip version and flags
            fs.seekg((size_t)fs.tellg() + 3);
            size_t size = extractSize(fs);
            fs.seekg((size_t)fs.tellg() + size);
            skipPadding(fs);
            // can be more to skip
            syncLookup(fs);
            return;
        }
    }
    // suspect RIFF
    else if (buf[0] == 'R' && buf[1] == 'I' && remainFsize >= 4) {
        fs.read((char*)&buf[2], 2);
        if (buf[2] == 'F' && buf[3] == 'F') {
            // RIFF found
            uint8_t RIFFBuf[SyncLookupSize];
            fs.read((char*)&RIFFBuf[0], std::min(remainFsize - 4, SyncLookupSize));
            for(size_t i = 0; i < SyncLookupSize - 1; ++i) {
                if ((RIFFBuf[i] == 0xff) && ((RIFFBuf[i + 1] & 0b11100000) == 0b11100000)) {
                    fs.seekg(initOffset + 4 + i);
                    return;
                }
            }
            // not found sync seq in RIFFLookupSize bytes - returning to initial offset
            fs.seekg(initOffset);
            return;
        }
    }
    // something else - try to find sync seq
    else {
        uint8_t dataBuf[SyncLookupSize];
        fs.read((char*)&dataBuf[0], std::min(remainFsize - 2, SyncLookupSize));
        for(size_t i = 0; i < SyncLookupSize - 1; ++i) {
            if ((dataBuf[i] == 0xff) && ((dataBuf[i + 1] & 0b11100000) == 0b11100000)) {
                fs.seekg(initOffset + 2 + i);
                return;
            }
        }
        // not found sync seq in RIFFLookupSize bytes - returning to initial offset
        fs.seekg(initOffset);
        return;
    }
    fs.seekg(initOffset);
    return;
}

tag::id3v2::ID3V2Parser::ID3V2Parser(std::ifstream& fs)
{
    try {
        extractor = std::shared_ptr<Extractor>(new ID3V2Extractor(fs));
    }
    // recoverable errors - still try to find duration
    catch (NoTagException&) {
        // not a error, just no tag
    }
    catch (UnknownTagException&) {
        // not a error, just unknown tag
    }
    catch (InvalidTagException&) {
        // tag is invalid, but some data may be ok
    }
    try {
        _durationMs = mp3::getMp3FileDuration(fs);
    }
    catch (mp3::Mp3FrameParser::EOFException&) {
        // just EOF of mp3 frame data
    }
    catch (mp3::Mp3FrameParser::NoFrameException&) {
        ;
    }
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

std::string tag::id3v2::ID3V2Parser::year() {
    return std::get<1>(Textual("TYER"));
}

std::string tag::id3v2::ID3V2Parser::trackNumber() {
    return std::get<1>(Textual("TRCK"));
}

std::string tag::id3v2::ID3V2Parser::comment() {
    auto comment = COMM();
    if (comment.empty()) {
        return "";
    }
    return std::get<3>(comment.front());
}

std::vector<tag::user::APICUserData> tag::id3v2::ID3V2Parser::image() {
    auto images = APIC();
    std::vector<tag::user::APICUserData> res;
    for (auto& image : images) {
        res.push_back({(user::ImageType)std::get<2>(image), std::get<1>(image), std::get<4>(image)});
    }
    return res;
}

size_t tag::id3v2::ID3V2Parser::durationMs() {
    return _durationMs;
}
