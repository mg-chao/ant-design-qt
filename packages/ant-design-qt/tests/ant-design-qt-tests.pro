QT += core gui widgets testlib svg network
CONFIG += c++17 testcase
TEMPLATE = app
TARGET = ant-design-qt-tests

INCLUDEPATH += $$PWD/../src
INCLUDEPATH += $$clean_path($$PWD/../../adqt-icon-core/src)
INCLUDEPATH += $$clean_path($$PWD/../../ant-design-icons-qt/src)
MOC_DIR = $$PWD/.moc
INCLUDEPATH += $$MOC_DIR

SOURCES += \
    alert_tests.cpp \
    button_tests.cpp \
    color_picker_tests.cpp \
    date_picker_tests.cpp \
    image_tests.cpp \
    input_tests.cpp \
    input_number_tests.cpp \
    modal_tests.cpp \
    menu_tests.cpp \
    popup_tests.cpp \
    radio_tests.cpp \
    select_tests.cpp \
    slider_tests.cpp \
    switch_tests.cpp \
    tag_tests.cpp

HEADERS += \
    select_tests.h

isEmpty(ADQT_LIB_BUILD_DIR) {
    ADQT_LIB_BUILD_DIR = $$clean_path($$OUT_PWD/..)
}
win32-g++:ADQT_LIB_DEBUG_PROBE = $$ADQT_LIB_BUILD_DIR/debug/libant-design-qt.a
win32-g++:ADQT_LIB_RELEASE_PROBE = $$ADQT_LIB_BUILD_DIR/release/libant-design-qt.a
win32-msvc:ADQT_LIB_DEBUG_PROBE = $$ADQT_LIB_BUILD_DIR/debug/ant-design-qt.lib
win32-msvc:ADQT_LIB_RELEASE_PROBE = $$ADQT_LIB_BUILD_DIR/release/ant-design-qt.lib
!exists($$ADQT_LIB_DEBUG_PROBE):!exists($$ADQT_LIB_RELEASE_PROBE) {
    ADQT_LIB_BUILD_DIR = $$clean_path($$PWD/../build-mingw)
}

isEmpty(ADQT_ICON_CORE_LIB_BUILD_DIR) {
    ADQT_ICON_CORE_LIB_BUILD_DIR = $$clean_path($$OUT_PWD/../../adqt-icon-core)
}
win32-g++:ADQT_ICON_CORE_DEBUG_PROBE = $$ADQT_ICON_CORE_LIB_BUILD_DIR/debug/libadqt-icon-core.a
win32-g++:ADQT_ICON_CORE_RELEASE_PROBE = $$ADQT_ICON_CORE_LIB_BUILD_DIR/release/libadqt-icon-core.a
win32-msvc:ADQT_ICON_CORE_DEBUG_PROBE = $$ADQT_ICON_CORE_LIB_BUILD_DIR/debug/adqt-icon-core.lib
win32-msvc:ADQT_ICON_CORE_RELEASE_PROBE = $$ADQT_ICON_CORE_LIB_BUILD_DIR/release/adqt-icon-core.lib
!exists($$ADQT_ICON_CORE_DEBUG_PROBE):!exists($$ADQT_ICON_CORE_RELEASE_PROBE) {
    ADQT_ICON_CORE_LIB_BUILD_DIR = $$clean_path($$PWD/../../adqt-icon-core/build-mingw)
}

isEmpty(ADQT_ICONS_LIB_BUILD_DIR) {
    ADQT_ICONS_LIB_BUILD_DIR = $$clean_path($$OUT_PWD/../../ant-design-icons-qt)
}
win32-g++:ADQT_ICONS_DEBUG_PROBE = $$ADQT_ICONS_LIB_BUILD_DIR/debug/libant-design-icons-qt.a
win32-g++:ADQT_ICONS_RELEASE_PROBE = $$ADQT_ICONS_LIB_BUILD_DIR/release/libant-design-icons-qt.a
win32-msvc:ADQT_ICONS_DEBUG_PROBE = $$ADQT_ICONS_LIB_BUILD_DIR/debug/ant-design-icons-qt.lib
win32-msvc:ADQT_ICONS_RELEASE_PROBE = $$ADQT_ICONS_LIB_BUILD_DIR/release/ant-design-icons-qt.lib
!exists($$ADQT_ICONS_DEBUG_PROBE):!exists($$ADQT_ICONS_RELEASE_PROBE) {
    ADQT_ICONS_LIB_BUILD_DIR = $$clean_path($$PWD/../../ant-design-icons-qt/build-mingw)
}

CONFIG(debug, debug|release) {
    ADQT_LIB_DIR = $$ADQT_LIB_BUILD_DIR/debug
    ADQT_ICON_CORE_LIB_DIR = $$ADQT_ICON_CORE_LIB_BUILD_DIR/debug
    ADQT_ICONS_LIB_DIR = $$ADQT_ICONS_LIB_BUILD_DIR/debug
} else {
    ADQT_LIB_DIR = $$ADQT_LIB_BUILD_DIR/release
    ADQT_ICON_CORE_LIB_DIR = $$ADQT_ICON_CORE_LIB_BUILD_DIR/release
    ADQT_ICONS_LIB_DIR = $$ADQT_ICONS_LIB_BUILD_DIR/release
}

win32-g++ {
    PRE_TARGETDEPS += $$ADQT_LIB_DIR/libant-design-qt.a
    PRE_TARGETDEPS += $$ADQT_ICON_CORE_LIB_DIR/libadqt-icon-core.a
    PRE_TARGETDEPS += $$ADQT_ICONS_LIB_DIR/libant-design-icons-qt.a
    LIBS += -L$$ADQT_LIB_DIR -lant-design-qt
    LIBS += -L$$ADQT_ICON_CORE_LIB_DIR -ladqt-icon-core
    LIBS += -L$$ADQT_ICONS_LIB_DIR -lant-design-icons-qt
}

win32-msvc {
    PRE_TARGETDEPS += $$ADQT_LIB_DIR/ant-design-qt.lib
    PRE_TARGETDEPS += $$ADQT_ICON_CORE_LIB_DIR/adqt-icon-core.lib
    PRE_TARGETDEPS += $$ADQT_ICONS_LIB_DIR/ant-design-icons-qt.lib
    LIBS += $$ADQT_LIB_DIR/ant-design-qt.lib
    LIBS += $$ADQT_ICON_CORE_LIB_DIR/adqt-icon-core.lib
    LIBS += $$ADQT_ICONS_LIB_DIR/ant-design-icons-qt.lib
}
