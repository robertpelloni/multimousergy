#include "WebcamCapture.hpp"
#include <iostream>

#ifdef _WIN32
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#endif

WebcamCapture::WebcamCapture() {
#ifdef _WIN32
    m_device = nullptr;
    m_reader = nullptr;
    m_currentFrame = nullptr;
#endif
}

WebcamCapture::~WebcamCapture() {
#ifdef _WIN32
    if (m_reader) m_reader->Release();
    if (m_currentFrame) m_currentFrame->Release();
#endif
}

#ifdef _WIN32
bool WebcamCapture::Initialize(ID3D11Device* device) {
    m_device = device;
    std::cout << "[Webcam] Initializing Media Foundation capture..." << std::endl;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    IMFAttributes* attributes = nullptr;
    hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) return false;

    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) { attributes->Release(); return false; }

    IMFActivate** devices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(attributes, &devices, &count);
    attributes->Release();

    if (FAILED(hr) || count == 0) return false;

    IMFMediaSource* source = nullptr;
    hr = devices[0]->ActivateObject(__uuidof(IMFMediaSource), (void**)&source);
    for (UINT32 i = 0; i < count; i++) devices[i]->Release();
    CoTaskMemFree(devices);

    if (FAILED(hr)) return false;

    hr = MFCreateSourceReaderFromMediaSource(source, NULL, &m_reader);
    source->Release();
    if (FAILED(hr)) return false;

    return true;
}

bool WebcamCapture::AcquireFrame() {
    return true;
}

void WebcamCapture::ReleaseFrame() {
}
#endif
