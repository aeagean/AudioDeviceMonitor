/**********************************************************
 * Author: Qt君
 * 微信公众号: Qt君(首发)
 * Email:  2088201923@qq.com
 * QQ交流群: 732271126
 * LISCENSE: MIT
 ************************************************************/
#include <QObject>

class IMMDevice;
class IMMDeviceEnumerator;
class IAudioEndpointVolume;

class DeviceNotificationClient;
class AudioEndpointVolumeCallback;

namespace QtHub
{
class AudioDeviceMonitor : public QObject
{
    Q_OBJECT
public:
    AudioDeviceMonitor();
    virtual ~AudioDeviceMonitor();

    static AudioDeviceMonitor *createAudioDeviceMonitor();

signals:
    void deviceStateChanged();
    void volumeStateChanged();

private:
    DeviceNotificationClient    *m_deviceNotificationClient;
    AudioEndpointVolumeCallback *m_audioEndpointVolumeCallback;

    IMMDevice            *m_device;
    IAudioEndpointVolume *m_endpoint;
    IMMDeviceEnumerator  *m_enumerator;
};
};
