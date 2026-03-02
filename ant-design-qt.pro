TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    ant_design_qt \
    ant_design_icons_qt \
    theme_demo

ant_design_qt.subdir = packages/ant-design-qt
ant_design_icons_qt.subdir = packages/ant-design-icons-qt
theme_demo.subdir = examples/theme-demo
theme_demo.depends = ant_design_qt ant_design_icons_qt
