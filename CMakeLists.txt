set (sources
    ID3V2Parser.hpp ID3V2Parser.cpp
    FlacTagParser.hpp FlacTagParser.cpp
    Mp3FrameParser.hpp Mp3FrameParser.cpp
    Tag.hpp Tag.cpp
    TagScout.hpp TagScout.cpp
    util.hpp util.cpp
)

add_library(MetaTagsParser STATIC ${sources})
