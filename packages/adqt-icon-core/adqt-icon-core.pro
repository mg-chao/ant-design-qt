TEMPLATE = lib
TARGET = adqt-icon-core

QT += core gui widgets svg
CONFIG += c++17
CONFIG += staticlib
DEFINES += ADQT_ICON_CORE_LIBRARY

INCLUDEPATH += $$PWD/src

HEADERS +=     src/adqt_icon_core_global.h     src/icon_core.h     src/icon_core_types.h     src/icon_registry.h     src/version.h

SOURCES +=     src/icon_registry.cpp

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32 {
    PREFIX = C:/ant-design-qt
}

target.path = $$PREFIX/lib
headers.path = $$PREFIX/include/adqt-icon-core
headers.files = $$HEADERS
INSTALLS += target headers
