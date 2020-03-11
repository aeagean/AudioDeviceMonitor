#********************************************************
# Author: Qt君
# 微信公众号: Qt君(首发)
# Email:  2088201923@qq.com
# QQ交流群: 732271126
# LISCENSE: MIT
#***********************************************************
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
