QT += core gui widgets svg
CONFIG += c++17
TEMPLATE = app
TARGET = theme-demo

INCLUDEPATH += ../../packages/ant-design-qt/src
INCLUDEPATH += ../../packages/ant-design-icons-qt/src

HEADERS += \
    icon_theme_adapter.h \
    menu_docs_page.h \
    select_docs_page.h

SOURCES += \
    icon_theme_adapter.cpp \
    main.cpp \
    menu_docs_page.cpp \
    select_docs_page.cpp

isEmpty(ADQT_LIB_BUILD_DIR) {
    ADQT_LIB_BUILD_DIR = $$clean_path($$OUT_PWD/../../packages/ant-design-qt)
}
!exists($$ADQT_LIB_BUILD_DIR) {
    ADQT_LIB_BUILD_DIR = $$clean_path($$PWD/../../packages/ant-design-qt/build-mingw)
}

isEmpty(ADQT_ICONS_LIB_BUILD_DIR) {
    ADQT_ICONS_LIB_BUILD_DIR = $$clean_path($$OUT_PWD/../../packages/ant-design-icons-qt)
}
!exists($$ADQT_ICONS_LIB_BUILD_DIR) {
    ADQT_ICONS_LIB_BUILD_DIR = $$clean_path($$PWD/../../packages/ant-design-icons-qt/build-mingw)
}

CONFIG(debug, debug|release) {
    ADQT_LIB_DIR = $$ADQT_LIB_BUILD_DIR/debug
    ADQT_ICONS_LIB_DIR = $$ADQT_ICONS_LIB_BUILD_DIR/debug
} else {
    ADQT_LIB_DIR = $$ADQT_LIB_BUILD_DIR/release
    ADQT_ICONS_LIB_DIR = $$ADQT_ICONS_LIB_BUILD_DIR/release
}

win32-g++ {
    PRE_TARGETDEPS += $$ADQT_LIB_DIR/libant-design-qt.a
    PRE_TARGETDEPS += $$ADQT_ICONS_LIB_DIR/libant-design-icons-qt.a
    LIBS += -L$$ADQT_LIB_DIR -lant-design-qt
    LIBS += -L$$ADQT_ICONS_LIB_DIR -lant-design-icons-qt
}

win32-msvc {
    PRE_TARGETDEPS += $$ADQT_LIB_DIR/ant-design-qt.lib
    PRE_TARGETDEPS += $$ADQT_ICONS_LIB_DIR/ant-design-icons-qt.lib
    LIBS += $$ADQT_LIB_DIR/ant-design-qt.lib
    LIBS += $$ADQT_ICONS_LIB_DIR/ant-design-icons-qt.lib
}
