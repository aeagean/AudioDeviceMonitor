/**********************************************************
 * Author: Qt君
 * 微信公众号: Qt君(首发)
 * Email:  2088201923@qq.com
 * QQ交流群: 732271126
 * LISCENSE: MIT
 ************************************************************/
#include "AudioDeviceMonitor.h"

#include <QCoreApplication>
#include <QDebug>

using namespace QtHub;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    AudioDeviceMonitor monitor;
    QObject::connect(&monitor,
                     &AudioDeviceMonitor::volumeStateChanged,
                     [](){ qDebug() << "Volume changed!"; });

    QObject::connect(&monitor,
                     &AudioDeviceMonitor::deviceStateChanged,
                     [](){ qDebug() << "Device status changed!"; });

    return a.exec();
}
