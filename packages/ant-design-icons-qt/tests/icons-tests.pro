QT += core gui widgets svg testlib
CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = ant-design-icons-qt-tests

INCLUDEPATH += ../src
INCLUDEPATH += ../src/generated

SOURCES += \
    icons_tests.cpp

isEmpty(ADQT_ICONS_LIB_BUILD_DIR) {
    ADQT_ICONS_LIB_BUILD_DIR = $$clean_path($$OUT_PWD/../../build-mingw)
}
!exists($$ADQT_ICONS_LIB_BUILD_DIR) {
    ADQT_ICONS_LIB_BUILD_DIR = $$clean_path($$PWD/../build-mingw)
}

CONFIG(debug, debug|release) {
    ADQT_ICONS_LIB_DIR = $$ADQT_ICONS_LIB_BUILD_DIR/debug
} else {
    ADQT_ICONS_LIB_DIR = $$ADQT_ICONS_LIB_BUILD_DIR/release
}

win32-g++ {
    PRE_TARGETDEPS += $$ADQT_ICONS_LIB_DIR/libant-design-icons-qt.a
    LIBS += -L$$ADQT_ICONS_LIB_DIR -lant-design-icons-qt
}

win32-msvc {
    PRE_TARGETDEPS += $$ADQT_ICONS_LIB_DIR/ant-design-icons-qt.lib
    LIBS += $$ADQT_ICONS_LIB_DIR/ant-design-icons-qt.lib
}
