QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += concurrent
}

HEADERS += \
    AudioDeviceMonitor.h

SOURCES += \
        main.cpp \
        AudioDeviceMonitor.cpp

LIBS += -lole32
