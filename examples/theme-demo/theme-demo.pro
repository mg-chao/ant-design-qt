QT += core gui widgets svg network
CONFIG += c++17
TEMPLATE = app
TARGET = theme-demo

INCLUDEPATH += ../../packages/ant-design-qt/src
INCLUDEPATH += ../../packages/ant-design-icons-qt/src

HEADERS += \
    alert_docs_page.h \
    color_picker_docs_page.h \
    icon_theme_adapter.h \
    image_docs_page.h \
    input_docs_page.h \
    input_number_docs_page.h \
    menu_docs_page.h \
    modal_docs_page.h \
    popconfirm_docs_page.h \
    popover_docs_page.h \
    radio_docs_page.h \
    select_docs_page.h \
    slider_docs_page.h \
    switch_docs_page.h \
    tooltip_docs_page.h

SOURCES += \
    alert_docs_page.cpp \
    color_picker_docs_page.cpp \
    icon_theme_adapter.cpp \
    image_docs_page.cpp \
    input_docs_page.cpp \
    input_number_docs_page.cpp \
    main.cpp \
    menu_docs_page.cpp \
    modal_docs_page.cpp \
    popconfirm_docs_page.cpp \
    popover_docs_page.cpp \
    radio_docs_page.cpp \
    select_docs_page.cpp \
    slider_docs_page.cpp \
    switch_docs_page.cpp \
    tooltip_docs_page.cpp

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
