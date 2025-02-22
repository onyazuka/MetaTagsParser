#ifndef UTIL_HPP
#define UTIL_HPP
#include <cstdint>
#include <concepts>
#include <string>

#ifdef __GNUC__
#define struct_packed_begin(name) struct __attribute__ ((packed)) name {
#define struct_packed_end }
#elif _MSC_VER
#define struct_packed_begin(name) __pragma(pack(push, 1)) struct name {
#define struct_packed_end } __pragma(pack(pop))
#endif

namespace util {

    template<std::unsigned_integral T>
    T swapBytes(T n) {
        if constexpr(std::is_same_v<T, uint8_t>) {
            return n;
        }
        else if (std::is_same_v<T, uint16_t>) {
            return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
        }
        else if (std::is_same_v<T, uint32_t>) {
            return ((n & 0xff) << 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24);
        }
        else if (std::is_same_v<T, uint64_t>) {
            return ((n & 0xff) << 56) | ((n & 0xff00) << 40) | ((n & 0xff0000) << 24) | ((n & 0xff000000) << 8) | ((n & 0xff00000000) >> 8) | ((n & 0xff0000000000) >> 24) | ((n & 0xff000000000000) >> 40) | ((n & 0xff00000000000000) >> 56);
        }
    }

    // swap first N bytes in type T
    template<std::unsigned_integral T, size_t N>
    T swapBytes(T n) {
        static_assert(sizeof(T) >= N);
        // swap 24 bits in 32 bit uint
        if constexpr (std::is_same_v<T, uint32_t> && (N == 3)) {
            return ((n & 0xff) << 16) | ((n & 0xff00)) | ((n & 0xff0000) >> 16);
        }
    }

    std::string utf16ToUtf8(char* str, size_t n);
    std::string asciiToUtf8(char* str, size_t n);
    std::basic_string<char16_t> utf8ToUtf16(char* str, size_t n);
    std::string utf8ToAscii(char* str, size_t n);

}

#endif // UTIL_HPP
