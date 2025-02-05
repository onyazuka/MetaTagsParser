cmake_minimum_required(VERSION 3.16)
project(MetaTagsParser VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set (sources
    ID3V2Parser.hpp ID3V2Parser.cpp
    FlacTagParser.hpp FlacTagParser.cpp
    Mp3FrameParser.hpp Mp3FrameParser.cpp
    Tag.hpp Tag.cpp
    TagScout.hpp TagScout.cpp
    util.hpp util.cpp
    WavParser.hpp WavParser.cpp
)

add_library(MetaTagsParser STATIC ${sources})
add_executable(MetaTagsParserExe ${sources} main.cpp)
