TEMPLATE = lib
TARGET = ant-design-qt

QT += core gui widgets
CONFIG += c++17
CONFIG += staticlib

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$clean_path($$PWD/../ant-design-icons-qt/src)

HEADERS += \
    src/widgets/button.h \
    src/widgets/button_group.h \
    src/widgets/button_style.h \
    src/widgets/interaction_overlay_manager.h \
    src/widgets/menu.h \
    src/widgets/menu_style.h \
    src/widgets/select.h \
    src/widgets/select_style.h \
    src/widgets/widgets.h \
    src/theme/fast_color_lite.h \
    src/theme/palette_generate.h \
    src/theme/theme.h \
    src/theme/theme_algorithms.h \
    src/theme/theme_manager.h \
    src/theme/theme_palette.h \
    src/theme/theme_types.h

SOURCES += \
    src/placeholder.cpp \
    src/widgets/button.cpp \
    src/widgets/button_group.cpp \
    src/widgets/button_style.cpp \
    src/widgets/interaction_overlay_manager.cpp \
    src/widgets/menu.cpp \
    src/widgets/menu_style.cpp \
    src/widgets/select.cpp \
    src/widgets/select_style.cpp \
    src/theme/fast_color_lite.cpp \
    src/theme/palette_generate.cpp \
    src/theme/theme_algorithms.cpp \
    src/theme/theme_manager.cpp \
    src/theme/theme_palette.cpp \
    src/theme/theme_types.cpp
