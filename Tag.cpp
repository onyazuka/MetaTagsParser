#include "Tag.hpp"

using namespace util;
using namespace tag;

tag::DataBlock::DataBlock(uint8_t* data, size_t size)
    : data{data}, size{size}, offset{0}, encoding{Encoding::Ascii}
{
    ;
}

AsciiStrNullTerminated::Data tag::AsciiStrNullTerminated::read(DataBlock& data) {
    assert (((int64_t)data.size - (int64_t)data.offset) >= 0);
    if ((data.size - data.offset) == 0) {
        return Data{};
    }
    size_t i = data.offset;
    for (i = data.offset; i < data.size; ++i) {
        if (data.data[i] == 0) {
            break;
        }
    }
    assert (i >= data.offset);
    Data res = Tag::asUtf8String_ascii(data.data + data.offset, i - data.offset);
    data.offset = i + 1;
    return res;
}

std::string tag::decodeStr(uint8_t* data, size_t sz, Encoding encoding) {
    switch (encoding) {
    case Encoding::Ascii:
        return Tag::asUtf8String_ascii(data, sz);
    case Encoding::Utf16BOM:
        return Tag::asUtf8String_utf16BOM(data, sz);
    case Encoding::Utf16BE:
        return Tag::asUtf8String_utf16BE(data, sz);
    case Encoding::Utf8:
        return Tag::asUtf8String_utf8(data, sz);
    default:
        ;
    }
}

// !!!encoding stored in data!!!
EncodedStrNullTerminated::Data tag::EncodedStrNullTerminated::EncodedStrNullTerminated::read(DataBlock& data) {
    assert (((int64_t)data.size - (int64_t)data.offset) >= 0);
    if ((data.size - data.offset) == 0) {
        return Data{};
    }
    //Data res = asUtf8String(data.data + data.offset, n);
    size_t i = data.offset;
    if (data.encoding == Encoding::Utf16BE || data.encoding == Encoding::Utf16BOM) {
        for (i = data.offset; i < data.size; i += 2) {
            if (*((char16_t*)(data.data + i)) == 0) {
                break;
            }
        }
    }
    else {
        for (i = data.offset; i < data.size; ++i) {
            if (data.data[i] == 0) {
                break;
            }
        }
    }
    assert (i >= data.offset);
    Data res = decodeStr(data.data + data.offset, i - data.offset, Encoding(data.encoding));
    if (data.encoding == Encoding::Utf16BE || data.encoding == Encoding::Utf16BOM) {
        // \0\0
        data.offset = i + 2;
    }
    else {
        // \0
        data.offset = i + 1;
    }
    return res;
}

EncodedStr::Data tag::EncodedStr::read(DataBlock& data) {
    assert (((int64_t)data.size - (int64_t)data.offset) >= 0);
    if ((data.size - data.offset) == 0) {
        return Data{};
    }
    Data res = decodeStr(data.data + data.offset, data.sizeOfData, Encoding(data.encoding));
    data.offset += data.sizeOfData;
    return res;
}

BinaryData::Data tag::BinaryData::read(DataBlock& data) {
    assert (((int64_t)data.size - (int64_t)data.offset) >= 0);
    if ((data.size - data.offset) == 0) {
        return Data{};
    }
    Data res{new uint8_t[data.size - data.offset], 0};
    memcpy(res.first.get(), data.data + data.offset, data.size - data.offset);
    res.second = data.size - data.offset;
    return res;
}

EncodingByte::Data EncodingByte::read(DataBlock& data) {
    assert (data.size - data.offset);
    Data res;
    res = (Encoding)(*(data.data + data.offset));
    data.offset += sizeof(Encoding);
    data.encoding = res;
    return res;
}

// returns utf8 string
std::string tag::Tag::asString(const std::string& title) {
    return asUtf8String(title);
}

std::string tag::Tag::asNString(const std::string& title) {
    auto [data, size] = getFrameData(title);
    if (!data || !size) {
        return "";
    }
    return asNString(data, size);
}

std::string tag::Tag::asNString(uint8_t* data, size_t n) {
    if (data[0] != 0) {
        return "";
    }
    return std::string((char*)&data[1], n - 1);
}

std::basic_string<char16_t> tag::Tag::asUtf16LEString(const std::string& title) {
    auto [data, size] = getFrameData(title);
    if (!data || !size) {
        return {};
    }
    return asUtf16LEString(data, size - 3);
}

std::basic_string<char16_t> tag::Tag::asUtf16LEString(uint8_t* data, size_t size) {
    if (data[0] != 1) {
        return u"";
    }
    // bin endian - swapping bytes
    if (data[1] == 0xfe && data[2] == 0xff) {
        for (size_t i = 3; i < size; i+=2) {
            std::swap(data[i], data[i+1]);
        }
    }
    // little endian - ok
    else if (data[1] == 0xff && data[2] == 0xfe) {
        ;
    }
    // error - unknown encoding
    else {
        return u"";
    }
    return std::basic_string<char16_t>((char16_t*)&data[3], (size - 3) / 2);
}

std::wstring tag::Tag::asUtf16LEWstring(const std::string& title) {
    auto str = asUtf16LEString(title);
    return std::wstring((wchar_t*)str.data());
}

std::wstring tag::Tag::asUtf16LEWstring(uint8_t* data, size_t n) {
    auto str = asUtf16LEString(data, n);
    return std::wstring((wchar_t*)str.data());
}

std::string tag::Tag::asUtf8String(const std::string& title) {
    auto [data, size] = getFrameData(title);
    if (!data || !size) {
        return "";
    }
    return asUtf8String(data, size);
}

std::string tag::Tag::asUtf8String(uint8_t* data, size_t size) {
    if (size <= 1) {
        // empty string or just encoding without content
        return "";
    }
    return asUtf8String(&data[1], size - 1, data[0]);
}

std::string tag::Tag::asUtf8String(uint8_t* data, size_t n, uint8_t encoding) {
    switch (encoding) {
    case 0:
        return asUtf8String_ascii(data, n);
    case 1:
        return asUtf8String_utf16BOM(data, n);
    case 2:
        return asUtf8String_utf16BE(data, n);
    case 3:
        return asUtf8String_utf8(data, n);
    default:
        return "";
    }
}

std::string tag::Tag::asUtf8String_ascii(uint8_t* data, size_t n) {
    return asciiToUtf8((char*)data, n);
}

std::string tag::Tag::asUtf8String_utf16BOM(uint8_t* data, size_t size) {
    // bin endian - swapping bytes
    if (data[0] == 0xfe && data[1] == 0xff) {
        for (size_t i = 2; i < size; i+=2) {
            std::swap(data[i], data[i+1]);
        }
    }
    // little endian - ok
    else if (data[0] == 0xff && data[1] == 0xfe) {
        ;
    }
    // error - unknown encoding
    else {
        return "";
    }
    return utf16ToUtf8((char*)&data[2], size - 2);
}

std::string tag::Tag::asUtf8String_utf16BE(uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i+=2) {
        std::swap(data[i], data[i+1]);
    }
    return utf16ToUtf8((char*)&data[0], size);
}

std::string tag::Tag::asUtf8String_utf8(uint8_t* data, size_t n) {
    return std::string((char*)&data[0], n);
}
