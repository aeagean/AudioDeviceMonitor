#include "AudioDeviceMonitor.h"

#if (QT_VERSION <= QT_VERSION_CHECK(5, 0, 0))
    #include <QtConcurrentRun>
#else
    #include <QtConcurrent>
#endif
#include <QDebug>

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>

#define SAFE_RELEASE(x) if ((x) != NULL) { (x)->Release(); (x) = NULL; }

using namespace QtHub;

/*
 * 当音频端点设备的状态更改时，
 * 则MMDevice模块调用这些方法来通知客户端。
 */
class DeviceNotificationClient : public IMMNotificationClient
{
public:
    explicit DeviceNotificationClient();
    virtual ~DeviceNotificationClient();

    /* 注册外部监听器用于转发设备状态改变的回调 */
    void setListener(AudioDeviceMonitor *listener);

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);

    /* 设备事件通知的回调方法 */
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow,
                                                     ERole role,
                                                     LPCWSTR pwstrDeviceId);

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);

    /* 用于监听音频设备状态是否改变(设备插入/拔出) */
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId,
                                                   DWORD dwNewState);

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId,
                                                     const PROPERTYKEY key);

private:
    LONG                  m_ref;
    AudioDeviceMonitor   *m_listener;
};

DeviceNotificationClient::DeviceNotificationClient() :
    m_ref(1),
    m_listener(NULL)
{
}

DeviceNotificationClient::~DeviceNotificationClient()
{
}

void DeviceNotificationClient::setListener(AudioDeviceMonitor *listener)
{
    m_listener = listener;
}

ULONG DeviceNotificationClient::AddRef()
{
    return InterlockedIncrement(&m_ref);
}

ULONG DeviceNotificationClient::Release()
{
    ULONG _ref = InterlockedDecrement(&m_ref);
    if (_ref == 0) {
        delete this;
    }
    return _ref;
}

HRESULT DeviceNotificationClient::QueryInterface(const IID &riid, void **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid) {
        AddRef();
        *ppvInterface = (IMMNotificationClient*)this;
    }
    else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

HRESULT DeviceNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
{
    Q_UNUSED(flow)
    Q_UNUSED(role)
    Q_UNUSED(pwstrDeviceId)

    return S_OK;
}

HRESULT DeviceNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    Q_UNUSED(pwstrDeviceId)
    return S_OK;
}

HRESULT DeviceNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    Q_UNUSED(pwstrDeviceId)
    return S_OK;
}

HRESULT DeviceNotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
    Q_UNUSED(pwstrDeviceId)
    Q_UNUSED(dwNewState)

    if (m_listener) {
        m_listener->deviceStateChanged();
    }

    return S_OK;
}

HRESULT DeviceNotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
    Q_UNUSED(pwstrDeviceId)
    Q_UNUSED(key)
    return S_OK;
}

class AudioEndpointVolumeCallback : public IAudioEndpointVolumeCallback
{
public:
    explicit AudioEndpointVolumeCallback() : m_ref(1) { }
    virtual ~AudioEndpointVolumeCallback() { }

    /* 注册外部监听器用于转发设备状态改变的回调 */
    void setListener(AudioDeviceMonitor *listener)
    {
        m_listener = listener;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement(&m_ref);
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        ULONG ulRef = InterlockedDecrement(&m_ref);
        if (0 == ulRef)
        {
            delete this;
        }
        return ulRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface)
    {
        if (IID_IUnknown == riid)
        {
            AddRef();
            *ppvInterface = (IUnknown*)this;
        }
        else if (__uuidof(IAudioEndpointVolumeCallback) == riid)
        {
            AddRef();
            *ppvInterface = (IAudioEndpointVolumeCallback*)this;
        }
        else
        {
            *ppvInterface = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify)
    {
        Q_UNUSED(pNotify)
        if (m_listener) {
            m_listener->volumeStateChanged();
        }

        return S_OK;
    }

private:
    LONG                m_ref;
    AudioDeviceMonitor *m_listener;
};

AudioDeviceMonitor::AudioDeviceMonitor() :
    m_deviceNotificationClient(NULL),
    m_audioEndpointVolumeCallback(NULL),
    m_device(NULL),
    m_endpoint(NULL),
    m_enumerator(NULL)
{
    CoInitialize(NULL);

    if ( CoCreateInstance(__uuidof(MMDeviceEnumerator),
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(IMMDeviceEnumerator),
                          (void**)&m_enumerator) != S_OK )
    {
        goto Exit;
    }

    if ( m_enumerator->GetDefaultAudioEndpoint(eRender,
                                             eConsole,
                                             &m_device) != S_OK )
    {
        goto Exit;
    }

    if ( m_device->Activate(__uuidof(IAudioEndpointVolume),
                          CLSCTX_INPROC_SERVER,
                          NULL,
                          (void **)&m_endpoint) != S_OK )
    {
        goto Exit;
    }

    m_deviceNotificationClient = new DeviceNotificationClient();
    m_deviceNotificationClient->setListener(this);
    m_enumerator->RegisterEndpointNotificationCallback(m_deviceNotificationClient);

    m_audioEndpointVolumeCallback = new AudioEndpointVolumeCallback();
    m_audioEndpointVolumeCallback->setListener(this);
    m_endpoint->RegisterControlChangeNotify(m_audioEndpointVolumeCallback);

    return;

    Exit:
        SAFE_RELEASE(m_enumerator)
        SAFE_RELEASE(m_device)
        SAFE_RELEASE(m_endpoint)
        CoUninitialize();
}

AudioDeviceMonitor::~AudioDeviceMonitor()
{
    if (m_deviceNotificationClient) {
        delete m_deviceNotificationClient;
        m_deviceNotificationClient = NULL;
    }

    if (m_audioEndpointVolumeCallback) {
        delete m_audioEndpointVolumeCallback;
        m_audioEndpointVolumeCallback = NULL;
    }

    SAFE_RELEASE(m_device)
    SAFE_RELEASE(m_endpoint)
    SAFE_RELEASE(m_enumerator)
    CoUninitialize();
}

static AudioDeviceMonitor *_createAudioDeviceMonitor()
{
    return new AudioDeviceMonitor();
}

AudioDeviceMonitor *AudioDeviceMonitor::createAudioDeviceMonitor()
{
    /* 用于创建独立线程资源 */
    QFuture<AudioDeviceMonitor *> future = QtConcurrent::run(_createAudioDeviceMonitor);
    return future.result();
}
