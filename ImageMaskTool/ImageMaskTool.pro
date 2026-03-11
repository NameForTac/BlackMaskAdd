QT       += core gui widgets concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = ImageMaskTool
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/ImageProcessor.cpp \
    src/BlpReader.cpp

HEADERS += \
    include/MainWindow.h \
    include/ImageProcessor.h \
    include/BlpReader.h

INCLUDEPATH += include

# Default output directory
DESTDIR = bin
