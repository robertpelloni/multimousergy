#include "WebcamCapture.hpp"
#include <iostream>

WebcamCapture::WebcamCapture() {
}

WebcamCapture::~WebcamCapture() {
#ifdef _WIN32
    if (m_sourceReader) m_sourceReader->Release();
    if (m_mediaSource) m_mediaSource->Release();
    MFShutdown();
#endif
}

bool WebcamCapture::Initialize() {
    std::cout << "[WebcamCapture] Initializing Windows Media Foundation capture..." << std::endl;
#ifdef _WIN32
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    // A real implementation would enumerate devices here and select one.
    // We are putting up a stub for the architecture.
#endif
    return true;
}

void WebcamCapture::Update() {
#ifdef _WIN32
    if (!m_sourceReader) return;

    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    IMFSample* sample = nullptr;

    // Read a frame from the webcam
    HRESULT hr = m_sourceReader->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0, &streamIndex, &flags, &timestamp, &sample
    );

    if (SUCCEEDED(hr) && sample) {
        // Here we would process the sample (e.g., pass it to WebRTC)
        sample->Release();
    }
#endif
}
