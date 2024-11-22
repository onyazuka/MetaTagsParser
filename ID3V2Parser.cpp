#include "ID3V2Parser.hpp"

uint32_t swapU32(uint32_t number) {
    return ((number & 0xff) << 24) | ((number & 0xff00) << 8) | ((number & 0xff0000) >> 8) | ((number & 0xff000000) >> 24);
}

uint16_t swapU16(uint16_t number) {
    return ((number & 0xff) << 8) | ((number & 0xff00) >> 8);
}

ID3V2Extractor::ID3V2Extractor(std::ifstream& fs) {
    if (!checkFile(fs) || extractHeader(fs) || extractFrames(fs)) {
        throw std::runtime_error("invalid file");
    }
    if (unsynchronisation()) {
        throw std::runtime_error("unsupported file: unsynchronization");
    }
    if (extendedHeader()) {
        throw std::runtime_error("unsupported file: extended headers");
    }
    if (extractFrames(fs)) {
        throw std::runtime_error("unsupported file: invalid tags");
    }
}

bool ID3V2Extractor::checkFile(std::ifstream& fs) {
    std::unique_ptr<uint8_t[]> data(new uint8_t[5]);
    fs.read((char*)data.get(), 5);
    // 0x49 0x44 0x33 - ID3 string
    // 0x03 0x00 - version
    return
        data[0] == 0x49 &&
        data[1] == 0x44 &&
        data[2] == 0x33 &&
        data[3] == 0x03  &&
        data[4] == 0x00;

}

int ID3V2Extractor::extractHeader(std::ifstream& fs) {
    fs.read((char*)&_flags, 1);
    fs.read((char*)&_size, 4);
    _size = swapU32(_size);
    _size = (_size & 0b01111111) | ((_size & (0b01111111 << 8)) >> 1) | ((_size & (0b01111111 << 16)) >> 2) | ((_size & (0b01111111 << 24)) >> 3);
    return 0;
}

int ID3V2Extractor::extractFrames(std::ifstream& fs) {
    size_t offset = 0;
    while (offset < _size) {
        int nbytes = extractFrame(fs);
        if (nbytes <= 0) {
            // error or PADDING
            return nbytes;
        }
        offset += nbytes;
        if (offset > _size) {
            return -1;
        }
    }
    return 0;
}

int ID3V2Extractor::extractFrame(std::ifstream& fs) {
    Frame frame;
    char ID[4];
    fs.read((char*)&ID[0], sizeof(ID));
    fs.read((char*)&frame, sizeof(frame.flags) + sizeof(frame.size));
    frame.size = swapU32(frame.size);
    frame.flags = swapU16(frame.flags);
    if (frame.size == 0) {
        return frame.size;
    }
    frame.data = Data(new uint8_t[frame.size]);
    fs.read((char*)frame.data.get(), frame.size);
    _frames[ID] = std::move(frame);
    return frame.size;
}
