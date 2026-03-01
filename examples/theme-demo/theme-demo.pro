QT += core gui widgets
CONFIG += c++17
TEMPLATE = app
TARGET = theme-demo

INCLUDEPATH += ../../src

HEADERS += \
    ../../src/theme/fast_color_lite.h \
    ../../src/theme/palette_generate.h \
    ../../src/theme/theme.h \
    ../../src/theme/theme_algorithms.h \
    ../../src/theme/theme_manager.h \
    ../../src/theme/theme_palette.h \
    ../../src/theme/theme_types.h

SOURCES += \
    main.cpp \
    ../../src/theme/fast_color_lite.cpp \
    ../../src/theme/palette_generate.cpp \
    ../../src/theme/theme_algorithms.cpp \
    ../../src/theme/theme_manager.cpp \
    ../../src/theme/theme_palette.cpp \
    ../../src/theme/theme_types.cpp
