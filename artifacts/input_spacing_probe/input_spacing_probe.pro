QT += core gui widgets svg
CONFIG += c++17 release
CONFIG -= app_bundle
TEMPLATE = app
TARGET = input-spacing-probe

INCLUDEPATH += ../../packages/ant-design-qt/src
INCLUDEPATH += ../../packages/ant-design-icons-qt/src

win32-g++ {
    PRE_TARGETDEPS += ../../packages/ant-design-qt/release/libant-design-qt.a
    PRE_TARGETDEPS += ../../packages/ant-design-icons-qt/release/libant-design-icons-qt.a
    LIBS += -L../../packages/ant-design-qt/release -lant-design-qt
    LIBS += -L../../packages/ant-design-icons-qt/release -lant-design-icons-qt
}

SOURCES += input_spacing_probe.cpp
