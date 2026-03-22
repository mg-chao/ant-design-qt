TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    adqt_icon_core \
    ant_design_icons_qt \
    ant_design_qt \
    ant_design_qt_tests \
    theme_demo

adqt_icon_core.subdir = packages/adqt-icon-core
ant_design_icons_qt.subdir = packages/ant-design-icons-qt
ant_design_icons_qt.depends = adqt_icon_core
ant_design_qt.subdir = packages/ant-design-qt
ant_design_qt.depends = adqt_icon_core ant_design_icons_qt
ant_design_qt_tests.file = packages/ant-design-qt/tests/ant-design-qt-tests.pro
ant_design_qt_tests.depends = adqt_icon_core ant_design_qt ant_design_icons_qt
theme_demo.subdir = examples/theme-demo
theme_demo.depends = adqt_icon_core ant_design_qt ant_design_icons_qt
