TEMPLATE = lib
TARGET = ant-design-icons-qt

QT += core gui widgets svg
CONFIG += c++17
CONFIG += staticlib
DEFINES += ADQT_ICONS_LIBRARY

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/src/generated

HEADERS += \
    src/ant_design_icons_qt_global.h \
    src/icon_engine.h \
    src/icon_provider.h \
    src/icon_render.h \
    src/icons_types.h \
    src/icons.h \
    src/generated/icon_functions.h \
    src/generated/icon_manifest.h \
    src/version.h

SOURCES += \
    src/icon_engine.cpp \
    src/icon_provider.cpp \
    src/icon_render.cpp \
    src/icons.cpp \
    src/generated/icon_functions.cpp \
    src/generated/icon_manifest.cpp

RESOURCES += \
    resources/ant_design_icons.qrc

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32 {
    PREFIX = C:/ant-design-qt
}

target.path = $$PREFIX/lib
headers.path = $$PREFIX/include/ant-design-icons-qt
headers.files = $$HEADERS
INSTALLS += target headers
