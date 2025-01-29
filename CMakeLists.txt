set (sources ID3V2Parser.cpp FlacTagParser.cpp Mp3FrameParser.cpp Tag.cpp TagScout.cpp util.cpp)

add_library(MetaTagsParser STATIC ${sources})
