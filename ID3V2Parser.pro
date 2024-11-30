TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        ID3V2Parser.cpp \
        main.cpp \
        util.cpp

HEADERS += \
    ID3V2Parser.hpp \
    util.hpp
