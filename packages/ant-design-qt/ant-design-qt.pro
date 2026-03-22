TEMPLATE = lib
TARGET = ant-design-qt

QT += core gui widgets network
CONFIG += c++17
CONFIG += staticlib

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$clean_path($$PWD/../adqt-icon-core/src)
INCLUDEPATH += $$clean_path($$PWD/../ant-design-icons-qt/src)

HEADERS += \
    src/widgets/abstract_select_widget.h \
    src/widgets/alert.h \
    src/widgets/alert_style.h \
    src/widgets/button.h \
    src/widgets/button_style.h \
    src/widgets/combo_box.h \
    src/widgets/color_picker.h \
    src/widgets/color_picker_style.h \
    src/widgets/color_selection.h \
    src/widgets/date_picker.h \
    src/widgets/detail/button_grouping.h \
    src/widgets/detail/color_picker_value_model.h \
    src/widgets/detail/flow_layout.h \
    src/widgets/detail/overlay_accessibility.h \
    src/widgets/detail/overlay_popup_controller.h \
    src/widgets/detail/overlay_popup_surface.h \
    src/widgets/detail/navigation_menu_popup_state.h \
    src/widgets/detail/navigation_menu_state.h \
    src/widgets/detail/navigation_menu_view_state.h \
    src/widgets/detail/select_models.h \
    src/widgets/detail/select_option_utils.h \
    src/widgets/detail/select_selection_controller.h \
    src/widgets/detail/animated_scalar.h \
    src/widgets/detail/themed_scrollbar.h \
    src/widgets/detail/timing_hub.h \
    src/widgets/image.h \
    src/widgets/image_style.h \
    src/widgets/field_group.h \
    src/widgets/input.h \
    src/widgets/input_internal.h \
    src/widgets/input_line_edit.h \
    src/widgets/input_number.h \
    src/widgets/input_number_value_model.h \
    src/widgets/input_number_style.h \
    src/widgets/input_policies.h \
    src/widgets/input_otp_edit.h \
    src/widgets/input_password_edit.h \
    src/widgets/input_search_edit.h \
    src/widgets/input_text_edit.h \
    src/widgets/input_style.h \
    src/widgets/in_window_popup_host.h \
    src/widgets/interaction_overlay_manager.h \
    src/widgets/modal.h \
    src/widgets/multi_select.h \
    src/widgets/navigation_menu.h \
    src/widgets/menu_style.h \
    src/widgets/popover.h \
    src/widgets/popconfirm.h \
    src/widgets/popover_style.h \
    src/widgets/popup_types.h \
    src/widgets/popup_placement.h \
    src/widgets/radio.h \
    src/widgets/radio_button_group.h \
    src/widgets/radio_style.h \
    src/widgets/scroll_area.h \
    src/widgets/select.h \
    src/widgets/select_types.h \
    src/widgets/tag_select.h \
    src/widgets/tag.h \
    src/widgets/tag_group.h \
    src/widgets/tag_style.h \
    src/widgets/select_style.h \
    src/widgets/slider.h \
    src/widgets/slider_style.h \
    src/widgets/switch.h \
    src/widgets/switch_style.h \
    src/widgets/tooltip.h \
    src/widgets/tooltip_style.h \
    src/widgets/widgets.h \
    src/theme/fast_color_lite.h \
    src/theme/palette_generate.h \
    src/theme/theme_color_utils.h \
    src/theme/theme.h \
    src/theme/theme_manager.h \
    src/theme/theme_palette.h \
    src/theme/theme_types.h

SOURCES += \
    src/placeholder.cpp \
    src/widgets/abstract_select_widget.cpp \
    src/widgets/alert.cpp \
    src/widgets/alert_style.cpp \
    src/widgets/button.cpp \
    src/widgets/button_style.cpp \
    src/widgets/combo_box.cpp \
    src/widgets/color_picker.cpp \
    src/widgets/color_picker_style.cpp \
    src/widgets/date_picker.cpp \
    src/widgets/detail/color_picker_value_model.cpp \
    src/widgets/detail/flow_layout.cpp \
    src/widgets/detail/overlay_accessibility.cpp \
    src/widgets/detail/overlay_popup_controller.cpp \
    src/widgets/detail/overlay_popup_surface.cpp \
    src/widgets/detail/navigation_menu_state.cpp \
    src/widgets/detail/select_models.cpp \
    src/widgets/detail/select_option_utils.cpp \
    src/widgets/detail/select_selection_controller.cpp \
    src/widgets/detail/themed_scrollbar.cpp \
    src/widgets/detail/timing_hub.cpp \
    src/widgets/image.cpp \
    src/widgets/image_style.cpp \
    src/widgets/field_group.cpp \
    src/widgets/input_internal.cpp \
    src/widgets/input_line_edit.cpp \
    src/widgets/input_otp_edit.cpp \
    src/widgets/input_password_edit.cpp \
    src/widgets/input_search_edit.cpp \
    src/widgets/input_text_edit.cpp \
    src/widgets/input_number.cpp \
    src/widgets/input_number_value_model.cpp \
    src/widgets/input_number_style.cpp \
    src/widgets/input_style.cpp \
    src/widgets/in_window_popup_host.cpp \
    src/widgets/interaction_overlay_manager.cpp \
    src/widgets/modal.cpp \
    src/widgets/multi_select.cpp \
    src/widgets/navigation_menu.cpp \
    src/widgets/menu_style.cpp \
    src/widgets/popover.cpp \
    src/widgets/popconfirm.cpp \
    src/widgets/popover_style.cpp \
    src/widgets/popup_placement.cpp \
    src/widgets/radio.cpp \
    src/widgets/radio_button_group.cpp \
    src/widgets/radio_style.cpp \
    src/widgets/scroll_area.cpp \
    src/widgets/select.cpp \
    src/widgets/tag_select.cpp \
    src/widgets/tag.cpp \
    src/widgets/tag_group.cpp \
    src/widgets/tag_style.cpp \
    src/widgets/select_style.cpp \
    src/widgets/slider.cpp \
    src/widgets/slider_style.cpp \
    src/widgets/switch.cpp \
    src/widgets/switch_style.cpp \
    src/widgets/tooltip.cpp \
    src/widgets/tooltip_style.cpp \
    src/theme/fast_color_lite.cpp \
    src/theme/palette_generate.cpp \
    src/theme/theme_color_utils.cpp \
    src/theme/theme_manager.cpp \
    src/theme/theme_palette.cpp \
    src/theme/theme_types.cpp
