TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        FlacTagParser.cpp \
        ID3V2Parser.cpp \
        Mp3FrameParser.cpp \
        Tag.cpp \
        TagScout.cpp \
        main.cpp \
        util.cpp

HEADERS += \
    FlacTagParser.hpp \
    ID3V2Parser.hpp \
    Mp3FrameParser.hpp \
    Tag.hpp \
    TagScout.hpp \
    util.hpp
