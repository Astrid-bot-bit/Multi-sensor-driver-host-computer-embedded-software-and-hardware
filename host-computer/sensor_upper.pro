QT += core gui widgets serialport charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
INCLUDEPATH += $$[QT_INSTALL_HEADERS]

TARGET   = SensorUpper
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    scrollablechartview.cpp \
    sensorprotocol.cpp \

HEADERS += \
    mainwindow.h \
    scrollablechartview.h \
    sensorprotocol.h \
    scrollablechartview.h
