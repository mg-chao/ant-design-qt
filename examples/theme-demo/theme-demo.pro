QT += core gui widgets
CONFIG += c++17
TEMPLATE = app
TARGET = theme-demo

INCLUDEPATH += ../../src

HEADERS += \
    menu_docs_page.h \
    ../../src/widgets/button.h \
    ../../src/widgets/button_group.h \
    ../../src/widgets/button_style.h \
    ../../src/widgets/interaction_overlay_manager.h \
    ../../src/widgets/menu.h \
    ../../src/widgets/menu_style.h \
    ../../src/widgets/widgets.h \
    ../../src/theme/fast_color_lite.h \
    ../../src/theme/palette_generate.h \
    ../../src/theme/theme.h \
    ../../src/theme/theme_algorithms.h \
    ../../src/theme/theme_manager.h \
    ../../src/theme/theme_palette.h \
    ../../src/theme/theme_types.h

SOURCES += \
    main.cpp \
    menu_docs_page.cpp \
    ../../src/widgets/button.cpp \
    ../../src/widgets/button_group.cpp \
    ../../src/widgets/button_style.cpp \
    ../../src/widgets/interaction_overlay_manager.cpp \
    ../../src/widgets/menu.cpp \
    ../../src/widgets/menu_style.cpp \
    ../../src/theme/fast_color_lite.cpp \
    ../../src/theme/palette_generate.cpp \
    ../../src/theme/theme_algorithms.cpp \
    ../../src/theme/theme_manager.cpp \
    ../../src/theme/theme_palette.cpp \
    ../../src/theme/theme_types.cpp
