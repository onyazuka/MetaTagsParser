TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        ID3V2Parser.cpp \
        Mp3FrameParser.cpp \
        TagScout.cpp \
        main.cpp \
        util.cpp

HEADERS += \
    ID3V2Parser.hpp \
    Mp3FrameParser.hpp \
    TagScout.hpp \
    util.hpp
