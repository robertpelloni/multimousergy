#include "WebcamCapture.hpp"
#include <iostream>

WebcamCapture::WebcamCapture()
    : m_isCapturing(false) {
    m_activeDevice = {-1, "None", ""};
}

WebcamCapture::~WebcamCapture() {
    Shutdown();
}

void WebcamCapture::Shutdown() {
#ifdef _WIN32
    if (m_sourceReader) {
        m_sourceReader->Release();
        m_sourceReader = nullptr;
    }
    if (m_mediaSource) {
        m_mediaSource->Shutdown();
        m_mediaSource->Release();
        m_mediaSource = nullptr;
    }
    MFShutdown();
#endif
    m_isCapturing = false;
    m_activeDevice = {-1, "None", ""};
}

bool WebcamCapture::IsCapturing() const {
    return m_isCapturing;
}

WebcamDeviceInfo WebcamCapture::GetCurrentDeviceInfo() const {
    return m_activeDevice;
}

std::vector<WebcamDeviceInfo> WebcamCapture::EnumerateDevices() {
    m_deviceList.clear();

#ifdef _WIN32
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cerr << "[WebcamCapture] Failed to initialize Media Foundation for enumeration: " << hr << std::endl;
        return m_deviceList;
    }

    IMFAttributes* pAttributes = nullptr;
    hr = MFCreateAttributes(&pAttributes, 1);
    if (SUCCEEDED(hr)) {
        hr = pAttributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );

        if (SUCCEEDED(hr)) {
            IMFActivate** ppDevices = nullptr;
            UINT32 count = 0;
            hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);

            if (SUCCEEDED(hr) && count > 0) {
                for (UINT32 i = 0; i < count; ++i) {
                    WCHAR* szFriendlyName = nullptr;
                    UINT32 cchName = 0;
                    WCHAR* szEndpointId = nullptr;
                    UINT32 cchEndpoint = 0;

                    std::string friendlyNameStr = "Generic Video Source " + std::to_string(i);
                    std::string endpointStr = "";

                    if (SUCCEEDED(ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchName)) && szFriendlyName) {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, szFriendlyName, (int)cchName, NULL, 0, NULL, NULL);
                        friendlyNameStr.resize(size_needed);
                        WideCharToMultiByte(CP_UTF8, 0, szFriendlyName, (int)cchName, &friendlyNameStr[0], size_needed, NULL, NULL);
                        CoTaskMemFree(szFriendlyName);
                    }

                    if (SUCCEEDED(ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &szEndpointId, &cchEndpoint)) && szEndpointId) {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, szEndpointId, (int)cchEndpoint, NULL, 0, NULL, NULL);
                        endpointStr.resize(size_needed);
                        WideCharToMultiByte(CP_UTF8, 0, szEndpointId, (int)cchEndpoint, &endpointStr[0], size_needed, NULL, NULL);
                        CoTaskMemFree(szEndpointId);
                    }

                    m_deviceList.push_back({(int)i, friendlyNameStr, endpointStr});
                    ppDevices[i]->Release();
                }
                CoTaskMemFree(ppDevices);
            }
        }
        pAttributes->Release();
    }
#else
    // Non-Windows stub: provide a mock virtual device for CI testing and cross-platform verification
    m_deviceList.push_back({0, "Virtual Video Loopback", "/dev/video0"});
#endif

    std::cout << "[WebcamCapture] Enumerated " << m_deviceList.size() << " video capture devices." << std::endl;
    return m_deviceList;
}

bool WebcamCapture::Initialize(int deviceIndex) {
    std::cout << "[WebcamCapture] Initializing video capture (device " << deviceIndex << ")..." << std::endl;
    Shutdown();

    std::vector<WebcamDeviceInfo> devices = EnumerateDevices();
    if (devices.empty()) {
        std::cerr << "[WebcamCapture] No video capture devices available." << std::endl;
        return false;
    }

    if (deviceIndex < 0 || deviceIndex >= (int)devices.size()) {
        deviceIndex = 0;
    }

#ifdef _WIN32
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    IMFAttributes* pAttributes = nullptr;
    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) return false;

    pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );

    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    pAttributes->Release();

    if (FAILED(hr) || count == 0 || (UINT32)deviceIndex >= count) {
        if (ppDevices) {
            for (UINT32 i = 0; i < count; ++i) if (ppDevices[i]) ppDevices[i]->Release();
            CoTaskMemFree(ppDevices);
        }
        return false;
    }

    // Activate the requested device
    hr = ppDevices[deviceIndex]->ActivateObject(IID_PPV_ARGS(&m_mediaSource));
    for (UINT32 i = 0; i < count; ++i) {
        if (ppDevices[i]) ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);

    if (FAILED(hr) || !m_mediaSource) {
        std::cerr << "[WebcamCapture] Failed to activate IMFMediaSource: " << hr << std::endl;
        return false;
    }

    // Create SourceReader for fast frame polling
    hr = MFCreateSourceReaderFromMediaSource(m_mediaSource, nullptr, &m_sourceReader);
    if (FAILED(hr)) {
        std::cerr << "[WebcamCapture] Failed to create IMFSourceReader: " << hr << std::endl;
        m_mediaSource->Release();
        m_mediaSource = nullptr;
        return false;
    }

    // Configure preferred output type (e.g. RGB32 or NV12 for WebRTC)
    IMFMediaType* pMediaType = nullptr;
    hr = MFCreateMediaType(&pMediaType);
    if (SUCCEEDED(hr)) {
        pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        m_sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pMediaType);
        pMediaType->Release();
    }
#endif

    m_activeDevice = devices[deviceIndex];
    m_isCapturing = true;
    std::cout << "[WebcamCapture] Successfully activated video source: " << m_activeDevice.name << std::endl;
    return true;
}

void WebcamCapture::Update() {
    if (!m_isCapturing) return;

#ifdef _WIN32
    if (!m_sourceReader) return;

    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    IMFSample* sample = nullptr;

    HRESULT hr = m_sourceReader->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0, &streamIndex, &flags, &timestamp, &sample
    );

    if (SUCCEEDED(hr) && sample) {
        // Frame received - ready for passing to WebRTC video track
        sample->Release();
    }
#endif
}
