TEMPLATE = lib
TARGET = ant-design-qt

QT += core gui widgets network
CONFIG += c++17
CONFIG += staticlib

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$clean_path($$PWD/../ant-design-icons-qt/src)

HEADERS += \
    src/widgets/alert.h \
    src/widgets/alert_style.h \
    src/widgets/button.h \
    src/widgets/button_group.h \
    src/widgets/button_style.h \
    src/widgets/color_picker.h \
    src/widgets/color_picker_style.h \
    src/widgets/detail/icon_utils.h \
    src/widgets/detail/timing_hub.h \
    src/widgets/image.h \
    src/widgets/image_style.h \
    src/widgets/input.h \
    src/widgets/input_number.h \
    src/widgets/input_number_style.h \
    src/widgets/input_style.h \
    src/widgets/in_window_popup_host.h \
    src/widgets/interaction_overlay_manager.h \
    src/widgets/modal.h \
    src/widgets/menu.h \
    src/widgets/menu_style.h \
    src/widgets/popover.h \
    src/widgets/popconfirm.h \
    src/widgets/popover_style.h \
    src/widgets/popup_placement.h \
    src/widgets/radio.h \
    src/widgets/radio_group.h \
    src/widgets/radio_style.h \
    src/widgets/scroll_area.h \
    src/widgets/select.h \
    src/widgets/select_style.h \
    src/widgets/slider.h \
    src/widgets/slider_style.h \
    src/widgets/switch.h \
    src/widgets/switch_style.h \
    src/widgets/tooltip.h \
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
    src/widgets/alert.cpp \
    src/widgets/alert_style.cpp \
    src/widgets/button.cpp \
    src/widgets/button_group.cpp \
    src/widgets/button_style.cpp \
    src/widgets/color_picker.cpp \
    src/widgets/color_picker_style.cpp \
    src/widgets/detail/icon_utils.cpp \
    src/widgets/detail/timing_hub.cpp \
    src/widgets/image.cpp \
    src/widgets/image_style.cpp \
    src/widgets/input.cpp \
    src/widgets/input_number.cpp \
    src/widgets/input_number_style.cpp \
    src/widgets/input_style.cpp \
    src/widgets/in_window_popup_host.cpp \
    src/widgets/interaction_overlay_manager.cpp \
    src/widgets/modal.cpp \
    src/widgets/menu.cpp \
    src/widgets/menu_style.cpp \
    src/widgets/popover.cpp \
    src/widgets/popconfirm.cpp \
    src/widgets/popover_style.cpp \
    src/widgets/popup_placement.cpp \
    src/widgets/radio.cpp \
    src/widgets/radio_group.cpp \
    src/widgets/radio_style.cpp \
    src/widgets/scroll_area.cpp \
    src/widgets/select.cpp \
    src/widgets/select_style.cpp \
    src/widgets/slider.cpp \
    src/widgets/slider_style.cpp \
    src/widgets/switch.cpp \
    src/widgets/switch_style.cpp \
    src/widgets/tooltip.cpp \
    src/theme/fast_color_lite.cpp \
    src/theme/palette_generate.cpp \
    src/theme/theme_algorithms.cpp \
    src/theme/theme_manager.cpp \
    src/theme/theme_palette.cpp \
    src/theme/theme_types.cpp
