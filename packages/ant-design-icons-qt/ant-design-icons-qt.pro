TEMPLATE = lib
TARGET = ant-design-icons-qt

QT += core gui widgets svg
CONFIG += c++17
CONFIG += staticlib
DEFINES += ADQT_ICONS_LIBRARY

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/src/generated
INCLUDEPATH += $$clean_path($$PWD/../adqt-icon-core/src)

HEADERS += \
    src/ant_design_icons_qt_global.h \
    src/antd_icons.h \
    src/generated/antd_pack_data.h \
    src/version.h

SOURCES += \
    src/antd_icons.cpp \
    src/generated/antd_pack_data.cpp

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
